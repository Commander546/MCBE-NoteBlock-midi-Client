#include "player.hpp"

#include <windows.h>
#include <timeapi.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <mutex>
#include <thread>
#include <utility>

#include "attack.hpp"
#include "core/midi.hpp"
#include "core/schedule.hpp"
#include "log.hpp"
#include "note_table.hpp"

namespace {

using Clock = std::chrono::steady_clock;

struct Event {
  std::uint32_t tick;
  BlockPos pos;
};

std::mutex g_mutex;
mcnote::core::MidiFile g_midi;
bool g_loaded = false;
std::string g_message = "no file loaded";
std::size_t g_hits = 0;
std::size_t g_missing = 0;
double g_length_sec = 0;

std::atomic_bool g_playing{false};
std::atomic_bool g_stop{false};
std::atomic<std::uint32_t> g_pos_tick{0};

void set_message(std::string m) {
  std::lock_guard lock(g_mutex);
  g_message = std::move(m);
}

void play_thread(std::vector<Event> events, HWND hwnd, int face) {
  timeBeginPeriod(1); // 待ち時間の精度を1msに上げる
  const auto start = Clock::now() + std::chrono::seconds(1); // 最初の音までの余裕

  std::size_t i = 0;
  while (i < events.size() && !g_stop.load()) {
    const auto target =
        start + std::chrono::milliseconds(static_cast<long long>(events[i].tick) * 50);
    while (!g_stop.load()) {
      const auto now = Clock::now();
      if (now >= target) break;
      std::this_thread::sleep_for(
          std::min<Clock::duration>(target - now, std::chrono::milliseconds(2)));
    }
    if (g_stop.load()) break;

    const auto tick = events[i].tick;
    while (i < events.size() && events[i].tick == tick) { // 同じtickの音をまとめて依頼
      request_attack(hwnd, events[i].pos.x, events[i].pos.y, events[i].pos.z, face);
      ++i;
    }
    g_pos_tick = tick;
  }

  timeEndPeriod(1);
  attack_set_quiet(false);
  set_message(g_stop.load() ? "stopped" : "finished");
  g_playing = false;
}

} // namespace

bool player_load(std::string path) {
  // エクスプローラーの「パスのコピー」は前後に " が付くので取り除く
  while (!path.empty() && (path.front() == '"' || path.front() == ' ')) path.erase(path.begin());
  while (!path.empty() && (path.back() == '"' || path.back() == ' ')) path.pop_back();

  mcnote::core::MidiFile m;
  std::string err;
  if (!mcnote::core::load_midi_file(path, m, err)) {
    std::lock_guard lock(g_mutex);
    g_loaded = false;
    g_message = "load failed: " + err;
    return false;
  }

  std::lock_guard lock(g_mutex);
  g_midi = std::move(m);
  g_loaded = true;
  g_message = std::format("loaded: {} notes, {:.0f} BPM", g_midi.notes.size(),
                          60e6 / g_midi.tempos.front().us_per_quarter);
  return true;
}

std::vector<TrackInfo> player_tracks() {
  std::lock_guard lock(g_mutex);
  std::map<std::pair<int, int>, TrackInfo> by_key;
  for (const auto &n : g_midi.notes) {
    auto &t = by_key[{n.track, n.channel}];
    t.track = n.track;
    t.channel = n.channel;
    ++t.count;
    t.lo = std::min<int>(t.lo, n.pitch);
    t.hi = std::max<int>(t.hi, n.pitch);
  }
  std::vector<TrackInfo> out;
  for (const auto &[key, t] : by_key) out.push_back(t);
  return out;
}

bool player_play(HWND hwnd, const PlayOptions &opt) {
  if (g_playing.load()) return false;
  if (!attack_ready()) {
    set_message("left-click any block once (UI closed) first");
    return false;
  }

  mcnote::core::ScheduleOptions so;
  so.low_pitch = opt.base_pitch;
  so.high_pitch = opt.base_pitch + 24;
  so.octave_shift = opt.octave_shift;
  so.track = opt.track;
  so.channel = opt.channel;
  so.max_per_tick = 4;

  mcnote::core::Schedule sc;
  {
    std::lock_guard lock(g_mutex);
    if (!g_loaded) {
      g_message = "load a MIDI file first";
      return false;
    }
    sc = mcnote::core::build_schedule(g_midi, so);
  }

  // ブロックの音程(0..24) -> 座標
  std::map<int, BlockPos> block_of;
  for (const auto &e : note_table_snapshot()) {
    if (e.instrument == opt.instrument) block_of.try_emplace(e.note, BlockPos{e.x, e.y, e.z});
  }

  std::vector<Event> events;
  std::size_t missing = 0;
  for (const auto &h : sc.hits) {
    const auto it = block_of.find(h.pitch - opt.base_pitch);
    if (it == block_of.end()) {
      ++missing;
      continue;
    }
    events.push_back({h.game_tick, it->second});
  }

  {
    std::lock_guard lock(g_mutex);
    g_hits = events.size();
    g_missing = missing;
    g_length_sec = sc.total_ticks / 20.0;
  }
  if (events.empty()) {
    set_message("nothing to play (calibrate first?)");
    return false;
  }

  g_stop = false;
  g_pos_tick = 0;
  g_playing = true;
  attack_set_quiet(true);
  set_message("playing");
  std::thread(play_thread, std::move(events), hwnd, opt.face).detach();
  return true;
}

void player_stop() { g_stop = true; }

PlayerStatus player_status() {
  PlayerStatus s;
  std::lock_guard lock(g_mutex);
  s.loaded = g_loaded;
  s.playing = g_playing.load();
  s.message = g_message;
  s.hits = g_hits;
  s.missing = g_missing;
  s.length_sec = g_length_sec;
  s.position_sec = g_pos_tick.load() / 20.0;
  return s;
}