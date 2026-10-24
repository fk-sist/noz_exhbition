#include "DxLib.h"
#include <math.h>
#include <string.h>

#ifndef ENV_LOADED_H
#include "env_value.h"
#endif

#include "gameGenre.h"

// =====================================================
// ミニゲームの分類（ジャンルと形式）。使い方は gameGenre.h
// =====================================================

// ===== 調整値 =====
static const int GENRE_TAG_GAP = 10;        // ジャンルの札と、形式の札の間
static const int GENRE_TAG_PAD = 12;        // 札の中の、文字の左右の余白
static const int GENRE_TAG_SLANT = 10;      // 形式の札の、左右の辺の傾き（上の辺が右へずれる幅）

// ジャンルの表：gamelist.txt に書くキー・画面に出す名前・色 {R, G, B}
//static const char* genre_key[GENRE_NUM] = { "", "timing", "action", "reflex", "memory", "brain" };
//static const char* genre_name[GENRE_NUM] = { "", "タイミング", "アクション", "反射", "記憶", "知能" };
//static const char* genre_key[GENRE_NUM] = { "", "timing", "action", "reflex", "memory", "brain", "mash", "sense" };
//static const char* genre_name[GENRE_NUM] = { "", "タイミング", "アクション", "反射", "記憶", "知能", "連打", "感覚" };
static const char* genre_key[GENRE_NUM] = { "", "timing", "action", "reflex", "memory", "brain", "mash", "sense", "bet" };
static const char* genre_name[GENRE_NUM] = { "", "タイミング", "アクション", "反射", "記憶", "知能", "連打", "感覚", "賭け" };
static const int genre_col[GENRE_NUM][3] = {
	{ 120, 124, 140 },      // 無し
	{ 255, 186,  40 },      // タイミング：金
	{ 240,  82,  70 },      // アクション：赤
	{  50, 196, 250 },      // 反射：水色
	{ 172, 112, 255 },      // 記憶：紫
	{  84, 208, 118 },      // 知能：緑
	{ 255,  96, 168 },      // 連打：桃色
	{  92, 110, 240 },      // 感覚：藍色
	{ 186,  50,  78 },      // 賭け：えんじ
};

// 形式の表
static const char* format_key[FORMAT_NUM] = { "solo", "team", "1vs3", "coop" };
static const char* format_name[FORMAT_NUM] = { "個人戦", "タッグ", "１ＶＳ３", "協力" };
static const int format_col[FORMAT_NUM][3] = {
	{ 120, 124, 140 },      // 個人戦（札は出さない）
	{  76, 132, 255 },      // タッグ：青
	{ 255, 122,  48 },      // 1対3：橙
	{  60, 200, 150 },      // 協力：青緑
};


int GameGenre_FromKey(const char* key) {
	if (key == NULL) return GENRE_NONE;
	for (int i = 1; i < GENRE_NUM; i++) {
		if (strcmp(key, genre_key[i]) == 0 || strcmp(key, genre_name[i]) == 0) return i;   // 日本語の名前で書いてあってもよい
	}
	return GENRE_NONE;
}

int GameFormat_FromKey(const char* key) {
	if (key == NULL) return FORMAT_SOLO;
	for (int i = 0; i < FORMAT_NUM; i++) {
		if (strcmp(key, format_key[i]) == 0 || strcmp(key, format_name[i]) == 0) return i;
	}
	return FORMAT_SOLO;
}

const char* GameGenre_Name(int genre) {
	if (genre < 0 || genre >= GENRE_NUM) return "";
	return genre_name[genre];
}

const char* GameFormat_Name(int format) {
	if (format < 0 || format >= FORMAT_NUM) return "";
	return format_name[format];
}

unsigned int GameGenre_Color(int genre) {
	if (genre < 0 || genre >= GENRE_NUM) genre = GENRE_NONE;
	return GetColor(genre_col[genre][0], genre_col[genre][1], genre_col[genre][2]);
}

// 札に使うフォント（札の高さで決める）
static int GameGenre_Font(int h) {
	return (h >= 36) ? Fonts[3] : Fonts[7];
}

// 上から下へ色が変わる箱（x1 と x2 は、行ごとに skew だけ右へずらせる：上の端が一番右。0 ならまっすぐ）
static void GameGenre_GradBox(int x1, int y1, int x2, int y2, int skew, const int* top, const int* bottom) {
	int h = y2 - y1;
	if (h <= 0) return;
	for (int y = 0; y < h; y++) {
		float t = (float)y / (float)h;
		int ofs = (int)(skew * (1.0f - t));
		DrawLine(x1 + ofs, y1 + y, x2 + ofs, y1 + y,
			GetColor(top[0] + (int)((bottom[0] - top[0]) * t), top[1] + (int)((bottom[1] - top[1]) * t), top[2] + (int)((bottom[2] - top[2]) * t)));
	}
}

/*
*  アイコンの中の絵を描く。(cx, cy):真ん中  r:アイコンの半径  color:絵の色
*  影の分と本体の分で2回呼ぶので、色を引数にしている
*/
static void GameGenre_DrawSymbol(float cx, float cy, float r, int genre, unsigned int color) {
	switch (genre) {
	case GENRE_TIMING:
		// ストップウォッチ：輪・上のつまみ・針2本
		DrawCircleAA(cx, cy + r * 0.06f, r * 0.50f, 32, color, FALSE, r * 0.13f);
		DrawBoxAA(cx - r * 0.10f, cy - r * 0.68f, cx + r * 0.10f, cy - r * 0.46f, color, TRUE);
		DrawLineAA(cx, cy + r * 0.06f, cx, cy - r * 0.26f, color, r * 0.12f);
		DrawLineAA(cx, cy + r * 0.06f, cx + r * 0.24f, cy + r * 0.18f, color, r * 0.12f);
		break;

	case GENRE_ACTION:
		// 稲妻：三角を2つ、互い違いに
		DrawTriangleAA(cx + r * 0.16f, cy - r * 0.66f, cx - r * 0.36f, cy + r * 0.10f, cx + r * 0.10f, cy + r * 0.10f, color, TRUE);
		DrawTriangleAA(cx - r * 0.16f, cy + r * 0.66f, cx + r * 0.36f, cy - r * 0.10f, cx - r * 0.10f, cy - r * 0.10f, color, TRUE);
		break;

	case GENRE_REFLEX:
		// 十字のひらめき：細いひし形を縦と横に
		DrawQuadrangleAA(cx, cy - r * 0.70f, cx + r * 0.15f, cy, cx, cy + r * 0.70f, cx - r * 0.15f, cy, color, TRUE);
		DrawQuadrangleAA(cx - r * 0.70f, cy, cx, cy - r * 0.15f, cx + r * 0.70f, cy, cx, cy + r * 0.15f, color, TRUE);
		DrawCircleAA(cx, cy, r * 0.20f, 16, color, TRUE);
		break;

	case GENRE_MEMORY:
		// 重ねた札2枚：後ろは枠だけ、手前は塗り
		DrawBoxAA(cx - r * 0.46f, cy - r * 0.54f, cx + r * 0.14f, cy + r * 0.26f, color, FALSE, r * 0.10f);
		DrawBoxAA(cx - r * 0.16f, cy - r * 0.26f, cx + r * 0.46f, cy + r * 0.56f, color, TRUE);
		break;

	case GENRE_BRAIN:
		// 電球：丸い玉と、下の口金
		DrawCircleAA(cx, cy - r * 0.14f, r * 0.40f, 24, color, TRUE);
		DrawBoxAA(cx - r * 0.20f, cy + r * 0.18f, cx + r * 0.20f, cy + r * 0.40f, color, TRUE);
		DrawBoxAA(cx - r * 0.13f, cy + r * 0.46f, cx + r * 0.13f, cy + r * 0.60f, color, TRUE);
		break;

	case GENRE_MASH:
		// 押し込まれるボタン：台・その上のボタン・上へ飛ぶ3本の線（叩いた勢い）
		DrawBoxAA(cx - r * 0.50f, cy + r * 0.30f, cx + r * 0.50f, cy + r * 0.54f, color, TRUE);
		DrawBoxAA(cx - r * 0.30f, cy + r * 0.06f, cx + r * 0.30f, cy + r * 0.30f, color, TRUE);
		DrawOvalAA(cx, cy + r * 0.06f, r * 0.30f, r * 0.12f, 20, color, TRUE);
		DrawLineAA(cx, cy - r * 0.28f, cx, cy - r * 0.66f, color, r * 0.12f);
		DrawLineAA(cx - r * 0.26f, cy - r * 0.22f, cx - r * 0.50f, cy - r * 0.52f, color, r * 0.12f);
		DrawLineAA(cx + r * 0.26f, cy - r * 0.22f, cx + r * 0.50f, cy - r * 0.52f, color, r * 0.12f);
		break;

	case GENRE_BET:
		// 重ねたチップ：下に2枚（横から見た厚み）、上に1枚（内側に輪）
		DrawOvalAA(cx, cy + r * 0.44f, r * 0.50f, r * 0.20f, 20, color, TRUE);
		DrawOvalAA(cx, cy + r * 0.18f, r * 0.50f, r * 0.20f, 20, color, FALSE, r * 0.09f);
		DrawOvalAA(cx, cy - r * 0.14f, r * 0.50f, r * 0.30f, 24, color, TRUE);
		DrawLineAA(cx - r * 0.50f, cy - r * 0.14f, cx - r * 0.50f, cy + r * 0.44f, color, r * 0.09f);
		DrawLineAA(cx + r * 0.50f, cy - r * 0.14f, cx + r * 0.50f, cy + r * 0.44f, color, r * 0.09f);
		break;

	case GENRE_SENSE:
		// 波紋：真ん中の点と、広がる輪2つ（見えないものを感じ取る）
		DrawCircleAA(cx, cy, r * 0.15f, 16, color, TRUE);
		DrawCircleAA(cx, cy, r * 0.36f, 24, color, FALSE, r * 0.10f);
		DrawCircleAA(cx, cy, r * 0.60f, 32, color, FALSE, r * 0.08f);
		break;

	default:
		break;
	}
}

/*
*  丸いアイコンを描く。(cx, cy):真ん中  r:半径  alpha:濃さ(0～255)
*  縁取り → ジャンルの色の玉（下へ向かって暗くなる）→ 上半分のつや → 白い絵（下に影）
*/
void GameGenre_DrawIcon(int cx, int cy, int r, int genre, int alpha) {
	if (genre <= GENRE_NONE || genre >= GENRE_NUM || alpha <= 0) return;
	const int* c = genre_col[genre];
	float fx = (float)cx, fy = (float)cy, fr = (float)r;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawCircleAA(fx, fy, fr, 32, GetColor(16, 18, 34), TRUE);
	DrawCircleAA(fx, fy, fr - 2.0f, 32, GetColor(c[0] * 6 / 10, c[1] * 6 / 10, c[2] * 6 / 10), TRUE);
	DrawCircleAA(fx, fy - fr * 0.10f, fr * 0.84f, 32, GetColor(c[0], c[1], c[2]), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ADD, alpha * 70 / 255);
	DrawOvalAA(fx, fy - fr * 0.40f, fr * 0.60f, fr * 0.34f, 24, GetColor(255, 255, 255), TRUE);

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha * 150 / 255);
	GameGenre_DrawSymbol(fx + fr * 0.07f, fy + fr * 0.09f, fr, genre, GetColor(c[0] * 3 / 10, c[1] * 3 / 10, c[2] * 3 / 10));
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	GameGenre_DrawSymbol(fx, fy, fr, genre, GetColor(255, 255, 255));
	DrawCircleAA(fx, fy, fr, 32, GetColor(255, 255, 255), FALSE, 1.0f);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 札の中の、名前を書く所の幅。名前の長さに関係なく、一番長い名前に合わせる（どのジャンルでも札が同じ大きさになって、並べた時に揃う）
static int GameGenre_NameAreaWidth(int h, const char* names[], int from, int num) {
	int font = GameGenre_Font(h);
	int w = 0;
	for (int i = from; i < num; i++) {
		int nw = GetDrawStringWidthToHandle(names[i], (int)strlen(names[i]), font);
		if (nw > w) w = nw;
	}
	return w;
}

// ジャンルの札の幅（アイコン＋名前）。どのジャンルでも同じ幅
static int GameGenre_GenreTagWidth(int h, int genre) {
	if (genre <= GENRE_NONE || genre >= GENRE_NUM) return 0;
	//const char* name = genre_name[genre];
	//return h + 6 + GetDrawStringWidthToHandle(name, (int)strlen(name), GameGenre_Font(h)) + GENRE_TAG_PAD + h / 2;
	return h + 6 + GameGenre_NameAreaWidth(h, genre_name, 1, GENRE_NUM) + GENRE_TAG_PAD + h / 2;
}

// 1 の間は、形式の札を「詰めた形」で描く：人数の丸を出さず、名前だけ（幅の狭い所用。GameGenre_DrawTagsCompact が、描く間だけ 1 にする）
static int genre_compact = 0;

// 形式の札の幅（人数の丸の並び＋名前）
static int GameGenre_FormatTagWidth(int h, int format) {
	if (format <= FORMAT_SOLO || format >= FORMAT_NUM) return 0;
	const char* name = format_name[format];
	int pipW = (h / 3) * 4 + h / 4;     // 丸4つと、組の間のすき間
	//return GENRE_TAG_SLANT + GENRE_TAG_PAD + pipW + 8 + GetDrawStringWidthToHandle(name, (int)strlen(name), GameGenre_Font(h)) + GENRE_TAG_PAD;
	(void)name;
	// 詰めた形：丸の並びと、その後ろのすき間が無い。幅も、その名前ぴったりにする（少しでも狭く）
	if (genre_compact) return GENRE_TAG_SLANT + GENRE_TAG_PAD + GetDrawStringWidthToHandle(name, (int)strlen(name), GameGenre_Font(h)) + GENRE_TAG_PAD;
	return GENRE_TAG_SLANT + GENRE_TAG_PAD + pipW + 8 + GameGenre_NameAreaWidth(h, format_name, 1, FORMAT_NUM) + GENRE_TAG_PAD;   // どの形式でも同じ幅
}

int GameGenre_TagsWidth(int h, int genre, int format) {
	int gw = GameGenre_GenreTagWidth(h, genre);
	int fw = GameGenre_FormatTagWidth(h, format);
	return gw + fw + ((gw > 0 && fw > 0) ? GENRE_TAG_GAP : 0);
}

/*
*  形式の札と、ジャンルの札を、左からこの順で横に並べて描く。x:左端  cy:真ん中の高さ  h:札の高さ  戻り値は描いた幅
*  ジャンルの札：左に丸いアイコン、右へ伸びる角丸の帯に名前
*  形式の札    ：斜めに傾いた帯に、人数の丸（組ごとに間を空ける）と名前。丸い札のジャンルと、形で見分けが付く
*/
int GameGenre_DrawTags(int x, int cy, int h, int genre, int format, int alpha) {
	if (alpha <= 0) return GameGenre_TagsWidth(h, genre, format);
	int font = GameGenre_Font(h);
	int fh = GetFontSizeToHandle(font);
	int cur = x;

	// 並べる順は「形式 → ジャンル」。個人戦のゲームは形式の札が出ないので、右寄せで並べた時に、ジャンルの札の位置がどの行でも揃う
	//   （前は「ジャンル → 形式」の順で、タッグのゲームだけジャンルの札が左へずれていた）
	int gw = GameGenre_GenreTagWidth(h, genre);
	int fw = GameGenre_FormatTagWidth(h, format);

	// --- 形式 ---
	if (fw > 0) {
		const int* c = format_col[format];
		const char* name = format_name[format];
		int x1 = cur, x2 = cur + fw - GENRE_TAG_SLANT;
		int y1 = cy - h / 2 + 3, y2 = cy + h / 2 - 3;
		int top[3] = { c[0], c[1], c[2] };
		int bottom[3] = { c[0] * 45 / 100, c[1] * 45 / 100, c[2] * 45 / 100 };
		int dark[3] = { 16, 18, 34 };

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		GameGenre_GradBox(x1 - 3, y1 - 2, x2 + 3, y2 + 2, GENRE_TAG_SLANT, dark, dark);     // 縁取り
		GameGenre_GradBox(x1, y1, x2, y2, GENRE_TAG_SLANT, top, bottom);
		SetDrawBlendMode(DX_BLENDMODE_ADD, alpha * 50 / 255);
		{
			int white[3] = { 255, 255, 255 };
			GameGenre_GradBox(x1 + GENRE_TAG_SLANT / 2, y1 + 1, x2 + GENRE_TAG_SLANT / 2, y1 + (y2 - y1) / 2, GENRE_TAG_SLANT / 2, white, white);   // 上半分のつや
		}
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);

		// 人数の丸：組ごとに間を空ける（タッグ ●● ●●　1対3 ● ●●●　協力 ●●●●）
		int pr = h / 6, step = h / 3;
		int gapAfter = (format == FORMAT_TEAM) ? 1 : ((format == FORMAT_1VS3) ? 0 : -1);    // この番号の丸の後ろに、すき間を入れる
		int px = cur + GENRE_TAG_SLANT / 2 + GENRE_TAG_PAD + pr;
		//for (int i = 0; i < 4; i++) {
		for (int i = 0; i < 4 && !genre_compact; i++) {   // 詰めた形の時は、丸を出さない
			DrawCircleAA((float)px, (float)cy, (float)pr + 1.5f, 16, GetColor(16, 18, 34), TRUE);
			DrawCircleAA((float)px, (float)cy, (float)pr, 16, GetColor(255, 255, 255), TRUE);
			px += step;
			if (i == gapAfter) px += h / 4;
		}

		//int tx = cur + GENRE_TAG_SLANT / 2 + GENRE_TAG_PAD + step * 4 + h / 4 + 8;
		int tx = cur + GENRE_TAG_SLANT / 2 + GENRE_TAG_PAD + (genre_compact ? 0 : step * 4 + h / 4 + 8)
			+ (genre_compact ? 0 : (GameGenre_NameAreaWidth(h, format_name, 1, FORMAT_NUM) - GetDrawStringWidthToHandle(name, (int)strlen(name), font)) / 2);
		DrawStringToHandle(tx + 1, cy - fh / 2 + 1, name, GetColor(10, 10, 20), font);
		DrawStringToHandle(tx, cy - fh / 2, name, GetColor(255, 255, 255), font);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		cur += fw;
	}

	// --- ジャンル ---
	if (gw > 0) {
		if (fw > 0) cur += GENRE_TAG_GAP;
		const int* c = genre_col[genre];
		const char* name = genre_name[genre];
		int x1 = cur + h / 2, x2 = cur + gw;            // 帯は、アイコンの真ん中から右へ
		int y1 = cy - h / 2 + 3, y2 = cy + h / 2 - 3;
		int top[3] = { c[0] * 45 / 100 + 18, c[1] * 45 / 100 + 18, c[2] * 45 / 100 + 24 };
		int bottom[3] = { c[0] * 18 / 100 + 8, c[1] * 18 / 100 + 8, c[2] * 18 / 100 + 14 };

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		DrawRoundRect(x1 - 2, y1 - 2, x2 + 2, y2 + 2, (y2 - y1) / 2 + 2, (y2 - y1) / 2 + 2, GetColor(16, 18, 34), TRUE);
		GameGenre_GradBox(x1, y1, x2 - (y2 - y1) / 2, y2, 0, top, bottom);
		DrawCircleAA((float)(x2 - (y2 - y1) / 2), (float)(y1 + y2) / 2.0f, (float)(y2 - y1) / 2.0f, 24, GetColor((top[0] + bottom[0]) / 2, (top[1] + bottom[1]) / 2, (top[2] + bottom[2]) / 2), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ADD, alpha * 36 / 255);
		DrawBox(x1, y1 + 1, x2 - (y2 - y1) / 2, y1 + (y2 - y1) / 2, GetColor(255, 255, 255), TRUE);     // 上半分のつや
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
		DrawRoundRect(x1 - 2, y1 - 2, x2 + 2, y2 + 2, (y2 - y1) / 2 + 2, (y2 - y1) / 2 + 2, GetColor(c[0], c[1], c[2]), FALSE);

		//int tx = cur + h + 6;
		// 名前は、書く所の真ん中に置く（札の幅は一番長い名前に合わせてあるので、短い名前は左右が空く）
		int tx = cur + h + 6 + (GameGenre_NameAreaWidth(h, genre_name, 1, GENRE_NUM) - GetDrawStringWidthToHandle(name, (int)strlen(name), font)) / 2;
		DrawStringToHandle(tx + 1, cy - fh / 2 + 1, name, GetColor(10, 10, 20), font);
		DrawStringToHandle(tx, cy - fh / 2, name, GetColor(255, 255, 255), font);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		GameGenre_DrawIcon(cur + h / 2, cy, h / 2, genre, alpha);
		cur += gw;
	}

	return cur - x;
}

/*
*  GameGenre_DrawTags / GameGenre_TagsWidth の「詰めた形」：形式の札から人数の丸を外して、名前だけにする（ジャンルの札は同じ）
*  幅の狭い所で、形式とジャンルを1行に並べたい時に使う
*/
int GameGenre_TagsWidthCompact(int h, int genre, int format) {
	genre_compact = 1;
	int w = GameGenre_TagsWidth(h, genre, format);
	genre_compact = 0;
	return w;
}

int GameGenre_DrawTagsCompact(int x, int cy, int h, int genre, int format, int alpha) {
	genre_compact = 1;
	int w = GameGenre_DrawTags(x, cy, h, genre, format, alpha);
	genre_compact = 0;
	return w;
}
