// MInputPalette.h --- 入力パレット。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"

#define XGWM_SETINPUTMODE (WM_APP + 100)
#define XGWM_RECREATEINPUTPALETTE (WM_APP + 101)

enum MInputMode {
	xg_im_KANA,     // カナ入力。
	xg_im_ABC,      // 英字入力。
	xg_im_KANJI,    // 漢字入力。
	xg_im_RUSSIA,   // ロシア文字。
	xg_im_DIGITS,   // 数字入力
	xg_im_GREEK,    // ギリシャ文字。
	xg_im_ANY,      // 自由入力。
};

class MInputPalette : public MDialogBase {
public:
	MInputMode& m_imode;
	BOOL& m_bCharFeed;
	BOOL& m_bVertInput;
	BOOL& m_bVerticalLayout;
	BOOL& m_bShowInputPalette;
	BOOL m_bUpdating = FALSE;
	static inline HWND s_hwnd = nullptr;
	static inline BOOL m_bShowInputPalatte = FALSE;
	static inline INT s_nWindowX = CW_USEDEFAULT;
	static inline INT s_nWindowY = CW_USEDEFAULT;

	MInputPalette(MInputMode& imode, BOOL& bCharFeed, BOOL& bVertInput, BOOL& bVerticalLayout, BOOL& bShowInputPalette)
		: m_imode(imode)
		, m_bCharFeed(bCharFeed)
		, m_bVertInput(bVertInput)
		, m_bVerticalLayout(bVerticalLayout)
		, m_bShowInputPalette(bShowInputPalette)
	{
	}

	void UpdateUI(HWND hwnd, BOOL bInit = FALSE) {
		m_bUpdating++;
		if (bInit) {
			if (m_bCharFeed) {
				::CheckDlgButton(hwnd, 20050, BST_CHECKED);
			} else {
				::CheckDlgButton(hwnd, 20050, BST_UNCHECKED);
			}
		}
		if (m_bVerticalLayout) {
			if (m_bVertInput) {
				SetDlgItemTextW(hwnd, 20052, XgLoadString(IDS_VINPUT).c_str());
			} else {
				SetDlgItemTextW(hwnd, 20052, XgLoadString(IDS_HINPUT).c_str());
			}
		} else {
			if (m_bVertInput) {
				SetDlgItemTextW(hwnd, 20052, XgLoadString(IDS_VINPUT2).c_str());
			} else {
				SetDlgItemTextW(hwnd, 20052, XgLoadString(IDS_HINPUT2).c_str());
			}
		}
		m_bUpdating--;
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		s_hwnd = hwnd;
		UpdateUI(hwnd, TRUE);
		m_bShowInputPalette = TRUE;
		return FALSE;
	}

	// WM_DESTROY
	void OnDestroy(HWND hwnd) noexcept {
		s_hwnd = nullptr;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		if (m_bUpdating)
			return;

		switch (id) {
		case IDOK:
		case IDCANCEL:
		case IDCLOSE:
			m_bShowInputPalette = FALSE;
			DestroyWindow(hwnd);
			return;
		default:
			break;
		}

		HWND hButton = GetDlgItem(hwnd, id);

		WCHAR sz[10];
		GetWindowTextW(hButton, sz, 10);

		if (sz[0] && sz[1] == 0) {
			switch (sz[0]) {
			case ZEN_SPACE:
				PostMessageW(xg_hMainWnd, WM_IME_CHAR, L'_', 0);
				break;
			case L'↑':
				if (GetAsyncKeyState(VK_CONTROL) < 0) {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTUPPER, 0);
				} else {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_UP, 0);
				}
				break;
			case L'↓':
				if (GetAsyncKeyState(VK_CONTROL) < 0) {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTLOWER, 0);
				} else {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_DOWN, 0);
				}
				break;
			case L'←':
				if (GetAsyncKeyState(VK_CONTROL) < 0) {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTLEFT, 0);
				} else {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_LEFT, 0);
				}
				break;
			case L'→':
				if (GetAsyncKeyState(VK_CONTROL) < 0) {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTRIGHT, 0);
				} else {
					PostMessageW(xg_hMainWnd, WM_COMMAND, ID_RIGHT, 0);
				}
				break;
			default:
				PostMessageW(xg_hMainWnd, WM_IME_CHAR, sz[0], 0);
				break;
			}
		} else {
			switch (id) {
			case 10093: // 辞書
				PostMessageW(xg_hMainWnd, WM_COMMAND, ID_ONLINEDICT, 0);
				break;
			case 20050: // 文字送り
				PostMessageW(xg_hMainWnd, WM_COMMAND, ID_CHARFEED, 0);
				break;
			case 20052: // ﾀﾃ/ﾖｺ入力
				PostMessageW(xg_hMainWnd, WM_COMMAND, ID_INPUTHV, 0);
				break;
			case 20054: // BS
				PostMessageW(xg_hMainWnd, WM_KEYDOWN, VK_BACK, 0);
				break;
			case 20064: // NL
				PostMessageW(xg_hMainWnd, WM_KEYDOWN, VK_RETURN, 0);
				break;
			case 20070: // カナに
				PostMessageW(xg_hMainWnd, XGWM_SETINPUTMODE, (WPARAM)xg_im_KANA, 0);
				break;
			case 20071: // 縦置き/横置き
				m_bVerticalLayout = !m_bVerticalLayout;
				PostMessageW(xg_hMainWnd, XGWM_RECREATEINPUTPALETTE, 0, 0);
				break;
			case 20072: // 英字に
				PostMessageW(xg_hMainWnd, XGWM_SETINPUTMODE, (WPARAM)xg_im_ABC, 0);
				break;
			case 20073: // ロシアに
				PostMessageW(xg_hMainWnd, XGWM_SETINPUTMODE, (WPARAM)xg_im_RUSSIA, 0);
				break;
			default:
				break;
			}
		}

		::SetForegroundWindow(xg_hMainWnd);
	}

	// WM_KEYDOWN
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) noexcept {
		if (!fDown)
			return;

		switch (vk) {
		case VK_F6:
			if (::GetAsyncKeyState(VK_SHIFT) < 0)
				::PostMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
			else
				::PostMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
			break;

		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;

		default:
			break;
		}
	}

	// WM_MOVE
	void OnMove(HWND hwnd, int x, int y) noexcept {
		RECT rc;
		::GetWindowRect(hwnd, &rc);
		s_nWindowX = rc.left;
		s_nWindowY = rc.top;
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
			HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
			HANDLE_MSG(hwnd, WM_MOVE, OnMove);
		default:
			break;
		}
		return 0;
	}
};
