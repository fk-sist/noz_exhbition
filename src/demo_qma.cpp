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
#include "effect_open.h"   // 開幕の光(quiz_academy から写した)
#include "qz_pack.h"       // クイズの芯(quiz_academy の core/ から写した。問題パックの読み込み)
#include "qz_serve.h"      // (出題の準備:選択肢の並び・並べ替えの札)

// =====================================================
// クイズ OS×UNIX のデモ(2026-10-24)。「突然ですが、ここで問題です」→ 1 問だけ出して、誰も答えないまま時間が切れて
// →「正解は 静岡理工科大学の 授業の中で！」で終わる。プレイはしない(見せるだけ)
//
//   quiz_academy(ブランチ rebuild)のクイズの画面から、出す所を写した:
//     問題パックの読み込みと出題の準備は芯(core/qz_pack・qz_serve)をそのまま。
//     絵は quiz_academy の UI の部品(qma/img/ui/quiz/。tools/gen_ui.py が作った物)と字の絵(qma/img/label/。gen_labels.py)。
//     座標・長さ・動きは game/quiz_scene*.cpp と同じ値(当時の 1360x768 の座標を SX / SY で広げる)。描く関数は名前も中身もなるべくそのまま写した(Qs_～)
//   流れ(quiz_scene の段階に合わせた):
//     突然ですが(このデモだけ)→ 開幕(黒板が落ちてくる・ゲージが伸びる)→ 間 → 形式の表示(光の裂け目から形式名)
//     → 出題の準備(板が浮いてくる)→ 出題(問題文が流れる・ゲージが減る・残り 5 秒から刻む)→ 時間切れ
//     → 黒板が暗くなって「正解は 静岡理工科大学の 授業の中で！」(このデモだけ。「時間切れ」の字は出さない)
//   出す問題:qma/quiz/packs/unix.qz の、○×・四択・連想・並べ替えのうち、絵の無い問題(問題文 4 行まで)。形式はまんべんなく選ぶ
// =====================================================

#define QD_PACK_FILE        "./qma/quiz/packs/unix.qz"
#define QD_BG_FILE          "./qma/bg/room/server.jpg"   // OS×UNIX の部屋(data/ui/genres.txt の room:)
#define QD_BGM_FILE         "./qma/bgm/後衛職業.mp3"      // OS×UNIX の予習の曲(genres.txt の bgm0: 59)
#define QD_BGM_VOLUME       190
#define QD_BGM_FADE_OUT     60      // 終わりで曲を小さくしていく長さ

// --- このデモだけの所(長さ・字) ---
#define QD_INTRO_FRAMES     170     // 「突然ですが、ここで問題です！」の全体
#define QD_INTRO_SLAM       40      // 「ここで問題です！」が叩きつけられるフレーム
#define QD_INTRO_OUT        16      // 最後に字が消えていく長さ
#define QD_IDLE_FRAMES      30      // 開幕のあと、形式の表示までの間(本物は 0～1 秒の乱数)
#define QD_OUTRO_FRAMES     300     // 時間切れのあと、締めの字を見せる長さ
#define QD_OUTRO_AT         24      // 時間切れから、締めの字が出始めるまで
#define QD_OUTRO_STEP       18      // 締めの字の行ごとのずらし
#define QD_TEXT_INTRO1      "突然ですが、"
#define QD_TEXT_INTRO2      "ここで問題です！"
#define QD_TEXT_OUTRO1      "正解は"
#define QD_TEXT_OUTRO2      "静岡理工科大学の"
#define QD_TEXT_OUTRO3      "授業の中で！"

// --- quiz_academy の quiz_scene と同じ値 ---
#define SX(v) ((int)((v) * XSIZE / 1360.0))
#define SY(v) ((int)((v) * YSIZE / 768.0))
#define QS_LIMIT_FRAMES     1500   // 制限時間(25 秒)
#define QS_TICK_FROM        5      // 残り何秒から刻むか
#define QS_SERVE_FRAMES     40     // 出題の準備
#define QS_TEXT_SPEED       7      // 問題文が 1 フレームに出る幅(当時の座標)
#define QS_PAUSE1_FRAMES    80     // 1 個目の @ で止まる
#define QS_PAUSE2_FRAMES    70     // 2 個目の @ で止まる
#define QS_RENSO_WAIT_BASE  85     // 連想:次のヒントまで 85 + 55 * 行
#define QS_RENSO_WAIT_STEP  55
#define QS_RENSO_COV_STEP   15     // 連想:文字が流れている間、覆いが 1 フレームにこれだけ薄くなる
#define QS_POP_FRAMES       12     // 押せる板が、沈んだ所からボコッと浮いてくる長さ
#define QS_POP_DELAY        4
#define QS_POP_STAGGER      3
#define QS_GLOW_PERIOD      200    // 押せる板の縁を光が走る周期
#define QS_GLOW_TAIL        14
#define QS_GLOW_ALPHA       150
#define QS_GLOW_WAVE_C      9
#define QS_TIMEUP_DIM       90     // 時間切れ:黒板の硝子を暗くする濃さ
#define QS_TIMEUP_DIM_IN    10
#define QS_OPEN_FALL        24     // 黒板が落ちてくる長さ
#define QS_OPEN_BOUNCE      8
#define QS_OPEN_BOUNCE_PX   14
#define QS_OPEN_HDR_FRAMES  8
#define QS_OPEN_GAUGE       45
#define QZ_FMT_FRAMES       150    // 形式の表示の長さ
#define QS_FMT_GENRE        23     // 形式の表示:ジャンル名が出るフレーム
#define QS_FMT_GENRE_POP    10
#define QS_FMT_SLIDE        23
#define QS_FMT_BEAMS        20
#define QS_FMT_BEAM_OUT     8
#define QS_FMT_BEAM_SPREAD  120
#define QS_FMT_BEAM_SPEED   40
#define QS_FMT_FLARE_GROW   4
#define QS_FMT_FLARE_FADE   10
#define QS_FMT_FLARE_TH     12
#define QS_FMT_FLASH        8
#define QS_FMT_SHAKE        8
#define QS_FMT_SHAKE_PX     7
#define QS_FMT_ADD_UNTIL    34
#define QS_FMT_SHOT         9
#define QS_FMT_SHOT_W       10
#define QS_FMT_OPEN_AT      7
#define QS_FMT_OPEN_DUR     16
#define QS_FMT_OPEN_OVER    1.25f
#define QS_FMT_OPEN_SCALE   1.18f
#define QS_FMT_EDGE_FADE    14
#define QS_FMT_SHARDS       36
#define QS_FMT_SHARD_SPEED  16
#define QS_FMT_RING_DUR     16
#define QS_FMT_GHOST_STEP   60
#define QS_FMT_RAYS         20
#define QS_FMT_RAY_LEN      620
#define QS_FMT_RAY_OUT      10
#define QS_FMT_BLOOM        12
#define QS_FMT_RING_W       560
#define QO_BOARD_X          322
#define QO_BOARD_Y          0
#define QO_BOARD_H          397
#define QO_TEXT_CX          675
#define QO_TEXT_Y0          90
#define QO_LINE_H           45
#define QO_GAUGE_X          405
#define QO_GAUGE_Y          336
#define QO_GAUGE_W          500
#define QO_GAUGE_H          14
#define QO_GTIME_X          913
#define QO_GTIME_Y          334
#define QO_GLASS_X1         368
#define QO_GLASS_Y1         60
#define QO_GLASS_X2         1012
#define QO_GLASS_BOTTOM     318
#define QO_OX_X             203
#define QO_OX_X2            619
#define QO_OX_Y             413
#define QO_OX_PLATE_DX      168
#define QO_OX_PLATE_DY      58
#define QO_OX_BASE_DX       106
#define QO_OX_BASE_DY       12
#define QO_OX_FLOOR_Y       438
#define QO_OX_PRESS_T       40
#define QO_OX_PLATE_W       423
#define QO_OX_PLATE_H       352
#define QO_CH_CX            685
#define QO_CH_Y0            435
#define QO_CH_PITCH         90
#define QO_CH_TEXT_DY       -28
#define QO_CH_PRESS_T       16
#define QO_CH_FACE_DY       14
#define QO_CH_FACE_H        84
#define QO_CH_FLOOR_Y       114
#define QO_CH_BADGE_CX      44
#define QO_CH_BADGE_CY      64
#define QO_HINT_X           426
#define QO_HINT_W           500
#define QO_NARA_PITCH       92
#define QO_NARA_Y           444
#define QO_NARA_FW          119
#define QO_NARA_FH          166
#define QO_NARA_T           12
#define QO_TILE_M           10
#define QO_NARA_OK_X        601
#define QO_NARA_OK_Y        604
#define QS_NARA_RISE        11
#define QS_NARA_RISE_PX     120
#define QS_NARA_SPIN        13
#define QS_NARA_SPIN_STEP   2
#define QS_NARA_SIZE        56
#define QO_OK_FACE_DX       14
#define QO_OK_FACE_DY       12
#define QO_OK_FACE_W        194
#define QO_OK_FACE_H        109
#define QO_OK_PRESS_T       16
#define QO_OK_FLOOR_Y       137
#define QS_QNUM_CELL_W      49
#define QS_QNUM_CELL_H      63
#define QS_TIME_CELL_W      21
#define QS_TIME_CELL_H      25
#define QS_OX_CELL          30
#define QS_PER_CELL_W       37
#define QS_PER_CELL_H       45
#define QS_PER_IMG_CY       33
#define QS_PER_ROW_CX       898
#define QS_PER_ROW_GAP1     8
#define QS_PER_ROW_GAP2     4
#define QS_PER_NUM_W        64
#define QS_PER_NUM_SCALE    1.2
#define QC_WHITE            GetColor(0xFF, 0xFF, 0xFF)
#define QC_BAR_BLACK        GetColor(0x08, 0x08, 0x0C)

// 形式(quiz_scene の QS_KIND_～ と同じ番号)
#define QS_KIND_OX      1
#define QS_KIND_CHOICE  2
#define QS_KIND_RENSO   3
#define QS_KIND_NARA    7

// 段階
#define QD_PH_INTRO     0
#define QD_PH_OPEN      1
#define QD_PH_IDLE      2
#define QD_PH_FMT       3
#define QD_PH_SERVE     4
#define QD_PH_ASK       5
#define QD_PH_OUTRO     6

#define RESOURCE_GROUP_QMA 30   // このデモの絵と音(終了まで持つ。1 回目に読む)

extern ResourceManager resMgr;

// フォント(quiz_scene の Qs_InitFonts と同じ作り)
static int QF_QUESTION = -1, QF_CHOICE = -1, QF_HINT = -1, QF_NARA = -1;
static int QF_INTRO_S = -1, QF_INTRO_L = -1, QF_OUTRO_S = -1, QF_OUTRO_L = -1;   // このデモだけの字(ＤＦ麗雅宋)

// 絵
enum { QG_BOARD, QG_CH_BASE, QG_CH_PLATE, QG_OX_BASE, QG_OX_MARU, QG_OX_BATU, QG_HINT, QG_GAUGE_G, QG_GAUGE_Y, QG_GAUGE_R,
       QG_TILE_NARA, QG_OK_BASE, QG_OK_PLATE, QG_BADGE_1, QG_BADGE_2, QG_BADGE_3, QG_BADGE_4, QG_BADGE_A, QG_BADGE_B, QG_BADGE_C, QG_BADGE_D,
       QG_PER_LABEL, QG_PER_PCT, QG_BAND_G, QG_TITLE_G, QG_NUM };
static int QGra[QG_NUM];
static int qd_bandk[4], qd_show[4];   // 形式ごとの帯の字・形式名(○×・四択・連想・並べ替え)
static int qnumber[10], qnumber2[5], gtime[11], oxgraph[3], seikairitu[12];
static int qd_bg = -1, qd_bgm = -1;
static int qd_se_board = -1, qd_se_flash = -1, qd_se_serve = -1, qd_se_tick = -1, qd_se_last = -1, qd_se_end = -1;
static int qd_loaded = 0;
static QzPack qd_pack;
static int qd_pack_ok = 0;
static int qd_last_q = -1;

// 今の 1 問(quiz_scene の QsScene から、使う所だけ)
typedef struct {
	const QzQuestion* q;
	QzServe sv;
	int kind;                 // QS_KIND_～
	int kind_i;               // 0 ○× / 1 四択 / 2 連想 / 3 並べ替え(帯・形式名の絵の番号)
	char line[8][256];
	int line_num, cur_line, shown_px, text_done;
	int pause_num, pause_line[3], pause_px[3], pause_used, pause_wait;
	int renso_wait, renso_cov;
	int choice_num;
	char choice[4][64];
	int ox_depth[2], ch_depth[4], ok_depth;
	char word[16][4];
	int word_num;
	int grace;                // ゲージが緑の間(秒)
	int se_1;                 // 最後に刻みを鳴らした残り秒
} QdScene;

static QdScene qs;
static int qd_ph = QD_PH_INTRO, qd_f = 0;      // 段階と、その中のフレーム
static int qd_total = 0;                       // デモの頭からのフレーム(光の時刻)
static int qd_board_dy = 0, qd_hdr_a = 0;      // 開幕:黒板のずれ・帯の中身の濃さ
static float qd_gauge_open = 0.0f;
static int qd_genre_on = 0;                    // 帯にジャンル・形式を出すか(形式の表示から)
static int qs_shift_x = 0, qs_shift_y = 0;
static int qs_glow_on = 0, qs_glow_t = 0;
static float qd_bgm_vol = 1.0f;


// =====================================================
// 読み込み
// =====================================================

// 関数の役割:ＤＦ のフォントを作る(quiz_scene の Qs_InitFonts と同じ形。大きい字はキャッシュを少なく)
static void Qd_InitFonts(void) {
	SetFontCacheCharNum(256);
	QF_QUESTION = CreateFontToHandle("ＤＦＰ平成ゴシック体W9", SY(28), 4, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 2);
	QF_CHOICE   = CreateFontToHandle("ＤＦＰ平成ゴシック体W9", SY(24), 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8, -1, 1);
	QF_HINT     = CreateFontToHandle("ＤＦＰ平成ゴシック体W9", SY(20), 3, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 1);
	SetFontCacheCharNum(48);
	QF_NARA     = CreateFontToHandle("ＤＦＰ平成ゴシック体W9", SY(QS_NARA_SIZE), 3, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 2);
	QF_INTRO_S  = CreateFontToHandle("ＤＦ麗雅宋", SY(58), 7, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 3);
	QF_OUTRO_S  = CreateFontToHandle("ＤＦ麗雅宋", SY(40), 7, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 3);
	SetFontCacheCharNum(16);
	QF_INTRO_L  = CreateFontToHandle("ＤＦ麗雅宋", SY(96), 9, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 3);
	QF_OUTRO_L  = CreateFontToHandle("ＤＦ麗雅宋", SY(68), 9, DX_FONTTYPE_ANTIALIASING_EDGE, -1, 3);
	SetFontCacheCharNum(256);
}

static void Qd_Load(void) {
	static const char* files[] = { "board", "choice_base", "choice_plate", "ox_base", "ox_plate_maru", "ox_plate_batu", "hint", "gauge_green", "gauge_yellow", "gauge_red",
	                               "tile_nara", "ok_base", "ok_plate", "badge_1", "badge_2", "badge_3", "badge_4", "badge_A", "badge_B", "badge_C", "badge_D" };
	// 帯の字・形式名・ジャンル名の絵(img/label/<種類>_<字の FNV-1a>.png。quiz_scene の Qs_LabelGraph と同じ名前)
	static const char* bandk[4] = { "bandk_5ffbe939", "bandk_818b9b35", "bandk_29019d5a", "bandk_bfb78a0b" };   // ～○×クイズ～ ～四択クイズ～ ～連想クイズ～ ～並べ替えクイズ～
	static const char* show[4]  = { "show_614de5b5", "show_edca1ba5", "show_68c470ea", "show_b883019b" };      // ○×クイズ 四択クイズ 連想クイズ 並べ替えクイズ
	ResourceGroup* g;
	char path[160];
	int i;
	if (qd_loaded) return;
	qd_loaded = 1;
	g = resMgr.CreateGroup(RESOURCE_GROUP_QMA);
	for (i = 0; i < QG_PER_LABEL; i++) { _snprintf(path, sizeof(path), "./qma/img/ui/quiz/%s.png", files[i]); path[sizeof(path) - 1] = '\0'; QGra[i] = g->LoadImg(path); }
	QGra[QG_PER_LABEL] = g->LoadImg("./qma/img/label/per_label.png");
	QGra[QG_PER_PCT] = g->LoadImg("./qma/img/label/per_pct.png");
	QGra[QG_BAND_G] = g->LoadImg("./qma/img/label/bandg_0e7aac36.png");    // OS×UNIX(帯の上の段)
	QGra[QG_TITLE_G] = g->LoadImg("./qma/img/label/title_0e7aac36.png");   // OS×UNIX(形式の表示のジャンル名)
	for (i = 0; i < 4; i++) {
		_snprintf(path, sizeof(path), "./qma/img/label/%s.png", bandk[i]); path[sizeof(path) - 1] = '\0'; qd_bandk[i] = g->LoadImg(path);
		_snprintf(path, sizeof(path), "./qma/img/label/%s.png", show[i]); path[sizeof(path) - 1] = '\0'; qd_show[i] = g->LoadImg(path);
	}
	g->LoadDivImg("./qma/img/label/qnum_digit.png", 10, 10, 1, QS_QNUM_CELL_W, QS_QNUM_CELL_H, qnumber);
	g->LoadDivImg("./qma/img/label/qnum_kanji.png", 5, 5, 1, QS_QNUM_CELL_W, QS_QNUM_CELL_H, qnumber2);
	g->LoadDivImg("./qma/img/label/time.png", 11, 11, 1, QS_TIME_CELL_W, QS_TIME_CELL_H, gtime);
	g->LoadDivImg("./qma/img/label/ox.png", 3, 3, 1, QS_OX_CELL, QS_OX_CELL, oxgraph);
	g->LoadDivImg("./qma/img/label/per.png", 12, 12, 1, QS_PER_CELL_W, QS_PER_CELL_H, seikairitu);
	qd_bg = g->LoadImg(QD_BG_FILE);
	qd_bgm = g->LoadSound(QD_BGM_FILE);
	qd_se_board = g->LoadSound("./qma/se/黒板落下.ogg");   // quiz_academy の data/sound/se.txt の 1
	qd_se_flash = g->LoadSound("./qma/se/keisikiflash.ogg");   // 2(形式表示)
	qd_se_serve = g->LoadSound("./qma/se/出題.ogg");          // 3
	qd_se_tick  = g->LoadSound("./qma/se/timeless.ogg");      // 12(残り 5 秒からの刻み)
	qd_se_last  = g->LoadSound("./qma/se/notime.ogg");        // 34(最後の 1 秒)
	qd_se_end   = g->LoadSound("./qma/se/quizend.ogg");       // 9(クイズの終わり)
	Qd_InitFonts();
	qd_pack_ok = (QzPack_Load(&qd_pack, QD_PACK_FILE, NULL) > 0);
}

static void Qd_Se(int h) { if (h >= 0) PlaySoundMem(h, DX_PLAYTYPE_BACK); }

// =====================================================
// 問題を選ぶ・準備する
// =====================================================

// 関数の役割:このデモで出せる問題か(○×・四択・連想・並べ替え。絵が無く、問題文 4 行まで)。戻り値:形式の番号(0～3)。出せなければ -1
static int Qd_Usable(const QzQuestion* q) {
	int k;
	if (q->sub != QZ_SUB_NORMAL || q->image != NULL || q->image2 != NULL) return -1;
	switch (q->kind) {
	case QZ_KIND_OX:     k = 0; break;
	case QZ_KIND_CHOICE: k = 1; break;
	case QZ_KIND_RENSO:  k = 2; break;
	case QZ_KIND_NARA:   k = 3; break;
	default: return -1;
	}
	if (k == 2) { if (q->hint_num < 1 || q->hint_num > 4) return -1; }
	else if (q->text_num < 1 || q->text_num > 4) return -1;
	if (k == 3 && (q->answer_num < 1 || Qz_CharCount(q->answer[0]) < 2 || Qz_CharCount(q->answer[0]) > 9)) return -1;
	return k;
}

// 関数の役割:形式をまんべんなく選んでから、その形式の問題を 1 つ選ぶ(前と同じ問題は避ける)
static int Qd_Pick(void) {
	int cnt[4] = { 0, 0, 0, 0 }, i, kind, n, pick, tries;
	for (i = 0; i < qd_pack.q_num; i++) { int k = Qd_Usable(&qd_pack.q[i]); if (k >= 0) cnt[k]++; }
	for (tries = 0; tries < 8; tries++) {
		kind = GetRand(3);
		if (cnt[kind] == 0) continue;
		pick = GetRand(cnt[kind] - 1);
		for (i = 0, n = 0; i < qd_pack.q_num; i++) {
			if (Qd_Usable(&qd_pack.q[i]) != kind) continue;
			if (n++ == pick) {
				if (i == qd_last_q && cnt[kind] > 1) break;   // 前と同じ:選び直し
				return i;
			}
		}
	}
	for (i = 0; i < qd_pack.q_num; i++) if (Qd_Usable(&qd_pack.q[i]) >= 0) return i;
	return -1;
}

// 関数の役割:問題文を行に分け、@ の位置を覚える(quiz_scene の Qs_SplitText と同じ)
static void Qs_SplitText(QdScene* s) {
	const QzQuestion* q = s->q;
	const char* const* src = q->text;
	int src_num = q->text_num, order[QZ_HINT_MAX], k, li = 0;
	if (s->kind == QS_KIND_RENSO && q->hint_num > 0) {
		src = q->hint; src_num = q->hint_num;
		for (k = 0; k < src_num; k++) order[k] = k;
		if (q->hint_order != QZ_HINT_FIXED && src_num > 1) {   // ヒントの出し方(問題パックの hint_order)
			int from = (q->hint_order == QZ_HINT_KEEP_FIRST) ? 1 : 0, a, b, t;
			for (k = 0; k < 30 && src_num - from > 1; k++) {
				a = from + Qz_Rand(src_num - from - 1); b = from + Qz_Rand(src_num - from - 1);
				t = order[a]; order[a] = order[b]; order[b] = t;
			}
		}
	}
	else for (k = 0; k < src_num && k < QZ_HINT_MAX; k++) order[k] = k;
	s->line_num = 0; s->pause_num = 0;
	for (k = 0; k < src_num && li < 8; k++) {
		const char* t = (src_num <= QZ_HINT_MAX) ? src[order[k]] : src[k];
		char* d = s->line[li];
		const char* qq = t;
		int len = 0;
		while (*qq) {
			int cl = Qz_CharBytes(qq);
			if (Qz_IsBsAt(qq)) { if (len + 1 >= 255) break; d[len++] = QZ_AT_ESC; qq += 2; continue; }   // \\@ は、字の \ を 1 字写して、続く @ は待ち
			if (Qz_IsAtEsc(qq)) { if (len + 1 >= 255) break; d[len++] = '@'; qq += 2; continue; }        // \@ は字としての @
			if (*qq == '@') {
				if (s->pause_num < 3) { d[len] = '\0'; s->pause_line[s->pause_num] = li; s->pause_px[s->pause_num] = GetDrawStringWidthToHandle(d, len, QF_QUESTION); s->pause_num++; }
				qq++; continue;
			}
			if (len + cl >= 255) break;
			memcpy(d + len, qq, cl); len += cl; qq += cl;
		}
		d[len] = '\0';
		li++;
	}
	s->line_num = li;
	s->cur_line = 0; s->shown_px = 0; s->pause_wait = 0; s->pause_used = 0; s->text_done = (li == 0);
	s->renso_wait = 0; s->renso_cov = 0;
}

// 関数の役割:1 問の準備(quiz_scene の Qs_Prepare の、○×・四択・連想・並べ替えの所)
static int Qd_Prepare(void) {
	int qi = Qd_Pick(), i;
	memset(&qs, 0, sizeof(qs));
	if (qi < 0) return 0;
	qd_last_q = qi;
	qs.q = &qd_pack.q[qi];
	qs.kind_i = Qd_Usable(qs.q);
	qs.kind = (qs.kind_i == 0) ? QS_KIND_OX : (qs.kind_i == 1) ? QS_KIND_CHOICE : (qs.kind_i == 2) ? QS_KIND_RENSO : QS_KIND_NARA;
	if (!Qz_ServeSeed(qs.q, &qs.sv, Qz_NewSeed())) return 0;
	Qs_SplitText(&qs);
	if (qs.kind == QS_KIND_OX) qs.choice_num = 2;
	else if (qs.kind == QS_KIND_NARA) {
		qs.word_num = (qs.sv.word_num > 16) ? 16 : qs.sv.word_num;
		for (i = 0; i < qs.word_num; i++) { strncpy(qs.word[i], qs.sv.word[i], 3); qs.word[i][3] = '\0'; }
	}
	else {
		qs.choice_num = (qs.sv.item_num > 4) ? 4 : qs.sv.item_num;
		for (i = 0; i < qs.choice_num; i++) { strncpy(qs.choice[i], qs.sv.item[i], 63); qs.choice[i][63] = '\0'; }
	}
	for (i = 0; i < 2; i++) qs.ox_depth[i] = QO_OX_PRESS_T;   // 出る時は沈んだ所から立ち上がる
	for (i = 0; i < 4; i++) qs.ch_depth[i] = QO_CH_PRESS_T;
	qs.ok_depth = QO_OK_PRESS_T;
	// ゲージが緑の間(満点の猶予。芯の Qz_Grace と同じ考え:○× 2 秒、並べ替え 4 秒、他 3 秒。分岐(@)の問題は 1 秒足す)
	qs.grace = (qs.kind == QS_KIND_OX) ? 2 : (qs.kind == QS_KIND_NARA) ? 4 : 3;
	if (qs.sv.bunki) qs.grace++;
	qs.se_1 = 0;
	return 1;
}

static int Qd_ServeIntro(void) { return (qs.kind == QS_KIND_NARA) ? QS_NARA_RISE : 0; }

// 関数の役割:出題の準備で、押せる板が沈んだ所から 1 枚ずつボコッと浮いてくる(quiz_scene の Qs_ServePop)
static void Qs_ServePop(int f) {
	int i;
	if (qs.kind == QS_KIND_OX) { for (i = 0; i < 2; i++) qs.ox_depth[i] = Ease_Move(QO_OX_PRESS_T, 0, f, QS_POP_DELAY + i * QS_POP_STAGGER, QS_POP_FRAMES, EASE_OUT_BACK); return; }
	if (qs.kind == QS_KIND_NARA) { qs.ok_depth = Ease_Move(QO_OK_PRESS_T, 0, f, QS_NARA_RISE + QS_POP_DELAY, QS_POP_FRAMES, EASE_OUT_BACK); return; }
	for (i = 0; i < qs.choice_num; i++) qs.ch_depth[i] = Ease_Move(QO_CH_PRESS_T, 0, f, QS_POP_DELAY + i * QS_POP_STAGGER, QS_POP_FRAMES, EASE_OUT_BACK);
}

// 関数の役割:問題文を流す(quiz_scene の Qs_AdvanceText の、@ の待ちと連想の待ち・覆い。2 個目の @ の抜けは、出す問題を 4 行までにしたので無い)
static void Qs_AdvanceText(QdScene* s) {
	if (s->text_done) return;
	if (s->pause_wait > 0) { s->pause_wait--; return; }
	if (s->renso_wait > 0) { s->renso_wait--; return; }
	{
		int len = (int)strlen(s->line[s->cur_line]);
		int full = GetDrawStringWidthToHandle(s->line[s->cur_line], len, QF_QUESTION);
		s->shown_px += SX(QS_TEXT_SPEED);
		if (s->kind == QS_KIND_RENSO && s->renso_cov < 255) { s->renso_cov += QS_RENSO_COV_STEP; if (s->renso_cov > 255) s->renso_cov = 255; }
		if (s->pause_used < s->pause_num && s->pause_line[s->pause_used] == s->cur_line && s->shown_px >= s->pause_px[s->pause_used]) {
			s->shown_px = s->pause_px[s->pause_used];
			s->pause_wait = (s->pause_used == 0) ? QS_PAUSE1_FRAMES : QS_PAUSE2_FRAMES;
			s->pause_used++;
		}
		if (s->shown_px >= full) {
			s->shown_px = full;
			if (s->cur_line + 1 < s->line_num) {
				s->cur_line++; s->shown_px = 0;
				if (s->kind == QS_KIND_RENSO) { s->renso_wait = QS_RENSO_WAIT_BASE + QS_RENSO_WAIT_STEP * s->cur_line; s->renso_cov = 0; }
			}
			else s->text_done = 1;
		}
	}
}


// =====================================================
// 描く部品(quiz_scene_draw.cpp・quiz_scene_items.cpp・quiz_scene_build.cpp・quiz_scene_show.cpp から写した)
//   絵は全部、画面の実寸で作ってある(gen_ui.py・gen_labels.py)ので、位置だけ SX / SY で広げて、大きさはそのまま
// =====================================================

static void Qs_Blend(int a) { SetDrawBlendMode(DX_BLENDMODE_ALPHA, a); }
static void Qs_BlendAdd(int a) { SetDrawBlendMode(DX_BLENDMODE_ADD, a); }
static void Qs_BlendOff(void) { SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0); }
static void Qs_DrawImg(int x, int y, int gra) { if (gra > 0) DrawGraph(SX(x) + qs_shift_x, SY(y) + qs_shift_y, gra, TRUE); }
static void Qs_DrawRota(int cx, int cy, float scale, int gra) { if (gra > 0) DrawRotaGraph(SX(cx) + qs_shift_x, SY(cy) + qs_shift_y, scale, 0.0, gra, TRUE); }

static float Qs_Hash01(int i) {
	unsigned int h = (unsigned int)i * 2654435761u;
	h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
	return (h & 0xFFFF) / 65535.0f;
}

// 押せる板の縁がさりげなく光る(上の縁を左から右へ、下の縁を右から左へ、光の粒が尾を引いて走る)
static void Qs_EdgeGlow(float x1, float y1, float x2, float y2, int phase) {
	const int period = QS_GLOW_PERIOD, alpha = QS_GLOW_ALPHA, pulse = 18;
	unsigned int edge = GetColor(140, 230, 255), core = GetColor(200, 245, 255);
	float u, w = x2 - x1, seg = 1.0f / (QS_GLOW_TAIL * 4), fade;
	int k, base;
	if (!qs_glow_on || w <= 0) return;
	u = (float)((qs_glow_t + phase) % period) / period * 1.3f - 0.15f;
	base = (int)(pulse + pulse * 0.8f * sinf((qs_glow_t + phase) * 0.04f * QS_GLOW_PERIOD / period));
	Qs_BlendAdd(base);
	DrawLineAA(x1 + 6, y1 + 1, x2 - 6, y1 + 1, edge, 2.0f);
	DrawLineAA(x1 + 6, y2 - 1, x2 - 6, y2 - 1, edge, 2.0f);
	for (k = 0; k < QS_GLOW_TAIL; k++) {
		float a0 = u - seg * k, a1 = u - seg * (k + 1);
		if (a0 < 0.0f || a1 > 1.0f) continue;
		if (a1 < 0.0f) a1 = 0.0f;
		if (a0 > 1.0f) a0 = 1.0f;
		fade = 1.0f - (float)k / QS_GLOW_TAIL;
		Qs_BlendAdd((int)(alpha * fade * fade));
		DrawLineAA(x1 + w * a1, y1 + 1, x1 + w * a0, y1 + 1, core, 3.0f);
		DrawLineAA(x2 - w * a1, y2 - 1, x2 - w * a0, y2 - 1, core, 3.0f);
	}
	Qs_BlendOff();
}

// 黒板
static void Qs_DrawBoard(void) { DrawGraph(SX(QO_BOARD_X), SY(QO_BOARD_Y) + qd_board_dy, QGra[QG_BOARD], TRUE); }

// 黒板の上の帯の「全国正解率 ？ ％」(quiz_scene の Qs_DrawPerImg。出題中は「？」)
static void Qs_DrawPerImg(void) {
	int lw = 0, lh = 0, pw = 0, ph = 0, cy = SY(QS_PER_IMG_CY) + qs_shift_y, x;
	float ncx;
	GetGraphSize(QGra[QG_PER_LABEL], &lw, &lh);
	GetGraphSize(QGra[QG_PER_PCT], &pw, &ph);
	x = SX(QS_PER_ROW_CX) + qs_shift_x - (lw + QS_PER_ROW_GAP1 + QS_PER_NUM_W + QS_PER_ROW_GAP2 + pw) / 2;
	DrawGraph(x, cy - lh / 2, QGra[QG_PER_LABEL], TRUE);
	ncx = (float)(x + lw + QS_PER_ROW_GAP1 + QS_PER_NUM_W / 2);
	DrawRotaGraphF(ncx, (float)cy, QS_PER_NUM_SCALE, 0.0, seikairitu[10], TRUE);
	DrawGraph(x + lw + QS_PER_ROW_GAP1 + QS_PER_NUM_W + QS_PER_ROW_GAP2, cy - ph / 2, QGra[QG_PER_PCT], TRUE);
}

// 黒板の上の帯:ジャンル・形式、第 1 問、全国正解率、○× の並び(1 問ぶん)
static void Qs_DrawHeader(void) {
	if (qd_hdr_a <= 0) return;
	if (qd_hdr_a < 255) Qs_Blend(qd_hdr_a);
	if (qd_genre_on) {
		Qs_DrawImg(403, 14, QGra[QG_BAND_G]);
		Qs_DrawImg(403, 13, qd_bandk[qs.kind_i]);
	}
	Qs_DrawImg(620, 11, qnumber2[0]);    // 第
	Qs_DrawImg(670, 11, qnumber[1]);     // 1
	Qs_DrawImg(720, 11, qnumber2[3]);    // 問
	Qs_DrawPerImg();
	Qs_DrawImg(680 - 11, 62, oxgraph[2]);   // ○× の並び(まだ答えていない印)
	Qs_BlendOff();
}

// タイムゲージ(緑 → 黄 → 赤)
static void Qs_DrawGauge(int left, int used) {
	int g = (used <= qs.grace * 60) ? QG_GAUGE_G : (used <= 15 * 60) ? QG_GAUGE_Y : QG_GAUGE_R;
	int gw = 0, gh = 0, w;
	GetGraphSize(QGra[g], &gw, &gh);
	w = gw * left / QS_LIMIT_FRAMES;
	if (w <= 0) return;
	SetDrawArea(SX(QO_GAUGE_X), SY(QO_GAUGE_Y), SX(QO_GAUGE_X) + w, SY(QO_GAUGE_Y) + gh);
	DrawGraph(SX(QO_GAUGE_X), SY(QO_GAUGE_Y), QGra[g], TRUE);
	SetDrawArea(0, 0, XSIZE, YSIZE);
}

// 開幕のタイムゲージ(右から p だけ伸びる)
static void Qs_DrawGaugeOpen(float p) {
	int gw = 0, gh = 0, x0 = SX(QO_GAUGE_X), x1 = SX(QO_GAUGE_X + QO_GAUGE_W);
	int left = x1 - (int)((x1 - x0) * p);
	GetGraphSize(QGra[QG_GAUGE_G], &gw, &gh);
	SetDrawArea(left, SY(QO_GAUGE_Y), x1, SY(QO_GAUGE_Y) + gh);
	DrawGraph(x0, SY(QO_GAUGE_Y), QGra[QG_GAUGE_G], TRUE);
	SetDrawArea(0, 0, XSIZE, YSIZE);
}

// 残り時間の数字(xx.xx)
static void Qs_DrawTimeDigits(int left) {
	int sec = left / 60, cs = (left % 60) * 100 / 60;
	Qs_DrawImg(QO_GTIME_X,      QO_GTIME_Y, gtime[(sec / 10) % 10]);
	Qs_DrawImg(QO_GTIME_X + 13, QO_GTIME_Y, gtime[sec % 10]);
	Qs_DrawImg(QO_GTIME_X + 26, QO_GTIME_Y, gtime[10]);
	Qs_DrawImg(QO_GTIME_X + 39, QO_GTIME_Y, gtime[(cs / 10) % 10]);
	Qs_DrawImg(QO_GTIME_X + 52, QO_GTIME_Y, gtime[cs % 10]);
}

// ○× の板(台を描き、板を depth だけ下げて床の線で切って描く)
static void Qs_DrawOxPlate(int x, int y, int is_maru, int depth) {
	DrawGraph(SX(x) + QO_OX_BASE_DX, SY(y) + QO_OX_BASE_DY, QGra[QG_OX_BASE], TRUE);
	SetDrawArea(0, 0, XSIZE, SY(y) + QO_OX_BASE_DY + QO_OX_FLOOR_Y);
	DrawGraph(SX(x) + QO_OX_BASE_DX, SY(y) + QO_OX_BASE_DY + depth, QGra[is_maru ? QG_OX_MARU : QG_OX_BATU], TRUE);
	SetDrawArea(0, 0, XSIZE, YSIZE);
	Qs_EdgeGlow((float)(SX(x) + QO_OX_PLATE_DX), (float)(SY(y) + QO_OX_PLATE_DY + depth), (float)(SX(x) + QO_OX_PLATE_DX + QO_OX_PLATE_W), (float)(SY(y) + QO_OX_PLATE_DY + depth + QO_OX_PLATE_H - QO_OX_PRESS_T), is_maru ? 0 : 100);
}

// 四択・連想の帯(台・板・札。四択は 1～4、連想は A～D)
static void Qs_DrawChoiceBar(int cx, int cy, int index, int letter, int depth) {
	int gw = 0, gh = 0, bw = 0, bh = 0, x, y;
	int badge = (letter ? QG_BADGE_A : QG_BADGE_1) + (index & 3);
	GetGraphSize(QGra[QG_CH_BASE], &gw, &gh); GetGraphSize(QGra[badge], &bw, &bh);
	x = SX(cx) - gw / 2; y = SY(cy) - gh / 2;
	DrawGraph(x, y, QGra[QG_CH_BASE], TRUE);
	SetDrawArea(0, 0, XSIZE, y + QO_CH_FLOOR_Y);
	DrawGraph(x, y + depth, QGra[QG_CH_PLATE], TRUE);
	SetDrawArea(0, 0, XSIZE, YSIZE);
	DrawGraph(x + QO_CH_BADGE_CX - bw / 2, y + QO_CH_BADGE_CY - bh / 2, QGra[badge], TRUE);
	Qs_EdgeGlow((float)(x + QO_CH_BADGE_CX + bw / 2), (float)(y + QO_CH_FACE_DY + depth), (float)(x + gw - gw / 12), (float)(y + QO_CH_FACE_DY + depth + QO_CH_FACE_H), index * 47);
}

// 並べ替え
static int Qs_NaraX(int i) { return 680 - QO_NARA_PITCH / 2 * qs.word_num + QO_NARA_PITCH * i; }

static void Qs_DrawTile(int x, int y, int gra, int fw, int fh, const char* text, int font, unsigned int col) {
	int tw;
	DrawGraph(x - QO_TILE_M, y - QO_TILE_M, QGra[gra], TRUE);
	tw = GetDrawStringWidthToHandle(text, (int)strlen(text), font);
	DrawStringToHandle(x + fw / 2 - tw / 2, y + fh / 2 - GetFontSizeToHandle(font) / 2, text, col, font, QC_WHITE);
}

static void Qs_DrawOk(void) {
	int x = SX(QO_NARA_OK_X), y = SY(QO_NARA_OK_Y);
	DrawGraph(x, y, QGra[QG_OK_BASE], TRUE);
	SetDrawArea(0, 0, XSIZE, y + QO_OK_FLOOR_Y);
	DrawGraph(x, y + qs.ok_depth, QGra[QG_OK_PLATE], TRUE);
	SetDrawArea(0, 0, XSIZE, YSIZE);
	Qs_EdgeGlow((float)(x + QO_OK_FACE_DX), (float)(y + QO_OK_FACE_DY + qs.ok_depth), (float)(x + QO_OK_FACE_DX + QO_OK_FACE_W), (float)(y + QO_OK_FACE_DY + qs.ok_depth + QO_OK_FACE_H), 61);
}

// 並べ替えの出題の準備:札が下から上がってきて、波のように 1 回転する(縦の軸で。裏は藍)。字は出さない(quiz_scene の Qs_DrawNaraServe)
static void Qs_DrawNaraServe(int f) {
	int i;
	int rise = (int)Ease_MoveF((float)SY(QS_NARA_RISE_PX), 0.0f, f, 0, QS_NARA_RISE, EASE_OUT_CUBIC);
	int y0 = SY(QO_NARA_Y) + rise;
	for (i = 0; i < qs.word_num; i++) {
		int x = SX(Qs_NaraX(i)), sf = f - QS_NARA_RISE - QS_NARA_SPIN_STEP * i;
		float ang = (sf >= 0 && sf < QS_NARA_SPIN) ? 6.2831853f * (float)sf / (QS_NARA_SPIN - 1) : 0.0f;
		float c = cosf(ang), w = fabsf(c);
		int cx = x + QO_NARA_FW / 2, hw = (int)(QO_NARA_FW / 2 * w + QO_TILE_M * w);
		int a = (f < QS_NARA_RISE) ? 255 * (f + 1) / QS_NARA_RISE : 255;
		if (hw < 1) hw = 1;
		Qs_Blend(a);
		if (c < 0.0f) SetDrawBright(70, 80, 150);
		DrawExtendGraph(cx - hw, y0 - QO_TILE_M, cx + hw, y0 - QO_TILE_M + QO_NARA_FH + QO_NARA_T + QO_TILE_M * 2, QGra[QG_TILE_NARA], TRUE);
		SetDrawBright(255, 255, 255);
		if (sf >= 0 && sf < QS_NARA_SPIN && w < 0.35f) {
			Qs_BlendAdd((int)(200 * (1.0f - w / 0.35f)));
			DrawBoxAA((float)(cx - 3), (float)(y0 - QO_TILE_M), (float)(cx + 3), (float)(y0 + QO_NARA_FH + QO_NARA_T), GetColor(255, 240, 200), TRUE);
		}
		Qs_BlendOff();
	}
}

static void Qs_DrawNara(int serving, int f) {
	int i, y0 = SY(QO_NARA_Y);
	if (serving) { Qs_DrawNaraServe(f); Qs_DrawOk(); return; }
	for (i = 0; i < qs.word_num; i++) {
		Qs_DrawTile(SX(Qs_NaraX(i)), y0, QG_TILE_NARA, QO_NARA_FW, QO_NARA_FH, qs.word[i], QF_NARA, GetColor(0, 0, 125));
		Qs_EdgeGlow((float)SX(Qs_NaraX(i)), (float)y0, (float)(SX(Qs_NaraX(i)) + QO_NARA_FW), (float)(y0 + QO_NARA_FH), i * QS_GLOW_WAVE_C);
	}
	Qs_DrawOk();
}

// 選択肢(出題の準備から)
static void Qs_DrawChoices(int serving, int f) {
	int i;
	if (qs.kind == QS_KIND_NARA) { Qs_DrawNara(serving, f); return; }
	if (qs.kind == QS_KIND_OX) {
		Qs_DrawOxPlate(QO_OX_X, QO_OX_Y, 1, qs.ox_depth[0]);
		Qs_DrawOxPlate(QO_OX_X2, QO_OX_Y, 0, qs.ox_depth[1]);
		return;
	}
	for (i = 0; i < qs.choice_num; i++) {
		int w = GetDrawStringWidthToHandle(qs.choice[i], (int)strlen(qs.choice[i]), QF_CHOICE);
		Qs_DrawChoiceBar(QO_CH_CX, QO_CH_Y0 + QO_CH_PITCH * i, i, qs.kind == QS_KIND_RENSO, qs.ch_depth[i]);
		DrawStringToHandle(SX(700) - w / 2, SY(QO_CH_Y0 + QO_CH_TEXT_DY + QO_CH_PITCH * i) + qs.ch_depth[i], qs.choice[i], GetColor(0, 0, 0), QF_CHOICE, QC_WHITE);
	}
}

// 問題文(いちばん長い行を中央 675 に置いた左端に揃える。連想は各行中央)と、連想の覆い
static void Qs_DrawText(void) {
	int i, maxw = 0, tx;
	static const char* hint[4] = { "ヒント①", "ヒント②", "ヒント③", "ヒント④" };
	for (i = 0; i < qs.line_num; i++) { int w = GetDrawStringWidthToHandle(qs.line[i], (int)strlen(qs.line[i]), QF_QUESTION); if (w > maxw) maxw = w; }
	tx = SX(QO_TEXT_CX) - maxw / 2;
	for (i = 0; i < qs.line_num && i < 4; i++) {
		int w = GetDrawStringWidthToHandle(qs.line[i], (int)strlen(qs.line[i]), QF_QUESTION);
		int lx = (qs.kind == QS_KIND_RENSO) ? SX(QO_TEXT_CX) - w / 2 : tx;
		int ly = SY(QO_TEXT_Y0 + QO_LINE_H * i);
		if (i < qs.cur_line || qs.text_done) DrawStringToHandle(lx, ly, qs.line[i], QC_WHITE, QF_QUESTION, QC_BAR_BLACK);
		else if (i == qs.cur_line) {
			SetDrawArea(lx, ly, lx + qs.shown_px, ly + SY(QO_LINE_H));
			DrawStringToHandle(lx, ly, qs.line[i], QC_WHITE, QF_QUESTION, QC_BAR_BLACK);
			SetDrawArea(0, 0, XSIZE, YSIZE);
		}
	}
	if (qs.kind == QS_KIND_RENSO) {   // 連想の覆い:まだ始まっていない行は濃いまま。今の行は、文字が流れ出すと薄くなっていく
		for (i = 0; i < qs.line_num && i < 4; i++) {
			int a = (i > qs.cur_line) ? 255 : (i == qs.cur_line && !qs.text_done) ? 255 - qs.renso_cov : 0, hw;
			if (a <= 0) continue;
			hw = GetDrawStringWidthToHandle(hint[i], (int)strlen(hint[i]), QF_HINT);
			if (a < 255) Qs_Blend(a);
			DrawGraph(SX(QO_HINT_X), SY(89 + QO_LINE_H * i), QGra[QG_HINT], TRUE);
			DrawStringToHandle(SX(QO_HINT_X + QO_HINT_W / 2) - hw / 2, SY(89 + QO_LINE_H * i + 6), hint[i], QC_WHITE, QF_HINT, QC_BAR_BLACK);
			Qs_BlendOff();
		}
	}
}

// ---- 形式の表示の光(quiz_scene_show.cpp から写した) ----

// 横一文字の光(真ん中から左右へ伸びて、細くなって消える。縦にも短い光で十字)
static void Qs_FxFlare(float cx, float cy, float half_w, int land) {
	float w, fa, th;
	if (land < 0 || land >= QS_FMT_FLARE_GROW + QS_FMT_FLARE_FADE) return;
	w = Ease_MoveF(0.0f, half_w, land, 0, QS_FMT_FLARE_GROW, EASE_OUT_CUBIC);
	fa = Ease_MoveF(1.0f, 0.0f, land, QS_FMT_FLARE_GROW, QS_FMT_FLARE_FADE, EASE_IN_CUBIC);
	th = SY(QS_FMT_FLARE_TH) * fa;
	if (th <= 0.3f) return;
	Qs_BlendAdd((int)(80 * fa));  DrawLineAA(cx - w, cy, cx + w, cy, GetColor(90, 210, 255), th * 3.0f);
	Qs_BlendAdd((int)(255 * fa)); DrawLineAA(cx - w, cy, cx + w, cy, GetColor(255, 255, 255), th * 0.5f + 1.0f);
	Qs_BlendAdd((int)(200 * fa)); DrawLineAA(cx, cy - SY(60) * fa, cx, cy + SY(60) * fa, GetColor(255, 255, 255), 2.0f);
	Qs_BlendOff();
}

// 後ろの光(右へ走り抜ける光の束と、決まった瞬間の横一文字)
static void Qs_FmtFxBack(int j) {
	float gx1 = (float)SX(QO_GLASS_X1), gx2 = (float)SX(QO_GLASS_X2), gw = gx2 - gx1;
	float cx = (float)SX(680), cy = (float)SY(210);
	unsigned int white = GetColor(255, 255, 255), ice = GetColor(200, 240, 255), cyan = GetColor(90, 210, 255), gold = GetColor(255, 210, 110);
	int k, land = j - QS_FMT_SLIDE;
	SetDrawArea(SX(QO_GLASS_X1), SY(QO_GLASS_Y1), SX(QO_GLASS_X2), SY(QO_GLASS_BOTTOM));
	if (j < QS_FMT_SLIDE) {
		float fa = Ease_MoveF(1.0f, 0.0f, j, QS_FMT_SLIDE - QS_FMT_BEAM_OUT, QS_FMT_BEAM_OUT, EASE_IN_CUBIC);
		for (k = 0; k < QS_FMT_BEAMS; k++) {
			float r = Qs_Hash01(k * 7 + 1) - 0.5f;
			float y = cy + r * fabsf(r) * 4.0f * SY(QS_FMT_BEAM_SPREAD) / 2;
			float len = (float)SX(120) + Qs_Hash01(k * 7 + 2) * SX(380);
			float speed = (float)SX(QS_FMT_BEAM_SPEED) + Qs_Hash01(k * 7 + 3) * SX(QS_FMT_BEAM_SPEED);
			float span = gw + len * 2;
			float x = gx1 - len + fmodf(Qs_Hash01(k * 7 + 4) * span + j * speed, span);
			float th = 1.5f + Qs_Hash01(k * 7 + 5) * 3.0f;
			unsigned int col = (k % 5 == 0) ? gold : (k % 3 == 0) ? cyan : ice;
			Qs_BlendAdd((int)(55 * fa));  DrawLineAA(x, y, x + len, y, col, th * 4.0f);
			Qs_BlendAdd((int)(190 * fa)); DrawLineAA(x + len * 0.35f, y, x + len, y, col, th);
			Qs_BlendAdd((int)(255 * fa)); DrawLineAA(x + len * 0.85f, y, x + len, y, white, th + 1.0f);
		}
		Qs_BlendOff();
	}
	Qs_FxFlare(cx, cy, gw / 2, land);
	SetDrawArea(0, 0, XSIZE, YSIZE);
}

// 形式名を、光の裂け目から出す(撃ち抜く光の線 → 上下に裂けて形式名 → 開ききった瞬間に光の輪)
static void Qs_FmtReveal(int j, int gra) {
	float gx1 = (float)SX(QO_GLASS_X1), gx2 = (float)SX(QO_GLASS_X2);
	float cx = (float)SX(680), cy = (float)SY(210), nh, half;
	unsigned int white = GetColor(255, 255, 255), ice = GetColor(200, 240, 255), cyan = GetColor(90, 210, 255), gold = GetColor(255, 214, 120);
	int land = j - QS_FMT_SLIDE, k, open_end = QS_FMT_OPEN_AT + QS_FMT_OPEN_DUR, gw = 0, gh = 0;
	if (gra <= 0) return;
	GetGraphSize(gra, &gw, &gh);
	nh = (float)gh;
	half = Ease_MoveF(0.0f, nh * QS_FMT_OPEN_OVER / 2, j, QS_FMT_OPEN_AT, QS_FMT_OPEN_DUR, EASE_OUT_CUBIC);
	SetDrawArea(SX(QO_GLASS_X1), SY(QO_GLASS_Y1), SX(QO_GLASS_X2), SY(QO_GLASS_BOTTOM));

	if (j >= QS_FMT_OPEN_AT && j < open_end + QS_FMT_RAY_OUT) {   // 後ろから放たれる光の柱
		float grow = Ease_MoveF(0.0f, 1.0f, j, QS_FMT_OPEN_AT, QS_FMT_OPEN_DUR, EASE_OUT_CUBIC);
		float fa = (j < open_end) ? 1.0f : Ease_MoveF(1.0f, 0.0f, j, open_end, QS_FMT_RAY_OUT, EASE_IN_CUBIC);
		float len = SX(QS_FMT_RAY_LEN) * grow, rot = j * 0.8f;
		for (k = 0; k < QS_FMT_RAYS; k++) {
			float ang = (rot + 360.0f * k / QS_FMT_RAYS + Qs_Hash01(k * 3 + 41) * 8.0f) * DX_PI_F / 180.0f;
			float wid = (3.0f + Qs_Hash01(k * 3 + 42) * 4.0f) * DX_PI_F / 180.0f;
			float l = len * (0.6f + 0.4f * Qs_Hash01(k * 3 + 43));
			float x1 = cx + cosf(ang - wid) * l, y1 = cy + sinf(ang - wid) * l * 0.55f;
			float x2 = cx + cosf(ang + wid) * l, y2 = cy + sinf(ang + wid) * l * 0.55f;
			Qs_BlendAdd((int)(70 * fa)); DrawTriangleAA(cx, cy, x1, y1, x2, y2, (k % 3 == 0) ? gold : cyan, TRUE);
			Qs_BlendAdd((int)(110 * fa)); DrawLineAA(cx, cy, (x1 + x2) / 2, (y1 + y2) / 2, ice, 2.0f);
		}
		Qs_BlendAdd((int)(140 * fa)); DrawCircleAA(cx, cy, SY(60) * (0.5f + grow), 48, ice, TRUE);
		Qs_BlendOff();
	}
	if (j >= open_end && j < open_end + QS_FMT_BLOOM) {   // 開ききった瞬間の大きな白い光
		float r = Ease_MoveF((float)SY(80), (float)SX(420), j, open_end, QS_FMT_BLOOM, EASE_OUT_CUBIC);
		int a = Ease_Move(220, 0, j, open_end, QS_FMT_BLOOM, EASE_OUT_CUBIC);
		Qs_BlendAdd(a / 3); DrawOvalAA(cx, cy, r, r * 0.4f, 64, cyan, TRUE);
		Qs_BlendAdd(a / 2); DrawOvalAA(cx, cy, r * 0.6f, r * 0.22f, 64, white, TRUE);
		Qs_BlendOff();
	}
	if (j >= QS_FMT_OPEN_AT) {   // 形式名(裂け目の中だけ)と、右へ抜けていく残像
		float sc = Ease_MoveF(QS_FMT_OPEN_SCALE, 1.0f, j, QS_FMT_OPEN_AT, QS_FMT_OPEN_DUR, EASE_OUT_CUBIC);
		if (j < open_end) SetDrawArea(SX(QO_GLASS_X1), (int)(cy - half), SX(QO_GLASS_X2), (int)(cy + half) + 1);
		Qs_BlendAdd(150); Qs_DrawRota(-420 + j * QS_FMT_GHOST_STEP, 210, 1.3f, gra);
		if (j <= QS_FMT_ADD_UNTIL) Qs_BlendAdd(255); else Qs_BlendOff();
		Qs_DrawRota(680, 210, sc, gra);
		if (j < open_end) { Qs_BlendAdd(Ease_Move(255, 0, j, QS_FMT_OPEN_AT, QS_FMT_OPEN_DUR, EASE_IN_CUBIC)); Qs_DrawRota(680, 210, sc, gra); }
		Qs_BlendAdd(190); Qs_DrawRota(-420 + j * QS_FMT_GHOST_STEP, 210, 1.8f, gra);
		Qs_BlendOff();
		SetDrawArea(SX(QO_GLASS_X1), SY(QO_GLASS_Y1), SX(QO_GLASS_X2), SY(QO_GLASS_BOTTOM));
	}
	if (j < QS_FMT_OPEN_AT + 1) {   // 撃ち抜く光の線
		float hx = Ease_MoveF(gx1 - SX(40), gx2 + SX(40), j, 0, QS_FMT_SHOT, EASE_OUT_CUBIC);
		float th = (float)SY(QS_FMT_SHOT_W);
		Qs_BlendAdd(70);  DrawLineAA(gx1, cy, hx, cy, cyan, th * 3.5f);
		Qs_BlendAdd(160); DrawLineAA(gx1, cy, hx, cy, ice, th * 1.4f);
		Qs_BlendAdd(255); DrawLineAA(gx1, cy, hx, cy, white, th * 0.5f + 1.0f);
		if (j < QS_FMT_SHOT) { Qs_BlendAdd(120); DrawCircleAA(hx, cy, th * 2.4f, 32, ice, TRUE); Qs_BlendAdd(255); DrawCircleAA(hx, cy, th * 1.0f, 32, white, TRUE); }
		Qs_BlendOff();
	}
	if (j >= QS_FMT_OPEN_AT && j < open_end + QS_FMT_EDGE_FADE) {   // 裂け目の縁と、飛び散る光のかけら
		float fa = (j < open_end) ? 1.0f : Ease_MoveF(1.0f, 0.0f, j, open_end, QS_FMT_EDGE_FADE, EASE_IN_CUBIC);
		float th = SY(QS_FMT_SHOT_W) * (0.35f + 0.65f * fa);
		int t = j - QS_FMT_OPEN_AT, life = QS_FMT_OPEN_DUR + QS_FMT_EDGE_FADE;
		for (k = -1; k <= 1; k += 2) {
			float y = cy + k * half;
			Qs_BlendAdd((int)(70 * fa));  DrawLineAA(gx1, y, gx2, y, cyan, th * 3.0f);
			Qs_BlendAdd((int)(170 * fa)); DrawLineAA(gx1, y, gx2, y, ice, th * 1.2f);
			Qs_BlendAdd((int)(255 * fa)); DrawLineAA(gx1, y, gx2, y, white, th * 0.4f + 1.0f);
		}
		for (k = 0; k < QS_FMT_SHARDS; k++) {
			float born = Qs_Hash01(k * 5 + 11) * QS_FMT_OPEN_DUR * 0.7f, age = t - born;
			float x0 = gx1 + Qs_Hash01(k * 5 + 12) * (gx2 - gx1), dir = (k % 2) ? 1.0f : -1.0f;
			float sp = SY(QS_FMT_SHARD_SPEED) * (0.5f + Qs_Hash01(k * 5 + 13)), drift = (Qs_Hash01(k * 5 + 14) - 0.5f) * sp * 0.8f;
			float x, y;
			if (age < 0 || age > life) continue;
			x = x0 + drift * age;
			y = cy + dir * (Ease_MoveF(0.0f, nh * QS_FMT_OPEN_OVER / 2, (int)(born + QS_FMT_OPEN_AT), QS_FMT_OPEN_AT, QS_FMT_OPEN_DUR, EASE_OUT_CUBIC) + sp * age);
			Qs_BlendAdd((int)(255.0f * (1.0f - age / life)));
			DrawLineAA(x, y, x - drift * 1.5f, y - dir * sp * 1.5f, (k % 4 == 0) ? gold : ice, 2.0f);
		}
		Qs_BlendOff();
	}
	if (land >= 0 && land < QS_FMT_RING_DUR) {   // 決まった瞬間に、横長の光の輪が広がる
		float rx = Ease_MoveF((float)SX(60), (float)SX(QS_FMT_RING_W), land, 0, QS_FMT_RING_DUR, EASE_OUT_CUBIC), ry = rx * 0.28f;
		int ra = Ease_Move(220, 0, land, 0, QS_FMT_RING_DUR, EASE_IN_CUBIC);
		Qs_BlendAdd(ra / 3); DrawOvalAA(cx, cy, rx, ry, 64, cyan, FALSE, 10.0f);
		Qs_BlendAdd(ra);     DrawOvalAA(cx, cy, rx, ry, 64, white, FALSE, 2.5f);
		Qs_BlendOff();
	}
	SetDrawArea(0, 0, XSIZE, YSIZE);
}

// 決まった瞬間、黒板が白く光る
static void Qs_FmtFxFront(int j) {
	int land = j - QS_FMT_SLIDE;
	if (land >= 0 && land < QS_FMT_FLASH) {
		Qs_BlendAdd(Ease_Move(170, 0, land, 0, QS_FMT_FLASH, EASE_OUT_CUBIC));
		DrawBoxAA((float)SX(QO_GLASS_X1), (float)SY(QO_GLASS_Y1), (float)SX(QO_GLASS_X2), (float)SY(QO_GLASS_BOTTOM), GetColor(200, 230, 255), TRUE);
		Qs_BlendOff();
	}
}

static int Qs_FmtShake(int j) {
	int land = j - QS_FMT_SLIDE;
	if (land < 0 || land >= QS_FMT_SHAKE) return 0;
	return (int)(QS_FMT_SHAKE_PX * sinf(land * 2.4f) * (1.0f - (float)land / QS_FMT_SHAKE));
}


// =====================================================
// このデモだけの字(突然ですが / 正解は～)
//   形式名と同じ作り(赤い字に金の縁)・ジャンル名と同じ作り(白い字に青の縁)にそろえた
// =====================================================

// 関数の役割:字を (cx, cy) を真ん中に、倍率 sc で描く(縁の色は、フォントの縁で)
static void Qd_StringC(int cx, int cy, const char* s, int font, double sc, unsigned int col, unsigned int edge) {
	int w = GetDrawStringWidthToHandle(s, (int)strlen(s), font), h = GetFontSizeToHandle(font);
	DrawExtendStringToHandle(cx - (int)(w * sc / 2), cy - (int)(h * sc / 2), sc, sc, s, col, font, edge);
}

// 突然ですが、ここで問題です!(部屋の絵の上。黒板はまだ無い)
static void Qd_DrawIntro(int f) {
	int cx = SCREEN_WIDTH / 2, out = QD_INTRO_FRAMES - QD_INTRO_OUT;
	int a = (f >= out) ? Ease_Move(255, 0, f, out, QD_INTRO_OUT, EASE_IN_CUBIC) : 255;
	// 部屋を少し暗く
	Qs_Blend(Ease_Move(0, 110, f, 0, 20, EASE_OUT_CUBIC) * a / 255);
	DrawBox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, GetColor(4, 8, 24), TRUE);
	Qs_BlendOff();
	// 「突然ですが、」:左から滑り込む
	{
		int x = Ease_Move(-600, cx, f, 0, 18, EASE_OUT_BACK);
		Qs_Blend(Ease_Move(0, 255, f, 0, 10, EASE_OUT_CUBIC) * a / 255);
		Qd_StringC(x, SY(250), QD_TEXT_INTRO1, QF_INTRO_S, 1.0, QC_WHITE, GetColor(0x18, 0x40, 0xC0));
		Qs_BlendOff();
	}
	// 「ここで問題です!」:大きい所から叩きつけられる。決まった瞬間に横一文字の光と閃光
	if (f >= QD_INTRO_SLAM) {
		int t = f - QD_INTRO_SLAM;
		float sc = Ease_MoveF(2.4f, 1.0f, t, 0, 10, EASE_IN_CUBIC);
		Qs_Blend(Ease_Move(0, 255, t, 0, 6, EASE_OUT_CUBIC) * a / 255);
		Qd_StringC(cx + ((t >= 10 && t < 18) ? (int)(10 * sinf(t * 2.4f) * (18 - t) / 8) : 0), SY(400), QD_TEXT_INTRO2, QF_INTRO_L, sc, GetColor(0xD8, 0x14, 0x14), GetColor(0xF0, 0xC4, 0x48));
		if (t >= 10 && t < 40) {   // 収まった瞬間、字が白く光って消えていく
			Qs_BlendAdd((40 - t) * 8 * a / 255);
			Qd_StringC(cx, SY(400), QD_TEXT_INTRO2, QF_INTRO_L, 1.0, GetColor(255, 255, 255), GetColor(255, 255, 255));
		}
		Qs_BlendOff();
		Qs_FxFlare((float)cx, (float)SY(400), (float)SCREEN_WIDTH * 0.45f, t - 10);
	}
}

// 正解は 静岡理工科大学の 授業の中で!(時間切れのあと、暗くした黒板の硝子の上)
static void Qd_DrawOutro(int f) {
	int t = f - QD_OUTRO_AT, k;
	const char* s[3] = { QD_TEXT_OUTRO1, QD_TEXT_OUTRO2, QD_TEXT_OUTRO3 };
	const int cy[3] = { 110, 176, 252 };   // 当時の座標(硝子 60～318 の中)
	int dim = Ease_Move(0, QS_TIMEUP_DIM, f, 0, QS_TIMEUP_DIM_IN, EASE_OUT_CUBIC);
	Qs_Blend(dim); DrawBox(SX(QO_GLASS_X1), SY(QO_GLASS_Y1), SX(QO_GLASS_X2), SY(QO_GLASS_BOTTOM), GetColor(0x1C, 0x22, 0x2E), TRUE); Qs_BlendOff();
	if (t < 0) return;
	for (k = 0; k < 3; k++) {
		int lt = t - k * QD_OUTRO_STEP;
		float sc;
		if (lt < 0) continue;
		sc = Ease_MoveF(1.6f, 1.0f, lt, 0, 10, EASE_OUT_BACK);
		Qs_Blend(Ease_Move(0, 255, lt, 0, 8, EASE_OUT_CUBIC));
		if (k == 0) Qd_StringC(SX(680), SY(cy[k]), s[k], QF_OUTRO_S, sc, QC_WHITE, GetColor(0x18, 0x40, 0xC0));
		else Qd_StringC(SX(680), SY(cy[k]), s[k], QF_OUTRO_L, sc, GetColor(0xD8, 0x14, 0x14), GetColor(0xF0, 0xC4, 0x48));
		if (lt >= 4 && lt < 26) { Qs_BlendAdd((26 - lt) * 10); Qd_StringC(SX(680), SY(cy[k]), s[k], (k == 0) ? QF_OUTRO_S : QF_OUTRO_L, 1.0, GetColor(255, 255, 255), GetColor(255, 255, 255)); }
		Qs_BlendOff();
	}
	SetDrawArea(SX(QO_GLASS_X1), SY(QO_GLASS_Y1), SX(QO_GLASS_X2), SY(QO_GLASS_BOTTOM));
	Qs_FxFlare((float)SX(680), (float)SY(cy[2]), (float)(SX(QO_GLASS_X2) - SX(QO_GLASS_X1)) / 2, t - 2 * QD_OUTRO_STEP - 6);
	SetDrawArea(0, 0, XSIZE, YSIZE);
}


// =====================================================
// 進める・描く
// =====================================================

static void Qd_Phase(int ph) { qd_ph = ph; qd_f = 0; }

static int Qma_Begin(int arg) {
	(void)arg;
	Qd_Load();
	if (!qd_pack_ok || !Qd_Prepare()) return 0;   // 問題が読めない時は飛ばす
	qd_board_dy = -SY(QO_BOARD_H) * 2;   // 黒板は、開幕まで画面の外
	qd_hdr_a = 0; qd_gauge_open = 0.0f; qd_genre_on = 0;
	qd_total = 0;
	qs_glow_on = 0;
	Effect_Open_Stop();
	Qd_Phase(QD_PH_INTRO);
	qd_bgm_vol = 1.0f;
	if (qd_bgm >= 0) { ChangeVolumeSoundMem(QD_BGM_VOLUME, qd_bgm); PlaySoundMem(qd_bgm, DX_PLAYTYPE_LOOP, TRUE); }
	return 1;
}

static int Qma_Update(void) {
	int f = qd_f;
	qd_total++;
	Effect_Open_Update();
	switch (qd_ph) {
	case QD_PH_INTRO:
		if (f == QD_INTRO_SLAM + 10) Qd_Se(qd_se_flash);
		if (f + 1 >= QD_INTRO_FRAMES) { Qd_Phase(QD_PH_OPEN); return 0; }
		break;
	case QD_PH_OPEN: {   // 黒板が落ちてくる → 着地で少し沈む → 帯の中身 → ゲージが伸びる(quiz_scene の Qz_UpdateOpen)
		int g = f - QS_OPEN_FALL;
		if (f == 0) Qd_Se(qd_se_board);
		if (f <= QS_OPEN_FALL) qd_board_dy = Ease_Move(-SY(QO_BOARD_H), 0, f, 0, QS_OPEN_FALL, EASE_IN_CUBIC);
		else if (g <= QS_OPEN_BOUNCE) qd_board_dy = (int)(QS_OPEN_BOUNCE_PX * sinf(3.14159265f * g / QS_OPEN_BOUNCE) * (1.0f - (float)g / QS_OPEN_BOUNCE));
		else qd_board_dy = 0;
		if (f == QS_OPEN_FALL) Effect_Open_Land((float)SX(QO_GLASS_X1), (float)SY(QO_GLASS_Y1), (float)SX(QO_GLASS_X2), (float)SY(QO_GLASS_BOTTOM));
		qd_hdr_a = (g < 0) ? 0 : Ease_Move(0, 255, g, 0, QS_OPEN_HDR_FRAMES, EASE_OUT_CUBIC);
		qd_gauge_open = (g < 0) ? 0.0f : Ease_MoveF(0.0f, 1.0f, g, 0, QS_OPEN_GAUGE, EASE_OUT_CUBIC);
		if (g == QS_OPEN_GAUGE) Effect_Open_Ring((float)SX(QO_GAUGE_X), (float)SY(QO_GAUGE_Y + QO_GAUGE_H / 2), 40.0f);
		if (f > QS_OPEN_FALL + QS_OPEN_GAUGE) { qd_board_dy = 0; Qd_Phase(QD_PH_IDLE); return 0; }
		break;
	}
	case QD_PH_IDLE:
		if (f >= QD_IDLE_FRAMES) { Qd_Se(qd_se_flash); qd_genre_on = 1; Qd_Phase(QD_PH_FMT); return 0; }
		break;
	case QD_PH_FMT:
		if (f >= QZ_FMT_FRAMES) { Qd_Se(qd_se_serve); Qd_Phase(QD_PH_SERVE); return 0; }
		break;
	case QD_PH_SERVE:
		Qs_ServePop(f);
		if (f >= QS_SERVE_FRAMES + Qd_ServeIntro()) { Qd_Phase(QD_PH_ASK); return 0; }
		break;
	case QD_PH_ASK: {
		int rem = QS_LIMIT_FRAMES - f, sec = (rem + 59) / 60;
		Qs_AdvanceText(&qs);
		if (rem > 0 && sec <= QS_TICK_FROM && sec != qs.se_1) { qs.se_1 = sec; Qd_Se((sec == 1) ? qd_se_last : qd_se_tick); }   // 残り 5 秒から刻む
		if (f >= QS_LIMIT_FRAMES) { Qd_Se(qd_se_end); Qd_Phase(QD_PH_OUTRO); return 0; }   // 時間切れ(誰も答えない)
		break;
	}
	case QD_PH_OUTRO:
		if (f == QD_OUTRO_AT + 2 * QD_OUTRO_STEP + 6) Qd_Se(qd_se_flash);   // 最後の行が決まった所
		if (f >= QD_OUTRO_FRAMES - QD_BGM_FADE_OUT && qd_bgm >= 0) {   // 曲を小さくしていく
			qd_bgm_vol = (float)(QD_OUTRO_FRAMES - f) / QD_BGM_FADE_OUT;
			if (qd_bgm_vol < 0.0f) qd_bgm_vol = 0.0f;
			ChangeVolumeSoundMem((int)(QD_BGM_VOLUME * qd_bgm_vol), qd_bgm);
		}
		if (f >= QD_OUTRO_FRAMES) return 1;
		break;
	}
	qd_f++;
	return 0;
}

static void Qma_Draw(void) {
	int f = qd_f;
	qs_glow_on = (qd_ph == QD_PH_ASK);
	qs_glow_t = qd_total;

	// 背景(OS×UNIX の部屋。画面いっぱいに)
	if (qd_bg > 0) DrawExtendGraph(0, 0, XSIZE, YSIZE, qd_bg, FALSE);
	if (qd_ph == QD_PH_INTRO) { Qd_DrawIntro(f); Effect_Open_Draw(); return; }

	// 黒板と帯
	Qs_DrawBoard();
	qs_shift_y = qd_board_dy;
	Qs_DrawHeader();
	qs_shift_y = 0;

	// 問題文(出題中だけ。時間切れで消える)
	if (qd_ph == QD_PH_ASK) Qs_DrawText();

	// タイムゲージと残り時間
	if (qd_ph == QD_PH_OPEN) { if (qd_gauge_open > 0.0f) { Qs_DrawGaugeOpen(qd_gauge_open); Qs_DrawTimeDigits(QS_LIMIT_FRAMES); } }
	else if (qd_ph == QD_PH_ASK) { Qs_DrawGauge(QS_LIMIT_FRAMES - f, f); Qs_DrawTimeDigits(QS_LIMIT_FRAMES - f); }
	else if (qd_ph == QD_PH_OUTRO) Qs_DrawTimeDigits(0);
	else { Qs_DrawGaugeOpen(1.0f); Qs_DrawTimeDigits(QS_LIMIT_FRAMES); }   // 間・形式の表示・出題の準備:満タンのまま

	// 選択肢(出題の準備から)
	if (qd_ph >= QD_PH_SERVE) Qs_DrawChoices(qd_ph == QD_PH_SERVE, f);

	// 形式の表示
	if (qd_ph == QD_PH_FMT) {
		Qs_FmtFxBack(f);
		if (f >= QS_FMT_GENRE) {   // ジャンル名:大きい所から縮んで決まる
			float sc = Ease_MoveF(1.6f, 1.0f, f, QS_FMT_GENRE, QS_FMT_GENRE_POP, EASE_OUT_BACK);
			Qs_Blend(Ease_Move(0, 255, f, QS_FMT_GENRE, QS_FMT_GENRE_POP, EASE_OUT_CUBIC)); Qs_DrawRota(680, 112, sc, QGra[QG_TITLE_G]); Qs_BlendOff();
		}
		qs_shift_x = SX(Qs_FmtShake(f));
		Qs_FmtReveal(f, qd_show[qs.kind_i]);
		qs_shift_x = 0;
		Qs_FmtFxFront(f);
	}

	// 時間切れのあと
	if (qd_ph == QD_PH_OUTRO) Qd_DrawOutro(f);
	Effect_Open_Draw();
}

static void Qma_End(void) {
	if (qd_bgm >= 0) StopSoundMem(qd_bgm);
	Effect_Open_Stop();
}

const DemoDef demo_qma = { "qma", Qma_Begin, Qma_Update, Qma_Draw, Qma_End, 1, DEMO_BGM_NONE };
