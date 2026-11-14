#pragma once
#include <windows.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>


struct TrackInfo {
  int track = 0;
  int channel = 0;
  std::size_t count = 0;
  int lo = 127;
  int hi = 0;
};

struct PlayOptions {
  int instrument = 0;
  int base_pitch = 54; // 音程0のブロックのMIDI音高(ハープなら F#3 = 54)
  int octave_shift = 0;
  int track = -1; // -1 なら全部
  int channel = -1;
  int face = 1;
};

struct PlayerStatus {
  bool loaded = false;
  bool playing = false;
  std::string message;
  std::size_t hits = 0;
  std::size_t missing = 0; // 対応するブロックが無くて、鳴らせない音の数
  double length_sec = 0;
  double position_sec = 0;
};

bool player_load(std::string path);
bool player_load_path(const std::filesystem::path
                          &path); // ダイアログで選んだパスを、そのまま読む
std::vector<TrackInfo> player_tracks();
bool player_play(HWND hwnd, const PlayOptions &opt);
void player_stop();
PlayerStatus player_status();