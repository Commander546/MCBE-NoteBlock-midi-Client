#pragma once
#include <atomic>

// ファンクションキーのデバッグ機能を有効にするか。UIの Enable Debugs で切り替える(起動時はオフ)
inline std::atomic_bool &debug_flag() {
  static std::atomic_bool flag{false};
  return flag;
}
inline bool debug_enabled() { return debug_flag().load(std::memory_order_relaxed); }
inline void set_debug_enabled(bool on) { debug_flag().store(on); }