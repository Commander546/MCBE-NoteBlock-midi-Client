#include "core/schedule.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace mcnote::core {

Schedule build_schedule(const MidiFile &m, const ScheduleOptions &opt) {
  Schedule s;
  const int lo = opt.low_pitch;
  const int hi = opt.high_pitch;
  if (hi - lo < 11) return s; // 1オクターブ未満の音域には、折り返せない

  for (const auto &n : m.notes) {
    if (opt.track >= 0 && n.track != opt.track) continue;
    if (opt.channel >= 0 && n.channel != opt.channel) continue;
    if (opt.skip_percussion && n.channel == 9) continue;

    int p = n.pitch + 12 * opt.octave_shift;
    bool moved = false;
    while (p < lo) { p += 12; moved = true; }
    while (p > hi) { p -= 12; moved = true; }
    if (moved) ++s.folded;

    const auto tick = static_cast<std::uint32_t>(
        std::llround(midi_tick_to_seconds(m, n.tick) * 20.0));
    s.hits.push_back({tick, static_cast<std::uint8_t>(p)});
  }

  // tick順、同じtick内は高い音から。同じtick・同じ音の重複は1つにする
  std::ranges::sort(s.hits, [](const Hit &a, const Hit &b) {
    return std::tuple(a.game_tick, -static_cast<int>(a.pitch)) <
           std::tuple(b.game_tick, -static_cast<int>(b.pitch));
  });
  const auto dup = std::ranges::unique(s.hits, [](const Hit &a, const Hit &b) {
    return a.game_tick == b.game_tick && a.pitch == b.pitch;
  });
  s.hits.erase(dup.begin(), s.hits.end());

  // 同じtickに叩く数を制限する(高い音を優先して残す)
  std::vector<Hit> kept;
  kept.reserve(s.hits.size());
  std::uint32_t cur = 0;
  bool first = true;
  int count = 0;
  for (const auto &h : s.hits) {
    if (first || h.game_tick != cur) { cur = h.game_tick; count = 0; first = false; }
    if (count++ < opt.max_per_tick) kept.push_back(h);
    else ++s.dropped;
  }
  s.hits = std::move(kept);
  s.total_ticks = s.hits.empty() ? 0 : s.hits.back().game_tick;
  return s;
}

} // namespace mcnote::core