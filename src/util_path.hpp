#pragma once
#include <filesystem>
#include <string>

// imguiの文字列(UTF-8)と、ファイルパス(UTF-16)の変換
inline std::string path_to_utf8(const std::filesystem::path &p) {
  const std::u8string u = p.u8string();
  return std::string(reinterpret_cast<const char *>(u.data()), u.size());
}

inline std::filesystem::path path_from_utf8(const std::string &s) {
  return std::filesystem::path(
      std::u8string(reinterpret_cast<const char8_t *>(s.data()), s.size()));
}