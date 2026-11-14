#pragma once
#include <windows.h>

#include <filesystem>
#include <vector>

// 非同期のファイル選択ダイアログ。別スレッドで開くので、ゲームの描画を止めない
bool file_dialog_open(HWND owner, bool multi); // 開く(すでに開いていれば false)
bool file_dialog_busy();
bool file_dialog_take(std::vector<std::filesystem::path> &out); // 結果があれば受け取る(1回だけ)