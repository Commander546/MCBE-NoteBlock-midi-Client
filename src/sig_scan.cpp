#include "sig_scan.hpp"

#include <windows.h>

#include <cstddef>

#include "log.hpp"

namespace {

struct Pattern {
  std::vector<std::uint8_t> bytes;
  std::vector<std::uint8_t> mask; // 1 なら比較、0 ならワイルドカード
};

int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

bool parse(std::string_view s, Pattern &out) {
  std::size_t i = 0;
  while (i < s.size()) {
    if (s[i] == ' ') { ++i; continue; }
    if (s[i] == '?') {
      while (i < s.size() && s[i] == '?') ++i;
      out.bytes.push_back(0);
      out.mask.push_back(0);
      continue;
    }
    if (i + 1 >= s.size()) return false;
    const int h = hex_digit(s[i]);
    const int l = hex_digit(s[i + 1]);
    if (h < 0 || l < 0) return false;
    out.bytes.push_back(static_cast<std::uint8_t>(h * 16 + l));
    out.mask.push_back(1);
    i += 2;
  }
  return !out.bytes.empty();
}

} // namespace

std::vector<std::uintptr_t> sig_find_all(std::string_view pattern) {
  std::vector<std::uintptr_t> hits;

  Pattern pat;
  if (!parse(pattern, pat)) {
    log("[mcnote] sig_find_all: bad pattern");
    return hits;
  }

  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
  const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
  const IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);

  const std::size_t len = pat.bytes.size();
  const std::uint8_t *pb = pat.bytes.data();
  const std::uint8_t *pm = pat.mask.data();

  for (WORD s = 0; s < nt->FileHeader.NumberOfSections; ++s) {
    if (!(sec[s].Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;

    const auto *begin = reinterpret_cast<const std::uint8_t *>(base + sec[s].VirtualAddress);
    const std::size_t size = sec[s].Misc.VirtualSize;
    if (size < len) continue;

    for (std::size_t off = 0; off + len <= size; ++off) {
      bool ok = true;
      for (std::size_t k = 0; k < len; ++k) {
        if (pm[k] && begin[off + k] != pb[k]) { ok = false; break; }
      }
      if (ok) hits.push_back(reinterpret_cast<std::uintptr_t>(begin + off));
    }
  }
  return hits;
}

std::uintptr_t sig_find(std::string_view pattern, int *count) {
  const auto hits = sig_find_all(pattern);
  if (count) *count = static_cast<int>(hits.size());
  return hits.size() == 1 ? hits[0] : 0;
}