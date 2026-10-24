#ifndef GUIDE_TEXT_H
#define GUIDE_TEXT_H

// =====================================================
// 案内の文(data/guide.txt。2026-10-24)。読むのは director.cpp、使うのはブース案内(demo_noz.cpp)と下の帯
//   place:  ブースの場所
//   main:   ブース案内の一番大きい文
//   sub:    その下の文(何行でも。GUIDE_SUB_MAX まで。%d はノズキンのゲームの数)
//   ticker: 画面の下の帯に流す文
// =====================================================

#define GUIDE_TEXT_LEN 256
#define GUIDE_SUB_MAX  4

typedef struct {
	char place[GUIDE_TEXT_LEN];
	char main[GUIDE_TEXT_LEN];
	char sub[GUIDE_SUB_MAX][GUIDE_TEXT_LEN];
	int  sub_num;
	char ticker[GUIDE_TEXT_LEN];
} GuideText;

extern GuideText guide;

#endif
