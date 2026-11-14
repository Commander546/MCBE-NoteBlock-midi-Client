#pragma once
#include <cstdint>
#include <vector>

struct NoteBlockInfo {
  std::uintptr_t obj;
  std::int32_t x, y, z;
  std::uint8_t note;
};

void request_note_scan(); // 走査を開始(実行中なら何もしない)
bool note_scan_running();
std::vector<NoteBlockInfo> note_scan_results(); // 直近の結果のコピー
bool read_note_live(std::uintptr_t obj,
                    std::uint8_t &note); // 今の音程を読む(失敗でfalse)

void note_scan_learn_vtable(
    std::uintptr_t vtable); // 音ブロックのvtable(絶対アドレス)を覚える
bool note_scan_vtable_known();

bool note_scan_has_pos(std::int32_t x, std::int32_t y,
                       std::int32_t z); // 最後のスキャンに含まれる座標か