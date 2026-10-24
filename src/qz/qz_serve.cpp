#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qz_serve.h"

// =====================================================
// クイズの芯:出題の準備。説明は qz_serve.h
//   並べ方は、当時の servequiz(を新しい画面に写した quiz_scene.cpp の Qs_Prepare～)と同じ。
//   乱数の取り方(GetRand(100000) % n など)も、当時の式のまま。当時のバグを直した所は、関数の頭に「直した所」と書いた。
// =====================================================

// --- 調整値 ---
#define QZ_REEL_START_RANDOM 1   // 1 で、スロットの最初の真ん中の字をランダムに(quiz_scene.cpp の QS_REEL_SHUFFLE と同じ)。0 で当時と同じ(毎回 2 番目)
//#define QZ_GROUP_SHOW_MAX    6   // 前の形:グループ分けで出す項目の数の上限(当時と同じ)
#define QZ_GROUP_SHOW_MAX    5   // グループ分けで出す項目の数の上限(2026-10-15。fumiya:最大 5。6 つは操作に慣れない人が答えきれない)

static int (*qz_rand)(int max) = NULL;
static QzRng* qz_seeded = NULL;   // Qz_ServeSeed の間だけ、Qz_Rand はこの種の乱数(2026-10-20)

void Qz_SetRand(int (*rnd)(int max)) { qz_rand = rnd; }

// 0～max(当時の GetRand と同じ。max が 0 以下なら 0)
int Qz_Rand(int max) {
	if (max <= 0) return 0;
	if (qz_seeded != NULL) return QzRng_Next(qz_seeded, max);
	if (qz_rand != NULL) return qz_rand(max);
	return rand() % (max + 1);
}

// ---- 種から決まる乱数(2026-10-20)。xorshift32(どの PC でも同じ数の並びになる。unsigned int は 32 ビット) ----
#define QZ_RNG_ZERO_SEED  0x2545F491u   // 種が 0 の時の代わり(xorshift は 0 から動かないので)
#define QZ_RNG_WARMUP     8             // 種の近い値どうしの並びを散らすため、始めに空回しする回数

static unsigned int QzRng_Step(QzRng* r) {
	unsigned int x = r->x;
	x ^= x << 13; x ^= x >> 17; x ^= x << 5;
	r->x = x;
	return x;
}

void QzRng_Init(QzRng* r, unsigned int seed) {
	int i;
	r->x = (seed != 0) ? seed : QZ_RNG_ZERO_SEED;
	for (i = 0; i < QZ_RNG_WARMUP; i++) QzRng_Step(r);
}

int QzRng_Next(QzRng* r, int max) {
	if (max <= 0) return 0;
	return (int)(QzRng_Step(r) % (unsigned int)(max + 1));
}

int Qz_ServeSeed(const QzQuestion* q, QzServe* s, unsigned int seed) {
	QzRng r;
	int ok;
	QzRng_Init(&r, seed);
	qz_seeded = &r;
	ok = Qz_Serve(q, s);
	qz_seeded = NULL;
	return ok;
}

unsigned int Qz_NewSeed(void) {
	unsigned int hi = (unsigned int)Qz_Rand(0x7FFF), lo = (unsigned int)Qz_Rand(0x7FFF), mid = (unsigned int)Qz_Rand(0x3);   // GetRand の max が小さくて済むように 3 つに分けて引く
	return (hi << 17) | (mid << 15) | lo;
}

static void Qz_SwapPtr(const char** a, const char** b) { const char* t = *a; *a = *b; *b = t; }
static void Qz_SwapInt(int* a, int* b) { int t = *a; *a = *b; *b = t; }
static void Qz_SwapChar(char* a, char* b) { char t[QZ_CHAR_LEN]; memcpy(t, a, QZ_CHAR_LEN); memcpy(a, b, QZ_CHAR_LEN); memcpy(b, t, QZ_CHAR_LEN); }

int Qz_HasBunki(const QzQuestion* q) {
	int i;
	// 当時:@ があって、列 6 が 0 の時だけ。列 6 は、問題の絵(四択・並べ替え・タイピングなど)、画像 2 択の絵、ビジュアルの崩す物(いつも 0 でない)
	if (q->image != NULL || q->kind == QZ_KIND_VISUAL) return 0;
	//for (i = 0; i < q->text_num; i++) if (strchr(q->text[i], '@') != NULL) return 1;   // 前の形(\@ も分岐とみていた)
	for (i = 0; i < q->text_num; i++) if (Qz_AtCount(q->text[i]) > 0) return 1;   // \@ は字としての @(2026-10-22)
	return 0;
}

// ============================
// ○×・四択・連想
// ============================

// 関数の役割:○× の準備。画像 2 択は、絵の左右どちらを A に置くかを GetRand(1) で決める(当時と同じ)
static void Qz_ServeOx(const QzQuestion* q, QzServe* s) {
	int i;
	s->item_num = 2;
	if (q->sub == QZ_SUB_IMAGE) {
		int left_is_a = (Qz_Rand(1) == 0);
		s->item[0] = "Ａ"; s->item[1] = "Ｂ";
		s->src[0] = left_is_a ? 0 : 1;
		s->src[1] = left_is_a ? 1 : 0;
		for (i = 0; i < 2; i++) s->truth[i] = (s->src[i] == q->answer_no) ? 1 : 0;   // 当時は「左半分が正解」で決め打ち(データは全部 A = 左)
	}
	else {
		s->item[0] = "○"; s->item[1] = "×";
		for (i = 0; i < 2; i++) { s->src[i] = i; s->truth[i] = (i == q->answer_no) ? 1 : 0; }
	}
}

// 関数の役割:四択・連想の準備。添字 1～x-1 を 30 回入れ替え、次に 0～3 を 30 回入れ替えて、先頭 4 つを出す(当時と同じ)
//   直した所:当時は、5 つ以上の時に「先頭の選択肢(添字 0)は必ず出る」ことで正解を出していた(データの正解はほぼ先頭)。
//            正解が先頭でない問題(連想に 1 問ある)は正解が出ないことがあったので、入れ替える前に正解を先頭へ動かす(正解が先頭の問題は当時と同じ並び)
static void Qz_ServeChoice(const QzQuestion* q, QzServe* s) {
	const char* item[QZ_ITEM_MAX];
	int src[QZ_ITEM_MAX];
	int i, a, b, x;

	if (q->sub == QZ_SUB_IMAGE) {   // 絵の選択肢:並びは変えない(当時と同じ)
		s->item_num = 4;
		for (i = 0; i < 4; i++) { s->item[i] = ""; s->src[i] = i; s->truth[i] = (i == q->answer_no) ? 1 : 0; }
		return;
	}
	x = q->item_num;
	for (i = 0; i < x; i++) { item[i] = q->item[i]; src[i] = i; }
	if (q->answer_no > 0 && q->answer_no < x && x > 4) { Qz_SwapPtr(&item[0], &item[q->answer_no]); Qz_SwapInt(&src[0], &src[q->answer_no]); }
	for (i = 0; i < 30 && x > 2; i++) { a = 1 + Qz_Rand(x - 2); b = 1 + Qz_Rand(x - 2); Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); }
	for (i = 0; i < 30 && x >= 4; i++) { a = Qz_Rand(3); b = Qz_Rand(3); Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); }
	s->item_num = (x > 4) ? 4 : x;
	for (i = 0; i < s->item_num; i++) { s->item[i] = item[i]; s->src[i] = src[i]; s->truth[i] = q->correct[src[i]] ? 1 : 0; }
}

// ============================
// 一問多答・順番当て・線結び
// ============================

// 関数の役割:一問多答の準備。出すのは 3 つ(show: 4 なら 4 つ)。
//   先頭(問題の 1 番目)は動かさず残りを入れ替え、50 回を過ぎたら、出す範囲に正解が 1 つでも入った所で止める。そのあと出す範囲の中で 30 回(当時と同じ)
//   絵は 4 分割の 4 枚を 30 回入れ替える
static void Qz_ServeMulti(const QzQuestion* q, QzServe* s) {
	const char* item[QZ_ITEM_MAX];
	int src[QZ_ITEM_MAX], tf[QZ_ITEM_MAX];
	int x, i, a, b, k, ntrue = Qz_CorrectCount(q);

	x = q->item_num;
	for (i = 0; i < x; i++) { item[i] = q->item[i]; src[i] = i; tf[i] = q->correct[i] ? 1 : 0; }
	if (q->sub == QZ_SUB_IMAGE) {
		s->item_num = 4;
		for (i = 0; i < 30; i++) { a = Qz_Rand(10000) % 4; b = Qz_Rand(10000) % 4; Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&tf[a], &tf[b]); }
	}
	else {
		s->item_num = (q->show == 4) ? 4 : 3;
		if (s->item_num > x) s->item_num = x;
		for (i = 0; x > 1; i++) {
			a = Qz_Rand(100000) % (x - 1) + 1; b = Qz_Rand(100000) % (x - 1) + 1;
			Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&tf[a], &tf[b]);
			if (i > 50) {
				int any = 0;
				for (k = 0; k < s->item_num; k++) if (tf[k]) any = 1;
				if (ntrue == 0 || any || i > 5000) break;
			}
		}
		for (i = 0; i < 30 && s->item_num > 0; i++) {
			a = Qz_Rand(10000) % s->item_num; b = Qz_Rand(10000) % s->item_num;
			Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&tf[a], &tf[b]);
		}
	}
	for (i = 0; i < s->item_num; i++) { s->item[i] = item[i]; s->src[i] = src[i]; s->truth[i] = tf[i]; }
}

// 関数の役割:順番当ての準備。問題の item は正しい順。出すのは 3 つ(show: 4 なら 4 つ)。絵は 4 枚
//   5 つ以上の時は、先頭を動かさずに 100 回入れ替えてから、先頭 4 つの順位を 1～4 に振り直す。
//   出す範囲で 100 回入れ替え、順位が 1～n にそろわなければ、問題の順に戻してやり直す(当時と同じ)
static void Qz_ServeOrder(const QzQuestion* q, QzServe* s) {
	const char* item[QZ_ITEM_MAX];
	int src[QZ_ITEM_MAX], ord[QZ_ITEM_MAX];
	int x, n, i, j, a, b;

	x = q->item_num;
	for (i = 0; i < x; i++) { item[i] = q->item[i]; src[i] = i; ord[i] = i + 1; }
	if (q->sub == QZ_SUB_IMAGE) {
		n = 4;
		for (i = 0; i < 100; i++) { a = Qz_Rand(10000) % 4; b = Qz_Rand(10000) % 4; Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&ord[a], &ord[b]); }
	}
	else {
		n = (q->show == 4) ? 4 : 3;
		if (n > x) n = x;
		if (x > 4) {
			int tmpo[4];
			for (i = 0; i < 100; i++) {
				a = Qz_Rand(100000) % (x - 1) + 1; b = Qz_Rand(100000) % (x - 1) + 1;
				Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&ord[a], &ord[b]);
			}
			for (i = 0; i < 4; i++) { int r = 1; for (j = 0; j < 4; j++) if (ord[i] > ord[j]) r++; tmpo[i] = r; }
			for (i = 0; i < 4; i++) ord[i] = tmpo[i];
		}
		while (n > 0) {
			int ok = 1, r;
			for (i = 0; i < 100; i++) {
				a = Qz_Rand(10000) % n; b = Qz_Rand(10000) % n;
				Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&ord[a], &ord[b]);
			}
			for (r = 1; r <= n; r++) { for (j = 0; j < n; j++) if (ord[j] == r) break; if (j == n) ok = 0; }
			if (ok) break;
			for (i = 0; i < x; i++) { item[i] = q->item[i]; src[i] = i; ord[i] = i + 1; }   // やり直し:問題の順に戻す(先頭 n 個の順位が 1～n になる)
		}
	}
	s->item_num = n;
	for (i = 0; i < n; i++) { s->item[i] = item[i]; s->src[i] = src[i]; s->truth[i] = ord[i]; }
}

// 関数の役割:線結びの準備。出すのは 3 組(show: 4 なら 4 組)。左が絵なら 3 組
//   組のまま 150 回入れ替えて先頭の組だけ使い、そのあと左だけ 150 回入れ替える(当時と同じ)
static void Qz_ServeLine(const QzQuestion* q, QzServe* s) {
	const char* l[QZ_ITEM_MAX];
	const char* r[QZ_ITEM_MAX];
	int h[QZ_ITEM_MAX], m[QZ_ITEM_MAX];
	int x, n, i, a, b;

	x = q->item_num;
	for (i = 0; i < x; i++) { l[i] = q->item[i]; r[i] = q->sub_item[i]; h[i] = i; }
	if (q->sub == QZ_SUB_IMAGE) {
		n = 3; if (n > x) n = x;
		for (i = 0; i < 150 && n > 0; i++) {   // 絵と右を、組のまま入れ替える(当時は絵の 3 枚の中で)
			a = Qz_Rand(10000) % n; b = Qz_Rand(10000) % n;
			Qz_SwapInt(&h[a], &h[b]); Qz_SwapPtr(&l[a], &l[b]); Qz_SwapPtr(&r[a], &r[b]);
		}
	}
	else {
		n = (q->show == 4) ? 4 : 3;
		if (n > x) n = x;
		for (i = 0; i < 150 && x > 0; i++) {   // 組を変えずに、両方を入れ替える
			a = Qz_Rand(10000) % x; b = Qz_Rand(10000) % x;
			Qz_SwapPtr(&l[a], &l[b]); Qz_SwapPtr(&r[a], &r[b]); Qz_SwapInt(&h[a], &h[b]);
		}
	}
	for (i = 0; i < n; i++) m[i] = i;
	for (i = 0; i < 150 && n > 0; i++) {   // 左だけを入れ替える(m[i] = 左 i の元の番目 = 正しい相手の右の番号)
		a = Qz_Rand(8000) % n; b = Qz_Rand(8000) % n;
		Qz_SwapPtr(&l[a], &l[b]); Qz_SwapInt(&h[a], &h[b]); Qz_SwapInt(&m[a], &m[b]);
	}
	s->item_num = n;
	for (i = 0; i < n; i++) { s->item[i] = l[i]; s->src[i] = h[i]; s->right[i] = r[i]; s->truth[i] = m[i]; }
}

// ============================
// 並べ替え・スロット・文字パネル
// ============================

// 関数の役割:並べ替えの準備。答えを 1 字ずつの札にして 150 回入れ替える(当時と同じ。最初から正解の並びになることは避けていない)
static void Qz_ServeNara(const QzQuestion* q, QzServe* s) {
	int i, a, b, n;
	n = Qz_CharCount(q->answer[0]);
	if (n > QZ_SERVE_MAX) n = QZ_SERVE_MAX;
	for (i = 0; i < n; i++) Qz_CharAt(q->answer[0], i, s->word[i], QZ_CHAR_LEN);
	for (i = 0; i < 150 && n > 0; i++) { a = Qz_Rand(100000) % n; b = Qz_Rand(100000) % n; Qz_SwapChar(s->word[a], s->word[b]); }
	s->word_num = n;
}

// 関数の役割:スロットの準備。リールの字は当時の環の next の順(書いた順の逆)。最初に真ん中にある字は QZ_REEL_START_RANDOM
static void Qz_ServeReel(const QzQuestion* q, QzServe* s) {
	int j, k;
	s->reel_num = (q->item_num > QZ_REEL_MAX) ? QZ_REEL_MAX : q->item_num;
	for (j = 0; j < s->reel_num; j++) {
		for (k = 0; k < 4; k++) Qz_CharAt(q->item[j], 3 - k, s->reel[j][k], QZ_CHAR_LEN);
#if QZ_REEL_START_RANDOM
		s->reel_start[j] = Qz_Rand(3);
#else
		s->reel_start[j] = 2;   // 当時:環の 2 番目
#endif
	}
}

// 関数の役割:文字パネルの準備。答えが 4 字以上なら 10 枚、3 字なら 8 枚を、問題のパネルの先頭から使い、100 回入れ替える(当時と同じ)
//   直した所:当時は字数を「バイト数 / 2」で数えていた(全角だけの答えなら同じ)
static void Qz_ServeMoji(const QzQuestion* q, QzServe* s) {
	const char* item[QZ_ITEM_MAX];
	int src[QZ_ITEM_MAX];
	int i, a, b, len, n;
	len = Qz_CharCount(q->answer[0]);
	if (len > 8) len = 8;
	n = (len >= 4) ? 10 : 8;
	if (n > q->item_num) n = q->item_num;
	if (n > QZ_SERVE_MAX) n = QZ_SERVE_MAX;
	for (i = 0; i < n; i++) { item[i] = q->item[i]; src[i] = i; }
	for (i = 0; i < 100 && n > 0; i++) { a = Qz_Rand(100000) % n; b = Qz_Rand(100000) % n; Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); }
	s->item_num = n;
	for (i = 0; i < n; i++) { s->item[i] = item[i]; s->src[i] = src[i]; }
}

// ============================
// グループ分け・早いもの勝ち
// ============================

// 関数の役割:グループ分けの準備。グループ A は先頭の項目のグループ。3 組の時は、前から見るか後ろから見るかをランダムに(当時と同じ)
//   出す数は、show: all なら全部(6 まで)、それ以外は項目の数に応じて 3～6 をランダムに。
//   組を崩さずに混ぜて、出す範囲に各グループが 1 つ以上入るまで(101 回目から確かめる。5000 回まで)。そのあと出す範囲の中で 30 回(当時と同じ)
static void Qz_ServeGroup(const QzQuestion* q, QzServe* s) {
	const char* item[QZ_ITEM_MAX];
	int src[QZ_ITEM_MAX], g[QZ_ITEM_MAX];
	int x, i, j, k, tries;
	const char* name[3] = { "", "", "" };

	x = q->item_num;
	s->group_num = (q->group_num == 3) ? 3 : 2;
	for (i = 0; i < x; i++) { item[i] = q->item[i]; src[i] = i; }
	if (s->group_num == 2) {   // A は先頭のグループ、それと違う名前が B(番号 2)
		name[0] = q->sub_item[0]; g[0] = 0;
		for (i = 1; i < x; i++) {
			if (strcmp(name[0], q->sub_item[i]) != 0) { name[2] = q->sub_item[i]; g[i] = 2; }
			else g[i] = 0;
		}
	}
	else {
		int fwd = (Qz_Rand(1) == 0), st = fwd ? 0 : x - 1, d = fwd ? 1 : -1;
		name[0] = q->sub_item[st]; g[st] = 0;
		for (i = st + d, j = 0; i >= 0 && i < x; i += d) {
			if (strcmp(name[0], q->sub_item[i]) == 0) { g[i] = 0; continue; }
			if (j == 0) { name[1] = q->sub_item[i]; g[i] = 1; j = 1; }
			else if (strcmp(name[1], q->sub_item[i]) != 0) { name[2] = q->sub_item[i]; g[i] = 2; j = 2; }
			else g[i] = 1;
		}
	}
	if (q->show == QZ_SHOW_ALL) s->item_num = (x >= QZ_GROUP_SHOW_MAX) ? QZ_GROUP_SHOW_MAX : x;
	else if (x <= 3) s->item_num = 3;
	else if (x <= 4) s->item_num = Qz_Rand(100) % 2 + 3;
	else if (x <= 5) s->item_num = Qz_Rand(100) % 3 + 3;
	//else s->item_num = Qz_Rand(100) % 4 + 3;   // 前の形(当時と同じ:6 つ以上あれば 3～6)
	else s->item_num = Qz_Rand(100) % (QZ_GROUP_SHOW_MAX - 2) + 3;   // 3～QZ_GROUP_SHOW_MAX
	if (s->item_num > x) s->item_num = x;
	for (tries = 0; tries < 5000; tries++) {
		int a = Qz_Rand(10000) % x, b = Qz_Rand(10000) % x, ok = 0, cnt[3] = { 0, 0, 0 };
		Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&g[a], &g[b]);
		if (tries <= 100) continue;
		for (k = 0; k < s->item_num; k++) cnt[g[k]]++;
		if (s->group_num == 2) ok = (cnt[0] >= 1 && cnt[0] < s->item_num);
		else ok = (cnt[0] >= 1 && cnt[1] >= 1 && cnt[2] >= 1);
		if (ok) break;
	}
	for (i = 0; i < 30; i++) {
		int a = Qz_Rand(10000) % s->item_num, b = Qz_Rand(10000) % s->item_num;
		Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&g[a], &g[b]);
	}
	for (i = 0; i < s->item_num; i++) { s->item[i] = item[i]; s->src[i] = src[i]; s->truth[i] = g[i]; }
	for (i = 0; i < 3; i++) s->group_name[i] = name[i];
}

// 関数の役割:早いもの勝ちの準備。8 個以上なら 8 択、それ未満は 6 択(問題の先頭から)。
//   混ぜるのは 0～(択数-2) の中だけ(最後の 1 つは動かない)で 201 回。問題の 1 番目の選択肢の場所を hayai_mea で追う(当時と同じ)
static void Qz_ServeHayai(const QzQuestion* q, QzServe* s) {
	const char* item[QZ_ITEM_MAX];
	int src[QZ_ITEM_MAX], tf[QZ_ITEM_MAX];
	int x, i, a, b, mea = 0;

	x = q->item_num;
	for (i = 0; i < x; i++) { item[i] = q->item[i]; src[i] = i; tf[i] = q->correct[i] ? 1 : 0; }
	s->item_num = (x < 8) ? 6 : 8;
	if (s->item_num > x) s->item_num = x;
	for (i = 0; i <= 200 && s->item_num > 1; i++) {
		a = Qz_Rand(100000) % (s->item_num - 1); b = Qz_Rand(100000) % (s->item_num - 1);
		if (mea == a) mea = b; else if (mea == b) mea = a;
		Qz_SwapPtr(&item[a], &item[b]); Qz_SwapInt(&src[a], &src[b]); Qz_SwapInt(&tf[a], &tf[b]);
	}
	for (i = 0; i < s->item_num; i++) { s->item[i] = item[i]; s->src[i] = src[i]; s->truth[i] = tf[i]; }
	s->true_num = Qz_CorrectCount(q);
	s->hayai_mea = mea;
}

// 関数の役割:1 問を出す形にする(当時の servequiz の、絵を使わない所)。戻り値:出せれば 1
// 注意:s は問題(q)を指す。q(とそのパック)を捨てたら使えない
int Qz_Serve(const QzQuestion* q, QzServe* s) {
	memset(s, 0, sizeof(*s));
	s->q = q;
	s->bunki = Qz_HasBunki(q);
	s->group_name[0] = s->group_name[1] = s->group_name[2] = "";
	if (q->item_num > QZ_ITEM_MAX) return 0;

	switch (q->kind) {
	case QZ_KIND_OX:     Qz_ServeOx(q, s); break;
	case QZ_KIND_CHOICE:
	case QZ_KIND_RENSO:  Qz_ServeChoice(q, s); break;
	case QZ_KIND_MULTI:  Qz_ServeMulti(q, s); break;
	case QZ_KIND_ORDER:  Qz_ServeOrder(q, s); break;
	case QZ_KIND_LINE:   Qz_ServeLine(q, s); break;
	case QZ_KIND_NARA:   Qz_ServeNara(q, s); break;
	case QZ_KIND_REEL:   Qz_ServeReel(q, s); break;
	case QZ_KIND_MOJI:   Qz_ServeMoji(q, s); break;
	case QZ_KIND_GROUP:  Qz_ServeGroup(q, s); break;
	case QZ_KIND_HAYAI:  Qz_ServeHayai(q, s); break;
	case QZ_KIND_TYPE:
	case QZ_KIND_VISUAL: break;   // 並べる物は無い
	default: return 0;
	}
	return 1;
}
