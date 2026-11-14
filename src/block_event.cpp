#include "block_event.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include <safetyhook.hpp>

#include "log.hpp"
#include "note_scan.hpp"
#include "note_table.hpp"
#include "sig_scan.hpp"

namespace {

using U64 = std::uint64_t;

constexpr int kBlockEventId = 0x1A; // BlockEventPacketの番号(実機で確認済み)
constexpr int kReadSlotRel =
    8; // getId のスロットから数えた、読み込み側の関数の位置

safetyhook::InlineHook g_hook;
std::atomic<int> g_logged{0};

bool safe_read(std::uintptr_t src, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(),
                           reinterpret_cast<const void *>(src), dst, n, &got) &&
         got == n;
}

// 読み取り専用のデータ領域から、target を値として持つ8バイトの場所を全部探す
std::vector<std::uintptr_t> find_pointers_to(std::uintptr_t target) {
  std::vector<std::uintptr_t> out;
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt =
      reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);

  std::vector<std::uint64_t> buf(1 << 17);
  for (WORD s = 0; s < nt->FileHeader.NumberOfSections; ++s) {
    const DWORD ch = sec[s].Characteristics;
    if (!(ch & IMAGE_SCN_MEM_READ) || (ch & IMAGE_SCN_MEM_EXECUTE) ||
        (ch & IMAGE_SCN_MEM_DISCARDABLE))
      continue;

    const auto begin = base + sec[s].VirtualAddress;
    const std::size_t size = sec[s].Misc.VirtualSize;
    for (std::size_t off = 0; off < size; off += buf.size() * 8) {
      const std::size_t bytes =
          std::min<std::size_t>(buf.size() * 8, size - off) & ~std::size_t{7};
      if (bytes == 0 || !safe_read(begin + off, buf.data(), bytes))
        continue;
      for (std::size_t i = 0; i < bytes / 8; ++i) {
        if (buf[i] == target)
          out.push_back(begin + off + i * 8);
      }
    }
  }
  return out;
}

// vtableの1つ前にあるRTTI情報から、クラスの名前(例
// ".?AVBlockEventPacket@@")を読む
std::string rtti_name(std::uintptr_t vtable_start) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt =
      reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto image_end =
      base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);

  std::uintptr_t col = 0;
  if (!safe_read(vtable_start - 8, &col, sizeof(col)) || col < base ||
      col >= image_end)
    return {};

  struct Col {
    std::uint32_t signature, offset, cd_offset;
    std::int32_t type_descriptor, class_hierarchy, self;
  } c{};
  if (!safe_read(col, &c, sizeof(c)) || c.signature != 1)
    return {};

  std::array<char, 96> name{};
  if (!safe_read(base + c.type_descriptor + 16, name.data(), name.size() - 1))
    return {};
  return std::string(name.data());
}

// パケットが読み込まれた後に呼ばれる:
// 座標・楽器・音程を読んで、音程の表に入れる
void on_packet(U64 self) {
  std::array<std::int32_t, 18> d{}; // 先頭 0x48 バイト
  if (!safe_read(self, d.data(), sizeof(d)))
    return;

  const int x = d[0x30 / 4], y = d[0x34 / 4], z = d[0x38 / 4];
  const int instrument = d[0x3C / 4];
  const int note = d[0x40 / 4];

  if (instrument < 0 || instrument > 15 || note < 0 || note > 24)
    return;
  if (!note_scan_has_pos(x, y, z))
    return; // スキャンした音ブロックの座標だけを信じる

  note_table_learn(x, y, z, static_cast<std::uint32_t>(instrument),
                   static_cast<std::uint32_t>(note));

  if (g_logged.fetch_add(1) < 200) {
    log(std::format("[mcnote] block event: ({}, {}, {}) instrument={} note={}",
                    x, y, z, instrument, note));
  }
}

// 6個の整数引数をそのまま元の関数へ渡して、戻ってきたあとに this の中身を見る
U64 hooked_read(U64 a, U64 b, U64 c, U64 d, U64 e, U64 f) {
  const U64 r = g_hook.stdcall<U64>(a, b, c, d, e, f);
  on_packet(a);
  return r;
}

} // namespace

void install_block_event_hook() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt =
      reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto image_end =
      base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);

  const auto fns =
      sig_find_all(std::format("B8 {:02X} 00 00 00 C3", kBlockEventId));
  log(std::format("[mcnote] block event: {} getId candidate(s)", fns.size()));

  std::uintptr_t chosen = 0;   // RTTIの名前が合ったもの
  std::uintptr_t fallback = 0; // 名前が読めなかったときの、最初の候補
  for (const auto fn : fns) {
    for (const auto slot : find_pointers_to(fn)) {
      const auto vtable = slot - 8; // getId は vtable の2番目(0始まりで1番)
      const std::string name = rtti_name(vtable);
      log(std::format("[mcnote]   candidate vtable +0x{:X} rtti='{}'",
                      vtable - base, name));

      std::uintptr_t read_fn = 0;
      if (!safe_read(slot + kReadSlotRel * 8, &read_fn, sizeof(read_fn)))
        continue;
      if (read_fn < base || read_fn >= image_end)
        continue;

      if (name.find("BlockEventPacket") != std::string::npos) {
        if (!chosen)
          chosen = read_fn;
      } else if (name.empty() && !fallback) {
        fallback = read_fn;
      }
    }
  }

  if (!chosen && fallback) {
    log("[mcnote] block event: RTTI name not readable; using the first "
        "candidate");
    chosen = fallback;
  }
  if (!chosen) {
    log("[mcnote] block event: BlockEventPacket not found");
    return;
  }

  DWORD64 image_base = 0;
  const RUNTIME_FUNCTION *rf =
      RtlLookupFunctionEntry(chosen, &image_base, nullptr);
  if (!rf || image_base + rf->BeginAddress != chosen ||
      rf->EndAddress - rf->BeginAddress < 16) {
    log("[mcnote] block event: read function is not a plain function");
    return;
  }

  auto hook = safetyhook::InlineHook::create(
      reinterpret_cast<void *>(chosen), reinterpret_cast<void *>(&hooked_read));
  if (!hook) {
    log("[mcnote] block event: failed to hook");
    return;
  }
  g_hook = std::move(*hook);
  log(std::format("[mcnote] block event hooked: +0x{:X} size=0x{:X}",
                  chosen - base, rf->EndAddress - rf->BeginAddress));
}