// MNewDialog.h - [新規作成]ダイアログ。
// Author: katahiromz
// License: MIT
// Q/C: OK

#pragma once

#include "MWindowBase.h"

// [新規作成]ダイアログ。
class MNewDialog : public MDialogBase {
public:
	INT m_nColumns;
	INT m_nRows;
	MNewDialog(INT nColumns, INT nRows) noexcept : m_nColumns(nColumns), m_nRows(nRows) { }

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept {
		// ダイアログを中央へ移動する。
		CenterWindowDx(hwnd);
		// サイズの欄を設定する。
		::SetDlgItemInt(hwnd, edt1, m_nRows, FALSE);
		::SetDlgItemInt(hwnd, edt2, m_nColumns, FALSE);
		// IMEをOFFにする。
		{
			HWND hwndCtrl = ::GetDlgItem(hwnd, edt1);
			::ImmAssociateContext(hwndCtrl, nullptr);

			hwndCtrl = ::GetDlgItem(hwnd, edt2);
			::ImmAssociateContext(hwndCtrl, nullptr);
		}
		// スピンコントロールの範囲を指定。
		SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
		SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
		// rad1を選ぶ。
		CheckRadioButton(hwnd, rad1, rad3, rad1);
		return TRUE;
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		INT n1, n2;
		BOOL b1, b2;
		switch (id) {
		case IDOK:
			// サイズの欄をチェックする。
			n1 = ::GetDlgItemInt(hwnd, edt1, &b1, FALSE);
			if (!b1 || n1 < XG_MIN_SIZE || n1 > XG_MAX_SIZE) {
				::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
				ErrorBoxDx(IDS_ENTERINT);
				::SetFocus(::GetDlgItem(hwnd, edt1));
				return;
			}
			n2 = ::GetDlgItemInt(hwnd, edt2, &b2, FALSE);
			if (!b2 || n2 < XG_MIN_SIZE || n2 > XG_MAX_SIZE) {
				::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
				ErrorBoxDx(IDS_ENTERINT);
				::SetFocus(::GetDlgItem(hwnd, edt2));
				return;
			}
			m_nRows = n1;
			m_nColumns = n2;
			if (IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED) {
				::EndDialog(hwnd, rad1);
			} else if (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED) {
				::EndDialog(hwnd, rad2);
			} else if (IsDlgButtonChecked(hwnd, rad3) == BST_CHECKED) {
				::EndDialog(hwnd, rad3);
			}
			break;

		case IDCANCEL:
			// ダイアログを閉じる。
			::EndDialog(hwnd, IDCANCEL);
			break;

		default:
			break;
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		default:
			break;
		}
		return 0;
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_NEW);
	}

	static void UnitTest(void) {
		MNewDialog dialog(2, 2);
		auto ret = dialog.DoModal(nullptr);
		PrintfW(L"ret: %d\n", (INT)ret);
		PrintfW(L"%d, %d\n", dialog.m_nColumns, dialog.m_nRows);
	}
};
