#define _CRT_SECURE_NO_WARNINGS
#include "DxLib.h"

#ifndef ENV_LOADED_H
#include "env_value.h"
#endif
#include "resource_manager.h"
#include "demo.h"
#include "qz_serve.h"   // Qz_SetRand(クイズの芯の乱数を DXLib の乱数に)

// =====================================================
// 展示用のデモ巡回(noz_exhbition。2026-10-24 大学祭の入口のモニター用)
//   起動したら全画面で、data/demos.txt のデモを順番に流し続ける(demo.h)。操作は Esc で終了するだけ
//   放っておいてもモニターが消えたりスリープしたりしないように、Windows に知らせておく(SetThreadExecutionState)
// =====================================================

#define FRAME_US        (1000000 / 60)   // 1 フレームの長さ(マイクロ秒。60Hz より速い画面でも 60fps にそろえる。quiz_academy の frame_rate と同じ待ち方)
#define FRAME_RESET     4                // これ以上遅れたら、遅れを取り戻そうとせず、今を基準にし直す
#define FRAME_SLEEP_US  2000             // 残りがこれより長い間は Sleep(1) で待つ

int Fonts[FONT_MAX];
int ScreenCenter_X = SCREEN_WIDTH / 2;
int ScreenCenter_Y = SCREEN_HEIGHT / 2;
ResourceManager resMgr;

// 組み込みのフォント(Windows に入れずに、このプロセスでだけ使う)
static const char* embedFonts[] = {
	"./noz/fonts/togalite.otf",              // トガリテ Heavy(ノズキン)
	"./qma/fon/DF-HeiSeiGothic-W7.ttc",      // ＤＦＰ平成ゴシック体W7(クイズ OS×UNIX)
	"./qma/fon/DF-HeiSeiGothic-W9.ttc",      // ＤＦＰ平成ゴシック体W9
	"./qma/fon/DF-KakuPop-W7.ttc",           // ＤＦ角POPW7
	"./qma/fon/DF-ReiGaSou-W9.ttc",          // ＤＦ麗雅宋
};
#define EMBED_FONT_NUM ((int)(sizeof(embedFonts) / sizeof(embedFonts[0])))

// 関数の役割:書き換えて、60fps になるように待つ
static void ScreenFlip60(void) {
	static LONGLONG next = 0;
	LONGLONG now;
	ScreenFlip();
	now = GetNowHiPerformanceCount();
	if (next == 0 || now - next > (LONGLONG)FRAME_US * FRAME_RESET) next = now;
	next += FRAME_US;
	while ((now = GetNowHiPerformanceCount()) < next) {
		if (next - now > FRAME_SLEEP_US) Sleep(1);
	}
}

// 関数の役割:フォント(noz_kingdom の Fonts[] と同じ番号の意味。大きい字はキャッシュを少なくしてから作る)
static void InitFonts(void) {
	int i;
	for (i = 0; i < EMBED_FONT_NUM; i++) AddFontResourceExA(embedFonts[i], FR_PRIVATE, NULL);
	SetFontCacheCharNum(256);
	Fonts[0] = CreateFontToHandle(NULL, 48, 3);
	Fonts[1] = CreateFontToHandle(NULL, 32, 3);
	Fonts[3] = CreateFontToHandle("トガリテ Heavy", 24, 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);
	Fonts[5] = CreateFontToHandle("トガリテ Heavy", 48, 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);
	Fonts[6] = CreateFontToHandle("トガリテ Heavy", 32, 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);
	Fonts[7] = CreateFontToHandle("トガリテ Heavy", 18, 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);
	SetFontCacheCharNum(48);   // 60px を超える字は、キャッシュを少なく(quiz_academy の CLAUDE.md)
	Fonts[2] = CreateFontToHandle(NULL, 72, 3);
	Fonts[4] = CreateFontToHandle("トガリテ Heavy", 72, 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);
	Fonts[8] = CreateFontToHandle("トガリテ Heavy", 96, 3, DX_FONTTYPE_ANTIALIASING_EDGE_8X8);
	SetFontCacheCharNum(256);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	SetOutApplicationLogValidFlag(FALSE);
	SetGraphMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32);
	ChangeWindowMode(GAME_WINDOW_MODE ? TRUE : FALSE);
	SetWindowText(GAME_TITLE);
	SetAlwaysRunFlag(TRUE);
	if (DxLib_Init() == -1) return -1;

	SetDrawScreen(DX_SCREEN_BACK);
	SetMouseDispFlag(FALSE);   // マウスのカーソルを消す
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED);   // 放っておいてもモニターが消えない・スリープしない
	Qz_SetRand(GetRand);
	InitFonts();
	Director_Init();

	while (ProcessMessage() == 0) {
		if (CheckHitKey(KEY_INPUT_ESCAPE)) break;   // Esc で終了
		ClearDrawScreen();
		Director_Update();
		Director_Draw();
		ScreenFlip60();
	}

	Director_End();
	resMgr.UnloadAll();
	SetThreadExecutionState(ES_CONTINUOUS);   // 元に戻す
	DxLib_End();
	return 0;
}
