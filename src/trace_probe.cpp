#include <windows.h>

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
#include "thread_info.hpp"
#include "trace_probe.hpp"

namespace {

std::atomic_bool g_recording{false};
std::atomic<int> g_seq{0};
std::mutex g_mutex;
std::set<std::pair<std::uintptr_t, DWORD>> g_seen; // (関数のRVA, スレッドID)
std::map<std::uintptr_t, int> g_dump_count; // (RVA*2 + MAINなら1) -> ダンプ回数
std::vector<safetyhook::MidHook> g_hooks;

void trace_hit(std::uintptr_t rva) {
  if (!g_recording.load(std::memory_order_relaxed))
    return;

  const DWORD tid = GetCurrentThreadId();
  {
    std::lock_guard lock(g_mutex);
    if (!g_seen.emplace(rva, tid).second)
      return;
  }
  log(std::format("[mcnote] trace #{} fn=+0x{:X} {}", g_seq.fetch_add(1), rva,
                  thread_info()));
}

bool safe_read(std::uintptr_t addr, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(),
                           reinterpret_cast<const void *>(addr), dst, n,
                           &got) &&
         got == n;
}

// レジスタの値を表示する。ポインタに見えて読めるなら、指す先の32バイトをint8個で出す
std::string describe_reg(const char *name, std::uint64_t v) {
  std::array<std::int32_t, 8> d{};
  if (v > 0x10000 && safe_read(v, d.data(), sizeof(d))) {
    return std::format(" {}=0x{:X}->[{},{},{},{},{},{},{},{}]", name, v, d[0],
                       d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
  }
  return std::format(" {}=0x{:X}", name, v);
}

void dump_args(std::uintptr_t rva, const SafetyHookContext &ctx) {
  if (!g_recording.load(std::memory_order_relaxed))
    return;

  const std::string ti = thread_info();
  const bool is_main = ti.find("MINECRAFT MAIN THREAD") != std::string::npos;
  {
    std::lock_guard lock(g_mutex);
    int &n = g_dump_count[rva * 2 + (is_main ? 1 : 0)];
    if (n >= 4)
      return; // 関数・スレッドの組ごとに4回まで
    ++n;
  }
  log(std::format("[mcnote] args fn=+0x{:X} {}", rva, ti) +
      describe_reg("rcx", ctx.rcx) + describe_reg("rdx", ctx.rdx) +
      describe_reg("r8", ctx.r8) + describe_reg("r9", ctx.r9));
}

template <std::uintptr_t Rva> void trace_cb(SafetyHookContext &) {
  trace_hit(Rva);
}

template <std::uintptr_t Rva> void dump_cb(SafetyHookContext &ctx) {
  trace_hit(Rva);
  dump_args(Rva, ctx);
}

struct Target {
  std::uintptr_t rva;
  void (*fn)(SafetyHookContext &);
};

constexpr Target kTargets[] = {
    {0x32D39C0, &trace_cb<0x32D39C0>}, // 仮想関数(dest)
    {0x32CE100, &trace_cb<0x32CE100>},
    {0x3769640, &dump_cb<0x3769640>}, // 本命候補: 引数もダンプ
    {0x232B900, &trace_cb<0x232B900>},
    {0x22C6320, &trace_cb<0x22C6320>},
    {0x2430C20, &dump_cb<0x2430C20>}, // 補助候補
    {0x9BBF90, &trace_cb<0x9BBF90>},
    {0x22C6AA0, &trace_cb<0x22C6AA0>},
    {0x38BB40, &trace_cb<0x38BB40>},
    {0x2430F30, &dump_cb<0x2430F30>}, // 補助候補
};

void hotkey_loop() {
  bool was_down = false;
  for (;;) {
    const bool down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
    if (down && !was_down) {
      const bool now = !g_recording.load();
      if (now) {
        std::lock_guard lock(g_mutex);
        g_seen.clear();
        g_dump_count.clear();
        g_seq = 0;
      }
      g_recording = now;
      log(now ? "[mcnote] trace recording ON" : "[mcnote] trace recording OFF");
    }
    was_down = down;
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

} // namespace

void install_trace_probe() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));

  for (const auto &t : kTargets) {
    auto hook = safetyhook::MidHook::create(
        reinterpret_cast<void *>(base + t.rva), t.fn);
    if (!hook) {
      log(std::format("[mcnote] trace hook failed: +0x{:X}", t.rva));
      continue;
    }
    g_hooks.push_back(std::move(*hook));
  }
  log(std::format("[mcnote] trace hooks installed: {}/{}", g_hooks.size(),
                  std::size(kTargets)));

  std::thread(hotkey_loop).detach();
}