#define _CRT_SECURE_NO_WARNINGS
#include "DxLib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ENV_LOADED_H
#include "env_value.h"
#endif
#include "resource_manager.h"
#include "demo.h"
#include "guide_text.h"
#include "ui_ease.h"

// =====================================================
// PV(2026-10-24)。動画のファイルは使わず、DXLib の描画でその場で作って流す(約 38 秒。曲はノズキンのオープニング sys_opening.ogg)
//   場面の流れ(長さは下の #define。文は data/pv.txt):
//     1 ENGINE  「ゲームエンジン」が叩きつけられて、赤い帯が打ち消す →「使わない。」
//     2 CODE    流れるソースの上に「全部、/１から書く。」が 1 字ずつ。「C/C++ × DXライブラリ」の帯
//     3 CUT     ノズキンのクリップ(noz/img/thumb/)を斜めの額に入れて、左右から速く切り替える(1 本ずつ名前の帯)
//     4 GRID    同じクリップを 3x2 に並べて、最後に外へはじけ飛ぶ
//     5 COUNT   「ミニゲーム」→ 数が回って「50」→「本以上！」(数は data/pv.txt。ゲームの数ははっきり出さない)
//     6 PARTY   1P～4P の板が落ちてきて「最大４人で 対戦！」
//     7 END     ノズキンのタイトルの景色とロゴ(demo_noz.cpp の描き方を借りる)+ 場所の案内(data/guide.txt の main)
//   場面のつなぎは、白く飛ばす・はじける・速く流す。揺れと効果音は、叩きつけた瞬間に(下の pv_cue)
//   クリップは大きいので、PV が始まる時に読んで、終わったら手放す(1 本 27MB くらい)
//   前の形(movie/ の動画を流す)は、このファイルの一番下に #if 0 で残した
// =====================================================

extern ResourceManager resMgr;

#define RESOURCE_GROUP_PV       40   // 曲・効果音(終了まで持つ)
#define RESOURCE_GROUP_PV_CLIP  41   // クリップ(PV が終わったら手放す)

#define PV_TEXT_FILE  "./data/pv.txt"
#define PV_BGM_FILE   "./noz/audio/sys_opening.ogg"   // ノズキンのオープニングの曲(39 秒)
#define PV_SE_IMPACT  "./noz/audio/se/pd_impact.ogg"
#define PV_SE_WHOOSH  "./noz/audio/se/pd_whoosh.ogg"
#define PV_SE_FLASH   "./noz/audio/se/title_flash1.ogg"
#define PV_BGM_VOLUME 200        // 曲の音量(0～255。巡回の BGM と同じ)
#define PV_SE_VOLUME  170        // 効果音の音量
#define PV_BGM_FADE   60         // 終わりに曲を小さくしていく長さ(フレーム)

// 場面の始まりと長さ(フレーム)
#define PV_ENGINE_T    0
#define PV_ENGINE_LEN  210
#define PV_CODE_T      (PV_ENGINE_T + PV_ENGINE_LEN)
#define PV_CODE_LEN    240
#define PV_CUT_T       (PV_CODE_T + PV_CODE_LEN)
#define PV_CUT_LEN     96        // クリップ 1 本の長さ
#define PV_CLIP_NUM    6         // 使うクリップの数
#define PV_GRID_T      (PV_CUT_T + PV_CUT_LEN * PV_CLIP_NUM)
#define PV_GRID_LEN    240
#define PV_COUNT_T     (PV_GRID_T + PV_GRID_LEN)
#define PV_COUNT_LEN   360
#define PV_PARTY_T     (PV_COUNT_T + PV_COUNT_LEN)
#define PV_PARTY_LEN   330
#define PV_END_T       (PV_PARTY_T + PV_PARTY_LEN)
#define PV_END_LEN     324
#define PV_FRAMES      (PV_END_T + PV_END_LEN)

// 場面の中の決まった時(その場面の頭から数えたフレーム)
#define PV_E_WORD1     10        // ENGINE:「ゲームエンジン」
#define PV_E_SLASH     66        // ENGINE:赤い帯が走る
#define PV_E_WORD2     76        // ENGINE:「使わない。」
#define PV_C_LINE1     18        // CODE:1 行目の字が出始める
#define PV_C_LINE2     44        // CODE:2 行目
#define PV_C_STAGGER   5         // CODE:1 字ずつの遅れ
#define PV_C_SUB       120       // CODE:C/C++ の帯
#define PV_G_BREAK     170       // GRID:はじけ飛ぶ
#define PV_N_HEAD      4         // COUNT:「ミニゲーム」
#define PV_N_ROLL      20        // COUNT:数が回り始める
#define PV_N_ROLL_LEN  60        // COUNT:回っている長さ
#define PV_N_TAIL      96        // COUNT:「本以上！」
#define PV_N_SUB       150       // COUNT:小さい文
#define PV_P_STAGGER   8         // PARTY:板 1 枚ずつの遅れ
#define PV_P_BANNER    60        // PARTY:「最大４人で 対戦！」
#define PV_L_LOGO1     30        // END:ロゴを描き始める

#define PV_SLAM_DUR    10        // 叩きつける字が収まるまで
#define PV_EDGE        5         // 大きい字の縁の太さ
#define PV_FLASH_IN    12        // 場面の終わりに白く飛ばす長さ
#define PV_FLASH_OUT   16        // 次の場面の頭で白が引く長さ
#define PV_SHAKE_LEN   16        // 揺れの長さ

// 文(data/pv.txt。key: 値)
#define PV_TEXT_LEN 128
typedef struct {
	char engine1[PV_TEXT_LEN], engine2[PV_TEXT_LEN];
	char code1[PV_TEXT_LEN], code2[PV_TEXT_LEN], code_sub[PV_TEXT_LEN];
	char cut_label[PV_TEXT_LEN];
	char count_head[PV_TEXT_LEN], count_tail[PV_TEXT_LEN], count_sub[PV_TEXT_LEN];
	int  count;
	char party[PV_TEXT_LEN];
	char end_sub[PV_TEXT_LEN];
} PvText;

static PvText pv_text;
static int pv_loaded = 0;
static int pv_t = 0;
static int pv_bgm = -1, pv_se_impact = -1, pv_se_whoosh = -1, pv_se_flash = -1;
static int pv_clip[PV_CLIP_NUM][NOZ_CLIP_FRAMES];
static char pv_clip_name[PV_CLIP_NUM][NOZ_NAME_LEN];
static int pv_clip_num = 0;
static int pv_ox = 0, pv_oy = 0;            // 画面の揺れ(Draw の頭で決める)
static int pv_shake_t = -999, pv_shake_amp = 0;

// 効果音と揺れの合図
#define PV_SE_NONE 0
#define PV_SE_IMP  1
#define PV_SE_WHO  2
#define PV_SE_FLA  3
typedef struct { int t, se, shake; } PvCue;
#define PV_CUE_MAX 48
static PvCue pv_cue[PV_CUE_MAX];
static int pv_cue_num = 0;

// プレイヤーの色(1P～4P)と、クリップの場面の色
static const int pv_pcol[4][3] = { { 235, 60, 60 }, { 50, 120, 245 }, { 50, 190, 90 }, { 250, 196, 30 } };
static const int pv_cutcol[6][3] = { { 230, 60, 70 }, { 40, 120, 240 }, { 250, 170, 30 }, { 40, 190, 120 }, { 170, 70, 225 }, { 245, 110, 30 } };

// 流れるソース(この展示のプログラムと、ノズキンのソースから持ってきた行)
static const char* pv_code[] = {
	"while (ProcessMessage() == 0) {",
	"	ClearDrawScreen();",
	"	Director_Update();",
	"	Director_Draw();",
	"	ScreenFlip60();",
	"}",
	"SetDrawBlendMode(DX_BLENDMODE_ADD, 220);",
	"DrawRotaGraph3(sx, y1 + 1, 32, 32, 8.0, 0.5, 0.0, nz_img[9], TRUE);",
	"y = Ease_Move(y0 + 80, y0, frame, 0, 18, EASE_OUT_BACK);",
	"g = resMgr.CreateGroup(RESOURCE_GROUP_NOZ_CARDS);",
	"REGISTER_MINIGAME(\"Hi_game\", GameScene, HiGame_SceneReset)",
	"PushReturnScene(GAME_DEBUG_TOUR, DebugTour_GameReturnCallback);",
	"SetDrawBright(255, 222, 160);",
	"if (CheckHitKey(KEY_INPUT_ESCAPE)) break;",
	"PlaySoundMem(dir_bgm_noz, DX_PLAYTYPE_LOOP, TRUE);",
	"DrawExtendGraph((int)x1, (int)y1, (int)(x1 + dw), (int)(y1 + dh), nz_img[1], FALSE);",
	"for (int k = 0; k < 9; k++) {",
	"	double ang = t * 0.0025 + k * DX_PI / 9.0;",
	"SetDrawMode(DX_DRAWMODE_BILINEAR);",
	"Title_DrawLogo(1, nz_img[7], cx, NZ_LOGO2_Y, t, logo2Start);",
};
#define PV_CODE_NUM ((int)(sizeof(pv_code) / sizeof(pv_code[0])))


// =====================================================
// 読み込み
// =====================================================

// 関数の役割:data/pv.txt を読む
static void Pv_LoadText(void) {
	FILE* fp;
	char line[512];
	memset(&pv_text, 0, sizeof(pv_text));
	pv_text.count = 50;
	fp = fopen(PV_TEXT_FILE, "r");
	if (fp == NULL) return;
	while (Demo_ReadLine(fp, line, sizeof(line))) {
		char *k, *v, *e;
		char* dst = NULL;
		e = strchr(line, ':');
		if (e == NULL) continue;
		*e = '\0';
		k = line; v = e + 1;
		while (*v == ' ' || *v == '\t') v++;
		{ int n = (int)strlen(k); while (n > 0 && (k[n - 1] == ' ' || k[n - 1] == '\t')) k[--n] = '\0'; }
		if (strcmp(k, "engine1") == 0) dst = pv_text.engine1;
		else if (strcmp(k, "engine2") == 0) dst = pv_text.engine2;
		else if (strcmp(k, "code1") == 0) dst = pv_text.code1;
		else if (strcmp(k, "code2") == 0) dst = pv_text.code2;
		else if (strcmp(k, "code_sub") == 0) dst = pv_text.code_sub;
		else if (strcmp(k, "cut_label") == 0) dst = pv_text.cut_label;
		else if (strcmp(k, "count_head") == 0) dst = pv_text.count_head;
		else if (strcmp(k, "count_tail") == 0) dst = pv_text.count_tail;
		else if (strcmp(k, "count_sub") == 0) dst = pv_text.count_sub;
		else if (strcmp(k, "party") == 0) dst = pv_text.party;
		else if (strcmp(k, "end_sub") == 0) dst = pv_text.end_sub;
		else if (strcmp(k, "count") == 0) pv_text.count = atoi(v);
		if (dst != NULL) { strncpy(dst, v, PV_TEXT_LEN - 1); dst[PV_TEXT_LEN - 1] = '\0'; }
	}
	fclose(fp);
}

static void Pv_AddCue(int t, int se, int shake) {
	if (pv_cue_num >= PV_CUE_MAX) return;
	pv_cue[pv_cue_num].t = t; pv_cue[pv_cue_num].se = se; pv_cue[pv_cue_num].shake = shake;
	pv_cue_num++;
}

// 関数の役割:効果音と揺れの合図を、場面の時から並べる
static void Pv_MakeCues(void) {
	int k;
	pv_cue_num = 0;
	Pv_AddCue(PV_ENGINE_T + PV_E_WORD1 + PV_SLAM_DUR, PV_SE_IMP, 22);
	Pv_AddCue(PV_ENGINE_T + PV_E_SLASH, PV_SE_WHO, 0);
	Pv_AddCue(PV_ENGINE_T + PV_E_WORD2 + PV_SLAM_DUR, PV_SE_IMP, 26);
	Pv_AddCue(PV_CODE_T, PV_SE_FLA, 0);
	Pv_AddCue(PV_CODE_T + PV_C_SUB, PV_SE_WHO, 0);
	for (k = 0; k < PV_CLIP_NUM; k++) {
		Pv_AddCue(PV_CUT_T + k * PV_CUT_LEN, PV_SE_WHO, 0);
		Pv_AddCue(PV_CUT_T + k * PV_CUT_LEN + 12, PV_SE_NONE, 10);
	}
	Pv_AddCue(PV_GRID_T, PV_SE_WHO, 0);
	Pv_AddCue(PV_GRID_T + PV_G_BREAK, PV_SE_IMP, 30);
	Pv_AddCue(PV_COUNT_T + PV_N_ROLL + PV_N_ROLL_LEN, PV_SE_IMP, 28);
	Pv_AddCue(PV_COUNT_T + PV_N_TAIL + PV_SLAM_DUR, PV_SE_IMP, 30);
	for (k = 0; k < 4; k++) Pv_AddCue(PV_PARTY_T + k * PV_P_STAGGER + 16, (k == 0) ? PV_SE_IMP : PV_SE_NONE, 14);
	Pv_AddCue(PV_PARTY_T + PV_P_BANNER + PV_SLAM_DUR, PV_SE_IMP, 24);
	Pv_AddCue(PV_END_T, PV_SE_FLA, 0);
}

// 関数の役割:曲・効果音・文を読む(初めての時に 1 回だけ)
static void Pv_LoadCommon(void) {
	ResourceGroup* g;
	if (pv_loaded) return;
	pv_loaded = 1;
	Pv_LoadText();
	g = resMgr.CreateGroup(RESOURCE_GROUP_PV);
	pv_bgm = g->LoadSound(PV_BGM_FILE);
	pv_se_impact = g->LoadSound(PV_SE_IMPACT);
	pv_se_whoosh = g->LoadSound(PV_SE_WHOOSH);
	pv_se_flash = g->LoadSound(PV_SE_FLASH);
	if (pv_se_impact != -1) ChangeVolumeSoundMem(PV_SE_VOLUME, pv_se_impact);
	if (pv_se_whoosh != -1) ChangeVolumeSoundMem(PV_SE_VOLUME, pv_se_whoosh);
	if (pv_se_flash != -1) ChangeVolumeSoundMem(PV_SE_VOLUME, pv_se_flash);
}


// =====================================================
// 小さい道具
// =====================================================

// CP932 の 1 字のバイト数
static int Pv_CharLen(const unsigned char* p) {
	if (((*p >= 0x81 && *p <= 0x9F) || (*p >= 0xE0 && *p <= 0xFC)) && p[1] != '\0') return 2;
	return 1;
}

static unsigned int Pv_Col(const int* c, float f) {
	int r = (int)(c[0] * f), g = (int)(c[1] * f), b = (int)(c[2] * f);
	return GetColor(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b);
}

// 関数の役割:縦のグラデーションの地(上の色 → 下の色)
static void Pv_Grad(int y1, int y2, int r1, int g1, int b1, int r2, int g2, int b2) {
	int y;
	for (y = y1; y < y2; y += 4) {
		float f = (float)(y - y1) / (y2 - y1);
		DrawBox(0, y, SCREEN_WIDTH, y + 4, GetColor((int)(r1 + (r2 - r1) * f), (int)(g1 + (g2 - g1) * f), (int)(b1 + (b2 - b1) * f)), TRUE);
	}
}

// 関数の役割:縁取りつきの字を、(cx, cy) を真ん中に、倍率 sc で描く。a は透明度。ew は縁の太さ。上半分を明るくして光沢を付ける
static void Pv_TextC(int cx, int cy, const char* s, int font, float sc, int a, unsigned int col, unsigned int hi, unsigned int edge, int ew) {
	static const int d8[8][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } };
	int tw, fh, x, y, i;
	if (a <= 0 || s == NULL || s[0] == '\0') return;
	if (a > 255) a = 255;
	tw = GetDrawStringWidthToHandle(s, (int)strlen(s), font);
	fh = GetFontSizeToHandle(font);
	x = cx + pv_ox - (int)(tw * sc / 2);
	y = cy + pv_oy - (int)(fh * sc / 2);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a * 160 / 255);
	DrawExtendStringToHandle(x + ew + 8, y + ew + 10, sc, sc, s, GetColor(0, 0, 0), font);   // 影
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
	for (i = 0; i < 8; i++) DrawExtendStringToHandle(x + d8[i][0] * ew, y + d8[i][1] * ew, sc, sc, s, edge, font);
	DrawExtendStringToHandle(x, y, sc, sc, s, col, font);
	SetDrawArea(0, 0, SCREEN_WIDTH, y + (int)(fh * sc * 0.48f));   // 上半分だけ明るい色を重ねる
	DrawExtendStringToHandle(x, y, sc, sc, s, hi, font);
	SetDrawArea(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:字を加算で重ねて光らせる
static void Pv_TextAdd(int cx, int cy, const char* s, int font, float sc, int a) {
	int tw, fh;
	if (a <= 0 || s == NULL || s[0] == '\0') return;
	if (a > 255) a = 255;
	tw = GetDrawStringWidthToHandle(s, (int)strlen(s), font);
	fh = GetFontSizeToHandle(font);
	SetDrawBlendMode(DX_BLENDMODE_ADD, a);
	DrawExtendStringToHandle(cx + pv_ox - (int)(tw * sc / 2), cy + pv_oy - (int)(fh * sc / 2), sc, sc, s, GetColor(255, 255, 255), font);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:大きい所から叩きつけられて収まる字。age は出てからのフレーム。来る間は残像、収まった瞬間に白く光る
static void Pv_Slam(int cx, int cy, const char* s, int font, float base, int age, unsigned int col, unsigned int hi, unsigned int edge) {
	float sc;
	if (age < 0) return;
	sc = Ease_MoveF(base * 2.8f, base, age, 0, PV_SLAM_DUR, EASE_OUT_CUBIC);
	if (age < PV_SLAM_DUR) Pv_TextAdd(cx, cy, s, font, sc * 1.3f, 70);
	Pv_TextC(cx, cy, s, font, sc, Ease_Move(0, 255, age, 0, PV_SLAM_DUR / 2, EASE_LINEAR), col, hi, edge, PV_EDGE);
	if (age >= PV_SLAM_DUR && age < PV_SLAM_DUR + 20) Pv_TextAdd(cx, cy, s, font, sc, 255 - (age - PV_SLAM_DUR) * 12);
}

// 関数の役割:字を 1 字ずつ、弾むように出す(左から)。start は 1 字目が出るフレーム。戻り値:並べた字の右端
static int Pv_PopChars(int cx, int cy, const char* s, int font, float sc, int age, int stagger, unsigned int col, unsigned int hi, unsigned int edge) {
	const unsigned char* p = (const unsigned char*)s;
	int tw = (int)(GetDrawStringWidthToHandle(s, (int)strlen(s), font) * sc), x = cx - tw / 2, idx = 0;
	while (*p != '\0') {
		char ch[4];
		int n = Pv_CharLen(p), cw, ca;
		memcpy(ch, p, n); ch[n] = '\0';
		cw = (int)(GetDrawStringWidthToHandle(ch, n, font) * sc);
		ca = age - idx * stagger;
		if (ca >= 0) {
			float s2 = Ease_MoveF(sc * 2.2f, sc, ca, 0, 9, EASE_OUT_BACK);
			int yy = Ease_Move(cy - 90, cy, ca, 0, 9, EASE_OUT_CUBIC);
			Pv_TextC(x + cw / 2, yy, ch, font, s2, Ease_Move(0, 255, ca, 0, 5, EASE_LINEAR), col, hi, edge, PV_EDGE);
			if (ca < 16) Pv_TextAdd(x + cw / 2, yy, ch, font, s2, 255 - ca * 16);
		}
		x += cw;
		p += n;
		idx++;
	}
	return x;
}

// 関数の役割:真ん中から外へ飛ぶ集中線(加算)
static void Pv_Burst(int cx, int cy, int t, int num, int a, int speed, int r, int g, int b) {
	int i;
	if (a <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ADD, a > 255 ? 255 : a);
	for (i = 0; i < num; i++) {
		float ang = i * 2.0f * DX_PI_F / num + (i * 37 % 11) * 0.03f;
		int sp = speed + (i * 53) % (speed + 1);
		float d = (float)((i * 211 + t * sp) % 1500) + 60.0f;
		float len = 140.0f + (i * 97) % 260 + d * 0.3f;
		float c = cosf(ang), s = sinf(ang);
		DrawLineAA(cx + c * d, cy + s * d, cx + c * (d + len), cy + s * (d + len), GetColor(r, g, b), 2.0f + (i % 3) * 1.5f);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:横に走る光の筋(加算。光の玉の絵を細長く引き伸ばす)。dir は流れる向き(1 で右へ)
static void Pv_Streaks(int t, int dir, int num, int a, int r, int g, int b) {
	int i, glow = Noz_PvImg(9);
	if (glow == -1 || a <= 0) return;
	SetDrawBright(r, g, b);
	SetDrawBlendMode(DX_BLENDMODE_ADD, a > 255 ? 255 : a);
	for (i = 0; i < num; i++) {
		int sp = 40 + (i * 29) % 50, span = SCREEN_WIDTH + 1600;
		int x = (i * 331 + t * sp) % span - 800;
		int y = (i * 157 + 40) % SCREEN_HEIGHT;
		if (dir < 0) x = SCREEN_WIDTH - x;
		DrawRotaGraph3(x, y, 32, 32, 10.0 + (i % 4) * 4.0, 0.08 + (i % 3) * 0.05, 0.0, glow, TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}

// 関数の役割:真ん中から回る放射の光(三角形を色違いで交互に)
static void Pv_Rays(int cx, int cy, int t, int num, int a, const int* c1, const int* c2) {
	int i;
	float rot = t * 0.004f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
	for (i = 0; i < num; i++) {
		float a1 = rot + i * 2.0f * DX_PI_F / num, a2 = a1 + DX_PI_F / num;
		DrawTriangleAA((float)cx, (float)cy, cx + cosf(a1) * 2400, cy + sinf(a1) * 2400, cx + cosf(a2) * 2400, cy + sinf(a2) * 2400, Pv_Col((i % 2) ? c1 : c2, 1.0f), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:白く飛ばす(場面の頭で引いていく)
static void Pv_FlashOut(int age, int dur, int peak) {
	if (age < 0 || age >= dur) return;
	SetDrawBlendMode(DX_BLENDMODE_ADD, peak * (dur - age) / dur);
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(255, 255, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:白く飛ばす(場面の終わりで強くなっていく)。rest は場面の残りのフレーム
static void Pv_FlashIn(int rest, int dur) {
	if (rest < 0 || rest >= dur) return;
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255 * (dur - rest) / dur);
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(255, 255, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:斜めの板(上の辺が sk だけ右にずれた平行四辺形)を、上の色から下の色へ塗る
static void Pv_SlantBox(float x1, float y1, float x2, float y2, float sk, const int* c1, const int* c2, int a) {
	const int n = 24;
	int j;
	if (a <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a > 255 ? 255 : a);
	for (j = 0; j < n; j++) {
		float ya = y1 + (y2 - y1) * j / n, yb = y1 + (y2 - y1) * (j + 1) / n + 1.0f;
		float sa = sk * (1.0f - (float)j / n), sb = sk * (1.0f - (float)(j + 1) / n);
		float f = (j + 0.5f) / n;
		unsigned int c = GetColor((int)(c1[0] + (c2[0] - c1[0]) * f), (int)(c1[1] + (c2[1] - c1[1]) * f), (int)(c1[2] + (c2[2] - c1[2]) * f));
		DrawQuadrangleAA(x1 + sa, ya, x2 + sa, ya, x2 + sb, yb, x1 + sb, yb, c, TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:クリップを斜めの金の額に入れて描く。(cx, cy) 真ん中、w x h、sk 斜め、fr はクリップの何コマ目
static void Pv_ClipPanel(int k, float cx, float cy, float w, float h, float sk, int fr, int a) {
	static const int goldT[3] = { 255, 238, 160 }, goldB[3] = { 180, 120, 24 };
	float x1 = cx - w / 2 + pv_ox, y1 = cy - h / 2 + pv_oy, x2 = cx + w / 2 + pv_ox, y2 = cy + h / 2 + pv_oy;
	if (pv_clip_num <= 0) return;
	k %= pv_clip_num;
	if (pv_clip[k][0] == -1) return;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a * 130 / 255);   // 影
	DrawQuadrangleAA(x1 + sk + 20, y1 + 24, x2 + sk + 20, y1 + 24, x2 + 20, y2 + 24, x1 + 20, y2 + 24, GetColor(0, 0, 0), TRUE);
	Pv_SlantBox(x1 - 10, y1 - 10, x2 + 10, y2 + 10, sk, goldT, goldB, a);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawModiGraphF(x1 + sk, y1, x2 + sk, y1, x2, y2, x1, y2, pv_clip[k][fr % NOZ_CLIP_FRAMES], FALSE);
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:額の上を、斜めの光の帯が横切る。age は 0 から dur まで
static void Pv_Shine(float cx, float cy, float w, float h, float sk, int age, int dur) {
	float x1 = cx - w / 2 + pv_ox, y1 = cy - h / 2 + pv_oy, y2 = cy + h / 2 + pv_oy, bx;
	if (age < 0 || age >= dur) return;
	bx = x1 - 300 + (w + 600) * age / dur;
	SetDrawArea((int)x1, (int)y1, (int)(x1 + w + sk), (int)y2);
	SetDrawBlendMode(DX_BLENDMODE_ADD, 110);
	DrawQuadrangleAA(bx + sk + 60, y1, bx + sk + 200, y1, bx + 140, y2, bx, y2, GetColor(255, 255, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ADD, 60);
	DrawQuadrangleAA(bx + sk + 230, y1, bx + sk + 260, y1, bx + 200, y2, bx + 170, y2, GetColor(255, 255, 255), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawArea(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

// 関数の役割:紙吹雪(真ん中からはじけて、ひらひら落ちる。四角が裏返りながら回る)
static void Pv_Confetti(int cx, int cy, int age, int num) {
	static const int col[6][3] = { { 255, 80, 80 }, { 80, 160, 255 }, { 255, 214, 70 }, { 90, 220, 120 }, { 230, 120, 255 }, { 255, 255, 255 } };
	int i;
	float drag;
	if (age < 0) return;
	drag = (1.0f - powf(0.95f, (float)age)) / 0.05f;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 255);
	for (i = 0; i < num; i++) {
		float ang = i * 2.39996f, v = 14.0f + (i * 31) % 22;
		float x = cx + cosf(ang) * v * drag + sinf(age * 0.08f + i) * 24.0f;
		float y = cy + (sinf(ang) * v - 6.0f) * drag + 2.6f * age;
		float rot = age * 0.12f + i, flip = fabsf(cosf(age * 0.2f + i * 0.7f));
		float hw = 13.0f, hh = 7.0f * flip + 1.0f, c = cosf(rot), s = sinf(rot);
		if (y > SCREEN_HEIGHT + 40) continue;
		DrawQuadrangleAA(x + (-hw * c - -hh * s), y + (-hw * s + -hh * c), x + (hw * c - -hh * s), y + (hw * s + -hh * c),
			x + (hw * c - hh * s), y + (hw * s + hh * c), x + (-hw * c - hh * s), y + (-hw * s + hh * c), Pv_Col(col[i % 6], 0.6f + 0.4f * flip), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:光の輪が広がる(加算)
static void Pv_Ring(int cx, int cy, int age, int dur, float size, int r, int g, int b) {
	int ring = Noz_PvImg(11);
	float p;
	if (ring == -1 || age < 0 || age >= dur) return;
	p = (float)age / dur;
	SetDrawBright(r, g, b);
	SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * (1.0f - p)));
	DrawRotaGraph(cx + pv_ox, cy + pv_oy, 0.5 + size * p, 0.0, ring, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}


// =====================================================
// 場面
// =====================================================

// 1 ENGINE:「ゲームエンジン」→ 赤い帯で打ち消して「使わない。」
static void Pv_DrawEngine(int a) {
	static const int redT[3] = { 240, 40, 50 }, redB[3] = { 120, 0, 12 };
	const int cx = ScreenCenter_X;
	Pv_Grad(0, SCREEN_HEIGHT, 8, 10, 36, 0, 0, 6);
	Pv_Burst(cx, 460, a, 72, Ease_Move(40, 170, a, 0, PV_ENGINE_LEN, EASE_IN_CUBIC), 18 + a / 6, 120, 170, 255);
	Pv_Slam(cx, 430, pv_text.engine1, Fonts[9], 1.0f, a - PV_E_WORD1, GetColor(235, 240, 255), GetColor(255, 255, 255), GetColor(20, 40, 120));

	// 赤い帯:左から一気に走って、1 語目を斜めの線で打ち消す
	if (a >= PV_E_SLASH) {
		int ba = a - PV_E_SLASH;
		float w = Ease_MoveF(0.0f, (float)SCREEN_WIDTH + 400, ba, 0, 10, EASE_OUT_CUBIC);
		float lx = Ease_MoveF(0.0f, 1700.0f, ba, 0, 8, EASE_OUT_CUBIC);
		Pv_SlantBox(-200.0f + pv_ox, 600.0f + pv_oy, -200.0f + w + pv_ox, 770.0f + pv_oy, -70.0f, redT, redB, 235);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
		DrawBox(0, 600 + pv_oy, (int)(w - 200), 604 + pv_oy, GetColor(255, 160, 160), TRUE);   // 帯の上の縁の光
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		DrawLineAA(cx - 850.0f + pv_ox, 500.0f + pv_oy, cx - 850.0f + lx + pv_ox, 500.0f - lx * 0.06f + pv_oy, GetColor(225, 30, 40), 26.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
		DrawLineAA(cx - 850.0f + pv_ox, 500.0f + pv_oy, cx - 850.0f + lx + pv_ox, 500.0f - lx * 0.06f + pv_oy, GetColor(255, 200, 200), 6.0f);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	Pv_Slam(cx, 685, pv_text.engine2, Fonts[8], 1.0f, a - PV_E_WORD2, GetColor(255, 255, 255), GetColor(255, 255, 255), GetColor(110, 0, 10));
	Pv_Streaks(a, 1, 18, Ease_Move(0, 200, a, PV_ENGINE_LEN - 50, 50, EASE_IN_CUBIC), 160, 200, 255);
	Pv_FlashIn(PV_ENGINE_LEN - 1 - a, PV_FLASH_IN);
}

// 2 CODE:流れるソースの上に「全部、/１から書く。」
static void Pv_DrawCode(int a) {
	static const int cyanT[3] = { 30, 200, 255 }, cyanB[3] = { 0, 70, 140 };
	const int cx = ScreenCenter_X;
	int col, row, endX;
	Pv_Grad(0, SCREEN_HEIGHT, 0, 12, 30, 0, 2, 10);

	// ソース:3 列が、違う速さで上へ流れる。ときどき 1 行が光る
	for (col = 0; col < 3; col++) {
		int sp = 5 + col * 2, x = 70 + col * 620, lh = 44;
		int ofs = a * sp + col * 300;
		for (row = -1; row < SCREEN_HEIGHT / lh + 2; row++) {
			int li = ((row + ofs / lh + col * 7) % PV_CODE_NUM + PV_CODE_NUM) % PV_CODE_NUM;
			int y = row * lh - ofs % lh;
			int hl = (((row + ofs / lh) * 7 + col * 3) % 23 + 23) % 23 == (a / 6) % 23;
			if (hl) {
				SetDrawBlendMode(DX_BLENDMODE_ADD, 70);
				DrawBox(x - 10, y - 4, x + 600, y + lh - 8, GetColor(40, 160, 255), TRUE);
			}
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, hl ? 230 : 80);
			DrawStringToHandle(x, y, pv_code[li], hl ? GetColor(200, 240, 255) : GetColor(70, 170, 230), Fonts[10]);
		}
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 120);   // 文字の後ろを少し沈める
	DrawBox(0, 210, SCREEN_WIDTH, 760, GetColor(0, 4, 14), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

	Pv_PopChars(cx, 330, pv_text.code1, Fonts[9], 1.0f, a - PV_C_LINE1, PV_C_STAGGER, GetColor(235, 245, 255), GetColor(255, 255, 255), GetColor(0, 60, 140));
	endX = Pv_PopChars(cx, 600, pv_text.code2, Fonts[9], 1.0f, a - PV_C_LINE2, PV_C_STAGGER, GetColor(235, 245, 255), GetColor(255, 255, 255), GetColor(0, 60, 140));
	if (a >= PV_C_LINE2 && (a / 15) % 2 == 0) {   // 打ち込みのカーソル
		DrawBox(endX + 20 + pv_ox, 690 + pv_oy, endX + 90 + pv_ox, 712 + pv_oy, GetColor(120, 220, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
		DrawBox(endX + 14 + pv_ox, 684 + pv_oy, endX + 96 + pv_ox, 718 + pv_oy, GetColor(60, 160, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	// 下の帯:左から滑り込む
	if (a >= PV_C_SUB) {
		int sa = a - PV_C_SUB;
		float x2 = Ease_MoveF(-200.0f, (float)SCREEN_WIDTH + 200, sa, 0, 14, EASE_OUT_CUBIC);
		Pv_SlantBox(-200.0f, 830.0f, x2, 930.0f, 40.0f, cyanT, cyanB, 230);
		Pv_TextC(Ease_Move(-600, cx, sa, 4, 16, EASE_OUT_CUBIC), 880, pv_text.code_sub, Fonts[4], 1.0f, 255, GetColor(255, 240, 180), GetColor(255, 255, 230), GetColor(0, 30, 80), 4);
	}
	Pv_FlashOut(a, PV_FLASH_OUT, 255);
	Pv_Streaks(a, -1, 24, Ease_Move(0, 220, a, PV_CODE_LEN - 40, 40, EASE_IN_CUBIC), 120, 200, 255);
	Pv_FlashIn(PV_CODE_LEN - 1 - a, PV_FLASH_IN);
}

// 3 CUT:クリップを 1 本ずつ、左右から速く切り替える
static void Pv_DrawCut(int a) {
	static const int black[3] = { 10, 10, 20 };
	const int cx = ScreenCenter_X;
	int k = a / PV_CUT_LEN, ca = a - k * PV_CUT_LEN, dir = (k % 2 == 0) ? 1 : -1, s;
	const int* c = pv_cutcol[k % 6];
	float px, bx, w, h;

	// 地:場面の色の暗い所に、斜めの縞が速く流れる
	Pv_Grad(0, SCREEN_HEIGHT, (int)(c[0] * 0.30f), (int)(c[1] * 0.30f), (int)(c[2] * 0.30f), 0, 0, 0);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 70);
	for (s = -2; s < 14; s++) {
		int x = ((s * 240 + a * dir * 16) % 3360 + 3360) % 3360 - 600;
		DrawQuadrangleAA((float)x, 0.0f, x + 110.0f, 0.0f, x - 290.0f, (float)SCREEN_HEIGHT, x - 400.0f, (float)SCREEN_HEIGHT, Pv_Col(c, 0.7f), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	Pv_Streaks(a, -dir, 26, 150, c[0] / 2 + 128, c[1] / 2 + 128, c[2] / 2 + 128);

	// クリップの額:入って、少しずつ寄りながら流れて、反対へ抜ける
	px = cx + Ease_MoveF(dir * 2300.0f, 0.0f, ca, 0, 12, EASE_OUT_CUBIC) + Ease_MoveF(0.0f, -dir * 2300.0f, ca, PV_CUT_LEN - 10, 10, EASE_IN_CUBIC) - dir * ca * 0.8f;
	w = 1280.0f * (1.0f + ca * 0.0008f);
	h = 720.0f * (1.0f + ca * 0.0008f);
	Pv_ClipPanel(k, px, 440.0f, w, h, 70.0f, ca * NOZ_CLIP_FPS / 60 + 24 + k * 13, 255);
	Pv_Shine(px, 440.0f, w, h, 70.0f, ca - 12, 26);

	// 名前の帯:額と反対から
	if (pv_clip_num > 0) {
		int bk = k % pv_clip_num;
		bx = Ease_MoveF(-dir * 2300.0f, 0.0f, ca, 4, 14, EASE_OUT_CUBIC) + Ease_MoveF(0.0f, dir * 2300.0f, ca, PV_CUT_LEN - 10, 10, EASE_IN_CUBIC);
		Pv_SlantBox(cx - 760 + bx + pv_ox, 860.0f + pv_oy, cx + 760 + bx + pv_ox, 990.0f + pv_oy, 40.0f, c, black, 240);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 160);
		DrawLineAA(cx - 720 + bx + pv_ox, 862.0f + pv_oy, cx + 800 + bx + pv_ox, 862.0f + pv_oy, Pv_Col(c, 1.4f), 3.0f);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		Pv_TextC((int)(cx - 520 + bx), 872, pv_text.cut_label, Fonts[3], 1.0f, 230, GetColor(255, 230, 140), GetColor(255, 250, 220), GetColor(0, 0, 0), 2);
		Pv_TextC((int)(cx + bx), 935, pv_clip_name[bk], Fonts[8], 0.9f, 255, GetColor(255, 255, 255), GetColor(255, 255, 255), Pv_Col(c, 0.35f), PV_EDGE);
	}
	Pv_FlashOut(ca, 10, (k == 0) ? 255 : 150);
}

// 4 GRID:クリップを 3x2 に並べて、最後に外へはじけ飛ぶ
static void Pv_DrawGrid(int a) {
	const int cx = ScreenCenter_X, cy = 500, tw = 560, th = 315, gap = 44;
	float zoom = Ease_MoveF(1.0f, 1.06f, a, 0, PV_G_BREAK, EASE_LINEAR);
	int i;
	Pv_Grad(0, SCREEN_HEIGHT, 14, 16, 46, 2, 2, 10);
	Pv_Burst(cx, cy, a, 64, (a >= PV_G_BREAK) ? 220 : 70, (a >= PV_G_BREAK) ? 40 : 14, 200, 210, 255);
	for (i = 0; i < PV_CLIP_NUM; i++) {
		int col = i % 3, row = i / 3;
		float tx = cx + (col - 1) * (tw + gap) * zoom, ty = cy + (row - 0.5f) * (th + gap) * zoom;
		float ang = i * 1.7f + 0.6f;
		float in = Ease_MoveF(1.0f, 0.0f, a, i * 5, 18, EASE_OUT_BACK);
		float out = Ease_MoveF(0.0f, 1.0f, a, PV_G_BREAK + (i % 3), 26, EASE_IN_CUBIC);
		float x = tx + cosf(ang) * 1600.0f * in + (tx - cx) * 3.0f * out;
		float y = ty + sinf(ang) * 1100.0f * in + (ty - cy) * 4.0f * out;
		Pv_ClipPanel(i, x, y, tw * zoom * (1.0f + out * 0.6f), th * zoom * (1.0f + out * 0.6f), 36.0f, a * NOZ_CLIP_FPS / 60 + i * 20, 255);
		Pv_Shine(x, y, tw * zoom, th * zoom, 36.0f, a - 40 - i * 6, 22);
	}
	Pv_Ring(cx, cy, a - PV_G_BREAK, 30, 5.0f, 200, 220, 255);
	Pv_FlashOut(a - PV_G_BREAK, 14, 200);
	Pv_FlashIn(PV_GRID_LEN - 1 - a, PV_FLASH_IN);
}

// 5 COUNT:「ミニゲーム」→ 数が回って止まる →「本以上！」
static void Pv_DrawCount(int a) {
	static const int rayA[3] = { 120, 30, 10 }, rayB[3] = { 70, 10, 0 }, goldT[3] = { 255, 214, 70 }, goldB[3] = { 150, 80, 0 };
	const int cx = ScreenCenter_X, ny = 560;
	char buf[16];
	int n, nw, tw2, total, nx, tx, landA = PV_N_ROLL + PV_N_ROLL_LEN;
	float nsc = 1.9f;
	Pv_Grad(0, SCREEN_HEIGHT, 60, 12, 4, 10, 0, 0);
	Pv_Rays(cx, ny, a, 18, 150, rayA, rayB);
	Pv_Burst(cx, ny, a, 48, 90, 20, 255, 200, 120);

	Pv_Slam(cx, 210, pv_text.count_head, Fonts[8], 1.0f, a - PV_N_HEAD, GetColor(255, 255, 255), GetColor(255, 255, 255), GetColor(90, 20, 0));

	// 数:速く回って、だんだん遅くなって止まる(止まった瞬間に光る・はじける)
	n = (int)Ease_MoveF(0.0f, (float)pv_text.count, a, PV_N_ROLL, PV_N_ROLL_LEN, EASE_OUT_CUBIC);
	if (n > pv_text.count) n = pv_text.count;
	_snprintf(buf, sizeof(buf), "%d", (a >= landA) ? pv_text.count : n);
	buf[sizeof(buf) - 1] = '\0';
	{   // 並べる位置は、止まった時の数と「本以上！」で決める
		char fin[16];
		_snprintf(fin, sizeof(fin), "%d", pv_text.count);
		fin[sizeof(fin) - 1] = '\0';
		nw = (int)(GetDrawStringWidthToHandle(fin, (int)strlen(fin), Fonts[9]) * nsc);
		tw2 = GetDrawStringWidthToHandle(pv_text.count_tail, (int)strlen(pv_text.count_tail), Fonts[9]);
		total = nw + 30 + tw2;
		nx = cx - total / 2 + nw / 2;
		tx = cx - total / 2 + nw + 30 + tw2 / 2;
	}
	if (a >= PV_N_ROLL) {
		float kick = (a < landA) ? 1.0f + 0.06f * ((a % 4) < 2) : Ease_MoveF(1.35f, 1.0f, a, landA, 12, EASE_OUT_BACK);
		Pv_TextC(nx, ny, buf, Fonts[9], nsc * kick, 255, Pv_Col(goldT, 1.0f), GetColor(255, 248, 190), Pv_Col(goldB, 0.6f), 7);
		if (a >= landA && a < landA + 24) Pv_TextAdd(nx, ny, buf, Fonts[9], nsc * kick, 255 - (a - landA) * 10);
	}
	Pv_Ring(nx, ny, a - landA, 30, 4.0f, 255, 220, 120);
	Pv_Slam(tx, ny + (int)(220 * nsc / 2) - 120, pv_text.count_tail, Fonts[9], 1.0f, a - PV_N_TAIL, GetColor(255, 70, 50), GetColor(255, 150, 120), GetColor(255, 230, 140));
	Pv_Ring(tx, ny + 100, a - PV_N_TAIL - PV_SLAM_DUR, 30, 3.0f, 255, 140, 80);
	Pv_Confetti(cx, ny, a - landA, 150);

	if (a >= PV_N_SUB) {   // 小さい文:帯ごと右から
		int sa = a - PV_N_SUB;
		float x1 = Ease_MoveF((float)SCREEN_WIDTH + 200, 420.0f, sa, 0, 14, EASE_OUT_CUBIC);
		static const int bandT[3] = { 30, 20, 30 }, bandB[3] = { 0, 0, 0 };
		Pv_SlantBox(x1, 880.0f, x1 + 1100.0f, 970.0f, 30.0f, bandT, bandB, 220);
		Pv_TextC((int)x1 + 550, 925, pv_text.count_sub, Fonts[5], 1.0f, 255, GetColor(255, 230, 140), GetColor(255, 250, 220), GetColor(40, 10, 0), 3);
	}
	Pv_FlashOut(a, PV_FLASH_OUT, 255);
	Pv_FlashIn(PV_COUNT_LEN - 1 - a, PV_FLASH_IN);
}

// 6 PARTY:1P～4P の板が落ちてきて「最大４人で 対戦！」
static void Pv_DrawParty(int a) {
	static const int black[3] = { 0, 0, 0 }, gold[3] = { 255, 220, 110 }, rayA[3] = { 30, 34, 70 }, rayB[3] = { 14, 16, 40 };
	const int cx = ScreenCenter_X, bw = 400, gap = 40, x0 = (SCREEN_WIDTH - (bw * 4 + gap * 3)) / 2, y1 = 110, y2 = 800;
	int i;
	Pv_Grad(0, SCREEN_HEIGHT, 16, 18, 44, 0, 0, 8);
	Pv_Rays(cx, 460, a, 24, 160, rayA, rayB);
	for (i = 0; i < 4; i++) {
		const int* c = pv_pcol[i];
		int bt = a - i * PV_P_STAGGER;
		float oy = Ease_MoveF(-1200.0f, 0.0f, bt, 0, 16, EASE_OUT_BACK);
		float bx1 = (float)(x0 + i * (bw + gap) + pv_ox), by1 = y1 + oy + pv_oy, by2 = y2 + oy + pv_oy;
		char label[8];
		int dark[3] = { c[0] / 4, c[1] / 4, c[2] / 4 };
		if (bt < 0) continue;
		Pv_SlantBox(bx1 - 8, by1 - 8, bx1 + bw + 8, by2 + 8, 60.0f, gold, black, 255);   // 金の縁
		Pv_SlantBox(bx1, by1, bx1 + bw, by2, 60.0f, c, dark, 255);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 50);   // 光沢:上の方を明るく
		DrawQuadrangleAA(bx1 + 60, by1, bx1 + bw + 60, by1, bx1 + bw + 33, by1 + 300, bx1 + 33, by1 + 300, GetColor(255, 255, 255), TRUE);
		if (a >= 100 && (a / 10) % 4 == i) {   // 1P から順に光が回る
			SetDrawBlendMode(DX_BLENDMODE_ADD, 90);
			DrawQuadrangleAA(bx1 + 60, by1, bx1 + bw + 60, by1, bx1 + bw, by2, bx1, by2, Pv_Col(c, 1.0f), TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		_snprintf(label, sizeof(label), "%dP", i + 1);
		label[sizeof(label) - 1] = '\0';
		Pv_TextC((int)(bx1 + bw / 2 + 30) - pv_ox, (int)(by1 + 300) - pv_oy, label, Fonts[9], 0.95f, 255, GetColor(255, 255, 255), GetColor(255, 255, 255), Pv_Col(c, 0.3f), 6);
		if (bt >= 16 && bt < 36) Pv_TextAdd((int)(bx1 + bw / 2 + 30) - pv_ox, (int)(by1 + 300) - pv_oy, label, Fonts[9], 0.95f, 255 - (bt - 16) * 12);
	}

	// 下の帯:真ん中から上下に開いて、文を叩きつける
	if (a >= PV_P_BANNER - 6) {
		int ba = a - (PV_P_BANNER - 6), hh = Ease_Move(0, 80, ba, 0, 10, EASE_OUT_CUBIC), my = 920;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 225);
		DrawBox(0, my - hh, SCREEN_WIDTH, my + hh, GetColor(6, 6, 16), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		DrawBox(0, my - hh, SCREEN_WIDTH, my - hh + 4, GetColor(230, 190, 80), TRUE);
		DrawBox(0, my + hh - 4, SCREEN_WIDTH, my + hh, GetColor(230, 190, 80), TRUE);
		Pv_Slam(cx, my, pv_text.party, Fonts[9], 0.62f, a - PV_P_BANNER, GetColor(255, 214, 70), GetColor(255, 248, 190), GetColor(80, 30, 0));
	}
	Pv_FlashOut(a, PV_FLASH_OUT, 255);
	Pv_FlashIn(PV_PARTY_LEN - 1 - a, PV_FLASH_IN);
}

// 7 END:ノズキンのタイトルの景色とロゴ + 場所の案内
static void Pv_DrawEnd(int a) {
	const int cx = ScreenCenter_X;
	int logo2 = PV_L_LOGO1 + Noz_PvLogoDur(0) - 5, done = logo2 + Noz_PvLogoDur(1), pt;
	Noz_PvBackdrop(1.12f, 0.4f + 0.2f * a / PV_END_LEN, 0.45f, Ease_MoveF(0.25f, 1.0f, a, 0, 90, EASE_OUT_CUBIC), PV_END_T + a);
	Noz_PvLogo(0, cx, 190, a, PV_L_LOGO1);
	Noz_PvLogo(1, cx, 410, a, logo2);
	pt = a - done;
	if (pt >= 0) {   // 帯:真ん中から上下に開く(ブース案内と同じ)
		int hh = Ease_Move(0, 110, pt, 0, 14, EASE_OUT_CUBIC), my = 760;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 200);
		DrawBox(0, my - hh, SCREEN_WIDTH, my + hh, GetColor(8, 10, 28), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		DrawBox(0, my - hh, SCREEN_WIDTH, my - hh + 3, GetColor(230, 190, 80), TRUE);
		DrawBox(0, my + hh - 3, SCREEN_WIDTH, my + hh, GetColor(230, 190, 80), TRUE);
		Pv_Slam(cx, my, guide.main, Fonts[8], 1.0f, pt - 14, GetColor(255, 214, 70), GetColor(255, 248, 190), GetColor(60, 20, 0));
		Pv_Ring(cx, my, pt - 14 - PV_SLAM_DUR, 30, 3.0f, 255, 220, 120);
		if (pt >= 50) {
			int sa = pt - 50;
			Pv_TextC(cx, Ease_Move(980, 950, sa, 0, 16, EASE_OUT_CUBIC), pv_text.end_sub, Fonts[5], 1.0f, Ease_Move(0, 255, sa, 0, 16, EASE_OUT_CUBIC), GetColor(240, 244, 255), GetColor(255, 255, 255), GetColor(0, 0, 20), 3);
		}
	}
	Pv_FlashOut(a, 24, 255);
}


// =====================================================
// デモの 4 つの関数
// =====================================================

static int Pv_Begin(int arg) {
	int k;
	(void)arg;
	Pv_LoadCommon();
	Noz_PvPrepare();
	pv_clip_num = 0;
	for (k = 0; k < PV_CLIP_NUM; k++) {   // クリップ:ゲーム紹介カードと同じ順に、続きから
		if (Noz_PvLoadClip(RESOURCE_GROUP_PV_CLIP, pv_clip[pv_clip_num], pv_clip_name[pv_clip_num], NOZ_NAME_LEN)) pv_clip_num++;
	}
	Pv_MakeCues();
	pv_t = 0;
	pv_shake_t = -999;
	if (pv_bgm != -1) {   // 読み終わってから鳴らす(読む間に曲が進まないように)
		ChangeVolumeSoundMem(PV_BGM_VOLUME, pv_bgm);
		PlaySoundMem(pv_bgm, DX_PLAYTYPE_BACK, TRUE);
	}
	return 1;
}

static int Pv_Update(void) {
	int i;
	for (i = 0; i < pv_cue_num; i++) {
		if (pv_cue[i].t != pv_t) continue;
		if (pv_cue[i].se == PV_SE_IMP && pv_se_impact != -1) PlaySoundMem(pv_se_impact, DX_PLAYTYPE_BACK, TRUE);
		if (pv_cue[i].se == PV_SE_WHO && pv_se_whoosh != -1) PlaySoundMem(pv_se_whoosh, DX_PLAYTYPE_BACK, TRUE);
		if (pv_cue[i].se == PV_SE_FLA && pv_se_flash != -1) PlaySoundMem(pv_se_flash, DX_PLAYTYPE_BACK, TRUE);
		if (pv_cue[i].shake > 0) { pv_shake_t = pv_t; pv_shake_amp = pv_cue[i].shake; }
	}
	if (pv_bgm != -1 && pv_t >= PV_FRAMES - PV_BGM_FADE) ChangeVolumeSoundMem(PV_BGM_VOLUME * (PV_FRAMES - pv_t) / PV_BGM_FADE, pv_bgm);
	pv_t++;
	return (pv_t >= PV_FRAMES) ? 1 : 0;
}

static void Pv_Draw(void) {
	int t = pv_t, k = pv_t - pv_shake_t;
	pv_ox = pv_oy = 0;
	if (k >= 0 && k < PV_SHAKE_LEN) {   // 揺れ:叩きつけた瞬間が一番大きく、すぐ収まる
		int amp = pv_shake_amp * (PV_SHAKE_LEN - k) / PV_SHAKE_LEN;
		if (amp > 0) { pv_ox = GetRand(amp * 2) - amp; pv_oy = GetRand(amp * 2) - amp; }
	}
	if (t < PV_CODE_T) Pv_DrawEngine(t - PV_ENGINE_T);
	else if (t < PV_CUT_T) Pv_DrawCode(t - PV_CODE_T);
	else if (t < PV_GRID_T) Pv_DrawCut(t - PV_CUT_T);
	else if (t < PV_COUNT_T) Pv_DrawGrid(t - PV_GRID_T);
	else if (t < PV_PARTY_T) Pv_DrawCount(t - PV_COUNT_T);
	else if (t < PV_END_T) Pv_DrawParty(t - PV_PARTY_T);
	else Pv_DrawEnd((t < PV_FRAMES) ? t - PV_END_T : PV_END_LEN - 1);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
	SetDrawArea(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

static void Pv_End(void) {
	if (pv_bgm != -1) StopSoundMem(pv_bgm);
	resMgr.UnloadGroup(RESOURCE_GROUP_PV_CLIP);   // クリップは大きいので、PV が終わったら手放す
	pv_clip_num = 0;
}

const DemoDef demo_pv = { "pv", Pv_Begin, Pv_Update, Pv_Draw, Pv_End, 0, DEMO_BGM_NONE };


#if 0
// ===== 前の形(2026-10-24):movie/ の中の動画を流す。fumiya:動画のファイルではなく、DXLib で作ってそのまま動かす、に替えた =====

// =====================================================
// PV(2026-10-24)。movie/ の中の動画(.mp4 / .wmv / .avi / .mpg / .ogv)を、名前の順に流す
//   巡回で pv が来るたびに、続きから PV_PLAY_NUM 本(data/demos.txt の数で替えられる)。最後まで行ったら頭から。
//   動画が 1 本も無い時は、このデモは飛ばす(巡回が次へ進む)。音は動画の音(このデモの間は BGM を止める)
//   画面に合わせて、縦横比を保ったまま大きく(余った所は黒)
// =====================================================

#define PV_DIR        "./movie/"
#define PV_MAX        64
#define PV_PLAY_NUM   1         // 1 回に流す本数
#define PV_VOLUME     10000     // 動画の音量(0～10000。SetMovieVolumeToGraph)
#define PV_WAIT_MAX   (60 * 60 * 10)   // 1 本の長さの上限(フレーム。止まらない動画の保険。10 分)

static char pv_file[PV_MAX][260];
static int  pv_num = -1;        // 見つけた動画の数(-1 ならまだ探していない)
static int  pv_pos = 0;         // 次に流す動画
static int  pv_left = 0;        // この回で、あと何本流すか
static int  pv_h = -1;          // 今の動画
static int  pv_t = 0;

// 関数の役割:movie/ の中の動画を探して、名前の順に並べる(起動して初めての時に 1 回だけ)
static void Pv_Find(void) {
	static const char* ext[] = { "*.mp4", "*.wmv", "*.avi", "*.mpg", "*.ogv" };
	int e, i, j;
	pv_num = 0;
	for (e = 0; e < (int)(sizeof(ext) / sizeof(ext[0])); e++) {
		WIN32_FIND_DATAA fd;
		char pat[260];
		HANDLE h;
		_snprintf(pat, sizeof(pat), "%s%s", PV_DIR, ext[e]);
		pat[sizeof(pat) - 1] = '\0';
		h = FindFirstFileA(pat, &fd);
		if (h == INVALID_HANDLE_VALUE) continue;
		do {
			if (pv_num < PV_MAX) {
				_snprintf(pv_file[pv_num], sizeof(pv_file[pv_num]), "%s%s", PV_DIR, fd.cFileName);
				pv_file[pv_num][sizeof(pv_file[pv_num]) - 1] = '\0';
				pv_num++;
			}
		} while (FindNextFileA(h, &fd));
		FindClose(h);
	}
	for (i = 0; i < pv_num; i++) for (j = i + 1; j < pv_num; j++) {   // 名前の順
		if (strcmp(pv_file[i], pv_file[j]) > 0) { char t[260]; strcpy(t, pv_file[i]); strcpy(pv_file[i], pv_file[j]); strcpy(pv_file[j], t); }
	}
}

// 関数の役割:次の動画を開いて流し始める。戻り値:流せたら 1
static int Pv_Open(void) {
	int tries;
	for (tries = 0; tries < pv_num; tries++) {
		const char* f = pv_file[pv_pos];
		pv_pos = (pv_pos + 1) % pv_num;
		pv_h = LoadGraph(f);
		if (pv_h == -1) continue;   // 開けない動画は飛ばす
		SetMovieVolumeToGraph(PV_VOLUME, pv_h);
		PlayMovieToGraph(pv_h);
		pv_t = 0;
		return 1;
	}
	pv_h = -1;
	return 0;
}

static void Pv_Close(void) {
	if (pv_h != -1) { PauseMovieToGraph(pv_h); DeleteGraph(pv_h); pv_h = -1; }
}

static int Pv_Begin(int arg) {
	if (pv_num < 0) Pv_Find();
	if (pv_num == 0) return 0;
	pv_left = (arg > 0) ? arg : PV_PLAY_NUM;
	return Pv_Open();
}

static int Pv_Update(void) {
	if (pv_h == -1) return 1;
	pv_t++;
	if ((pv_t > 30 && GetMovieStateToGraph(pv_h) == 0) || pv_t >= PV_WAIT_MAX) {   // 終わった(流し始めの数フレームは、まだ止まって見えることがあるので見ない)
		Pv_Close();
		if (--pv_left > 0 && Pv_Open()) return 0;   // この回の続き
		return 1;
	}
	return 0;
}

static void Pv_Draw(void) {
	int w = 0, h = 0;
	float fit, dw, dh;
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(0, 0, 0), TRUE);
	if (pv_h == -1) return;
	GetGraphSize(pv_h, &w, &h);
	if (w <= 0 || h <= 0) return;
	fit = (float)SCREEN_WIDTH / w;
	if (h * fit > SCREEN_HEIGHT) fit = (float)SCREEN_HEIGHT / h;
	dw = w * fit; dh = h * fit;
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawExtendGraphF((SCREEN_WIDTH - dw) / 2, (SCREEN_HEIGHT - dh) / 2, (SCREEN_WIDTH + dw) / 2, (SCREEN_HEIGHT + dh) / 2, pv_h, FALSE);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

static void Pv_End(void) { Pv_Close(); }

const DemoDef demo_pv = { "pv", Pv_Begin, Pv_Update, Pv_Draw, Pv_End, 0, DEMO_BGM_NONE };
#endif
