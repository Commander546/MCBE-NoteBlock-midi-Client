#pragma once
#include <cstdint>
#include <vector>

struct NoteEntry {
  std::int32_t x, y, z;
  std::uint8_t instrument;
  std::uint8_t note;
};

// 叩いたときに取れた値を覚える(同じ座標は上書き)
void note_table_learn(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t instrument,
                      std::uint32_t note);
std::vector<NoteEntry> note_table_snapshot(); // 楽器→音程→座標の順に並べて返す
void note_table_clear();