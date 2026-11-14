#include "note_scan.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <format>
#include <map>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>

#include "log.hpp"

namespace {

constexpr std::size_t kObjSize = 0x100;     // オブジェクトの先頭 0x100 バイトを見る
constexpr std::size_t kPosOff = 0x08;       // 座標(int3つ)
constexpr std::size_t kTypeOff = 0x14;      // ブロックエンティティの種類番号
constexpr std::size_t kNoteOff = 0xF8;      // 音程
constexpr std::uint8_t kNoteBlockType = 0x0B; // 音ブロックの種類番号(実機で確認)
constexpr std::size_t kChunk = 1 << 20;     // 1MBずつ読む
constexpr bool kLogObjects = true;          // 見つけた物を1つずつログに出す

std::atomic_bool g_running{false};
std::atomic<std::uintptr_t> g_vtable{0}; // 見つけた(または、サーバー側のフックで覚えた)vtable
std::mutex g_mutex;
std::vector<NoteBlockInfo> g_results;

bool read_mem(std::uintptr_t src, void *dst, std::size_t size) {
  SIZE_T n = 0;
  return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void *>(src), dst, size,
                           &n) &&
         n == size;
}

bool is_scannable(const MEMORY_BASIC_INFORMATION &mbi) {
  return mbi.State == MEM_COMMIT && mbi.Type == MEM_PRIVATE &&
         !(mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
         (mbi.Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)) != 0;
}

void scan_thread() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto img_end = base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);

  SYSTEM_INFO si{};
  GetSystemInfo(&si);
  auto addr = reinterpret_cast<std::uintptr_t>(si.lpMinimumApplicationAddress);
  const auto end = reinterpret_cast<std::uintptr_t>(si.lpMaximumApplicationAddress);

  std::vector<std::uint8_t> buf(kChunk + kObjSize); // 次の塊にまたがる分を重ねて読む
  const auto buf_begin = reinterpret_cast<std::uintptr_t>(buf.data());
  const auto buf_end = buf_begin + buf.size();

  struct Hit {
    NoteBlockInfo info;
    std::uintptr_t vtable;
  };
  std::vector<Hit> hits;
  const auto t0 = GetTickCount64();

  while (addr < end) {
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(reinterpret_cast<void *>(addr), &mbi, sizeof(mbi))) break;

    const auto region_begin = reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
    const auto region_end = region_begin + mbi.RegionSize;

    if (is_scannable(mbi)) {
      for (auto p = region_begin; p < region_end; p += kChunk) {
        const std::size_t want = std::min<std::uintptr_t>(kChunk + kObjSize, region_end - p);
        if (want < kObjSize || !read_mem(p, buf.data(), want)) continue;

        for (std::size_t off = 0; off + kObjSize <= want && off < kChunk; off += 8) {
          const std::uint8_t *o = buf.data() + off;
          if (o[kTypeOff] != kNoteBlockType) continue; // いちばん安い判定を先に

          std::uint64_t vt = 0;
          std::memcpy(&vt, o, sizeof(vt));
          if (vt < base || vt >= img_end) continue; // vtable はゲーム本体の中にあるはず

          std::int32_t pos[3];
          std::memcpy(pos, o + kPosOff, sizeof(pos));
          const std::uint8_t note = o[kNoteOff];
          if (note > 24 || pos[1] < -128 || pos[1] > 400) continue;
          if (std::abs(pos[0]) > 30'000'000 || std::abs(pos[2]) > 30'000'000) continue;

          const auto obj = p + off;
          if (obj >= buf_begin && obj < buf_end) continue; // 自分のバッファは除外
          hits.push_back({{obj, pos[0], pos[1], pos[2], note}, static_cast<std::uintptr_t>(vt)});
        }
      }
    }
    addr = region_end;
  }

  // 一番多いvtableを、音ブロックのvtableとして覚える(2個以上見つかったときだけ)
  std::uintptr_t pick = g_vtable.load();
  if (pick == 0) {
    std::map<std::uintptr_t, int> count;
    for (const auto &h : hits) ++count[h.vtable];
    int best = 0;
    for (const auto &[vt, c] : count) {
      if (c > best) { best = c; pick = vt; }
    }
    if (best >= 2) g_vtable = pick;
    else pick = 0;
  }

  std::vector<NoteBlockInfo> found;
  for (const auto &h : hits) {
    if (pick == 0 || h.vtable == pick) found.push_back(h.info);
  }
  std::ranges::sort(found, [](const NoteBlockInfo &a, const NoteBlockInfo &b) {
    return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
  });

  log(std::format("[mcnote] scan done: {} objects, {} ms, vtable +0x{:X}", found.size(),
                  GetTickCount64() - t0, pick ? pick - base : 0));
  if constexpr (kLogObjects) {
    for (const auto &n : found) {
      log(std::format("[mcnote]   obj=0x{:X} pos=({}, {}, {}) note={}", n.obj, n.x, n.y, n.z,
                      n.note));
    }
  }

  {
    std::lock_guard lock(g_mutex);
    g_results = std::move(found);
  }
  g_running = false;
}

} // namespace

void request_note_scan() {
  if (g_running.exchange(true)) return;
  std::thread(scan_thread).detach();
}

bool note_scan_running() { return g_running; }

std::vector<NoteBlockInfo> note_scan_results() {
  std::lock_guard lock(g_mutex);
  return g_results;
}

bool read_note_live(std::uintptr_t obj, std::uint8_t &note) {
  const std::uintptr_t vtable = g_vtable.load();
  if (vtable != 0) {
    std::uint64_t vt = 0;
    if (!read_mem(obj, &vt, sizeof(vt)) || vt != vtable) return false;
  }
  return read_mem(obj + kNoteOff, &note, 1);
}

void note_scan_learn_vtable(std::uintptr_t vtable) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto end = base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);
  if (vtable < base || vtable >= end) return; // ゲーム本体の外なら無視

  if (g_vtable.exchange(vtable) != vtable) {
    log(std::format("[mcnote] note block vtable learned: +0x{:X}", vtable - base));
  }
}

bool note_scan_vtable_known() { return g_vtable.load() != 0; }

bool note_scan_has_pos(std::int32_t x, std::int32_t y, std::int32_t z) {
  std::lock_guard lock(g_mutex);
  return std::any_of(g_results.begin(), g_results.end(), [&](const NoteBlockInfo &n) {
    return n.x == x && n.y == y && n.z == z;
  });
}