// MFileSettingsDialog.h --- 「ファイル」設定
// Author: katahiromz
// License: MIT

#include "MWindowBase.h"
#include "MPropSheet.h"
#include "board-image.h"
#include <shlobj.h>
#include <functional>

class MFileSettingsDialog : public MPropSheetPage {
public:
	BOOL m_bShowAnswerOnOpen = TRUE;
	BOOL m_bAutoSave = FALSE;
	BOOL m_bShowAnswerOnGenerate = TRUE;
	BOOL m_bNoReadLooks = FALSE;
	BOOL m_bNoWriteLooks = FALSE;
	WCHAR m_szNumberingFileName1[MAX_PATH] = L"";
	WCHAR m_szNumberingFileName2[MAX_PATH] = L"";
	MStringListW m_dirs_save_to;

	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void()> m_fnApply;

	MFileSettingsDialog() : MPropSheetPage(IDD_FILESETTINGS)
	{
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// 中央寄せ。
		if (!m_bSheet)
			CenterWindowDx(hwnd);
		// チェックボックスを初期化。
		if (m_bShowAnswerOnOpen)
			CheckDlgButton(hwnd, chx1, BST_CHECKED);
		if (m_bAutoSave)
			CheckDlgButton(hwnd, chx2, BST_CHECKED);
		if (m_bShowAnswerOnGenerate)
			CheckDlgButton(hwnd, chx3, BST_CHECKED);
		if (m_bNoReadLooks)
			CheckDlgButton(hwnd, chx4, BST_CHECKED);
		if (m_bNoWriteLooks)
			CheckDlgButton(hwnd, chx5, BST_CHECKED);
		// 保存先を初期化。
		for (const auto& dir : m_dirs_save_to) {
			SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)dir.c_str());
		}
		SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, 0, 0);
		// 連番ファイル名1。
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		ComboBox_AddString(hCmb2, L"Crossword-%6N.xd");
		ComboBox_AddString(hCmb2, L"Crossword-%Wx%H-%4N.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y%M%D-%h%m%s.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y%M%D-%h%m%s-%Wx%H.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y%M%D(%w)-%h%m%s.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y%M%D(%w)-%h%m%s-%Wx%H.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y_%M_%D-%h_%m_%s.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y_%M_%D-%h_%m_%s-%Wx%H.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y_%M_%D(%w)-%h_%m_%s.xd");
		ComboBox_AddString(hCmb2, L"Cross-%Y_%M_%D(%w)-%h_%m_%s-%Wx%H.xd");
		ComboBox_SetText(hCmb2, m_szNumberingFileName1);
		// 連番ファイル名2。
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		ComboBox_AddString(hCmb3, L"Pattern-%6N.xd");
		ComboBox_AddString(hCmb3, L"Pattern-%Wx%H-%4N.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y%M%D-%h%m%s.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y%M%D-%h%m%s-%Wx%H.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y%M%D(%w)-%h%m%s.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y%M%D(%w)-%h%m%s-%Wx%H.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y_%M_%D-%h_%m_%s.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y_%M_%D-%h_%m_%s-%Wx%H.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y_%M_%D(%w)-%h_%m_%s.xd");
		ComboBox_AddString(hCmb3, L"Pat-%Y_%M_%D(%w)-%h_%m_%s-%Wx%H.xd");
		ComboBox_SetText(hCmb3, m_szNumberingFileName2);
		// ドラッグ＆ドロップを受け付ける。
		DragAcceptFiles(hwnd, TRUE);
		return TRUE;
	}

	// 「保存先」参照。
	static INT CALLBACK 
	XgBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) noexcept
	{
		if (uMsg == BFFM_INITIALIZED) {
			// 初期化の際に、フォルダーの場所を指定する。
			::SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		}
		return 0;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id)
		{
		case chx1:
		case chx2:
		case chx3:
		case chx4:
		case chx5:
			if (codeNotify == BN_CLICKED) {
				// 変更された。更新ボタンを有効にする。
				Changed(hwnd);
			}
			break;
		case cmb1:
		case cmb2:
			if (codeNotify == CBN_EDITCHANGE ||
				codeNotify == CBN_SELCHANGE ||
				codeNotify == CBN_SELENDOK)
			{
				// 変更された。更新ボタンを有効にする。
				Changed(hwnd);
			}
			break;
		case psh1: // 「参照」ボタン。
			{
				// ユーザーにフォルダ位置を問い合わせる。
				BROWSEINFOW bi = { hwnd };
				auto title = XgLoadString(IDS_CROSSSTORAGE);
				bi.lpszTitle = &title[0];
				bi.ulFlags = BIF_RETURNONLYFSDIRS;
				bi.lpfn = XgBrowseCallbackProc; // 初期位置設定用のフック。
				bi.lParam = (LPARAM)m_dirs_save_to[0].c_str();
				LPITEMIDLIST pidl = ::SHBrowseForFolderW(&bi);
				if (pidl) {
					// テキストを設定。
					WCHAR szFile[MAX_PATH];
					::SHGetPathFromIDListW(pidl, szFile);
					::SetDlgItemTextW(hwnd, cmb1, szFile);
					// 忘れずPIDLを解放。
					::CoTaskMemFree(pidl);
					// 変更された。更新ボタンを有効にする。
					Changed(hwnd);
				}
			}
			break;
		case psh2: // 「フォルダを開く」ボタン。
			{
				// フォルダパス名を取得。
				WCHAR szDir[MAX_PATH];
				::GetDlgItemTextW(hwnd, cmb1, szDir, _countof(szDir));
				// なければ作成。
				XgMakePathW(szDir);
				// シェルで開く。
				::ShellExecuteW(hwnd, nullptr, szDir, nullptr, nullptr, SW_SHOWNORMAL);
			}
			break;
		}
	}

	BOOL OnOK(HWND hwnd) {
		// チェックボックスから設定を取得する。
		m_bShowAnswerOnOpen = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		m_bAutoSave = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
		m_bShowAnswerOnGenerate = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
		m_bNoReadLooks = (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
		m_bNoWriteLooks = (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);

		// テキストを取得する。
		WCHAR szFile[MAX_PATH];
		::GetDlgItemTextW(hwnd, cmb1, szFile, _countof(szFile));
		// 前後の空白を取り除く。
		StrTrimW(szFile, L" \t\r\n　");

		// 一致する項目を削除する。
		INT i = 0;
		for (const auto& dir : m_dirs_save_to) {
			if (lstrcmpiW(dir.c_str(), szFile) == 0) {
				m_dirs_save_to.erase(m_dirs_save_to.begin() + i);
				break;
			}
			++i;
		}

		// 先頭に挿入。
		m_dirs_save_to.insert(m_dirs_save_to.begin(), szFile);

		// 連番ファイル名のテキストを取得する。
		::GetDlgItemTextW(hwnd, cmb2, m_szNumberingFileName1, _countof(m_szNumberingFileName1));
		::GetDlgItemTextW(hwnd, cmb3, m_szNumberingFileName2, _countof(m_szNumberingFileName2));

		// 前後の空白を取り除く。
		StrTrimW(m_szNumberingFileName1, L" \t\r\n　");
		StrTrimW(m_szNumberingFileName2, L" \t\r\n　");

		// 空文字列・無効なファイル名ならデフォルトに戻す。
		if (!m_szNumberingFileName1[0] ||
			lstrcmpiW(m_szNumberingFileName1, L".") == 0 ||
			lstrcmpiW(m_szNumberingFileName1, L"..") == 0)
		{
			StringCchCopyW(m_szNumberingFileName1, _countof(m_szNumberingFileName1),
						   L"Crossword-%Wx%H-%4N.xd");
		}
		if (!m_szNumberingFileName2[0] ||
			lstrcmpiW(m_szNumberingFileName2, L".") == 0 ||
			lstrcmpiW(m_szNumberingFileName2, L"..") == 0)
		{
			StringCchCopyW(m_szNumberingFileName2, _countof(m_szNumberingFileName2),
						   L"Pattern-%Wx%H-%4N.xd");
		}

		// 必要なら拡張子を付ける。
		LPWSTR pch;
		pch = PathFindExtensionW(m_szNumberingFileName1);
		if (!pch || !*pch)
			PathAddExtensionW(m_szNumberingFileName1, L".xd");
		pch = PathFindExtensionW(m_szNumberingFileName2);
		if (!pch || !*pch)
			PathAddExtensionW(m_szNumberingFileName2, L".xd");

		// 不正な文字を置き換える。
		for (LPWSTR pch = m_szNumberingFileName1; *pch; ++pch) {
			switch (*pch)
			{
			case L'/': case L'\\': case L':': case L'*': case L'?':
			case L'<': case L'>': case L'|':
				*pch = L'_';
				break;
			}
		}
		for (LPWSTR pch = m_szNumberingFileName2; *pch; ++pch) {
			switch (*pch)
			{
			case L'/': case L'\\': case L':': case L'*': case L'?':
			case L'<': case L'>': case L'|':
				*pch = L'_';
				break;
			}
		}
		return TRUE;
	}

	// WM_NOTIFY
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);
		auto pListView = (NM_LISTVIEW*)pnmhdr;
		switch (pnmhdr->code) {
		case PSN_APPLY: // 適用
			if (!OnOK(hwnd))
				return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
			if (m_fnApply)
				m_fnApply();
			return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
		}
		return 0;
	}

	// WM_DROPFILES
	void OnDropFiles(HWND hwnd, HDROP hdrop) {
		WCHAR szDir[MAX_PATH];
		DragQueryFileW(hdrop, 0, szDir, _countof(szDir));
		if (PathIsDirectoryW(szDir)) { // フォルダか？
			// テキストを設定。
			::SetDlgItemTextW(hwnd, cmb1, szDir);
			// 変更された。更新ボタンを有効にする。
			Changed(hwnd);
		}
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
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
