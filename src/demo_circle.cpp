#define _CRT_SECURE_NO_WARNINGS
#include "DxLib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef ENV_LOADED_H
#include "env_value.h"
#endif
#include "resource_manager.h"
#include "demo.h"
#include "ui_ease.h"

// =====================================================
// NOZ GAMES のサークル紹介(2026-10-24)
//   data/circle.txt の場面を上から順に流す(書き方はそのファイルの頭)。写真は circle/photos/ の中(fumiya が撮った活動の写真)。
//   写真の場面は、写真がゆっくり寄りながら流れる(縦長の写真は、暗くした同じ写真を背景に敷いて、額に入れて出す)。
//   場面の替わり目は、前の場面の上に次の場面がふわっと重なる。文の板は、少し遅れて滑り込む
//   文面は学祭ポスター(サークル紹介の 1 枚)と同じ。ロゴは noz/img/nozlogo.png(スパルタの紋章)と titlenoz2.png(NOZ KINGDOM)
// =====================================================

#define CC_FILE          "./data/circle.txt"
#define CC_PHOTO_DIR     "./circle/photos/"
#define CC_SCENE_MAX     32
#define CC_TEXT_MAX      4
#define CC_FADE          24          // 場面が重なって替わる長さ
#define CC_PANEL_IN      20          // 文の板が滑り込む長さ
#define CC_PANEL_DELAY   12          // 場面が替わってから、文の板が入り始めるまで
#define CC_ZOOM0         1.04f       // 写真の寄り(場面の頭 → 終わり)
#define CC_ZOOM1         1.14f
#define CC_PAN           0.06f       // 写真が横へ流れる量(写真の幅に対する割合)
#define CC_PORTRAIT_H    900         // 縦長の写真を額に入れる時の高さ

#define CC_GOLD          GetColor(230, 190, 80)
#define CC_CREAM         GetColor(255, 244, 214)
#define CC_ACCENT        GetColor(255, 122, 48)    // NOZ GAMES の紋章の橙
#define CC_RED_BAND      GetColor(184, 32, 44)

#define CC_KIND_TITLE    0
#define CC_KIND_SLOGAN   1
#define CC_KIND_PILLAR   2
#define CC_KIND_MAKING   3
#define CC_KIND_PHOTO    4

#define RESOURCE_GROUP_CIRCLE 20   // 写真とロゴ(紹介が終わったら解放する)

extern ResourceManager resMgr;

typedef struct {
	int  kind;
	int  frames;
	int  photo;                         // 写真(無ければ -1)
	char text[CC_TEXT_MAX][256];
	int  text_num;
} CircleScene;

static CircleScene cc_scene[CC_SCENE_MAX];
static int cc_num = 0;
static int cc_cur = 0, cc_t = 0;        // 今の場面と、その中のフレーム
static int cc_logo = -1, cc_kingdom = -1, cc_glow = -1;

// 関数の役割:data/circle.txt を読んで、写真とロゴを読み込む
static void Circle_Load(void) {
	FILE* fp;
	char line[1024];
	ResourceGroup* g = resMgr.CreateGroup(RESOURCE_GROUP_CIRCLE);
	cc_logo = g->LoadImg("./noz/img/nozlogo.png");
	cc_kingdom = g->LoadImg("./noz/img/titlenoz2.png");
	cc_glow = g->LoadImg("./noz/img/title_glow.png");
	cc_num = 0;
	fp = fopen(CC_FILE, "r");
	if (fp == NULL) return;
	while (Demo_ReadLine(fp, line, sizeof(line)) && cc_num < CC_SCENE_MAX) {
		char kind[16], photo[128];
		int sec = 0, used = 0, k;
		CircleScene* s = &cc_scene[cc_num];
		char* p;
		if (line[0] == '\0') continue;
		if (sscanf(line, "%15s %d %127s %n", kind, &sec, photo, &used) < 3) continue;
		memset(s, 0, sizeof(*s));
		if (strcmp(kind, "title") == 0) s->kind = CC_KIND_TITLE;
		else if (strcmp(kind, "slogan") == 0) s->kind = CC_KIND_SLOGAN;
		else if (strcmp(kind, "pillar") == 0) s->kind = CC_KIND_PILLAR;
		else if (strcmp(kind, "making") == 0) s->kind = CC_KIND_MAKING;
		else if (strcmp(kind, "photo") == 0) s->kind = CC_KIND_PHOTO;
		else continue;
		s->frames = (sec > 0 ? sec : 4) * 60;
		s->photo = -1;
		if (strcmp(photo, "-") != 0) {
			char path[256];
			_snprintf(path, sizeof(path), "%s%s", CC_PHOTO_DIR, photo);
			path[sizeof(path) - 1] = '\0';
			s->photo = g->LoadImg(path);
		}
		// 文を | で分ける
		p = (used > 0 && used <= (int)strlen(line)) ? line + used : line + strlen(line);
		for (k = 0; k < CC_TEXT_MAX && *p != '\0'; k++) {
			char* bar = strchr(p, '|');
			int n = bar ? (int)(bar - p) : (int)strlen(p);
			if (n > 255) n = 255;
			memcpy(s->text[k], p, n);
			s->text[k][n] = '\0';
			s->text_num = k + 1;
			if (bar == NULL) break;
			p = bar + 1;
		}
		cc_num++;
	}
	fclose(fp);
}

// 影つきの字(真ん中ぞろえ)
static void Circle_StringC(int cx, int y, const char* s, unsigned int col, int font, int alpha) {
	int w = GetDrawStringWidthToHandle(s, (int)strlen(s), font);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawStringToHandle(cx - w / 2 + 4, y + 4, s, GetColor(0, 0, 16), font);
	DrawStringToHandle(cx - w / 2, y, s, col, font);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 関数の役割:幅 maxW で折り返して描く(CP932 の全角は 2 バイトで 1 字)。戻り値:描いた行の数
static int Circle_Wrapped(int x, int y, int maxW, int lineH, const char* s, unsigned int col, int font) {
	char line[256];
	int len = 0, rows = 0;
	const unsigned char* p = (const unsigned char*)s;
	while (*p != '\0') {
		int n = ((*p >= 0x81 && *p <= 0x9F) || (*p >= 0xE0 && *p <= 0xFC)) ? 2 : 1;
		if (n == 2 && p[1] == '\0') n = 1;
		if (len + n >= (int)sizeof(line)) break;
		memcpy(line + len, p, n);
		if (len > 0 && GetDrawStringWidthToHandle(line, len + n, font) > maxW) {
			line[len] = '\0';
			DrawStringToHandle(x + 3, y + 3 + rows * lineH, line, GetColor(0, 0, 16), font);
			DrawStringToHandle(x, y + rows * lineH, line, col, font);
			rows++; len = 0;
			continue;
		}
		len += n; p += n;
	}
	if (len > 0) {
		line[len] = '\0';
		DrawStringToHandle(x + 3, y + 3 + rows * lineH, line, GetColor(0, 0, 16), font);
		DrawStringToHandle(x, y + rows * lineH, line, col, font);
		rows++;
	}
	return rows;
}

// 関数の役割:写真を画面いっぱいに(はみ出す分は切って)、ゆっくり寄りながら横へ流して描く。縦長の写真は額に入れる
//   t は場面の中のフレーム、frames は場面の長さ、idx は場面の番号(流れる向きを場面ごとに替える)、alpha は濃さ
static void Circle_DrawPhoto(int g, int t, int frames, int idx, int alpha) {
	int iw = 0, ih = 0;
	float p = (frames > 0) ? (float)t / frames : 0.0f, zoom, fit, dw, dh, fx;
	if (g < 0 || alpha <= 0) return;
	GetGraphSize(g, &iw, &ih);
	if (iw <= 0 || ih <= 0) return;
	zoom = CC_ZOOM0 + (CC_ZOOM1 - CC_ZOOM0) * p;
	fx = (idx % 2) ? 0.5f + CC_PAN * (p - 0.5f) : 0.5f - CC_PAN * (p - 0.5f);   // 場面ごとに左へ・右へ
	fit = (float)SCREEN_WIDTH / iw;
	if (ih * fit < SCREEN_HEIGHT) fit = (float)SCREEN_HEIGHT / ih;
	dw = iw * fit * zoom; dh = ih * fit * zoom;
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	if (ih > iw) {   // 縦長:暗くした同じ写真を背景に、額に入れた写真を真ん中に
		float x1 = SCREEN_WIDTH / 2 - dw * fx, y1 = SCREEN_HEIGHT / 2 - dh / 2;
		float fh = (float)CC_PORTRAIT_H * (1.0f + 0.04f * p), fw = fh * iw / ih;
		float cx = SCREEN_WIDTH / 2.0f, cy = SCREEN_HEIGHT / 2.0f - 20;
		int k;
		SetDrawBright(60, 62, 80);
		DrawExtendGraphF(x1, y1, x1 + dw, y1 + dh, g, FALSE);
		SetDrawBright(255, 255, 255);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha * 150 / 255);
		DrawBoxAA(cx - fw / 2 + 16, cy - fh / 2 + 20, cx + fw / 2 + 16, cy + fh / 2 + 20, GetColor(0, 0, 0), TRUE);   // 影
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		DrawExtendGraphF(cx - fw / 2, cy - fh / 2, cx + fw / 2, cy + fh / 2, g, FALSE);
		for (k = 0; k < 4; k++) DrawBoxAA(cx - fw / 2 - k, cy - fh / 2 - k, cx + fw / 2 + k, cy + fh / 2 + k, CC_GOLD, FALSE);
	}
	else {
		float x1 = SCREEN_WIDTH / 2 - dw * fx, y1 = SCREEN_HEIGHT / 2 - dh / 2;
		if (x1 > 0) x1 = 0;
		if (x1 + dw < SCREEN_WIDTH) x1 = SCREEN_WIDTH - dw;
		DrawExtendGraphF(x1, y1, x1 + dw, y1 + dh, g, FALSE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawMode(DX_DRAWMODE_NEAREST);
}

// 関数の役割:下から上へ濃くなる黒(文を読みやすくする。y1 から下、alpha は一番濃い所)
static void Circle_Shade(int y1, int y2, int alpha) {
	int y;
	for (y = y1; y < y2; y += 2) {
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha * (y - y1) / (y2 - y1 > 0 ? y2 - y1 : 1));
		DrawBox(0, y, SCREEN_WIDTH, y + 2, GetColor(4, 6, 20), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// ---- 場面ごとの文 ----

// タイトル:紺の地に、ゆっくり回る光の筋、紋章のロゴ、「ゲーム制作サークル」、「ノズゲは本気だ」(本気 にルビ)
static void Circle_DrawTitle(const CircleScene* s, int t, int a) {
	int y, k, cx = SCREEN_WIDTH / 2;
	for (y = 0; y < SCREEN_HEIGHT; y += 4) {   // 紺の地(上が少し明るい)
		int c = 30 - y * 22 / SCREEN_HEIGHT;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
		DrawBox(0, y, SCREEN_WIDTH, y + 4, GetColor(c / 2 + 4, c / 2 + 8, c + 30), TRUE);
	}
	SetDrawBright(255, 160, 80);   // 紋章の後ろで回る光の筋
	for (k = 0; k < 10; k++) {
		SetDrawBlendMode(DX_BLENDMODE_ADD, a * (28 + (k % 3) * 10) / 255);
		DrawRotaGraph3(cx, 380, 32, 32, 2.0 + (k % 3), 26.0, t * 0.004 + k * DX_PI / 10, cc_glow, TRUE);
	}
	SetDrawBright(255, 255, 255);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	{   // 紋章:大きい所から縮んで収まる
		float sc = Ease_MoveF(1.6f, 0.82f, t, 0, 24, EASE_OUT_BACK);
		int la = Ease_Move(0, a, t, 0, 16, EASE_OUT_CUBIC);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, la);
		DrawRotaGraph(cx, 380, sc, 0.0, cc_logo, TRUE);
		if (t >= 20 && t < 50) { SetDrawBlendMode(DX_BLENDMODE_ADD, (50 - t) * 7); DrawRotaGraph(cx, 380, sc, 0.0, cc_logo, TRUE); }
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	if (s->text_num >= 1) Circle_StringC(cx, 90, s->text[0], CC_GOLD, Fonts[6], Ease_Move(0, a, t, 10, 20, EASE_OUT_CUBIC));
	if (s->text_num >= 2 && t >= 26) {   // 題:「ノズゲは本気だ」。ルビは題の中の字(text[2])の上に text[3]
		const char* title = s->text[1];
		int font = Fonts[8], tw = GetDrawStringWidthToHandle(title, (int)strlen(title), font);
		int ty = Ease_Move(760, 690, t, 26, 18, EASE_OUT_BACK), ta = Ease_Move(0, a, t, 26, 14, EASE_OUT_CUBIC);
		Circle_StringC(cx, ty, title, CC_CREAM, font, ta);
		if (s->text_num >= 4 && s->text[2][0] != '\0') {
			const char* at = strstr(title, s->text[2]);
			if (at != NULL) {
				int pre = GetDrawStringWidthToHandle(title, (int)(at - title), font);
				int bw = GetDrawStringWidthToHandle(s->text[2], (int)strlen(s->text[2]), font);
				Circle_StringC(cx - tw / 2 + pre + bw / 2, ty - 34, s->text[3], CC_GOLD, Fonts[6], ta);
			}
		}
	}
}

// スローガン:写真を暗くして、真ん中に赤い帯と大きな文
static void Circle_DrawSlogan(const CircleScene* s, int t, int a) {
	int cx = SCREEN_WIDTH / 2, pt = t - CC_PANEL_DELAY;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a * 130 / 255);
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(4, 6, 20), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	if (pt < 0) return;
	if (s->text_num >= 1) {   // 赤い帯(横に開く)
		int tw = GetDrawStringWidthToHandle(s->text[0], (int)strlen(s->text[0]), Fonts[5]);
		int hw = Ease_Move(0, tw / 2 + 50, pt, 0, CC_PANEL_IN, EASE_OUT_CUBIC), y = 360;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
		DrawBox(cx - hw, y, cx + hw, y + 84, CC_RED_BAND, TRUE);
		DrawBox(cx - hw, y, cx + hw, y + 4, CC_GOLD, TRUE);
		DrawBox(cx - hw, y + 80, cx + hw, y + 84, CC_GOLD, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		if (pt >= CC_PANEL_IN / 2) Circle_StringC(cx, y + 18, s->text[0], GetColor(255, 255, 255), Fonts[5], Ease_Move(0, a, pt, CC_PANEL_IN / 2, 10, EASE_OUT_CUBIC));
	}
	if (s->text_num >= 2) Circle_StringC(cx, Ease_Move(560, 500, pt, 14, CC_PANEL_IN, EASE_OUT_CUBIC), s->text[1], CC_CREAM, Fonts[4], Ease_Move(0, a, pt, 14, CC_PANEL_IN, EASE_OUT_CUBIC));
	if (s->text_num >= 3) Circle_StringC(cx, 630, s->text[2], GetColor(201, 205, 227), Fonts[6], Ease_Move(0, a, pt, 30, CC_PANEL_IN, EASE_OUT_CUBIC));
}

// 柱(3 つの方針):写真の左下に、番号・見出し・説明の板
static void Circle_DrawPillar(const CircleScene* s, int t, int a) {
	int pt = t - CC_PANEL_DELAY, x, y = 610, w = 1000, h = 330, k;
	Circle_Shade(SCREEN_HEIGHT / 2, SCREEN_HEIGHT, a * 200 / 255);
	if (pt < 0) return;
	x = Ease_Move(-w - 40, 90, pt, 0, CC_PANEL_IN, EASE_OUT_CUBIC);
	for (k = 0; k < h; k += 2) {   // 板(縦のグラデーションの紺)
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, a * (215 - k * 40 / h) / 255);
		DrawBox(x, y + k, x + w, y + k + 2, GetColor(18, 24, 60 - k * 20 / h), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
	DrawBox(x, y, x + 10, y + h, CC_ACCENT, TRUE);
	DrawBox(x, y + h - 3, x + w, y + h, CC_GOLD, TRUE);
	if (s->text_num >= 1) DrawStringToHandle(x + 44, y + 24, s->text[0], CC_ACCENT, Fonts[8]);
	if (s->text_num >= 2) {
		DrawStringToHandle(x + 210 + 3, y + 46 + 3, s->text[1], GetColor(0, 0, 16), Fonts[4]);
		DrawStringToHandle(x + 210, y + 46, s->text[1], CC_CREAM, Fonts[4]);
	}
	if (s->text_num >= 3) Circle_Wrapped(x + 48, y + 160, w - 96, 52, s->text[2], GetColor(220, 225, 240), Fonts[6]);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// いま作っているもの:写真の下に、NOZ KINGDOM のロゴと説明
static void Circle_DrawMaking(const CircleScene* s, int t, int a) {
	int pt = t - CC_PANEL_DELAY, cx = SCREEN_WIDTH / 2, y;
	Circle_Shade(SCREEN_HEIGHT / 3, SCREEN_HEIGHT, a * 225 / 255);
	if (pt < 0) return;
	y = Ease_Move(SCREEN_HEIGHT, 560, pt, 0, CC_PANEL_IN, EASE_OUT_CUBIC);
	if (s->text_num >= 1) Circle_StringC(cx, y, s->text[0], CC_GOLD, Fonts[6], a);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
	DrawRotaGraph(cx, y + 150, 0.62, 0.0, cc_kingdom, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	if (s->text_num >= 2) Circle_StringC(cx, y + 270, s->text[1], CC_CREAM, Fonts[5], Ease_Move(0, a, pt, 10, CC_PANEL_IN, EASE_OUT_CUBIC));
	if (s->text_num >= 3) Circle_StringC(cx, y + 340, s->text[2], CC_ACCENT, Fonts[5], Ease_Move(0, a, pt, 20, CC_PANEL_IN, EASE_OUT_CUBIC));
}

// 写真だけ:一言があれば右下に小さく
static void Circle_DrawPhotoOnly(const CircleScene* s, int t, int a) {
	if (s->text_num < 1 || s->text[0][0] == '\0') return;
	Circle_Shade(SCREEN_HEIGHT * 3 / 4, SCREEN_HEIGHT, a * 180 / 255);
	{
		const char* c = s->text[0];
		int w = GetDrawStringWidthToHandle(c, (int)strlen(c), Fonts[6]);
		int ca = Ease_Move(0, a, t, CC_PANEL_DELAY, CC_PANEL_IN, EASE_OUT_CUBIC);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, ca);
		DrawStringToHandle(SCREEN_WIDTH - 90 - w, SCREEN_HEIGHT - 150, c, CC_CREAM, Fonts[6]);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

// 関数の役割:場面 i を、その中のフレーム t で、濃さ a で描く
static void Circle_DrawScene(int i, int t, int a) {
	const CircleScene* s = &cc_scene[i];
	if (s->photo >= 0) Circle_DrawPhoto(s->photo, t, s->frames + CC_FADE, i, a);
	switch (s->kind) {
	case CC_KIND_TITLE:  Circle_DrawTitle(s, t, a); break;
	case CC_KIND_SLOGAN: Circle_DrawSlogan(s, t, a); break;
	case CC_KIND_PILLAR: Circle_DrawPillar(s, t, a); break;
	case CC_KIND_MAKING: Circle_DrawMaking(s, t, a); break;
	case CC_KIND_PHOTO:  Circle_DrawPhotoOnly(s, t, a); break;
	}
}

static int Circle_Begin(int arg) {
	(void)arg;
	Circle_Load();
	if (cc_num == 0) { resMgr.UnloadGroup(RESOURCE_GROUP_CIRCLE); return 0; }
	cc_cur = 0; cc_t = 0;
	return 1;
}

static int Circle_Update(void) {
	if (++cc_t >= cc_scene[cc_cur].frames) {
		if (cc_cur + 1 >= cc_num) return 1;   // 最後の場面が終わった(そのまま暗転で消える)
		cc_cur++; cc_t = 0;
	}
	return 0;
}

static void Circle_Draw(void) {
	if (cc_num == 0) return;
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(0, 0, 0), TRUE);
	if (cc_cur > 0 && cc_t < CC_FADE) Circle_DrawScene(cc_cur - 1, cc_scene[cc_cur - 1].frames + cc_t, 255);   // 前の場面(この上に次の場面が重なる)
	Circle_DrawScene(cc_cur, cc_t, (cc_cur > 0) ? Ease_Move(0, 255, cc_t, 0, CC_FADE, EASE_IN_OUT_CUBIC) : 255);
}

static void Circle_End(void) {
	resMgr.UnloadGroup(RESOURCE_GROUP_CIRCLE);
	cc_num = 0;
}

const DemoDef demo_circle = { "circle", Circle_Begin, Circle_Update, Circle_Draw, Circle_End, 1, DEMO_BGM_NOZ };
