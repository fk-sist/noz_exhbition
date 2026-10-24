#ifndef GAME_GENRE_H
#define GAME_GENRE_H

// =====================================================
// ミニゲームの分類（全画面共通）
//   ジャンル：何の力を使うゲームか（1ゲームに1つ）
//   形式    ：どういう形で戦うか（1ゲームに1つ。今は表示だけで、チーム分けなどの仕組みはまだ無い）
//
//   決め方：gamelist.txt の [game] に、1行ずつ書く（書いていないゲームは、ジャンル無し・個人戦）
//       genre  = timing      ← timing / action / reflex / memory / brain / mash / sense / bet
//       format = solo        ← solo / team / 1vs3 / coop
//   読んだ結果は GameEntry の genre と format に入る（GetGameEntryByID などで取れる）
//
//   描く（どれも DXライブラリの図形で描く。画像は使わない）:
//       GameGenre_DrawIcon(cx, cy, r, genre, alpha);            丸いアイコンだけ（狭い所用）
//       GameGenre_DrawTags(x, cy, h, genre, format, alpha);     形式の札と、「アイコン＋名前」のジャンルの札を、この順で横に並べる。戻り値は幅
//       GameGenre_TagsWidth(h, genre, format);                  上の幅だけ知りたい時（真ん中寄せ・右寄せ用）
//   ジャンル無しの時はジャンルの札を、個人戦の時は形式の札を出さない（幅は 0）
//
//   ジャンルや形式を増やす時は、下の番号と、gameGenre.cpp の表（名前・キー・色）と、アイコンの絵を足す
// =====================================================

// ジャンル
#define GENRE_NONE    0   // 決めていない
#define GENRE_TIMING  1   // タイミング
#define GENRE_ACTION  2   // アクション
#define GENRE_REFLEX  3   // 反射
#define GENRE_MEMORY  4   // 記憶
#define GENRE_BRAIN   5   // 知能
//#define GENRE_NUM     6
#define GENRE_MASH    6   // 連打（考えずに、押す速さで押し切る）
#define GENRE_SENSE   7   // 感覚（正解が画面に出ていない所を、勘で当てる。体内時計・目分量など）
//#define GENRE_NUM     8
#define GENRE_BET     8   // 賭け（自分の点を張って、増やすか減らすか）
#define GENRE_NUM     9

// 形式
#define FORMAT_SOLO   0   // 個人戦（全員が敵）
#define FORMAT_TEAM   1   // タッグ（2対2）
#define FORMAT_1VS3   2   // 変則（1人対3人）
#define FORMAT_COOP   3   // 協力（全員で1つの目標）
#define FORMAT_NUM    4

int  GameGenre_FromKey(const char* key);     // gamelist.txt に書いた文字からジャンルの番号へ（知らない文字は GENRE_NONE）
int  GameFormat_FromKey(const char* key);    // 同じく、形式の番号へ（知らない文字は FORMAT_SOLO）
const char* GameGenre_Name(int genre);       // 画面に出す名前（「タイミング」など）
const char* GameFormat_Name(int format);
unsigned int GameGenre_Color(int genre);     // ジャンルの色

void GameGenre_DrawIcon(int cx, int cy, int r, int genre, int alpha);
int  GameGenre_DrawTags(int x, int cy, int h, int genre, int format, int alpha);
int  GameGenre_TagsWidth(int h, int genre, int format);
int  GameGenre_DrawTagsCompact(int x, int cy, int h, int genre, int format, int alpha);   // 詰めた形：形式の札が名前だけ（人数の丸なし）。幅の狭い所用
int  GameGenre_TagsWidthCompact(int h, int genre, int format);

#endif
