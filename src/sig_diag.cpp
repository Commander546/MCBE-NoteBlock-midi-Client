#include <windows.h>

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

#include "log.hpp"
#include "sig_scan.hpp"

namespace {

void diag(const char *name, std::string_view pattern) {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  const auto hits = sig_find_all(pattern);
  log(std::format("[mcnote] diag '{}': {} hit(s)", name, hits.size()));

  for (std::size_t i = 0; i < hits.size() && i < 8; ++i) {
    const auto *p = reinterpret_cast<const std::uint8_t *>(hits[i]);
    std::string bytes;
    for (int k = 0; k < 24; ++k)
      bytes += std::format("{:02X} ", p[k]);
    log(std::format("[mcnote]   rva=+0x{:X}: {}", hits[i] - base, bytes));
  }
}

} // namespace

void run_sig_diag() {
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  log(std::format("[mcnote] diag: module base=0x{:X}", base));

  // 音程の書き込み: mov [reg+0xF8],al の直後に or byte ptr [reg+0x54],1
  diag("write (rex)", "41 88 ?? F8 00 00 00 41 80 ?? 54 01");
  diag("write (no rex)", "88 ?? F8 00 00 00 80 ?? 54 01");

  // 音程の読み込み: movzx r,[reg+0xF8] のあとに、vtableから仮想関数を呼ぶ
  diag("read", "0F B6 ?? F8 00 00 00 48 8B ?? 48 8B ?? ?? 02 00 00");
}