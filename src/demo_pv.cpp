#define _CRT_SECURE_NO_WARNINGS
#include "DxLib.h"
#include <stdio.h>
#include <string.h>

#ifndef ENV_LOADED_H
#include "env_value.h"
#endif
#include "demo.h"

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
