#pragma once
#include <windows.h>

#include <format>
#include <string>

inline std::string thread_info() {
  std::string name = "(no name)";
  PWSTR w = nullptr;
  if (SUCCEEDED(GetThreadDescription(GetCurrentThread(), &w)) && w) {
    const int n =
        WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (n > 1) {
      name.assign(n - 1, '\0');
      WideCharToMultiByte(CP_UTF8, 0, w, -1, name.data(), n, nullptr, nullptr);
    }
    LocalFree(w);
  }
  return std::format("tid={} name={}", GetCurrentThreadId(), name);
}

inline bool on_main_thread() {
  thread_local int cached = -1;
  if (cached < 0) cached = thread_info().find("MINECRAFT MAIN THREAD") != std::string::npos;
  return cached == 1;
}