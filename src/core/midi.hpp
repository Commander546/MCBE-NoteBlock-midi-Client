#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mcnote::core {

struct MidiNote {
  std::uint32_t tick;     // MIDIのtick(曲の先頭から)
  std::uint8_t pitch;     // 0..127 (60 = 中央のド)
  std::uint8_t velocity;
  std::uint8_t channel;   // 0..15 (9 がドラム)
  std::uint16_t track;
};

struct TempoChange {
  std::uint32_t tick;
  std::uint32_t us_per_quarter; // 四分音符1個あたりのマイクロ秒
};

struct MidiFile {
  std::uint16_t ticks_per_quarter = 480;
  std::uint16_t track_count = 0;
  std::vector<MidiNote> notes;     // tick順
  std::vector<TempoChange> tempos; // tick順(先頭は必ず tick=0)
};

// 失敗したら error に理由(英語)を入れて false を返す
bool parse_midi(const std::vector<std::uint8_t> &data, MidiFile &out, std::string &error);
bool load_midi_file(const std::string &path, MidiFile &out, std::string &error);

// MIDIのtickを秒に変換する(テンポの変化を考慮)
double midi_tick_to_seconds(const MidiFile &m, std::uint32_t tick);

} // namespace mcnote::core