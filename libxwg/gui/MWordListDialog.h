// MWordListDialog.h --- 「単語群から生成」ダイアログ
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include "MWindowBase.h"
#include "connectivity.h"

class MWordListDialog : public MDialogBase {
public:
	inline static std::vector<MStringW> s_word_list;
	inline static std::unordered_set<MStringW> s_words;
	inline static MStringW s_str_word_list;
	BOOL m_bUseAllWords = TRUE;

	RECT m_rcBitmap;
	SIZE m_sizBitmap;
	HBITMAP m_hbmBitmap = nullptr;

	MWordListDialog() noexcept
	{
	}

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		CenterWindowDx(hwnd);
		// 文字数の限界を増やす。
		::SendDlgItemMessageW(hwnd, edt1, EM_SETLIMITTEXT, MAXSHORT, 0);

		// コントロールstc1の位置とサイズを取得。
		HWND hStc1 = GetDlgItem(hwnd, stc1);
		assert(hStc1 != nullptr);
		GetWindowRect(hStc1, &m_rcBitmap);
		MapWindowRect(nullptr, hwnd, &m_rcBitmap);
		DestroyWindow(hStc1);

		// ビットマップをリソースから読み込む。
		m_hbmBitmap = LoadBitmapW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDB_FROMWORDS));
		assert(m_hbmBitmap != nullptr);

		// ビットマップのサイズを取得。
		BITMAP bm;
		GetObject(m_hbmBitmap, sizeof(bm), &bm);
		m_sizBitmap.cx = bm.bmWidth;
		m_sizBitmap.cy = bm.bmHeight;

		// 再描画。
		InvalidateRect(hwnd, &m_rcBitmap, TRUE);

		// テキストを設定。
		if (s_str_word_list.empty())
			SetDlgItemTextW(hwnd, edt1, L"Here are the example words for XWordGiver");
		else
			SetDlgItemTextW(hwnd, edt1, s_str_word_list.c_str());

		return TRUE;
	}

	BOOL OnOK(HWND hwnd) {
		s_word_list.clear();
		s_words.clear();

		// edt1からテキストを取得する。
		HWND hEdt1 = GetDlgItem(hwnd, edt1);
		const auto cch = GetWindowTextLengthW(hEdt1);
		if (cch == 0) {
			ErrorBoxDx(IDS_ADDMOREWORDS);
			return FALSE;
		}
		LPWSTR psz = new WCHAR[cch + 1];
		if (!GetWindowTextW(hEdt1, psz, cch + 1))
		{
			delete[] psz;
			ErrorBoxDx(IDS_OUTOFMEMORY);
			return FALSE;
		}
		MStringW str = psz;
		delete[] psz;

		// 単語リストと辞書を取得する。
		std::vector<MStringW> items;
		XgStrSplit(items, str, L" \t\r\n　");
		for (auto& item : items) {
			// ハイフン、アポストロフィ、ピリオド、カンマ、カッコを取り除く。
			MStringW tmp;
			for (auto ch : item) {
				if (ch == L'-' || ch == 0xFF0D)
					continue;
				if (ch == L'\'' || ch == 0xFF07)
					continue;
				if (ch == L'.' || ch == 0xFF0E)
					continue;
				if (ch == L',' || ch == 0xFF0C)
					continue;
				if (ch == L'(' || ch == 0xFF08)
					continue;
				if (ch == L')' || ch == 0xFF09)
					continue;
				tmp += ch;
			}
			item = std::move(tmp);
			// 前後の空白を取り除く。
			XgStrTrim(item);
			// 文字列を標準化。
			item = XgNormalizeString(item);
			// 2文字以上なら追加。
			if (item.size() >= 2) {
				s_word_list.push_back(item);
			}
		}
		items.clear();

		// 2文字未満の単語を削除する。
		s_word_list.erase(std::remove_if(s_word_list.begin(), s_word_list.end(), [](const MStringW& word) noexcept {
			return (word.size() <= 1);
		}), s_word_list.end());

		// 単語が少ない？
		if (s_word_list.size() < 2) {
			ErrorBoxDx(IDS_ADDMOREWORDS);
			return FALSE;
		}

		// 多すぎる？
		constexpr size_t max_num = 300;
		if (s_word_list.size() > max_num) {
			WCHAR szText[128];
			StringCchPrintfW(szText, _countof(szText), XgLoadString(IDS_TOOMANYWORDS).c_str(), INT(max_num));
			ErrorBoxDx(szText);
			return FALSE;
		}

		MStringW nonconnected;
		s_words = { s_word_list.begin(), s_word_list.end() };

		// すべてでなくてもよい？
	    m_bUseAllWords = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		if (m_bUseAllWords) {
			while (!check_connectivity(s_words, nonconnected)) {
				// 接続されていない単語を削除。
				s_words.erase(nonconnected);
				s_word_list.erase(std::remove(s_word_list.begin(), s_word_list.end(), nonconnected), s_word_list.end());

				// 単語が少ない？
				if (s_word_list.size() < 2) {
					ErrorBoxDx(IDS_ADDMOREWORDS);
					return FALSE;
				}
			}
		}

		if (!check_connectivity(s_words, nonconnected)) {
			// 連結されていない。エラー。
			WCHAR szText[256];
			StringCchPrintfW(szText, _countof(szText), XgLoadString(IDS_NOTCONNECTABLE).c_str(),
							 nonconnected.c_str());
			ErrorBoxDx(szText);
		} else {
			return TRUE;
		}

		return FALSE;
	}

	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
	{
		switch (id)
		{
		case IDOK:
			if (OnOK(hwnd)) {
				// ビットマップを破棄。
				::DeleteObject(m_hbmBitmap);
				m_hbmBitmap = nullptr;
				// ダイアログを終了。
				::EndDialog(hwnd, id);
			}
			break;
		case IDCANCEL:
			// ビットマップを破棄。
			::DeleteObject(m_hbmBitmap);
			m_hbmBitmap = nullptr;
			// ダイアログを終了。
			::EndDialog(hwnd, id);
			break;
		default:
			break;
		}
	}

	void OnPaint(HWND hwnd) {
		// ここでビットマップを使って、イメージを描画する。
		PAINTSTRUCT ps;
		if (HDC hdc = BeginPaint(hwnd, &ps)) {
			if (HDC hdcMem = CreateCompatibleDC(hdc)) {
				HGDIOBJ hbmOld = SelectObject(hdcMem, m_hbmBitmap);
				SetStretchBltMode(hdc, STRETCH_HALFTONE);
				StretchBlt(hdc, m_rcBitmap.left, m_rcBitmap.top,
					m_rcBitmap.right - m_rcBitmap.left,
					m_rcBitmap.bottom - m_rcBitmap.top,
					hdcMem,
					0, 0, m_sizBitmap.cx, m_sizBitmap.cy,
					SRCCOPY);
				SelectObject(hdcMem, hbmOld);
				DeleteObject(hdcMem);
			} else {
				assert(0);
			}
			EndPaint(hwnd, &ps);
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
		default:
			break;
		}
		return 0;
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_WORDLIST);
	}
};
