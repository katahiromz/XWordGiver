// MCandsWnd.h --- 「候補」ウィンドウ
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MScrollView.h"
#include "pat-match.h"

class MCandsWnd;

struct MCandsButtonData {
	MCandsWnd* m_pThis;
	WNDPROC m_fnOldWndProc;
};

// 候補ウィンドウ。
class MCandsWnd : public MWindowBase {
public:
	enum { c_nMaxCandidates = 256};

	// 候補ウィンドウの位置とサイズ。
	inline static INT s_nCandsWndX = CW_USEDEFAULT, s_nCandsWndY = CW_USEDEFAULT;
	inline static INT s_nCandsWndCX = CW_USEDEFAULT, s_nCandsWndCY = CW_USEDEFAULT;

	MDict&					 m_dict; // 辞書。
	MBoardDisplay&			 m_display;
	MBoard&					 m_board; // 盤面。
	MBoardPos&				 m_caret_pos; // キャレット位置。
	LOGFONTW&                m_lfCandsLogFont; // 論理フォント。
	MScrollView				 m_svCandsScrollView; // 候補ウィンドウのスクロールビュー。
	HFONT					 m_hCandsUIFont = nullptr; // 候補ウィンドウのUIフォント。
	std::vector<MStringW>	 m_vecCandidates; // 候補群。
	std::vector<HWND>		 m_ahwndCandButtons; // 候補ボタン群。
	BOOL					 m_bCandVertical; // 候補はタテかヨコか。
	INT						 m_jCandPos, m_iCandPos; // 候補を求める位置。

	MCandsWnd(MDict& dict, MBoardDisplay& display, LOGFONTW& lfCandsLogFont) noexcept
		: m_dict(dict)
		, m_display(display)
		, m_lfCandsLogFont(lfCandsLogFont)
		, m_board(display.m_board)
		, m_caret_pos(display.m_caret_pos)
	{
	}

	LPCTSTR GetWndClassNameDx() const override {
		return TEXT("MCandsWnd");
	}

	void ModifyWndClassDx(WNDCLASSEX& wcx) override {
		wcx.hIcon = nullptr;
		wcx.hbrBackground = ::CreateSolidBrush(RGB(255, 255, 192));
	}

	// 候補ウィンドウを作成する。
	BOOL CreateDx(HWND hwnd) {
		auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
		auto exstyle = WS_EX_TOOLWINDOW;
		auto text = XgLoadString(IDS_CANDIDATES);
		return !!CreateWindowDx(hwnd, text.c_str(), style, exstyle, s_nCandsWndX, s_nCandsWndY, s_nCandsWndCX, s_nCandsWndCY);
	}

	// 開く。
	BOOL Open(HWND hwnd, BOOL vertical) {
		// 候補を作成する。
		m_jCandPos = m_caret_pos.m_j;
		m_iCandPos = m_caret_pos.m_i;
		m_bCandVertical = vertical;
		if (m_board.GetCell(m_jCandPos, m_iCandPos) == ZEN_BLACK) {
			return FALSE;
		}

		// パターンを取得する。
		INT lo, hi;
		MStringW pattern;
		BOOL left_black, right_black;
		if (m_bCandVertical) {
			lo = hi = m_iCandPos;
			while (lo > 0) {
				if (m_board.GetCell(m_jCandPos, lo - 1) != ZEN_BLACK)
					--lo;
				else
					break;
			}
			while (hi + 1 < m_board.Rows()) {
				if (m_board.GetCell(m_jCandPos, hi + 1) != ZEN_BLACK)
					++hi;
				else
					break;
			}

			for (INT i = lo; i <= hi; ++i) {
				pattern += m_board.GetCell(m_jCandPos, i);
			}

			right_black = (hi + 1 != m_board.Rows());
		} else {
			lo = hi = m_jCandPos;
			while (lo > 0) {
				if (m_board.GetCell(lo - 1, m_iCandPos) != ZEN_BLACK)
					--lo;
				else
					break;
			}
			while (hi + 1 < m_board.Columns()) {
				if (m_board.GetCell(hi + 1, m_iCandPos) != ZEN_BLACK)
					++hi;
				else
					break;
			}

			for (INT j = lo; j <= hi; ++j) {
				pattern += m_board.GetCell(j, m_iCandPos);
			}

			right_black = (hi + 1 != m_board.Columns());
		}
		left_black = (lo != 0);

		// 候補を取得する。
		INT nSkip = 0;
		std::vector<MStringW> cands;
		XgGetCandidatesAddBlack(m_dict, cands, pattern, nSkip, left_black, right_black);

		// 正規化して右側の空白を取り除く。
		for (auto& cand : cands) {
			cand = XgNormalizeString(cand);
			XgStrTrimRight(cand);
		}

		// ソートして一意化。短い方を優先する。
		std::sort(cands.begin(), cands.end(), [](const MStringW& x, const MStringW& y) noexcept {
			if (x.size() == y.size())
				return x < y;
			return x.size() < y.size();
		});
		cands.erase(std::unique(cands.begin(), cands.end()), cands.end());

		// 仮に適用して、正当かどうか確かめ、正当なものだけを
		// 最終的な候補とする。
		m_vecCandidates.clear();
		for (auto& cand : cands) {
			MBoard board(m_board);
			ApplyCandidate(board, cand);
			if (XgRuleCheck(board, TRUE)) {
				m_vecCandidates.emplace_back(cand);
			}
		}

		// 個数制限。
		if (m_vecCandidates.size() > c_nMaxCandidates)
			m_vecCandidates.resize(c_nMaxCandidates);

		if (m_vecCandidates.empty()) {
			return false;
		}

		// ヒントウィンドウを作成する。
		if (CreateDx(hwnd)) {
			::ShowWindow(*this, SW_SHOWNORMAL);
			::UpdateWindow(*this);
			return true;
		}
		return false;
	}

	LRESULT CALLBACK
	XgCandsButton_WndProcDx(MCandsButtonData* data, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_CHAR:
			return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

		case WM_SETFOCUS:
			m_svCandsScrollView.EnsureCtrlVisible(hwnd);
			return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

		case WM_KEYDOWN:
			if (wParam == VK_RETURN) {
				::SetFocus(nullptr);
				break; // ここで処理しなくても WM_CHARで処理される。
			}

			if (wParam == VK_PRIOR || wParam == VK_NEXT) {
				HWND hwndParent = ::GetParent(hwnd);
				::SendMessageW(hwndParent, uMsg, wParam, lParam);
				break;
			}

			if (wParam == VK_F6) {
				if (::GetAsyncKeyState(VK_SHIFT) < 0)
					::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
				else
					::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
				break;
			}

			if (wParam == VK_ESCAPE) {
				DestroyWindow(GetParent(hwnd));
				break;
			}

			return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

		case WM_NCDESTROY:
			{
				WNDPROC fn = data->m_fnOldWndProc;
				::LocalFree(data);
				return ::CallWindowProc(fn, hwnd, uMsg, wParam, lParam);
			}

		case WM_MOUSEWHEEL:
			{
				HWND hwndParent = ::GetParent(hwnd);
				::SendMessageW(hwndParent, uMsg, wParam, lParam);
			}
			break;

		default:
			return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
		}
		return 0;
	}

	static LRESULT CALLBACK
	XgCandsButton_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		auto data = reinterpret_cast<MCandsButtonData*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
		if (data && data->m_pThis) {
			return data->m_pThis->XgCandsButton_WndProcDx(data, hwnd, uMsg, wParam, lParam);
		} else {
			return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	}

	// 候補ウィンドウのサイズが変わった。
	void OnSize(HWND hwnd, UINT /*state*/, INT /*cx*/, INT /*cy*/) {
		if (m_ahwndCandButtons.empty())
			return;

		m_svCandsScrollView.clear();

		MRect rcClient;
		::GetClientRect(hwnd, &rcClient);

		HDC hdc = ::CreateCompatibleDC(nullptr);
		HGDIOBJ hFontOld = ::SelectObject(hdc, m_hCandsUIFont);
		{
			MPoint pt;
			for (size_t i = 0; i < m_vecCandidates.size(); ++i) {
				const MStringW& strLabel = m_vecCandidates[i];

				MSize siz;
				::GetTextExtentPoint32W(hdc, strLabel.data(), INT(strLabel.size()), &siz);

				if (pt.x != 0 && pt.x + siz.cx + 16 > rcClient.Width()) {
					pt.x = 0;
					pt.y += siz.cy + 16;
				}

				const MRect rcCtrl(MPoint(pt.x + 4, pt.y + 4), MSize(siz.cx + 8, siz.cy + 8));
				m_svCandsScrollView.AddCtrlInfo(m_ahwndCandButtons[i], rcCtrl);

				pt.x += siz.cx + 16;
			}
		}
		::SelectObject(hdc, hFontOld);
		::DeleteDC(hdc);

		m_svCandsScrollView.SetExtentForAllCtrls(8, 8);
		m_svCandsScrollView.EnsureCtrlVisible(::GetFocus(), false);
		m_svCandsScrollView.UpdateAll();

		// 再描画。
		RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
	}

	// 候補ウィンドウが作成された。
	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/) {
		if (m_hCandsUIFont)
			::DeleteObject(m_hCandsUIFont);
		m_hCandsUIFont = CreateFontIndirectW(&m_lfCandsLogFont);

		// 初期化。
		m_ahwndCandButtons.clear();
		m_svCandsScrollView.clear();

		m_svCandsScrollView.SetParent(hwnd);
		m_svCandsScrollView.ShowScrollBars(FALSE, TRUE);

		HWND hwndCtrl;
		MCandsButtonData* data;
		WCHAR text[128];
		for (auto& cand : m_vecCandidates) {
			if (m_display.m_bHiragana) {
				LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
							 cand.data(), -1, text, _countof(text));
				cand = text;
			}
			if (m_display.m_bLowercase) {
				LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE,
							 cand.data(), -1, text, _countof(text));
				cand = text;
			}

			hwndCtrl = ::CreateWindowW(TEXT("BUTTON"), cand.data(),
									   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
									   0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
			assert(hwndCtrl);
			if (!hwndCtrl)
				return FALSE;

			::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(m_hCandsUIFont), FALSE);
			m_ahwndCandButtons.emplace_back(hwndCtrl);

			data = static_cast<MCandsButtonData*>(::LocalAlloc(LMEM_FIXED, sizeof(MCandsButtonData)));
			data->m_pThis = this;
			data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
				::SetWindowLongPtrW(hwndCtrl, GWLP_WNDPROC,
					reinterpret_cast<LONG_PTR>(XgCandsButton_WndProc)));
			::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
		}
		OnSize(hwnd, 0, 0, 0);

		if (m_ahwndCandButtons.size())
			::SetFocus(m_ahwndCandButtons[0]);
		else
			::SetFocus(hwnd);

		return TRUE;
	}

	// 候補ウィンドウが縦にスクロールされた。
	void OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, INT pos) noexcept {
		m_svCandsScrollView.Scroll(SB_VERT, code, pos);
	}

	// キーが押された。
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, INT cRepeat, UINT flags) noexcept {
		if (!fDown)
			return;

		switch (vk) {
		case VK_PRIOR:
			::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEUP, 0), 0);
			break;

		case VK_NEXT:
			::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEDOWN, 0), 0);
			break;

		case VK_TAB:
			if (m_ahwndCandButtons.size())
				::SetFocus(m_ahwndCandButtons[0]);
			break;

		case VK_F6:
			if (::GetAsyncKeyState(VK_SHIFT) < 0)
				::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
			else
				::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
			break;

		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;

		default:
			break;
		}
	}

	// 候補ウィンドウが破棄された。
	void OnDestroy(HWND hwnd) noexcept {
		// 現在の位置とサイズを保存する。
		MRect rc;
		::GetWindowRect(hwnd, &rc);
		s_nCandsWndX = rc.left;
		s_nCandsWndY = rc.top;
		s_nCandsWndCX = rc.Width();
		s_nCandsWndCY = rc.Height();

		m_ahwndCandButtons.clear();
		m_svCandsScrollView.clear();

		::DeleteObject(m_hCandsUIFont);
		m_hCandsUIFont = nullptr;

		SetForegroundWindow(xg_hMainWnd);
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) {
		if (codeNotify == BN_CLICKED) {
			for (size_t i = 0; i < m_ahwndCandButtons.size(); ++i) {
				if (m_ahwndCandButtons[i] == hwndCtl) {
					// 候補を適用する。
					::SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
					ApplyCandidate(m_board, m_vecCandidates[i]);
					::SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
					// 候補ウィンドウを破棄する。
					DestroyWindow(m_hwnd);
					return;
				}
			}
		}
	}

	// ズーム。
	void OnZoom(HWND hwnd, BOOL bZoomIn) {
		// フォント情報を取得。
		LOGFONTW *plf = &m_lfCandsLogFont;
		LONG height = labs(plf->lfHeight);
		if (height == 0)
			height = 12;

		HDC hdc = ::CreateCompatibleDC(nullptr);
		INT pointsize = MulDiv(height, 72, ::GetDeviceCaps(hdc, LOGPIXELSY));

		// ズームに応じて、フォントサイズを増減。
		if (bZoomIn)
			pointsize += 1;
		else
			pointsize -= 1;

		// 大きすぎない。小さすぎない。
		if (pointsize > 30)
			pointsize = 30;
		if (pointsize < 6)
			pointsize = 6;

		// フォント設定を更新。
		m_lfCandsLogFont.lfHeight = -MulDiv(pointsize, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
		::DeleteDC(hdc);

		// フォントを再作成。
		if (m_hCandsUIFont)
			::DeleteObject(m_hCandsUIFont);
		m_hCandsUIFont = ::CreateFontIndirectW(&m_lfCandsLogFont);

		// 再描画をOFFにする。
		SetWindowRedraw(hwnd, FALSE);

		// フォントをセット。
		for (auto hwndCtrl : m_ahwndCandButtons)
			SetWindowFont(hwndCtrl, m_hCandsUIFont, FALSE);

		// 再描画をONにする。
		SetWindowRedraw(hwnd, TRUE);

		// 再配置。
		::PostMessageW(hwnd, WM_SIZE, 0, 0);
	}

	// マウスホイールが回転した。
	void OnMouseWheel(HWND hwnd, INT xPos, INT yPos, INT zDelta, UINT fwKeys) noexcept {
		if (::GetAsyncKeyState(VK_CONTROL) < 0) {
			if (zDelta < 0) {
				OnZoom(hwnd, FALSE);
			} else if (zDelta > 0) {
				OnZoom(hwnd, TRUE);
			}
		} else if (::GetAsyncKeyState(VK_SHIFT) < 0) {
			if (zDelta < 0)
				::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
			else if (zDelta > 0)
				::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
		} else {
			if (zDelta < 0)
				::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
			else if (zDelta > 0)
				::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
		}
	}

	// 候補ウィンドウのサイズを制限する。
	void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept {
		lpMinMaxInfo->ptMinTrackSize.x = 256;
		lpMinMaxInfo->ptMinTrackSize.y = 128;
	}

	// 候補ウィンドウを描画する。
	void OnPaint(HWND hwnd) noexcept {
		if (m_vecCandidates.empty()) {
			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(hwnd, &ps);
			if (hdc) {
				MRect rcClient;
				::GetClientRect(hwnd, &rcClient);
				::SetTextColor(hdc, RGB(0, 0, 0));
				::SetBkMode(hdc, TRANSPARENT);
				::DrawTextW(hdc, XgLoadString(IDS_NOCANDIDATES).c_str(), -1, &rcClient,
							DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
				::EndPaint(hwnd, &ps);
			}
		} else {
			FORWARD_WM_PAINT(hwnd, DefWindowProcW);
		}
	}

	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);
		HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
		HANDLE_MSG(hwnd, WM_KEYUP, OnKey);
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
		HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
		default:
			return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
		return 0;
	}

	// 候補を適用する。
	void ApplyCandidate(MBoard& board, const MStringW& strCand) {
		MStringW cand = XgNormalizeString(strCand);
		XgStrTrimRight(cand);   // 念のため

		XgApplyCandidate(board, m_jCandPos, m_iCandPos, cand, m_bCandVertical);
	}
};
