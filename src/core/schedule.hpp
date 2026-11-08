#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/midi.hpp"

namespace mcnote::core {

struct Hit {
  std::uint32_t game_tick; // 再生開始からのゲームtick(20tick = 1秒)
  std::uint8_t pitch;      // MIDIの音高(音域に折り返した後)
};

struct ScheduleOptions {
  int low_pitch = 54;          // 使える一番低い音(ハープなら F#3 = 54)
  int high_pitch = 78;         // 使える一番高い音(F#5 = 78)
  int octave_shift = 0;        // 全体を何オクターブずらすか
  int track = -1;              // -1 なら全トラック
  int channel = -1;            // -1 なら全チャンネル
  bool skip_percussion = true; // チャンネル10(ドラム)を除く
  int max_per_tick = 4;        // 同じtickに叩く最大数(超えた分は低い音から捨てる)
};

struct Schedule {
  std::vector<Hit> hits; // tick順、同じtick内は高い音から
  std::uint32_t total_ticks = 0;
  std::size_t folded = 0;  // 音域の外で、オクターブを動かした数
  std::size_t dropped = 0; // 同じtickに多すぎて捨てた数
};

Schedule build_schedule(const MidiFile &m, const ScheduleOptions &opt);

} // namespace mcnote::core