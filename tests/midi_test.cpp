#include <algorithm>
#include <cstdio>
#include <map>

#include "core/midi.hpp"
#include "core/schedule.hpp"

int main(int argc, char **argv) {
  using namespace mcnote::core;
  if (argc < 2) { std::puts("usage: midi_test file.mid"); return 1; }

  MidiFile m;
  std::string err;
  if (!load_midi_file(argv[1], m, err)) { std::printf("error: %s\n", err.c_str()); return 1; }

  std::printf("tracks=%u ticks/quarter=%u notes=%zu tempo changes=%zu first BPM=%.1f\n",
              m.track_count, m.ticks_per_quarter, m.notes.size(), m.tempos.size(),
              60e6 / m.tempos.front().us_per_quarter);

  struct Stat { std::size_t count = 0; int lo = 127, hi = 0; };
  std::map<int, Stat> per_track;
  for (const auto &n : m.notes) {
    auto &s = per_track[n.track * 100 + n.channel];
    ++s.count;
    s.lo = std::min<int>(s.lo, n.pitch);
    s.hi = std::max<int>(s.hi, n.pitch);
  }
  for (const auto &[key, s] : per_track) {
    std::printf("  track %d ch %d: %zu notes, pitch %d..%d%s\n", key / 100, key % 100, s.count,
                s.lo, s.hi, key % 100 == 9 ? " (drums)" : "");
  }

  const Schedule sc = build_schedule(m, {});
  std::printf("hits=%zu total_ticks=%u (%.1f sec) folded=%zu dropped=%zu\n", sc.hits.size(),
              sc.total_ticks, sc.total_ticks / 20.0, sc.folded, sc.dropped);
  for (std::size_t i = 0; i < sc.hits.size() && i < 20; ++i) {
    std::printf("  tick %u: pitch %u\n", sc.hits[i].game_tick, sc.hits[i].pitch);
  }
  return 0;
}