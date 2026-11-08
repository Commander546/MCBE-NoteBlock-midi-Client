#include <windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

#include "log.hpp"

void install_present_hook();
void install_note_probe();

namespace {

HMODULE g_module{};
std::mutex g_log_mutex;

std::filesystem::path log_path() {
  wchar_t buf[MAX_PATH]{};
  GetModuleFileNameW(g_module, buf, MAX_PATH);
  return std::filesystem::path(buf).parent_path() / L"mcnote.log";
}

void setup_console() {
  if (AllocConsole()) {
    FILE *f{};
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    SetConsoleTitleW(L"mcnote debug");

    if (HWND wnd = GetConsoleWindow()) {
      if (HMENU menu = GetSystemMenu(wnd, FALSE)) {
        DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
      }
    }
  }
}

DWORD WINAPI main_thread(LPVOID) {
  setup_console();
  log("[mcnote] loaded");
  log("[mcnote] log file: " + log_path().string());
  install_present_hook();
  install_note_probe();
  return 0;
}

} // namespace

// ここは無名namespaceの外(他のcppから呼ぶため)
void log(const std::string &msg) {
  std::lock_guard lock(g_log_mutex);
  std::printf("%s\n", msg.c_str());
  std::fflush(stdout);
  std::ofstream(log_path(), std::ios::app) << msg << '\n';
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_module = module;
    DisableThreadLibraryCalls(module);
    if (HANDLE t = CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr)) {
      CloseHandle(t);
    }
  }
  return TRUE;
}