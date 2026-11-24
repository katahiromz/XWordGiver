// board-image.cpp --- 盤面のイメージ。
// Author: katahiromz
// License: MIT

#include "board-image.h"
#include "clues.h"
#include "file-io.h"
#include "marks.h"
#include "MPointSizeRect.h"
#include "MRegKey.h"
#include "image-io.h"

// ハイライト色。
constexpr COLORREF c_rgbHighlight = RGB(255, 255, 140);
constexpr COLORREF c_rgbHighlightAndDblFrame = RGB(255, 155, 100);

// 答え合わせで一致しないセルの色。
constexpr COLORREF c_rgbUnmatchedCell = RGB(255, 191, 191);

// ハイライト情報。
MHighLight xg_highlight = { 0, FALSE };

MBoard& __fastcall XgGetDisplayBoard(MBoardDisplay& display) noexcept {
	return (display.m_bSolved && display.m_bShowAnswer) ? display.m_solution : display.m_board;
}

// クロスワードの描画サイズを計算する。
SIZE __fastcall XgGetBoardExtent(MBoardDisplay& display, DRAW_MODE mode) {
	auto& board = XgGetDisplayBoard(display);
	INT nZoomedCellSize = ((mode == DRAW_MODE_SCREEN) ? display.m_nCellSize * display.m_nZoomRate / 100
													  : display.m_nCellSize);
	SIZE siz;
	siz.cx = static_cast<int>(display.m_nMargin * 2 + board.Columns() * nZoomedCellSize);
	siz.cy = static_cast<int>(display.m_nMargin * 2 + board.Rows() * nZoomedCellSize);
	return siz;
}

// マス位置からイメージ座標を取得する。
void __fastcall XgCellToImage(MBoardDisplay& display, RECT& rc, MBoardPos cell) {
	rc = {
		display.m_nMargin + cell.m_j * display.m_nZoomedCellSize,
		display.m_nMargin + cell.m_i * display.m_nZoomedCellSize,
		display.m_nMargin + (cell.m_j + 1) * display.m_nZoomedCellSize,
		display.m_nMargin + (cell.m_i + 1) * display.m_nZoomedCellSize
	};
}

// ポイント単位をピクセル単位に変換する（X座標）。
float XPixelsFromPoints(HDC hDC, float points) noexcept {
	return points * static_cast<float>(::GetDeviceCaps(hDC, LOGPIXELSX)) / 72;
}

// ポイント単位をピクセル単位に変換する（Y座標）。
float YPixelsFromPoints(HDC hDC, float points) noexcept {
	return points * static_cast<float>(::GetDeviceCaps(hDC, LOGPIXELSY)) / 72;
}

// 文字を表示用に置き換える。
static WCHAR XgReplaceCharImpl(const MBoardDisplay& display, WCHAR ch) noexcept {
	WCHAR new_ch, ch0 = ch;

	// 文字を変換する。
	if (display.m_bHiragana) {
		LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch0, 1, &new_ch, 1);
		ch0 = new_ch;
	} else {
		LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch0, 1, &new_ch, 1);
		ch0 = new_ch;
	}

	if (XgIsCharKanaW(ch0) || ch == ZEN_PROLONG || XgIsCharKanjiW(ch0)) {
		if (display.m_bLowercase) {
			LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE, &ch0, 1, &new_ch, 1);
			ch0 = new_ch;
		} else {
			LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_UPPERCASE, &ch0, 1, &new_ch, 1);
			ch0 = new_ch;
		}
	} else {
		if (display.m_bLowercase) {
			LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch0, 1, &new_ch, 1);
			ch0 = new_ch;
		} else {
			LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch0, 1, &new_ch, 1);
			ch0 = new_ch;
		}
	}

	return ch0;
}

// 文字を表示用に置き換える（キャッシュ付き。LCMapStringWは遅いので）。
WCHAR __fastcall XgReplaceChar(MBoardDisplay& display, WCHAR ch) noexcept {
	thread_local std::unordered_map<UINT, WCHAR> s_cache;
	const UINT key = (UINT(display.m_bHiragana ? 1 : 0) << 17) | (UINT(display.m_bLowercase ? 1 : 0) << 16) | UINT(ch);
	auto it = s_cache.find(key);
	if (it != s_cache.end())
		return it->second;
	const WCHAR ret = XgReplaceCharImpl(display, ch);
	s_cache.emplace(key, ret);
	return ret;
}

// キャレットを描画する。
void __fastcall XgDrawCaret(MBoardDisplay& display, HDC hdc, BOOL bScroll) {
	if (!display.m_bShowCaret)
		return;

	display.m_nZoomedCellSize = display.m_nCellSize * display.m_nZoomRate / 100;

	// キャレット位置をピクセル単位で取得。
	RECT rc;
	XgGetCellPosition(display, rc, display.m_caret_pos.m_j, display.m_caret_pos.m_i,
					  display.m_caret_pos.m_j + 1, display.m_caret_pos.m_i + 1, bScroll);
	rc.right -= 1;
	rc.bottom -= 1;

	// 赤いキャレットペンを作成する。
	LOGBRUSH lbRed;
	lbRed.lbStyle = BS_SOLID;
	lbRed.lbColor = RGB(255, 0, 0);
	HPEN hCaretPen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
		1, &lbRed, 0, nullptr);
	HGDIOBJ hPenOld = ::SelectObject(hdc, hCaretPen);

	// カギカッコみたいなもの、コーナーに四つ。
	const auto cxyMargin = display.m_nZoomedCellSize / 10; // 余白。
	const auto cxyLine = display.m_nZoomedCellSize / 3; // 線の位置。
	::MoveToEx(hdc, rc.left + cxyMargin, rc.top + cxyMargin, nullptr);
	::LineTo(hdc, rc.left + cxyMargin, rc.top + cxyLine);
	::MoveToEx(hdc, rc.left + cxyMargin, rc.top + cxyMargin, nullptr);
	::LineTo(hdc, rc.left + cxyLine, rc.top + cxyMargin);
	::MoveToEx(hdc, rc.right - cxyMargin, rc.top + cxyMargin, nullptr);
	::LineTo(hdc, rc.right - cxyMargin, rc.top + cxyLine);
	::MoveToEx(hdc, rc.right - cxyMargin, rc.top + cxyMargin, nullptr);
	::LineTo(hdc, rc.right - cxyLine, rc.top + cxyMargin);
	::MoveToEx(hdc, rc.right - cxyMargin, rc.bottom - cxyMargin, nullptr);
	::LineTo(hdc, rc.right - cxyMargin, rc.bottom - cxyLine);
	::MoveToEx(hdc, rc.right - cxyMargin, rc.bottom - cxyMargin, nullptr);
	::LineTo(hdc, rc.right - cxyLine, rc.bottom - cxyMargin);
	::MoveToEx(hdc, rc.left + cxyMargin, rc.bottom - cxyMargin, nullptr);
	::LineTo(hdc, rc.left + cxyMargin, rc.bottom - cxyLine);
	::MoveToEx(hdc, rc.left + cxyMargin, rc.bottom - cxyMargin, nullptr);
	::LineTo(hdc, rc.left + cxyLine, rc.bottom - cxyMargin);

	// 十字。
	const auto cxyCross = display.m_nZoomedCellSize / 10; // 十字の半径。
	::MoveToEx(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cxyCross, nullptr);
	::LineTo(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 + cxyCross);
	::MoveToEx(hdc, (rc.left + rc.right) / 2 - cxyCross, (rc.top + rc.bottom) / 2, nullptr);
	::LineTo(hdc, (rc.left + rc.right) / 2 + cxyCross, (rc.top + rc.bottom) / 2);

	::SelectObject(hdc, hPenOld);
	::DeleteObject(hCaretPen);
}

// 標準的な等幅フォント名を取得。
MStringW XgGetMonoFont(void) {
	if (XgIsUserJapanese())
		return L"ＭＳ ゴシック";
	return L"Lucida Console";
}

// デフォルトのUIフォントを取得。
MStringW XgGetDefaultUIFont(void) {
	LOGFONTW lf;
	::GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(lf), &lf);
	return lf.lfFaceName;
}

// 通常の文字のフォントを作成する。
HFONT __fastcall XgGetNormalFont(MBoardDisplay& display) noexcept {
	LOGFONTW lf = display.m_lfCellLogFont;
	lf.lfHeight = -display.m_nZoomedCellSize * display.m_nCellCharPercents / 100;
	lf.lfWidth = 0;
	if (!lf.lfWeight)
		lf.lfWeight = FW_NORMAL;
	if (!lf.lfQuality)
		lf.lfQuality = ANTIALIASED_QUALITY;
	if (!lf.lfCharSet)
		lf.lfCharSet = SHIFTJIS_CHARSET;
	if (!lf.lfFaceName[0])
		StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), XgGetMonoFont().c_str());
	// 要求が前回と同じなら作成済みのフォントを再利用する。
	if (display.m_hCellFont && ::memcmp(&display.m_lfCellCreated, &lf, sizeof(lf)) == 0)
		return display.m_hCellFont;
	if (display.m_hCellFont)
		::DeleteObject(display.m_hCellFont);
	display.m_hCellFont = ::CreateFontIndirectW(&lf);
	display.m_lfCellCreated = lf;
	assert(display.m_hCellFont);
	return display.m_hCellFont;
}

// 小さい文字のフォントを作成する。
HFONT __fastcall XgGetSmallFont(MBoardDisplay& display) noexcept {
	LOGFONTW lf = display.m_lfSmallLogFont;
	lf.lfHeight = -display.m_nZoomedCellSize * display.m_nSmallCharPercents / 100;
	lf.lfWidth = 0;
	if (!lf.lfWeight)
		lf.lfWeight = FW_NORMAL;
	if (!lf.lfQuality)
		lf.lfQuality = ANTIALIASED_QUALITY;
	if (!lf.lfCharSet)
		lf.lfCharSet = SHIFTJIS_CHARSET;
	if (!lf.lfFaceName[0])
		StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), XgGetDefaultUIFont().c_str());
	// 要求が前回と同じなら作成済みのフォントを再利用する。
	if (display.m_hSmallFont && ::memcmp(&display.m_lfSmallCreated, &lf, sizeof(lf)) == 0)
		return display.m_hSmallFont;
	if (display.m_hSmallFont)
		::DeleteObject(display.m_hSmallFont);
	display.m_hSmallFont = ::CreateFontIndirectW(&lf);
	display.m_lfSmallCreated = lf;
	assert(display.m_hSmallFont);
	return display.m_hSmallFont;
}

// 小さい文字のフォントを作成する。
HFONT __fastcall XgGetSmallFont2(MBoardDisplay& display) noexcept {
	LOGFONTW lf = display.m_lfSmallLogFont2;
	lf.lfHeight = -display.m_nZoomedCellSize * display.m_nSmallCharPercents / 100;
	lf.lfWidth = 0;
	if (!lf.lfWeight)
		lf.lfWeight = FW_NORMAL;
	if (!lf.lfQuality)
		lf.lfQuality = ANTIALIASED_QUALITY;
	if (!lf.lfCharSet)
		lf.lfCharSet = SHIFTJIS_CHARSET;
	if (!lf.lfFaceName[0])
		StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), XgGetDefaultUIFont().c_str());
	// 要求が前回と同じなら作成済みのフォントを再利用する。
	if (display.m_hSmallFont2 && ::memcmp(&display.m_lfSmall2Created, &lf, sizeof(lf)) == 0)
		return display.m_hSmallFont2;
	if (display.m_hSmallFont2)
		::DeleteObject(display.m_hSmallFont2);
	display.m_hSmallFont2 = ::CreateFontIndirectW(&lf);
	display.m_lfSmall2Created = lf;
	assert(display.m_hSmallFont2);
	return display.m_hSmallFont2;
}

// マスの数字を描画する。
static void __fastcall
XgDrawCellNumber(MBoardDisplay& display, HDC hdc, const RECT& rcCell, INT j, INT i, INT number) {
	auto& board = XgGetDisplayBoard(display);

	WCHAR sz[8];
	StringCchPrintfW(sz, _countof(sz), L"%d", number);

	// 小さいフォントを先に選択してから文字サイズを測る（ズームに追従させるため）。
	HGDIOBJ hFontOld = ::SelectObject(hdc, XgGetSmallFont(display));

	SIZE siz;
	::GetTextExtentPoint32W(hdc, sz, lstrlenW(sz), &siz);
	RECT rcText = rcCell;
	rcText.right = rcCell.left + siz.cx + 1;
	rcText.bottom = rcCell.top + siz.cy;

	// 文字の背景を塗りつぶす。
	COLORREF rgbBack;
	const INT nMarked = XgGetMarked(display, j, i);
	if (display.m_bCheckingAnswer && display.m_bSolved &&
		board.GetCell0(j, i) != display.m_solution.GetCell0(j, i))
	{
		// 答え合わせで一致しないセル。
		rgbBack = c_rgbUnmatchedCell;
	} else {
		const BOOL bDbl = (nMarked != -1 && !display.m_bNumCroMode && display.m_bShowDoubleFrame);
		if (display.m_slots.count(MBoardPos(j, i)) > 0) {
			rgbBack = bDbl ? c_rgbHighlightAndDblFrame : c_rgbHighlight;
		} else if (bDbl) {
			rgbBack = display.m_rgbMarkedCellColor;
		} else {
			rgbBack = display.m_rgbWhiteCellColor;
		}
	}
	// ExtTextOutのETO_OPAQUEで背景の塗りつぶしと文字描画を一度に行う（ブラシ作成も不要）。
	const COLORREF rgbOldBk = ::SetBkColor(hdc, rgbBack);
	::SetTextColor(hdc, display.m_rgbBlackCellColor);
	::ExtTextOutW(hdc, rcText.left, rcText.top, ETO_OPAQUE, &rcText, sz, lstrlenW(sz), nullptr);
	::SetBkColor(hdc, rgbOldBk);
	::SelectObject(hdc, hFontOld);
}

// スロット群を取得。
std::unordered_set<MBoardPos>
XgGetSlots(MBoardDisplay& display, INT number, BOOL vertical) {
	auto& board = XgGetDisplayBoard(display);
	std::unordered_set<MBoardPos> ret;
	const INT nColumns = board.Columns(), nRows = board.Rows();

	INT j = -1, i = -1;
	if (vertical) {
		for (auto& info : display.m_vVertInfo) {
			if (info.m_number == number) {
				j = info.m_jCol;
				i = info.m_iRow;
				break;
			}
		}
		if (i != -1) {
			ret.emplace(j, i);
			for (++i; i < nRows; ++i) {
				if (board.GetCell0(j, i) == ZEN_BLACK) {
					break;
				}
				ret.emplace(j, i);
			}
		}
	} else {
		for (auto& info : display.m_vHorzInfo) {
			if (info.m_number == number) {
				j = info.m_jCol;
				i = info.m_iRow;
				break;
			}
		}
		if (i != -1) {
			ret.emplace(j, i);
			for (++j; j < nColumns; ++j) {
				if (board.GetCell0(j, i) == ZEN_BLACK) {
					break;
				}
				ret.emplace(j, i);
			}
		}
	}

	return ret;
}

// 二重マスを描画する。
static void XgDrawDoubleFrameCell(MBoardDisplay& display, HDC hdc, INT nMarked, const RECT& rc, HPEN hThinPen, INT j, INT i) {
	// 二重マスの内側の枠を描く。
	if (display.m_bDrawFrameForMarkedCell) {
		RECT rcFrame = rc;
		::InflateRect(&rcFrame, -display.m_nZoomedCellSize / 9, -display.m_nZoomedCellSize / 9);
		::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
		HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
		::Rectangle(hdc, rcFrame.left, rcFrame.top, rcFrame.right + 1, rcFrame.bottom + 1);
		::SelectObject(hdc, hPenOld);
	}

	// 二重マスの文字を描画するか？
	if (display.m_bShowDoubleFrameLetters) {
		WCHAR sz[2];
		if (nMarked < INT(display.m_strDoubleFrameLetters.size()))
			StringCchPrintfW(sz, _countof(sz), L"%c", display.m_strDoubleFrameLetters[nMarked]);
		else
			StringCchPrintfW(sz, _countof(sz), L"%c", ZEN_BLACK);

		// 小さいフォント2を先に選択してから文字サイズを測る（ズームに追従させるため）。
		HGDIOBJ hFontOld = ::SelectObject(hdc, XgGetSmallFont2(display));

		// マスの右下端に合わせる。マスからはみ出さないようにする。
		SIZE siz;
		::GetTextExtentPoint32W(hdc, sz, lstrlenW(sz), &siz);
		const INT pad = 1;
		RECT rcText;
		rcText.right = rc.right - pad;
		rcText.bottom = rc.bottom - pad;
		rcText.left = rcText.right - siz.cx;
		rcText.top = rcText.bottom - siz.cy;
		if (rcText.left < rc.left)
			rcText.left = rc.left;
		if (rcText.top < rc.top)
			rcText.top = rc.top;

		// 背景色を決める。
		COLORREF rgbBack;
		if (display.m_bCheckingAnswer && display.m_bSolved &&
			display.m_board.GetCell0(j, i) != display.m_solution.GetCell0(j, i))
		{
			// 答え合わせで一致しないセル。
			rgbBack = c_rgbUnmatchedCell;
		} else if (display.m_slots.count(MBoardPos(j, i)) > 0) {
			// ハイライトかつ二重マス。
			rgbBack = c_rgbHighlightAndDblFrame;
		} else {
			rgbBack = display.m_rgbMarkedCellColor;
		}

		// 背景の塗りつぶしと文字描画を一度に行う。
		const COLORREF rgbOldBk = ::SetBkColor(hdc, rgbBack);
		::SetTextColor(hdc, display.m_rgbBlackCellColor);
		::ExtTextOutW(hdc, rcText.left, rcText.top, ETO_OPAQUE, &rcText, sz, lstrlenW(sz), nullptr);
		::SetBkColor(hdc, rgbOldBk);
		::SelectObject(hdc, hFontOld);
	}
}

// クロスワードを描画する（通常ビュー）。
static void __fastcall
XgDrawBoard_NormalView(MBoardDisplay& display, MBoard& board, HDC hdc, const SIZE *psiz, DRAW_MODE mode) {
	// セルの大きさ。
	if (mode == DRAW_MODE_SCREEN)
		display.m_nZoomedCellSize = (display.m_nCellSize * display.m_nZoomRate / 100);
	else
		display.m_nZoomedCellSize = display.m_nCellSize;

	// 全体を白で塗りつぶす。
	RECT rc0 = { 0, 0, psiz->cx, psiz->cy };
	::SelectObject(hdc, ::GetStockObject(NULL_PEN));
	::FillRect(hdc, &rc0, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

	// ブラシを作成する。
	HBRUSH hbrBlack = ::CreateSolidBrush(display.m_rgbBlackCellColor);
	HBRUSH hbrWhite = ::CreateSolidBrush(display.m_rgbWhiteCellColor);
	HBRUSH hbrMarked = ::CreateSolidBrush(display.m_rgbMarkedCellColor);
	HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
	HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);
	HBRUSH hbrUnmatched = ::CreateSolidBrush(c_rgbUnmatchedCell);

	display.m_slots = XgGetSlots(display, xg_highlight.m_number, xg_highlight.m_vertical);
	if (mode != DRAW_MODE_SCREEN)
		display.m_slots.clear();

	// 線の太さ。
	INT c_nThin = 1, c_nWide = 4;
	switch (mode) {
	case DRAW_MODE_SCREEN:
	case DRAW_MODE_PRINT:
		c_nThin = INT(YPixelsFromPoints(hdc, display.m_nLineWidthInPt));
		c_nWide = INT(YPixelsFromPoints(hdc, display.m_nOuterFrameInPt));
		break;
	case DRAW_MODE_EMF:
		// FIXME: "Microsoft Print to PDF" で印刷すると線の幅がおかしくなる。
		// よくわからないので、1pt == 1pxで近似する。
		c_nThin = INT(display.m_nLineWidthInPt);
		c_nWide = INT(display.m_nOuterFrameInPt);
		break;
	}
	if (c_nThin < 2)
		c_nThin = 2;
	if (c_nWide < 2)
		c_nWide = 2;
	if (c_nWide > display.m_nMargin)
		c_nWide = display.m_nMargin;

	LOGBRUSH lbBlack;
	lbBlack.lbStyle = BS_SOLID;
	lbBlack.lbColor = display.m_rgbBlackCellColor;

	// 黒の細いペンを作成する。
	HPEN hThinPen = ::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
								   c_nThin, &lbBlack, 0, nullptr);

	BITMAP bm;
	GetObject(display.m_hbmBlackCell, sizeof(bm), &bm);

	// 黒マスビットマップを選択する。
	HDC hdcMem = ::CreateCompatibleDC(nullptr);
	::SelectObject(hdcMem, display.m_hbmBlackCell);
	::SetStretchBltMode(hdcMem, STRETCH_HALFTONE);

	// セルの背景を描画する。
	const INT nColumns = board.Columns(), nRows = board.Rows();

	// 二重マス情報は何度も使うので、一度だけ取得しておく。
	std::vector<INT> vMarked(size_t(nColumns) * size_t(nRows), -1);
	for (INT i = 0; i < nRows; i++) {
		for (INT j = 0; j < nColumns; j++) {
			vMarked[size_t(i) * nColumns + j] = XgGetMarked(display, j, i);
		}
	}
	for (INT i = 0; i < nRows; i++) {
		for (INT j = 0; j < nColumns; j++) {
			// セルの座標をセットする。
			RECT rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};

			// 二重マスか？
			const INT nMarked = vMarked[size_t(i) * nColumns + j];

			// 塗りつぶす。
			const WCHAR ch = board.GetCell0(j, i);
			if (ch == ZEN_BLACK) {
				// 黒マス。
				if (display.m_hbmBlackCell) {
					StretchBlt(hdc,
							   rc.left, rc.top,
							   rc.right - rc.left, rc.bottom - rc.top,
							   hdcMem, 0, 0, bm.bmWidth, bm.bmHeight,
							   SRCCOPY);
				} else if (display.m_hBlackCellEMF) {
					::PlayEnhMetaFile(hdc, display.m_hBlackCellEMF, &rc);
				} else {
					::FillRect(hdc, &rc, hbrBlack);
				}
			} else if (display.m_bCheckingAnswer && display.m_bSolved &&
					   board.GetCell0(j, i) != display.m_solution.GetCell0(j, i))
			{
				// 答え合わせで一致しないセル。
				::FillRect(hdc, &rc, hbrUnmatched);
			} else if (display.m_slots.count(MBoardPos(j, i)) > 0 && nMarked != -1 && !display.m_bNumCroMode && display.m_bShowDoubleFrame) {
				// ハイライトかつ二重マスかつ非ナンクロかつ二重マスが有効。
				::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
			} else if (display.m_slots.count(MBoardPos(j, i)) > 0) {
				// ハイライト。
				::FillRect(hdc, &rc, hbrHighlight);
			} else if (nMarked != -1 && !display.m_bNumCroMode && display.m_bShowDoubleFrame) {
				// 二重マスかつ非ナンクロかつ二重マスが有効。
				::FillRect(hdc, &rc, hbrMarked);
			} else {
				// その他のマス。
				::FillRect(hdc, &rc, hbrWhite);
			}
		}
	}

	// デバイスコンテキストを破棄する。
	::DeleteDC(hdcMem);

	// 二重マスを描画する。
	if (!display.m_bNumCroMode && display.m_bShowDoubleFrame) {
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				// セルの座標をセットする。
				RECT rc = {
					display.m_nMargin + j * display.m_nZoomedCellSize,
					display.m_nMargin + i * display.m_nZoomedCellSize,
					display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
					display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
				};

				// 二重マスか？
				const INT nMarked = vMarked[size_t(i) * nColumns + j];
				if (nMarked == -1)
					continue;

				XgDrawDoubleFrameCell(display, hdc, nMarked, rc, hThinPen, j, i);
			}
		}
	}

	// タテのカギの先頭マス。
	if (!display.m_bNumCroMode) {
		const INT size = INT(display.m_vVertInfo.size());
		for (INT k = 0; k < size; k++) {
			const INT j = display.m_vVertInfo[k].m_jCol, i = display.m_vVertInfo[k].m_iRow;

			RECT rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};
			::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

			XgDrawCellNumber(display, hdc, rc, j, i, display.m_vVertInfo[k].m_number);
		}
	}

	// ヨコのカギの先頭マス。
	if (!display.m_bNumCroMode) {
		const INT size = INT(display.m_vHorzInfo.size());
		for (INT k = 0; k < size; k++) {
			const INT j = display.m_vHorzInfo[k].m_jCol, i = display.m_vHorzInfo[k].m_iRow;

			RECT rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};
			::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

			XgDrawCellNumber(display, hdc, rc, j, i, display.m_vHorzInfo[k].m_number);
		}
	}

	// ナンクロモードの小さい数字。
	if (display.m_bSolved && display.m_bNumCroMode) {
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				const WCHAR ch = display.m_solution.GetCell0(j, i);
				if (ch == ZEN_BLACK || ch == ZEN_SPACE)
					continue;

				RECT rc = {
					display.m_nMargin + j * display.m_nZoomedCellSize,
					display.m_nMargin + i * display.m_nZoomedCellSize,
					display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
					display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
				};
				::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

				XgDrawCellNumber(display, hdc, rc, j, i, display.m_mapNumCro1[ch]);
			}
		}
	}

	// セルの文字を描画する。
	{
		// 文字マス用の設定は一度だけ行う（セルごとのフォント選択・文字サイズ測定を避ける）。
		// TA_CENTERで水平方向の中央寄せはGDIに任せ、縦はフォントの高さから求める。
		// 文字の背景は透明。DrawTextだとなぜか失敗するのでTextOutを使う。
		::SetBkMode(hdc, TRANSPARENT);
		::SetTextColor(hdc, display.m_rgbBlackCellColor);
		const UINT uOldAlign = ::SetTextAlign(hdc, TA_CENTER | TA_TOP);
		HGDIOBJ hFontOld = ::SelectObject(hdc, XgGetNormalFont(display));
		TEXTMETRICW tmLetter = {};
		::GetTextMetricsW(hdc, &tmLetter);
		const INT cyLetterHalf = tmLetter.tmHeight / 2;
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				const WCHAR ch = XgReplaceChar(display, board.GetCell0(j, i));
				if (ch == ZEN_BLACK)
					continue;

				// セルの座標。
				const RECT rc = {
					display.m_nMargin + j * display.m_nZoomedCellSize,
					display.m_nMargin + i * display.m_nZoomedCellSize,
					display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
					display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
				};
				::TextOutW(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cyLetterHalf, &ch, 1);
			}
		}
		::SelectObject(hdc, hFontOld);
		::SetTextAlign(hdc, uOldAlign);
	}

	// 線を引く。
	HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
	for (INT i = 0; i <= nRows; i++) {
		::MoveToEx(hdc, display.m_nMargin, INT(display.m_nMargin + i * display.m_nZoomedCellSize), nullptr);
		::LineTo(hdc, psiz->cx - display.m_nMargin, INT(display.m_nMargin + i * display.m_nZoomedCellSize));
	}
	for (INT j = 0; j <= nColumns; j++) {
		::MoveToEx(hdc, INT(display.m_nMargin + j * display.m_nZoomedCellSize), display.m_nMargin, nullptr);
		::LineTo(hdc, INT(display.m_nMargin + j * display.m_nZoomedCellSize), psiz->cy - display.m_nMargin);
	}
	::SelectObject(hdc, hPenOld);

	// 周りに太い線を描く。線を引こうと思ったが、コーナーがうまく描画できないので、
	// 塗りつぶしで対処する。
	hPenOld = ::SelectObject(hdc, GetStockBrush(NULL_PEN));
	if (display.m_bAddThickFrame) {
		RECT rc = { display.m_nMargin, display.m_nMargin, psiz->cx - display.m_nMargin, psiz->cy - display.m_nMargin };
		::InflateRect(&rc, c_nWide, c_nWide);
		RECT rc2;

		rc2 = rc;
		rc2.right = rc.left + c_nWide;
		::FillRect(hdc, &rc2, hbrBlack);

		rc2 = rc;
		rc2.left = rc.right - c_nWide;
		::FillRect(hdc, &rc2, hbrBlack);

		rc2 = rc;
		rc2.bottom = rc.top + c_nWide;
		::FillRect(hdc, &rc2, hbrBlack);

		rc2 = rc;
		rc2.top = rc.bottom - c_nWide;
		::FillRect(hdc, &rc2, hbrBlack);
	}
	::SelectObject(hdc, hPenOld);

	// 破棄する。
	::DeleteObject(hThinPen);
	::DeleteObject(hbrBlack);
	::DeleteObject(hbrWhite);
	::DeleteObject(hbrMarked);
	::DeleteObject(hbrHighlight);
	::DeleteObject(hbrHighlightAndDblFrame);
	::DeleteObject(hbrUnmatched);
}

// クロスワードを描画する（スケルトンビュー）。
static void __fastcall
XgDrawBoard_SkeletonView(MBoardDisplay& display, MBoard& board, HDC hdc, const SIZE *psiz, DRAW_MODE mode) {
	// セルの大きさ。
	if (mode == DRAW_MODE_SCREEN)
		display.m_nZoomedCellSize = (display.m_nCellSize * display.m_nZoomRate / 100);
	else
		display.m_nZoomedCellSize = display.m_nCellSize;

	// スクリーンの場合は、全体を白で塗りつぶす。
	// それ以外は、四隅に白い点を描く（EMFで余白が省略されないように）。
	RECT rc = { 0, 0, psiz->cx, psiz->cy };
	if (mode == DRAW_MODE_SCREEN) {
		::FillRect(hdc, &rc, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
	} else {
		constexpr COLORREF rgbWhite = RGB(255, 255, 255);
		::SetPixelV(hdc, rc.left, rc.top, rgbWhite);
		::SetPixelV(hdc, rc.right - 1, rc.top, rgbWhite);
		::SetPixelV(hdc, rc.right - 1, rc.bottom - 1, rgbWhite);
		::SetPixelV(hdc, rc.left, rc.bottom - 1, rgbWhite);
	}

	// ブラシを作成する。
	HBRUSH hbrBlack = ::CreateSolidBrush(display.m_rgbBlackCellColor);
	HBRUSH hbrWhite = ::CreateSolidBrush(display.m_rgbWhiteCellColor);
	HBRUSH hbrMarked = ::CreateSolidBrush(display.m_rgbMarkedCellColor);
	HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
	HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);
	HBRUSH hbrUnmatched = ::CreateSolidBrush(c_rgbUnmatchedCell);

	display.m_slots = XgGetSlots(display, xg_highlight.m_number, xg_highlight.m_vertical);
	if (mode != DRAW_MODE_SCREEN)
		display.m_slots.clear();

	// 線の太さ。
	INT c_nThin = 1, c_nWide = 4;
	switch (mode) {
	case DRAW_MODE_SCREEN:
	case DRAW_MODE_PRINT:
		c_nThin = INT(YPixelsFromPoints(hdc, display.m_nLineWidthInPt));
		c_nWide = INT(YPixelsFromPoints(hdc, display.m_nOuterFrameInPt));
		break;
	case DRAW_MODE_EMF:
		// FIXME: "Microsoft Print to PDF" で印刷すると線の幅がおかしくなる。
		// よくわからないので、1pt == 1pxで近似する。
		c_nThin = INT(display.m_nLineWidthInPt);
		c_nWide = INT(display.m_nOuterFrameInPt);
		break;
	}
	if (c_nThin < 2)
		c_nThin = 2;
	if (c_nWide < 2)
		c_nWide = 2;

	// 黒いブラシ。
	LOGBRUSH lbBlack;
	lbBlack.lbStyle = BS_SOLID;
	lbBlack.lbColor = display.m_rgbBlackCellColor;

	// 黒の細いペンを作成する。
	HPEN hThinPen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
		c_nThin, &lbBlack, 0, nullptr);

	const INT nColumns = board.Columns(), nRows = board.Rows();

	// 外枠と背景を描画する。
	if (display.m_bAddThickFrame) {
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				const WCHAR ch = board.GetCell0(j, i);
				if (ch == ZEN_BLACK)
					continue;

				// セルの座標をセットする。
				rc = {
					display.m_nMargin + j * display.m_nZoomedCellSize,
					display.m_nMargin + i * display.m_nZoomedCellSize,
					display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
					display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
				};

				RECT rcExtended = rc;
				::InflateRect(&rcExtended, c_nWide, c_nWide);

				// 背景を塗りつぶす。
				::FillRect(hdc, &rcExtended, hbrBlack);
			}
		}
	}

	// 文字の背景を描画する。
	for (INT i = 0; i < nRows; i++) {
		for (INT j = 0; j < nColumns; j++) {
			const WCHAR ch = board.GetCell0(j, i);
			if (ch == ZEN_BLACK)
				continue;

			// セルの座標をセットする。
			rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};

			// 二重マスか？
			const INT nMarked = XgGetMarked(display, j, i);

			// 塗りつぶす。
			if (display.m_bCheckingAnswer && display.m_bSolved &&
				board.GetCell0(j, i) != display.m_solution.GetCell0(j, i))
			{
				// 答え合わせで一致しないセル。
				::FillRect(hdc, &rc, hbrUnmatched);
			} else if (display.m_slots.count(MBoardPos(j, i)) > 0 && nMarked != -1 && !display.m_bNumCroMode && display.m_bShowDoubleFrame) {
				// ハイライトかつ二重マスかつ非ナンクロかつ二重マスが有効。
				::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
			} else if (display.m_slots.count(MBoardPos(j, i)) > 0) {
				// ハイライト。
				::FillRect(hdc, &rc, hbrHighlight);
			} else if (nMarked != -1 && !display.m_bNumCroMode && display.m_bShowDoubleFrame) {
				// 二重マスかつ非ナンクロかつ二重マスが有効。
				::FillRect(hdc, &rc, hbrMarked);
			} else {
				// その他のマス。
				::FillRect(hdc, &rc, hbrWhite);
			}
		}
	}

	// 二重マスを描画する。
	if (!display.m_bNumCroMode && display.m_bShowDoubleFrame) {
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				// 二重マスか？
				const INT nMarked = XgGetMarked(display, j, i);
				if (nMarked == -1)
					continue;

				// セルの座標をセットする。
				rc = {
					display.m_nMargin + j * display.m_nZoomedCellSize,
					display.m_nMargin + i * display.m_nZoomedCellSize,
					display.m_nMargin + (j + 1) * display.m_nZoomedCellSize - 1,
					display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
				};

				XgDrawDoubleFrameCell(display, hdc, nMarked, rc, hThinPen, j, i);
			}
		}
	}

	// タテのカギの先頭マス。
	if (!display.m_bNumCroMode) {
		const INT size = INT(display.m_vVertInfo.size());
		for (INT k = 0; k < size; k++) {
			const auto j = display.m_vVertInfo[k].m_jCol, i = display.m_vVertInfo[k].m_iRow;

			rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};
			::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

			XgDrawCellNumber(display, hdc, rc, j, i, display.m_vVertInfo[k].m_number);
		}
	}

	// ヨコのカギの先頭マス。
	if (!display.m_bNumCroMode) {
		const INT size = INT(display.m_vHorzInfo.size());
		for (INT k = 0; k < size; k++) {
			const auto j = display.m_vHorzInfo[k].m_jCol, i = display.m_vHorzInfo[k].m_iRow;

			rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};
			::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

			XgDrawCellNumber(display, hdc, rc, j, i, display.m_vHorzInfo[k].m_number);
		}
	}

	// ナンクロモードの小さい数字。
	if (display.m_bSolved && display.m_bNumCroMode) {
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				const WCHAR ch = display.m_solution.GetCell0(j, i);
				if (ch == ZEN_BLACK || ch == ZEN_SPACE)
					continue;

				rc = {
					display.m_nMargin + j * display.m_nZoomedCellSize,
					display.m_nMargin + i * display.m_nZoomedCellSize,
					display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
					display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
				};
				::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

				XgDrawCellNumber(display, hdc, rc, j, i, display.m_mapNumCro1[ch]);
			}
		}
	}

	// セルの文字を描画する。
	// 文字マス用の設定は一度だけ行う（セルごとのフォント選択・文字サイズ測定を避ける）。
	// TA_CENTERで水平方向の中央寄せはGDIに任せ、縦はフォントの高さから求める。
	// 文字の背景は透明。DrawTextだとなぜか失敗するのでTextOutを使う。
	::SetBkMode(hdc, TRANSPARENT);
	::SetTextColor(hdc, display.m_rgbBlackCellColor);
	const UINT uOldAlign = ::SetTextAlign(hdc, TA_CENTER | TA_TOP);
	HGDIOBJ hFontOld = ::SelectObject(hdc, XgGetNormalFont(display));
	TEXTMETRICW tmLetter = {};
	::GetTextMetricsW(hdc, &tmLetter);
	const INT cyLetterHalf = tmLetter.tmHeight / 2;
	for (INT i = 0; i < nRows; i++) {
		for (INT j = 0; j < nColumns; j++) {
			auto ch = XgReplaceChar(display, board.GetCell0(j, i));
			if (ch == ZEN_BLACK)
				continue;

			// セルの座標をセットする。
			rc = {
				display.m_nMargin + j * display.m_nZoomedCellSize,
				display.m_nMargin + i * display.m_nZoomedCellSize,
				display.m_nMargin + (j + 1) * display.m_nZoomedCellSize,
				display.m_nMargin + (i + 1) * display.m_nZoomedCellSize
			};

			// マスの文字を描画する。
			::TextOutW(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cyLetterHalf, &ch, 1);

			// セルの枠を描画する。
			// Rectangle関数ではうまくいかないので直接線を描画する。
			HGDIOBJ hPen2Old = ::SelectObject(hdc, hThinPen);
			::MoveToEx(hdc, rc.left, rc.top, nullptr);
			::LineTo(hdc, rc.right, rc.top);
			::LineTo(hdc, rc.right, rc.bottom);
			::LineTo(hdc, rc.left, rc.bottom);
			::LineTo(hdc, rc.left, rc.top);
			::SelectObject(hdc, hPen2Old);
		}
	}
	::SelectObject(hdc, hFontOld);
	::SetTextAlign(hdc, uOldAlign);

	// 破棄する。
	::DeleteObject(hThinPen);
	::DeleteObject(hbrBlack);
	::DeleteObject(hbrWhite);
	::DeleteObject(hbrMarked);
	::DeleteObject(hbrHighlight);
	::DeleteObject(hbrHighlightAndDblFrame);
	::DeleteObject(hbrUnmatched);
}

// クロスワードを描画する。
void __fastcall XgDrawBoard(MBoardDisplay& display, MBoard& board, HDC hdc, const SIZE *psiz, DRAW_MODE mode) {
	SIZE siz;
	if (!psiz) {
		siz = XgGetBoardExtent(display, mode);
		psiz = &siz;
	}
	switch (display.m_nViewMode) {
	case XG_VIEW_NORMAL:
		XgDrawBoard_NormalView(display, board, hdc, psiz, mode);
		break;
	case XG_VIEW_SKELETON:
		XgDrawBoard_SkeletonView(display, board, hdc, psiz, mode);
		break;
	}
}

// クロスワードを描画する。
void __fastcall XgDrawBoard(MBoardDisplay& display, HDC hdc, const SIZE *psiz, DRAW_MODE mode) {
	auto& board = XgGetDisplayBoard(display);
	XgDrawBoard(display, board, hdc, psiz, mode);
}

// クロスワードのイメージを作成する。
HBITMAP __fastcall XgBitmapFromBoard(MBoardDisplay& display, MBoard& board, const SIZE *psiz, DRAW_MODE mode) {
	SIZE siz;
	if (!psiz) {
		siz = XgGetBoardExtent(display, mode);
		psiz = &siz;
	}

	// 互換DCを作成する。
	HDC hdc = ::CreateCompatibleDC(nullptr);
	if (!hdc)
		return nullptr;

	// DIBを作成する。
	HBITMAP hbm = XgCreate24BppBitmap(hdc, psiz->cx, psiz->cy);
	if (hbm == nullptr) {
		DeleteDC(hdc);
		return nullptr;
	}

	// 描画する。
	HGDIOBJ hbmOld = ::SelectObject(hdc, hbm);
	XgDrawBoard(display, hdc, psiz, mode);
	::SelectObject(hdc, hbmOld);

	// 互換DCを破棄する。
	::DeleteDC(hdc);
	return hbm;
}

// クロスワードのイメージを作成する。
HBITMAP __fastcall XgBitmapFromBoard(MBoardDisplay& display, const SIZE *psiz, DRAW_MODE mode) {
	auto& board = XgGetDisplayBoard(display);
	return XgBitmapFromBoard(display, board, psiz, mode);
}

// ビットマップを保存する。
BOOL __fastcall XgSaveBoardBitmap(MBoardDisplay& display, LPCWSTR pszFileName) {
	HBITMAP hbm = XgBitmapFromBoard(display, nullptr, DRAW_MODE_PRINT);
	if (!hbm)
		return FALSE;
	BOOL ret = SaveBitmapToFileW(pszFileName, hbm);
	::DeleteObject(hbm);
	return ret;
}

// EMFを保存する。
BOOL __fastcall XgSaveBoardEMF(MBoardDisplay& display, HWND hwnd, LPCWSTR pszFileName) {
	SIZE siz = XgGetBoardExtent(display, DRAW_MODE_EMF);

	HDC hdcRef = ::GetDC(hwnd);
	HDC hdc = ::CreateEnhMetaFileW(hdcRef, pszFileName, nullptr, L"XWordGiver");
	if (!hdc) {
		::ReleaseDC(hwnd, hdcRef);
		return FALSE;
	}

	XgDrawBoard(display, hdc, &siz, DRAW_MODE_EMF);
	::CloseEnhMetaFile(hdc);
	::ReleaseDC(hwnd, hdcRef);
	return TRUE;
}

// マスのピクセル位置を取得する。
void XgGetCellPosition(MBoardDisplay& display, RECT& rc, INT j1, INT i1, INT j2, INT i2, BOOL bScroll) {
	display.m_nZoomedCellSize = display.m_nCellSize * display.m_nZoomRate / 100;

	rc = {
		display.m_nMargin + j1 * display.m_nZoomedCellSize,
		display.m_nMargin + i1 * display.m_nZoomedCellSize,
		display.m_nMargin + j2 * display.m_nZoomedCellSize,
		display.m_nMargin + i2 * display.m_nZoomedCellSize
	};

	if (bScroll) {
		::OffsetRect(&rc, -display.m_xScrollPos, -display.m_yScrollPos);
	}
}

// マスのピクセル位置を設定する。
void XgSetCellPosition(MBoardDisplay& display, LONG& x, LONG& y, INT& j, INT& i, BOOL bEnd) {
	display.m_nZoomedCellSize = display.m_nCellSize * display.m_nZoomRate / 100;

	auto& board = XgGetDisplayBoard(display);
	const INT nColumns = board.Columns(), nRows = board.Rows();

	x += display.m_xScrollPos;
	x -= display.m_nMargin;
	j = (x + display.m_nZoomedCellSize / 2) / display.m_nZoomedCellSize;
	if (j < 0) {
		j = bEnd ? 1 : 0;
	} else if (j >= nColumns) {
		j = bEnd ? nColumns : (nColumns - 1);
	}
	x = j * display.m_nZoomedCellSize;
	x += display.m_nMargin;

	y += display.m_yScrollPos;
	y -= display.m_nMargin;
	i = (y + display.m_nZoomedCellSize / 2) / display.m_nZoomedCellSize;
	if (i < 0) {
		i = bEnd ? 1 : 0;
	} else if (i >= nRows) {
		i = bEnd ? nRows : (nRows - 1);
	}
	y = i * display.m_nZoomedCellSize;
	y += display.m_nMargin;
}

// EMFを作成する。
HENHMETAFILE __fastcall XgEMFFromBoard(MBoardDisplay& display, MBoard& board) {
	// 描画サイズを取得。ズームなし。
	SIZE siz = XgGetBoardExtent(display, DRAW_MODE_EMF);

	// EMFを作成する。
	HENHMETAFILE hEMF = nullptr;
	HDC hdcRef = ::CreateCompatibleDC(nullptr);
	HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, L"XWordGiver");
	if (hdc) {
		// EMFに描画する。
		XgDrawBoard(display, board, hdc, &siz, DRAW_MODE_EMF);
		hEMF = ::CloseEnhMetaFile(hdc);
		::DeleteDC(hdcRef);
	}

	return hEMF;
}

// EMFを作成する。
HENHMETAFILE __fastcall XgEMFFromBoard(MBoardDisplay& display) {
	auto board = XgGetDisplayBoard(display);
	return XgEMFFromBoard(display, board);
}

// クリップボード用のDIBを作成する。
HGLOBAL __fastcall XgDIBFromBoard(MBoardDisplay& display) {
	HBITMAP hbm = XgBitmapFromBoard(display, nullptr, DRAW_MODE_PRINT);
	if (!hbm)
		return nullptr;

	std::vector<BYTE> data;
	PackedDIB_CreateFromHandle(data, hbm);

	HGLOBAL hDIB = GlobalAlloc(GHND | GMEM_SHARE, static_cast<DWORD>(data.size()));
	if (hDIB) {
		if (PVOID pv = GlobalLock(hDIB)) {
			memcpy(pv, &data[0], data.size());
			GlobalUnlock(hDIB);
		}
	}
	DeleteObject(hbm);

	return hDIB;
}

// 16進をバイナリにする。
static void XgHexToBin(std::vector<BYTE>& data, const MStringW& str) {
	data.clear();
	WCHAR sz[3] = {};
	size_t len = str.size() & ~size_t(1); // 奇数なら最後の1文字を捨てる。
	for (size_t i = 0; i < len; i += 2) {
		sz[0] = str[i];
		sz[1] = str[i + 1];
		data.push_back(static_cast<BYTE>(wcstol(sz, nullptr, 16)));
	}
}

// バイナリを16進にする。
static MStringW XgBinToHex(const void *ptr, size_t size) {
	MStringW ret;
	auto pb = static_cast<const BYTE *>(ptr);
	WCHAR sz[8];
	for (size_t i = 0; i < size; ++i) {
		StringCchPrintfW(sz, _countof(sz), L"%02X", pb[i]);
		ret += sz;
	}
	return ret;
}

// 整数を文字列にする。
LPCWSTR XgIntToStr(int nValue) {
	static WCHAR s_szText[64];
	StringCchPrintfW(s_szText, _countof(s_szText), L"%d", nValue);
	return s_szText;
}

// イメージファイルを解決する。
MStringW XgResolveImageFile(const MStringW& strFileName, LPCWSTR subdir) {
	WCHAR path[MAX_PATH];
	auto title_name = PathFindFileNameW(strFileName.c_str());
	if (subdir && strFileName.find(L"$FILES\\") == 0) {
		StringCchCopyW(path, _countof(path), subdir);
		StringCchCatW(path, _countof(path), L"\\");
		StringCchCatW(path, _countof(path), title_name);
	} else if (strFileName.find(L"$BLOCK\\") == 0) {
		auto block_dir = XgFindLocalFile(L"BLOCK");
		StringCchCopyW(path, _countof(path), block_dir.c_str());
		StringCchCatW(path, _countof(path), L"\\");
		StringCchCatW(path, _countof(path), title_name);
	} else {
		StringCchCopyW(path, _countof(path), strFileName.c_str());
	}
	return MStringW(path);
}

// 盤面の画像ファイルを読み込む。
static BOOL XgImportImageFile(MBoardDisplay& display, const MStringW& strFileName, LPCWSTR subdir) {
	auto filename = XgResolveImageFile(strFileName, subdir);
	auto title = PathFindFileNameW(filename.c_str());

	for (auto& image : display.m_images) {
		auto title2 = PathFindFileNameW(image.m_filename.c_str());
		if (lstrcmpiW(title2, title) == 0) {
			return TRUE; // 読み込み済み。
		}
	}

	MStringA binary;
	HENHMETAFILE hEMF = nullptr;
	HBITMAP hBitmap = nullptr;
	if (!XgReadFileAll(filename.c_str(), binary))
		return FALSE; // 読み込めなかった。
	if (!XgLoadImage(filename.c_str(), hBitmap, hEMF))
		return FALSE; // 読み込めなかった。

	MBoardImage image = { strFileName, std::move(binary), hBitmap, hEMF };
	display.m_images.push_back(image);

	return TRUE;
}

// LOOKSファイルを読み込む。
BOOL XgImportLooksFile(MBoardDisplay& display, LPCWSTR filename) {
	if (!PathFileExistsW(filename))
		return FALSE;

	WCHAR subdir[MAX_PATH];
	StringCchCopyW(subdir, _countof(subdir), filename);
	PathRemoveFileSpecW(subdir);

	display.DeleteImages();
	display.m_images.clear();

	WCHAR szText[512], szText2[512];

	// 色。
	GetPrivateProfileStringW(L"Looks", L"WhiteCellColor", XG_WHITE_COLOR_DEFAULT, szText, _countof(szText), filename);
	display.m_rgbWhiteCellColor = _wtoi(szText);

	GetPrivateProfileStringW(L"Looks", L"BlackCellColor", XG_BLACK_COLOR_DEFAULT, szText, _countof(szText), filename);
	display.m_rgbBlackCellColor = _wtoi(szText);

	GetPrivateProfileStringW(L"Looks", L"MarkedCellColor", XG_MARKED_COLOR_DEFAULT, szText, _countof(szText), filename);
	display.m_rgbMarkedCellColor = _wtoi(szText);

	// 線の幅（pt）。
	GetPrivateProfileStringW(L"Looks", L"LineWidthInPt", XG_LINE_WIDTH_DEFAULT2, szText, _countof(szText), filename);
	display.m_nLineWidthInPt = wcstof(szText, nullptr);
	if (display.m_nLineWidthInPt < XG_MIN_LINEWIDTH)
		display.m_nLineWidthInPt = XG_MIN_LINEWIDTH;
	if (display.m_nLineWidthInPt > XG_MAX_LINEWIDTH)
		display.m_nLineWidthInPt = XG_MAX_LINEWIDTH;

	// フォント。
	GetPrivateProfileStringW(L"Looks", L"CellLogFont", L"", szText, _countof(szText), filename);
	if (szText[0]) {
		std::vector<BYTE> data;
		XgHexToBin(data, szText);
		if (data.size() == sizeof(LOGFONTW)) {
			memcpy(&display.m_lfCellLogFont, data.data(), sizeof(LOGFONTW));
		}
	}
	GetPrivateProfileStringW(L"Looks", L"SmallLogFont", L"", szText, _countof(szText), filename);
	if (szText[0]) {
		std::vector<BYTE> data;
		XgHexToBin(data, szText);
		if (data.size() == sizeof(LOGFONTW)) {
			memcpy(&display.m_lfSmallLogFont, data.data(), sizeof(LOGFONTW));
		}
	}
	GetPrivateProfileStringW(L"Looks", L"SmallLogFont2", L"", szText, _countof(szText), filename);
	if (szText[0]) {
		std::vector<BYTE> data;
		XgHexToBin(data, szText);
		if (data.size() == sizeof(LOGFONTW)) {
			memcpy(&display.m_lfSmallLogFont2, data.data(), sizeof(LOGFONTW));
		}
	}

	// スケルトンビューか？
	if (XgIsUserJapanese())
		GetPrivateProfileStringW(L"Looks", L"SkeletonView", L"0", szText, _countof(szText), filename);
	else
		GetPrivateProfileStringW(L"Looks", L"SkeletonView", L"1", szText, _countof(szText), filename);
	const BOOL bSkeltonView = _wtoi(szText);
	display.m_nViewMode = bSkeltonView ? XG_VIEW_SKELETON : XG_VIEW_NORMAL;

	// 太枠をつけるか？
	GetPrivateProfileStringW(L"Looks", L"AddThickFrame", L"1", szText, _countof(szText), filename);
	display.m_bAddThickFrame = !!_wtoi(szText);

	// 二重マスに枠をつけるか？
	GetPrivateProfileStringW(L"Looks", L"DrawFrameForMarkedCell", L"1", szText, _countof(szText), filename);
	display.m_bDrawFrameForMarkedCell = !!_wtoi(szText);

	// 英小文字か？
	GetPrivateProfileStringW(L"Looks", L"Lowercase", L"0", szText, _countof(szText), filename);
	display.m_bLowercase = !!_wtoi(szText);

	// ひらがなか？
	GetPrivateProfileStringW(L"Looks", L"Hiragana", L"0", szText, _countof(szText), filename);
	display.m_bHiragana = !!_wtoi(szText);

	// 文字の大きさ。
	StringCchPrintfW(szText2, _countof(szText2), L"%d", XG_DEF_CELL_CHAR_SIZE);
	GetPrivateProfileStringW(L"Looks", L"CellCharPercents", szText2, szText, _countof(szText), filename);
	display.m_nCellCharPercents = _wtoi(szText);

	StringCchPrintfW(szText2, _countof(szText2), L"%d", XG_DEF_SMALL_CHAR_SIZE);
	GetPrivateProfileStringW(L"Looks", L"SmallCharPercents", szText2, szText, _countof(szText), filename);
	display.m_nSmallCharPercents = _wtoi(szText);

	// 黒マス画像。
	GetPrivateProfileStringW(L"Looks", L"BlackCellImage", L"", szText, _countof(szText), filename);
	display.SetBlackCellImage(szText, subdir);

	// 二重マス文字。
	GetPrivateProfileStringW(L"Looks", L"DoubleFrameLetters", L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", szText, _countof(szText), filename);
	{
		std::vector<BYTE> data;
		XgHexToBin(data, szText);
		MStringW str;
		str.resize(data.size() / sizeof(WCHAR));
		memcpy(&str[0], data.data(), data.size());
		display.m_strDoubleFrameLetters = str;
	}

	return TRUE;
}

// LOOKSファイルを保存する。
BOOL XgExportLooksFile(MBoardDisplay& display, LPCWSTR filename) {
	WCHAR subdir[MAX_PATH];
	StringCchCopyW(subdir, _countof(subdir), filename);
	PathRemoveFileSpecW(subdir);

	// 文字の大きさの設定。
	WritePrivateProfileStringW(L"Looks", L"CellCharPercents", XgIntToStr(display.m_nCellCharPercents), filename);
	WritePrivateProfileStringW(L"Looks", L"SmallCharPercents", XgIntToStr(display.m_nSmallCharPercents), filename);

	// セルフォント。
	{
		MStringW str = XgBinToHex(&display.m_lfCellLogFont, sizeof(display.m_lfCellLogFont));
		WritePrivateProfileStringW(L"Looks", L"CellLogFont", str.c_str(), filename);
	}
	// 小さい文字のフォント。
	{
		MStringW str = XgBinToHex(&display.m_lfSmallLogFont, sizeof(display.m_lfSmallLogFont));
		WritePrivateProfileStringW(L"Looks", L"SmallLogFont", str.c_str(), filename);
	}
	{
		MStringW str = XgBinToHex(&display.m_lfSmallLogFont2, sizeof(display.m_lfSmallLogFont2));
		WritePrivateProfileStringW(L"Looks", L"SmallLogFont2", str.c_str(), filename);
	}

	// 画像ファイルを出力し、MBoardImage::m_filename を "$FILES\\..." 形式に変更する。
	for (auto& image : display.m_images) {
		auto title = PathFindFileNameW(image.m_filename.c_str());
		auto resolved = XgResolveImageFile(image.m_filename, subdir);
		if (lstrcmpiW(PathFindFileNameW(image.m_filename.c_str()), title) == 0) {
			XgWriteFileAll(resolved.c_str(), image.m_binary);
		}

		MStringW str = L"$FILES\\";
		str += title;
		image.m_filename = std::move(str);
	}

	// もし黒マス画像が指定されていれば
	if (display.m_strBlackCellImage.size() &&
		display.m_strBlackCellImage != L"(None)" && display.m_strBlackCellImage != L"(なし)")
	{
		// すでに、画像ファイルは上記のコードで保存されているはず。
		auto title = PathFindFileNameW(display.m_strBlackCellImage.c_str());

		// "$FILES\\" 形式に変更。
		MStringW str = L"$FILES\\";
		str += title;
		WritePrivateProfileStringW(L"Looks", L"BlackCellImage", str.c_str(), filename);
		display.m_strBlackCellImage = str;
	}
	else
	{
		WritePrivateProfileStringW(L"Looks", L"BlackCellImage", L"", filename);
	}

	// スケルトンビューか？
	const BOOL bSkeltonView = (display.m_nViewMode == XG_VIEW_SKELETON);
	WritePrivateProfileStringW(L"Looks", L"SkeletonView", XgIntToStr(bSkeltonView), filename);

	// 太枠をつけるか？
	WritePrivateProfileStringW(L"Looks", L"AddThickFrame", XgIntToStr(display.m_bAddThickFrame), filename);

	// 二重マスに枠をつけるか？
	WritePrivateProfileStringW(L"Looks", L"DrawFrameForMarkedCell", XgIntToStr(display.m_bDrawFrameForMarkedCell), filename);

	// 英小文字か？
	WritePrivateProfileStringW(L"Looks", L"Lowercase", XgIntToStr(display.m_bLowercase), filename);

	// ひらがなか？
	WritePrivateProfileStringW(L"Looks", L"Hiragana", XgIntToStr(display.m_bHiragana), filename);

	// 色を設定する。
	WritePrivateProfileStringW(L"Looks", L"WhiteCellColor", XgIntToStr(display.m_rgbWhiteCellColor), filename);
	WritePrivateProfileStringW(L"Looks", L"BlackCellColor", XgIntToStr(display.m_rgbBlackCellColor), filename);
	WritePrivateProfileStringW(L"Looks", L"MarkedCellColor", XgIntToStr(display.m_rgbMarkedCellColor), filename);

	// 線の幅（pt）を設定する。
	{
		WCHAR sz[64];
		StringCchPrintfW(sz, _countof(sz), L"%.1f", display.m_nLineWidthInPt);
		WritePrivateProfileStringW(L"Looks", L"LineWidthInPt", sz, filename);
	}

	// 二重マス文字。
	{
		MStringW str = display.m_strDoubleFrameLetters;
		str = XgBinToHex(str.c_str(), str.size() * sizeof(WCHAR));
		WritePrivateProfileStringW(L"Looks", L"DoubleFrameLetters", str.c_str(), filename);
	}

	// フラッシュ！ すべてを完了させるための儀式。
	return WritePrivateProfileStringW(nullptr, nullptr, nullptr, filename);
}

/////////////////////////////////////////////////////////////////////////////////////////
// MBoardDisplay

MBoardDisplay::MBoardDisplay() { }

void MBoardDisplay::Reset() {
	Close();
	*this = MBoardDisplay();

	if (m_lfCellLogFont.lfFaceName[0] == UNICODE_NULL)
		StringCchCopyW(m_lfCellLogFont.lfFaceName, _countof(m_lfCellLogFont.lfFaceName), XgGetMonoFont().c_str());
	if (m_lfSmallLogFont.lfFaceName[0] == UNICODE_NULL)
		StringCchCopyW(m_lfSmallLogFont.lfFaceName, _countof(m_lfSmallLogFont.lfFaceName), XgGetDefaultUIFont().c_str());
	if (m_lfSmallLogFont2.lfFaceName[0] == UNICODE_NULL)
		StringCchCopyW(m_lfSmallLogFont2.lfFaceName, _countof(m_lfSmallLogFont2.lfFaceName), XgGetDefaultUIFont().c_str());
}

void MBoardDisplay::Reset2() {
	auto bSolved = m_bSolved;
	auto board = m_board;
	auto solution = m_solution;
	auto vHorzInfo = m_vHorzInfo;
	auto vVertInfo = m_vVertInfo;
	auto vMarks = m_vMarks;
	auto nRules = xg_nRules;

	Reset();

	m_bSolved = bSolved;
	m_board = board;
	m_solution = solution;
	m_vHorzInfo = vHorzInfo;
	m_vVertInfo = vVertInfo;
	m_vMarks = vMarks;
	m_board.Rules() = nRules;
	m_solution.Rules() = nRules;
}

BOOL MBoardDisplay::Load() {
	Reset();

	// アプリ名キーを開く。
	MRegKey app_key(HKEY_CURRENT_USER, XG_REGKEY_APP, FALSE);
	if (!app_key)
		return FALSE;

	DWORD dwValue;
	WCHAR sz[1024];

	INT nColumns = XG_DEFAULT_BOARD_SIZE, nRows = XG_DEFAULT_BOARD_SIZE;
	if (!app_key.QueryDword(L"Cols", dwValue)) {
		nColumns = dwValue;
	}
	if (!app_key.QueryDword(L"Rows", dwValue)) {
		nRows = dwValue;
	}
	m_board = std::move(MBoard(nColumns, nRows, xg_nRules));
	m_solution = std::move(MBoard(nColumns, nRows, xg_nRules));

	// 論理フォントを読み込む。
	{
		LOGFONTW lf;
		if (!app_key.QueryStruct(L"CellLogFont", lf)) {
			lf.lfHeight = m_lfCellLogFont.lfHeight;
			lf.lfWidth = m_lfCellLogFont.lfWidth;
			m_lfCellLogFont = lf;
		}
	}
	{
		LOGFONTW lf;
		if (!app_key.QueryStruct(L"SmallLogFont", lf)) {
			lf.lfHeight = m_lfSmallLogFont.lfHeight;
			lf.lfWidth = m_lfSmallLogFont.lfWidth;
			m_lfSmallLogFont = lf;
		}
	}
	{
		LOGFONTW lf;
		if (!app_key.QueryStruct(L"SmallLogFont2", lf)) {
			lf.lfHeight = m_lfSmallLogFont2.lfHeight;
			lf.lfWidth = m_lfSmallLogFont2.lfWidth;
			m_lfSmallLogFont2 = lf;
		}
	}

	if (!app_key.QueryDword(L"AddThickFrame", dwValue)) {
		m_bAddThickFrame = !!dwValue;
	}

	if (!app_key.QueryDword(L"WhiteCellColor", dwValue)) {
		m_rgbWhiteCellColor = dwValue;
	}
	if (!app_key.QueryDword(L"BlackCellColor", dwValue)) {
		m_rgbBlackCellColor = dwValue;
	}
	if (!app_key.QueryDword(L"MarkedCellColor", dwValue)) {
		m_rgbMarkedCellColor = dwValue;
	}

	if (!app_key.QueryDword(L"DrawFrameForMarkedCell", dwValue)) {
		m_bDrawFrameForMarkedCell = dwValue;
	}
	if (!app_key.QueryDword(L"ZoomRate", dwValue)) {
		m_nZoomRate = dwValue;
	}
	if (!app_key.QueryDword(L"ShowCaret", dwValue)) {
		m_bShowCaret = dwValue;
	}

	if (!app_key.QueryDword(L"Hiragana", dwValue)) {
		m_bHiragana = !!dwValue;
	}
	if (!app_key.QueryDword(L"Lowercase", dwValue)) {
		m_bLowercase = !!dwValue;
	}

	if (!app_key.QueryDword(L"CellCharPercents", dwValue)) {
		m_nCellCharPercents = dwValue;
	}
	if (!app_key.QueryDword(L"SmallCharPercents", dwValue)) {
		m_nSmallCharPercents = dwValue;
	}

	if (!app_key.QueryDword(L"ShowDoubleFrameLetters", dwValue)) {
		m_bShowDoubleFrameLetters = !!dwValue;
	}
	if (!app_key.QueryDword(L"ShowDoubleFrame", dwValue)) {
		m_bShowDoubleFrame = !!dwValue;
	}

	if (!app_key.QueryDword(L"ViewMode", dwValue)) {
		m_nViewMode = static_cast<XG_VIEW_MODE>(dwValue);
		if (m_nViewMode != XG_VIEW_NORMAL && m_nViewMode != XG_VIEW_SKELETON) {
			m_nViewMode = XG_VIEW_NORMAL;
		}
	}
	if (!app_key.QueryDword(L"LineWidth", dwValue)) {
		float value = static_cast<float>(dwValue) * 0.01f;
		if (value < XG_MIN_LINEWIDTH)
			value = XG_MIN_LINEWIDTH;
		if (value > XG_MAX_LINEWIDTH)
			value = XG_MAX_LINEWIDTH;
		m_nLineWidthInPt = value;
	}
	if (!app_key.QueryDword(L"OuterFrame", dwValue)) {
		float value = static_cast<float>(dwValue) * 0.01f;
		if (value < XG_MIN_OUTERFRAME)
			value = XG_MIN_OUTERFRAME;
		if (value > XG_MAX_OUTERFRAME)
			value = XG_MAX_OUTERFRAME;
		m_nOuterFrameInPt = value;
	}

	if (!app_key.QuerySz(L"BlackCellImage", sz, _countof(sz))) {
		SetBlackCellImage(sz, nullptr);
	}

	if (!app_key.QuerySz(L"DoubleFrameLetters", sz, _countof(sz))) {
		m_strDoubleFrameLetters = sz;
	}

	return TRUE;
}

BOOL MBoardDisplay::Save() const {
	// アプリ名キーを開く。キーがなければ作成する。
	MRegKey app_key(HKEY_CURRENT_USER, XG_REGKEY_APP, TRUE);
	if (!app_key)
		return FALSE;

	app_key.SetDword(L"Rows", m_board.Rows());
	app_key.SetDword(L"Cols", m_board.Columns());

	// 論理フォントを保存
	{
		LOGFONTW lf = m_lfCellLogFont;
		lf.lfHeight = lf.lfWidth = 0;
		app_key.SetStruct(L"CellLogFont", lf);
	}
	{
		LOGFONTW lf = m_lfSmallLogFont;
		lf.lfHeight = lf.lfWidth = 0;
		app_key.SetStruct(L"SmallLogFont", lf);
	}
	{
		LOGFONTW lf = m_lfSmallLogFont2;
		lf.lfHeight = lf.lfWidth = 0;
		app_key.SetStruct(L"SmallLogFont2", lf);
	}

	app_key.SetDword(L"AddThickFrame", m_bAddThickFrame);

	app_key.SetDword(L"BlackCellColor", m_rgbBlackCellColor);
	app_key.SetDword(L"WhiteCellColor", m_rgbWhiteCellColor);
	app_key.SetDword(L"MarkedCellColor", m_rgbMarkedCellColor);

	app_key.SetDword(L"DrawFrameForMarkedCell", m_bDrawFrameForMarkedCell);
	app_key.SetDword(L"ZoomRate", m_nZoomRate);
	app_key.SetDword(L"ShowCaret", m_bShowCaret);

	app_key.SetDword(L"Hiragana", m_bHiragana);
	app_key.SetDword(L"Lowercase", m_bLowercase);

	app_key.SetDword(L"CellCharPercents", m_nCellCharPercents);
	app_key.SetDword(L"SmallCharPercents", m_nSmallCharPercents);

	app_key.SetDword(L"ShowDoubleFrameLetters", m_bShowDoubleFrameLetters);
	app_key.SetDword(L"ShowDoubleFrame", m_bShowDoubleFrame);
	app_key.SetDword(L"ViewMode", m_nViewMode);
	app_key.SetDword(L"LineWidth", INT(m_nLineWidthInPt * 100));
	app_key.SetDword(L"OuterFrame", INT(m_nOuterFrameInPt * 100));

	app_key.SetSz(L"DoubleFrameLetters", m_strDoubleFrameLetters.c_str());

	if (m_strBlackCellImage.find(L"$BLOCK\\") == 0)
		app_key.SetSz(L"BlackCellImage", m_strBlackCellImage.c_str());

	return TRUE;
}

void MBoardDisplay::Close() {
	DeleteImages();
	DeleteFonts();
}

void MBoardDisplay::DeleteImages() {
	for (auto& image : m_images) {
		if (image.m_hBitmap) {
			DeleteObject(image.m_hBitmap);
			image.m_hBitmap = nullptr;
		}
		if (image.m_hEMF) {
			DeleteEnhMetaFile(image.m_hEMF);
			image.m_hEMF = nullptr;
		}
	}
	DeleteBlackCellImage();
}

void MBoardDisplay::DeleteFonts() {
	if (m_hCellFont) {
		::DeleteObject(m_hCellFont);
		m_hCellFont = nullptr;
	}
	if (m_hSmallFont) {
		::DeleteObject(m_hSmallFont);
		m_hSmallFont = nullptr;
	}
	if (m_hSmallFont2) {
		::DeleteObject(m_hSmallFont2);
		m_hSmallFont2 = nullptr;
	}
}

// ズーム倍率を設定。
void MBoardDisplay::SetZoom(INT nZoomRate) {
	m_nZoomRate = nZoomRate;
	m_nZoomedCellSize = (m_nCellSize * m_nZoomRate / 100);
	DeleteFonts();
}

// キャレット位置を修正。
void MBoardDisplay::FixCaretPos() {
	if (m_caret_pos.m_j >= m_board.Columns())
		m_caret_pos.m_j = m_board.Columns() - 1;
	if (m_caret_pos.m_j < 0)
		m_caret_pos.m_j = 0;
	if (m_caret_pos.m_i >= m_board.Rows())
		m_caret_pos.m_i = m_board.Rows() - 1;
	if (m_caret_pos.m_i < 0)
		m_caret_pos.m_i = 0;
}

void MBoardDisplay::CopyFrom(const MBoardDisplay& another) {
	if (this == &another)
		return;

	Close(); // 自分の古い画像とフォントを破棄する。

	*this = another;

	// 画像データを複製する(LR_COPYRETURNORG は使わない)。
	m_hBlackCellEMF = m_hBlackCellEMF ? CopyEnhMetaFileW(m_hBlackCellEMF, nullptr) : nullptr;
	m_hbmBlackCell = m_hbmBlackCell
		? (HBITMAP)CopyImage(m_hbmBlackCell, IMAGE_BITMAP, 0, 0, 0) : nullptr;
	for (auto& image : m_images) {
		if (image.m_hBitmap)
			image.m_hBitmap = (HBITMAP)CopyImage(image.m_hBitmap, IMAGE_BITMAP, 0, 0, 0);
		if (image.m_hEMF)
			image.m_hEMF = CopyEnhMetaFileW(image.m_hEMF, nullptr);
	}

	// フォントは、XgGetNormalFont、XgGetSmallFontなどの再作成にまかせる。
	m_hCellFont = nullptr;
	m_hSmallFont = nullptr;
	m_hSmallFont2 = nullptr;
}

// 「画像なし」を表す名前か？
BOOL XgIsNoneImageName(const MStringW& name) {
	return name.empty() || name == L"(None)" || name == L"(なし)";
}

void MBoardDisplay::DeleteBlackCellImage() {
	if (m_hBlackCellEMF) {
		DeleteEnhMetaFile(m_hBlackCellEMF);
		m_hBlackCellEMF = nullptr;
	}
	if (m_hbmBlackCell) {
		DeleteObject(m_hbmBlackCell);
		m_hbmBlackCell = nullptr;
	}
}

BOOL MBoardDisplay::CopyImageHandles(const MStringW& name, LPCWSTR subdir,
									 HBITMAP& hbm, HENHMETAFILE& hEMF)
{
	hbm = nullptr;
	hEMF = nullptr;
	if (!XgImportImageFile(*this, name, subdir))
		return FALSE;

	auto resolved = XgResolveImageFile(name, subdir);
	auto title = PathFindFileNameW(resolved.c_str());
	for (auto& image : m_images) {
		if (lstrcmpiW(PathFindFileNameW(image.m_filename.c_str()), title) != 0)
			continue;
		// LR_COPYRETURNORG を付けない。付けると元のハンドルが返ってしまう。
		if (image.m_hBitmap)
			hbm = (HBITMAP)CopyImage(image.m_hBitmap, IMAGE_BITMAP, 0, 0, 0);
		if (image.m_hEMF)
			hEMF = CopyEnhMetaFileW(image.m_hEMF, nullptr);
		break;
	}
	return hbm || hEMF;
}

BOOL MBoardDisplay::SetBlackCellImage(const MStringW& name, LPCWSTR subdir) {
	DeleteBlackCellImage();
	m_strBlackCellImage.clear();
	if (XgIsNoneImageName(name))
		return TRUE;
	if (!CopyImageHandles(name, subdir, m_hbmBlackCell, m_hBlackCellEMF))
		return FALSE;
	m_strBlackCellImage = name;
	return TRUE;
}

// ナンクロの写像が正当か確認する。
BOOL __fastcall XgValidateNumCro(MBoardDisplay& display) {
	if (!display.m_bSolved)
		return FALSE;

	auto& board = display.m_solution;
	for (INT i = 0; i < board.Rows(); ++i) {
		for (INT j = 0; j < board.Columns(); ++j) {
			const WCHAR ch = board.GetCell(j, i);
			if (ch == ZEN_SPACE || ch == ZEN_BLACK)
				continue;

			auto it = display.m_mapNumCro1.find(ch);
			if (it == display.m_mapNumCro1.end())
				return FALSE;
		}
	}

	for (auto& pair : display.m_mapNumCro1) {
		auto it = display.m_mapNumCro2.find(pair.second);
		if (it == display.m_mapNumCro2.end()) {
			return FALSE;
		}
	}

	return TRUE;
}

// ナンクロの写像を生成する。
void __fastcall XgMakeItNumCro(MBoardDisplay& display) {
	display.m_mapNumCro1.clear();
	display.m_mapNumCro2.clear();

	if (!display.m_bSolved)
		return;

	auto& board = display.m_solution;
	INT next_number = 1;
	for (INT i = 0; i < board.Rows(); ++i) {
		for (INT j = 0; j < board.Columns(); ++j) {
			WCHAR ch = board.GetCell(j, i);
			if (ch == ZEN_SPACE || ch == ZEN_BLACK)
				continue;

			auto it = display.m_mapNumCro1.find(ch);
			if (it == display.m_mapNumCro1.end()) {
				display.m_mapNumCro2[next_number] = ch;
				display.m_mapNumCro1[ch] = next_number;
				++next_number;
			} else {
				INT number = it->second;
				display.m_mapNumCro2[number] = ch;
				display.m_mapNumCro1[ch] = number;
			}
		}
	}

	if (!XgValidateNumCro(display)) {
		display.m_mapNumCro1.clear();
		display.m_mapNumCro2.clear();
		display.m_bNumCroMode = FALSE;
	} else {
		display.m_bNumCroMode = TRUE;
	}
}

#define XG_NARROW_MARGIN 8

// 二重マス単語のサイズを取得する。
SIZE __fastcall XgGetMarkWordExtent(MBoardDisplay& display, INT count) noexcept {
	SIZE siz = {
		INT(XG_NARROW_MARGIN * 2 + count * display.m_nCellSize),
		INT(XG_NARROW_MARGIN * 2 + 1 * display.m_nCellSize)
	};
	return siz;
}

// 二重マス単語を描画する。
void __fastcall XgDrawMarkWord(MBoardDisplay& display, HDC hdc, const SIZE *psiz) {
	auto nCount = INT(display.m_vMarks.size());
	if (nCount == 0)
		return;

	// ブラシを作成する。
	HBRUSH hbrBlack = ::CreateSolidBrush(display.m_rgbBlackCellColor);
	HBRUSH hbrWhite = ::CreateSolidBrush(display.m_rgbWhiteCellColor);
	HBRUSH hbrMarked = ::CreateSolidBrush(display.m_rgbMarkedCellColor);

	// 線の太さ。
	int c_nThin = static_cast<int>(YPixelsFromPoints(hdc, display.m_nLineWidthInPt));
	int c_nWide = static_cast<int>(YPixelsFromPoints(hdc, display.m_nOuterFrameInPt));
	if (c_nThin < 2)
		c_nThin = 2;
	if (c_nWide < 2)
		c_nWide = 2;
	if (c_nWide > XG_NARROW_MARGIN)
		c_nWide = XG_NARROW_MARGIN;

	// セルの幅。
	auto nCellSize = display.m_nCellSize;
	display.m_nZoomedCellSize = display.m_nCellSize;

	// 黒いブラシ。
	LOGBRUSH lbBlack;
	lbBlack.lbStyle = BS_SOLID;
	lbBlack.lbColor = display.m_rgbBlackCellColor;

	// 細いペンを作成し、選択する。
	HPEN hThinPen = ::ExtCreatePen(
		PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
		c_nThin, &lbBlack, 0, nullptr);

	// 文字マスのフォントを作成する。
	HFONT hFontNormal = XgGetNormalFont(display);

	// 小さい文字のフォントを作成する。
	HFONT hFontSmall2 = XgGetSmallFont2(display);

	// 全体を白で塗りつぶす。
	RECT rc0 = { 0, 0, psiz->cx, psiz->cy };
	::FillRect(hdc, &rc0, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

	// 周りに太い線を描く。
	if (display.m_bAddThickFrame) {
		::InflateRect(&rc0, -c_nWide, -c_nWide);
		::FillRect(hdc, &rc0, hbrBlack);
		::InflateRect(&rc0, +c_nWide, +c_nWide);
	}

	auto& board = XgGetDisplayBoard(display);

	// 二重マスを描画する。
	HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall2);
	for (INT i = 0; i < nCount; ++i) {
		RECT rc = {
			XG_NARROW_MARGIN + i * nCellSize,
			XG_NARROW_MARGIN,
			XG_NARROW_MARGIN + (i + 1) * nCellSize,
			XG_NARROW_MARGIN + nCellSize
		};

		// ナンクロなら白。さもなければ二重マスの色。
		if (display.m_bNumCroMode) {
			::FillRect(hdc, &rc, hbrWhite);
		} else {
			::FillRect(hdc, &rc, hbrMarked);
		}

		const MBoardPos& pos = display.m_vMarks[i];

		// ナンクロなら数字、さもなければ二重マスの文字を描画する。
		if (display.m_bSolved && display.m_bNumCroMode) {
			const WCHAR ch = display.m_solution.GetCell(pos.m_j, pos.m_i);
			::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);
			XgDrawCellNumber(display, hdc, rc, pos.m_j, pos.m_i, display.m_mapNumCro1[ch]);
		} else {
			XgDrawDoubleFrameCell(display, hdc, i, rc, hThinPen, -1, -1);
		}
	}
	::SelectObject(hdc, hFontOld);

	// 塗りつぶさない。
	::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));

	// マスの文字とセルの枠を描画する。
	HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
	HGDIOBJ hFontOld2 = ::SelectObject(hdc, hFontNormal);
	::SetTextColor(hdc, display.m_rgbBlackCellColor);
	::SetBkMode(hdc, TRANSPARENT);

	const UINT uOldAlign = ::SetTextAlign(hdc, TA_CENTER | TA_TOP);
	TEXTMETRICW tm = {};
	::GetTextMetricsW(hdc, &tm);
	const INT cyHalf = tm.tmHeight / 2;

	for (INT i = 0; i < nCount; ++i) {
		RECT rc = {
			XG_NARROW_MARGIN + i * nCellSize,
			XG_NARROW_MARGIN,
			XG_NARROW_MARGIN + (i + 1) * nCellSize,
			XG_NARROW_MARGIN + nCellSize
		};

		const MBoardPos& pos = display.m_vMarks[i];
		const WCHAR ch = XgReplaceChar(display, board.GetCell(pos.m_j, pos.m_i));
		::TextOutW(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cyHalf, &ch, 1);

		// 枠線
		::MoveToEx(hdc, rc.left, rc.top, nullptr);
		::LineTo(hdc, rc.right, rc.top);
		::LineTo(hdc, rc.right, rc.bottom);
		::LineTo(hdc, rc.left, rc.bottom);
		::LineTo(hdc, rc.left, rc.top);
	}
	::SetTextAlign(hdc, uOldAlign);
	::SelectObject(hdc, hFontOld2);
	::SelectObject(hdc, hPenOld);

	::DeleteObject(hThinPen);
	::DeleteObject(hbrBlack);
	::DeleteObject(hbrWhite);
	::DeleteObject(hbrMarked);
}
