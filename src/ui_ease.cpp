#include <math.h>
#include "ui_ease.h"

// =====================================================
// 動きの部品(2026 改修)。使い方は ui_ease.h を参照
// =====================================================

#define EASE_BACK_OVERSHOOT 1.70158f   // EASE_OUT_BACK の行き過ぎの量(大きいほど、大きく行き過ぎる)

// 関数の役割:t(0～1)を、カーブの種類で 0～1 に直す
float Ease_Apply(int type, float t) {
	float u;

	if (t <= 0.0f) return 0.0f;
	if (t >= 1.0f) return 1.0f;

	switch (type) {
	case EASE_IN_CUBIC:
		return t * t * t;

	case EASE_OUT_CUBIC:
		u = 1.0f - t;
		return 1.0f - u * u * u;

	case EASE_IN_OUT_CUBIC:
		if (t < 0.5f) return 4.0f * t * t * t;
		u = -2.0f * t + 2.0f;
		return 1.0f - u * u * u / 2.0f;

	case EASE_OUT_BACK:
		u = t - 1.0f;
		return 1.0f + (EASE_BACK_OVERSHOOT + 1.0f) * u * u * u + EASE_BACK_OVERSHOOT * u * u;

	case EASE_OUT_BOUNCE:
		if (t < 1.0f / 2.75f) return 7.5625f * t * t;
		if (t < 2.0f / 2.75f) { u = t - 1.5f / 2.75f;   return 7.5625f * u * u + 0.75f; }
		if (t < 2.5f / 2.75f) { u = t - 2.25f / 2.75f;  return 7.5625f * u * u + 0.9375f; }
		u = t - 2.625f / 2.75f;
		return 7.5625f * u * u + 0.984375f;

	case EASE_OUT_ELASTIC:
		return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * (2.0f * 3.14159265f / 3.0f)) + 1.0f;

	case EASE_LINEAR:
	default:
		return t;
	}
}

// 関数の役割:frame が、start から dur フレームの間のどこにあるか(0～1)。start より前なら 0、過ぎていれば 1
float Ease_T(int frame, int start, int dur) {
	if (dur <= 0) return (frame >= start) ? 1.0f : 0.0f;
	if (frame <= start) return 0.0f;
	if (frame >= start + dur) return 1.0f;
	return (float)(frame - start) / dur;
}

float Ease_Lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

// 関数の役割:from から to へ、frame が start から dur の間に、type の調子で動いた時の位置
int Ease_Move(int from, int to, int frame, int start, int dur, int type) {
	float t = Ease_Apply(type, Ease_T(frame, start, dur));
	float v = Ease_Lerp((float)from, (float)to, t);
	return (int)floorf(v + 0.5f);
}

float Ease_MoveF(float from, float to, int frame, int start, int dur, int type) {
	return Ease_Lerp(from, to, Ease_Apply(type, Ease_T(frame, start, dur)));
}

int Ease_Done(int frame, int start, int dur) {
	return (frame >= start + dur) ? 1 : 0;
}
