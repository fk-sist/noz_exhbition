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
#include "gameGenre.h"   // ジャンルと形式の札(noz_kingdom から写した)

// =====================================================
// ノズキン(NOZ KINGDOM)のデモ(2026-10-24)
//   ブース案内(noz_guide):タイトル画面の景色とロゴ(ノズキンの BootManager.cpp の描き方を写した。ロゴは毎回、光の粒が集まって描かれる所から)
//                          → 帯が開いて、場所の案内(data/guide.txt)
//   ゲーム紹介カード(noz_cards):左に大きなサムネイル(ノズキンが撮ったプレイ中のクリップ。動く)、右にジャンル・名前・説明・人数。
//                          紹介するゲームは、クリップ(noz/img/thumb/<タグ>.png)があるゲームから。まぜた順に少しずつ、一巡したらまぜ直す
//   素材は noz/ の下(noz_kingdom から写した物。ゲームの一覧は noz/data/gamelist.txt)
// =====================================================

// --- ブース案内 ---
#define NZ_GUIDE_FRAMES     (12 * 60)   // 出している時間
#define NZ_LOGO1_Y          200         // ロゴの高さ(ノズキンのタイトル画面と同じ)
#define NZ_LOGO2_Y          422
#define NZ_PANEL_Y          610         // 案内の帯の上端
#define NZ_PANEL_H          330         // 案内の帯の高さ

// --- ゲーム紹介カード ---
#define NZ_CARD_FRAMES      (7 * 60)    // 1 枚を出している時間
#define NZ_CARD_NUM         4           // 1 回で紹介する数(data/demos.txt の数で替えられる)
#define NZ_CARD_NUM_MAX     8
#define NZ_CARD_IN          20          // 右から滑り込む長さ
#define NZ_CARD_OUT         16          // 左へ抜ける長さ
#define NZ_THUMB_X          110         // サムネイルの左上と大きさ(16:9)
#define NZ_THUMB_Y          250
#define NZ_THUMB_W          1104
#define NZ_THUMB_H          621
#define NZ_INFO_X           1290        // 右側の文字の左端と幅
#define NZ_INFO_W           540

// クリップの形(noz_kingdom の thumbClip.h と同じ。320x180 のコマを横 10 x 縦 12 に並べた 1 枚の PNG)
#define NZ_CLIP_W           320
#define NZ_CLIP_H           180
#define NZ_CLIP_FPS         12
#define NZ_CLIP_FRAMES      120
#define NZ_CLIP_COLS        10
#define NZ_CLIP_ROWS        12

#define NZ_GAME_MAX         128
#define RESOURCE_GROUP_NOZ_COMMON 10   // タイトルの景色・ロゴ・光(終了まで持つ)
#define RESOURCE_GROUP_NOZ_CARDS  11   // 紹介カードのクリップ(紹介が終わったら解放する。1 本 27MB くらいあるので、持ちっぱなしにしない)

extern ResourceManager resMgr;

// ノズキンの BootManager.cpp と同じ番号で使う絵(1 タイトルの景色 / 6・7 ロゴ / 9 光の玉 / 10 十字の輝き / 11 光の輪 / 12 光の柱)
static int nz_img[13];
static int nz_loaded = 0;

// 関数の役割:タイトルの景色・ロゴ・光の絵を読む(初めて使う時に 1 回だけ)
static void Noz_LoadCommon(void) {
	ResourceGroup* g;
	int i;
	if (nz_loaded) return;
	for (i = 0; i < 13; i++) nz_img[i] = -1;
	g = resMgr.CreateGroup(RESOURCE_GROUP_NOZ_COMMON);
	nz_img[1] = g->LoadImg("./noz/img/noztitle.png");
	nz_img[6] = g->LoadImg("./noz/img/titlenoz1.png");
	nz_img[7] = g->LoadImg("./noz/img/titlenoz2.png");
	nz_img[9] = g->LoadImg("./noz/img/title_glow.png");
	nz_img[10] = g->LoadImg("./noz/img/title_star.png");
	nz_img[11] = g->LoadImg("./noz/img/title_ring.png");
	nz_img[12] = g->LoadImg("./noz/img/title_beam.png");
	nz_loaded = 1;
}



// =====================================================
// 小さい道具
// =====================================================

// 影つきの字
static void Noz_ShadowString(int x, int y, const char* s, unsigned int col, int font) {
	DrawStringToHandle(x + 4, y + 4, s, GetColor(0, 0, 20), font);
	DrawStringToHandle(x, y, s, col, font);
}

// 関数の役割:幅 maxW で折り返して描く(CP932 の全角は 2 バイトで 1 字)。戻り値:最後の行の下端
static int Noz_DrawWrapped(int x, int y, int maxW, int lineH, const char* s, unsigned int col, int font) {
	char line[256];
	int len = 0, fh = GetFontSizeToHandle(font);
	const unsigned char* p = (const unsigned char*)s;
	while (*p != '\0') {
		int n = ((*p >= 0x81 && *p <= 0x9F) || (*p >= 0xE0 && *p <= 0xFC)) ? 2 : 1;   // CP932 の全角の 1 バイト目
		if (n == 2 && p[1] == '\0') n = 1;
		if (len + n >= (int)sizeof(line)) break;
		memcpy(line + len, p, n);
		if (len > 0 && GetDrawStringWidthToHandle(line, len + n, font) > maxW) {   // 入り切らない:ここまでを 1 行にして、この字は次の行の頭へ
			line[len] = '\0';
			Noz_ShadowString(x, y, line, col, font);
			y += lineH;
			len = 0;
			continue;
		}
		len += n;
		p += n;
	}
	if (len > 0) { line[len] = '\0'; Noz_ShadowString(x, y, line, col, font); y += lineH; }
	return y - lineH + fh;
}

// 関数の役割:左上の見出し(英字の題 + 日本語の小見出し。金の線)。t は出てからのフレーム(左から滑り込む)
static void Noz_DrawHeader(const char* title, const char* sub, int t) {
	float r = (t < 16) ? t / 16.0f : 1.0f;
	int ofs = (int)(-700 * (1.0f - r) * (1.0f - r) * (1.0f - r));
	int x = 70 + ofs, y = 60;
	int tw = GetDrawStringWidthToHandle(title, (int)strlen(title), Fonts[4]);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 170);
	DrawBox(0, y - 10, x + tw + 60, y + 128, GetColor(6, 8, 26), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawBox(0, y + 128, x + tw + 60, y + 131, GetColor(230, 190, 80), TRUE);
	DrawBox(x - 26, y + 6, x - 18, y + 112, GetColor(230, 190, 80), TRUE);
	Noz_ShadowString(x, y, title, GetColor(255, 214, 70), Fonts[4]);
	Noz_ShadowString(x + 4, y + 82, sub, GetColor(225, 230, 245), Fonts[6]);
}

// ===== ここから noz_kingdom の BootManager.cpp から写した(2026-10-24。中身はそのまま。絵の配列 img[] だけ nz_img[] に名前を替えた) =====
// ===== タイトルロゴ演出の設定 =====
static const int LOGO_PTCL_MAX = 200;     // ロゴ1枚あたりの光の粒の最大数
static const int LOGO_PTCL_STEP = 10;     // ロゴの横幅 何pxごとに粒を1個飛ばすか
static const int LOGO_FLY_DUR = 35;       // 粒が飛んでくるのにかかる時間
static const int LOGO_WIPE_SPEED = 30;    // ロゴを左から右へ描いていく速さ(px/フレーム)
static const int LOGO_BEAM_LEN = 1000;    // 光の柱の長さ（上下合わせて・px）
static const int LOGO_BEAM_STEP = 120;    // ロゴの横幅 何pxごとに光の柱を1本閃かせるか（今の速さだと4フレームに1本）
static const int LOGO_BEAM_DUR = 10;      // 光の柱1本が閃いてから消えるまでの時間
static const int LOGO_BEAM_LAST_DUR = 28; // 最後の1本（右端の大きい柱）が消えるまでの時間
static const int LOGO_POP_DUR = 8;        // ロゴの切れ端が細長い状態から本来の形に収まるまでの時間
static const double LOGO_POP_STRETCH = 6.0;  // 現れた瞬間の縦の引き伸ばし倍率
static const int LOGO_SHINE_DUR = 30;     // キラーンの光の帯がロゴを横切る時間
static const int LOGO_SHINE_CYCLE = 300;  // 描き終わった後、キラーンを繰り返す間隔
static const int LOGO_SHINE_WIDTH = 120;  // キラーンの光の帯の幅(px)
// ---- ここから（2026-10-19）タイトル名を NOZ KINGDOM に変えて、ロゴを作り直した時に足した ----
// ロゴの中の光らせる所（ロゴの画像の真ん中からの位置・px）
//   ロゴの画像は docs/title_logo_gen.py で作っている。ロゴを作り直したら、ここも測り直す
//static const int LOGO_STAR_POS[2][2] = { { 2, -166 }, { 686, 18 } };  // 描き終わりのキラーン：紋章は王冠の十字の先、本題は剣の切っ先
static const int LOGO_STAR_POS[2][2] = { { 2, -166 }, { 730, 49 } };  // 描き終わりのキラーン：紋章は王冠の十字の先、本題は剣の切っ先（N・K を大きくした版で測り直した）
static const double LOGO_STAR_SIZE[2] = { 1.0, 1.5 };                 // その大きさ（剣の切っ先は大きく）
// 宝石のきらめき：出揃った時と、その後はキラーンと同じ間隔で、左から順に宝石が瞬く
struct LogoGlint { int no, dx, dy, r, g, b; };   // no:どちらのロゴか  dx,dy:ロゴの真ん中からの位置  r,g,b:光の色
static const LogoGlint LOGO_GLINTS[] = {
//	{ 1, -670, 22, 255, 120, 120 },   // 剣の柄頭のルビー
//	{ 1, -617, 21, 255, 120, 120 },   // 剣のつばのルビー
	// ↑ N・K を大きくして、本題のロゴの絵が大きくなったので測り直した（下）。N・K に埋めたサファイアも足した
	{ 1, -713, 52, 255, 120, 120 },   // 剣の柄頭のルビー
	{ 1, -661, 53, 255, 120, 120 },   // 剣のつばのルビー
	{ 1, -452, 30, 140, 190, 255 },   // N のサファイア
	{ 1, -121, -6, 140, 190, 255 },   // K のサファイア
//	{ 0,  -76, 64, 140, 190, 255 },   // リボンのサファイア（左）  … 本題のロゴを上げて、K の後ろに隠れたので外した
	{ 0,  -76, 64, 140, 190, 255 },   // リボンのサファイア（左）  … TITLE2_Y を 380 に下げて、また見えるようになったので戻した
	{ 0,  -54,  2, 140, 190, 255 },   // 王冠のサファイア（左）
//	{ 0,    2, 62, 255, 120, 120 },   // リボンのルビー            … 同上
	{ 0,    2, 62, 255, 120, 120 },   // リボンのルビー            … 同上（戻した）
	{ 0,    2,  2, 255, 120, 120 },   // 王冠のルビー
	{ 0,   58,  2, 140, 190, 255 },   // 王冠のサファイア（右）
	{ 0,   80, 64, 140, 190, 255 },   // リボンのサファイア（右）
};
static const int LOGO_GLINT_NUM = sizeof(LOGO_GLINTS) / sizeof(LOGO_GLINTS[0]);
static const int LOGO_GLINT_STEP = 5;     // 宝石1つごとの遅れ
static const int LOGO_GLINT_DUR = 22;     // 宝石1つが瞬いて消えるまでの時間
// 出揃う瞬間（衝撃波と同時）に、ロゴの裏から金の火花が飛び散る
static const int TITLE_SPARK_NUM = 56;    // 火花の数
static const int TITLE_SPARK_DUR = 54;    // 飛び散って消えるまでの時間
// 出揃った後、紋章の後ろでゆっくり回る光の筋（後光）。TITLE_RAYS_ALPHA を 0 にすれば出ない
static const int TITLE_RAYS_ALPHA = 70;   // 明るさ（加算）
static const int TITLE_RAYS_NUM = 7;      // 光の柱の本数（1本で上下2方向に伸びるので、筋はこの倍）
static const int TITLE_RAYS_FADE = 40;    // 出揃ってから、明るくなりきるまでの時間
static const int TITLE_SUN_X = 240, TITLE_SUN_Y = 136;   // 背景の絵（画面いっぱいに描いた時）の中の、太陽の位置

struct LogoPtcl { float rate, ry, dist, angle, size; int color; };
static LogoPtcl logoPtcl[2][LOGO_PTCL_MAX];
static bool logoPtcl_init[2] = { false, false };

// ロゴを描き終わるのにかかるフレーム数（横幅が違っても同じ速さで描く）
static int Title_LogoWipeDur(int grHandle) {
	int w, h;
	GetGraphSize(grHandle, &w, &h);
	return w / LOGO_WIPE_SPEED;
}

// ロゴ1枚ぶんの出現演出
//   光の粒が渦を巻いて集まる → 集まった所から左→右へ、光の柱が閃くたびに少しずつ現れる → 描き終わりにキラーン
//   no:ロゴ番号(0,1)  cx,cy:ロゴの中心  startT:描き始めるフレーム
//   光は nz_img[9]（光の玉）と nz_img[10]（十字の輝き）を加算で重ねて描く。色は SetDrawBright で付ける
static void Title_DrawLogo(int no, int grHandle, int cx, int cy, int t, int startT) {
	int w, h;
	GetGraphSize(grHandle, &w, &h);
	int x0 = cx - w / 2, y0 = cy - h / 2;
	int localT = t - startT;  // 描き始めを0とした時間（粒はマイナスの時間から飛び始める）
	int wipeDur = Title_LogoWipeDur(grHandle);
	int ptclNum = min(LOGO_PTCL_MAX, w / LOGO_PTCL_STEP);

	// ロゴの配色（白っぽいハイライト・黄・朱）に合わせた光の色 {R, G, B}
	static const int lightColor[3][3] = { {255, 245, 200}, {255, 220, 60}, {255, 130, 40} };

	if (!logoPtcl_init[no]) {
		for (int i = 0; i < ptclNum; i++) {
			logoPtcl[no][i].rate = (float)i / ptclNum;              // ロゴの左端0.0 - 右端1.0 のどこへ向かうか
			logoPtcl[no][i].ry = GetRand(100) * 0.01f;              // ロゴの上端0.0 - 下端1.0 のどこへ向かうか
			logoPtcl[no][i].dist = 200.0f + GetRand(400);           // どれだけ遠くから飛んでくるか
			logoPtcl[no][i].angle = (float)GetRand(628) * 0.01f;    // どの方向から飛んでくるか
			logoPtcl[no][i].size = 0.3f + GetRand(40) * 0.01f;      // 光の玉の拡大率 0.3 - 0.7
			logoPtcl[no][i].color = GetRand(2);
		}
		logoPtcl_init[no] = true;
	}

	// 光の先頭の位置（十字の輝きを出す所）
	float wipe = max(0.0f, min(1.0f, (float)localT / wipeDur));
	int headX = (int)(w * wipe);

	// --- ロゴ本体：光の柱が閃くのに合わせて、柱1本ぶんの幅ずつ現れる ---
	//   縦に細長く引き伸ばされた状態で光りながら現れて、本来の形に縮んで収まる
	int beamNum = max(1, w / LOGO_BEAM_STEP);
	for (int k = 0; k < beamNum; k++) {
		int sx = w * k / beamNum;                                  // 切り出す範囲の左端
		int sw = w * (k + 1) / beamNum - sx;                       // 切り出す幅
		int age = localT - (int)(wipeDur * (k + 0.5f) / beamNum);  // 光の柱が閃いてからの時間（柱と同じ式）
		if (age < 0) continue;
		if (age >= LOGO_POP_DUR) {
			DrawRectGraph(x0 + sx, y0, sx, 0, sw, h, grHandle, TRUE, FALSE);  // 収まった後はそのまま描く
			continue;
		}

		float rest = 1.0f - (float)age / LOGO_POP_DUR;             // 1.0:現れた瞬間 → 0.0:収まる
		float r3 = rest * rest * rest;                             // 最初に一気に縮む
		double rateX = 1.0 - 0.6 * r3;                             // 横：0.4倍 → 1.0倍
		double rateY = 1.0 + (LOGO_POP_STRETCH - 1.0) * r3;        // 縦：LOGO_POP_STRETCH倍 → 1.0倍
		int px = x0 + sx + sw / 2, py = y0 + h / 2;                // 切れ端の中心を軸に伸び縮みさせる
		DrawRectRotaGraph3(px, py, sx, 0, sw, h, sw / 2, h / 2, rateX, rateY, 0.0, grHandle, TRUE, FALSE);

		// 現れた瞬間は光らせる（自分自身を加算で2回重ねる。収まるにつれて元の色に戻る）
		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * rest));
		DrawRectRotaGraph3(px, py, sx, 0, sw, h, sw / 2, h / 2, rateX, rateY, 0.0, grHandle, TRUE, FALSE);
		DrawRectRotaGraph3(px, py, sx, 0, sw, h, sw / 2, h / 2, rateX, rateY, 0.0, grHandle, TRUE, FALSE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	// --- 光の粒：尾を引いて飛んできて、光の先頭が通るタイミングで到着してキラッと弾ける ---
	const int TAIL_NUM = 8;   // 尾の残像の数（0.5フレーム刻みなので4フレームぶんの尾）
	const int POP_DUR = 14;   // 到着してから弾けて消えるまでの時間
	for (int i = 0; i < ptclNum; i++) {
		LogoPtcl* p = &logoPtcl[no][i];
		int flyT = localT - ((int)(wipeDur * p->rate) - LOGO_FLY_DUR);  // 飛び始めを0とした時間
		if (flyT < 0 || flyT >= LOGO_FLY_DUR + POP_DUR) continue;

		float tx = x0 + w * p->rate;
		float ty = y0 + h * p->ry;
		SetDrawBright(lightColor[p->color][0], lightColor[p->color][1], lightColor[p->color][2]);

		if (flyT < LOGO_FLY_DUR) {
			// 飛行中：後ろの残像から順に描く（後ろほど小さく薄い）
			float twinkle = 0.8f + 0.2f * sinf(t * 0.5f + i);  // 粒ごとにちらちら瞬く
			for (int k = TAIL_NUM - 1; k >= 0; k--) {
				float q = (flyT - k * 0.5f) / LOGO_FLY_DUR;  // 0.0:飛び始め → 1.0:到着
				if (q < 0.0f) continue;
				float rest = 1.0f - q * q;               // 残りの距離（加速しながら吸い込まれる）
				float ang = p->angle + rest * 2.5f;      // 渦を巻きながら近づく
				int px = (int)(tx + cosf(ang) * p->dist * rest);
				int py = (int)(ty + sinf(ang) * p->dist * rest);
				float fade = 1.0f - (float)k / TAIL_NUM;
				SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * min(1.0f, q * 4.0f) * fade));
				DrawRotaGraph(px, py, p->size * fade * twinkle, 0.0, nz_img[9], TRUE);
			}
		}
		else {
			// 到着：十字の輝きが回りながら広がって消える
			float q = (float)(flyT - LOGO_FLY_DUR) / POP_DUR;
			SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * (1.0f - q)));
			DrawRotaGraph((int)tx, (int)ty, p->size * (0.5f + 1.0f * q), q * 1.5f, nz_img[10], TRUE);
		}
	}

	// --- 連続フラッシュ：光の先頭が通った所で、光の柱が上下に次々と閃く（しゅぴぴぴ） ---
	//   柱は横には動かない。その場で太く閃いて、細くなって消える
	//   最後の1本だけ右端で大きく長く残る（ぃーーん）
	SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
	for (int k = 0; k <= beamNum; k++) {
		bool last = (k == beamNum);
		float rate = last ? 1.0f : (k + 0.5f) / beamNum;   // ロゴの左端0.0 - 右端1.0 のどこで閃くか
		int dur = last ? LOGO_BEAM_LAST_DUR : LOGO_BEAM_DUR;
		int age = localT - (int)(wipeDur * rate);          // 閃いてからの時間
		if (age < 0 || age >= dur) continue;

		float p = (float)age / dur;                        // 0.0:閃いた瞬間 → 1.0:消える
		float grow = min(1.0f, (age + 1) / 3.0f);          // 3フレームで上下に伸びきる
		float vary = 0.7f + 0.15f * ((k * 7) % 5);         // 1本ごとに長さを変える 0.7 - 1.3
		double lenRate = LOGO_BEAM_LEN / 512.0 * grow * (last ? 1.6 : vary);  // 512 は光の柱の画像の高さ
		double widRate = (last ? 4.5 : 3.0) * (1.0f - p) * (1.0f - p);        // 太く閃いて細く消える
		int bx = x0 + (int)(w * rate), by = y0 + h / 2;

		SetDrawBright(lightColor[1][0], lightColor[1][1], lightColor[1][2]);   // 外側：黄色く太い光
		DrawRotaGraph3(bx, by, 32, 256, widRate * 2.0, lenRate, 0.0, nz_img[12], TRUE);  // 32,256 は光の柱(64x512)の中心
		SetDrawBright(255, 255, 255);                                          // 芯：白く細い光
		DrawRotaGraph3(bx, by, 32, 256, widRate, lenRate, 0.0, nz_img[12], TRUE);
	}

	// --- 光の先頭：描いている最中だけ、回る十字の輝き ---
	if (localT >= 0 && localT < wipeDur) {
		SetDrawBright(lightColor[0][0], lightColor[0][1], lightColor[0][2]);
		DrawRotaGraph(x0 + headX, y0 + h / 2, 1.2, t * 0.15, nz_img[10], TRUE);
	}

	// --- キラーン：描き終わった瞬間と、その後は一定間隔で光の帯が横切る ---
	SetDrawBright(255, 255, 255);
	if (localT >= wipeDur) {
		int s = localT - wipeDur;
		int cyc = s % LOGO_SHINE_CYCLE;
		if (cyc < LOGO_SHINE_DUR) {
			// ロゴ自身を細切りにして加算で重ねるので、ロゴの形にだけ光が乗る
			const int SLICE_NUM = 12;
			const int sliceW = LOGO_SHINE_WIDTH / SLICE_NUM;
			int bandLeft = -LOGO_SHINE_WIDTH + (w + LOGO_SHINE_WIDTH) * cyc / LOGO_SHINE_DUR;
			for (int k = 0; k < SLICE_NUM; k++) {
				int sx = bandLeft + k * sliceW;
				if (sx < 0 || sx + sliceW > w) continue;
				SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(200 * sinf(DX_PI_F * (k + 0.5f) / SLICE_NUM)));  // 帯の中央ほど明るい
				DrawRectGraph(x0 + sx, y0, sx, 0, sliceW, h, grHandle, TRUE, FALSE);
			}
		}

		// 描き終わった瞬間だけ、最後の文字の右肩に十字の輝き（画像に余白が無いので右上の角から少し内側）
		const int STAR_DUR = 30;
		if (s < STAR_DUR) {
			//int starX = x0 + w - 20, starY = y0 + 20;
			// ↑ 右上の角から少し内側に出していた。NOZ KINGDOM のロゴにした時に、紋章は王冠の十字の先、本題は剣の切っ先で光らせるようにした
			int starX = cx + LOGO_STAR_POS[no][0], starY = cy + LOGO_STAR_POS[no][1];
			float size = sinf(DX_PI_F * s / STAR_DUR) * (float)LOGO_STAR_SIZE[no];  // 0.0 → 1.0 → 0.0
			SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
			DrawRotaGraph(starX, starY, 2.5 * size, 0.0, nz_img[9], TRUE);
			DrawRotaGraph(starX, starY, 3.0 * size, s * 0.03, nz_img[10], TRUE);
		}
	}

	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}





static void Title_DrawBackdrop(float zoom, float focusX, float focusY, float dawn, int t) {
	const int W = SCREEN_WIDTH, H = SCREEN_HEIGHT;
	float dw = W * zoom, dh = H * zoom;
	float x1 = W / 2 - focusX * dw, y1 = H / 2 - focusY * dh;
	if (x1 > 0) x1 = 0;
	if (y1 > 0) y1 = 0;
	if (x1 + dw < W) x1 = W - dw;
	if (y1 + dh < H) y1 = H - dh;

	// 景色（夜明け前は、暗く青い）
	int r = (int)(36 + 219 * dawn), g = (int)(46 + 209 * dawn), b = (int)(92 + 163 * dawn);
	SetDrawBright(r, g, b);
	SetDrawMode(DX_DRAWMODE_BILINEAR);
	DrawExtendGraph((int)x1, (int)y1, (int)(x1 + dw), (int)(y1 + dh), nz_img[1], FALSE);
	SetDrawMode(DX_DRAWMODE_NEAREST);
	SetDrawBright(255, 255, 255);

	// 太陽：絵の中の太陽の位置に、ぼんやりした光と、ゆっくり回る光の筋（nz_img[12] 光の柱）
	float sunX = x1 + TITLE_SUN_X * zoom, sunY = y1 + TITLE_SUN_Y * zoom;
	SetDrawBright(255, 222, 160);
	SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(200 * dawn));
	DrawRotaGraph((int)sunX, (int)sunY, 5.0 + 4.0 * dawn, 0.0, nz_img[9], TRUE);
	for (int k = 0; k < 9; k++) {
		double ang = t * 0.0025 + k * DX_PI / 9.0;
		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)((26 + 14 * sinf(t * 0.02f + k * 1.9f)) * dawn));
		DrawRotaGraph3((int)sunX, (int)sunY, 32, 256, 2.2 + (k % 3) * 0.6, 3.6 + (k % 4) * 0.4, ang, nz_img[12], TRUE);
	}

	// 下の方を、ゆっくり流れる霧（大きくて薄い光の玉を、横長に引き伸ばして重ねる）
	SetDrawBright(220, 230, 255);
	for (int k = 0; k < 7; k++) {
		int span = W + 1200;
		int mx = (k * 397 + t * (1 + k % 3)) % span - 600;
		int my = H * 62 / 100 + (k * 71) % 220;
		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)((18 + (k % 3) * 6) * (0.4f + 0.6f * dawn)));
		DrawRotaGraph3(mx, my, 32, 32, 12.0 + (k % 3) * 3.0, 2.6, 0.0, nz_img[9], TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	SetDrawBright(255, 255, 255);
}

// ===== ここまで =====


// =====================================================
// ゲームの一覧(noz/data/gamelist.txt。ノズキンと同じ書き方:[game] の下に key = 値)
// =====================================================

typedef struct {
	char tag[64];
	char name[128];
	char help[256];
	int  genre, format, players;
	int  has_clip;   // クリップ(noz/img/thumb/<タグ>.png)があるか
} NozGame;

static NozGame nz_games[NZ_GAME_MAX];
static int nz_game_num = 0;
static int nz_order[NZ_GAME_MAX];   // 紹介する順(クリップがあるゲームだけ。まぜてある)
static int nz_order_num = 0, nz_order_pos = 0;
static int nz_list_loaded = 0;

static void Noz_ClipPath(const char* tag, char* out, int cap) { _snprintf(out, cap, "./noz/img/thumb/%s.png", tag); out[cap - 1] = '\0'; }

static void Noz_Shuffle(void) {
	int i;
	for (i = nz_order_num - 1; i > 0; i--) {
		int j = GetRand(i), t = nz_order[i];
		nz_order[i] = nz_order[j]; nz_order[j] = t;
	}
	nz_order_pos = 0;
}

// 関数の役割:ゲームの一覧を読む(初めて使う時に 1 回だけ)。visible = false のゲームは入れない
static void Noz_LoadList(void) {
	FILE* fp;
	char line[512];
	NozGame* cur = NULL;
	int visible = 1, i;
	if (nz_list_loaded) return;
	nz_list_loaded = 1;
	nz_game_num = 0;
	fp = fopen("./noz/data/gamelist.txt", "r");
	if (fp == NULL) return;
	while (Demo_ReadLine(fp, line, sizeof(line))) {
		char *k, *v, *e;
		if (strcmp(line, "[game]") == 0) {
			if (cur != NULL && visible) nz_game_num++;   // 1 つ前のゲームを確定
			if (nz_game_num >= NZ_GAME_MAX) { cur = NULL; break; }
			cur = &nz_games[nz_game_num];
			memset(cur, 0, sizeof(*cur));
			cur->players = 1;
			visible = 1;
			continue;
		}
		if (cur == NULL) continue;
		e = strchr(line, '=');
		if (e == NULL) continue;
		*e = '\0';
		k = line; v = e + 1;
		while (*v == ' ' || *v == '\t') v++;
		{ int n = (int)strlen(k); while (n > 0 && (k[n - 1] == ' ' || k[n - 1] == '\t')) k[--n] = '\0'; }
		if (strcmp(k, "tag") == 0) strncpy(cur->tag, v, sizeof(cur->tag) - 1);
		else if (strcmp(k, "name") == 0) strncpy(cur->name, v, sizeof(cur->name) - 1);
		else if (strcmp(k, "help") == 0) strncpy(cur->help, v, sizeof(cur->help) - 1);
		else if (strcmp(k, "genre") == 0) cur->genre = GameGenre_FromKey(v);
		else if (strcmp(k, "format") == 0) cur->format = GameFormat_FromKey(v);
		else if (strcmp(k, "players") == 0) cur->players = atoi(v);
		else if (strcmp(k, "visible") == 0) visible = (strcmp(v, "true") == 0);
	}
	if (cur != NULL && visible && nz_game_num < NZ_GAME_MAX) nz_game_num++;
	fclose(fp);

	// クリップがあるゲームだけを、紹介する順に
	nz_order_num = 0;
	for (i = 0; i < nz_game_num; i++) {
		char path[256];
		FILE* f;
		Noz_ClipPath(nz_games[i].tag, path, sizeof(path));
		f = fopen(path, "rb");
		nz_games[i].has_clip = (f != NULL);
		if (f != NULL) { fclose(f); nz_order[nz_order_num++] = i; }
	}
	Noz_Shuffle();
}


// =====================================================
// ブース案内(noz_guide)
// =====================================================

static int ng_t = 0;

static int NozGuide_Begin(int arg) {
	(void)arg;
	Noz_LoadCommon();
	Noz_LoadList();
	ng_t = 0;
	return 1;
}

static int NozGuide_Update(void) {
	ng_t++;
	return (ng_t >= NZ_GUIDE_FRAMES) ? 1 : 0;
}

static void NozGuide_Draw(void) {
	const int t = ng_t, cx = ScreenCenter_X;
	const int logo1Start = 35;                                         // ノズキンのタイトル画面の TITLE1_START と同じ
	const int logo2Start = logo1Start + Title_LogoWipeDur(nz_img[6]) - 5;
	const int logoDone = logo2Start + Title_LogoWipeDur(nz_img[7]);
	int pt, i;

	// 景色:ゆっくり横へ流す
	Title_DrawBackdrop(1.12f, 0.35f + 0.3f * (float)t / NZ_GUIDE_FRAMES, 0.45f, 1.0f, t);
	Title_DrawLogo(0, nz_img[6], cx, NZ_LOGO1_Y, t, logo1Start);
	Title_DrawLogo(1, nz_img[7], cx, NZ_LOGO2_Y, t, logo2Start);

	// 案内の帯:ロゴを描き終わったら、真ん中から上下に開く
	pt = t - logoDone;
	if (pt < 0) return;
	{
		float open = (pt < 16) ? pt / 16.0f : 1.0f;
		int midY = NZ_PANEL_Y + NZ_PANEL_H / 2, hh, y1, y2, y, sx;
		open = 1.0f - (1.0f - open) * (1.0f - open) * (1.0f - open);
		hh = (int)(NZ_PANEL_H / 2 * open);
		y1 = midY - hh; y2 = midY + hh;
		for (y = y1; y < y2; y += 2) {   // 地:上下の端ほど薄くなる黒
			float d = fabsf((float)(y - midY)) / (NZ_PANEL_H / 2);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(200 * (1.0f - d * d * 0.6f)));
			DrawBox(0, y, SCREEN_WIDTH, y + 2, GetColor(8, 10, 28), TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		DrawBox(0, y1, SCREEN_WIDTH, y1 + 3, GetColor(230, 190, 80), TRUE);   // 上下の金の線と、線の上を走る光
		DrawBox(0, y2 - 3, SCREEN_WIDTH, y2, GetColor(230, 190, 80), TRUE);
		sx = (t * 14) % (SCREEN_WIDTH + 800) - 400;
		SetDrawBright(255, 230, 150);
		SetDrawBlendMode(DX_BLENDMODE_ADD, 220);
		DrawRotaGraph3(sx, y1 + 1, 32, 32, 8.0, 0.5, 0.0, nz_img[9], TRUE);
		DrawRotaGraph3(SCREEN_WIDTH - sx, y2 - 1, 32, 32, 8.0, 0.5, 0.0, nz_img[9], TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		SetDrawBright(255, 255, 255);
	}
	if (pt < 16) return;

	// 一番大きい文:大きい所から叩きつけられて収まる。収まった瞬間に光り、その後もときどき光の帯が横切る
	{
		int mt = pt - 16, font = Fonts[8];
		const char* s = guide.main;
		int tw = GetDrawStringWidthToHandle(s, (int)strlen(s), font), fh = GetFontSizeToHandle(font);
		float r = (mt < 12) ? mt / 12.0f : 1.0f;
		double scale = 1.0 + 1.2 * (1.0 - r) * (1.0 - r);
		int ty = NZ_PANEL_Y + 40;
		int x = cx - (int)(tw * scale / 2), y = ty + fh / 2 - (int)(fh * scale / 2);
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(255 * r));
		DrawExtendStringToHandle(x + 6, y + 6, scale, scale, s, GetColor(30, 10, 0), font);
		DrawExtendStringToHandle(x, y, scale, scale, s, GetColor(255, 214, 70), font);
		if (mt >= 12 && mt < 40) {
			SetDrawBlendMode(DX_BLENDMODE_ADD, 255 - (mt - 12) * 9);
			DrawExtendStringToHandle(x, y, scale, scale, s, GetColor(255, 255, 255), font);
		}
		if (mt >= 12 && (mt - 12) % 300 < 40) {
			int cyc = (mt - 12) % 300, bx = x - 200 + (tw + 400) * cyc / 40;
			SetDrawArea(x, ty, x + tw, ty + fh);
			SetDrawBright(255, 245, 200);
			SetDrawBlendMode(DX_BLENDMODE_ADD, 200);
			DrawRotaGraph3(bx, ty + fh / 2, 32, 32, 2.0, 4.0, 0.35, nz_img[9], TRUE);
			SetDrawBright(255, 255, 255);
			SetDrawArea(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		if (mt >= 12 && mt < 42) {   // 収まった所に、広がる光の輪と、文字の右端の十字の輝き
			float p = (mt - 12) / 30.0f;
			SetDrawBright(255, 220, 120);
			SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(255 * (1.0f - p)));
			DrawRotaGraph(cx, ty + fh / 2, 0.6 + 2.6 * p, 0.0, nz_img[11], TRUE);
			DrawRotaGraph(x + tw, ty + 10, 1.6 * (1.0f - p), mt * 0.1, nz_img[10], TRUE);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			SetDrawBright(255, 255, 255);
		}
	}

	// その下の文:1 行ずつ右から滑り込む(%d はゲームの数)
	for (i = 0; i < guide.sub_num; i++) {
		int st = pt - 40 - i * 12, tw, x;
		char buf[GUIDE_TEXT_LEN + 16];
		float r, e;
		if (st < 0) continue;
		_snprintf(buf, sizeof(buf), guide.sub[i], nz_game_num);
		buf[sizeof(buf) - 1] = '\0';
		tw = GetDrawStringWidthToHandle(buf, (int)strlen(buf), Fonts[5]);
		r = (st < 16) ? st / 16.0f : 1.0f;
		e = 1.0f - (1.0f - r) * (1.0f - r) * (1.0f - r);
		x = cx - tw / 2 + (int)(700 * (1.0f - e));
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, (int)(255 * r));
		Noz_ShadowString(x, NZ_PANEL_Y + 170 + i * 70, buf, GetColor(240, 244, 255), Fonts[5]);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

static void NozGuide_End(void) {}

const DemoDef demo_noz_guide = { "noz_guide", NozGuide_Begin, NozGuide_Update, NozGuide_Draw, NozGuide_End, 1, DEMO_BGM_NOZ };


// =====================================================
// ゲーム紹介カード(noz_cards)
// =====================================================

static int nc_t = 0;
static int nc_num = 0;
static int nc_pick[NZ_CARD_NUM_MAX];               // 紹介するゲーム(nz_games の番号)
static int nc_clip[NZ_CARD_NUM_MAX][NZ_CLIP_FRAMES];   // そのクリップのコマ(読めなければ [0] が -1)

static int NozCards_Begin(int arg) {
	int k, n = (arg > 0) ? arg : NZ_CARD_NUM;
	if (n > NZ_CARD_NUM_MAX) n = NZ_CARD_NUM_MAX;
	Noz_LoadCommon();
	Noz_LoadList();
	if (nz_order_num == 0) return 0;   // クリップが 1 本も無い時は飛ばす
	if (n > nz_order_num) n = nz_order_num;
	nc_num = 0;
	for (k = 0; k < n; k++) {
		ResourceGroup* g = resMgr.CreateGroup(RESOURCE_GROUP_NOZ_CARDS);
		char path[256];
		int gi;
		if (nz_order_pos >= nz_order_num) Noz_Shuffle();
		gi = nz_order[nz_order_pos++];
		nc_pick[nc_num] = gi;
		Noz_ClipPath(nz_games[gi].tag, path, sizeof(path));
		if (g->LoadDivImg(path, NZ_CLIP_FRAMES, NZ_CLIP_COLS, NZ_CLIP_ROWS, NZ_CLIP_W, NZ_CLIP_H, nc_clip[nc_num]) == -1) nc_clip[nc_num][0] = -1;
		nc_num++;
	}
	nc_t = 0;
	return 1;
}

static int NozCards_Update(void) {
	nc_t++;
	return (nc_t >= NZ_CARD_FRAMES * nc_num) ? 1 : 0;
}

static void NozCards_Draw(void) {
	int k, ct, gi, ofs, rest, alpha, x1, y1, x2, y2, it, ix, y, w;
	float in, ein;
	const NozGame* g;

	// 景色:タイトルの景色を暗く沈めて、文字を立たせる
	Title_DrawBackdrop(1.15f, 0.5f + 0.15f * sinf(nc_t * 0.002f), 0.5f, 0.35f, nc_t);
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 110);
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(0, 0, 10), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	Noz_DrawHeader("PICK UP GAME", "ノズキングダムのミニゲーム紹介", nc_t);
	if (nc_num == 0) return;

	k = nc_t / NZ_CARD_FRAMES;
	if (k >= nc_num) k = nc_num - 1;
	ct = nc_t - k * NZ_CARD_FRAMES;
	gi = nc_pick[k];
	g = &nz_games[gi];

	// 入り(右から)と抜け(左へ)。最後の 1 枚は抜けずに暗転で消える
	in = (ct < NZ_CARD_IN) ? ct / (float)NZ_CARD_IN : 1.0f;
	ein = 1.0f - (1.0f - in) * (1.0f - in) * (1.0f - in);
	ofs = (int)(1500 * (1.0f - ein));
	rest = NZ_CARD_FRAMES - ct;
	alpha = 255;
	if (k < nc_num - 1 && rest < NZ_CARD_OUT) {
		float o = 1.0f - rest / (float)NZ_CARD_OUT;
		ofs = -(int)(1500 * o * o);
		alpha = 255 * rest / NZ_CARD_OUT;
	}

	// サムネイル(クリップの今のコマ)
	x1 = NZ_THUMB_X + ofs; y1 = NZ_THUMB_Y; x2 = x1 + NZ_THUMB_W; y2 = y1 + NZ_THUMB_H;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha * 140 / 255);
	DrawBox(x1 + 14, y1 + 16, x2 + 14, y2 + 16, GetColor(0, 0, 0), TRUE);   // 影
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
	DrawBox(x1, y1, x2, y2, GetColor(0, 0, 0), TRUE);
	if (nc_clip[k][0] >= 0) {
		int f = (ct * NZ_CLIP_FPS / 60) % NZ_CLIP_FRAMES;
		SetDrawMode(DX_DRAWMODE_BILINEAR);   // 小さいコマを引き伸ばすので、なめらかに
		DrawExtendGraph(x1, y1, x2, y2, nc_clip[k][f], TRUE);
		SetDrawMode(DX_DRAWMODE_NEAREST);
	}
	for (w = 0; w < 4; w++) DrawBox(x1 - w, y1 - w, x2 + w, y2 + w, GetColor(230, 190, 80), FALSE);   // 金の枠(2 重)
	DrawBox(x1 - 9, y1 - 9, x2 + 9, y2 + 9, GetColor(120, 96, 40), FALSE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	if (ct >= NZ_CARD_IN - 4 && ct < NZ_CARD_IN + 14) {   // 滑り込んできた瞬間:白い光がさっと払う
		SetDrawBlendMode(DX_BLENDMODE_ADD, 180 - (ct - (NZ_CARD_IN - 4)) * 10);
		DrawBox(x1, y1, x2, y2, GetColor(255, 255, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	if (ct < NZ_CARD_IN) {   // 入ってくる間は、後ろに光の尾を引く
		int s;
		SetDrawBright(255, 230, 160);
		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(200 * (1.0f - in)));
		for (s = 0; s < 4; s++) DrawRotaGraph3(x2 + 300, y1 + NZ_THUMB_H * (s + 1) / 5, 32, 32, 18.0, 0.4, 0.0, nz_img[9], TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		SetDrawBright(255, 255, 255);
	}

	// 右側の文字:少しずつ遅れて、ふわっと出る
	ix = NZ_INFO_X + ofs / 2;
	y = NZ_THUMB_Y;
	it = ct - NZ_CARD_IN;
	if (it >= 0) {   // ジャンルと形式の札
		int tagH = 52;
		GameGenre_DrawTags(ix, y + tagH / 2, tagH, g->genre, g->format, (it < 12) ? alpha * it / 12 : alpha);
	}
	y += 52 + 30;
	if (it >= 6) {   // 名前(長ければ折り返す)と、下の金の線
		int ny;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, (it < 18) ? alpha * (it - 6) / 12 : alpha);
		ny = Noz_DrawWrapped(ix, y, NZ_INFO_W, 66, g->name, GetColor(255, 255, 255), Fonts[5]);
		DrawBox(ix, ny + 8, ix + NZ_INFO_W, ny + 11, GetColor(230, 190, 80), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		y = ny + 40;
	}
	if (it >= 14) {   // 説明(gamelist.txt の help)
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, (it < 26) ? alpha * (it - 14) / 12 : alpha);
		Noz_DrawWrapped(ix, y, NZ_INFO_W, 46, g->help, GetColor(215, 222, 240), Fonts[6]);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
	if (it >= 22) {   // 遊べる人数(絵の下端にそろえた板)
		int a = (it < 34) ? alpha * (it - 22) / 12 : alpha, ph = 70, py = NZ_THUMB_Y + NZ_THUMB_H - ph, fh = GetFontSizeToHandle(Fonts[6]), yy;
		char buf[64];
		for (yy = 0; yy < ph; yy++) {   // 縦のグラデーションの紺の板
			int c = 40 - yy * 24 / ph;
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, a * 220 / 255);
			DrawBox(ix, py + yy, ix + NZ_INFO_W, py + yy + 1, GetColor(c / 2, c / 2 + 4, c + 40), TRUE);
		}
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
		DrawBox(ix, py, ix + 8, py + ph, GetColor(230, 190, 80), TRUE);
		_snprintf(buf, sizeof(buf), "プレイ人数　%d～4人", (g->players > 1) ? g->players : 1);
		buf[sizeof(buf) - 1] = '\0';
		DrawStringToHandle(ix + 40, py + (ph - fh) / 2, buf, GetColor(255, 255, 255), Fonts[6]);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}

	// 何枚目か(絵の下に点。今の 1 枚は金)
	{
		int r = 9, gap = 34, i, dcx = NZ_THUMB_X + NZ_THUMB_W / 2 - (nc_num - 1) * gap / 2, dy = NZ_THUMB_Y + NZ_THUMB_H + 50;
		for (i = 0; i < nc_num; i++) DrawCircleAA((float)(dcx + i * gap), (float)dy, (float)((i == k) ? r + 2 : r), 24, (i == k) ? GetColor(255, 214, 70) : GetColor(90, 96, 130), TRUE);
	}
}

static void NozCards_End(void) {
	resMgr.UnloadGroup(RESOURCE_GROUP_NOZ_CARDS);   // クリップは大きいので、紹介が終わったら手放す
	nc_num = 0;
}

const DemoDef demo_noz_cards = { "noz_cards", NozCards_Begin, NozCards_Update, NozCards_Draw, NozCards_End, 1, DEMO_BGM_NOZ };
