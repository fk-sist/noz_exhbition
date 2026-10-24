#ifndef QZ_QUESTION_H
#define QZ_QUESTION_H

// =====================================================
// クイズの芯:1 問の形(2026-10-14 作り直し。docs/rebuild.md)
//   問題パック(.qz。形式は docs/quiz_pack.md)の 1 問を、形式ごとに型の付いた形で持つ。
//   当時の Problem(8 列の文字列を、形式ごとに違う意味で読む)の代わり。形式は strcmp ではなく QZ_KIND_～ の番号で見る。
//   ここは DXLib を使わない(Linux でもビルドして試せるように)。
//
//   文字列(問題文・選択肢など)は、パックの置き場(QzPack の buf)を指すだけ。パックを捨てたら使えない。
//   文字は CP932 のまま(ゲームが Shift_JIS で描くので)。1 字ずつ数える時は Qz_CharBytes / Qz_CharCount。
//
//   形式ごとに使う所:
//     ○×         answer_no(0 = ○、1 = ×)。画像 2 択(sub = QZ_SUB_IMAGE)は image に左右 2 枚の絵、answer_no(0 = A(左)、1 = B(右))
//     四択        item[] と correct[]、answer_no(正解の番号。0 から)。問題の絵は image。
//                 選択肢が絵(sub = QZ_SUB_IMAGE)は image2 に 4 枚並べた絵、item は無し、answer_no
//     連想        hint[](ヒントの行)、hint_order、item[] と correct[]、answer_no。ヒントが絵なら image(4 分割)。
//                 選択肢が絵(sub = QZ_SUB_IMAGE)は四択と同じく image2
//     並べ替え    answer[0](答えの文。札は 1 字ずつにばらして作る)。問題の絵は image
//     スロット    item[](リール 1 本ずつの字を詰めた物。例 "アセラネ")、answer[0]。リール i の字に answer[0] の i 字目が入っている
//     文字パネル  item[](パネル 1 枚 1 字)、answer[0]
//     タイピング  input、answer[](どれでも正解)。問題の絵は image
//     ビジュアル  sub で 3 つ:
//                   QZ_SUB_CUBE   キューブ。input、answer[0](面は字数で決まる)
//                   QZ_SUB_TOUCH  画像タッチ。image(絵)、image2(判定の色の絵。当時の -a.png)、answer_no(色の番号)
//                   QZ_SUB_EFFECT エフェクト。word(文字を崩す)か image(絵を崩す)、input、answer[]
//     線結び      item[](左)と sub_item[](右。同じ番号が正しい相手)、show。左が絵(sub = QZ_SUB_IMAGE)なら image(3 分割)で item は "1" "2" "3"
//     一問多答    item[] と correct[]、show。絵(sub = QZ_SUB_IMAGE)なら image(4 分割)と correct[](item は空の文字が 4 つ)
//     順番当て    item[](正しい順に)、show。絵(sub = QZ_SUB_IMAGE)なら image(4 分割。正しい順)、item は空の文字が 4 つ
//     グループ分け item[](項目)と sub_item[](項目のグループの名前)、group_num、show(QZ_SHOW_ALL で全部出す)
//     早いもの勝ち item[] と correct[]
//   共通:id、level(1～5)、icon(0～3)、chapter、text[](問題文の行。@ は分岐。当時のまま)、note[](解説の行)、
//         quiz_group(同類の問題の印。同じ印の問題は、どれか 1 つ出したらその回はもう出さない。2026-10-18)
// =====================================================

#define QZ_ID_LEN          32
#define QZ_TEXT_LINE_MAX   8      // 問題文の行の数の上限(データの最大は 8)
#define QZ_HINT_MAX        8      // 連想のヒントの数の上限(データの最大は 4)
#define QZ_ITEM_MAX        32     // 選択肢・項目・リール・パネルの数の上限(データの最大は 19)
#define QZ_ANSWER_MAX      32     // 文字の答えの数の上限(タイピングの最大は 25)
#define QZ_NOTE_LINE_MAX   8      // 解説の行の数の上限

// 形式(並びは当時の servingquiz の番号 - 1。検定の配点の表の行と同じ。グループ分け・早いもの勝ちは後ろへ足した)
#define QZ_KIND_NONE       (-1)
#define QZ_KIND_OX         0      // ○×
#define QZ_KIND_CHOICE     1      // 四択
#define QZ_KIND_RENSO      2      // 連想
#define QZ_KIND_NARA       3      // 並べ替え
#define QZ_KIND_REEL       4      // スロット
#define QZ_KIND_MOJI       5      // 文字パネル
#define QZ_KIND_TYPE       6      // タイピング
#define QZ_KIND_VISUAL     7      // ビジュアル
#define QZ_KIND_LINE       8      // 線結び
#define QZ_KIND_MULTI      9      // 一問多答
#define QZ_KIND_ORDER      10     // 順番当て
#define QZ_KIND_GROUP      11     // グループ分け
#define QZ_KIND_HAYAI      12     // 早いもの勝ち
#define QZ_KIND_NUM        13

// 形式の中の種類
#define QZ_SUB_NORMAL      0
#define QZ_SUB_IMAGE       1      // 絵の問題(○× の画像 2 択、四択の絵の選択肢、一問多答・順番当て・線結びの絵)
#define QZ_SUB_CUBE        2      // ビジュアル:キューブ
#define QZ_SUB_TOUCH       3      // ビジュアル:画像タッチ
#define QZ_SUB_EFFECT      4      // ビジュアル:エフェクト(回転・ズーム・パズル・モザイク。どれにするかは出題の時に決める)

// 入力(タイピング・ビジュアル。.qz の input:)
#define QZ_INPUT_NONE      0
#define QZ_INPUT_HIRA      1      // ひら
#define QZ_INPUT_KATA      2      // カタ
#define QZ_INPUT_EISU      3      // 英数

// 連想のヒントの出し方(.qz の hint_order:。当時の列 6。当時は効いていなかった)
#define QZ_HINT_FIXED      0      // 書いた順(書かなければ、これ)
#define QZ_HINT_KEEP_FIRST 1      // 1 つ目は先頭のまま、残りを混ぜる
#define QZ_HINT_SHUFFLE    2      // 全部混ぜる

#define QZ_SHOW_ALL        99     // show: all(グループ分けで全部出す)

typedef struct {
	char id[QZ_ID_LEN];          // 問題の ID(パックの中で一意。成績はこれに紐づく)
	int  kind;                   // 形式(QZ_KIND_～)
	int  sub;                    // 形式の中の種類(QZ_SUB_～)
	int  level;                  // 難易度 1～5
	int  icon;                   // 出題中のアイコン 0～3(当時の列 4)
	int  chapter;                // 章(パックの [chapter N]。無ければ 0)
	int  input;                  // 入力(QZ_INPUT_～)
	int  show;                   // 出す数(3 か 4。QZ_SHOW_ALL。書いていなければ 0)
	int  hint_order;             // 連想のヒントの出し方(QZ_HINT_～)

	const char* text[QZ_TEXT_LINE_MAX];   // 問題文の行
	int  text_num;
	const char* hint[QZ_HINT_MAX];        // 連想のヒント
	int  hint_num;
	const char* item[QZ_ITEM_MAX];        // 選択肢・項目(形式ごとの中身は上の説明)
	const char* sub_item[QZ_ITEM_MAX];    // 線結びの右、グループ分けのグループの名前(item と同じ番号)
	char correct[QZ_ITEM_MAX];            // 選択肢が正解か(四択・連想・一問多答・早いもの勝ち)
	int  item_num;
	int  group_num;                       // グループ分けのグループの数(2 か 3)
	const char* answer[QZ_ANSWER_MAX];    // 文字の答え
	int  answer_num;
	int  answer_no;                       // 番号の答え(0 から。○× は 0 = ○ / A、1 = × / B)。無ければ -1

	const char* image;           // 問題の絵(形式ごとの意味は上の説明。無ければ NULL)
	const char* image2;          // 2 枚目の絵(画像タッチの判定の色、四択・連想の絵の選択肢)
	const char* word;            // エフェクトで崩す文字
	const char* quiz_group;      // 同類の問題の印(.qz の quiz_group:。分岐の前が同じ物など)。無ければ NULL
	const char* note[QZ_NOTE_LINE_MAX];   // 解説の行(答え合わせの後に出す)
	int  note_num;
} QzQuestion;

// 形式の名前(.qz の type: と同じ書き方。ビジュアルは "ビジュアル")
const char* Qz_KindName(int kind);
// .qz の type: の名前から、形式と種類。分からなければ 0(kind は QZ_KIND_NONE)
int  Qz_KindFind(const char* type, int* kind, int* sub);
// 遊べる問題か。遊べなければ 0 を返し、why に理由を書く(why は NULL でもいい)
int  Qz_Check(const QzQuestion* q, char* why, int cap);
// 正解の選択肢の数(correct[] の 1 の数)
int  Qz_CorrectCount(const QzQuestion* q);

// CP932 の文字
int  Qz_CharBytes(const char* s);          // 頭の 1 字のバイト数(1 か 2。空なら 0)
int  Qz_CharCount(const char* s);          // 字の数
int  Qz_CharAt(const char* s, int n, char* out, int cap);   // n 字目(0 から)を out へ。無ければ 0

// 問題文の @ (2026-10-22 fumiya:エスケープを決める)
//   @ は問題文の待ちの印。字としての @ を出したい時は \@ と書く(Verilog の @(posedge clk)、Python のデコレータなど)。
//   \ は CP932 の 2 バイト文字の 2 バイト目(ソ・表など)にも出るので、必ず 1 字ずつ進めて見る
//   決まり(2026-10-22 fumiya:\ もそれとして使うので、細かく決める):
//     ・\ は、ふつうは字としてそのまま出る(\n・\0・\\ も書いたとおり。C のコードの話で使う)
//     ・\@ だけが特別で、字としての @ になる(待ちにも分岐にも数えない)
//     ・字としての \ の直後に待ちの @ を置きたい時は \\@ と書く(\ を 1 字出して、@ は待ち)
#define QZ_AT_ESC '\\'
int  Qz_IsAtEsc(const char* s);            // s が \@ の頭か(1 字目の位置で呼ぶこと)
int  Qz_IsBsAt(const char* s);             // s が \\@ の頭か(\ を 1 字出して、@ は待ち)
int  Qz_AtCount(const char* s);            // 待ちの印の @ の数(\@ は数えない)

#endif
