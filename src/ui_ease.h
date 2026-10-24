#ifndef UI_EASE_H
#define UI_EASE_H

// =====================================================
// 動きの部品(2026 改修)
//   当時は、演出の動き(出てくる・引っ込む・ふわっと止まる)を、全部その場で手書きの数値で作っていた。
//   ここに「何フレーム目に、どこからどこへ、どんな調子で」を渡すと位置を返す関数をまとめ、演出はこれだけで書く。
//   調子(カーブ)は名前で選ぶ。式はここに閉じ込めて、数値を手で調整しない。
//
//   使い方の例(ボタンが、下の 80px 下から 18 フレームかけて、行き過ぎて戻る調子で入ってくる):
//     y = Ease_Move(y0 + 80, y0, frame, 0, 18, EASE_OUT_BACK);
//   何個かを時間差で入れる時は、start(始まるフレーム)をずらす:
//     y = Ease_Move(y0 + 80, y0, frame, i * 3, 18, EASE_OUT_BACK);
// =====================================================

// カーブの種類
#define EASE_LINEAR        0   // 等速
#define EASE_IN_CUBIC      1   // ゆっくり出て、速く着く(引っ込む時に)
#define EASE_OUT_CUBIC     2   // 速く出て、ふわっと止まる(出てくる時に。迷ったらこれ)
#define EASE_IN_OUT_CUBIC  3   // ゆっくり出て、ゆっくり止まる(横へ移す時に)
#define EASE_OUT_BACK      4   // 行き過ぎて、少し戻って止まる(ボタンが入ってくる時に)
#define EASE_OUT_BOUNCE    5   // 着いて、弾んで止まる
#define EASE_OUT_ELASTIC   6   // 着いて、びよんと揺れて止まる

float Ease_Apply(int type, float t);                              // t(0～1)を、カーブで 0～1 に直す
float Ease_T(int frame, int start, int dur);                      // frame が start から dur の間でどこか(0～1。前なら 0、過ぎたら 1)
float Ease_Lerp(float a, float b, float t);                       // a と b の t(0～1)の所
int   Ease_Move(int from, int to, int frame, int start, int dur, int type);          // 位置(整数)
float Ease_MoveF(float from, float to, int frame, int start, int dur, int type);     // 位置(小数。大きさや透明度に)
int   Ease_Done(int frame, int start, int dur);                   // その動きが終わったか(frame >= start + dur)

#endif
