// MCluesWnd.cpp --- [ヒント]ウィンドウ
// Author: katahiromz
// License: MIT

#include "MCluesWnd.h"

BOOL MCluesWnd::CreateDx(HWND hwndOwner) {
	if (m_nWindowX == CW_USEDEFAULT) {
		AdjustPosInfo();
	}

	const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
	const DWORD exstyle = WS_EX_TOOLWINDOW;
	auto text = XgLoadString(IDS_HINTS);
	if (!CreateWindowDx(hwndOwner, text.c_str(), style, exstyle, m_nWindowX, m_nWindowY, m_nWindowCX, m_nWindowCY))
		return FALSE;

	ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
	UpdateWindow(m_hwnd);
	return TRUE;
}

// ハイライトを更新する。
void MCluesWnd::SetHighlight(INT nHorz, INT nVert) noexcept {
	HWND hwndVertOld = m_hwndHighlightVertEdit;
	HWND hwndHorzOld = m_hwndHighlightHorzEdit;
	HWND hwndVert = nullptr, hwndHorz = nullptr;

	if (nVert != -1) {
		assert(m_vVertInfo.size() == m_ahwndVertEdits.size());
		for (size_t i = 0; i < m_vVertInfo.size(); ++i) {
			if (m_vVertInfo[i].m_number == nVert) {
				hwndVert = m_ahwndVertEdits[i];
				break;
			}
		}
	}
	if (nHorz != -1) {
		assert(m_vHorzInfo.size() == m_ahwndHorzEdits.size());
		for (size_t j = 0; j < m_vHorzInfo.size(); ++j) {
			if (m_vHorzInfo[j].m_number == nHorz) {
				hwndHorz = m_ahwndHorzEdits[j];
				break;
			}
		}
	}

	m_hwndHighlightVertEdit = hwndVert;
	m_hwndHighlightHorzEdit = hwndHorz;

	if (hwndVert)
		::InvalidateRect(hwndVert, nullptr, TRUE);
	if (hwndHorz)
		::InvalidateRect(hwndHorz, nullptr, TRUE);
	if (hwndVertOld)
		::InvalidateRect(hwndVertOld, nullptr, TRUE);
	if (hwndHorzOld)
		::InvalidateRect(hwndHorzOld, nullptr, TRUE);
}

// ハイライトに背景色を付ける。
HBRUSH MCluesWnd::OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type) noexcept {
	if (hwndChild == m_hwndTrackMenuOn) {
		auto rgbColor = RGB(255, 140, 255);
		SetBkColor(hdc, rgbColor);
		SetDCBrushColor(hdc, rgbColor);
		return GetStockBrush(DC_BRUSH);
	} else if (hwndChild == m_hwndHighlightVertEdit || hwndChild == m_hwndHighlightHorzEdit) {
		auto rgbColor = RGB(140, 255, 255);
		SetBkColor(hdc, rgbColor);
		SetDCBrushColor(hdc, rgbColor);
		return GetStockBrush(DC_BRUSH);
	} else if (hwndChild == GetFocus()) {
		auto rgbColor = RGB(255, 255, 140);
		SetBkColor(hdc, rgbColor);
		SetDCBrushColor(hdc, rgbColor);
		return GetStockBrush(DC_BRUSH);
	} else {
		return FORWARD_WM_CTLCOLOREDIT(hwnd, hdc, hwndChild, ::DefWindowProcW);
	}
}

// ヒントが変更されたか？
BOOL MCluesWnd::IsModified() noexcept {
	if (m_hwnd) {
		for (size_t i = 0; i < m_ahwndVertEdits.size(); ++i) {
			if (::SendMessageW(m_ahwndVertEdits[i], EM_GETMODIFY, 0, 0)) {
				return TRUE;
			}
		}
		for (size_t j = 0; j < m_ahwndHorzEdits.size(); ++j) {
			if (::SendMessageW(m_ahwndHorzEdits[j], EM_GETMODIFY, 0, 0)) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

// ヒントデータを設定する。
void MCluesWnd::SetHintsData() noexcept {
	assert(m_vVertInfo.size() == m_ahwndVertEdits.size());
	for (size_t i = 0; i < m_vVertInfo.size(); ++i) {
		::SetWindowTextW(m_ahwndVertEdits[i], m_word2clue[m_vVertInfo[i].m_word].c_str());
	}
	assert(m_vHorzInfo.size() == m_ahwndHorzEdits.size());
	for (size_t i = 0; i < m_vHorzInfo.size(); ++i) {
		::SetWindowTextW(m_ahwndHorzEdits[i], m_word2clue[m_vHorzInfo[i].m_word].c_str());
	}
}

// ヒントデータを更新する。
BOOL MCluesWnd::UpdateHintData() {
	if (!m_hwnd)
		return FALSE;

	WCHAR sz[512];
	BOOL updated = FALSE;

	::SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_CLUES, 0);
	for (size_t i = 0; i < m_ahwndVertEdits.size(); ++i) {
		if (::SendMessageW(m_ahwndVertEdits[i], EM_GETMODIFY, 0, 0)) {
			updated = TRUE;
			::GetWindowTextW(m_ahwndVertEdits[i], sz, INT(_countof(sz)));
			m_word2clue[m_vVertInfo[i].m_word] = sz;
			::SendMessageW(m_ahwndVertEdits[i], EM_SETMODIFY, FALSE, 0);
		}
	}
	for (size_t i = 0; i < m_ahwndHorzEdits.size(); ++i) {
		if (::SendMessageW(m_ahwndHorzEdits[i], EM_GETMODIFY, 0, 0)) {
			updated = TRUE;
			::GetWindowTextW(m_ahwndHorzEdits[i], sz, INT(_countof(sz)));
			m_word2clue[m_vHorzInfo[i].m_word] = sz;
			::SendMessageW(m_ahwndHorzEdits[i], EM_SETMODIFY, FALSE, 0);
		}
	}
	::SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_CLUES, !updated);

	return updated;
}

void MCluesWnd::LayoutAcross(HDC hdc, INT& y, MRect& rcClient, MSize& size1, MSize& size2) {
	WCHAR szText[512];
	const MRect rcCtrl(MPoint(0, y + 4), MSize(rcClient.Width(), size1.cy + 4));
	m_svHintsScrollView.AddCtrlInfo(m_hwndHorzCaptionStatic, rcCtrl);
	y += size1.cy + 8;
	if (rcClient.Width() - size2.cx - 8 > 0) {
		for (size_t i = 0; i < m_ahwndHorzEdits.size(); ++i) {
			::GetWindowTextW(m_ahwndHorzEdits[i], szText, _countof(szText));
			MRect rcCtrl(MPoint(size2.cx, y), MSize(rcClient.Width() - size2.cx - 8, 0));
			if (szText[0] == 0) {
				szText[0] = L' ';
				szText[1] = 0;
			}
			::DrawTextW(hdc, szText, -1, &rcCtrl, DT_LEFT | DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK);
			rcCtrl.right = rcClient.right;
			rcCtrl.bottom += 8;
			m_svHintsScrollView.AddCtrlInfo(m_ahwndHorzEdits[i], rcCtrl);
			rcCtrl.left = 0;
			rcCtrl.right = size2.cx;
			m_svHintsScrollView.AddCtrlInfo(m_ahwndHorzStatics[i], rcCtrl);
			y += rcCtrl.Height();
		}
	}
}

void MCluesWnd::LayoutDown(HDC hdc, INT& y, MRect& rcClient, MSize& size1, MSize& size2) {
	WCHAR szText[512];
	const MRect rcCtrl(MPoint(0, y + 4), MSize(rcClient.Width(), size1.cy + 4));
	m_svHintsScrollView.AddCtrlInfo(m_hwndVertCaptionStatic, rcCtrl);
	y += size1.cy + 8;
	if (rcClient.Width() - size2.cx - 8 > 0) {
		for (size_t i = 0; i < m_ahwndVertEdits.size(); ++i) {
			::GetWindowTextW(m_ahwndVertEdits[i], szText, _countof(szText));
			MRect rcCtrl(MPoint(size2.cx, y), MSize(rcClient.Width() - size2.cx - 8, 0));
			if (szText[0] == 0) {
				szText[0] = L' ';
				szText[1] = 0;
			}
			::DrawTextW(hdc, szText, -1, &rcCtrl, DT_LEFT | DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK);
			rcCtrl.right = rcClient.right;
			rcCtrl.bottom += 8;
			m_svHintsScrollView.AddCtrlInfo(m_ahwndVertEdits[i], rcCtrl);
			rcCtrl.left = 0;
			rcCtrl.right = size2.cx;
			m_svHintsScrollView.AddCtrlInfo(m_ahwndVertStatics[i], rcCtrl);
			y += rcCtrl.Height();
		}
	}
}

// WM_SIZE
void MCluesWnd::OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/) {
	if (m_hwndVertCaptionStatic == nullptr)
		return;

	m_svHintsScrollView.clear();

	MRect rcClient;
	::GetClientRect(hwnd, &rcClient);

	MSize size1, size2;
	{
		HDC hdc = ::CreateCompatibleDC(nullptr);
		WCHAR label[64];
		StringCchPrintfW(label, _countof(label), XgLoadString(IDS_DOWNNUMBER).c_str(), 100);
		MStringW strLabel = label;
		::SelectObject(hdc, ::GetStockObject(SYSTEM_FIXED_FONT));
		::GetTextExtentPoint32W(hdc, strLabel.data(), INT(strLabel.size()), &size1);
		::SelectObject(hdc, m_hCluesFont);
		::GetTextExtentPoint32W(hdc, strLabel.data(), INT(strLabel.size()), &size2);
		::DeleteDC(hdc);
	}

	HDC hdc = ::CreateCompatibleDC(nullptr);
	HGDIOBJ hFontOld = ::SelectObject(hdc, m_hCluesFont);
	INT y = 0;

	if (XgIsUserJapanese()) {
		// タテのカギ。
		LayoutDown(hdc, y, rcClient, size1, size2);
		// ヨコのカギ。
		LayoutAcross(hdc, y, rcClient, size1, size2);
	} else {
		// ヨコのカギ。
		LayoutAcross(hdc, y, rcClient, size1, size2);
		// タテのカギ。
		LayoutDown(hdc, y, rcClient, size1, size2);
	}

	::SelectObject(hdc, hFontOld);
	::DeleteDC(hdc);

	m_svHintsScrollView.SetExtentForAllCtrls();
	m_svHintsScrollView.UpdateAll();

	// 再描画。
	RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

/*static*/ LRESULT CALLBACK MCluesWnd::XgHintEditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	WNDPROC fn;
	auto data = reinterpret_cast<MClueEditData*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	auto pParent = data->m_pParent;

	switch (uMsg) {
	case WM_CHAR:
		if (wParam == L'\t') { // Tabが押された？
			return 0; // 無視する。
		}
		if (wParam == L'\r' || wParam == L'\n') {
			// 改行が押された。必要ならばカギを更新する。
			if (pParent->IsModified()) {
				pParent->UpdateHintData();
				::SendMessageW(hwnd, EM_SETMODIFY, FALSE, 0);
				return 0;
			}
		}
		return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

	case WM_SETFOCUS: // フォーカスを得た。
		if (wParam) {
			// フォーカスを失うコントロールの選択を解除する。
			HWND hwndLoseFocus = HWND(wParam);
			::SendMessageW(hwndLoseFocus, EM_SETSEL, 0, 0);
		}
		// ハイライト。
		{
			bool found = false, vertical = false;
			size_t i = 0;
			if (!found) {
				assert(pParent->m_ahwndVertEdits.size() == pParent->m_vVertInfo.size());
				for (auto& edit : pParent->m_ahwndVertEdits) {
					if (edit == hwnd) {
						found = true;
						vertical = true;
						break;
					}
					++i;
				}
			}
			if (!found) {
				i = 0;
				assert(pParent->m_ahwndHorzEdits.size() == pParent->m_vHorzInfo.size());
				for (auto& edit : pParent->m_ahwndHorzEdits) {
					if (edit == hwnd) {
						found = true;
						vertical = false;
						break;
					}
					++i;
				}
			}
			if (found) {
				const auto number = (vertical ? pParent->m_vVertInfo[i].m_number : pParent->m_vHorzInfo[i].m_number);
				PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT1, TRUE, MAKELPARAM(number, vertical));
			}
		}

		// フォーカスのあるコントロールが見えるように移動する。
		pParent->m_svHintsScrollView.EnsureCtrlVisible(hwnd);

		// フォーカスを失うコントロールを再描画する。
		if (wParam)
			::InvalidateRect(HWND(wParam), nullptr, TRUE);

		return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

	case WM_KILLFOCUS:  // フォーカスを失った。
		// ヒントに更新があれば、データを更新し、レイアウトを調整する。
		if (pParent->IsModified()) {
			pParent->UpdateHintData();
			::PostMessageW(*pParent, WM_SIZE, 0, 0);
		}

		// ハイライトを当てる。
		PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT1, TRUE, MAKELPARAM(-1, false));

		// フォーカスを失うコントロールを再描画する。
		if (wParam)
			::InvalidateRect(HWND(wParam), nullptr, TRUE);

		return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			::PostMessageW(*pParent, WM_SIZE, 0, 0);
			::SetFocus(nullptr);
			break;
		}

		if (wParam == VK_TAB) {
			HWND hwndNext;
			if (::GetAsyncKeyState(VK_SHIFT) < 0)
				hwndNext = ::GetNextDlgTabItem(*pParent, hwnd, TRUE);
			else
				hwndNext = ::GetNextDlgTabItem(*pParent, hwnd, FALSE);
			::SendMessageW(hwnd, EM_SETSEL, 0, 0);
			::SendMessageW(hwndNext, EM_SETSEL, 0, -1);
			::SetFocus(hwndNext);
			break;
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

		return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

	case WM_NCDESTROY:
		fn = data->m_fnOldWndProc;
		::LocalFree(data);
		return ::CallWindowProc(fn, hwnd, uMsg, wParam, lParam);

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

void MCluesWnd::DestroyArray(std::vector<HWND>& ahwndCtrls) {
	for (HWND& hwndCtrl : ahwndCtrls) {
		DestroyCtrl(hwndCtrl);
	}
	ahwndCtrls.clear();
}

void MCluesWnd::DestroyCtrl(HWND& hwndCtrl) {
	if (hwndCtrl) {
		::DestroyWindow(hwndCtrl);
		hwndCtrl = nullptr;
	}
}

void MCluesWnd::DestroyControls() {
	DestroyCtrl(m_hwndVertCaptionStatic);
	DestroyCtrl(m_hwndHorzCaptionStatic);
	DestroyArray(m_ahwndVertStatics);
	DestroyArray(m_ahwndVertEdits);
	DestroyArray(m_ahwndHorzStatics);
	DestroyArray(m_ahwndHorzEdits);
	m_svHintsScrollView.clear();
	m_hwndHighlightVertEdit = nullptr; // 所有権はない。
	m_hwndHighlightHorzEdit = nullptr; // 所有権はない。
}

BOOL MCluesWnd::CreateControls(HWND hwnd) {
	HWND hwndCtrl;

	// タテのカギのタイトル。
	DWORD style = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY | SS_CENTER | SS_CENTERIMAGE;
	hwndCtrl = ::CreateWindowW(TEXT("STATIC"), XgLoadString(IDS_DOWN).c_str(), style, 0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
	if (hwndCtrl == nullptr)
		return FALSE;
	m_hwndVertCaptionStatic = hwndCtrl;
	::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(::GetStockObject(SYSTEM_FIXED_FONT)), TRUE);

	// ヨコのカギのタイトル。
	style = WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY | SS_CENTER | SS_CENTERIMAGE;
	hwndCtrl = ::CreateWindowW(TEXT("STATIC"), XgLoadString(IDS_ACROSS).c_str(), style, 0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
	if (hwndCtrl == nullptr)
		return FALSE;
	m_hwndHorzCaptionStatic = hwndCtrl;
	::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(::GetStockObject(SYSTEM_FIXED_FONT)), TRUE);

	MClueEditData *data;
	WCHAR sz[256];

	// タテのカギについて。
	for (const auto& hint : m_vVertInfo) {
		// 左側のラベルを作成。
		StringCchPrintfW(sz, _countof(sz), XgLoadString(IDS_DOWNNUMBER).c_str(), hint.m_number);
		style = WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE;
		hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz, style, 0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
		assert(hwndCtrl);
		::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(m_hCluesFont), TRUE);
		::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, MAKELONG(hint.m_number, TRUE));
		if (hwndCtrl == nullptr)
			return FALSE;
		m_ahwndVertStatics.emplace_back(hwndCtrl);

		// テキストボックス（EDITコントロール）を追加。
		style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE;
		hwndCtrl = ::CreateWindowExW(WS_EX_CLIENTEDGE, TEXT("EDIT"), m_word2clue[hint.m_word].c_str(), style, 0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
		assert(hwndCtrl);
		::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(m_hCluesFont), TRUE);
		if (hwndCtrl == nullptr)
			return FALSE;
		data = (MClueEditData*)(::LocalAlloc(LMEM_FIXED, sizeof(MClueEditData)));
		data->m_fVert = true;
		data->m_fnOldWndProc = WNDPROC(::SetWindowLongPtrW(hwndCtrl, GWLP_WNDPROC, LONG_PTR(XgHintEditWndProc)));
		data->m_pParent = this;
		::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, LONG_PTR(data));
		m_ahwndVertEdits.emplace_back(hwndCtrl);
	}

	// ヨコのカギについて。
	for (const auto& hint : m_vHorzInfo) {
		// 左側のラベルを作成。
		StringCchPrintfW(sz, _countof(sz), XgLoadString(IDS_ACROSSNUMBER).c_str(), hint.m_number);
		style = WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE;
		hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz, style, 0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
		assert(hwndCtrl);
		::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(m_hCluesFont),  TRUE);
		::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, MAKELONG(hint.m_number, FALSE));
		if (hwndCtrl == nullptr)
			return FALSE;
		m_ahwndHorzStatics.emplace_back(hwndCtrl);

		// テキストボックス（EDITコントロール）を追加。
		style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE;
		hwndCtrl = ::CreateWindowExW(WS_EX_CLIENTEDGE, TEXT("EDIT"), m_word2clue[hint.m_word].c_str(), style, 0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
		assert(hwndCtrl);
		::SendMessageW(hwndCtrl, WM_SETFONT, WPARAM(m_hCluesFont), TRUE);
		if (hwndCtrl == nullptr)
			return FALSE;
		data = static_cast<MClueEditData*>(::LocalAlloc(LMEM_FIXED, sizeof(MClueEditData)));
		data->m_fVert = false;
		data->m_fnOldWndProc = WNDPROC(::SetWindowLongPtrW(hwndCtrl, GWLP_WNDPROC, LONG_PTR(XgHintEditWndProc)));
		data->m_pParent = this;
		::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, LONG_PTR(data));
		m_ahwndHorzEdits.emplace_back(hwndCtrl);
	}

	return TRUE;
}

// WM_CREATE
BOOL MCluesWnd::OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/) {
	// 初期化。
	m_ahwndVertStatics.clear();
	m_ahwndVertEdits.clear();
	m_ahwndHorzStatics.clear();
	m_ahwndHorzEdits.clear();
	m_svHintsScrollView.SetParent(hwnd);
	m_svHintsScrollView.ShowScrollBars(FALSE, TRUE);
	m_hwndHighlightVertEdit = m_hwndHighlightHorzEdit = nullptr;

	if (m_hCluesFont)
		::DeleteObject(m_hCluesFont);
	m_hCluesFont = ::CreateFontIndirectW(&m_lfCluesLogFont);
	if (m_hCluesFont == nullptr)
		m_hCluesFont = HFONT(::GetStockObject(DEFAULT_GUI_FONT));

	if (!CreateControls(hwnd))
		return FALSE;

	// 再配置。
	::PostMessageW(hwnd, WM_SIZE, 0, 0);
	return TRUE;
}

// WM_DESTROY
void MCluesWnd::OnDestroy(HWND hwnd) {
	UpdateHintData();

	// 現在の位置とサイズを保存する。
	MRect rc;
	::GetWindowRect(hwnd, &rc);
	m_nWindowX = rc.left;
	m_nWindowY = rc.top;
	m_nWindowCX = rc.Width();
	m_nWindowCY = rc.Height();

	DestroyControls();

	::DeleteObject(m_hCluesFont);
	m_hCluesFont = nullptr;

	if (m_bTest)
		PostQuitMessage(0);
}

// WM_COMMAND
void MCluesWnd::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept {
	if (id == ID_JUMPNUMBER) {
		PostMessageW(GetParent(hwnd), WM_COMMAND, id, 0);
		return;
	}
	if (codeNotify == STN_CLICKED) {
		for (size_t i = 0; i < m_ahwndVertStatics.size(); ++i) {
			if (m_ahwndVertStatics[i] == hwndCtl) {
				::SendMessageW(m_ahwndVertEdits[i], EM_SETSEL, 0, -1);
				::SetFocus(m_ahwndVertEdits[i]);
				return;
			}
		}
		for (size_t j = 0; j < m_ahwndHorzStatics.size(); ++j) {
			if (m_ahwndHorzStatics[j] == hwndCtl) {
				::SendMessageW(m_ahwndHorzEdits[j], EM_SETSEL, 0, -1);
				::SetFocus(m_ahwndHorzEdits[j]);
				return;
			}
		}
	}
}

// WM_KEYDOWN / WM_KEYUP
void MCluesWnd::OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) noexcept {
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
		if (m_ahwndVertEdits.size())
			::SetFocus(m_ahwndVertEdits[0]);
		break;

	case VK_F6:
		if (::GetAsyncKeyState(VK_SHIFT) < 0)
			::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
		else
			::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
		break;

	default:
		break;
	}
}

// ズーム。
void MCluesWnd::OnZoom(HWND hwnd, BOOL bZoomIn) {
	// フォント情報を取得。
	LOGFONTW *plf = &m_lfCluesLogFont;
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
	m_lfCluesLogFont.lfHeight = MulDiv(pointsize, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
	::DeleteDC(hdc);

	// フォントを再作成。
	if (m_hCluesFont)
		::DeleteObject(m_hCluesFont);
	m_hCluesFont = ::CreateFontIndirectW(&m_lfCluesLogFont);

	// 再描画をOFFにする。
	SetWindowRedraw(hwnd, FALSE);

	// フォントをセット。再描画はWM_SIZEに任せる。
	for (auto hwndCtrl : m_ahwndVertStatics)
		SetWindowFont(hwndCtrl, m_hCluesFont, FALSE);
	for (auto hwndCtrl : m_ahwndVertEdits)
		SetWindowFont(hwndCtrl, m_hCluesFont, FALSE);
	for (auto hwndCtrl : m_ahwndHorzStatics)
		SetWindowFont(hwndCtrl, m_hCluesFont, FALSE);
	for (auto hwndCtrl : m_ahwndHorzEdits)
		SetWindowFont(hwndCtrl, m_hCluesFont, FALSE);

	// 再描画をONにする。
	SetWindowRedraw(hwnd, TRUE);

	// 再配置。
	::PostMessageW(hwnd, WM_SIZE, 0, 0);
}

// WM_MOUSEWHEEL
void __fastcall MCluesWnd::OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys) {
	if (::GetAsyncKeyState(VK_CONTROL) < 0) {
		if (zDelta < 0) {
			OnZoom(hwnd, FALSE);
		} else if (zDelta > 0) {
			OnZoom(hwnd, TRUE);
		}
	}
	else if (::GetAsyncKeyState(VK_SHIFT) < 0) {
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

// WM_CONTEXTMENU
void MCluesWnd::OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos) noexcept {
	WCHAR szClass[8];
	szClass[0] = 0;
	GetClassNameW(hwndContext, szClass, _countof(szClass));
	if (lstrcmpiW(szClass, L"EDIT") == 0)
		return;
	BOOL bStatic = (lstrcmpiW(szClass, L"STATIC") == 0);

	// メニューを読み込む。
	HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(2));
	HMENU hSubMenu = GetSubMenu(hMenu, 1);

	// スタティックでなければ、番号ジャンプのメニュー項目を削除する。
	LONG nData = (LONG)::GetWindowLongPtrW(hwndContext, GWLP_USERDATA);
	m_nNumber = LOWORD(nData);
	m_bVert = HIWORD(nData);
	if (!bStatic || !m_nNumber)
		DeleteMenu(hSubMenu, ID_JUMPNUMBER, MF_BYCOMMAND);

	if (!m_nNumber) {
		DeleteMenu(hSubMenu, ID_CLEARTHISCLUE, MF_BYCOMMAND);
		DeleteMenu(hSubMenu, ID_GENERATEHINT, MF_BYCOMMAND);
		DeleteMenu(hSubMenu, ID_JUMPNUMBER, MF_BYCOMMAND);
		DeleteMenu(hSubMenu, ID_EXPLAINTHISCLUE, MF_BYCOMMAND);
	}

	// スタティックなら色を変える。
	if (bStatic) {
		m_hwndTrackMenuOn = hwndContext;
		::InvalidateRect(hwndContext, nullptr, TRUE);
	}

	// 右クリックメニューを表示する。
	::SetForegroundWindow(hwnd);
	constexpr auto flags = TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD;
	const auto nCmd = ::TrackPopupMenu(hSubMenu, flags, xPos, yPos, 0, hwnd, nullptr);

	// 色を戻す。
	if (bStatic) {
		m_hwndTrackMenuOn = nullptr;
		::InvalidateRect(hwndContext, nullptr, TRUE);
	}

	::PostMessageW(hwnd, WM_NULL, 0, 0);

	if (nCmd != 0) {
		PostMessageW(xg_hMainWnd, WM_COMMAND, nCmd, 0);
	}

	::DestroyMenu(hMenu);
}

// 位置情報を調整する。
// キャンバスウィンドウの左右のうち、作業領域とのマージンが大きい側に隣接して配置する。
void MCluesWnd::AdjustPosInfo() {
	// 基準はキャンバスウィンドウ。無ければメインウィンドウで代用する。
	HWND hwndBase = (m_hwndCanvas && ::IsWindow(m_hwndCanvas)) ? m_hwndCanvas : xg_hMainWnd;
	if (!hwndBase)
		return;

	// キャンバスの画面座標での矩形(ウィンドウ全体)。
	RECT rcCanvas;
	::GetWindowRect(hwndBase, &rcCanvas);

	HMONITOR hMon = ::MonitorFromWindow(hwndBase, MONITOR_DEFAULTTONEAREST);
	MONITORINFO info = { sizeof(info) };
	RECT rcWork;
	if (::GetMonitorInfoW(hMon, &info))
		rcWork = info.rcWork;
	else
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);

	// キャンバスの左右のマージン。
	const INT marginLeft = rcCanvas.left - rcWork.left;
	const INT marginRight = rcWork.right - rcCanvas.right;

	// マージンが大きい側に隣接させる(同じなら右)。
	INT x;
	if (marginRight >= marginLeft)
		x = rcCanvas.right;
	else
		x = rcCanvas.left - m_nWindowCX;

	// 収まらない場合は作業領域内に収める(キャンバスに重なることがある)。
	if (x + m_nWindowCX > rcWork.right)
		x = rcWork.right - m_nWindowCX;
	if (x < rcWork.left)
		x = rcWork.left;

	// 縦はキャンバスの上端に合わせ、作業領域内に収める。
	INT y = rcCanvas.top;
	if (y + m_nWindowCY > rcWork.bottom)
		y = rcWork.bottom - m_nWindowCY;
	if (y < rcWork.top)
		y = rcWork.top;

	m_nWindowX = x;
	m_nWindowY = y;
}

LRESULT CALLBACK
MCluesWnd::WindowProcDx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
	HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
	HANDLE_MSG(hWnd, WM_SIZE, OnSize);
	HANDLE_MSG(hWnd, WM_VSCROLL, OnVScroll);
	HANDLE_MSG(hWnd, WM_KEYDOWN, OnKey);
	HANDLE_MSG(hWnd, WM_KEYUP, OnKey);
	HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
	HANDLE_MSG(hWnd, WM_MOUSEWHEEL, OnMouseWheel);
	HANDLE_MSG(hWnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
	HANDLE_MSG(hWnd, WM_CONTEXTMENU, OnContextMenu);
	HANDLE_MSG(hWnd, WM_CTLCOLOREDIT, OnCtlColor);
	HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, OnCtlColor);
	default:
		return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

// 単体テスト。
void MCluesWnd::UnitTest() {
	std::vector<MPlaceInfo> vHorzInfo = {
		{ 0, 0, L"単語１", 1 },
		{ 1, 0, L"単語２", 2 },
		{ 1, 2, L"単語３", 3 },
	};
	std::vector<MPlaceInfo> vVertInfo = {
		{ 0, 0, L"単語４", 1 },
		{ 0, 1, L"単語５", 2 },
		{ 1, 2, L"単語６", 3 },
		{ 0, 3, L"単語７", 4 },
	};
	MStringMapW word2clue = {
		{ L"単語１", L"これはテストです１" },
		{ L"単語２", L"これはテストです２" },
		{ L"単語３", L"これはテストです３" },
		{ L"単語４", L"これはテストです４" },
		{ L"単語５", L"これはテストです５" },
		{ L"単語６", L"これはテストです６" },
		{ L"単語７", L"これはテストです７" },
	};
	LOGFONT lfUIFont;
    GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(lfUIFont), &lfUIFont);

	MCluesWnd clues_wnd(vHorzInfo, vVertInfo, word2clue, lfUIFont);
	clues_wnd.m_bTest = TRUE;

	BOOL ret = clues_wnd.CreateDx(nullptr);
	assert(ret);

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	for (auto& pair : word2clue) {
		PrintfW(L"%s --> %s\n", pair.first.c_str(), pair.second.c_str());
	}
}
