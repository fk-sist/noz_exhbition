#include <math.h>
#include "DxLib.h"
#include "env_value.h"
#include "ui_ease.h"
#include "effect_open.h"

// =====================================================
// クイズの開幕の光の演出(2026 改修)。使い方は effect_open.h を参照
//   線は effect_push / effect_judge と同じく「太くて薄い線(にじみ)」の上に「細くて明るい線(芯)」を重ねて光らせる。全部、加算合成
// =====================================================

#define OPEN_PI 3.14159265f
#define OPEN_FX_LAND  1
#define OPEN_FX_BURST 2
#define OPEN_FX_RING  3

typedef struct {
	int   type;            // 0 で空き
	int   frame;
	float x1, y1, x2, y2;  // 着地:硝子の範囲
	float cx, cy, w, r;    // 爆発・輪:中心、横の線の長さ、輪の大きさ
} OpenFx;

typedef struct {
	float x, y, vx, vy;
	int   life, life0;
	int   col;             // 0 白 1 水色 2 金
	float grav;
} OpenSpark;

static OpenFx    open_fx[OPEN_FX_MAX];
static OpenSpark open_spark[OPEN_SPARKS];

static unsigned int Open_Col(int c) {
	if (c == 1) return GetColor(90, 220, 255);
	if (c == 2) return GetColor(255, 210, 110);
	return GetColor(255, 255, 255);
}

// 関数の役割:火花を 1 つ出す(空きが無ければ出さない)
static void Open_Spark(float x, float y, float vx, float vy, int life, int col, float grav) {
	int i;
	for (i = 0; i < OPEN_SPARKS; i++) if (open_spark[i].life <= 0) {
		open_spark[i].x = x; open_spark[i].y = y; open_spark[i].vx = vx; open_spark[i].vy = vy;
		open_spark[i].life = open_spark[i].life0 = life; open_spark[i].col = col; open_spark[i].grav = grav;
		return;
	}
}

static OpenFx* Open_New(int type) {
	int i;
	for (i = 0; i < OPEN_FX_MAX; i++) if (open_fx[i].type == 0) { open_fx[i].type = type; open_fx[i].frame = 0; return &open_fx[i]; }
	return 0;
}

void Effect_Open_Land(float x1, float y1, float x2, float y2) {
	OpenFx* f = Open_New(OPEN_FX_LAND);
	int i;
	if (!f) return;
	f->x1 = x1; f->y1 = y1; f->x2 = x2; f->y2 = y2;
	// 下の縁から火の粉(左右へ低く散って、少し落ちる)
	for (i = 0; i < 28; i++) {
		float x = x1 + (x2 - x1) * (float)GetRand(1000) / 1000.0f;
		float dir = (x < (x1 + x2) / 2) ? -1.0f : 1.0f;
		Open_Spark(x, y2 + 10.0f, dir * (2.0f + (float)GetRand(60) / 10.0f), -(1.0f + (float)GetRand(40) / 10.0f), 22 + GetRand(14), (i % 3 == 0) ? 0 : 1, 0.25f);
	}
}

void Effect_Open_Burst(float cx, float cy, float width) {
	OpenFx* f = Open_New(OPEN_FX_BURST);
	int i;
	if (!f) return;
	f->cx = cx; f->cy = cy; f->w = width;
	for (i = 0; i < 24; i++) {   // 全方向へ火花
		float a = (float)GetRand(359) * OPEN_PI / 180.0f, sp = 6.0f + (float)GetRand(90) / 10.0f;
		Open_Spark(cx, cy, cosf(a) * sp * 1.6f, sinf(a) * sp * 0.7f, 20 + GetRand(14), (i % 4 == 0) ? 2 : (i % 2 ? 1 : 0), 0.0f);
	}
}

void Effect_Open_Ring(float cx, float cy, float r) {
	OpenFx* f = Open_New(OPEN_FX_RING);
	int i;
	if (!f) return;
	f->cx = cx; f->cy = cy; f->r = r;
	for (i = 0; i < 10; i++) {
		float a = (float)GetRand(359) * OPEN_PI / 180.0f, sp = 4.0f + (float)GetRand(40) / 10.0f;
		Open_Spark(cx + cosf(a) * r * 0.5f, cy + sinf(a) * r * 0.5f, cosf(a) * sp, sinf(a) * sp, 14 + GetRand(8), (i % 2) ? 1 : 2, 0.0f);
	}
}

void Effect_Open_Stop(void) {
	int i;
	for (i = 0; i < OPEN_FX_MAX; i++) open_fx[i].type = 0;
	for (i = 0; i < OPEN_SPARKS; i++) open_spark[i].life = 0;
}

void Effect_Open_Update(void) {
	int i;
	for (i = 0; i < OPEN_FX_MAX; i++) {
		int dur;
		if (open_fx[i].type == 0) continue;
		open_fx[i].frame++;
		dur = (open_fx[i].type == OPEN_FX_LAND) ? OPEN_LAND_FRAMES : (open_fx[i].type == OPEN_FX_BURST) ? OPEN_BURST_FRAMES : OPEN_RING_FRAMES;
		if (open_fx[i].frame >= dur) open_fx[i].type = 0;
	}
	for (i = 0; i < OPEN_SPARKS; i++) {
		OpenSpark* p = &open_spark[i];
		if (p->life <= 0) continue;
		p->x += p->vx; p->y += p->vy;
		p->vx *= 0.92f; p->vy = p->vy * 0.92f + p->grav;
		p->life--;
	}
}

// 関数の役割:光る線(にじみ + 芯)
static void Open_Line(float x1, float y1, float x2, float y2, unsigned int glow, unsigned int core, int a, float gw, float cw) {
	if (a <= 0) return;
	SetDrawBlendMode(DX_BLENDMODE_ADD, a / 3); DrawLineAA(x1, y1, x2, y2, glow, gw);
	SetDrawBlendMode(DX_BLENDMODE_ADD, a);     DrawLineAA(x1, y1, x2, y2, core, cw);
}

// 関数の役割:枠の内側をたどる道の上の点(t = 0 が下の真ん中、1 が上の真ん中。side は -1 で左回り、1 で右回り)
static void Open_TracePoint(const OpenFx* f, float t, int side, float* x, float* y) {
	float w = (f->x2 - f->x1) / 2, h = f->y2 - f->y1, total = w + h + w, d = t * total;
	float cx = (f->x1 + f->x2) / 2;
	if (d < w) { *x = cx + side * d; *y = f->y2; return; }
	d -= w;
	if (d < h) { *x = cx + side * w; *y = f->y2 - d; return; }
	d -= h;
	*x = cx + side * (w - d); *y = f->y1;
}

static void Open_DrawLand(const OpenFx* f) {
	unsigned int white = GetColor(255, 255, 255), cyan = GetColor(90, 220, 255);
	int k, side;
	// 硝子に走る白い光(着地の瞬間が一番明るく、すぐ消える)
	if (f->frame < 12) {
		int a = Ease_Move(170, 0, f->frame, 0, 12, EASE_OUT_CUBIC);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a / 2);
		DrawBox((int)f->x1, (int)f->y1, (int)f->x2, (int)f->y2, GetColor(120, 200, 255), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a);
		DrawBox((int)f->x1, (int)(f->y2 - 6), (int)f->x2, (int)f->y2, white, TRUE);   // 下の縁が一番光る(ぶつかった所)
	}
	// 枠の内側を、下の真ん中から上の真ん中へ駆け上がる 2 つの光(尾を引く)
	if (f->frame < OPEN_TRACE_FRAMES + OPEN_TRACE_FADE) {
		float head = Ease_MoveF(0.0f, 1.0f, f->frame, 0, OPEN_TRACE_FRAMES, EASE_IN_OUT_CUBIC);
		int fade = (f->frame < OPEN_TRACE_FRAMES) ? 255 : Ease_Move(255, 0, f->frame, OPEN_TRACE_FRAMES, OPEN_TRACE_FADE, EASE_LINEAR);
		for (side = -1; side <= 1; side += 2) {
			float px = 0, py = 0, hx = 0, hy = 0;
			for (k = 0; k <= 12; k++) {
				float t = head - OPEN_TRACE_TAIL * (1.0f - k / 12.0f), x, y;
				if (t < 0) t = 0;
				Open_TracePoint(f, t, side, &x, &y);
				if (k > 0) Open_Line(px, py, x, y, cyan, white, fade * k / 12 * 3 / 4, 12.0f, 2.5f);
				px = x; py = y; hx = x; hy = y;
			}
			SetDrawBlendMode(DX_BLENDMODE_ADD, fade);
			DrawCircleAA(hx, hy, 5.0f, 16, white, TRUE);
			SetDrawBlendMode(DX_BLENDMODE_ADD, fade / 3);
			DrawCircleAA(hx, hy, 16.0f, 20, cyan, TRUE);
		}
	}
}

static void Open_DrawBurst(const OpenFx* f) {
	unsigned int white = GetColor(255, 255, 255), cyan = GetColor(90, 220, 255), gold = GetColor(255, 210, 110);
	int i, fr = f->frame;
	float T = (float)fr / OPEN_BURST_FRAMES;
	// 白い芯(最初が一番明るい)
	if (fr < 10) {
		int a = Ease_Move(255, 0, fr, 0, 10, EASE_OUT_CUBIC);
		float r = Ease_MoveF(40.0f, 130.0f, fr, 0, 10, EASE_OUT_CUBIC);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a / 4); DrawCircleAA(f->cx, f->cy, r * 1.6f, 40, cyan, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a / 2); DrawCircleAA(f->cx, f->cy, r, 40, white, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a);     DrawCircleAA(f->cx, f->cy, r * 0.45f, 32, white, TRUE);
	}
	// 横に伸びる光の線(黒板いっぱいに、細くなりながら消える)
	{
		float hw = Ease_MoveF(20.0f, f->w / 2, fr, 0, 14, EASE_OUT_CUBIC);
		float th = Ease_MoveF(14.0f, 2.0f, fr, 0, OPEN_BURST_FRAMES, EASE_OUT_CUBIC);
		int a = (int)(220 * (1.0f - T));
		SetDrawBlendMode(DX_BLENDMODE_ADD, a / 3); DrawBoxAA(f->cx - hw, f->cy - th * 2.5f, f->cx + hw, f->cy + th * 2.5f, cyan, TRUE);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a);     DrawBoxAA(f->cx - hw, f->cy - th / 2, f->cx + hw, f->cy + th / 2, white, TRUE);
	}
	// 広がる輪 2 本(2 本目は少し遅れて)
	for (i = 0; i < 2; i++) {
		int f0 = i * 6;
		float r; int a;
		if (fr < f0) continue;
		r = Ease_MoveF(30.0f, 300.0f + 120.0f * i, fr, f0, 24, EASE_OUT_CUBIC);
		a = Ease_Move(230, 0, fr, f0, 24, EASE_OUT_CUBIC);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a / 3); DrawCircleAA(f->cx, f->cy, r, 64, i ? gold : cyan, FALSE, 20.0f);
		SetDrawBlendMode(DX_BLENDMODE_ADD, a);     DrawCircleAA(f->cx, f->cy, r, 64, white, FALSE, 2.5f);
	}
	// 回る光の筋(外へ伸びてから消える)
	for (i = 0; i < OPEN_BURST_RAYS; i++) {
		float ang = (fr * 1.5f + 360.0f * i / OPEN_BURST_RAYS) * OPEN_PI / 180.0f;
		float r0 = Ease_MoveF(20.0f, 160.0f, fr, 0, OPEN_BURST_FRAMES, EASE_OUT_CUBIC);
		float len = (i % 2 ? 90.0f : 150.0f) * sinf(T * OPEN_PI);
		int a = (int)(200 * (1.0f - T));
		float dx = cosf(ang), dy = sinf(ang) * 0.55f;   // 横長に(黒板の形に合わせる)
		Open_Line(f->cx + dx * r0, f->cy + dy * r0, f->cx + dx * (r0 + len), f->cy + dy * (r0 + len), (i % 3 == 0) ? gold : cyan, white, a, 8.0f, 2.0f);
	}
}

static void Open_DrawRing(const OpenFx* f) {
	float r = Ease_MoveF(f->r * 0.3f, f->r * 1.6f, f->frame, 0, OPEN_RING_FRAMES, EASE_OUT_CUBIC);
	int a = Ease_Move(220, 0, f->frame, 0, OPEN_RING_FRAMES, EASE_OUT_CUBIC);
	SetDrawBlendMode(DX_BLENDMODE_ADD, a / 3); DrawCircleAA(f->cx, f->cy, r, 48, GetColor(255, 210, 110), FALSE, 14.0f);
	SetDrawBlendMode(DX_BLENDMODE_ADD, a);     DrawCircleAA(f->cx, f->cy, r, 48, GetColor(255, 255, 255), FALSE, 2.0f);
}

void Effect_Open_GaugeHead(float x, float y1, float y2) {
	unsigned int white = GetColor(255, 255, 255), cyan = GetColor(90, 220, 255);
	float cy = (y1 + y2) / 2;
	SetDrawBlendMode(DX_BLENDMODE_ADD, 90);  DrawCircleAA(x, cy, (y2 - y1) * 1.6f, 24, cyan, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_ADD, 230); DrawLineAA(x, y1 - 4.0f, x, y2 + 4.0f, white, 3.0f);
	SetDrawBlendMode(DX_BLENDMODE_ADD, 200); DrawCircleAA(x, cy, 4.0f, 12, white, TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	// 先端から後ろ(右)へ火花をこぼす
	if (GetRand(1) == 0) Open_Spark(x, cy + (float)(GetRand((int)(y2 - y1)) - (int)((y2 - y1) / 2)), 2.0f + (float)GetRand(30) / 10.0f, -(float)GetRand(20) / 10.0f, 12 + GetRand(8), GetRand(1) ? 0 : 1, 0.15f);
}

void Effect_Open_Draw(void) {
	int i;
	for (i = 0; i < OPEN_FX_MAX; i++) {
		if (open_fx[i].type == OPEN_FX_LAND) Open_DrawLand(&open_fx[i]);
		else if (open_fx[i].type == OPEN_FX_BURST) Open_DrawBurst(&open_fx[i]);
		else if (open_fx[i].type == OPEN_FX_RING) Open_DrawRing(&open_fx[i]);
	}
	for (i = 0; i < OPEN_SPARKS; i++) {
		const OpenSpark* p = &open_spark[i];
		float k;
		if (p->life <= 0) continue;
		k = (float)p->life / p->life0;
		SetDrawBlendMode(DX_BLENDMODE_ADD, (int)(220 * k));
		DrawLineAA(p->x - p->vx * 2.0f, p->y - p->vy * 2.0f, p->x, p->y, Open_Col(p->col), 2.0f);
		DrawCircleAA(p->x, p->y, 1.2f + 2.0f * k, 10, GetColor(255, 255, 255), TRUE);
	}
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}
