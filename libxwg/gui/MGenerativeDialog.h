// MGenerativeDialog.h --- 「生成」設定
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MPropSheet.h"
#include <functional>

class MGenerativeDialog : public MPropSheetPage {
public:
	BOOL m_bNoGeneratedMsg = FALSE;
	BOOL m_bNoCanceledMsg = FALSE;
	WCHAR m_aszSoundFiles[3][MAX_PATH] = {};

	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void()> m_fnApply;

	MGenerativeDialog() : MPropSheetPage(IDD_GENERATIVE)
	{
	}

	// 音声コンボボックスを初期化。
	BOOL InitSound(HWND hwnd, INT nComboID, INT nCheckID, LPCWSTR pszPathName)
	{
		HWND hCmb = GetDlgItem(hwnd, nComboID);

		if (pszPathName[0])
			::CheckDlgButton(hwnd, nCheckID, BST_CHECKED);
		else
			::EnableWindow(hCmb, FALSE);

		// 「(なし)」を追加して選択。
		ComboBox_AddString(hCmb, XgLoadString(IDS_NONE).c_str());
		ComboBox_SetCurSel(hCmb, 0);

		// SOUND (音声)フォルダ。
		auto sound_dir = XgFindLocalFile(L"SOUND");

		// WAVファイルの列挙を開始する。
		WIN32_FIND_DATAW find;
		auto path = sound_dir + L"\\*.wav";
		HANDLE hFind = FindFirstFileW(path.c_str(), &find);
		if (hFind == INVALID_HANDLE_VALUE)
			return FALSE;

		do
		{
			// コンボボックスに追加。
			INT iItem = ComboBox_AddString(hCmb, find.cFileName);

			// 一致すれば選択。
			if (lstrcmpiW(find.cFileName, PathFindFileNameW(pszPathName)) == 0)
				ComboBox_SetCurSel(hCmb, iItem);
		} while (FindNextFileW(hFind, &find));

		FindClose(hFind);
		return TRUE;
	}

	// 音を鳴らす。
	void MyPlaySound(HWND hwnd, INT nID)
	{
		// SOUND (音声)フォルダ
		auto sound_dir = XgFindLocalFile(L"SOUND");

		// コンボボックスからテキストを取得。
		WCHAR szText[MAX_PATH];
		ComboBox_GetText(GetDlgItem(hwnd, nID), szText, _countof(szText));

		// 「(なし)」は鳴らさない。
		if (lstrcmpiW(szText, XgLoadString(IDS_NONE).c_str()) == 0)
			return;

		// パス名を構築して、音声を鳴らす。
		auto path = sound_dir + L"\\" + szText;
		::PlaySoundW(path.c_str(), nullptr, SND_ASYNC | SND_FILENAME);
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		if (!m_bSheet)
			CenterWindowDx(hwnd);
		// チェックボックスとコンボボックスを初期化。
		if (m_bNoGeneratedMsg)
			::CheckDlgButton(hwnd, chx1, BST_CHECKED);
		if (m_bNoCanceledMsg)
			::CheckDlgButton(hwnd, chx2, BST_CHECKED);
		if (!InitSound(hwnd, cmb1, chx4, m_aszSoundFiles[0]))
			EnableWindow(GetDlgItem(hwnd, chx4), FALSE);
		if (!InitSound(hwnd, cmb2, chx5, m_aszSoundFiles[1]))
			EnableWindow(GetDlgItem(hwnd, chx5), FALSE);
		if (!InitSound(hwnd, cmb3, chx6, m_aszSoundFiles[2]))
			EnableWindow(GetDlgItem(hwnd, chx6), FALSE);
		return TRUE;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		// 変更があれば「更新」ボタンを有効にする。
		switch (id) {
		case chx1:
		case chx2:
		case chx3:
		case chx4:
		case chx5:
		case chx6:
			if (codeNotify == BN_CLICKED)
				Changed(hwnd);
			break;
		case cmb1:
		case cmb2:
		case cmb3:
			if (codeNotify == CBN_SELCHANGE || codeNotify == CBN_SELENDOK)
				Changed(hwnd);
			break;
		}
		// チェックボックスの状態によりコンボボックスの有効化・無効化を切り替える。
		// 「再生」ボタンで音を出す。
		switch (id) {
		case chx4:
			if (codeNotify == BN_CLICKED)
				EnableWindow(GetDlgItem(hwnd, cmb1), IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
			break;
		case chx5:
			if (codeNotify == BN_CLICKED)
				EnableWindow(GetDlgItem(hwnd, cmb2), IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
			break;
		case chx6:
			if (codeNotify == BN_CLICKED)
				EnableWindow(GetDlgItem(hwnd, cmb3), IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
			break;
		case psh1:
			if (codeNotify == BN_CLICKED) {
				MyPlaySound(hwnd, cmb1);
			}
			break;
		case psh2:
			if (codeNotify == BN_CLICKED) {
				MyPlaySound(hwnd, cmb2);
			}
			break;
		case psh3:
			if (codeNotify == BN_CLICKED) {
				MyPlaySound(hwnd, cmb3);
			}
			break;
		}
	}

	BOOL OnOK(HWND hwnd) {
		// 設定を取得する。
		m_bNoGeneratedMsg = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
		m_bNoCanceledMsg = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);

		// いったん音声ファイルの情報をクリアする。
		m_aszSoundFiles[0][0] = 0;
		m_aszSoundFiles[1][0] = 0;
		m_aszSoundFiles[2][0] = 0;

		// 音声フォルダ
		auto sound_dir = XgFindLocalFile(L"SOUND");

		// コンボボックスとチェックボックスの状態に応じて音声ファイルを設定する。
		WCHAR szText[MAX_PATH];
		ComboBox_GetText(GetDlgItem(hwnd, cmb1), szText, _countof(szText));
		if (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED && szText[0]) {
			if (lstrcmpiW(szText, XgLoadString(IDS_NONE).c_str()) != 0) {
				StringCchCopyW(m_aszSoundFiles[0], _countof(m_aszSoundFiles[0]), sound_dir.c_str());
				PathAppendW(m_aszSoundFiles[0], szText);
			}
		}
		ComboBox_GetText(GetDlgItem(hwnd, cmb2), szText, _countof(szText));
		if (::IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED && szText[0]) {
			if (lstrcmpiW(szText, XgLoadString(IDS_NONE).c_str()) != 0) {
				StringCchCopyW(m_aszSoundFiles[1], _countof(m_aszSoundFiles[1]), sound_dir.c_str());
				PathAppendW(m_aszSoundFiles[1], szText);
			}
		}
		ComboBox_GetText(GetDlgItem(hwnd, cmb3), szText, _countof(szText));
		if (::IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED && szText[0]) {
			if (lstrcmpiW(szText, XgLoadString(IDS_NONE).c_str()) != 0) {
				StringCchCopyW(m_aszSoundFiles[2], _countof(m_aszSoundFiles[2]), sound_dir.c_str());
				PathAppendW(m_aszSoundFiles[2], szText);
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
