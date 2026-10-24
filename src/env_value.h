#ifndef ENV_LOADED_H
#define ENV_LOADED_H

// =====================================================
// 展示用のデモ巡回(noz_exhbition)で、全体で使う決まった値(2026-10-24)
//   ノズキン(noz_kingdom)・クイズ OS×UNIX(quiz_academy)から写した部品も、このファイルを読む
//   (gameGenre.cpp は noz_kingdom の env_value.h の Fonts[] を、effect_open.cpp は quiz_academy の env_value.h を読んでいた)
// =====================================================

#define GAME_TITLE "NOZ GAMES EXHIBITION"   // ウィンドウの名前

#define SCREEN_WIDTH  (1920)
#define SCREEN_HEIGHT (1080)

// quiz_academy から写した部品(effect_open など)が使う名前
#define XSIZE SCREEN_WIDTH
#define YSIZE SCREEN_HEIGHT

#define GAME_WINDOW_MODE 0          // 1 でウィンドウ、0 で全画面。確かめる時に手元で替える

// フォント(main.cpp の起動時に作って使い回す。番号は noz_kingdom の Fonts[] と同じ意味にしてある。gameGenre.cpp が Fonts[3] / Fonts[7] を使うため)
//   0～2 : Windows の標準のフォント(48 / 32 / 72)
//   3～8 : トガリテ Heavy(24 / 72 / 48 / 32 / 18 / 96)
#define FONT_MAX 32
extern int Fonts[FONT_MAX];

extern int ScreenCenter_X, ScreenCenter_Y;

#endif
