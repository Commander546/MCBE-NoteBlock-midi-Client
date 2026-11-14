#include "player_probe.hpp"

#include <windows.h>

#include <cstdint>
#include <format>
#include <map>
#include <string>
#include <vector>

#include "attack.hpp"
#include "log.hpp"

namespace {

constexpr std::size_t kSize = 0x8000; // プレイヤーの先頭32KBを見る
constexpr std::int32_t kUnreadable = 0x7BADF00D;

std::map<int, std::vector<std::int32_t>> g_snaps;

bool safe_read(std::uintptr_t src, void *dst, std::size_t n) {
  SIZE_T got = 0;
  return ReadProcessMemory(GetCurrentProcess(),
                           reinterpret_cast<const void *>(src), dst, n, &got) &&
         got == n;
}

// ラベルごとに期待する値(スペクテイターは版によって 3 / 4 / 6 のどれか)
bool expected(int label, std::int32_t v) {
  switch (label) {
  case 0:
    return v == 0;
  case 1:
    return v == 1;
  case 2:
    return v == 2;
  case 3:
    return v == 3 || v == 4 || v == 6;
  default:
    return false;
  }
}

std::size_t count_diff(const std::vector<std::int32_t> &a,
                       const std::vector<std::int32_t> &b) {
  std::size_t n = 0;
  for (std::size_t i = 0; i < a.size(); ++i) {
    if (a[i] != b[i] && a[i] != kUnreadable && b[i] != kUnreadable)
      ++n;
  }
  return n;
}

} // namespace

void player_probe_snapshot(int mode_label) {
  const auto player = attack_player();
  if (!player) {
    log("[mcnote] player probe: left-click a block first (to capture the "
        "player)");
    return;
  }

  std::vector<std::int32_t> v(kSize / 4, kUnreadable);
  std::size_t readable = 0;
  for (std::size_t off = 0; off < kSize; off += 0x1000) {
    if (safe_read(player + off, v.data() + off / 4, 0x1000))
      readable += 0x1000;
  }

  // プレイヤーの先頭8バイト(vtableのはず)がゲーム本体の中を指しているか
  const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
  std::uintptr_t vt = 0;
  safe_read(player, &vt, sizeof(vt));
  const std::string vt_text =
      (vt >= base && vt < base + 0x20000000)
          ? std::format("+0x{:X}", vt - base)
          : std::format("0x{:X} (not in game image)", vt);

  // 他のスナップショットと何個の値が違うか(モードを切り替えられていれば、ある程度は違うはず)
  std::string diffs;
  for (const auto &[label, other] : g_snaps) {
    if (label != mode_label)
      diffs += std::format(" vs{}:{}", label, count_diff(v, other));
  }

  g_snaps[mode_label] = std::move(v);
  log(std::format("[mcnote] player probe: snapshot {} saved (player=0x{:X}, "
                  "vtable {}, readable "
                  "0x{:X} bytes, differing ints:{})",
                  mode_label, player, vt_text, readable,
                  diffs.empty() ? " -" : diffs));
}

void player_probe_find() {
  if (g_snaps.size() < 2) {
    log("[mcnote] player probe: need at least 2 snapshots");
    return;
  }

  // 厳しい条件:
  // サバイバル=0、クリエイティブ=1、アドベンチャー=2、スペクテイター=3/4/6
  int strict = 0;
  for (std::size_t i = 0; i < kSize / 4; ++i) {
    bool all = true;
    for (const auto &[label, v] : g_snaps) {
      if (!expected(label, v[i])) {
        all = false;
        break;
      }
    }
    if (!all)
      continue;

    std::string vals;
    for (const auto &[label, v] : g_snaps)
      vals += std::format("{}={} ", label, v[i]);
    log(std::format("[mcnote] strict candidate: player+0x{:X} ({})", i * 4,
                    vals));
    if (++strict >= 40)
      break;
  }
  log(std::format("[mcnote] strict candidates: {}", strict));

  // 緩い条件: 全部のスナップショットで 0〜8
  // の小さい整数で、かつ全部が同じ値ではない場所
  int loose = 0;
  for (std::size_t i = 0; i < kSize / 4; ++i) {
    const std::int32_t first = g_snaps.begin()->second[i];
    bool ssmall = true;
    bool all_same = true;
    for (const auto &[label, v] : g_snaps) {
      if (v[i] < 0 || v[i] > 8) {
        ssmall = false;
        break;
      }
      if (v[i] != first)
        all_same = false;
    }
    if (!ssmall || all_same)
      continue;

    std::string vals;
    for (const auto &[label, v] : g_snaps)
      vals += std::format("{}={} ", label, v[i]);
    log(std::format("[mcnote] loose candidate: player+0x{:X} ({})", i * 4,
                    vals));
    if (++loose >= 60)
      break;
  }
  log(std::format("[mcnote] loose candidates: {}", loose));
}