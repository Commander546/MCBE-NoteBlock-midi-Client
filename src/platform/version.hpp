#pragma once
#include <string>

inline constexpr const char *kMcnoteVersion = "0.1.0-beta";

// Minecraft.Windows.exe のファイルバージョン("1.21.x.y"
// のような形)。取れなければ "unknown"
std::string game_version_string();