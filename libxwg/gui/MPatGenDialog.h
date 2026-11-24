// MPatGenDialog.h --- [黒マスパターンの作成]ダイアログ。
// Author: katahiromz
// License: MIT
// Q/C: OK

#pragma once

#include "xwg.h"
#include "MWindowBase.h"

// [黒マスパターンの作成]ダイアログ。
class MPatGenDialog : public MDialogBase {
public:
	INT m_nMaxWordLen = 5; // 単語の最大長。

	explicit MPatGenDialog(INT nMaxWordLen = 5) noexcept
		: m_nMaxWordLen(nMaxWordLen)
	{
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_PATGEN);
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
	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// ダイアログを中央へ移動する。
		CenterWindowDx(hwnd);

		// 単語長の範囲を指定する(UDM_SETRANGEは「最大, 最小」の順)。
		::SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0,
		                      MAKELPARAM(XG_MAX_WORD_LEN, XG_MIN_WORD_LEN));

		// 初期値(呼び出し側が指定した好ましい最大長)を範囲内に収めて表示する。
		if (m_nMaxWordLen < XG_MIN_WORD_LEN)
			m_nMaxWordLen = XG_MIN_WORD_LEN;
		else if (m_nMaxWordLen > XG_MAX_WORD_LEN)
			m_nMaxWordLen = XG_MAX_WORD_LEN;
		::SetDlgItemInt(hwnd, edt3, m_nMaxWordLen, FALSE);

		return TRUE;
	}

	// 入力値を検証して取り込む。不正ならエラーを表示してFALSE。
	BOOL OnOK(HWND hwnd) {
		BOOL bOK = FALSE;
		const UINT value = ::GetDlgItemInt(hwnd, edt3, &bOK, FALSE);
		if (!bOK || value < (UINT)XG_MIN_WORD_LEN || value > (UINT)XG_MAX_WORD_LEN) {
			ErrorBoxDx(IDS_ENTERINT);
			// メッセージ ボックスを閉じた後に、全選択してフォーカスを戻す。
			::SetFocus(::GetDlgItem(hwnd, edt3));
			::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
			return FALSE;
		}
		m_nMaxWordLen = (INT)value;
		return TRUE;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
			if (OnOK(hwnd))
				EndDialog(IDOK);
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		default:
			break;
		}
	}
};
