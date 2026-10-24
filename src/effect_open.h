#ifndef EFFECT_OPEN_H
#define EFFECT_OPEN_H

// =====================================================
// クイズの開幕の光の演出(2026 改修)
//   当時の開幕(黒板が落ちてくる → ゲージが伸びる → 形式の表示)の順番と長さはそのまま、その上に DXLib の描画(加算合成の線・円)で光を重ねる。
//   fumiya:DXLib の描画でかっこよくできる部分はかっこよく
//
//   着地(Land):  黒板が落ちきった瞬間。硝子に白い光が走り、下の縁から火の粉が散り、枠の内側を 2 つの光が下の真ん中から上の真ん中へ駆け上がる
//   ゲージ(GaugeHead):伸びていくタイムゲージの先端の光と火花(毎フレーム呼ぶ)
//   爆発(Burst): 形式の表示の光(当時の startflash の連番の代わり)。白い芯、広がる輪 2 本、回る光の筋、横に伸びる光の線
//   輪(Ring):    小さい着地(検定の文字、看板)。広がる輪と火花
//
//   使い方:
//     始める時に Effect_Open_Land / Burst / Ring(画面の px)。毎フレーム Effect_Open_Update()、描く所で Effect_Open_Draw()。
//     ゲージの先端は、描く所で Effect_Open_GaugeHead(x, 上, 下) を毎フレーム。
//     開幕が終わったら Effect_Open_Stop()。
// =====================================================

#define OPEN_FX_MAX         8      // 同時に出せる演出の数
#define OPEN_SPARKS         96     // 火花の数(全部の演出で共有)
//#define OPEN_LAND_FRAMES    32   // 前の形
#define OPEN_LAND_FRAMES    54     // 着地の演出の長さ
//#define OPEN_TRACE_FRAMES   26   // 前の形
#define OPEN_TRACE_FRAMES   44     // (2026-10-15。fumiya:光が移動する感じを少しゆっくり)     // 枠の内側を光が駆け上がる長さ
#define OPEN_TRACE_FADE     10     // 駆け上がったあと消えていく長さ(前は 6)
#define OPEN_TRACE_TAIL     0.22f  // 光の尾の長さ(枠の半周を 1 として)
#define OPEN_BURST_FRAMES   36     // 形式の表示の光の長さ(当時の startflash は 35 枚)
#define OPEN_BURST_RAYS     16     // 光の筋の数
#define OPEN_RING_FRAMES    18     // 小さい着地の輪の長さ

void Effect_Open_Land(float x1, float y1, float x2, float y2);   // 着地(黒板の硝子の範囲)
void Effect_Open_Burst(float cx, float cy, float width);          // 形式の表示の光(width は横の光の線の長さ)
void Effect_Open_Ring(float cx, float cy, float r);               // 小さい着地
void Effect_Open_GaugeHead(float x, float y1, float y2);          // ゲージの先端(描く所で毎フレーム)
void Effect_Open_Update(void);
void Effect_Open_Draw(void);
void Effect_Open_Stop(void);

#endif
