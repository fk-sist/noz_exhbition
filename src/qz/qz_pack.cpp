#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qz_pack.h"

// =====================================================
// クイズの芯:問題パック(.qz)の読み込み。使い方と考え方は qz_pack.h
//
//   1 行は `key: value`、`[q ID]`、`[chapter N]`、`[pack]`、`#` か `;` のコメント、空行のどれか(当時の quiz_pack.cpp と同じ読み方)。
//   `key:` の後に何も無い行は、次の行から「空白 2 つ(かタブ)で始まる行」が続く間、それを値の行として読む(問題文など)。
//   続く行が 1 つも無ければ、空の値が 1 つ(`hint:` だけの行 = 空のヒント。当時は読み飛ばしていて、ヒントの位置がずれていた)。
//   同じ key の繰り返し(choice: など)は、並びに足していく。
//   文字列は buf の中でその場で切る(行の終わり・`=` の前・字の間の空白を '\0' にする)ので、写しは作らない。
// =====================================================

#define QZ_KEY_LEN   32      // key の長さの上限
#define QZ_LEVEL_DEFAULT 3   // level: を書かなかった時の難易度(当時の quiz_pack.cpp と同じ)

static const char* qz_empty = "";

// 読み取り途中の 1 問(QzQuestion に、形が決まる前の値を足した物)
typedef struct {
	QzQuestion q;
	char type[32];           // type: の名前
	int  correct_n;          // correct:(先頭 N 個が正解)。書いていなければ 0
} QzWork;

static char* Qz_Trim(char* s);
static void  Qz_RStrip(char* s);
static void  Qz_SetKey(QzPack* pk, QzWork* w, const char* key, char* val);
static void  Qz_Finish(QzPack* pk, QzWork* w, FILE* log);
static void  Qz_AddStr(QzPack* pk, const char** list, int* num, int max, const char* s);

// 右端の改行と空白を落とす(書き換える)
static void Qz_RStrip(char* s) {
	int n = (int)strlen(s);
	while (n > 0 && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = '\0';
}

// 前後の空白(半角)を落とす。全角の空白は落とさない(問題文の行の「　」は意味がある)
static char* Qz_Trim(char* s) {
	while (*s == ' ' || *s == '\t') s++;
	Qz_RStrip(s);
	return s;
}

static void Qz_Copy(char* dst, int cap, const char* s) {
	strncpy(dst, s, cap - 1);
	dst[cap - 1] = '\0';
}

// 並びに 1 つ足す。いっぱいなら足さずに over_num を数える
static void Qz_AddStr(QzPack* pk, const char** list, int* num, int max, const char* s) {
	if (*num >= max) { pk->over_num++; return; }
	list[(*num)++] = s;
}

// 項目を 1 つ足す(item と sub_item を同じ番号で)
static void Qz_AddItem(QzPack* pk, QzWork* w, const char* item, const char* sub) {
	QzQuestion* q = &w->q;
	if (q->item_num >= QZ_ITEM_MAX) { pk->over_num++; return; }
	q->item[q->item_num] = item;
	q->sub_item[q->item_num] = sub;
	q->item_num++;
}

// 一問多答・順番当ての絵の問題:4 分割の 1 枚ずつが選択肢(item は空の文字)
static void Qz_ImageItems(QzPack* pk, QzWork* w) {
	int i;
	if (w->q.image == NULL || w->q.item_num != 0) return;
	w->q.sub = QZ_SUB_IMAGE;
	for (i = 0; i < 4; i++) Qz_AddItem(pk, w, qz_empty, NULL);
}

// 関数の役割:`key: value` の 1 つを、読み取り途中の問題に入れる。知らない key は読み飛ばす
// 注意:val は buf の中。書き換えてよい(reel: の空白を詰める、panel: を 1 字ずつ切る、pair: を `=` で切る)
static void Qz_SetKey(QzPack* pk, QzWork* w, const char* key, char* val) {
	QzQuestion* q = &w->q;

	if (strcmp(key, "type") == 0)            Qz_Copy(w->type, sizeof(w->type), val);
	else if (strcmp(key, "level") == 0)      q->level = atoi(val);
	else if (strcmp(key, "icon") == 0)       q->icon = atoi(val);
	else if (strcmp(key, "text") == 0)       Qz_AddStr(pk, q->text, &q->text_num, QZ_TEXT_LINE_MAX, val);
	else if (strcmp(key, "hint") == 0)       Qz_AddStr(pk, q->hint, &q->hint_num, QZ_HINT_MAX, val);
	else if (strcmp(key, "note") == 0)       Qz_AddStr(pk, q->note, &q->note_num, QZ_NOTE_LINE_MAX, val);
	else if (strcmp(key, "answer") == 0)     Qz_AddStr(pk, q->answer, &q->answer_num, QZ_ANSWER_MAX, val);
	else if (strcmp(key, "choice") == 0)     Qz_AddItem(pk, w, val, NULL);
	else if (strcmp(key, "image") == 0)      q->image = val;
	else if (strcmp(key, "image_pair") == 0) { q->image = val; q->sub = QZ_SUB_IMAGE; }    // ○× の画像 2 択(左右 2 枚の絵)
	else if (strcmp(key, "hint_image") == 0) q->image = val;                                 // 連想のヒントの絵(4 分割)
	else if (strcmp(key, "choice_image") == 0) { q->image2 = val; q->sub = QZ_SUB_IMAGE; }  // 四択・連想の絵の選択肢
	else if (strcmp(key, "mask") == 0)       q->image2 = val;                                // 画像タッチの判定の色
	else if (strcmp(key, "word") == 0)       q->word = val;
	else if (strcmp(key, "quiz_group") == 0) q->quiz_group = val;
	else if (strcmp(key, "correct") == 0)    w->correct_n = atoi(val);
	else if (strcmp(key, "chapter") == 0)    q->chapter = atoi(val);   // (2026-10-21)問題ごとの章。[chapter N] より勝つ。手直しで足した新しい問題が使う(エディタが書き直すと [chapter N] の行は消えるので、ブロックの中に持つ)
	else if (strcmp(key, "input") == 0) {
		if (strcmp(val, "ひら") == 0) q->input = QZ_INPUT_HIRA;
		else if (strcmp(val, "カタ") == 0) q->input = QZ_INPUT_KATA;
		else if (strcmp(val, "英数") == 0) q->input = QZ_INPUT_EISU;
	}
	else if (strcmp(key, "show") == 0)       q->show = (strcmp(val, "all") == 0) ? QZ_SHOW_ALL : atoi(val);
	else if (strcmp(key, "hint_order") == 0) {
		if (strcmp(val, "keep_first") == 0) q->hint_order = QZ_HINT_KEEP_FIRST;
		else if (strcmp(val, "shuffle") == 0) q->hint_order = QZ_HINT_SHUFFLE;
		else q->hint_order = QZ_HINT_FIXED;
	}
	else if (strcmp(key, "reel") == 0) {
		// 「reel: ア セ ラ ネ」:字の間の空白を詰めて 1 本("アセラネ")
		char* s = val;
		char* d = val;
		while (*s) { if (*s != ' ' && *s != '\t') *d++ = *s; s++; }
		*d = '\0';
		Qz_AddItem(pk, w, val, NULL);
	}
	else if (strcmp(key, "panel") == 0) {
		// 「panel: グ レ ー ト …」:空白で切って、1 枚 1 字
		char* s = val;
		while (*s) {
			char* t;
			while (*s == ' ' || *s == '\t') s++;
			if (*s == '\0') break;
			t = s;
			while (*s && *s != ' ' && *s != '\t') s++;
			if (*s) *s++ = '\0';
			Qz_AddItem(pk, w, t, NULL);
		}
	}
	else if (strcmp(key, "pair") == 0 || strcmp(key, "group") == 0) {
		// 「pair: 左 = 右」/「group: グループの名前 = 項目」
		//   区切りは最後の「 = 」(2026-10-18)。左に「 = 」を含む物(再帰式 T(n) = T(n - 1) + 1、for(i = 0; …))があるので。
		//   前は最初の「 = 」で切っていて、左が全部「T(n)」「for(i」になり、線結びが解答不能になっていた(fumiya)
		char* eq = strstr(val, " = ");
		char* nx;
		if (eq == NULL) return;
		while ((nx = strstr(eq + 3, " = ")) != NULL) eq = nx;
		*eq = '\0';
		if (strcmp(key, "pair") == 0) Qz_AddItem(pk, w, Qz_Trim(val), Qz_Trim(eq + 3));
		else Qz_AddItem(pk, w, Qz_Trim(eq + 3), Qz_Trim(val));   // グループ分けは、項目が item、グループの名前が sub_item
	}
	// tag: は、まだ使わない
}

// 関数の役割:読み取り終わった 1 問の形を決めて(答えの番号など)、遊べればパックの並びに入れる
static void Qz_Finish(QzPack* pk, QzWork* w, FILE* log) {
	QzQuestion* q = &w->q;
	char why[128];
	int i, n;

	if (!Qz_KindFind(w->type, &q->kind, &n)) {
		pk->skip_num++;
		sprintf(pk->why, "形式が分からない(type: %.64s)", w->type);
		if (log) fprintf(log, "%s: 形式が分からない(type: %s)\n", q->id, w->type);
		return;
	}
	if (n != QZ_SUB_NORMAL) q->sub = n;   // キューブ・画像タッチ・エフェクト

	// 答えを、形式ごとの形へ
	switch (q->kind) {
	case QZ_KIND_OX:
		if (q->answer_num >= 1) {
			if (strcmp(q->answer[0], "○") == 0 || strcmp(q->answer[0], "A") == 0) q->answer_no = 0;
			else if (strcmp(q->answer[0], "×") == 0 || strcmp(q->answer[0], "B") == 0) q->answer_no = 1;
		}
		q->answer_num = 0;
		break;

	case QZ_KIND_CHOICE:
	case QZ_KIND_RENSO:
		if (q->answer_num >= 1) q->answer_no = atoi(q->answer[0]) - 1;
		if (q->sub != QZ_SUB_IMAGE && q->answer_no >= 0 && q->answer_no < q->item_num) q->correct[q->answer_no] = 1;
		q->answer_num = 0;
		break;

	case QZ_KIND_VISUAL:
		if (q->sub == QZ_SUB_TOUCH) {
			if (q->answer_num >= 1) q->answer_no = atoi(q->answer[0]);
			q->answer_num = 0;
		}
		break;

	case QZ_KIND_ORDER:
		Qz_ImageItems(pk, w);
		break;

	case QZ_KIND_MULTI:
	case QZ_KIND_HAYAI:
		if (q->kind == QZ_KIND_MULTI) Qz_ImageItems(pk, w);
		// 正解の書き方は、一問多答と早いもの勝ちで同じ
		if (w->correct_n > 0) {
			// correct: N … 先頭 N 個が正解(当時の書き方を変換した物)
			for (i = 0; i < w->correct_n && i < q->item_num; i++) q->correct[i] = 1;
		}
		else {
			// answer: 1 3 … 正解の番号(docs/quiz_pack.md の書き方)
			for (n = 0; n < q->answer_num; n++) {
				const char* s = q->answer[n];
				while (*s >= '0' && *s <= '9') {
					int k = atoi(s);
					if (k >= 1 && k <= q->item_num) q->correct[k - 1] = 1;
					while (*s >= '0' && *s <= '9') s++;
					while (*s == ' ' || *s == ',') s++;
				}
			}
		}
		q->answer_num = 0;
		break;

	case QZ_KIND_LINE:
		if (q->image != NULL) q->sub = QZ_SUB_IMAGE;
		break;

	case QZ_KIND_GROUP:
		// グループの数:名前の種類を数える
		{
			const char* names[QZ_ITEM_MAX];
			int k, m = 0;
			for (i = 0; i < q->item_num; i++) {
				if (q->sub_item[i] == NULL) continue;   // (2026-10-14)グループの名前が無い項目(group: でなく choice: で書いた時)。Qz_Check で遊べない問題にする
				for (k = 0; k < m; k++) if (strcmp(names[k], q->sub_item[i]) == 0) break;
				if (k == m) names[m++] = q->sub_item[i];
			}
			q->group_num = m;
		}
		break;
	}

	if (!Qz_Check(q, why, sizeof(why))) {
		pk->skip_num++;
		Qz_Copy(pk->why, sizeof(pk->why), why);   // (2026-10-21)ゲームの中で直した時に、理由を見せる
		if (log) fprintf(log, "%s: %s(%s)\n", q->id, why, w->type);
		return;
	}
	pk->q[pk->q_num++] = *q;
}

// 新しい 1 問を始める
static void Qz_Begin(QzWork* w, const char* id, int chapter) {
	memset(w, 0, sizeof(*w));
	Qz_Copy(w->q.id, QZ_ID_LEN, id);
	w->q.level = QZ_LEVEL_DEFAULT;
	w->q.chapter = chapter;
	w->q.answer_no = -1;
}

// 関数の役割:pk->buf(読んだ中身)を、問題の並びにする。QzPack_Load と QzPack_LoadText の中身(2026-10-21 に分けた)
static int Qz_Parse(QzPack* pk, FILE* log);

// 関数の役割:.qz を 1 つ読む。戻り値:並びに入れた問題の数(開けなければ -1)
// 注意:前に読んだ中身があっても捨てない(先に QzPack_Free すること)。pk は 0 で埋めてから使う
int QzPack_Load(QzPack* pk, const char* path, FILE* log) {
	FILE* f;
	long size;

	memset(pk, 0, sizeof(*pk));
	Qz_Copy(pk->path, sizeof(pk->path), path);

	f = fopen(path, "rb");
	if (f == NULL) return -1;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	pk->buf = (char*)malloc(size + 1);
	if (pk->buf == NULL) { fclose(f); return -1; }
	size = (long)fread(pk->buf, 1, size, f);
	pk->buf[size] = '\0';
	fclose(f);
	return Qz_Parse(pk, log);
}

int QzPack_LoadText(QzPack* pk, const char* text, FILE* log) {
	size_t size = strlen(text);
	memset(pk, 0, sizeof(*pk));
	pk->buf = (char*)malloc(size + 1);
	if (pk->buf == NULL) return -1;
	memcpy(pk->buf, text, size + 1);
	return Qz_Parse(pk, log);
}

static int Qz_Parse(QzPack* pk, FILE* log) {
	char* line;
	char* next;
	int count = 0, chapter = 0, in_q = 0, in_pack = 0;
	char lastkey[QZ_KEY_LEN];      // 続きの行を足す先の key
	int  pend = 0;                 // lastkey が「key: だけ」の行で、まだ続きの行が来ていない
	char* pendval = NULL;          // その行の、空の値(buf の中の '\0'。空の値として渡す)
	QzWork w;

	// 問題の数を数える(行の頭の "[q ")
	for (line = pk->buf; line != NULL; line = strchr(line, '\n') ? strchr(line, '\n') + 1 : NULL) {
		if (strncmp(line, "[q ", 3) == 0) count++;
	}
	pk->q = (QzQuestion*)malloc((count > 0 ? count : 1) * sizeof(QzQuestion));
	if (pk->q == NULL) { QzPack_Free(pk); return -1; }

	lastkey[0] = '\0';
	memset(&w, 0, sizeof(w));
	for (line = pk->buf; line != NULL; line = next) {
		char* p;
		char key[QZ_KEY_LEN];
		char* val;

		next = strchr(line, '\n');
		if (next != NULL) *next++ = '\0';
		Qz_RStrip(line);
		if (line[0] == '#' || line[0] == ';') continue;

		// 続きの行(先頭が空白 2 つかタブ)。直前の key に足す
		if ((line[0] == ' ' && line[1] == ' ') || line[0] == '\t') {
			if (in_q && lastkey[0]) { Qz_SetKey(pk, &w, lastkey, Qz_Trim(line)); pend = 0; }
			continue;
		}
		if (line[0] == '\0') continue;

		// ここからは新しい行。「key: だけ」の行に続きが来なかったら、空の値を 1 つ
		if (in_q && pend) Qz_SetKey(pk, &w, lastkey, pendval);
		pend = 0;
		lastkey[0] = '\0';

		if (line[0] == '[') {
			if (in_q) { Qz_Finish(pk, &w, log); in_q = 0; }
			in_pack = 0;
			if (strncmp(line, "[q ", 3) == 0) {
				p = strchr(line, ']');
				if (p != NULL) *p = '\0';
				Qz_Begin(&w, Qz_Trim(line + 3), chapter);
				in_q = 1;
			}
			else if (strncmp(line, "[chapter", 8) == 0) {
				chapter = atoi(line + 8);
				if (chapter > pk->chapter_num) pk->chapter_num = (chapter <= QZ_PACK_CHAPTER_MAX) ? chapter : QZ_PACK_CHAPTER_MAX;
			}
			else if (strncmp(line, "[pack]", 6) == 0) in_pack = 1;
			continue;
		}

		// `key: value`
		p = strchr(line, ':');
		if (p == NULL) continue;
		*p = '\0';
		Qz_Copy(key, sizeof(key), Qz_Trim(line));
		val = Qz_Trim(p + 1);
		// 行末の `; コメント` は、値の中に `;` を使うかもしれないので落とさない(書く側で使わない。当時の読み込みと同じ)

		if (in_pack) {
			if (strcmp(key, "id") == 0) Qz_Copy(pk->id, sizeof(pk->id), val);
			else if (strcmp(key, "title") == 0) Qz_Copy(pk->title, sizeof(pk->title), val);
			else if (strcmp(key, "version") == 0) Qz_Copy(pk->version, sizeof(pk->version), val);
		}
		else if (in_q) {
			Qz_Copy(lastkey, sizeof(lastkey), key);
			if (*val) Qz_SetKey(pk, &w, key, val);
			else { pend = 1; pendval = val; }
		}
		else if (chapter >= 1 && chapter <= QZ_PACK_CHAPTER_MAX && strcmp(key, "title") == 0) {
			Qz_Copy(pk->chapter_title[chapter - 1], QZ_PACK_TITLE_LEN, val);
		}
	}
	if (in_q && pend) Qz_SetKey(pk, &w, lastkey, pendval);
	if (in_q) Qz_Finish(pk, &w, log);

	if (pk->over_num > 0 && log) fprintf(log, "%s: 上限を超えて切り捨てた所が %d 個(QZ_～_MAX を見直す)\n", pk->path, pk->over_num);
	return pk->q_num;
}

// 関数の役割:手直しのファイルで、同じ ID の問題を差し替える。説明は qz_pack.h(2026-10-18)
//   手直しは、fumiya がプレイしながら見つけたおかしな問題を、自分で直した物(quiz/fix/<パックの ID>.qz)。
//   生成するパック(tools/*/gen.py)を作り直しても消えないように、元のパックとは別のファイルに置いて、読む時に重ねる。
//   章は、手直しの方に [chapter N](か chapter:)が無ければ(0)、元の問題の章のまま
//   (2026-10-21 fumiya:新しい問題もエディタでじゃんじゃん足したい。置き場は手直しと混ぜてよい)
//   元のパックに無い ID でも、章(chapter:)を書いた問題は、新しい問題として並びの後ろに足す。章の無い物は、ID の書き間違いとみて今までどおり捨てる。
//   足すのはここ(起動時、成績を作る前)だけ。成績・出題の印の並びは q_num で作るので、遊んでいる途中では増やさない(QzPack_FixText は足さない)
int QzPack_Fix(QzPack* pk, const char* path, FILE* log) {
	QzPack fx;
	int i, k, n = 0, add = 0;
	char* hit;   // 手直しの問題ごとに:元のパックの何番か(-1 なら新しい問題)

	if (pk->fix_buf != NULL) return -1;
	if (QzPack_Load(&fx, path, log) < 0) return 0;   // ファイルが無い(手直しが無い)
	hit = (char*)calloc(fx.q_num > 0 ? fx.q_num : 1, 1);
	if (hit == NULL) { QzPack_Free(&fx); return 0; }
	for (i = 0; i < fx.q_num; i++) {
		int chapter;
		for (k = 0; k < pk->q_num; k++) if (strcmp(pk->q[k].id, fx.q[i].id) == 0) break;
		if (k == pk->q_num) {
			if (fx.q[i].chapter != 0) { hit[i] = 1; add++; continue; }   // 新しい問題(後で足す)
			if (log) fprintf(log, "%s: 手直しの問題が、元のパックに無い(%s。新しい問題なら chapter: を書く)\n", fx.q[i].id, path);
			continue;
		}
		chapter = (fx.q[i].chapter != 0) ? fx.q[i].chapter : pk->q[k].chapter;
		pk->q[k] = fx.q[i];
		pk->q[k].chapter = chapter;
		n++;
	}
	if (add > 0) {   // 新しい問題を後ろに足す
		QzQuestion* q = (QzQuestion*)realloc(pk->q, (pk->q_num + add) * sizeof(QzQuestion));
		if (q == NULL) add = 0;
		else {
			pk->q = q;
			for (i = 0; i < fx.q_num; i++) {
				if (!hit[i]) continue;
				for (k = 0; k < pk->q_num; k++) if (strcmp(pk->q[k].id, fx.q[i].id) == 0) break;
				if (k < pk->q_num) continue;   // 同じ ID を 2 つ書いた(先の物だけ)
				pk->q[pk->q_num++] = fx.q[i];
				if (fx.q[i].chapter > pk->chapter_num) pk->chapter_num = (fx.q[i].chapter <= QZ_PACK_CHAPTER_MAX) ? fx.q[i].chapter : QZ_PACK_CHAPTER_MAX;
				pk->add_num++;
				n++;
			}
		}
	}
	free(hit);
	if (fx.q != NULL) free(fx.q);
	if (n > 0) pk->fix_buf = fx.buf;   // 差し替えた問題の文字列は、手直しの中身を指している。パックを捨てる時に一緒に捨てる
	else if (fx.buf != NULL) free(fx.buf);
	pk->fix_num = n;
	return n;
}

// 関数の役割:文字列の問題で差し替える。説明は qz_pack.h(2026-10-21)
int QzPack_FixText(QzPack* pk, const char* text, FILE* log) {
	QzPack fx;
	char** more;
	int i, k, n = 0;

	pk->why[0] = '\0';
	if (QzPack_LoadText(&fx, text, log) < 0) return 0;
	if (fx.q_num == 0) {   // 遊べない(理由を写す)
		Qz_Copy(pk->why, sizeof(pk->why), fx.why[0] ? fx.why : "問題が読めない");
		QzPack_Free(&fx);
		return 0;
	}
	for (i = 0; i < fx.q_num; i++) {
		int chapter;
		for (k = 0; k < pk->q_num; k++) if (strcmp(pk->q[k].id, fx.q[i].id) == 0) break;
		if (k == pk->q_num) {
			sprintf(pk->why, "元のパックに無い ID(%.64s。新しい問題は起動し直すと入る)", fx.q[i].id);
			if (log) fprintf(log, "%s: 直した問題が、元のパックに無い(新しい問題は QzPack_Fix で起動時に足す)\n", fx.q[i].id);
			continue;
		}
		chapter = (fx.q[i].chapter != 0) ? fx.q[i].chapter : pk->q[k].chapter;
		pk->q[k] = fx.q[i];
		pk->q[k].chapter = chapter;
		n++;
	}
	if (fx.q != NULL) free(fx.q);
	if (n == 0) { free(fx.buf); return 0; }
	more = (char**)realloc(pk->fix_more, (pk->fix_more_num + 1) * sizeof(char*));   // 文字列の置き場を足す(前の物も残す)
	if (more == NULL) { free(fx.buf); return 0; }   // (起きないはず。差し替えた問題の文字列が消えるので、差し替えは無かったことにはできない)
	pk->fix_more = more;
	pk->fix_more[pk->fix_more_num++] = fx.buf;
	return n;
}

void QzPack_Free(QzPack* pk) {
	int i;
	if (pk->q != NULL) free(pk->q);
	if (pk->buf != NULL) free(pk->buf);
	if (pk->fix_buf != NULL) free(pk->fix_buf);
	for (i = 0; i < pk->fix_more_num; i++) free(pk->fix_more[i]);   // (2026-10-21)
	if (pk->fix_more != NULL) free(pk->fix_more);
	pk->q = NULL;
	pk->buf = NULL;
	pk->fix_buf = NULL;
	pk->fix_more = NULL;
	pk->fix_more_num = 0;
	pk->q_num = 0;
	pk->fix_num = 0;
	pk->add_num = 0;
}

const QzQuestion* QzPack_Find(const QzPack* pk, const char* id) {
	int i;
	for (i = 0; i < pk->q_num; i++) if (strcmp(pk->q[i].id, id) == 0) return &pk->q[i];
	return NULL;
}
