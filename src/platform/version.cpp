#include "version.hpp"

#include <windows.h>

#include <format>
#include <vector>

std::string game_version_string() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);

  DWORD handle = 0;
  const DWORD size = GetFileVersionInfoSizeW(path, &handle);
  if (size == 0) return "unknown";

  std::vector<char> data(size);
  if (!GetFileVersionInfoW(path, 0, size, data.data())) return "unknown";

  VS_FIXEDFILEINFO *info = nullptr;
  UINT len = 0;
  if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<LPVOID *>(&info), &len) || !info)
    return "unknown";

  return std::format("{}.{}.{}.{}", HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                     HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS));
}