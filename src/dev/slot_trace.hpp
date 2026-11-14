#pragma once
#include <windows.h>

#include <cstdint>

// thisのvtableを調べて、startDestroyBlockの隣のスロットに回数記録用のフックを張る
void slot_trace_on_capture(void *game_mode, std::uintptr_t start_fn);
void slot_trace_note_start();         // startDestroyBlockが呼ばれたことを記録
void slot_trace_poll_keys(HWND hwnd); // F8で記録のON/OFF(毎フレーム呼ぶ)
void slot_trace_measure_idle(); // 3秒間の呼ばれた回数を測る