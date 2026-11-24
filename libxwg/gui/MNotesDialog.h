// MNotesDialog.h --- [ヘッダーと備考欄]ダイアログ。
// Author: katahiromz
// License: MIT
// Q/C: OK

#pragma once

#include "MWindowBase.h"

// [ヘッダーと備考欄]ダイアログ。
class MNotesDialog : public MDialogBase
{
public:
	MStringW m_header; // ヘッダー。
	MStringW m_notes; // 備考欄。

	// 各テキストボックスの最大バッファ サイズ(終端のNULを含む)。
	static constexpr INT MAX_TEXT = 512;

	MNotesDialog(const MStringW& header, const MStringW& notes)
		: m_header(header)
		, m_notes(notes)
	{
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_NOTES);
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

		// 文字数制限をする。
		::SendDlgItemMessageW(hwnd, edt1, EM_LIMITTEXT, MAX_TEXT - 1, 0);
		::SendDlgItemMessageW(hwnd, edt2, EM_LIMITTEXT, MAX_TEXT - 1, 0);

		// ヘッダーを設定する。空ならひな形を使う。
		if (m_header.empty())
			m_header = L"Title: (Untitled)\r\nAuthor: \r\nEditor: \r\nCopyright: \r\nDate: YYYY-MM-DD";
		::SetDlgItemTextW(hwnd, edt1, m_header.c_str());

		// 備考欄を設定する。先頭の定型見出しは除く。
		MStringW str = m_notes;
		XgStrTrim(str);
		const MStringW msg = XgLoadString(IDS_BELOWISNOTES);
		if (!msg.empty() && str.find(msg) == 0) {
			str = str.substr(msg.size());
			XgStrTrim(str);
		}
		::SetDlgItemTextW(hwnd, edt2, str.c_str());

		return TRUE;
	}

	// テキストボックスの内容を取得し、前後の空白を除く。
	static MStringW GetTrimmedText(HWND hwnd, INT id) {
		WCHAR text[MAX_TEXT];
		text[0] = 0;
		::GetDlgItemTextW(hwnd, id, text, _countof(text));
		MStringW str = text;
		// 前後の空白を取り除く。
		XgStrTrim(str);
		// 二重改行はダメ。
		while (XgStrReplace(str, L"\r\n\r\n", L"\r\n"))
			;
		while (XgStrReplace(str, L"\n\n", L"\n"))
			;
		// 改行の後の空白を取り除く。
		while (XgStrReplace(str, L"\n ", L"\n") || XgStrReplace(str, L"\n\t", L"\n") ||
		       XgStrReplace(str, L"\n　", L"\n"))
		{
			;
		}
		return str;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
			m_header = GetTrimmedText(hwnd, edt1);
			m_notes = GetTrimmedText(hwnd, edt2);
			EndDialog(IDOK);
			break;
		case IDCANCEL:
			EndDialog(IDCANCEL);
			break;
		default:
			break;
		}
	}

public:
	// 単体テスト。
	static void UnitTest(void) {
		MNotesDialog dialog(L"OK", L"OKOK");
		auto ret = (INT)dialog.DoModal(NULL);
		PrintfW(L"ret: %d\n", ret);
		PrintfW(L"%s, %s\n", dialog.m_header.c_str(), dialog.m_notes.c_str());
	}
};
