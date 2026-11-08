#pragma once

// 0=survival 1=creative 2=adventure 3=spectator のラベルで、プレイヤーの中身を保存する
void player_probe_snapshot(int mode_label);
// 保存した全部のモードで、期待した値になっているオフセットを表示する
void player_probe_find();