#include "core/midi.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace mcnote::core {
namespace {

struct Reader {
  const std::vector<std::uint8_t> &d;
  std::size_t pos;
  std::size_t end;

  bool has(std::size_t n) const { return pos + n <= end; }
  std::uint8_t u8() { return d[pos++]; }
  std::uint16_t u16() {
    const auto v = static_cast<std::uint16_t>((d[pos] << 8) | d[pos + 1]);
    pos += 2;
    return v;
  }
  std::uint32_t u32() {
    const std::uint32_t v = (static_cast<std::uint32_t>(d[pos]) << 24) |
                            (static_cast<std::uint32_t>(d[pos + 1]) << 16) |
                            (static_cast<std::uint32_t>(d[pos + 2]) << 8) | d[pos + 3];
    pos += 4;
    return v;
  }
  // 可変長の数値(7ビットずつ、最上位ビットが続きの印)
  bool vlq(std::uint32_t &out) {
    out = 0;
    for (int i = 0; i < 4; ++i) {
      if (!has(1)) return false;
      const std::uint8_t b = u8();
      out = (out << 7) | (b & 0x7F);
      if (!(b & 0x80)) return true;
    }
    return false;
  }
};

bool parse_track(const std::vector<std::uint8_t> &d, std::size_t begin, std::size_t end,
                 std::uint16_t track, MidiFile &out, std::string &error) {
  Reader r{d, begin, end};
  std::uint32_t tick = 0;
  std::uint8_t running = 0;

  while (r.pos < end) {
    std::uint32_t delta = 0;
    if (!r.vlq(delta)) { error = "bad delta time"; return false; }
    tick += delta;
    if (!r.has(1)) break;

    std::uint8_t status = d[r.pos];
    if (status & 0x80) {
      ++r.pos;
      running = status < 0xF0 ? status : 0; // メタ・SysExはランニングステータスを解除する
    } else if (running) {
      status = running; // 直前と同じ種類のメッセージ(状態の省略)
    } else {
      error = "bad running status";
      return false;
    }

    if (status == 0xFF) { // メタイベント
      if (!r.has(1)) { error = "truncated meta"; return false; }
      const std::uint8_t type = r.u8();
      std::uint32_t len = 0;
      if (!r.vlq(len) || !r.has(len)) { error = "truncated meta"; return false; }
      if (type == 0x51 && len == 3) { // テンポ
        const std::uint32_t us = (static_cast<std::uint32_t>(d[r.pos]) << 16) |
                                 (static_cast<std::uint32_t>(d[r.pos + 1]) << 8) | d[r.pos + 2];
        if (us > 0) out.tempos.push_back({tick, us});
      }
      r.pos += len;
      if (type == 0x2F) break; // トラックの終わり
    } else if (status == 0xF0 || status == 0xF7) { // SysEx
      std::uint32_t len = 0;
      if (!r.vlq(len) || !r.has(len)) { error = "truncated sysex"; return false; }
      r.pos += len;
    } else { // チャンネルメッセージ
      const std::uint8_t kind = status & 0xF0;
      const std::uint8_t ch = status & 0x0F;
      const std::size_t need = (kind == 0xC0 || kind == 0xD0) ? 1 : 2;
      if (!r.has(need)) { error = "truncated message"; return false; }
      const std::uint8_t a = r.u8();
      const std::uint8_t b = need == 2 ? r.u8() : 0;
      if (kind == 0x90 && b > 0) out.notes.push_back({tick, a, b, ch, track}); // ベロシティ0はノートオフ扱い
    }
  }
  return true;
}

} // namespace

bool parse_midi(const std::vector<std::uint8_t> &data, MidiFile &out, std::string &error) {
  out = {};
  if (data.size() < 14 || data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd') {
    error = "not a MIDI file";
    return false;
  }

  Reader r{data, 4, data.size()};
  const std::uint32_t header_len = r.u32();
  if (header_len < 6 || !r.has(header_len)) { error = "bad header"; return false; }
  r.u16(); // フォーマット(0/1/2)。今回は気にしない
  r.u16(); // トラック数。実際に読んだ数を使う
  const std::uint16_t division = r.u16();
  r.pos = 8 + header_len;

  if (division & 0x8000) { error = "SMPTE time division is not supported"; return false; }
  if (division == 0) { error = "bad time division"; return false; }
  out.ticks_per_quarter = division;

  std::uint16_t track = 0;
  while (r.has(8)) {
    const bool is_track = data[r.pos] == 'M' && data[r.pos + 1] == 'T' &&
                          data[r.pos + 2] == 'r' && data[r.pos + 3] == 'k';
    r.pos += 4;
    const std::uint32_t len = r.u32();
    if (!r.has(len)) { error = "truncated track"; return false; }
    if (is_track) {
      if (!parse_track(data, r.pos, r.pos + len, track, out, error)) return false;
      ++track;
    }
    r.pos += len;
  }
  out.track_count = track;

  std::ranges::stable_sort(out.notes, {}, &MidiNote::tick);
  std::ranges::stable_sort(out.tempos, {}, &TempoChange::tick);
  if (out.tempos.empty() || out.tempos.front().tick != 0) {
    out.tempos.insert(out.tempos.begin(), {0, 500000}); // 指定がなければ 120 BPM
  }
  return true;
}

bool load_midi_file(const std::filesystem::path &path, MidiFile &out, std::string &error) {
  std::ifstream f(path, std::ios::binary);
  if (!f) { error = "cannot open file"; return false; }
  const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(f)),
                                       std::istreambuf_iterator<char>());
  return parse_midi(data, out, error);
}

double midi_tick_to_seconds(const MidiFile &m, std::uint32_t tick) {
  double us = 0;
  std::uint32_t prev_tick = 0;
  std::uint32_t tempo = m.tempos.front().us_per_quarter;
  for (std::size_t i = 1; i < m.tempos.size() && m.tempos[i].tick <= tick; ++i) {
    us += static_cast<double>(m.tempos[i].tick - prev_tick) * tempo / m.ticks_per_quarter;
    prev_tick = m.tempos[i].tick;
    tempo = m.tempos[i].us_per_quarter;
  }
  us += static_cast<double>(tick - prev_tick) * tempo / m.ticks_per_quarter;
  return us / 1e6;
}

} // namespace mcnote::core