#ifndef QZ_PACK_H
#define QZ_PACK_H

#include <stdio.h>
#include "qz_question.h"

// =====================================================
// クイズの芯:問題パック(.qz)の読み込み(2026-10-14 作り直し。docs/rebuild.md)
//   形式は docs/quiz_pack.md。1 ファイル = 1 パック(1 科目・1 ジャンル)。
//   読んだ問題は QzQuestion(qz_question.h)の並び。当時の Problem には直さない。
//
//   使い方:
//     QzPack pk;
//     QzPack_Load(&pk, "./quiz/packs/rikei.qz", log);   // log は読めなかった問題の理由を書く先(NULL なら書かない)
//     for (i = 0; i < pk.q_num; i++) { const QzQuestion* q = &pk.q[i]; … }
//     QzPack_Free(&pk);
//
//   ファイルの中身は丸ごと buf に読み、問題の文字列はそこを指す(行の終わりを '\0' にして、その場で切る)。
//   遊べない問題(Qz_Check に通らない物)は、並びに入れない(skip_num に数え、log に ID と理由を書く)。
//   グローバル変数は持たない。読んだパックは呼んだ側が持つ。
// =====================================================

#define QZ_PACK_TITLE_LEN   64
#define QZ_PACK_CHAPTER_MAX 16

typedef struct {
	char id[QZ_ID_LEN];          // パックの ID([pack] の id:。成績のファイルの名前・サーバーとの照合に使う)
	char title[QZ_PACK_TITLE_LEN];
	char version[32];
	char path[256];              // 読んだファイル
	int  chapter_num;            // 章の数(一番大きい [chapter N] の N)
	char chapter_title[QZ_PACK_CHAPTER_MAX][QZ_PACK_TITLE_LEN];   // 章の名前([chapter N] の title:。N - 1 番目)

	QzQuestion* q;               // 問題の並び(ファイルに書いた順)
	int  q_num;
	int  skip_num;               // 読めなかった・遊べない問題の数
	int  over_num;               // 上限(QZ_～_MAX)を超えて、切り捨てた所の数(0 でなければ上限を見直す)

	char* buf;                   // ファイルの中身(問題の文字列の置き場)

	char* fix_buf;               // 手直し(quiz/fix/<ID>.qz)の中身。差し替えた問題の文字列の置き場(QzPack_Fix。無ければ NULL)
	int  fix_num;                // 手直しで差し替えた問題の数(足した問題も数える)
	int  add_num;                // そのうち、手直しのファイルで足した新しい問題の数(2026-10-21。並びの後ろにある)

	char** fix_more;             // ゲームの中で直した問題の文字列の置き場(QzPack_FixText。直すたびに 1 つ増える。2026-10-21)
	int  fix_more_num;           //   前の文字列は、出題中の問題が指しているかもしれないので、パックを捨てるまで捨てない
	char why[128];               // 最後に遊べないとした問題の理由(Qz_Check の理由。QzPack_LoadText で 1 問だけ確かめる時に見る)
} QzPack;

int  QzPack_Load(QzPack* pk, const char* path, FILE* log);   // 戻り値:問題の数(開けなければ -1)
void QzPack_Free(QzPack* pk);
// 関数の役割:手直しのファイル(fumiya が直した問題。形式は .qz と同じ)を読んで、同じ ID の問題を差し替える(2026-10-18)
//   戻り値:差し替えた数(ファイルが無ければ 0。手直しが無いのが普通)。元のパックに無い ID・遊べない問題は差し替えず、log に書く
//   (2026-10-21)元のパックに無い ID でも、chapter: を書いた問題は新しい問題として並びの後ろに足す(add_num。戻り値にも数える)
//   注意:QzPack_Load の後に 1 回だけ。2 回目は何もしないで -1(前の差し替えの文字列が消えるので)
int  QzPack_Fix(QzPack* pk, const char* path, FILE* log);
const QzQuestion* QzPack_Find(const QzPack* pk, const char* id);   // ID で引く(無ければ NULL)
// 関数の役割:ファイルではなく、文字列(.qz の書き方)から読む(2026-10-21。ゲームの中で直した 1 問を確かめる時に)。text は写して持つ
//   戻り値は QzPack_Load と同じ。遊べなかった問題の理由は pk->why
int  QzPack_LoadText(QzPack* pk, const char* text, FILE* log);
// 関数の役割:文字列(.qz の書き方。[q ID] のブロック 1 つ以上)の問題で、同じ ID の問題を差し替える(2026-10-21。ゲームの中で直した時)
//   QzPack_Fix の後に何度呼んでもよい。章は QzPack_Fix と同じく、ブロックに [chapter N] が無ければ元のまま。
//   戻り値:差し替えた数。遊べない問題は差し替えない(理由は pk->why)。新しい ID は足さない(起動し直すと QzPack_Fix が足す)
int  QzPack_FixText(QzPack* pk, const char* text, FILE* log);

#endif
