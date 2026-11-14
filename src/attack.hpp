#pragma once
#include <windows.h>

#include <cstdint>

struct BlockPos {
  std::int32_t x, y, z;
};

constexpr UINT kMsgAttack = WM_APP + 0x4D43;

void install_attack_hook();
bool attack_ready();                     // 今のモードで叩いてよいか
const char *attack_state();              // UIに出す、今の状態の説明
bool attack_last_clicked(BlockPos &out); // 最後にクリックしたブロック
std::uintptr_t attack_player(); // 捕まえたプレイヤーのアドレス(0 なら未取得)
void request_attack(HWND hwnd, int x, int y, int z,
                    int face); // どのスレッドからでもOK
void attack_on_ui_thread();    // WndProcから呼ぶ(MAINスレッド上で実行される)
void attack_set_quiet(bool quiet); // 演奏中はログを出さない
void attack_heartbeat(
    void *game_mode); // 毎tick呼ばれる関数から呼ぶ(GameModeを自動で捕まえる)
long long attack_beat_age_ms(); // 最後の心拍から何ミリ秒か(まだ無ければ -1)
void request_use(HWND hwnd, int x, int y, int z,
                 int face); // 右クリック(押す→使う→離す)を依頼
void attack_set_use_variant(
    int variant); // 右クリックの呼び方を切り替える(0〜2)
void attack_capture_use(
    std::uint64_t item,
    std::uint64_t rsp); // スロット[13]の本物の呼び出しの引数を覚える
bool attack_in_progress(); // 自分でゲームの関数を呼んでいる最中か(調査用の印付けに使う)