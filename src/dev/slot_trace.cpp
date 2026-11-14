#include "slot_trace.hpp"

#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <format>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <safetyhook.hpp>

#include "log.hpp"
#include "thread_info.hpp"

#include <chrono>

#include "attack.hpp"
#include "debug_flag.hpp"

namespace {

constexpr int kMaxSlots = 24;
constexpr int kStartIdx = kMaxSlots; // 「START」を表す番号
constexpr int kVtableSlots = 64;
constexpr std::size_t kMaxEvents = 4096;

std::atomic_bool g_on{false};
std::array<std::atomic<int>, kMaxSlots + 1> g_hits{};
std::array<std::atomic_bool, kMaxSlots + 1> g_first_logged{};
std::array<std::uint8_t, kMaxEvents> g_events{};
std::atomic<std::size_t> g_event_n{0};

std::array<int, kMaxSlots> g_slot_offset{}; // startからのスロット差(例: +3)
std::array<std::uintptr_t, kMaxSlots> g_slot_rva{};
int g_hooked_count = 0;
std::vector<safetyhook::MidHook> g_hooks;
std::atomic_bool g_installed{false};
std::atomic<std::uintptr_t> g_last_vtable{0};

bool safe_read(const void *src, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(), src, dst, n, &got) && got == n;
}

std::string describe(const char *name, std::uint64_t v) {
  std::array<std::int32_t, 4> d{};
  if (v > 0x10000 &&
      safe_read(reinterpret_cast<const void *>(v), d.data(), sizeof(d))) {
    return std::format(" {}=0x{:X}->[{},{},{},{}]", name, v, d[0], d[1], d[2],
                       d[3]);
  }
  return std::format(" {}=0x{:X}", name, v);
}

std::string label(int idx) {
  if (idx == kStartIdx)
    return "START";
  return std::format(
      "slot[{}]",
      g_slot_offset[idx]); // g_slot_offset にはスロット番号をそのまま入れる
}

void record(int idx, const SafetyHookContext *ctx) {
  if (ctx && g_slot_offset[idx] != 0 && on_main_thread())
    attack_heartbeat(
        reinterpret_cast<void *>(ctx->rcx)); // スロット0(後片付けの関数)は除く

  if (ctx && g_slot_offset[idx] == 13 && on_main_thread())
    attack_capture_use(ctx->r8, ctx->rsp); // 本物の右クリックの引数を覚える

  if (!g_on.load(std::memory_order_relaxed) || !on_main_thread())
    return;

  g_hits[idx].fetch_add(1);
  const auto n = g_event_n.fetch_add(1);
  if (n < kMaxEvents)
    g_events[n] = static_cast<std::uint8_t>(idx);

  if (ctx && !g_first_logged[idx].exchange(true)) {
    log(std::format("[mcnote] slot {} first hit:", label(idx)) +
        describe("rcx", ctx->rcx) + describe("rdx", ctx->rdx) +
        describe("r8", ctx->r8) + describe("r9", ctx->r9));
  }
}

template <std::size_t I> void slot_cb(SafetyHookContext &ctx) {
  record(static_cast<int>(I), &ctx);
}

using Fn = void (*)(SafetyHookContext &);
template <std::size_t... I>
constexpr std::array<Fn, sizeof...(I)>
make_callbacks(std::index_sequence<I...>) {
  return {&slot_cb<I>...};
}
constexpr auto kCallbacks =
    make_callbacks(std::make_index_sequence<kMaxSlots>{});

void install_hooks(std::uintptr_t vtable, std::uintptr_t start_fn) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt =
      reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const auto image_end =
      base + static_cast<std::uintptr_t>(nt->OptionalHeader.SizeOfImage);

  std::array<std::uintptr_t, kVtableSlots> slots{};
  int n = 0;
  int start_slot = -1;
  for (; n < kVtableSlots; ++n) {
    std::uintptr_t v = 0;
    if (!safe_read(reinterpret_cast<const void *>(
                       vtable + static_cast<std::uintptr_t>(n) * 8),
                   &v, sizeof(v)))
      break;
    slots[n] = v;
    if (v == start_fn && start_slot < 0)
      start_slot = n;
  }

  // vtableの中身(ゲーム本体のコードを指すスロットだけRVAで表示)
  log(std::format("[mcnote] game mode vtable = +0x{:X}", vtable - base));
  for (int row = 0; row < n; row += 4) {
    std::string line;
    for (int i = row; i < std::min(row + 4, n); ++i) {
      line += (slots[i] >= base && slots[i] < image_end)
                  ? std::format(" [{}]=+0x{:X}", i, slots[i] - base)
                  : std::format(" [{}]=-", i);
    }
    log("[mcnote] vtable" + line);
  }

  if (start_slot < 0) {
    log("[mcnote] startDestroyBlock is not in this vtable; tracing slots 0..15 "
        "instead");
  } else {
    log(std::format("[mcnote] startDestroyBlock = vtable slot {}", start_slot));
  }

  const int first_slot = start_slot < 0 ? 0 : std::max(0, start_slot - 8);
  const int last_slot = start_slot < 0 ? std::min(n - 1, kMaxSlots - 1)
                                       : std::min(n - 1, start_slot + 8);

  std::set<std::uintptr_t> seen{start_fn};
  for (int i = first_slot; i <= last_slot; ++i) {
    if (g_hooked_count >= kMaxSlots)
      break;
    const auto fn = slots[i];
    if (fn < base || fn >= image_end || !seen.insert(fn).second)
      continue;

    DWORD64 image_base = 0;
    const RUNTIME_FUNCTION *rf =
        RtlLookupFunctionEntry(fn, &image_base, nullptr);
    if (!rf || image_base + rf->BeginAddress != fn ||
        rf->EndAddress - rf->BeginAddress < 16) {
      log(std::format(
          "[mcnote]   skip slot[{}] (+0x{:X}): not a plain function", i,
          fn - base));
      continue;
    }

    const int idx = g_hooked_count;
    g_slot_offset[idx] = i; // スロット番号をそのまま入れる
    g_slot_rva[idx] = fn - base;
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void *>(fn),
                                            kCallbacks[idx]);
    if (!hook) {
      log(std::format("[mcnote]   failed to hook slot[{}] (+0x{:X})", i,
                      fn - base));
      continue;
    }
    g_hooks.push_back(std::move(*hook));
    ++g_hooked_count;
    log(std::format("[mcnote]   traced slot[{}]: +0x{:X} size=0x{:X}", i,
                    fn - base, rf->EndAddress - rf->BeginAddress));
  }
}

void print_summary() {
  std::string counts = " START=" + std::to_string(g_hits[kStartIdx].load());
  for (int i = 0; i < g_hooked_count; ++i) {
    counts += std::format(" {}(+0x{:X})={}", label(i), g_slot_rva[i],
                          g_hits[i].load());
  }
  log("[mcnote] slot trace OFF; hits:" + counts);

  const auto n = std::min<std::size_t>(g_event_n.load(), kMaxEvents);
  std::string seq;
  int groups = 0;
  for (std::size_t i = 0; i < n && groups < 80;) {
    std::size_t j = i;
    while (j < n && g_events[j] == g_events[i])
      ++j;
    seq += std::format(" {}x{}", label(g_events[i]), j - i);
    ++groups;
    i = j;
  }
  log("[mcnote] sequence:" + seq);
}

} // namespace

void slot_trace_on_capture(void *game_mode, std::uintptr_t start_fn) {
  std::uintptr_t vt = 0;
  if (!safe_read(game_mode, &vt, sizeof(vt)))
    return;
  if (g_last_vtable.exchange(vt) == vt)
    return;
  if (g_installed.exchange(true)) {
    log("[mcnote] game mode vtable changed; tracing stays on the first one "
        "(restart to retrace)");
    return;
  }
  // フックの導入は、ゲームの関数の中ではなく別スレッドで行う
  std::thread(install_hooks, vt, start_fn).detach();
}

void slot_trace_note_start() { record(kStartIdx, nullptr); }

void slot_trace_poll_keys(HWND hwnd) {
  if (!debug_enabled()) return;
  static bool was_down = false;
  const bool down =
      GetForegroundWindow() == hwnd && (GetAsyncKeyState(VK_F8) & 0x8000) != 0;

  if (down && !was_down) {
    if (!g_on.load()) {
      for (auto &h : g_hits)
        h = 0;
      for (auto &f : g_first_logged)
        f = false;
      g_event_n = 0;
      g_on = true;
      log("[mcnote] slot trace ON");
    } else {
      g_on = false;
      print_summary();
    }
  }
  was_down = down;
}

// 立っている間に、どのスロットが何回呼ばれるかを3秒間測る(心拍に使える関数を探すため)
void slot_trace_measure_idle() {
  if (g_on.load())
    return;
  std::thread([] {
    for (auto &h : g_hits)
      h = 0;
    for (auto &f : g_first_logged)
      f = false;
    g_event_n = 0;
    g_on = true;
    log("[mcnote] idle measure ON (3 s): stand still, do not click");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    g_on = false;
    print_summary();
  }).detach();
}