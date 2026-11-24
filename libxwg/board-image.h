// board-image.h --- 盤面のイメージ。
// Author: katahiromz
// License: MIT

#pragma once

#include "clues.h"

// 描画モード。
enum DRAW_MODE {
	DRAW_MODE_SCREEN,
	DRAW_MODE_PRINT,
	DRAW_MODE_EMF,
};

// ハイライト情報。
struct MHighLight {
	INT m_number;
	BOOL m_vertical;
};
extern MHighLight xg_highlight;

/////////////////////////////////////////////////////////////////////////////////////////
// MBoardDisplay

#define XG_DEFAULT_ZOOM 100			 // ズーム比率(%)。
#define XG_DEF_CELL_CHAR_SIZE 66		// 文字の大きさ（％）。
#define XG_DEF_SMALL_CHAR_SIZE 30	   // 小さい文字の大きさ（％）。

// 線の太さ（pt）。
#define XG_MIN_LINEWIDTH 0.05f
#define XG_MAX_LINEWIDTH 5.0f
#define XG_LINE_WIDTH_DEFAULT 1.0f
#define XG_LINE_WIDTH_DEFAULT2 L"1.0"
#define XG_LINE_WIDTH_DELTA 0.5f
#define XG_LINE_WIDTH_FORMAT L"%.1f"

// 外枠の幅（pt）。
#define XG_MIN_OUTERFRAME 0.05f
#define XG_MAX_OUTERFRAME 8.00f
#define XG_OUTERFRAME_DEFAULT 4.0f
#define XG_OUTERFRAME_DEFAULT2 L"4.0"
#define XG_OUTERFRAME_DELTA 0.5f
#define XG_OUTERFRAME_FORMAT L"%.1f"

// 色。
#define XG_WHITE_COLOR_DEFAULT_0 16777215
#define XG_BLACK_COLOR_DEFAULT_0 3355443
#define XG_MARKED_COLOR_DEFAULT_0 16777215
#define XG_WHITE_COLOR_DEFAULT L"16777215"
#define XG_BLACK_COLOR_DEFAULT L"3355443"
#define XG_MARKED_COLOR_DEFAULT L"16777215"

// 盤面で使用する画像データ。
struct MBoardImage {
	MStringW m_filename; // "$FILES\\..." "$BLOCK\\...", or pathname
	MStringA m_binary;
	HBITMAP m_hBitmap = nullptr;
	HENHMETAFILE m_hEMF = nullptr;
};

// 盤面表示情報。
struct MBoardDisplay {
	MBoard m_board; // 盤面。
	MBoard m_solution; // 盤面（答え）。
	INT m_nCellSize = 48; // マスのサイズ。
	INT m_nZoomedCellSize = 48; // マスのサイズ（ズーム後）。
	INT m_nMargin = 16; // マージン。
	INT m_nZoomRate = XG_DEFAULT_ZOOM; // ズーム率。
	LOGFONTW m_lfCellLogFont = {}; // マスのフォント（論理）。
	LOGFONTW m_lfSmallLogFont = {}; // 小さいフォント1（論理）。
	LOGFONTW m_lfSmallLogFont2 = {}; // 小さいフォント2（論理）。
	HFONT m_hCellFont = nullptr; // マスのフォント（ハンドル）。
	HFONT m_hSmallFont = nullptr; // 小さいフォント（ハンドル）。
	HFONT m_hSmallFont2 = nullptr; // 小さいフォント２（ハンドル）。
	LOGFONTW m_lfCellCreated = {}; // 作成済みフォントのキャッシュ判定用。
	LOGFONTW m_lfSmallCreated = {}; // 作成済みフォントのキャッシュ判定用。
	LOGFONTW m_lfSmall2Created = {}; // 作成済みフォントのキャッシュ判定用。
	BOOL m_bHiragana = FALSE; // ひらがな表示か？
	BOOL m_bLowercase = FALSE; // 英字小文字表示か？
	COLORREF m_rgbBlackCellColor = XG_BLACK_COLOR_DEFAULT_0; // 黒マスの色。
	COLORREF m_rgbWhiteCellColor = XG_WHITE_COLOR_DEFAULT_0; // 白マスの色。
	COLORREF m_rgbMarkedCellColor = XG_MARKED_COLOR_DEFAULT_0; // 二重マスの色。
	INT m_nCellCharPercents = XG_DEF_CELL_CHAR_SIZE; // 文字の割合（％）。
	INT m_nSmallCharPercents = XG_DEF_SMALL_CHAR_SIZE; // 小さい文字の割合（％）。
	float m_nLineWidthInPt = XG_LINE_WIDTH_DEFAULT; // 線の幅（point）。
	float m_nOuterFrameInPt = XG_OUTERFRAME_DEFAULT; // 外枠の幅（point）。
	std::vector<MPlaceInfo> m_vHorzInfo; // カギ情報（ヨコ）。
	std::vector<MPlaceInfo> m_vVertInfo; // カギ情報（タテ）。
	HBITMAP m_hbmBlackCell = nullptr; // 黒マス画像（ビットマップ）。
	HENHMETAFILE m_hBlackCellEMF = nullptr; // 黒マス画像（EMF）。
	std::vector<MBoardImage> m_images; // 画像を所有する。
	BOOL m_bCheckingAnswer = FALSE; // 答え合わせ中か？
	BOOL m_bSolved = FALSE; // 解があるか？
	MStringMapW m_word2clue; // 単語からヒント文章への写像。
	BOOL m_bNumCroMode = FALSE; // ナンクロモード。
	BOOL m_bDrawFrameForMarkedCell = TRUE; // 二重マスの枠を書くか？
	BOOL m_bShowDoubleFrameLetters = TRUE; // 二重マスの文字を書くか？
	BOOL m_bShowDoubleFrame = TRUE; // 二重マスを表示するか？
	BOOL m_bAddThickFrame = TRUE; // 外枠を表示するか？
	BOOL m_bShowAnswer = TRUE; // 答えを表示するか？
	BOOL m_bShowCaret = TRUE; // キャレットを表示するか？
	std::unordered_set<MBoardPos> m_slots;
	std::unordered_map<WCHAR, INT> m_mapNumCro1; // ナンクロ用写像。
	std::unordered_map<INT, WCHAR> m_mapNumCro2; // ナンクロ用写像。
	MStringW m_strDoubleFrameLetters = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; // 二重マスの文字。
	MStringW m_strBlackCellImage; // 黒マスの画像ファイル名。
	MStringW m_header; // ヘッダー。
	MStringW m_notes; // 備考欄。
	INT m_xScrollPos = 0; // キャンバスのスクロール位置。
	INT m_yScrollPos = 0; // キャンバスのスクロール位置。
	std::vector<MBoardPos> m_vMarks; // 二重マスが付いている文字位置。
	MBoardPos m_caret_pos = { 0, 0 }; // キャレット位置。
	XG_VIEW_MODE m_nViewMode = XG_VIEW_NORMAL; // スケルトンかどうか？

	MBoardDisplay();
	~MBoardDisplay() {
		Close();
	}

	void Reset();
	void Reset2();
	BOOL Load();
	BOOL Save() const;

	void Close();
	void SetZoom(INT nZoomRate);
	void FixCaretPos();
	void DeleteImages();
	void DeleteFonts();
	void CopyFrom(const MBoardDisplay& another);

	void DeleteBlackCellImage();
	BOOL CopyImageHandles(const MStringW& name, LPCWSTR subdir, HBITMAP& hbm, HENHMETAFILE& hEMF);
	BOOL SetBlackCellImage(const MStringW& name, LPCWSTR subdir = nullptr);

private:
	// 直接コピーは禁止。CopyFromを使うこと。
	MBoardDisplay(const MBoardDisplay&) = default;
	MBoardDisplay(MBoardDisplay&&) = default;
	MBoardDisplay& operator=(const MBoardDisplay&) = default;
	MBoardDisplay& operator=(MBoardDisplay&&) = default;
};

/////////////////////////////////////////////////////////////////////////////////////////

WCHAR __fastcall XgReplaceChar(MBoardDisplay& display, WCHAR ch) noexcept;
HFONT __fastcall XgGetNormalFont(MBoardDisplay& display) noexcept;
HFONT __fastcall XgGetSmallFont(MBoardDisplay& display) noexcept;
HFONT __fastcall XgGetSmallFont2(MBoardDisplay& display) noexcept;

MBoard& __fastcall XgGetDisplayBoard(MBoardDisplay& display) noexcept;
SIZE __fastcall XgGetBoardExtent(MBoardDisplay& display, DRAW_MODE mode = DRAW_MODE_PRINT);
void __fastcall XgDrawBoard(MBoardDisplay& display, HDC hdc, const SIZE* psiz = nullptr, DRAW_MODE mode = DRAW_MODE_PRINT);
void __fastcall XgDrawBoard(MBoardDisplay& display, MBoard& board, HDC hdc, const SIZE* psiz = nullptr, DRAW_MODE mode = DRAW_MODE_PRINT);

HBITMAP __fastcall XgBitmapFromBoard(MBoardDisplay& display, const SIZE *psiz = nullptr, DRAW_MODE mode = DRAW_MODE_PRINT);
HBITMAP __fastcall XgBitmapFromBoard(MBoardDisplay& display, MBoard& board, const SIZE *psiz = nullptr, DRAW_MODE mode = DRAW_MODE_PRINT);
HGLOBAL __fastcall XgDIBFromBoard(MBoardDisplay& display);
HENHMETAFILE __fastcall XgEMFFromBoard(MBoardDisplay& display);
HENHMETAFILE __fastcall XgEMFFromBoard(MBoardDisplay& display, MBoard& board);

BOOL __fastcall XgSaveBoardBitmap(MBoardDisplay& display, LPCWSTR pszFileName);
BOOL __fastcall XgSaveBoardEMF(MBoardDisplay& display, HWND hwnd, LPCWSTR pszFileName);
void XgGetCellPosition(MBoardDisplay& display, RECT& rc, INT j1, INT i1, INT j2, INT i2, BOOL bScroll = FALSE);
void XgSetCellPosition(MBoardDisplay& display, LONG& x, LONG& y, INT& j, INT& i, BOOL bEnd = FALSE);
void __fastcall XgDrawCaret(MBoardDisplay& display, HDC hdc, BOOL bScroll);

BOOL XgImportLooksFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgExportLooksFile(MBoardDisplay& display, LPCWSTR filename);

BOOL XgIsNoneImageName(const MStringW& name);
BOOL __fastcall XgValidateNumCro(MBoardDisplay& display);
void __fastcall XgMakeItNumCro(MBoardDisplay& display);

SIZE __fastcall XgGetMarkWordExtent(MBoardDisplay& display, INT count) noexcept;
void __fastcall XgDrawMarkWord(MBoardDisplay& display, HDC hdc, const SIZE *psiz);

MStringW XgGetMonoFont(void);
MStringW XgGetDefaultUIFont(void);
