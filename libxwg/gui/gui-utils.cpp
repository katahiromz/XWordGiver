// gui-utils.cpp
// Author: katahiromz
// License: MIT

#include "gui-utils.h"
#include "board-image.h"
#include "board-text-io.h"
#include "image-io.h"
#include "utils.h"
#include "resource.h"

// ポップアップメニューを読み込む。
HMENU __fastcall XgLoadPopupMenu(HWND hwnd, INT nPos) noexcept {
	return LoadMenuW(xg_hInstance, MAKEINTRESOURCE(2));
}

MStringW __fastcall XgUrlEncode(const MStringW& url) {
	MStringA str;

	size_t len = url.size() * 4;
	str.resize(len);
	if (len > 0)
		WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, &str[0], INT(len), nullptr, nullptr);

	len = strlen(str.c_str());
	str.resize(len);

	MStringW ret;
	WCHAR buf[4];
	static const WCHAR s_hex[] = L"0123456789ABCDEF";
	for (auto ch : str) {
		using namespace std;
		if (ch == ' ') {
			ret += L'+';
			continue;
		}
		if (('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') ||  ('0' <= ch && ch <= '9')) {
			ret += CHAR(ch);
			continue;
		}

		switch (ch) {
		case L'.': case L'-': case L'_': case L'*':
			ret += CHAR(ch);
			break;
		default:
			buf[0] = L'%';
			buf[1] = s_hex[(ch >> 4) & 0xF];
			buf[2] = s_hex[ch & 0xF];
			buf[3] = 0;
			ret += buf;
			break;
		}
	}

	return ret;
}

// ウェブ検索。
void __fastcall XgDoWebSearch(HWND hwnd, LPCWSTR str) {
	auto query = XgLoadString(IDS_GOOGLESEARCH);
	query += XgUrlEncode(XgNaturalizeString(str));

	::ShellExecuteW(hwnd, nullptr, query.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// コピー。
BOOL __fastcall XgCopyBoard(HWND hwnd, MBoardDisplay& display, HGLOBAL hText, HGLOBAL hDIB, HENHMETAFILE hEMF) {
	BOOL ret = FALSE;
	// クリップボードを開く。
	if (::OpenClipboard(hwnd)) {
		// クリップボードを空にする。
		if (::EmptyClipboard()) {
			if (hText) {
				::SetClipboardData(CF_UNICODETEXT, hText);
				ret = TRUE;
			}
			if (hDIB) {
				::SetClipboardData(CF_DIB, hDIB);
				ret = TRUE;
			}
			if (hEMF) {
				::SetClipboardData(CF_ENHMETAFILE, hEMF);
				ret = TRUE;
			}
		}
		// クリップボードを閉じる。
		::CloseClipboard();
	}
	return ret;
}

// クリップボードにクロスワードをコピー。
BOOL XgCopyBoard(HWND hwnd, MBoardDisplay& display, BOOL bText, BOOL bDIB, BOOL bEMF) {
	auto& board = XgGetDisplayBoard(display);

	HGLOBAL hText = nullptr;
	HGLOBAL hDIB = nullptr;
	HENHMETAFILE hEMF = nullptr;

	if (bText) {
		MStringW text;
		XgTextFromBoard(text, board);
		hText = XgCreateTextDataW(text);
	}
	if (bDIB) {
		hDIB = XgDIBFromBoard(display);
	}
	if (bEMF) {
		hEMF = XgEMFFromBoard(display);
	}

	if (!XgCopyBoard(hwnd, display, hText, hDIB, hEMF)) {
		GlobalFree(hText);
		GlobalFree(hDIB);
		DeleteEnhMetaFile(hEMF);
		return FALSE;
	}

	return TRUE;
}

// コンボボックスのテキストを取得する。
BOOL XgGetComboBoxText(HWND hwndCombo, LPWSTR text, INT length) {
	text[0] = UNICODE_NULL;
	INT iItem = (INT)SendMessageW(hwndCombo, CB_GETCURSEL, 0, 0);
	if (iItem == CB_ERR) {
		return !!GetWindowTextW(hwndCombo, text, length);
	} else {
		INT cch = (INT)SendMessageW(hwndCombo, CB_GETLBTEXTLEN, iItem, 0);
		if (length <= cch)
			return FALSE;
		return SendMessageW(hwndCombo, CB_GETLBTEXT, iItem, (LPARAM)text) != CB_ERR;
	}
}
