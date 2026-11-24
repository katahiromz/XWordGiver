// MAboutDialog.h --- バージョン情報ダイアログ。
// Author: katahiromz
// License: MIT
// Q/C: OK

#pragma once

#include "MWindowBase.h"
#include "utils.h"

BOOL XgIsRegdSW(void);

// バージョン情報ダイアログ。
class MAboutDialog : public MDialogBase {
public:
	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_ABOUT);
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_CONTEXTMENU, OnContextMenu);
		default:
			break;
		}
		return 0;
	}

protected:
	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// 画面中央に移動する。
		CenterWindowDx(hwnd);

		// バージョンと著作権情報。
		SetDlgItemTextW(hwnd, stc1, XgLoadString(IDS_VERSION).c_str());
		auto str = XgLoadString(XgIsRegdSW() ? IDS_SWREGD : IDS_SWUNREGD);
		str += L"\r\n\r\n";
		str += XgLoadString(IDS_COPYRIGHTTEXT);
		SetDlgItemTextW(hwnd, edt1, str.c_str());

		// プロセッサ数。
		SYSTEM_INFO sys_info;
		GetSystemInfo(&sys_info);
		WCHAR text[128];
		StringCchPrintfW(text, _countof(text),
		                 XgLoadString(IDS_HOWMANYCORES).c_str(), sys_info.dwNumberOfProcessors);
		SetDlgItemTextW(hwnd, stc2, text);

		// ビルドの種類。
#ifdef _WIN64
		SetDlgItemTextW(hwnd, stc3, L"Win64");
#else
		SetDlgItemTextW(hwnd, stc3, L"Win32");
#endif

		SetFocus(GetDlgItem(hwnd, IDOK));
		return FALSE; // 自動フォーカスしない。
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
		case IDCANCEL:
			EndDialog(id);
			break;
		case IDYES:
			// HISTORY.txtを開く。
			XgOpenLocalFile(hwnd, L"HISTORY.txt");
			break;
		default:
			break;
		}
	}

	// WM_NOTIFY
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) {
		switch (pnmhdr->code) {
		case NM_CLICK:
		case NM_RETURN:
			if (idFrom == ctl1) {
				// ユーザーがURLをクリックした。ホームページを開く。
				ShellExecuteW(hwnd, nullptr, XgLoadString(IDS_HOMEPAGE).c_str(),
				              nullptr, nullptr, SW_SHOWNORMAL);
			}
			break;
		default:
			break;
		}
		return 0;
	}

	// WM_CONTEXTMENU
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos) {
		// URLのコントロール以外は無視する。
		HWND hwndCtl1 = GetDlgItem(hwnd, ctl1);
		if (hwndContext != hwndCtl1)
			return;

		// 座標はスクリーン座標(複数モニターでは負数もある)。16ビットの符号付きとして扱う。
		INT x = static_cast<SHORT>(xPos);
		INT y = static_cast<SHORT>(yPos);

		// キーボード操作(Shift+F10など)のときは座標が-1になる。コントロールの中央に出す。
		if (xPos == MAXWORD && yPos == MAXWORD) {
			RECT rc;
			GetWindowRect(hwndCtl1, &rc);
			x = (rc.left + rc.right) / 2;
			y = (rc.top + rc.bottom) / 2;
		}

		// ユーザーがURLを右クリックした。メニューを表示する。
		HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(3));
		if (!hMenu)
			return;
		HMENU hSubMenu = GetSubMenu(hMenu, 2);
		INT id = 0;
		if (hSubMenu) {
			SetForegroundWindow(hwnd);
			id = static_cast<INT>(::TrackPopupMenu(hSubMenu,
				TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
				x, y, 0, hwnd, nullptr));
			PostMessageW(hwnd, WM_NULL, 0, 0);
		}
		DestroyMenu(hMenu);

		if (id == ID_COPYURL)
			XgSetClipboardUnicodeText(hwnd, XgLoadString(IDS_HOMEPAGE).c_str());
	}
};
