#include "packet_probe.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <safetyhook.hpp>

#include "log.hpp"
#include "sig_scan.hpp"
#include "thread_info.hpp"

namespace {

using U64 = std::uint64_t;

constexpr std::size_t kMaxEnter = 40; // 入口を見るだけのフックの最大数
constexpr std::size_t kMaxExit = 8;   // 関数が戻った後を見るフックの最大数
constexpr int kDumpInts = 32;

struct SlotInfo {
  int packet_id = 0;
  int rel = 0; // getId のスロットからの差(-1 = 1つ前、+1 = 1つ後ろ)
  std::uintptr_t rva = 0;
};

std::atomic_bool g_on{false};
std::mutex g_mutex;
std::map<std::size_t, int> g_enter_logged;
std::map<std::size_t, int> g_exit_logged;

std::array<SlotInfo, kMaxEnter> g_enter_info{};
std::vector<safetyhook::MidHook> g_enter_hooks;

std::array<SlotInfo, kMaxExit> g_exit_info{};
std::array<safetyhook::InlineHook, kMaxExit> g_exit_hooks;
std::size_t g_exit_count = 0;

bool safe_read(std::uintptr_t src, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void *>(src), dst, n,
                           &got) &&
         got == n;
}

std::string dump_ints(std::uintptr_t addr) {
  std::array<std::int32_t, kDumpInts> d{};
  if (!safe_read(addr, d.data(), sizeof(d))) return "(unreadable)";
  std::string s;
  for (const auto v : d) s += std::format("{} ", v);
  return s;
}

// 入口: パケットがまだ読み込まれる前かもしれない
void on_enter(std::size_t idx, SafetyHookContext &ctx) {
  if (!g_on.load(std::memory_order_relaxed)) return;
  {
    std::lock_guard lock(g_mutex);
    int &n = g_enter_logged[idx];
    if (n >= 4) return;
    ++n;
  }
  const auto &si = g_enter_info[idx];
  log(std::format("[mcnote] pkt id={} getId{:+d} (+0x{:X}) ENTER {} rcx->[{}]", si.packet_id,
                  si.rel, si.rva, thread_info(), dump_ints(ctx.rcx)));
}

// 出口: 関数が戻った後(読み込み済みのはず)。MAINスレッドのときだけ記録する
void on_exit(std::size_t e, U64 self) {
  if (!g_on.load(std::memory_order_relaxed) || !on_main_thread()) return;
  {
    std::lock_guard lock(g_mutex);
    int &n = g_exit_logged[e];
    if (n >= 8) return;
    ++n;
  }
  const auto &si = g_exit_info[e];
  log(std::format("[mcnote] pkt id={} getId{:+d} (+0x{:X}) EXIT {} this->[{}]", si.packet_id,
                  si.rel, si.rva, thread_info(), dump_ints(self)));
}

template <std::size_t I> void enter_cb(SafetyHookContext &ctx) { on_enter(I, ctx); }

using EnterFn = void (*)(SafetyHookContext &);
template <std::size_t... I>
constexpr std::array<EnterFn, sizeof...(I)> make_enter(std::index_sequence<I...>) {
  return {&enter_cb<I>...};
}
constexpr auto kEnter = make_enter(std::make_index_sequence<kMaxEnter>{});

// 6個の整数引数をそのまま元の関数へ渡して、戻ってきたあとに this の中身を見る
template <std::size_t I> U64 exit_cb(U64 a, U64 b, U64 c, U64 d, U64 e, U64 f) {
  const U64 r = g_exit_hooks[I].stdcall<U64>(a, b, c, d, e, f);
  on_exit(I, a);
  return r;
}

using ExitFn = U64 (*)(U64, U64, U64, U64, U64, U64);
template <std::size_t... I>
constexpr std::array<ExitFn, sizeof...(I)> make_exit(std::index_sequence<I...>) {
  return {&exit_cb<I>...};
}
constexpr auto kExit = make_exit(std::make_index_sequence<kMaxExit>{});

// 読み取り専用のデータ領域から、target を値として持つ8バイトの場所を全部探す
std::vector<std::uintptr_t> find_pointers_to(std::uintptr_t target) {
  std::vector<std::uintptr_t> out;
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);

  std::vector<std::uint64_t> buf(1 << 17); // 1MBずつ読む
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
      if (bytes == 0 || !safe_read(begin + off, buf.data(), bytes)) continue;
      for (std::size_t i = 0; i < bytes / 8; ++i) {
        if (buf[i] == target) out.push_back(begin + off + i * 8);
      }
    }
  }
  return out;
}

void hook_window(int packet_id, std::uintptr_t getid_slot, std::set<std::uintptr_t> &seen) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto image_end = base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);

  for (int rel = -1; rel <= 13; ++rel) {
    const auto slot_addr =
        static_cast<std::uintptr_t>(static_cast<std::intptr_t>(getid_slot) + rel * 8);
    std::uintptr_t fn = 0;
    if (!safe_read(slot_addr, &fn, sizeof(fn))) continue;
    if (rel == 0 || fn < base || fn >= image_end) continue;
    if (!seen.insert(fn).second) continue; // 他のパケットと共通の関数は1回だけ

    DWORD64 image_base = 0;
    const RUNTIME_FUNCTION *rf = RtlLookupFunctionEntry(fn, &image_base, nullptr);
    if (!rf || image_base + rf->BeginAddress != fn || rf->EndAddress - rf->BeginAddress < 16) {
      log(std::format("[mcnote]   getId{:+d} = +0x{:X} (skipped: tiny or not plain)", rel,
                      fn - base));
      continue;
    }
    const auto size = rf->EndAddress - rf->BeginAddress;

    // 読み込みの候補は「戻った後」を見る。それ以外は入口だけ見る
    const bool want_exit = rel == 5 || rel == 7 || rel == 8 || rel == 13;
    if (want_exit && g_exit_count < kMaxExit) {
      const std::size_t e = g_exit_count;
      g_exit_info[e] = {packet_id, rel, fn - base};
      auto hook = safetyhook::InlineHook::create(reinterpret_cast<void *>(fn),
                                                 reinterpret_cast<void *>(kExit[e]));
      if (!hook) {
        log(std::format("[mcnote]   getId{:+d} = +0x{:X} (exit hook failed)", rel, fn - base));
        continue;
      }
      g_exit_hooks[e] = std::move(*hook);
      ++g_exit_count;
      log(std::format("[mcnote]   getId{:+d} = +0x{:X} size=0x{:X} traced (exit)", rel, fn - base,
                      size));
      continue;
    }

    if (g_enter_hooks.size() >= kMaxEnter) continue;
    const std::size_t idx = g_enter_hooks.size();
    g_enter_info[idx] = {packet_id, rel, fn - base};
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void *>(fn), kEnter[idx]);
    if (!hook) {
      log(std::format("[mcnote]   getId{:+d} = +0x{:X} (hook failed)", rel, fn - base));
      continue;
    }
    g_enter_hooks.push_back(std::move(*hook));
    log(std::format("[mcnote]   getId{:+d} = +0x{:X} size=0x{:X} traced (enter)", rel, fn - base,
                    size));
  }
}

void hotkey_loop() {
  bool was_down = false;
  for (;;) {
    const bool down = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
    if (down && !was_down) {
      const bool now = !g_on.load();
      if (now) {
        std::lock_guard lock(g_mutex);
        g_enter_logged.clear();
        g_exit_logged.clear();
      }
      g_on = now;
      log(now ? "[mcnote] packet probe ON" : "[mcnote] packet probe OFF");
    }
    was_down = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

} // namespace

void install_packet_probe() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  std::set<std::uintptr_t> seen;

  for (const int id : {0x1A, 0x7B}) { // BlockEvent と LevelSoundEvent(記憶ベースの番号)
    const auto fns = sig_find_all(std::format("B8 {:02X} 00 00 00 C3", id));
    log(std::format("[mcnote] packet id {}: {} getId candidate(s)", id, fns.size()));

    for (const auto fn : fns) {
      for (const auto slot : find_pointers_to(fn)) {
        log(std::format("[mcnote]  id {}: getId +0x{:X} is stored at +0x{:X}", id, fn - base,
                        slot - base));
        hook_window(id, slot, seen);
      }
    }
  }
  log(std::format("[mcnote] packet probe: {} enter hook(s), {} exit hook(s) (F7 to record)",
                  g_enter_hooks.size(), g_exit_count));
  std::thread(hotkey_loop).detach();
}