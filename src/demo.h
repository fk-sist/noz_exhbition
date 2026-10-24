#ifndef DEMO_H
#define DEMO_H

#include <stdio.h>

// =====================================================
// デモの巡回(2026-10-24。大学祭の入口のモニター用)
//   data/demos.txt に書いた順に、デモを 1 つずつ流して、最後まで行ったら頭に戻る。間は暗転でつなぐ。
//   デモ 1 つは「始める・1 フレーム進める・描く・終わる」の 4 つの関数(DemoDef)。進める関数が 1 を返したら、そのデモは終わり。
//   デモを足す時は、demo_～.cpp に DemoDef を 1 つ作って、director.cpp の demo_table[] に足し、data/demos.txt に名前を書く
//
//   data/demos.txt の 1 行:  名前  [数]      (# から行末はコメント)
//     数はデモごとの意味(ゲーム紹介カードの枚数・PV の本数など。書かなければ 0 = デモのおまかせ)
// =====================================================

typedef struct {
	const char* name;           // data/demos.txt に書く名前
	int  (*Begin)(int arg);     // 始める(暗転の真ん中で呼ぶ)。0 を返したら、そのデモは飛ばす(PV の動画が無い時など)
	int  (*Update)(void);       // 1 フレーム進める。1 を返したら終わり
	void (*Draw)(void);         // 描く(暗転の間も呼ぶ)
	void (*End)(void);          // 終わる(暗転の真ん中で、次のデモの Begin の前)。音を止める・素材を解放する
	int  ticker;                // 1 なら、画面の下に案内の帯を流す(PV は 0)
	int  bgm;                   // 流す BGM(DEMO_BGM_～)。デモの中で自分の曲を鳴らす物は DEMO_BGM_NONE
} DemoDef;

// 巡回が流す BGM(director.cpp。曲が替わる時は、前の曲を小さくしてから次を頭から)
#define DEMO_BGM_NONE   0
#define DEMO_BGM_NOZ    1   // ノズキンのメニューの曲(noz/audio/sys_menu.ogg)

// 各デモ(demo_～.cpp)
extern const DemoDef demo_noz_guide;    // ノズキン:ブース案内(タイトルの景色とロゴ + 場所)
extern const DemoDef demo_noz_cards;    // ノズキン:ゲーム紹介カード
extern const DemoDef demo_qma;          // クイズ OS×UNIX:突然ですが、ここで問題です
extern const DemoDef demo_circle;       // NOZ GAMES:サークル紹介
extern const DemoDef demo_pv;           // PV(DXLib の描画でその場で作る。2026-10-24 に動画から替えた)

// ノズキンの部品を PV から借りる(demo_noz.cpp。2026-10-24)
#define NOZ_CLIP_FRAMES 120   // クリップのコマ数(demo_noz.cpp の NZ_CLIP_FRAMES と同じ)
#define NOZ_CLIP_FPS    12    // クリップの速さ(NZ_CLIP_FPS と同じ)
#define NOZ_NAME_LEN    128   // ゲームの名前の長さ
void Noz_PvPrepare(void);                                       // タイトルの絵とゲームの一覧を読む(読んであれば何もしない)
int  Noz_PvImg(int no);                                         // タイトルの絵(BootManager.cpp と同じ番号。9 光の玉 / 10 十字の輝き / 11 光の輪 / 12 光の柱)
int  Noz_PvLoadClip(int group, int* frames, char* name, int cap);   // 紹介の順の続きのクリップを 1 本、group に読む。戻り値:読めたら 1
void Noz_PvBackdrop(float zoom, float focusX, float focusY, float dawn, int t);   // タイトルの景色
void Noz_PvLogo(int no, int cx, int cy, int t, int startT);     // タイトルのロゴ(0 紋章 / 1 本題)
int  Noz_PvLogoDur(int no);                                     // そのロゴを描き終わるまでのフレーム数

// 巡回(director.cpp)
void Director_Init(void);
void Director_Update(void);
void Director_Draw(void);
void Director_End(void);

// テキストファイルの 1 行を読む小さい道具(director.cpp)。行末の改行と、# から行末を消す。戻り値:読めたら 1
int  Demo_ReadLine(FILE* fp, char* buf, int cap);

#endif
