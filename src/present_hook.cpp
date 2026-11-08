#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <wrl/client.h>

#include <atomic>

#include <safetyhook.hpp>

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include "attack.hpp"
#include "log.hpp"
#include "note_scan.hpp"
#include "slot_trace.hpp"
#include "thread_info.hpp"

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

#include "note_table.hpp"

#include <format>

#include "player.hpp"
#include "player_probe.hpp"

using Microsoft::WRL::ComPtr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM,
                                                             LPARAM);

namespace {

// ---------------------------------------------------------------------------
// 状態
// ---------------------------------------------------------------------------

safetyhook::InlineHook g_present_hook;
safetyhook::InlineHook g_resize_hook;
safetyhook::InlineHook g_clip_hook;
safetyhook::InlineHook g_setpos_hook;

bool g_cursor_hooks_ok = false;
RECT g_game_clip{}; // ゲームが最後に設定したClipCursorの範囲
bool g_game_has_clip = false;

struct State {
  bool initialized = false;
  std::atomic_bool visible = false;
  HWND hwnd{};
  WNDPROC orig_wndproc{};
  IDXGISwapChain *swap_chain{};
  ComPtr<ID3D11Device> device;
  ComPtr<ID3D11DeviceContext> context;
  ComPtr<ID3D11RenderTargetView> rtv;
} g;

int g_face = 1;
int g_instrument = 0; // 使う楽器の番号(UIの "instrument")

// F9: UIを開かずに攻撃する。対象は「最後に左クリックしたブロック」
void handle_attack_key() {
  static bool was_down = false;
  const bool down = GetForegroundWindow() == g.hwnd &&
                    (GetAsyncKeyState(VK_F9) & 0x8000) != 0;

  if (down && !was_down && !g.visible) {
    BlockPos p{};
    if (attack_last_clicked(p)) {
      request_attack(g.hwnd, p.x, p.y, p.z, g_face);
    } else {
      log("[mcnote] F9: no block clicked yet");
    }
  }
  was_down = down;
}

// ---------------------------------------------------------------------------
// カーソルの閉じ込め(ClipCursor)の制御
// ---------------------------------------------------------------------------

// フックを経由せず、本物のClipCursorを呼ぶ
void apply_clip(const RECT *rect) {
  if (g_cursor_hooks_ok) {
    g_clip_hook.stdcall<BOOL>(rect);
  } else {
    ClipCursor(rect);
  }
}

void on_ui_opened() { apply_clip(nullptr); }

void on_ui_closed() { apply_clip(g_game_has_clip ? &g_game_clip : nullptr); }

BOOL WINAPI hooked_clip_cursor(const RECT *rect) {
  // ゲームの意図を覚えておく(UIを閉じたとき復元するため)
  g_game_has_clip = (rect != nullptr);
  if (rect)
    g_game_clip = *rect;

  if (g.visible)
    return TRUE; // UI中は閉じ込めさせない
  return g_clip_hook.stdcall<BOOL>(rect);
}

BOOL WINAPI hooked_set_cursor_pos(int x, int y) {
  if (g.visible)
    return TRUE; // UI中はカーソルを中央に戻させない
  return g_setpos_hook.stdcall<BOOL>(x, y);
}

// ---------------------------------------------------------------------------
// ウィンドウプロシージャ
// ---------------------------------------------------------------------------

bool is_mouse_press_or_wheel(UINT msg) {
  switch (msg) {
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
  case WM_XBUTTONDOWN:
  case WM_XBUTTONDBLCLK:
  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL:
    return true;
  default:
    return false;
  }
}

bool is_key_press_or_char(UINT msg) {
  return msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN || msg == WM_CHAR ||
         msg == WM_SYSCHAR || msg == WM_UNICHAR;
}

LRESULT CALLBACK hooked_wndproc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  if (msg == kMsgAttack) {
    attack_on_ui_thread();
    return 0;
  }
  if (g.visible) {
    // imguiには常に入力を渡す(戻り値ではゲームへの伝達を止めない)
    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);

    // 押下系だけを止める。離す系は通して、ゲーム側が押しっぱなしにならないようにする。
    // マウス移動とWM_SETCURSORも通すので、ゲームのカーソル表示はそのまま使われる。
    const ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureMouse && is_mouse_press_or_wheel(msg))
      return 0;
    if (io.WantCaptureKeyboard && is_key_press_or_char(msg))
      return 0;
  }
  return CallWindowProcW(g.orig_wndproc, hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// imgui の初期化と描画
// ---------------------------------------------------------------------------

bool create_rtv(IDXGISwapChain *sc) {
  ComPtr<ID3D11Texture2D> back_buffer;
  if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), &back_buffer)))
    return false;
  return SUCCEEDED(
      g.device->CreateRenderTargetView(back_buffer.Get(), nullptr, &g.rtv));
}

bool init_imgui(IDXGISwapChain *sc) {
  if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), &g.device))) {
    log("[mcnote] init failed: not a D3D11 device");
    return false;
  }
  g.device->GetImmediateContext(&g.context);

  DXGI_SWAP_CHAIN_DESC desc{};
  sc->GetDesc(&desc);
  g.hwnd = desc.OutputWindow;
  if (!g.hwnd) {
    log("[mcnote] init failed: no window");
    return false;
  }

  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;
  io.ConfigFlags |=
      ImGuiConfigFlags_NoMouseCursorChange; // カーソルの形はゲームに任せる
  ImGui::StyleColorsDark();

  ImGui_ImplWin32_Init(g.hwnd);
  ImGui_ImplDX11_Init(g.device.Get(), g.context.Get());

  g.orig_wndproc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
      g.hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&hooked_wndproc)));

  g.swap_chain = sc;
  g.initialized = true;
  log("[mcnote] imgui initialized");
  log("[mcnote] imgui initialized, render thread: " + thread_info());
  return true;
}

void draw_ui() {

  ImGui::SetNextWindowSize(ImVec2(420, 360), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("mcnote")) {
    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
    ImGui::TextDisabled("Insert: show/hide");
    ImGui::Separator();

    if (ImGui::Button("Scan note blocks"))
      request_note_scan();
    ImGui::SameLine();
    ImGui::TextUnformatted(note_scan_running() ? "scanning..." : "idle");

    const auto notes = note_scan_results();
    ImGui::Text("found: %zu", notes.size());

    if (ImGui::BeginChild("notes", ImVec2(0, 200), true)) {
      for (const auto &n : notes) {
        std::uint8_t live = 0;
        const bool ok = read_note_live(n.obj, live);
        ImGui::Text("note@0x%llX (%d, %d, %d) live=%d",
                    static_cast<unsigned long long>(n.obj + 0xF8), n.x, n.y,
                    n.z, ok ? static_cast<int>(live) : -1);
      }
    }

    ImGui::EndChild(); // BeginChildの結果に関係なく、必ず1回呼ぶ

    ImGui::Separator();
    static int target[3] = {0, 0, 0};
    static int face = 1;
    ImGui::InputInt3("block pos", target);
    ImGui::SliderInt("face", &face, 0, 5);
    if (ImGui::Button("Use last clicked block")) {
      BlockPos p{};
      if (attack_last_clicked(p)) {
        target[0] = p.x;
        target[1] = p.y;
        target[2] = p.z;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Attack"))
      request_attack(g.hwnd, target[0], target[1], target[2], face);
    ImGui::TextUnformatted(attack_state());

    {
      const long long age = attack_beat_age_ms();
      if (age < 0)
        ImGui::TextUnformatted("heartbeat: none");
      else
        ImGui::Text("heartbeat: %lld ms ago", age);
      if (ImGui::Button("Measure idle (3s)"))
        slot_trace_measure_idle();
    }
  }

  ImGui::Separator();
  if (ImGui::Button("Calibrate (hit scanned blocks)")) {
    std::vector<BlockPos> targets;
    for (const auto &n : note_scan_results()) {
      if (n.x == 0 && n.y == 0 && n.z == 0)
        continue;
      const BlockPos p{n.x, n.y, n.z};
      const bool dup =
          std::any_of(targets.begin(), targets.end(), [&](const BlockPos &q) {
            return q.x == p.x && q.y == p.y && q.z == p.z;
          });
      if (!dup)
        targets.push_back(p);
    }
    note_table_clear();
    std::thread([hwnd = g.hwnd, targets, face = g_face] {
      for (const auto &p : targets) {
        request_attack(hwnd, p.x, p.y, p.z, face);
        Sleep(150); // 1個ずつ、間隔を空けて叩く
      }
    }).detach();
  }

  const auto table = note_table_snapshot();
  ImGui::Text("learned: %zu", table.size());

  static int inst = 0;
  ImGui::InputInt("instrument", &inst);
  std::string cover;
  for (int k = 0; k <= 24; ++k) {
    const bool has =
        std::any_of(table.begin(), table.end(), [&](const NoteEntry &e) {
          return e.instrument == inst && e.note == k;
        });
    cover += has ? 'X' : '.';
  }
  ImGui::Text("pitch 0..24: %s", cover.c_str());

  if (ImGui::Button("Learn pitches from scan")) {
    note_table_clear();
    for (const auto &n : note_scan_results()) {
      if (n.x == 0 && n.y == 0 && n.z == 0)
        continue;
      note_table_learn(n.x, n.y, n.z, static_cast<std::uint32_t>(inst), n.note);
    }
  }

  ImGui::Separator();
  static char midi_path[260] = "";
  ImGui::InputText("midi path", midi_path, sizeof(midi_path));
  if (ImGui::Button("Load MIDI"))
    player_load(midi_path);

  static int sel = -1; // -1 = 全トラック
  const auto tracks = player_tracks();
  if (sel >= static_cast<int>(tracks.size()))
    sel = -1;
  const std::string preview =
      sel < 0 ? "all tracks"
              : std::format("track {} ch {} ({} notes, {}..{})",
                            tracks[sel].track, tracks[sel].channel,
                            tracks[sel].count, tracks[sel].lo, tracks[sel].hi);
  if (ImGui::BeginCombo("track", preview.c_str())) {
    if (ImGui::Selectable("all tracks", sel < 0))
      sel = -1;
    for (int i = 0; i < static_cast<int>(tracks.size()); ++i) {
      const std::string label = std::format(
          "track {} ch {} ({} notes, {}..{})", tracks[i].track,
          tracks[i].channel, tracks[i].count, tracks[i].lo, tracks[i].hi);
      if (ImGui::Selectable(label.c_str(), sel == i))
        sel = i;
    }
    ImGui::EndCombo();
  }

  static int base_pitch = 54;
  static int octave = 0;
  ImGui::InputInt("base pitch (MIDI)", &base_pitch);
  ImGui::InputInt("octave shift", &octave);

  if (ImGui::Button("Play")) {
    PlayOptions po;
    po.instrument = inst; // 上の "instrument" の値
    po.base_pitch = base_pitch;
    po.octave_shift = octave;
    po.face = g_face;
    if (sel >= 0) {
      po.track = tracks[sel].track;
      po.channel = tracks[sel].channel;
    }
    player_play(g.hwnd, po);
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop"))
    player_stop();

  if (ImGui::Button("Prepare & Play")) {
    PlayOptions po;
    po.instrument = g_instrument;
    po.base_pitch = base_pitch;
    po.octave_shift = octave;
    po.face = g_face;
    if (sel >= 0) {
      po.track = tracks[sel].track;
      po.channel = tracks[sel].channel;
    }

    // スキャン → 全部叩いて音程を学習 → 再生、を順番に実行する
    std::thread([hwnd = g.hwnd, po] {
      request_note_scan();
      while (note_scan_running())
        Sleep(50);

      std::vector<BlockPos> targets;
      for (const auto &n : note_scan_results()) {
        const BlockPos p{n.x, n.y, n.z};
        const bool dup =
            std::any_of(targets.begin(), targets.end(), [&](const BlockPos &q) {
              return q.x == p.x && q.y == p.y && q.z == p.z;
            });
        if (!dup)
          targets.push_back(p);
      }

      note_table_clear();
      for (const auto &p : targets) {
        request_attack(hwnd, p.x, p.y, p.z, po.face);
        Sleep(150);
      }
      Sleep(400); // 最後の通知が届くのを待つ
      player_play(hwnd, po);
    }).detach();
  }
  ImGui::SameLine();

  const auto st = player_status();
  ImGui::TextWrapped("%s", st.message.c_str());
  ImGui::Text("hits %zu  missing %zu  %.1f / %.1f s", st.hits, st.missing,
              st.position_sec, st.length_sec);

  ImGui::Separator();
  static int snap_label = 0;
  ImGui::InputInt("mode (0 surv,1 crea,2 adv,3 spec)", &snap_label);
  if (ImGui::Button("Snapshot player"))
    player_probe_snapshot(snap_label);
  ImGui::SameLine();
  if (ImGui::Button("Find game type"))
    player_probe_find();

  ImGui::End(); // Beginの結果に関係なく、必ず1回呼ぶ
}

// Insertキーで表示を切り替える。ゲームがフォーカスされているときだけ反応する。
void handle_toggle_key() {
  static bool was_down = false;
  const bool down = GetForegroundWindow() == g.hwnd &&
                    (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;

  if (down && !was_down) {
    g.visible = !g.visible;
    if (g.visible) {
      on_ui_opened();
    } else {
      on_ui_closed();
    }
  }
  was_down = down;
}

void render_frame(IDXGISwapChain *sc) {
  if (!g.initialized && !init_imgui(sc))
    return;

  if (g.swap_chain != sc) { // スワップチェーンが作り直された
    g.rtv.Reset();
    g.swap_chain = sc;
  }

  handle_toggle_key();
  handle_attack_key();
  slot_trace_poll_keys(g.hwnd);

  if (!g.visible)
    return;
  if (!g.rtv && !create_rtv(sc))
    return;

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame(); // OSのカーソル位置を読む(ゲームと同じ位置)
  ImGui::NewFrame();
  draw_ui();
  ImGui::Render();

  ID3D11RenderTargetView *rtv = g.rtv.Get();
  g.context->OMSetRenderTargets(1, &rtv, nullptr);
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

HRESULT STDMETHODCALLTYPE hooked_present(IDXGISwapChain *sc, UINT sync,
                                         UINT flags) {
  render_frame(sc);
  return g_present_hook.stdcall<HRESULT>(sc, sync, flags);
}

HRESULT STDMETHODCALLTYPE hooked_resize_buffers(IDXGISwapChain *sc, UINT count,
                                                UINT w, UINT h, DXGI_FORMAT fmt,
                                                UINT flags) {
  g.rtv.Reset(); // バックバッファを参照したままだとResizeBuffersが失敗する
  return g_resize_hook.stdcall<HRESULT>(sc, count, w, h, fmt, flags);
}

// ---------------------------------------------------------------------------
// フック対象のアドレス探し
// ---------------------------------------------------------------------------

struct Targets {
  void *present{};
  void *resize_buffers{};
};

// ダミーのスワップチェーンを作って、vtableから関数アドレスを取る
Targets find_targets() {
  WNDCLASSEXW wc{sizeof(wc)};
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = L"mcnote_dummy";
  RegisterClassExW(&wc);
  HWND hwnd = CreateWindowW(wc.lpszClassName, L"", WS_OVERLAPPEDWINDOW, 0, 0,
                            100, 100, nullptr, nullptr, wc.hInstance, nullptr);

  DXGI_SWAP_CHAIN_DESC desc{};
  desc.BufferCount = 1;
  desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.OutputWindow = hwnd;
  desc.SampleDesc.Count = 1;
  desc.Windowed = TRUE;
  desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  ComPtr<IDXGISwapChain> sc;
  ComPtr<ID3D11Device> dev;
  ComPtr<ID3D11DeviceContext> ctx;
  D3D_FEATURE_LEVEL fl{};
  const HRESULT hr = D3D11CreateDeviceAndSwapChain(
      nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
      D3D11_SDK_VERSION, &desc, &sc, &dev, &fl, &ctx);

  Targets t;
  if (SUCCEEDED(hr)) {
    void **vtbl = *reinterpret_cast<void ***>(sc.Get());
    t.present = vtbl[8];         // IDXGISwapChain::Present
    t.resize_buffers = vtbl[13]; // IDXGISwapChain::ResizeBuffers
  }

  sc.Reset();
  ctx.Reset();
  dev.Reset();
  DestroyWindow(hwnd);
  UnregisterClassW(wc.lpszClassName, wc.hInstance);
  return t;
}

void install_cursor_hooks() {
  HMODULE user32 = GetModuleHandleW(L"user32.dll");
  auto clip = safetyhook::InlineHook::create(
      reinterpret_cast<void *>(GetProcAddress(user32, "ClipCursor")),
      reinterpret_cast<void *>(&hooked_clip_cursor));
  auto setpos = safetyhook::InlineHook::create(
      reinterpret_cast<void *>(GetProcAddress(user32, "SetCursorPos")),
      reinterpret_cast<void *>(&hooked_set_cursor_pos));

  if (!clip || !setpos) {
    log("[mcnote] failed to hook cursor APIs");
    return;
  }
  g_clip_hook = std::move(*clip);
  g_setpos_hook = std::move(*setpos);
  g_cursor_hooks_ok = true;
  log("[mcnote] cursor hooks installed");
}

} // namespace

void install_present_hook() {
  const Targets t = find_targets();
  if (!t.present || !t.resize_buffers) {
    log("[mcnote] failed to find Present/ResizeBuffers");
    return;
  }

  auto present = safetyhook::InlineHook::create(
      t.present, reinterpret_cast<void *>(&hooked_present));
  auto resize = safetyhook::InlineHook::create(
      t.resize_buffers, reinterpret_cast<void *>(&hooked_resize_buffers));
  if (!present || !resize) {
    log("[mcnote] failed to create hooks");
    return;
  }
  g_present_hook = std::move(*present);
  g_resize_hook = std::move(*resize);
  log("[mcnote] Present/ResizeBuffers hooked");

  install_cursor_hooks();
}