#pragma once
#include <cstdint>
#include <string_view>
#include <vector>

// "48 8B ?? 08" の形式。一致した全アドレスを返す(実行可能な区画だけを探す)
std::vector<std::uintptr_t> sig_find_all(std::string_view pattern);

// ちょうど1個見つかれば、そのアドレスを返す(count に個数)
std::uintptr_t sig_find(std::string_view pattern, int *count = nullptr);