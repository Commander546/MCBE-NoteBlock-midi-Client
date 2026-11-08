#include "attack.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <deque>
#include <format>
#include <map>
#include <mutex>
#include <set>
#include <string>

#include <safetyhook.hpp>

#include "log.hpp"
#include "player.hpp"
#include "sig_scan.hpp"
#include "slot_trace.hpp"
#include "thread_info.hpp"

namespace {

std::atomic<ULONGLONG> g_beat_tick{0};

// startDestroyBlock
// の先頭のバイト列(位置ではなく、バイト列で探すので、更新に強い)
constexpr const char *kStartDestroySig =
    "41 57 41 56 41 55 41 54 56 57 55 53 48 83 EC ?? 4C 89 CB 44 89 C5 48 89 "
    "D6 48 89 CF "
    "44 88 44 24 ?? 48 8B 49 08";

constexpr int kStopSlot = 4; // vtableのスロット番号(停止側の関数)
constexpr long kKnownGmOffset =
    -1; // プレイヤー内のGameModeの場所が分かったら、ここに入れる(-1
        // = 自動で探す)
constexpr std::size_t kPlayerScan = 0x8000; // プレイヤーの先頭32KBを探す

using StopFn = void (*)(void *self, const BlockPos *pos);

struct Request {
  BlockPos pos;
  int face;
};

safetyhook::InlineHook g_hook;
std::atomic_bool g_hooked{false};
std::atomic_bool g_attacking{false};
std::atomic_bool g_quiet{false};
std::atomic<void *> g_game_mode{nullptr}; // 本物のクリックで捕まえた this
std::atomic<std::uintptr_t> g_player{0};  // this+8 のプレイヤー
std::atomic<long> g_gm_off{
    kKnownGmOffset}; // プレイヤー内で GameMode を指す場所
std::atomic_bool g_have_last{false};
std::atomic<int> g_real_logged{0};
std::atomic<ULONGLONG> g_last_real_tick{0};
std::uintptr_t g_start_fn = 0;
BlockPos g_last{};
std::mutex g_mutex;
std::deque<Request> g_queue;
std::map<std::uintptr_t, bool>
    g_instant_of; // vtable -> 直近の本物のクリックで即座に壊れたか
std::set<std::size_t> g_gm_candidates;
std::uintptr_t g_candidate_player = 0;

bool safe_read(const void *src, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(), src, dst, n, &got) && got == n;
}

std::string hex16(const void *p) {
  std::array<std::uint8_t, 16> b{};
  if (!p || !safe_read(p, b.data(), b.size()))
    return "(unreadable)";
  std::string s;
  for (const auto v : b)
    s += std::format("{:02X} ", v);
  return s;
}

std::string module_of(std::uintptr_t addr) {
  HMODULE m = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(addr), &m) ||
      !m) {
    return std::format("(unknown module) 0x{:X}", addr);
  }
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(m, path, MAX_PATH);
  const std::wstring w(path);
  const auto slash = w.find_last_of(L"\\/");
  const std::wstring name =
      slash == std::wstring::npos ? w : w.substr(slash + 1);

  std::string utf8(static_cast<std::size_t>(name.size()) * 4 + 1, '\0');
  const int n =
      WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, utf8.data(),
                          static_cast<int>(utf8.size()), nullptr, nullptr);
  utf8.resize(n > 0 ? n - 1 : 0);
  return std::format("{}+0x{:X}", utf8,
                     addr - reinterpret_cast<std::uintptr_t>(m));
}

// 自分で呼んでいる間に落ちたら、落ちた場所と呼び出しの道筋を記録する(止めるのは、下のSEHの役目)
LONG CALLBACK crash_logger(EXCEPTION_POINTERS *ep) {
  if (!g_attacking.load())
    return EXCEPTION_CONTINUE_SEARCH;

  const auto *er = ep->ExceptionRecord;
  switch (er->ExceptionCode) {
  case EXCEPTION_ACCESS_VIOLATION:
  case EXCEPTION_IN_PAGE_ERROR:
  case EXCEPTION_ILLEGAL_INSTRUCTION:
  case EXCEPTION_PRIV_INSTRUCTION:
  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    break;
  default:
    return EXCEPTION_CONTINUE_SEARCH;
  }

  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const CONTEXT *c0 = ep->ContextRecord;
  const auto access =
      er->NumberParameters >= 2 ? er->ExceptionInformation[0] : 0;
  const auto fault =
      er->NumberParameters >= 2 ? er->ExceptionInformation[1] : 0;

  log(std::format("[mcnote] CRASH code=0x{:X} at {} access={} (0=read 1=write "
                  "8=exec) addr=0x{:X}",
                  er->ExceptionCode,
                  module_of(static_cast<std::uintptr_t>(c0->Rip)), access,
                  fault));
  log(std::format(
      "[mcnote]   rax=0x{:X} rcx=0x{:X} rdx=0x{:X} r8=0x{:X} r9=0x{:X}",
      c0->Rax, c0->Rcx, c0->Rdx, c0->R8, c0->R9));

  CONTEXT c = *c0;
  for (int i = 0; i < 16 && c.Rip; ++i) {
    DWORD64 image_base = 0;
    PRUNTIME_FUNCTION rf = RtlLookupFunctionEntry(c.Rip, &image_base, nullptr);

    std::string extra;
    if (rf && image_base == base)
      extra = std::format(" (game fn=+0x{:X})", rf->BeginAddress);
    log(std::format("[mcnote]   #{} {}{}", i,
                    module_of(static_cast<std::uintptr_t>(c.Rip)), extra));

    if (!rf) { // 葉の関数: スタックの先頭がリターンアドレス
      DWORD64 ret = 0;
      if (!safe_read(reinterpret_cast<const void *>(c.Rsp), &ret, sizeof(ret)))
        break;
      c.Rip = ret;
      c.Rsp += 8;
    } else {
      void *handler_data = nullptr;
      DWORD64 establisher = 0;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, c.Rip, rf, &c,
                       &handler_data, &establisher, nullptr);
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

// SEH: アクセス違反などのときだけ捕まえる(C++の例外などは、そのまま通す)
int seh_filter(unsigned long code) {
  return (code == EXCEPTION_ACCESS_VIOLATION ||
          code == EXCEPTION_IN_PAGE_ERROR ||
          code == EXCEPTION_ILLEGAL_INSTRUCTION)
             ? EXCEPTION_EXECUTE_HANDLER
             : EXCEPTION_CONTINUE_SEARCH;
}

// ゲームの関数を呼ぶ。落ちたら
// false(デストラクタを持つ物を、この関数の中に置かないこと)
bool guarded_start(void *self, std::int32_t *pos, std::uint32_t face,
                   std::uint8_t *arg4, bool *ret) {
  __try {
    *ret = g_hook.stdcall<bool>(self, pos, face, arg4);
    return true;
  } __except (seh_filter(GetExceptionCode())) {
    return false;
  }
}

using StopFn = void (*)(void *self, const BlockPos *pos);

bool guarded_stop(StopFn stop, void *self, const BlockPos *pos) {
  __try {
    stop(self, pos);
    return true;
  } __except (seh_filter(GetExceptionCode())) {
    return false;
  }
}

// thisのvtableから停止側の関数を取り出す
StopFn resolve_stop(void *self) {
  std::uintptr_t vt = 0;
  if (!safe_read(self, &vt, sizeof(vt)))
    return nullptr;

  std::uintptr_t fn = 0;
  if (!safe_read(reinterpret_cast<const void *>(vt + kStopSlot * 8), &fn,
                 sizeof(fn)))
    return nullptr;

  DWORD64 image_base = 0;
  const RUNTIME_FUNCTION *rf = RtlLookupFunctionEntry(fn, &image_base, nullptr);
  const DWORD size = rf ? rf->EndAddress - rf->BeginAddress : 0;
  if (!rf || image_base + rf->BeginAddress != fn || size < 0x10 || size > 0x100)
    return nullptr;

  static bool logged = false;
  if (!logged) {
    logged = true;
    const auto *p = reinterpret_cast<const std::uint8_t *>(fn);
    std::string bytes;
    for (int i = 0; i < 40; ++i)
      bytes += std::format("{:02X} ", p[i]);
    log(std::format(
        "[mcnote] stop fn (+0x{:X}, size 0x{:X}) bytes: {}",
        fn - reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr)), size,
        bytes));
  }
  return reinterpret_cast<StopFn>(fn);
}

// 今のGameModeを返す。プレイヤー内の場所が分かっていれば、そこから読む(モードを切り替えても最新)
void *current_game_mode() {
  const long off = g_gm_off.load();
  const auto player = g_player.load();
  if (off >= 0 && player) {
    std::uintptr_t p = 0;
    if (safe_read(reinterpret_cast<const void *>(
                      player + static_cast<std::uintptr_t>(off)),
                  &p, sizeof(p)) &&
        p) {
      return reinterpret_cast<void *>(p);
    }
  }
  return g_game_mode.load();
}

// 本物のクリックで「壊れない種類」と分かっているvtableだけを許可する(未知の種類は拒否)
bool class_allowed(std::uintptr_t vtable) {
  std::lock_guard lock(g_mutex);
  const auto it = g_instant_of.find(vtable);
  return it != g_instant_of.end() && !it->second;
}

// this を捕まえるたびに、プレイヤーの中から this
// と同じ値の場所を探して、候補を絞る
void learn_game_mode_offset(void *self, std::uintptr_t player) {
  const auto self_addr = reinterpret_cast<std::uintptr_t>(self);

  const long known = g_gm_off.load();
  if (known >= 0) { // すでに分かっているなら、合っているか確認するだけ
    std::uintptr_t p = 0;
    if (safe_read(reinterpret_cast<const void *>(
                      player + static_cast<std::uintptr_t>(known)),
                  &p, sizeof(p)) &&
        p == self_addr) {
      return;
    }
    log("[mcnote] game mode offset check failed; searching again");
    g_gm_off = -1;
  }

  std::set<std::size_t> now;
  std::array<std::uint64_t, 512> buf{}; // 4KBずつ
  for (std::size_t off = 0; off < kPlayerScan; off += sizeof(buf)) {
    if (!safe_read(reinterpret_cast<const void *>(player + off), buf.data(),
                   sizeof(buf)))
      continue;
    for (std::size_t i = 0; i < buf.size(); ++i) {
      if (buf[i] == self_addr)
        now.insert(off + i * 8);
    }
  }

  std::lock_guard lock(g_mutex);
  if (g_candidate_player != player || g_gm_candidates.empty()) {
    g_candidate_player = player;
    g_gm_candidates = now;
  } else {
    std::set<std::size_t> both;
    for (const auto o : g_gm_candidates) {
      if (now.contains(o))
        both.insert(o);
    }
    g_gm_candidates = std::move(both);
  }

  std::string list;
  for (const auto o : g_gm_candidates)
    list += std::format(" +0x{:X}", o);
  log(std::format("[mcnote] game mode offset candidates in player:{}",
                  list.empty() ? " (none)" : list));
  if (g_gm_candidates.size() == 1) {
    g_gm_off = static_cast<long>(*g_gm_candidates.begin());
    log(std::format("[mcnote] game mode offset found: player+0x{:X}",
                    g_gm_off.load()));
  }
}

// 本物のクリックで呼ばれたときの形をそのまま通しつつ、this・最後のクリック位置・モードの種類を覚える
bool hooked_start_destroy(void *self, const BlockPos *pos, std::uint32_t face,
                          void *arg4) {
  const bool is_main = on_main_thread();
  if (is_main) {
    slot_trace_note_start();
    if (g_game_mode.exchange(self) != self) {
      std::uintptr_t player = 0;
      safe_read(reinterpret_cast<const void *>(
                    reinterpret_cast<std::uintptr_t>(self) + 8),
                &player, sizeof(player));
      g_player = player;
      log(std::format("[mcnote] game mode captured: this=0x{:X}",
                      reinterpret_cast<std::uintptr_t>(self)));
      if (player)
        learn_game_mode_offset(self, player);
      slot_trace_on_capture(self, g_start_fn);
    }
    {
      std::lock_guard lock(g_mutex);
      g_last = *pos;
    }
    g_have_last = true;
    g_last_real_tick = GetTickCount64();
  }

  const bool log_it = is_main && g_real_logged.load() < 10;
  const std::string before = log_it ? hex16(arg4) : std::string{};

  const bool ret = g_hook.stdcall<bool>(self, pos, face, arg4);

  if (is_main) {
    std::uint8_t flag = 0;
    std::uintptr_t vt = 0;
    if (safe_read(arg4, &flag, 1) && safe_read(self, &vt, sizeof(vt))) {
      std::lock_guard lock(g_mutex);
      g_instant_of[vt] =
          flag !=
          0; // 最新の結果で上書き(壊れやすいブロックの誤判定が、残り続けないように)
    }
  }

  if (log_it) {
    ++g_real_logged;
    log(std::format("[mcnote] real call: pos=({}, {}, {}) face={} ret={} arg4 "
                    "before: {}after: {}",
                    pos->x, pos->y, pos->z, face, ret, before, hex16(arg4)));
  }
  return ret;
}

} // namespace

void install_attack_hook() {
  int hits = 0;
  const auto addr = sig_find(kStartDestroySig, &hits);
  log(std::format("[mcnote] sig 'start destroy': {} hit(s)", hits));
  if (!addr)
    return;
  g_start_fn = addr;

  auto hook = safetyhook::InlineHook::create(
      reinterpret_cast<void *>(addr),
      reinterpret_cast<void *>(&hooked_start_destroy));
  if (!hook) {
    log("[mcnote] failed to hook attack fn");
    return;
  }
  g_hook = std::move(*hook);
  g_hooked = true;

  AddVectoredExceptionHandler(1, crash_logger);
  log("[mcnote] attack fn hooked + crash logger installed (left-click blocks "
      "to capture)");
}

bool attack_ready() {
  void *gm = current_game_mode();
  if (!gm)
    return false;
  std::uintptr_t vt = 0;
  return safe_read(gm, &vt, sizeof(vt)) && class_allowed(vt);
}

const char *attack_state() {
  void *gm = current_game_mode();
  if (!gm)
    return "left-click a block once (UI closed)";

  std::uintptr_t vt = 0;
  if (!safe_read(gm, &vt, sizeof(vt)))
    return "game mode unreadable";

  std::lock_guard lock(g_mutex);
  const auto it = g_instant_of.find(vt);
  if (it == g_instant_of.end())
    return "unknown mode: left-click a block once in this mode";
  if (it->second)
    return "blocked: creative-like mode";
  return "ready";
}

bool attack_last_clicked(BlockPos &out) {
  if (!g_have_last)
    return false;
  std::lock_guard lock(g_mutex);
  out = g_last;
  return true;
}

std::uintptr_t attack_player() { return g_player.load(); }

void attack_heartbeat(void *gm) {
  if (!gm)
    return;
  const long off = g_gm_off.load();
  if (off < 0)
    return; // プレイヤー内の場所がまだ分からないうちは、何もしない

  const ULONGLONG now = GetTickCount64();
  if (g_beat_tick.load() == now)
    return; // 同じ瞬間の呼び出しは、まとめて無視する

  std::uintptr_t owner = 0;
  if (!safe_read(reinterpret_cast<const void *>(
                     reinterpret_cast<std::uintptr_t>(gm) + 8),
                 &owner, sizeof(owner)) ||
      !owner)
    return;

  // 持ち主のプレイヤーが、今のGameModeとして持っている物だけを信じる
  std::uintptr_t back = 0;
  if (!safe_read(reinterpret_cast<const void *>(
                     owner + static_cast<std::uintptr_t>(off)),
                 &back, sizeof(back)) ||
      back != reinterpret_cast<std::uintptr_t>(gm))
    return;

  g_player = owner;
  g_game_mode = gm;
  g_beat_tick = now;
}

long long attack_beat_age_ms() {
  const ULONGLONG t = g_beat_tick.load();
  return t ? static_cast<long long>(GetTickCount64() - t) : -1;
}

void attack_set_quiet(bool quiet) { g_quiet = quiet; }

void request_attack(HWND hwnd, int x, int y, int z, int face) {
  {
    std::lock_guard lock(g_mutex);
    if (g_queue.size() >= 64)
      return;
    g_queue.push_back({{x, y, z}, face});
  }
  PostMessageW(hwnd, kMsgAttack, 0, 0);
}

void attack_on_ui_thread() {
  std::deque<Request> work;
  {
    std::lock_guard lock(g_mutex);
    work.swap(g_queue);
  }
  if (work.empty())
    return;

  // 拒否するときは、ログを1秒に1回だけ出して、演奏も止める
  static ULONGLONG last_refuse_log = 0;
  const auto refuse = [&](const char *why) {
    const ULONGLONG now = GetTickCount64();
    if (now - last_refuse_log > 1000) {
      last_refuse_log = now;
      log(std::string("[mcnote] attack refused: ") + why);
    }
    player_stop();
  };

  if (!g_hooked || !on_main_thread()) {
    refuse("not on MAIN THREAD or not hooked");
    return;
  }

  void *self = current_game_mode();
  if (!self) {
    refuse("game mode not captured yet (left-click a block once)");
    return;
  }

  // this が古くなっていないか: vtable
  // と、持ち主(this+8)が、捕まえたプレイヤーと同じか
  std::uintptr_t vt = 0;
  std::uintptr_t owner = 0;
  if (!safe_read(self, &vt, sizeof(vt)) ||
      !safe_read(reinterpret_cast<const void *>(
                     reinterpret_cast<std::uintptr_t>(self) + 8),
                 &owner, sizeof(owner)) ||
      owner != g_player.load()) {
    g_game_mode = nullptr;
    refuse("game mode looks stale (left-click a block once)");
    return;
  }

  if (!class_allowed(vt)) {
    refuse("this game mode is not allowed (creative / spectator / unknown): "
           "left-click a block "
           "in survival once");
    return;
  }

  for (const auto &r : work) {
    alignas(16) std::int32_t pos[4] = {r.pos.x, r.pos.y, r.pos.z, 0};
    alignas(16) std::uint8_t arg4[64] = {}; // 4つ目の引数: 大きめのゼロ領域

    if (!g_quiet.load()) {
      const auto age_ms = GetTickCount64() - g_last_real_tick.load();
      log(std::format("[mcnote] attack calling: this=0x{:X} pos=({}, {}, {}) "
                      "face={} (last real click {} ms ago)",
                      reinterpret_cast<std::uintptr_t>(self), pos[0], pos[1],
                      pos[2], r.face, age_ms));
    }

    bool ret = false;
    g_attacking = true;
    const bool ok = guarded_start(self, pos, static_cast<std::uint32_t>(r.face),
                                  arg4, &ret);
    g_attacking = false;
    if (!ok) {
      g_game_mode = nullptr;
      log("[mcnote] attack failed (exception in game call): game mode invalid; "
          "stopping");
      player_stop();
      break;
    }
    if (!g_quiet.load())
      log(std::format("[mcnote] attack done: ret={} arg4[0]={}", ret, arg4[0]));

    if (const StopFn stop = resolve_stop(self)) {
      g_attacking = true;
      const bool stop_ok =
          guarded_stop(stop, self, reinterpret_cast<const BlockPos *>(pos));
      g_attacking = false;
      if (!stop_ok) {
        g_game_mode = nullptr;
        log("[mcnote] stop failed (exception in game call); stopping");
        player_stop();
        break;
      }
      if (!g_quiet.load())
        log("[mcnote] stop called");
    } else if (!g_quiet.load()) {
      log("[mcnote] stop skipped: vtable slot layout mismatch (game updated?)");
    }
  }
}