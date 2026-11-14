#include "rc_probe.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <safetyhook.hpp>

#include "attack.hpp"
#include "log.hpp"
#include "thread_info.hpp"
#include "debug_flag.hpp"

namespace {

// 落ちた関数と、その呼び出し元の共通部分(今のバージョンのRVA。ゲームが更新されたら取り直す)
constexpr std::array<std::uintptr_t, 4> kTargets = {0x18DAEB0, 0x1ACDB20, 0x1ACD2C0, 0x1ACD100};

std::atomic_bool g_on{false};
std::mutex g_mutex;
std::map<std::pair<std::size_t, bool>, int> g_count; // (どの関数か, 自分の呼び出しか) -> 記録した回数
std::vector<safetyhook::MidHook> g_hooks;

bool safe_read(std::uintptr_t src, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void *>(src), dst, n,
                           &got) &&
         got == n;
}

// ポインタなら、指す先の先頭32バイトを8バイトずつ16進で出す
std::string describe(const char *name, std::uint64_t v) {
  std::array<std::uint64_t, 4> q{};
  if (v > 0x10000 && safe_read(v, q.data(), sizeof(q))) {
    return std::format(" {}=0x{:X}->[0x{:X},0x{:X},0x{:X},0x{:X}]", name, v, q[0], q[1], q[2], q[3]);
  }
  return std::format(" {}=0x{:X}", name, v);
}

void on_hit(std::size_t idx, SafetyHookContext &ctx) {
  if (!g_on.load(std::memory_order_relaxed) || !on_main_thread()) return;

  const bool ours = attack_in_progress();
  {
    std::lock_guard lock(g_mutex);
    int &n = g_count[{idx, ours}];
    if (n >= 4) return; // 関数ごと、本物/自分ごとに4回まで
    ++n;
  }
  log(std::format("[mcnote] rc {} fn=+0x{:X}", ours ? "OURS" : "REAL", kTargets[idx]) +
      describe("rcx", ctx.rcx) + describe("rdx", ctx.rdx) + describe("r8", ctx.r8) +
      describe("r9", ctx.r9));
}

template <std::size_t I> void cb(SafetyHookContext &ctx) { on_hit(I, ctx); }

using Fn = void (*)(SafetyHookContext &);
template <std::size_t... I>
constexpr std::array<Fn, sizeof...(I)> make_callbacks(std::index_sequence<I...>) {
  return {&cb<I>...};
}
constexpr auto kCallbacks = make_callbacks(std::make_index_sequence<kTargets.size()>{});

void hotkey_loop() {
  bool was_down = false;
  for (;;) {
    const bool down = debug_enabled() && (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
    if (down && !was_down) {
      const bool now = !g_on.load();
      if (now) {
        std::lock_guard lock(g_mutex);
        g_count.clear();
      }
      g_on = now;
      log(now ? "[mcnote] rc probe ON" : "[mcnote] rc probe OFF");
    }
    was_down = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

} // namespace

void install_rc_probe() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  for (std::size_t i = 0; i < kTargets.size(); ++i) {
    const auto fn = base + kTargets[i];

    DWORD64 image_base = 0;
    const RUNTIME_FUNCTION *rf = RtlLookupFunctionEntry(fn, &image_base, nullptr);
    if (!rf || image_base + rf->BeginAddress != fn) {
      log(std::format("[mcnote] rc probe: +0x{:X} is not a function start (game updated?)",
                      kTargets[i]));
      continue;
    }
    auto hook = safetyhook::MidHook::create(reinterpret_cast<void *>(fn), kCallbacks[i]);
    if (!hook) {
      log(std::format("[mcnote] rc probe: failed to hook +0x{:X}", kTargets[i]));
      continue;
    }
    g_hooks.push_back(std::move(*hook));
  }
  log(std::format("[mcnote] rc probe: {} hook(s) installed (F6 to record)", g_hooks.size()));
  std::thread(hotkey_loop).detach();
}