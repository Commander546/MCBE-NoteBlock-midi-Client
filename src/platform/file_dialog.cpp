#include "file_dialog.hpp"

#include <windows.h>

#include <commdlg.h>

#include <atomic>
#include <mutex>
#include <string>
#include <thread>

namespace {

std::atomic_bool g_busy{false};
std::mutex g_mutex;
std::vector<std::filesystem::path> g_result;
bool g_has_result = false;

void dialog_thread(HWND owner, bool multi) {
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  std::vector<wchar_t> buf(32768, L'\0');
  OPENFILENAMEW ofn{};
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = owner;
  ofn.lpstrFilter =
      L"MIDI (*.mid;*.midi)\0*.mid;*.midi\0All files (*.*)\0*.*\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFile = buf.data();
  ofn.nMaxFile = static_cast<DWORD>(buf.size());
  ofn.lpstrTitle = L"MIDIファイルを選択";
  ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
              OFN_HIDEREADONLY |
              OFN_NOCHANGEDIR | // ゲームの作業フォルダを変えない
              (multi ? OFN_ALLOWMULTISELECT : 0);

  std::vector<std::filesystem::path> picked;
  if (GetOpenFileNameW(&ofn)) {
    // 1つだけ: "フルパス\0\0"、複数: "フォルダ\0ファイル1\0ファイル2\0\0"
    const wchar_t *p = buf.data();
    const std::wstring first(p);
    p += first.size() + 1;
    if (*p == L'\0') {
      picked.emplace_back(first);
    } else {
      while (*p) {
        const std::wstring name(p);
        picked.emplace_back(std::filesystem::path(first) / name);
        p += name.size() + 1;
      }
    }
  }

  {
    std::lock_guard lock(g_mutex);
    g_result = std::move(picked);
    g_has_result = !g_result.empty();
  }
  CoUninitialize();
  g_busy = false;
}

} // namespace

bool file_dialog_open(HWND owner, bool multi) {
  if (g_busy.exchange(true))
    return false;
  std::thread(dialog_thread, owner, multi).detach();
  return true;
}

bool file_dialog_busy() { return g_busy.load(); }

bool file_dialog_take(std::vector<std::filesystem::path> &out) {
  std::lock_guard lock(g_mutex);
  if (!g_has_result)
    return false;
  out = std::move(g_result);
  g_result.clear();
  g_has_result = false;
  return true;
}