#pragma once
#include <cstdio>
#include <cstddef>
#define TRUE 1
#define FALSE 0
#define WINAPI
#define MB_OK 0
typedef void* HINSTANCE; typedef char* LPSTR; typedef long long LONGLONG;
struct VECTOR { float x, y, z; };
#define DX_INPUT_KEY_PAD1 0x1001
#define PAD_INPUT_DOWN 1
#define PAD_INPUT_LEFT 2
#define PAD_INPUT_RIGHT 4
#define PAD_INPUT_UP 8
#define PAD_INPUT_1 16
#define PAD_INPUT_2 32
#define PAD_INPUT_3 64
#define PAD_INPUT_4 128
#define PAD_INPUT_5 256
#define PAD_INPUT_6 512
#define PAD_INPUT_7 1024
#define PAD_INPUT_8 2048
#define PAD_INPUT_9 4096
#define PAD_INPUT_10 8192
#define DX_BLENDMODE_ALPHA 1
#define DX_BLENDMODE_NOBLEND 0
#define DX_SCREEN_BACK -2
#define DX_DIRECT3D_11 3
#define DX_FONTTYPE_ANTIALIASING_EDGE_8X8 5
#define KEY_INPUT_ESCAPE 1
extern int g_lastAlpha;
int MessageBox(void*, const char*, const char*, int);
int SetDrawBlendMode(int, int); int DrawBox(int,int,int,int,unsigned int,int); unsigned int GetColor(int,int,int);
int GetJoypadInputState(int); int SetGraphMode(int,int,int); int ChangeWindowMode(int); int SetWindowText(const char*);
int SetAlwaysRunFlag(int); int SetUseDirect3DVersion(int); int DxLib_Init(void); int DxLib_End(void); int ProcessMessage(void);
int CheckHitKey(int); int ClearDrawScreen(void); int ScreenFlip(void); int SetDrawScreen(int); int SetUseZBuffer3D(int); int SetWriteZBuffer3D(int);
int CreateFontToHandle(const char*, int, int, int = -1, int CharSet = -1, int EdgeSize = -1, int Italic = FALSE, int Handle = -1); /* (2026-10-14 引数を本物に合わせた) */ int DrawStringToHandle(int,int,const char*,unsigned int,int,unsigned int=0);
int DrawFormatStringToHandle(int,int,unsigned int,int,const char*, ...);
int DrawFormatString(int x, int y, unsigned int Color, const char* FormatString, ...);
LONGLONG GetNowHiPerformanceCount(void); int WaitTimer(int);
int sprintf_s(char*, const char*, ...);
VECTOR VGet(float,float,float); VECTOR ConvScreenPosToWorldPos(VECTOR);
#include <cstdlib>
#include <cmath>
#define DX_PI_F 3.14159265f
int LoadGraph(const char*); int LoadDivGraph(const char*,int,int,int,int,int,int*); int LoadSoundMem(const char*); int DeleteGraph(int); int DeleteSoundMem(int);
int DrawGraph(int,int,int,int); int DrawRotaGraph(int,int,double,double,int,int,int=0);
int GetRand(int);
int SetCameraNearFar(float,float); int SetupCamera_Perspective(float); int SetCameraPositionAndTarget_UpVecY(VECTOR,VECTOR);
int DrawLine3D(VECTOR,VECTOR,unsigned int); int DrawTriangle3D(VECTOR,VECTOR,VECTOR,unsigned int,int);
int DrawSphere3D(VECTOR,float,int,unsigned int,unsigned int,int); int DrawCapsule3D(VECTOR,VECTOR,float,int,unsigned int,unsigned int,int);
int DrawCone3D(VECTOR,VECTOR,float,int,unsigned int,unsigned int,int); VECTOR ConvWorldPosToScreenPos(VECTOR);
#define DX_DRAWMODE_NEAREST 0
#define DX_DRAWMODE_BILINEAR 1
int SetDrawMode(int);
int GetDrawStringWidthToHandle(const char*, int, int);
int SetDrawBright(int,int,int);
int DrawLine(int,int,int,int,unsigned int,int=1);
#define DX_BLENDMODE_ADD 2
int GetGraphSize(int,int*,int*);
int GetNowCount(int UseRDTSCFlag=FALSE);
int SetApplicationLogSaveDirectory(const char *DirectoryPath);   // Log.txt の置き場所(DxLib_Init の前。ref/dxlib/DxLib.h で確かめた。2026-10-14)
int DrawRectExtendGraph(int,int,int,int,int,int,int,int,int,int);

int DrawLineAA(float,float,float,float,unsigned int,float Thickness=1.0f);
int DrawCircleAA(float,float,float,int,unsigned int,int FillFlag=TRUE,float LineThickness=1.0f);
int DrawBoxAA(float,float,float,float,unsigned int,int,float LineThickness=1.0f);   // (2026-10-13。ref/dxlib/DxLib.h で確かめた)
int DrawOvalAA(float,float,float,float,int,unsigned int,int,float LineThickness=1.0f);
int DrawQuadrangleAA(float,float,float,float,float,float,float,float,unsigned int,int,float LineThickness=1.0f);
int DrawRoundRectAA(float,float,float,float,float,float,int,unsigned int,int,float LineThickness=1.0f);
int DrawTriangleAA(float,float,float,float,float,float,unsigned int,int,float LineThickness=1.0f);
int DrawExtendGraph(int,int,int,int,int,int);
int SetDrawArea(int,int,int,int);
int SetDrawAreaFull(void);
struct COLOR_U8 { unsigned char b, g, r, a; };
struct VERTEX3D { VECTOR pos; VECTOR norm; COLOR_U8 dif; COLOR_U8 spc; float u, v; float su, sv; };
struct VERTEX2D { VECTOR pos; float rhw; COLOR_U8 dif; float u, v; };
int DrawPolygon2D(const VERTEX2D*,int,int,int);
int MakeScreen(int,int,int UseAlphaChannel=FALSE);
int GetDrawScreenGraph(int,int,int,int,int,int UseClientFlag=TRUE);
int SaveDrawScreen(int,int,int,int,const char*);
int SaveDrawScreenToPNG(int,int,int,int,const char*);   // (2026-10-18。fumiya が DXLib の掲示板 no=4079 で確かめた。今の描画先を PNG で保存)   // (2026-10-18。公式の関数一覧の「マイナー関数」:今の描画先を BMP で保存。後ろの引数は使わないので書かない)
int GetGraphTextureSize(int,int*,int*);
COLOR_U8 GetColorU8(int,int,int,int);
int DrawPolygon3D(VERTEX3D*, int, int, int);
// 頂点を使い回して描く(2026-10-13。リファレンスの「DrawPolygonIndexed3D」で宣言を確かめた。頂点は 65536 個まで)
int DrawPolygonIndexed3D(VERTEX3D*, int, unsigned short*, int, int, int);
// 頂点バッファ(GPU に頂点を置いておく。2026-10-13。ref/dxlib/DxLib.h(3.24f)で宣言を確かめた)
#define DX_VERTEX_TYPE_NORMAL_3D (0)
#define DX_INDEX_TYPE_16BIT (0)
int CreateVertexBuffer(int VertexNum, int VertexType);
int DeleteVertexBuffer(int VertexBufHandle);
int SetVertexBufferData(int SetIndex, const void* VertexArray, int VertexNum, int VertexBufHandle);
int CreateIndexBuffer(int IndexNum, int IndexType);
int DeleteIndexBuffer(int IndexBufHandle);
int SetIndexBufferData(int SetIndex, const void* IndexArray, int IndexNum, int IndexBufHandle);
int DrawPolygon3D_UseVertexBuffer(int VertexBufHandle, int GrHandle, int TransFlag);
int DrawPolygonIndexed3D_UseVertexBuffer(int VertexBufHandle, int IndexBufHandle, int GrHandle, int TransFlag);
int SetUseLighting(int); int SetTextureAddressModeUV(int,int);
#define DX_TEXADDRESS_WRAP 1
#define DX_TEXADDRESS_CLAMP 3
#define DX_NONE_GRAPH (-5)

// Windows の関数(本物は windows.h。DxLib.h が取り込む)：フォントのファイルを、このプロセスの中だけで使えるようにする
#define FR_PRIVATE 0x10
int RemoveFontResourceExA(const char*, unsigned long, void*);
// 右スティック(2026-10-12。XInput のパッドだけ。リファレンスの「GetJoypadXInputState」)
#define DX_INPUT_PAD1 0x0001
#define DX_INPUT_KEY 0x1000
// 最後に触ったのがキーボードかパッドか(2026-10-13。ref/dxlib/DxLib.h で確かめた)
#define DX_CHECKINPUT_KEY 0x0001
#define DX_CHECKINPUT_PAD 0x0002
int CheckHitKeyAll(int CheckType);
int GetJoypadNum(void);
#define KEY_INPUT_Q 0x10
#define KEY_INPUT_W 0x11
#define KEY_INPUT_E 0x12
#define KEY_INPUT_R 0x13
#define KEY_INPUT_F2 0x3C
#define KEY_INPUT_F3 0x3D
#define KEY_INPUT_F4 0x3E
#define KEY_INPUT_F1 0x3B   // (2026-10-13。ref/dxlib/DxLib.h で確かめた)
float GetFPS(void);
struct XINPUT_STATE { unsigned char Buttons[16]; unsigned char LeftTrigger, RightTrigger; short ThumbLX, ThumbLY, ThumbRX, ThumbRY; };
int GetJoypadXInputState(int, XINPUT_STATE*);
// フォグ(2026-10-12。名前はリファレンスの一覧で確かめた。引数は、確かめていない)
int SetFogEnable(int); int SetFogColor(int,int,int); int SetFogStartEnd(float,float);
// 音(2026-10-12。リファレンスの「音利用関数」で確かめた)
#define DX_PLAYTYPE_NORMAL 0
#define DX_PLAYTYPE_BACK 1
#define DX_PLAYTYPE_LOOP 3
int PlaySoundMem(int,int,int=TRUE); int StopSoundMem(int, int=FALSE); int CheckSoundMem(int);
int ChangeVolumeSoundMem(int,int);
// ---- MidAcA2 (game/) で使う物(2026-10-14。zyaryu の ref/dxlib/DxLib.h と、当時の器のビルドで確かめた物) ----
#define MOUSE_INPUT_LEFT 1
#define MOUSE_INPUT_RIGHT 2
int SetMainWindowText(const char*); int SetWindowIconID(int); int SetWindowUserCloseEnableFlag(int); int GetWindowUserCloseFlag(int = FALSE);
int SetMouseDispFlag(int); int GetMouseInput(void); int GetMousePoint(int*, int*); int GetMouseWheelRotVol(int = TRUE);
// マウスの押した・離したの履歴と、タッチ(2026-10-16。公式リファレンスの入力関係の関数で確かめた。定数の値は宣言のため仮)
#define MOUSE_INPUT_LOG_DOWN 0
#define MOUSE_INPUT_LOG_UP 1
int GetMouseInputLog2(int* Button, int* ClickX, int* ClickY, int* LogType, int LogDelete = TRUE);
int GetTouchInputNum(void); int GetTouchInput(int InputNo, int* PositionX, int* PositionY, int* ID, int* Device);
// タッチの触り始め・離したの記録(2026-10-22。zyaryu の ref/dxlib/DxLib.h 3.24f で確かめた)
#define TOUCHINPUTPOINT_MAX 16
#define DX_TOUCHINPUT_TOOL_TYPE_MOUSE 3
typedef struct tagTOUCHINPUTPOINT { unsigned long Device; unsigned long ID; int PositionX; int PositionY; float Pressure; float Orientation; float Tilt; int ToolType; } TOUCHINPUTPOINT;
int GetTouchInputDownLogNum(void); int ClearTouchInputDownLog(void); TOUCHINPUTPOINT GetTouchInputDownLogOne(int PeekFlag = FALSE);
int GetTouchInputUpLogNum(void); int ClearTouchInputUpLog(void);
void Sleep(unsigned long);
int GetFontSizeToHandle(int); int SetFontCacheCharNum(int);
#define DX_FONTTYPE_ANTIALIASING 4
#define DX_FONTTYPE_ANTIALIASING_EDGE 6
int SetLoopPosSoundMem(LONGLONG, int); int SetTransColor(int, int, int);

// キー(2026-10-14。タイピングの物理キーボード others.cpp。ref/dxlib/DxLib.h から写した)
#define KEY_INPUT_BACK 0x0E
#define KEY_INPUT_TAB 0x0F
#define KEY_INPUT_RETURN 0x1C
#define KEY_INPUT_LSHIFT 0x2A
#define KEY_INPUT_RSHIFT 0x36
#define KEY_INPUT_LCONTROL 0x1D
#define KEY_INPUT_RCONTROL 0x9D
#define KEY_INPUT_SPACE 0x39
#define KEY_INPUT_PGUP 0xC9
#define KEY_INPUT_PGDN 0xD1
#define KEY_INPUT_END 0xCF
#define KEY_INPUT_HOME 0xC7
#define KEY_INPUT_LEFT 0xCB
#define KEY_INPUT_UP 0xC8
#define KEY_INPUT_RIGHT 0xCD
#define KEY_INPUT_DOWN 0xD0
#define KEY_INPUT_INSERT 0xD2
#define KEY_INPUT_DELETE 0xD3
#define KEY_INPUT_MINUS 0x0C
#define KEY_INPUT_YEN 0x7D
#define KEY_INPUT_PREVTRACK 0x90
#define KEY_INPUT_PERIOD 0x34
#define KEY_INPUT_SLASH 0x35
#define KEY_INPUT_LALT 0x38
#define KEY_INPUT_RALT 0xB8
#define KEY_INPUT_SCROLL 0x46
#define KEY_INPUT_SEMICOLON 0x27
#define KEY_INPUT_COLON 0x92
#define KEY_INPUT_LBRACKET 0x1A
#define KEY_INPUT_RBRACKET 0x1B
#define KEY_INPUT_AT 0x91
#define KEY_INPUT_BACKSLASH 0x2B
#define KEY_INPUT_COMMA 0x33
#define KEY_INPUT_KANJI 0x94
#define KEY_INPUT_CONVERT 0x79
#define KEY_INPUT_NOCONVERT 0x7B
#define KEY_INPUT_KANA 0x70
#define KEY_INPUT_APPS 0xDD
#define KEY_INPUT_CAPSLOCK 0x3A
#define KEY_INPUT_SYSRQ 0xB7
#define KEY_INPUT_PAUSE 0xC5
#define KEY_INPUT_LWIN 0xDB
#define KEY_INPUT_RWIN 0xDC
#define KEY_INPUT_NUMLOCK 0x45
#define KEY_INPUT_NUMPAD0 0x52
#define KEY_INPUT_NUMPAD1 0x4F
#define KEY_INPUT_NUMPAD2 0x50
#define KEY_INPUT_NUMPAD3 0x51
#define KEY_INPUT_NUMPAD4 0x4B
#define KEY_INPUT_NUMPAD5 0x4C
#define KEY_INPUT_NUMPAD6 0x4D
#define KEY_INPUT_NUMPAD7 0x47
#define KEY_INPUT_NUMPAD8 0x48
#define KEY_INPUT_NUMPAD9 0x49
#define KEY_INPUT_MULTIPLY 0x37
#define KEY_INPUT_ADD 0x4E
#define KEY_INPUT_SUBTRACT 0x4A
#define KEY_INPUT_DECIMAL 0x53
#define KEY_INPUT_DIVIDE 0xB5
#define KEY_INPUT_NUMPADENTER 0x9C
#define KEY_INPUT_F5 0x3F
#define KEY_INPUT_F6 0x40
#define KEY_INPUT_F7 0x41
#define KEY_INPUT_F8 0x42
#define KEY_INPUT_F9 0x43
#define KEY_INPUT_F10 0x44
#define KEY_INPUT_F11 0x57
#define KEY_INPUT_F12 0x58
#define KEY_INPUT_A 0x1E
#define KEY_INPUT_B 0x30
#define KEY_INPUT_C 0x2E
#define KEY_INPUT_D 0x20
#define KEY_INPUT_F 0x21
#define KEY_INPUT_G 0x22
#define KEY_INPUT_H 0x23
#define KEY_INPUT_I 0x17
#define KEY_INPUT_J 0x24
#define KEY_INPUT_K 0x25
#define KEY_INPUT_L 0x26
#define KEY_INPUT_M 0x32
#define KEY_INPUT_N 0x31
#define KEY_INPUT_O 0x18
#define KEY_INPUT_P 0x19
#define KEY_INPUT_S 0x1F
#define KEY_INPUT_T 0x14
#define KEY_INPUT_U 0x16
#define KEY_INPUT_V 0x2F
#define KEY_INPUT_X 0x2D
#define KEY_INPUT_Y 0x15
#define KEY_INPUT_Z 0x2C
#define KEY_INPUT_0 0x0B
#define KEY_INPUT_1 0x02
#define KEY_INPUT_2 0x03
#define KEY_INPUT_3 0x04
#define KEY_INPUT_4 0x05
#define KEY_INPUT_5 0x06
#define KEY_INPUT_6 0x07
#define KEY_INPUT_7 0x08
#define KEY_INPUT_8 0x09
#define KEY_INPUT_9 0x0A
int GetHitKeyStateAll(char* KeyStateArray);
// クイズの画面(2026-10-14。ref/dxlib/DxLib.h で確かめた)
int DerivationGraph(int SrcX, int SrcY, int Width, int Height, int SrcGraphHandle);
int SetCreateDrawValidGraphMultiSample(int Samples, int Quality);   // 3.24f の DxLib.h で確かめた(2026-10-15。キューブ)
int SetCameraScreenCenter(float x, float y);
int SetUseBackCulling(int Flag);
int ClearInputCharBuf(void);   // 3.24f の DxLib.h で確かめた(2026-10-15。打った字の溜まり)
char GetInputChar(int DeleteFlag);
/* 文字の入力のハンドル(2026-10-21 その場の問題の手直し quiz_edit)。宣言は公式のリファレンス(dxfunc_input)と sh-miyoshi/dxlib の写しで見た形。
   SetKeyInputStringFont と DrawKeyInputModeString は 3.24f の DxLib.h で fumiya に確かめてもらう */
int MakeKeyInput(int MaxStrLength, int CancelValidFlag, int SingleCharOnlyFlag, int NumCharOnlyFlag, int DoubleCharOnlyFlag = FALSE, int EnableNewLineFlag = FALSE);
int DeleteKeyInput(int InputHandle); int SetActiveKeyInput(int InputHandle); int CheckKeyInput(int InputHandle);
int GetKeyInputString(char* StrBuffer, int InputHandle); int SetKeyInputString(const char* String, int InputHandle);
int DrawKeyInputString(int x, int y, int InputHandle, int DrawCandidateList = TRUE); int DrawKeyInputModeString(int x, int y);
int SetKeyInputStringFont(int FontHandle);
#define CTRL_CODE_BS (0x08)
#define CTRL_CODE_CR (0x0d)
int GetImageSize_File(const char* FileName, int* SizeX, int* SizeY);   // 3.24f の DxLib.h で確かめた(2026-10-15)
int DrawRotaGraphF(float xf, float yf, double ExRate, double Angle, int GrHandle, int TransFlag, int ReverseXFlag=FALSE, int ReverseYFlag=FALSE);
int DrawExtendGraphF(float x1f, float y1f, float x2f, float y2f, int GrHandle, int TransFlag);   // (2026-10-17。ref/dxlib/DxLib.h(zyaryu)で確かめた)
int PlaySoundFile(const char* FileName, int PlayType);
int LoadARGB8ColorSoftImage(const char* FileName);   // (2026-10-20。ref/dxlib/DxLib.h(zyaryu)で確かめた)
int MakeARGB8ColorSoftImage(int SizeX, int SizeY);
int DrawPixelSoftImage(int SIHandle, int x, int y, int r, int g, int b, int a);
int CreateGraphFromSoftImage(int SIHandle);
int LoadSoftImage(const char* FileName);
int DeleteSoftImage(int SIHandle);
int GetSoftImageSize(int SIHandle, int* Width, int* Height);
int GetPixelSoftImage(int SIHandle, int x, int y, int* r, int* g, int* b, int* a);
#define DX_GRAPH_FILTER_GAUSS (1)
int GraphFilterBlt(int SrcGrHandle, int DestGrHandle, int FilterType, ...);
int MV1LoadModel(const char* FileName);
int MV1DeleteModel(int MHandle);
int MV1DrawModel(int MHandle);
int MV1SetPosition(int MHandle, VECTOR Position);
int MV1SetRotationXYZ(int MHandle, VECTOR Rotate);
int MV1SetTextureGraphHandle(int MHandle, int TexIndex, int GrHandle, int SemiTransFlag);
// 文字を伸び縮みさせて描く(2026-10-15。ref/dxlib/DxLib.h で確かめた)
int DrawExtendStringToHandle(int x, int y, double ExRateX, double ExRateY, const char* String, unsigned int Color, int FontHandle, unsigned int EdgeColor = 0, int VerticalFlag = FALSE);
// 画像の自由変形(2026-10-17。看板の字の奥行きのある回転。zyaryu の ref/dxlib/DxLib.h で確かめた)
int DrawModiGraphF(float,float,float,float,float,float,float,float,int,int);
// 通信(2026-10-21。game/net.cpp。zyaryu の ref/dxlib/DxLib.h で確かめた)
typedef struct tagIPDATA { unsigned char d1, d2, d3, d4; } IPDATA, *LPIPDATA;
int GetHostIPbyName(const char* HostName, IPDATA* IPDataBuf, int IPDataBufLength = 1, int* IPDataGetNum = 0);
int ConnectNetWork(IPDATA IPData, int Port = -1);
int CloseNetWork(int NetHandle);
int GetNetWorkDataLength(int NetHandle);
int GetLostNetWork(void);
int NetWorkRecv(int NetHandle, void* Buffer, int Length);
int NetWorkSend(int NetHandle, const void* Buffer, int Length);

// 動画のグラフィック(2026-10-21 形式の説明の動画 game/form_movie。ref/dxlib/DxLib.h(zyaryu)の 3450～3455 行で確かめた)
int PlayMovieToGraph(int GraphHandle, int PlayType = DX_PLAYTYPE_BACK, int SysPlay = 0);
int PauseMovieToGraph(int GraphHandle, int SysPause = 0);
int SeekMovieToGraph(int GraphHandle, int Time);
int GetMovieStateToGraph(int GraphHandle);
// ---- noz_exhbition の構文チェックで足した物(宣言だけ。本物の DxLib.h / windows.h の形に合わせた) ----
#ifndef NOZ_EX_STUB_EXTRA
#define NOZ_EX_STUB_EXTRA
#include <algorithm>
using std::min; using std::max;
#define DX_PI (3.14159265358979323846)
int DrawRectGraph(int,int,int,int,int,int,int,int,int=0,int=0);
int DrawRectRotaGraph3(int,int,int,int,int,int,int,int,double,double,double,int,int,int=0,int=0);
int DrawRotaGraph3(int,int,int,int,double,double,double,int,int,int=0,int=0);
int SetMovieVolumeToGraph(int,int);
typedef unsigned int DWORD;
#define ES_CONTINUOUS 0x80000000u
#define ES_DISPLAY_REQUIRED 0x2u
#define ES_SYSTEM_REQUIRED 0x1u
DWORD SetThreadExecutionState(DWORD);
int _snprintf(char*, size_t, const char*, ...);
int SetOutApplicationLogValidFlag(int);
int DrawRoundRect(int,int,int,int,int,int,unsigned int,int);
#endif
int AddFontResourceExA(const char*, unsigned long, void*);
typedef void* HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)(long long)-1)
typedef struct { char cFileName[260]; } WIN32_FIND_DATAA;
HANDLE FindFirstFileA(const char*, WIN32_FIND_DATAA*); int FindNextFileA(HANDLE, WIN32_FIND_DATAA*); int FindClose(HANDLE);
