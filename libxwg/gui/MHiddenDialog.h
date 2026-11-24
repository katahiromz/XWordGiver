// MHiddenDialog.h --- 「隠し機能」ダイアログ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MPropSheet.h"
#include "board-image.h"
#include <functional>

class MHiddenDialog : public MPropSheetPage {
public:
	MBoardDisplay& m_display;
	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void()> m_fnApply;

	MHiddenDialog(MBoardDisplay& display)
		: MPropSheetPage(IDD_HIDDEN)
		, m_display(display)
	{
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		if (!m_bSheet)
			CenterWindowDx(hwnd);
		return TRUE;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case psh1: // デバッグ アクション 1
			if (codeNotify == BN_CLICKED) {
				SendMessageW(xg_hMainWnd, WM_COMMAND, ID_DEBUGACTION1, 0);
			}
			break;
		case psh2: // デバッグ アクション 2
			if (codeNotify == BN_CLICKED) {
				SendMessageW(xg_hMainWnd, WM_COMMAND, ID_DEBUGACTION2, 0);
			}
			break;
		case psh3: // アプリのフォルダを開く。
			if (codeNotify == BN_CLICKED) {
				WCHAR szPath[MAX_PATH];
				GetModuleFileNameW(nullptr, szPath, _countof(szPath));
				PathRemoveFileSpecW(szPath);
				ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
			}
			break;
		}
	}

	// WM_NOTIFY
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);
		auto pListView = (NM_LISTVIEW*)pnmhdr;
		switch (pnmhdr->code) {
		case PSN_APPLY: // 適用
			if (m_fnApply)
				m_fnApply();
			return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
		}
		return 0;
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
		}
		return 0;
	}

	INT_PTR DoModal(HWND hwnd) {
		m_bSheet = FALSE;
		return DialogBoxDx(hwnd, IDD_AIHELPER);
	}

	void Changed(HWND hwnd) {
		if (m_bSheet)
			PropSheet_Changed(GetParent(hwnd), hwnd);
	}

protected:
	BOOL m_bSheet = TRUE;
};
