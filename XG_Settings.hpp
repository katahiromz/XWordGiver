#pragma once

// マスのフォント。
extern WCHAR xg_szCellFont[LF_FACESIZE];

// 小さな文字のフォント。
extern WCHAR xg_szSmallFont[LF_FACESIZE];

// UIフォント。
extern WCHAR xg_szUIFont[LF_FACESIZE];

// ひらがな表示か？
extern BOOL xg_bHiragana;
// Lowercase表示か？
extern BOOL xg_bLowercase;

// 文字の大きさ（％）。
extern int xg_nCellCharPercents;

// 小さい文字の大きさ（％）。
extern int xg_nSmallCharPercents;

// 線の太さ（pt）。
extern float xg_nLineWidthInPt;
#define XG_MIN_LINEWIDTH 0.05f
#define XG_MAX_LINEWIDTH 5.0f
#define XG_LINE_WIDTH_DEFAULT 1.0f
#define XG_LINE_WIDTH_DEFAULT2 L"1.0"
#define XG_LINE_WIDTH_DELTA 0.5f
#define XG_LINE_WIDTH_FORMAT L"%.1f"

// 外枠の幅（pt）。
extern float xg_nOuterFrameInPt;
#define XG_MIN_OUTERFRAME 0.05f
#define XG_MAX_OUTERFRAME 8.00f
#define XG_OUTERFRAME_DEFAULT 4.0f
#define XG_OUTERFRAME_DEFAULT2 L"4.0"
#define XG_OUTERFRAME_DELTA 0.5f
#define XG_OUTERFRAME_FORMAT L"%.1f"

// 色。
extern COLORREF xg_rgbWhiteCellColor;
extern COLORREF xg_rgbBlackCellColor;
extern COLORREF xg_rgbMarkedCellColor;
#define XG_WHITE_COLOR_DEFAULT L"16777215"
#define XG_BLACK_COLOR_DEFAULT L"3355443"
#define XG_MARKED_COLOR_DEFAULT L"16777215"

// ツールバーを表示するか？
extern bool xg_bShowToolBar;

// ビューモード。
extern XG_VIEW_MODE xg_nViewMode;

// 黒マス画像。
extern HBITMAP xg_hbmBlackCell;
extern HENHMETAFILE xg_hBlackCellEMF;
extern QStringW xg_strBlackCellImage;
