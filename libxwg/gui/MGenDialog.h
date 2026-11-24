// MGenDialog.h --- [問題の作成]ダイアログ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "dict.h"

class MGenDialog : public MDialogBase {
public:
	const MDict& m_dict;
	INT m_nColumns;
	INT m_nRows;
	BOOL m_bChoosePAT;
	BOOL m_bShowAnswerOnGenerate;
	INT m_nMaxWordLen = 5;

	MGenDialog(const MDict& dict, INT nColumns, INT nRows, BOOL bChoosePAT, BOOL bShowAnswerOnGenerate) noexcept
		: m_dict(dict)
		, m_nColumns(nColumns)
		, m_nRows(nRows)
		, m_bChoosePAT(bChoosePAT)
		, m_bShowAnswerOnGenerate(bShowAnswerOnGenerate)
	{
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept {
		// ダイアログを中央へ移動する。
		CenterWindowDx(hwnd);
		// サイズの欄を設定する。
		::SetDlgItemInt(hwnd, edt1, m_nRows, FALSE);
		::SetDlgItemInt(hwnd, edt2, m_nColumns, FALSE);

		// 現在の状態で好ましいと思われる単語の最大長を取得する。
		const INT n3 = XgGetPreferredMaxLength(m_dict);
		::SetDlgItemInt(hwnd, edt3, n3, FALSE);

		// 答えを表示するか？
		if (m_bShowAnswerOnGenerate)
			::CheckDlgButton(hwnd, chx3, BST_CHECKED);
		// 自動的にPAT.txtから選択するか？
		if (m_bChoosePAT)
			::CheckDlgButton(hwnd, chx4, BST_CHECKED);
		// IMEをOFFにする。
		{
			HWND hwndCtrl;
			hwndCtrl = ::GetDlgItem(hwnd, edt1);
			::ImmAssociateContext(hwndCtrl, nullptr);
			hwndCtrl = ::GetDlgItem(hwnd, edt2);
			::ImmAssociateContext(hwndCtrl, nullptr);
		}
		SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
		SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
		SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_WORD_LEN, XG_MIN_WORD_LEN));
		return TRUE;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		INT n1, n2, n3;
		BOOL b1, b2, b3;
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
			n3 = ::GetDlgItemInt(hwnd, edt3, &b3, FALSE);
			if (!b3 || n3 < XG_MIN_WORD_LEN || n3 > XG_MAX_WORD_LEN) {
				::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
				ErrorBoxDx(IDS_ENTERINT);
				::SetFocus(::GetDlgItem(hwnd, edt3));
				return;
			}
			m_nRows = n1;
			m_nColumns = n2;
			m_nMaxWordLen = n3;
			// 答えを表示するか？
			m_bShowAnswerOnGenerate = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
			// 自動的にPAT.txtから選択するか？
			m_bChoosePAT = (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
			// 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
			if (!(m_nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
				ErrorBoxDx(IDS_EVENROWLINESYMV);
				break;
			}
			// 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
			if (!(m_nColumns & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
				ErrorBoxDx(IDS_EVENCOLLINESYMH);
				break;
			}
			// ダイアログを閉じる。
			::EndDialog(hwnd, IDOK);
			break;

		case IDCANCEL:
			// ダイアログを閉じる。
			::EndDialog(hwnd, IDCANCEL);
			break;

		case chx2:
			if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
				EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
				EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
				EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
			} else {
				EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
				EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
				EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
			}
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
		return DialogBoxDx(hwnd, IDD_CREATE);
	}
};
