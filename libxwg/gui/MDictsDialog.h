// MDictsDialog.h --- [辞書]ダイアログ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MPropSheet.h"
#include "dict.h"
#include <functional>

class MDictsDialog : public MPropSheetPage {
public:
	INT m_bUpdating = 0;
	MDictList m_dict_list;
	MStringW m_dict_name;
	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void()> m_fnApply;

	MDictsDialog(const MStringW& dict_name) : MPropSheetPage(IDD_DICTLIST) {
		XgLoadDictList(m_dict_list);
		m_dict_name = dict_name;
	}

	// ラジオボタン風の状態を表示するためのイメージリストを作成する。
	static HIMAGELIST CreateRadioButtonImageList(HWND hwnd) {
		HIMAGELIST himl;
		HDC hdc_wnd, hdc;
		HBITMAP hbm_im, hbm_mask;
		HGDIOBJ hbm_orig;
		HBRUSH hbr_white = GetStockBrush(WHITE_BRUSH), hbr_black = GetStockBrush(BLACK_BRUSH);
		RECT rc;

		// 小さいイメージリストを作成する。
		himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
								ILC_COLOR | ILC_MASK, 2, 2);
		// いろいろ準備。
		hdc_wnd = GetDC(hwnd);
		hdc = CreateCompatibleDC(hdc_wnd);
		hbm_im = CreateCompatibleBitmap(hdc_wnd, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
		hbm_mask = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 1, nullptr);
		ReleaseDC(hwnd, hdc_wnd);

		// マスクビットマップを描画。
		SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
		hbm_orig = SelectObject(hdc, hbm_mask);
		FillRect(hdc, &rc, hbr_white);
		InflateRect(&rc, -1, -1);
		SelectObject(hdc, hbr_black);
		Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);

		// チェックしていないアイコンを追加。
		SelectObject(hdc, hbm_im);
		SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW));
		Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, hbm_orig);
		ImageList_Add(himl, hbm_im, hbm_mask); 

		// チェック済みのアイコンを追加。
		SelectObject(hdc, hbm_im);
		SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW));
		Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
		InflateRect(&rc, -3, -3);
		SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
		Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject(hdc, hbm_orig);
		ImageList_Add(himl, hbm_im, hbm_mask);

		// 後片付け。
		DeleteObject(hbm_mask);
		DeleteObject(hbm_im);
		DeleteDC(hdc);

		return himl;
	}

	// リストビューのチェック情報を更新する。
	void SetCurSel(HWND hwnd, HWND hwndLst1, INT iSelect) {
		if (iSelect < 0)
			return; // 不正な選択位置なら何もしない（全項目破壊を防ぐ）。
		INT cItems = ListView_GetItemCount(hwndLst1);
		for (INT iItem = 0; iItem < cItems; ++iItem) {
			if (iItem == iSelect) {
				ListView_SetItemState(hwndLst1, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			} else {
				ListView_SetItemState(hwndLst1, iItem, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
			}
		}

		ListView_SetItemState(hwndLst1, iSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(hwndLst1, iSelect, FALSE);
	}

	// リストを再読み込みする。
	void ReloadList(HWND hwnd) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);

		// すべての項目を消去する。
		ListView_DeleteAllItems(hwndLst1);
		SendDlgItemMessageW(hwnd, cmb1, CB_RESETCONTENT, 0, 0);

		// 辞書リストを読み込む。
		XgLoadDictList(m_dict_list);

		// リストビューを埋める。
		INT iItem = 0;
		for (auto& entry : m_dict_list) {
			LV_ITEMW item = { LVIF_TEXT };

			// リストビューに一行追加する。列「ファイル名」のテキストを設定。
			item.pszText = const_cast<LPWSTR>(PathFindFileNameW(entry.m_filename.c_str()));
			item.iItem = iItem;
			item.iSubItem = 0;
			ListView_InsertItem(hwndLst1, &item);

			// 列「表示名」のテキストを設定。
			item.iItem = iItem;
			item.iSubItem = 1;
			if (XgIsUserJapanese())
				item.pszText = const_cast<LPWSTR>(entry.m_japanese_name.c_str());
			else
				item.pszText = const_cast<LPWSTR>(entry.m_english_name.c_str());
			ListView_SetItem(hwndLst1, &item);

			// ファイルの更新日時を取得するために、FindFirstFile関数を使う。
			WIN32_FIND_DATAW find;
			HANDLE hFind = ::FindFirstFileW(entry.m_filename.c_str(), &find);
			::FindClose(hFind);

			// 更新日時に従って文字列を構築する。
			WCHAR szText[32];
			FILETIME ftLocal;
			SYSTEMTIME st;
			if (hFind == INVALID_HANDLE_VALUE)
			{
				StringCchCopyW(szText, _countof(szText), L"N/A");
			}
			else
			{
				::FileTimeToLocalFileTime(&find.ftLastWriteTime, &ftLocal);
				::FileTimeToSystemTime(&ftLocal, &st);
				StringCchPrintfW(szText, _countof(szText), L"%04d-%02d-%02d %02d:%02d",
								 st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute);
			}

			// 列「更新日時」のテキストを設定。
			item.iItem = iItem;
			item.iSubItem = 2;
			item.pszText = szText;
			ListView_SetItem(hwndLst1, &item);

			++iItem;
		}

		// コンボボックスを埋める。
		for (auto& item : m_dict_list) {
			LPCWSTR pszText = PathFindFileNameW(item.m_filename.c_str());
			::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)pszText);
		}

		// コンボボックスの項目を選択する。
		{
			LPCWSTR pszText = PathFindFileNameW(m_dict_name.c_str());
			INT iItem = (INT)::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, -1, (LPARAM)pszText);
			if (iItem != CB_ERR) {
				::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, iItem, 0);
			}
		}

		// リストビューの項目を選択する。
		LV_FINDINFO Find = { LVFI_STRING, PathFindFileNameW(m_dict_name.c_str()) };
		iItem = ListView_FindItem(hwndLst1, -1, &Find);
		if (iItem != -1) {
			ListView_SetItemState(hwndLst1, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
			ListView_EnsureVisible(hwndLst1, iItem, FALSE);
		} else if (ListView_GetItemCount(hwndLst1) > 0) {
			// 一致しなければ先頭の辞書を仮選択しておく。
			SetCurSel(hwnd, hwndLst1, 0);
		}
	}

	// リストビューのチェック位置を取得する。
	INT GetCurSel(HWND hwnd, HWND hwndLst1) {
		INT cItems = ListView_GetItemCount(hwndLst1);
		for (INT iItem = 0; iItem < cItems; ++iItem) {
			UINT uState = ListView_GetItemState(hwndLst1, iItem, LVIS_STATEIMAGEMASK);
			if (uState & INDEXTOSTATEIMAGEMASK(2))
				return iItem;
		}
		return -1;
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);

		// 拡張リストビュースタイルを設定。
		ListView_SetExtendedListViewStyle(hwndLst1, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

		// 辞書リストのヘッダーの初期化を開始。
		LV_COLUMNW column = { LVCF_TEXT | LVCF_WIDTH };

		// ファイル名。
		auto filename = XgLoadString(IDS_FILENAME);
		column.pszText = &filename[0];
		column.cx = 200;
		ListView_InsertColumn(hwndLst1, 0, &column);

		// 表示名。
		auto display_name = XgLoadString(IDS_DISPLAYNAME);
		column.pszText = &display_name[0];
		column.cx = 250;
		ListView_InsertColumn(hwndLst1, 1, &column);

		// 更新日時。
		auto updated_time = XgLoadString(IDS_UPDATEDTIME);
		column.pszText = &updated_time[0];
		column.cx = 125;
		ListView_InsertColumn(hwndLst1, 2, &column);

		// イメージリストを設定。
		HIMAGELIST himl = CreateRadioButtonImageList(hwnd);
		ListView_SetImageList(hwndLst1, himl, LVSIL_STATE);

		// リストを再読み込みする。
		++m_bUpdating;
		ReloadList(hwnd);
		--m_bUpdating;

		return TRUE;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);

		// 変更があれば「更新」ボタンを有効にする。
		switch (id)
		{
		case psh1:
			if (codeNotify == BN_CLICKED)
				Changed(hwnd);
			break;
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELCHANGE ||
				codeNotify == CBN_SELENDOK)
			{
				Changed(hwnd);
			}
			break;
		}

		// 実際の処理。
		switch (id)
		{
		case psh1: // ファイルを開く。
			if (codeNotify == BN_CLICKED) {
				INT iItem = GetCurSel(hwnd, hwndLst1);
				if (iItem == -1)
					break;

				// コンボボックスからテキストを取得。
				WCHAR szText[MAX_PATH];
				szText[0] = 0;
				SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
				szText[_countof(szText) - 1] = 0;

				// パスファイル名を構築する。
				auto dict_path = GetDictName(szText);

				// シェルで開く。開けなければメモ帳で開く。
				SHELLEXECUTEINFOW info = { sizeof(info), SEE_MASK_FLAG_NO_UI, hwnd };
				info.lpFile = dict_path.c_str();
				if (!ShellExecuteExW(&info)) {
					WCHAR szQuotedPath[MAX_PATH];
					StringCchPrintfW(szQuotedPath, _countof(szQuotedPath), L"\"%s\"", dict_path.c_str());
					info.lpFile = L"notepad.exe";
					info.lpParameters = szQuotedPath;
					info.nShow = SW_SHOWNORMAL;
					ShellExecuteExW(&info);
				}
			}
			break;
		case psh2: // フォルダを開く。
			if (codeNotify == BN_CLICKED) {
				auto dict_dir = XgFindLocalFile(L"DICT");
				ShellExecuteW(hwnd, nullptr, dict_dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
			}
			break;
		case psh4: // 再読み込み。
			ReloadList(hwnd);
			break;
		case cmb1:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELCHANGE ||
				codeNotify == CBN_SELENDOK)
			{
				if (m_bUpdating)
					break;

				// コンボボックスで現在選択されているテキストを取得。
				INT iItem = (INT)::SendDlgItemMessageW(hwnd, cmb1, CB_GETCURSEL, 0, 0);
				if (iItem == CB_ERR)
					return;

				// コンボボックスからテキストを取得。
				WCHAR szText[MAX_PATH];
				szText[0] = 0;
				SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
				szText[_countof(szText) - 1] = 0;

				// リストビューからテキストを探す。
				LV_FINDINFO Find = { LVFI_STRING, szText };
				iItem = ListView_FindItem(hwndLst1, -1, &Find);

				// リストビューの選択を更新する。
				m_bUpdating++;
				SetCurSel(hwnd, hwndLst1, iItem);
				m_bUpdating--;
			}
			break;
		}
	}

	MStringW GetDictName(LPCWSTR szText) {
		// パスファイル名を構築する。
		auto dict_path = XgFindLocalFile(L"DICT");
		dict_path += L"\\";
		dict_path += szText;
		return dict_path;
	}

	// WM_NOTIFY
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);
		auto pListView = (NM_LISTVIEW*)pnmhdr;
		switch (pnmhdr->code) {
		case PSN_APPLY: // 適用
			{
				INT iItem = GetCurSel(hwnd, hwndLst1);
				if (iItem == -1) {
					// 選択が無ければ変更せず終了（辞書名を破壊しない）。
					return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
				}

				// コンボボックスからテキストを取得。
				WCHAR szText[MAX_PATH];
				szText[0] = 0;
				SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
				szText[_countof(szText) - 1] = 0;

				// 辞書を指定する。
				m_dict_name = GetDictName(szText);

				// 再読み込み。
				ReloadList(hwnd);

				if (m_fnApply)
					m_fnApply();
			}
			return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);

		case LVN_ITEMCHANGED:
			if (pnmhdr->idFrom == lst1 && !m_bUpdating && (pListView->uNewState & LVIS_SELECTED)) {
				// リストビューの選択が変わった。
				HWND hwndLst1 = GetDlgItem(hwnd, lst1);

				// テキストを取得。
				WCHAR szText[MAX_PATH];
				LV_ITEMW item = { LVIF_TEXT };
				item.pszText = szText;
				item.cchTextMax = _countof(szText);
				item.iItem = pListView->iItem;
				item.iSubItem = 0;
				ListView_GetItem(hwndLst1, &item);

				// リストビューのチェック状態を更新する。
				m_bUpdating++;
				SetCurSel(hwnd, hwndLst1, pListView->iItem);
				m_bUpdating--;

				// コンボボックスからテキストを探す。
				INT iItem = (INT)::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, -1, (LPARAM)szText);
				if (iItem != CB_ERR) {
					// コンボボックスの選択を変更する。
					m_bUpdating++;
					::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, iItem, 0);
					m_bUpdating--;

					// 更新ボタンを有効にする。
					Changed(hwnd);
				}
			}
			break;
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
		return DialogBoxDx(hwnd, IDD_RULEPRESET);
	}

	void Changed(HWND hwnd) {
		if (m_bSheet)
			PropSheet_Changed(GetParent(hwnd), hwnd);
	}

protected:
	BOOL m_bSheet = TRUE;

public:
	static void UnitTest();
};
