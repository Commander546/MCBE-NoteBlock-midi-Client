#include "note_table.hpp"

#include <algorithm>
#include <map>
#include <mutex>
#include <tuple>

namespace {

std::mutex g_mutex;
std::map<std::tuple<int, int, int>, NoteEntry> g_table;

} // namespace

void note_table_learn(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t instrument,
                      std::uint32_t note) {
  if (note > 24 || instrument > 15) return; // 範囲外の値は、読み違いとして捨てる
  std::lock_guard lock(g_mutex);
  g_table[{x, y, z}] = {x, y, z, static_cast<std::uint8_t>(instrument),
                        static_cast<std::uint8_t>(note)};
}

std::vector<NoteEntry> note_table_snapshot() {
  std::vector<NoteEntry> out;
  {
    std::lock_guard lock(g_mutex);
    for (const auto &[key, e] : g_table) out.push_back(e);
  }
  std::ranges::sort(out, [](const NoteEntry &a, const NoteEntry &b) {
    return std::tie(a.instrument, a.note, a.x, a.y, a.z) <
           std::tie(b.instrument, b.note, b.x, b.y, b.z);
  });
  return out;
}

void note_table_clear() {
  std::lock_guard lock(g_mutex);
  g_table.clear();
}