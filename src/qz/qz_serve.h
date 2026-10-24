#ifndef QZ_SERVE_H
#define QZ_SERVE_H

#include "qz_question.h"

// =====================================================
// クイズの芯:出題の準備(2026-10-14 作り直し。docs/rebuild.md)
//   1 問を出す時の形(どの選択肢を、どの並びで出すか。どれが正解か)を決める。当時の servequiz の、絵を使わない所。
//   並べ方(入れ替えの回数・動かさない所・出す数)は、当時の servequiz と同じ(quiz_scene.cpp の Qs_Prepare～ を写した)。
//   絵(4 分割の 1 枚ずつなど)は、src[] の番号で「問題の絵の何枚目か」を渡す。切るのは画面の側。
//
//   乱数は Qz_Rand(当時の GetRand と同じく 0～max)。ゲームは起動時に Qz_SetRand(GetRand) で DXLib の乱数をつなぐ。
//   つながなければ C の rand()(Linux のテストはこっち。srand で同じ並びを作れる)。
//
//   形式ごとの中身:
//     ○×          item[0..1](○ ×。画像 2 択は Ａ Ｂ)、truth[](正解の方が 1)。画像 2 択は src[](0 = 絵の左半分、1 = 右半分)
//     四択・連想    item[](出す順。4 つまで)、src[](問題の item の何番目か)、truth[](正解が 1)。絵の選択肢は item が空の文字、src が絵の何枚目か
//     一問多答      item[]、src[]、truth[](正解が 1)。絵なら item は空の文字、src が絵の何枚目か
//     順番当て      item[]、src[]、truth[](正しい順位。1 から)
//     線結び        item[](左)、src[](左の絵の何枚目か)、right[](右。出す順)、truth[](左 i の正しい相手の右の番号。0 から)
//     グループ分け  item[]、truth[](正しいグループ。0 = A、1 = B、2 = C。2 組の時は 0 か 2。当時と同じ)、group_name[3]、group_num
//     早いもの勝ち  item[](6 か 8)、truth[](正解が 1)、true_num、hayai_mea
//     並べ替え      word[](札。答えを 1 字ずつにばらして混ぜた物)、word_num
//     スロット      reel[][4](リールの字。当時の環の順)、reel_num、reel_start[](最初に真ん中にある字の番目)
//     文字パネル    item[](パネル。出す順)
//     タイピング・ビジュアル  並べる物は無い(答えは問題の answer[])
// =====================================================

#define QZ_SERVE_MAX     16     // 出す物の数の上限(早いもの勝ち 8、文字パネル 10、並べ替えの札は答えの字数。データの最大は 12)
#define QZ_REEL_MAX      8      // スロットのリールの数の上限(データは 2～6)
#define QZ_CHAR_LEN      4      // 1 字(CP932 の 2 バイト + '\0')

typedef struct {
	const QzQuestion* q;
	int  item_num;                        // 出す数
	const char* item[QZ_SERVE_MAX];       // 出す順の選択肢・項目・パネル・線結びの左
	int  src[QZ_SERVE_MAX];               // それぞれが、問題の item(絵なら絵の何枚目)の何番目か
	int  truth[QZ_SERVE_MAX];             // 正解(形式ごとの意味は上の説明)
	const char* right[QZ_SERVE_MAX];      // 線結びの右(出す順)
	int  group_num;                       // グループ分け:グループの数(2 か 3)
	const char* group_name[3];            // グループ分け:A・B・C の名前(2 組の時は [0] と [2]。[1] は空)
	int  true_num;                        // 早いもの勝ち:正解の数(定員の表に使う。当時の howmanyans)
	int  hayai_mea;                       // 早いもの勝ち:問題の 1 番目の選択肢が、今どこにあるか(COM の作戦に使う。当時の mea)
	char word[QZ_SERVE_MAX][QZ_CHAR_LEN]; // 並べ替えの札
	int  word_num;
	char reel[QZ_REEL_MAX][4][QZ_CHAR_LEN];   // スロットのリールの字
	int  reel_num;
	int  reel_start[QZ_REEL_MAX];         // スロット:最初に真ん中にある字(reel[j][番目])
	int  bunki;                           // 問題文に分岐(@)があって、得点の猶予が伸びる問題か(当時の bunkiflag。Qz_HasBunki)
} QzServe;

void Qz_SetRand(int (*rnd)(int max));   // 乱数をつなぐ(DXLib の GetRand など。0～max を返す物)
int  Qz_Rand(int max);                  // 0～max
// 関数の役割:1 問を出す形にする。戻り値:出せれば 1
int  Qz_Serve(const QzQuestion* q, QzServe* s);
// 問題文に分岐(@)があって、得点の猶予が伸びる問題か(当時:@ があって、列 6 が 0 の時)
int  Qz_HasBunki(const QzQuestion* q);

// ---- 種から決まる乱数(2026-10-20。docs/rebuild.md の 8 の C) ----
//   通信対戦では、同じ問題を全員が同じ並び(選択肢の順・パネル・リール・グループの項目)で見ないと、早いもの勝ちの定員や「Ａ を押した」が食い違う。
//   そこで出題の形は、進行役が配る種(seed)だけから決める。種が同じなら、どの端末でも同じ形になる(Windows でも Linux でも同じ計算)。
//   ゲームの他の乱数(GetRand。演出・COM の判断)とは別の流れなので、演出で乱数を何回呼んでも、出題の形はずれない。
//   種は問題を並べる時に決める(game/lesson の seed[])。一人で遊ぶ時も同じ道を通る(見た目は今まで通り、毎回違う並び)
typedef struct {
	unsigned int x;                       // 中の状態(xorshift32。0 にはしない)
} QzRng;
void QzRng_Init(QzRng* r, unsigned int seed);
int  QzRng_Next(QzRng* r, int max);     // 0～max(Qz_Rand と同じ形)
int  Qz_ServeSeed(const QzQuestion* q, QzServe* s, unsigned int seed);   // 種から決まる Qz_Serve(中で使う Qz_Rand を、この間だけ種の乱数にする)
unsigned int Qz_NewSeed(void);          // 新しい種(Qz_Rand から作る。問題を並べる時に 1 問に 1 つ)

#endif
