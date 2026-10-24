#define _CRT_SECURE_NO_WARNINGS
#include "DxLib.h"
#include <stdio.h>
#include <string.h>

#ifndef ENV_LOADED_H
#include "env_value.h"
#endif
#include "resource_manager.h"
#include "demo.h"
#include "guide_text.h"

// =====================================================
// デモの巡回(2026-10-24)。説明は demo.h
//   暗転:今のデモが終わったら、DIR_FADE_FRAMES の半分で暗くなりきり、そこで次のデモに替えて、残りで明ける
//   下の帯:案内の文(data/guide.txt の ticker:)を右から左へ流し続ける。デモの ticker が 0 の間は出さない
//   BGM:デモの bgm を見て鳴らす。曲が替わる・無くなる時は、小さくしてから止める
// =====================================================

#define DIR_LIST_FILE     "./data/demos.txt"
#define DIR_LIST_MAX      64
#define DIR_FADE_FRAMES   36       // 暗転の長さ(半分で暗くなりきる)
#define DIR_TICKER_H      64       // 下の帯の高さ
#define DIR_TICKER_SPEED  3        // 帯の文が流れる速さ(px/フレーム)
#define DIR_BGM_VOLUME    200      // BGM の音量(0～255)
#define DIR_BGM_FADE      45       // 曲を止める時に小さくしていく長さ(フレーム)
#define DIR_BGM_NOZ_FILE  "./noz/audio/sys_menu.ogg"

#define RESOURCE_GROUP_DIRECTOR 1  // (resource_manager.h の番号。巡回の共用の物。終了まで持つ)

// デモの一覧(data/demos.txt に書く名前と結ぶ。デモを足したらここにも足す)
static const DemoDef* demo_table[] = {
	&demo_noz_guide, &demo_noz_cards, &demo_qma, &demo_circle, &demo_pv,
};
#define DEMO_TABLE_NUM ((int)(sizeof(demo_table) / sizeof(demo_table[0])))

typedef struct { const DemoDef* def; int arg; } DirItem;
static DirItem dir_list[DIR_LIST_MAX];
static int dir_num = 0;
static int dir_cur = -1;          // 今のデモ(dir_list の番号。-1 なら何も流していない)
static int dir_fade = -1;         // 暗転のフレーム(-1 なら暗転していない)
static int dir_ticker_x = 0;
static int dir_bgm_noz = -1;      // BGM のハンドル
static float dir_bgm_vol = 0.0f;  // 今の音量の度合い(0.0～1.0)
extern ResourceManager resMgr;

GuideText guide;                  // 案内の文(data/guide.txt)

// 関数の役割:テキストファイルの 1 行を読む。行末の改行と、# から行末を消し、前後の空白も消す。戻り値:読めたら 1(ファイルの終わりで 0)
int Demo_ReadLine(FILE* fp, char* buf, int cap) {
	char* p;
	int n;
	if (fgets(buf, cap, fp) == NULL) return 0;
	for (p = buf; *p != '\0'; p++) {
		if ((unsigned char)*p >= 0x81 && ((unsigned char)*p <= 0x9F || (unsigned char)*p >= 0xE0) && p[1] != '\0') { p++; continue; }   // CP932 の 2 バイト文字は飛ばす(2 バイト目に # が来ても切らない)
		if (*p == '#') { *p = '\0'; break; }
	}
	n = (int)strlen(buf);
	while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ' || buf[n - 1] == '\t')) buf[--n] = '\0';
	p = buf;
	while (*p == ' ' || *p == '\t') p++;
	if (p != buf) memmove(buf, p, strlen(p) + 1);
	return 1;
}

// 関数の役割:案内の文(data/guide.txt。key: 値 の行)を読む。無い所は、はじめから入れてある文のまま
static void Director_LoadGuide(void) {
	FILE* fp;
	char line[512];
	strcpy(guide.place, "306講義室");
	strcpy(guide.main, "306講義室で 遊べます！");
	guide.sub_num = 0;
	strcpy(guide.ticker, "NOZ KINGDOM　★　306講義室で遊べます　★　");
	fp = fopen("./data/guide.txt", "r");
	if (fp == NULL) return;
	while (Demo_ReadLine(fp, line, sizeof(line))) {
		char* v = strchr(line, ':');
		if (v == NULL) continue;
		*v++ = '\0';
		while (*v == ' ') v++;
		if (strcmp(line, "place") == 0) strncpy(guide.place, v, GUIDE_TEXT_LEN - 1);
		else if (strcmp(line, "main") == 0) strncpy(guide.main, v, GUIDE_TEXT_LEN - 1);
		else if (strcmp(line, "sub") == 0 && guide.sub_num < GUIDE_SUB_MAX) { strncpy(guide.sub[guide.sub_num], v, GUIDE_TEXT_LEN - 1); guide.sub[guide.sub_num][GUIDE_TEXT_LEN - 1] = '\0'; guide.sub_num++; }
		else if (strcmp(line, "ticker") == 0) strncpy(guide.ticker, v, GUIDE_TEXT_LEN - 1);
	}
	guide.place[GUIDE_TEXT_LEN - 1] = guide.main[GUIDE_TEXT_LEN - 1] = guide.ticker[GUIDE_TEXT_LEN - 1] = '\0';
	fclose(fp);
}

// 関数の役割:デモの一覧(data/demos.txt)を読む。読めなかった時・1 つも無い時は、全部のデモを 1 回ずつ
static void Director_LoadList(void) {
	FILE* fp = fopen(DIR_LIST_FILE, "r");
	char line[256], name[64];
	int arg, k;
	dir_num = 0;
	if (fp != NULL) {
		while (Demo_ReadLine(fp, line, sizeof(line)) && dir_num < DIR_LIST_MAX) {
			if (line[0] == '\0') continue;
			arg = 0;
			name[0] = '\0';
			if (sscanf(line, "%63s %d", name, &arg) < 1) continue;
			for (k = 0; k < DEMO_TABLE_NUM; k++) {
				if (strcmp(name, demo_table[k]->name) == 0) { dir_list[dir_num].def = demo_table[k]; dir_list[dir_num].arg = arg; dir_num++; break; }
			}
		}
		fclose(fp);
	}
	if (dir_num == 0) {
		for (k = 0; k < DEMO_TABLE_NUM; k++) { dir_list[dir_num].def = demo_table[k]; dir_list[dir_num].arg = 0; dir_num++; }
	}
}

// 関数の役割:次のデモを始める(Begin が 0 を返したデモは飛ばす。一周しても始められなければ、何も流さない)
static void Director_Next(void) {
	int tries;
	for (tries = 0; tries < dir_num; tries++) {
		dir_cur = (dir_cur + 1) % dir_num;
		if (dir_list[dir_cur].def->Begin(dir_list[dir_cur].arg)) return;
	}
	dir_cur = -1;
}

void Director_Init(void) {
	ResourceGroup* g = resMgr.CreateGroup(RESOURCE_GROUP_DIRECTOR);
	Director_LoadGuide();
	Director_LoadList();
	dir_bgm_noz = g->LoadSound(DIR_BGM_NOZ_FILE);
	dir_cur = -1;
	dir_fade = -1;
	Director_Next();
}

// 関数の役割:BGM(今のデモの bgm を鳴らす。暗転の後半で替わる)
static void Director_UpdateBgm(void) {
	int want = (dir_cur >= 0) ? dir_list[dir_cur].def->bgm : DEMO_BGM_NONE;
	if (dir_bgm_noz < 0) return;
	if (want == DEMO_BGM_NOZ) {
		if (CheckSoundMem(dir_bgm_noz) == 0) PlaySoundMem(dir_bgm_noz, DX_PLAYTYPE_LOOP, TRUE);   // 止まっていたら頭から
		dir_bgm_vol = 1.0f;
	}
	else if (dir_bgm_vol > 0.0f) {
		dir_bgm_vol -= 1.0f / DIR_BGM_FADE;
		if (dir_bgm_vol <= 0.0f) { dir_bgm_vol = 0.0f; StopSoundMem(dir_bgm_noz); }
	}
	if (dir_bgm_vol > 0.0f) ChangeVolumeSoundMem((int)(DIR_BGM_VOLUME * dir_bgm_vol), dir_bgm_noz);
}

void Director_Update(void) {
	if (dir_fade >= 0) {
		// 暗転中:真ん中で替える
		if (dir_fade == DIR_FADE_FRAMES / 2) {
			if (dir_cur >= 0) dir_list[dir_cur].def->End();
			Director_Next();
		}
		if (++dir_fade > DIR_FADE_FRAMES) dir_fade = -1;
	}
	if (dir_cur >= 0 && (dir_fade < 0 || dir_fade > DIR_FADE_FRAMES / 2)) {   // 暗くなっていく間は止めておく(終わった絵のまま暗くなる)
		if (dir_list[dir_cur].def->Update() == 1 && dir_fade < 0) dir_fade = 0;
	}
	if (dir_cur < 0 && dir_fade < 0) Director_Next();   // 何も始められなかった時は、毎フレーム試す
	Director_UpdateBgm();
	dir_ticker_x -= DIR_TICKER_SPEED;
}

// 関数の役割:下の帯(案内の文を右から左へ流す)
static void Director_DrawTicker(void) {
	int y1 = SCREEN_HEIGHT - DIR_TICKER_H, tw, fh, ty, x;
	const char* s = guide.ticker;
	SetDrawBlendMode(DX_BLENDMODE_ALPHA, 210);
	DrawBox(0, y1, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(6, 8, 22), TRUE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	DrawBox(0, y1, SCREEN_WIDTH, y1 + 3, GetColor(230, 190, 80), TRUE);
	tw = GetDrawStringWidthToHandle(s, (int)strlen(s), Fonts[6]);
	if (tw < 1) return;
	fh = GetFontSizeToHandle(Fonts[6]);
	ty = y1 + 3 + (DIR_TICKER_H - 3 - fh) / 2;
	while (dir_ticker_x <= -tw) dir_ticker_x += tw;
	for (x = dir_ticker_x; x < SCREEN_WIDTH; x += tw) DrawStringToHandle(x, ty, s, GetColor(255, 230, 150), Fonts[6]);
}

void Director_Draw(void) {
	if (dir_cur >= 0) {
		dir_list[dir_cur].def->Draw();
		if (dir_list[dir_cur].def->ticker) Director_DrawTicker();
	}
	if (dir_fade >= 0) {
		int half = DIR_FADE_FRAMES / 2;
		int a = (dir_fade < half) ? 255 * dir_fade / half : 255 * (DIR_FADE_FRAMES - dir_fade) / half;
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, a);
		DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(0, 0, 0), TRUE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}

void Director_End(void) {
	if (dir_cur >= 0) dir_list[dir_cur].def->End();
	dir_cur = -1;
	if (dir_bgm_noz >= 0) StopSoundMem(dir_bgm_noz);
}
