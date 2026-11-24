// MJumpDialog.h --- [ジャンプ]ダイアログ。
// Author: katahiromz
// License: MIT
// Q/C: OK

#pragma once

#include "MWindowBase.h"
#include <climits>
#include <functional>
#include <vector>

// 「ジャンプ」ダイアログ。
//
// ジャンプの種類(m_nType):
//   0: マス位置(m_jColumn, m_iRow。0ベース)
//   1: カギの番号(m_nNumber, m_bVert)
//   2: 単語(m_strWord)
class MJumpDialog : public MDialogBase
{
public:
	MBoardDisplay& m_display;
	MBoard& m_board;
	std::vector<MPlaceInfo>& m_vHorzInfo;
	std::vector<MPlaceInfo>& m_vVertInfo;
	INT m_nType = 0; // ジャンプの種類。
	INT m_jColumn = 0, m_iRow = 0; // マス位置。0ベース。
	BOOL m_bVert = FALSE; // タテか？
	INT m_nNumber = 1; // カギの番号。
	MStringW m_strWord; // 単語(正規化済み)。
	std::function<void()> m_fnDoIt; // 「OK」で実行する処理。

	explicit MJumpDialog(MBoardDisplay& display) noexcept
		: m_display(display)
		, m_board(display.m_board)
		, m_vHorzInfo(display.m_vHorzInfo)
		, m_vVertInfo(display.m_vVertInfo)
	{
	}

	// マス位置をセットする(0ベース)。
	void SetData(INT jCol, INT iRow) noexcept {
		m_jColumn = jCol;
		m_iRow = iRow;
	}

	BOOL CreateDx(HWND hwnd) noexcept {
		if (!CreateDialogDx(hwnd, IDD_JUMP))
			return FALSE;
		ShowWindow(m_hwnd, SW_SHOWNORMAL);
		UpdateWindow(m_hwnd);
		return TRUE;
	}

	INT_PTR DoModal(HWND hwnd) noexcept {
		return DialogBoxDx(hwnd, IDD_JUMP);
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

protected:
	// 指定位置から始まるカギの番号を探す。
	static bool FindNumberAt(const std::vector<MPlaceInfo>& infos, INT jCol, INT iRow, INT& nNumber) {
		for (auto& info : infos) {
			if (info.m_jCol == jCol && info.m_iRow == iRow) {
				nNumber = info.m_number;
				return true;
			}
		}
		return false;
	}

	// 指定した番号のカギがあるか？
	static bool HasNumber(const std::vector<MPlaceInfo>& infos, INT nNumber) {
		for (auto& info : infos) {
			if (info.m_number == nNumber)
				return true;
		}
		return false;
	}

	// カギ番号の最大値(なければ1)。
	static INT MaxNumber(const std::vector<MPlaceInfo>& infos) {
		INT nMax = 1;
		for (auto& info : infos) {
			if (nMax < info.m_number)
				nMax = info.m_number;
		}
		return nMax;
	}

	// 編集ボックスの数値を取得する。失敗したら0。
	static INT GetInt(HWND hwnd, INT id) {
		BOOL bOK = FALSE;
		UINT value = GetDlgItemInt(hwnd, id, &bOK, FALSE);
		if (!bOK)
			return 0;
		return (value > (UINT)INT_MAX) ? INT_MAX : (INT)value;
	}

	// 値を[0, nCount - 1]に収める。収めたらtrueを返す。
	static bool FixIndex(INT& index, INT nCount) {
		const INT nMax = (nCount > 0) ? (nCount - 1) : 0;
		if (index < 0) {
			index = 0;
			return true;
		}
		if (index > nMax) {
			index = nMax;
			return true;
		}
		return false;
	}

	// m_nTypeに合わせてUIを更新する。
	void UpdateUI(HWND hwnd) {
		const BOOL bPos = (m_nType == 0);
		const BOOL bNum = (m_nType == 1);
		const BOOL bWord = (m_nType == 2);

		CheckDlgButton(hwnd, rad1, bPos ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, rad2, bNum ? BST_CHECKED : BST_UNCHECKED);
		CheckDlgButton(hwnd, rad5, bWord ? BST_CHECKED : BST_UNCHECKED);

		EnableWindow(GetDlgItem(hwnd, edt1), bPos);
		EnableWindow(GetDlgItem(hwnd, edt2), bPos);
		EnableWindow(GetDlgItem(hwnd, edt3), bNum);
		EnableWindow(GetDlgItem(hwnd, rad3), bNum);
		EnableWindow(GetDlgItem(hwnd, rad4), bNum);
		EnableWindow(GetDlgItem(hwnd, edt4), bWord);

		if (bPos) {
			SetDlgItemInt(hwnd, edt1, m_jColumn + 1, FALSE);
			SetDlgItemInt(hwnd, edt2, m_iRow + 1, FALSE);
		} else if (bNum) {
			SetDlgItemInt(hwnd, edt3, m_nNumber, FALSE);
			CheckRadioButton(hwnd, rad3, rad4, (m_bVert ? rad4 : rad3));
		}
	}

	// 入力内容を検証してメンバに取り込む。不正なら補正してエラーを表示しFALSE。
	BOOL OnOK(HWND hwnd) {
		if (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED)
			m_nType = 1;
		else if (IsDlgButtonChecked(hwnd, rad5) == BST_CHECKED)
			m_nType = 2;
		else
			m_nType = 0;

		m_bVert = (IsDlgButtonChecked(hwnd, rad4) == BST_CHECKED);
		m_jColumn = GetInt(hwnd, edt1) - 1; // 画面は1ベース、内部は0ベース。
		m_iRow = GetInt(hwnd, edt2) - 1;
		m_nNumber = GetInt(hwnd, edt3);

		INT id = 0; // 補正した編集ボックス(最初のもの)。

		if (m_nType == 0) {
			if (FixIndex(m_jColumn, m_board.Columns()))
				id = edt1;
			if (FixIndex(m_iRow, m_board.Rows()) && !id)
				id = edt2;
		} else if (m_nType == 1) {
			const auto& infos = (m_bVert ? m_vVertInfo : m_vHorzInfo);
			if (m_nNumber < 1) {
				m_nNumber = 1;
				id = edt3;
			} else if (!HasNumber(infos, m_nNumber)) {
				m_nNumber = MaxNumber(infos);
				id = edt3;
			}
		} else {
			WCHAR szText[128];
			GetDlgItemTextW(hwnd, edt4, szText, _countof(szText));
			StrTrimW(szText, L" \t\r\n　");
			if (!szText[0]) {
				SetFocus(GetDlgItem(hwnd, edt4));
				return FALSE;
			}
			m_strWord = XgNormalizeString(szText);
		}

		if (id) {
			SetFocus(GetDlgItem(hwnd, id));
			ErrorBoxDx(IDS_OUTOFRANGE);
			UpdateUI(hwnd);
			return FALSE;
		}

		return TRUE;
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// ダイアログを中央へ移動する。
		CenterWindowDx(hwnd);

		// 現在位置がカギの開始マスなら、その番号と向きを初期値にする。
		const bool bHasClues = (m_vHorzInfo.size() || m_vVertInfo.size());
		if (bHasClues) {
			if (FindNumberAt(m_vHorzInfo, m_jColumn, m_iRow, m_nNumber))
				m_bVert = FALSE;
			if (FindNumberAt(m_vVertInfo, m_jColumn, m_iRow, m_nNumber))
				m_bVert = TRUE; // タテが優先。
		} else if (m_nType == 1) {
			m_nType = 0; // カギがなければ番号ジャンプは使えない。
		}
		EnableWindow(GetDlgItem(hwnd, rad2), bHasClues);

		SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(m_board.Columns(), 1));
		SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(m_board.Rows(), 1));
		SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(9999, 1));

		UpdateUI(hwnd);
		return TRUE;
	}

	// NOTE: m_fnDoItや文字列処理が例外を投げうるので noexcept にしない。
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
			if (OnOK(hwnd)) {
				if (m_fnDoIt)
					m_fnDoIt();
				if (m_bModal)
					EndDialog(IDOK);
			}
			break;

		case IDCANCEL:
			if (m_bModal)
				EndDialog(IDCANCEL);
			else
				DestroyWindow(hwnd);
			break;

		case rad1:
		case rad2:
		case rad5:
			if (IsDlgButtonChecked(hwnd, id) == BST_CHECKED) {
				m_nType = (id == rad1) ? 0 : (id == rad2) ? 1 : 2;
				UpdateUI(hwnd);
			}
			break;

		default:
			break;
		}
	}

public:
	// 単体テスト。
	static void UnitTest(void) {
		MBoardDisplay display;

		MJumpDialog dialog(display);
		dialog.SetData(1, 2);
		INT ret = (INT)dialog.DoModal(nullptr);
		PrintfW(L"%d, %d, %d, %d, %d, %d\n",
			ret,
			dialog.m_nType,
			dialog.m_jColumn,
			dialog.m_iRow,
			dialog.m_bVert,
			dialog.m_nNumber);
	}
};
