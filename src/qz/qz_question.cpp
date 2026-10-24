#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

#include "qz_question.h"

// =====================================================
// クイズの芯:1 問の形(形式の名前・遊べるかの確かめ・CP932 の文字)。説明は qz_question.h
// =====================================================

// 形式の名前(QZ_KIND_～ の順)
static const char* qz_kindNames[QZ_KIND_NUM] = {
	"○×", "四択", "連想", "並べ替え", "スロット", "文字パネル", "タイピング", "ビジュアル",
	"線結び", "一問多答", "順番当て", "グループ分け", "早いもの勝ち"
};

// .qz の type: に書く名前のうち、形式の名前と違う物(ビジュアルは 3 つに分けて書く。docs/quiz_pack.md)
static const struct { const char* type; int kind; int sub; } qz_typeAlias[] = {
	{ "キューブ",   QZ_KIND_VISUAL, QZ_SUB_CUBE },
	{ "画像タッチ", QZ_KIND_VISUAL, QZ_SUB_TOUCH },
	{ "エフェクト", QZ_KIND_VISUAL, QZ_SUB_EFFECT },
};

const char* Qz_KindName(int kind) {
	return (kind >= 0 && kind < QZ_KIND_NUM) ? qz_kindNames[kind] : "";
}

int Qz_KindFind(const char* type, int* kind, int* sub) {
	int i;
	*kind = QZ_KIND_NONE;
	*sub = QZ_SUB_NORMAL;
	if (type == NULL) return 0;
	for (i = 0; i < (int)(sizeof(qz_typeAlias) / sizeof(qz_typeAlias[0])); i++) {
		if (strcmp(qz_typeAlias[i].type, type) == 0) { *kind = qz_typeAlias[i].kind; *sub = qz_typeAlias[i].sub; return 1; }
	}
	for (i = 0; i < QZ_KIND_NUM; i++) {
		if (strcmp(qz_kindNames[i], type) == 0) { *kind = i; return 1; }
	}
	return 0;
}

int Qz_CorrectCount(const QzQuestion* q) {
	int i, n = 0;
	for (i = 0; i < q->item_num; i++) if (q->correct[i]) n++;
	return n;
}

// ============================
// CP932 の文字
// ============================

// 2 バイト文字の 1 バイト目か(0x81～0x9F、0xE0～0xFC。半角カナ 0xA1～0xDF は 1 バイト)
static int Qz_IsLead(unsigned char c) {
	return (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xFC);
}

int Qz_CharBytes(const char* s) {
	if (s == NULL || s[0] == '\0') return 0;
	if (Qz_IsLead((unsigned char)s[0]) && s[1] != '\0') return 2;
	return 1;
}

// 関数の役割:s(1 字の頭)が字としての @ の書き方 \@ か
int Qz_IsAtEsc(const char* s) {
	return (s != NULL && s[0] == QZ_AT_ESC && s[1] == '@');
}

// 関数の役割:s(1 字の頭)が \\@(字としての \ と、待ちの @)か
int Qz_IsBsAt(const char* s) {
	return (s != NULL && s[0] == QZ_AT_ESC && s[1] == QZ_AT_ESC && s[2] == '@');
}

// 関数の役割:問題文の 1 行の、待ちの印の @ の数。\@(字としての @)は数えない。2 バイト文字は 1 字ずつ飛ばす
int Qz_AtCount(const char* s) {
	int n = 0, b;
	if (s == NULL) return 0;
	while ((b = Qz_CharBytes(s)) > 0) {
		if (Qz_IsBsAt(s)) { s += 2; continue; }   // \\@:\ は字。続く @ は次で待ちとして数える
		if (Qz_IsAtEsc(s)) { s += 2; continue; }
		if (*s == '@') n++;
		s += b;
	}
	return n;
}

int Qz_CharCount(const char* s) {
	int n = 0, b;
	if (s == NULL) return 0;
	while ((b = Qz_CharBytes(s)) > 0) { s += b; n++; }
	return n;
}

int Qz_CharAt(const char* s, int n, char* out, int cap) {
	int i = 0, b;
	if (cap > 0) out[0] = '\0';
	if (s == NULL) return 0;
	while ((b = Qz_CharBytes(s)) > 0) {
		if (i == n) {
			if (b + 1 > cap) return 0;
			memcpy(out, s, b);
			out[b] = '\0';
			return 1;
		}
		s += b;
		i++;
	}
	return 0;
}

// 字 c(1 字)が、文字列 s の中にあるか
static int Qz_HasChar(const char* s, const char* c) {
	int b, cb = (int)strlen(c);
	while ((b = Qz_CharBytes(s)) > 0) {
		if (b == cb && memcmp(s, c, b) == 0) return 1;
		s += b;
	}
	return 0;
}

// ============================
// 遊べるかの確かめ
// ============================

static int Qz_Ng(char* why, int cap, const char* msg) {
	if (why != NULL && cap > 0) { strncpy(why, msg, cap - 1); why[cap - 1] = '\0'; }
	return 0;
}

// 関数の役割:遊べる問題か(出題の準備・判定が成り立つか)を確かめる。遊べなければ 0 と理由
// 注意:当時の読み込み(quiz_pack.cpp の qz_to_problem)が捨てていた物と、同じ所で捨てるのが基本。
//      当時は通していたが、出すと止まる・正解できない物も、ここで捨てる(理由に書く)
int Qz_Check(const QzQuestion* q, char* why, int cap) {
	char c[4];
	int i, n;

	if (q->kind < 0 || q->kind >= QZ_KIND_NUM) return Qz_Ng(why, cap, "形式が分からない");
	if (q->level < 1 || q->level > 5) return Qz_Ng(why, cap, "難易度が 1～5 でない");

	switch (q->kind) {
	case QZ_KIND_OX:
		if (q->answer_no != 0 && q->answer_no != 1) return Qz_Ng(why, cap, "答えが ○ × (A B) でない");
		if (q->sub == QZ_SUB_IMAGE && q->image == NULL) return Qz_Ng(why, cap, "画像 2 択の絵が無い");
		break;

	case QZ_KIND_CHOICE:
	case QZ_KIND_RENSO:
		if (q->kind == QZ_KIND_RENSO && q->hint_num < 1 && q->image == NULL) return Qz_Ng(why, cap, "ヒントが無い");
		if (q->sub == QZ_SUB_IMAGE) {
			if (q->image2 == NULL) return Qz_Ng(why, cap, "選択肢の絵が無い");
			if (q->answer_no < 0 || q->answer_no > 3) return Qz_Ng(why, cap, "答えの番号が 1～4 でない");
			break;
		}
		if (q->item_num < 3) return Qz_Ng(why, cap, "選択肢が 3 つ未満");
		if (q->answer_no < 0 || q->answer_no >= q->item_num) return Qz_Ng(why, cap, "答えの番号が選択肢の中に無い");
		break;

	case QZ_KIND_NARA:
		if (q->answer_num < 1 || Qz_CharCount(q->answer[0]) < 2) return Qz_Ng(why, cap, "答えが 2 字未満");
		break;

	case QZ_KIND_REEL:
		if (q->answer_num < 1) return Qz_Ng(why, cap, "答えが無い");
		n = Qz_CharCount(q->answer[0]);
		if (q->item_num < 1) return Qz_Ng(why, cap, "リールが無い");
		if (n != q->item_num) return Qz_Ng(why, cap, "答えの字数とリールの数が違う");
		for (i = 0; i < n; i++) {
			Qz_CharAt(q->answer[0], i, c, sizeof(c));
			if (!Qz_HasChar(q->item[i], c)) return Qz_Ng(why, cap, "リールに答えの字が無い");
		}
		break;

	case QZ_KIND_MOJI:
		if (q->answer_num < 1 || Qz_CharCount(q->answer[0]) < 1) return Qz_Ng(why, cap, "答えが無い");
		if (q->item_num < 1) return Qz_Ng(why, cap, "パネルが無い");
		n = Qz_CharCount(q->answer[0]);
		for (i = 0; i < n; i++) {
			int k, found = 0;
			Qz_CharAt(q->answer[0], i, c, sizeof(c));
			for (k = 0; k < q->item_num; k++) if (strcmp(q->item[k], c) == 0) found = 1;
			if (!found) return Qz_Ng(why, cap, "パネルに答えの字が無い");
		}
		break;

	case QZ_KIND_TYPE:
		if (q->input == QZ_INPUT_NONE) return Qz_Ng(why, cap, "入力(input:)が無い");
		if (q->answer_num < 1) return Qz_Ng(why, cap, "答えが無い");
		break;

	case QZ_KIND_VISUAL:
		if (q->sub == QZ_SUB_CUBE) {
			if (q->input == QZ_INPUT_NONE) return Qz_Ng(why, cap, "入力(input:)が無い");
			if (q->answer_num < 1) return Qz_Ng(why, cap, "答えが無い");
		}
		else if (q->sub == QZ_SUB_TOUCH) {
			if (q->image == NULL) return Qz_Ng(why, cap, "画像タッチの絵が無い");
			if (q->answer_no < 0) return Qz_Ng(why, cap, "答えの色の番号が無い");
			if (q->answer_no > 5) return Qz_Ng(why, cap, "答えの色の番号が 0～5 でない");   // (2026-10-14)判定の色は 6 つ(当時の touchans)
		}
		else if (q->sub == QZ_SUB_EFFECT) {
			if (q->image == NULL && q->word == NULL) return Qz_Ng(why, cap, "崩す文字(word:)も絵も無い");
			if (q->input == QZ_INPUT_NONE) return Qz_Ng(why, cap, "入力(input:)が無い");
			if (q->answer_num < 1) return Qz_Ng(why, cap, "答えが無い");
		}
		else return Qz_Ng(why, cap, "ビジュアルの種類が分からない");
		break;

	case QZ_KIND_LINE:
		if (q->item_num < 3) return Qz_Ng(why, cap, "組が 3 つ未満");
		for (i = 0; i < q->item_num; i++) if (q->sub_item[i] == NULL) return Qz_Ng(why, cap, "右の項目が無い組がある(pair: 左 = 右)");   // (2026-10-14)
		break;

	case QZ_KIND_MULTI:
		if (q->sub != QZ_SUB_IMAGE && q->item_num < 3) return Qz_Ng(why, cap, "選択肢が 3 つ未満");
		if (Qz_CorrectCount(q) < 1) return Qz_Ng(why, cap, "正解が無い");
		break;

	case QZ_KIND_ORDER:
		if (q->sub != QZ_SUB_IMAGE && q->item_num < 3) return Qz_Ng(why, cap, "選択肢が 3 つ未満");
		break;

	case QZ_KIND_GROUP:
		if (q->item_num < 3) return Qz_Ng(why, cap, "項目が 3 つ未満");
		for (i = 0; i < q->item_num; i++) if (q->sub_item[i] == NULL) return Qz_Ng(why, cap, "グループの名前が無い項目がある(group: 名前 = 項目)");   // (2026-10-14)
		if (q->group_num < 2 || q->group_num > 3) return Qz_Ng(why, cap, "グループが 2 つか 3 つでない");
		break;

	case QZ_KIND_HAYAI:
		if (q->item_num < 6) return Qz_Ng(why, cap, "選択肢が 6 つ未満");   // (2026-10-14)画面は 6 択か 8 択(当時と同じ。前は 3 つ以上にしていた)
		//if (q->item_num < 3) return Qz_Ng(why, cap, "選択肢が 3 つ未満");   // 前の形
		if (Qz_CorrectCount(q) < 1) return Qz_Ng(why, cap, "正解が無い");
		break;
	}
	return 1;
}
