// MSettingsDialog.h --- [設定]ダイアログ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MPropSheet.h"
#include "MColorBox.h"
#include "board-image.h"
#include <functional>

class MSettingsDialog : public MPropSheetPage {
public:
	MBoardDisplay m_display;
	MStringW m_strFileName;
	MColorBox m_hwndWhite;
	MColorBox m_hwndBlack;
	MColorBox m_hwndMarked;
	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void()> m_fnApply;

	MSettingsDialog(const MBoardDisplay& display, const MStringW& strFileName) : MPropSheetPage(IDD_CONFIG) {
		m_strFileName = strFileName;
		m_display.CopyFrom(display);
	}

	// UIを更新する。
	void RefreshUI(HWND hwnd) {
		// 色をセットする。
		m_hwndWhite.SetColor(m_display.m_rgbWhiteCellColor);
		m_hwndBlack.SetColor(m_display.m_rgbBlackCellColor);
		m_hwndMarked.SetColor(m_display.m_rgbMarkedCellColor);

		// フォント名を格納する。
		::SetDlgItemTextW(hwnd, edt1, m_display.m_lfCellLogFont.lfFaceName);
		::SetDlgItemTextW(hwnd, edt2, m_display.m_lfSmallLogFont.lfFaceName);
		::SetDlgItemTextW(hwnd, edt3, m_display.m_lfSmallLogFont2.lfFaceName);

		// スケルトンビューか？
		BOOL bSkeletone = (m_display.m_nViewMode == XG_VIEW_SKELETON);
		::CheckDlgButton(hwnd, chx1, (bSkeletone ? BST_CHECKED : BST_UNCHECKED));

		// ナンクロか？
		BOOL bNumCroMode = m_display.m_bNumCroMode;
		::CheckDlgButton(hwnd, chx6, (bNumCroMode ? BST_CHECKED : BST_UNCHECKED));
		::EnableWindow(GetDlgItem(hwnd, chx6), m_display.m_bSolved);

		// 外枠をつけるか？
		::CheckDlgButton(hwnd, chx2, (m_display.m_bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));
		::EnableWindow(GetDlgItem(hwnd, edt7), m_display.m_bAddThickFrame);

		// 二重マスに枠をつけるか？
		::CheckDlgButton(hwnd, chx3, (m_display.m_bDrawFrameForMarkedCell ? BST_CHECKED : BST_UNCHECKED));
		// 英小文字か？
		::CheckDlgButton(hwnd, chx4, (m_display.m_bLowercase ? BST_CHECKED : BST_UNCHECKED));
		// ひらがなか？
		::CheckDlgButton(hwnd, chx5, (m_display.m_bHiragana ? BST_CHECKED : BST_UNCHECKED));

		// 文字の大きさ。
		::SetDlgItemInt(hwnd, edt4, m_display.m_nCellCharPercents, FALSE);
		::SetDlgItemInt(hwnd, edt5, m_display.m_nSmallCharPercents, FALSE);

		// 大きさの範囲を指定。
		::SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(100, 3));
		::SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(100, 3));

		// コンボボックスに項目を追加する。
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		ComboBox_ResetContent(hCmb1);
		ComboBox_AddString(hCmb1, XgLoadString(IDS_NONE).c_str());

		// 重複を避けて追加する(大文字小文字は区別しない)。
		auto AddUnique = [hCmb1](const MStringW& str) {
			if (ComboBox_FindStringExact(hCmb1, -1, str.c_str()) == CB_ERR)
				ComboBox_AddString(hCmb1, str.c_str());
		};

		auto block_dir = XgFindLocalFile(L"BLOCK");
		if (!block_dir.empty() && PathIsDirectoryW(block_dir.c_str())) {
			auto block_path = block_dir + L"\\*";
			WIN32_FIND_DATAW find;
			HANDLE hFind = FindFirstFileW(block_path.c_str(), &find);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
						continue;
					auto dotext = PathFindExtensionW(find.cFileName);
					if (lstrcmpi(dotext, L".emf") != 0 &&
						lstrcmpi(dotext, L".png") != 0 &&
						lstrcmpi(dotext, L".gif") != 0 &&
						lstrcmpi(dotext, L".jpg") != 0 &&
						lstrcmpi(dotext, L".jpeg") != 0 &&
						lstrcmpi(dotext, L".jpe") != 0 &&
						lstrcmpi(dotext, L".bmp") != 0)
					{
						continue;
					}
					MStringW str = L"$BLOCK\\";
					str += find.cFileName;
					AddUnique(str);
				} while (FindNextFileW(hFind, &find));
				FindClose(hFind);
			}
		}

		for (auto& image : m_display.m_images) {
			AddUnique(image.m_filename);
		}

		// 現在の黒マス画像を選択する(リストになければ追加して選ぶ)。
		INT iSel = 0;
		if (!m_display.m_strBlackCellImage.empty()) {
			AddUnique(m_display.m_strBlackCellImage);
			iSel = ComboBox_FindStringExact(hCmb1, -1, m_display.m_strBlackCellImage.c_str());
			if (iSel == CB_ERR)
				iSel = 0;
		}
		ComboBox_SetCurSel(hCmb1, iSel);

		// 二重マス文字。
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		for (INT i = IDS_DBLFRAME_LETTERS_1; i <= IDS_DBLFRAME_LETTERS_10; ++i) {
			ComboBox_AddString(hCmb2, XgLoadString(i).c_str());
		}
		ComboBox_SetText(hCmb2, m_display.m_strDoubleFrameLetters.c_str());

		WCHAR szText[MAX_PATH];

		// 線の太さ。
		StringCchPrintfW(szText, _countof(szText), XG_LINE_WIDTH_FORMAT, m_display.m_nLineWidthInPt);
		SetDlgItemTextW(hwnd, edt6, szText);

		// 外枠の幅。
		StringCchPrintfW(szText, _countof(szText), XG_OUTERFRAME_FORMAT, m_display.m_nOuterFrameInPt);
		SetDlgItemTextW(hwnd, edt7, szText);

		UpdateBlockPreview(hwnd);
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// ドロップを受け付ける。
		::DragAcceptFiles(hwnd, TRUE);

		// 画面の中央に寄せる。
		if (!m_bSheet)
			CenterWindowDx(hwnd);

		// ハンドルをセットする。
		m_hwndWhite.SubclassDx(GetDlgItem(hwnd, psh7));
		m_hwndBlack.SubclassDx(GetDlgItem(hwnd, psh8));
		m_hwndMarked.SubclassDx(GetDlgItem(hwnd, psh9));

		// UIを更新する。
		RefreshUI(hwnd);
		return TRUE;
	}

	void OnChange(HWND hwnd, INT i) {
		LOGFONTW lf;
		ZeroMemory(&lf, sizeof(lf));
		lf.lfQuality = ANTIALIASED_QUALITY;

		// ユーザーにフォント名を問い合わせる。
		CHOOSEFONTW cf;
		ZeroMemory(&cf, sizeof(cf));
		cf.lStructSize = sizeof(cf);
		cf.hwndOwner = hwnd;
		cf.lpLogFont = &lf;
		cf.nFontType = SCREEN_FONTTYPE | SIMULATED_FONTTYPE | REGULAR_FONTTYPE;
		lf.lfCharSet = DEFAULT_CHARSET;

		switch (i) {
		case 0: // マス フォント
			cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCALABLEONLY |
				CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
			lf = m_display.m_lfCellLogFont;
			lf.lfHeight = 24;
			lf.lfQuality = ANTIALIASED_QUALITY;
			if (::ChooseFontW(&cf)) {
				// 取得したフォントをダイアログへ格納する。
				::SetDlgItemTextW(hwnd, edt1, lf.lfFaceName);
				lf.lfHeight = 24;
				m_display.m_lfCellLogFont = lf;
			}
			break;

		case 1:
			cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCALABLEONLY |
				CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
			lf = m_display.m_lfSmallLogFont;
			lf.lfHeight = 12;
			if (::ChooseFontW(&cf)) {
				// 取得したフォントをダイアログへ格納する。
				::SetDlgItemTextW(hwnd, edt2, lf.lfFaceName);
				lf.lfHeight = 12;
				m_display.m_lfSmallLogFont = lf;
			}
			break;

		case 2:
			cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCALABLEONLY |
				CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
			lf = m_display.m_lfSmallLogFont;
			lf.lfHeight = 12;
			if (::ChooseFontW(&cf)) {
				// 取得したフォントをダイアログへ格納する。
				::SetDlgItemTextW(hwnd, edt3, lf.lfFaceName);
				lf.lfHeight = 12;
				m_display.m_lfSmallLogFont2 = lf;
			}
			break;

		default:
			break;
		}
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case edt1:
		case edt2:
		case edt3:
		case edt4:
		case edt5:
		case edt6:
		case edt7:
			if (codeNotify == EN_CHANGE) {
				Changed(hwnd);
			}
			break;
		case psh1:
		case psh2:
		case psh3:
		case psh4:
		case psh5:
		case psh6:
		case psh7:
		case psh8:
		case psh9:
		case psh10:
		case psh11:
		//case psh12:
		case psh13:
		case psh14:
		case psh15:
		case chx1:
		case chx2:
		case chx3:
		case chx4:
		case chx5:
		case chx6:
			if (codeNotify == BN_CLICKED) {
				Changed(hwnd);
			}
			break;
		case cmb1:
		case cmb2:
			if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELCHANGE ||
				codeNotify == CBN_SELENDOK)
			{
				Changed(hwnd);
			}
			break;
		}

		// 実際に処理をする。
		switch (id) {
		case psh1: // マスのフォント変更
			OnChange(hwnd, 0);
			break;

		case psh2: // 小さいフォント変更
			OnChange(hwnd, 1);
			break;

		case psh3: // マスのフォントリセット
			m_display.m_lfCellLogFont = {};
			StringCchCopyW(m_display.m_lfCellLogFont.lfFaceName, _countof(m_display.m_lfCellLogFont.lfFaceName), XgGetMonoFont().c_str());
			::SetDlgItemTextW(hwnd, edt1, m_display.m_lfCellLogFont.lfFaceName);
			break;

		case psh4: // 小さいフォントリセット
			m_display.m_lfSmallLogFont = {};
			StringCchCopyW(m_display.m_lfSmallLogFont.lfFaceName, _countof(m_display.m_lfSmallLogFont.lfFaceName), XgGetDefaultUIFont().c_str());
			::SetDlgItemTextW(hwnd, edt2, m_display.m_lfSmallLogFont.lfFaceName);
			break;

		case psh5: // 小さいフォント2変更
			OnChange(hwnd, 2);
			break;

		case psh6: // 小さいフォント2リセット
			m_display.m_lfSmallLogFont2 = {};
			StringCchCopyW(m_display.m_lfSmallLogFont2.lfFaceName, _countof(m_display.m_lfSmallLogFont2.lfFaceName), XgGetDefaultUIFont().c_str());
			::SetDlgItemTextW(hwnd, edt3, m_display.m_lfSmallLogFont2.lfFaceName);
			break;

		case psh7: // 白マスの色選択。
			m_hwndWhite.DoChooseColor();
			break;

		case psh8: // 黒マスの色選択。
			m_hwndBlack.DoChooseColor();
			break;

		case psh9: // 二重マスの色選択。
			m_hwndMarked.DoChooseColor();
			break;

		case psh10: // マス文字のサイズリセット。
			SetDlgItemInt(hwnd, edt4, XG_DEF_CELL_CHAR_SIZE, FALSE);
			break;

		case psh11: // 小さい文字のサイズリセット。
			SetDlgItemInt(hwnd, edt5, XG_DEF_SMALL_CHAR_SIZE, FALSE);
			break;

		case psh12: // メニューボタン。
		{
			HWND hPsh12 = GetDlgItem(hwnd, psh12);
			RECT rc;
			GetWindowRect(hPsh12, &rc);
			POINT pt = { rc.left, (rc.top + rc.bottom) / 2 };
			HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(3));
			HMENU hSubMenu = GetSubMenu(hMenu, 0);
			SetForegroundWindow(hwnd);
			TPMPARAMS params = { sizeof(params), rc };
			constexpr auto flags = TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_RETURNCMD;
			const auto id = TrackPopupMenuEx(hSubMenu, flags, pt.x, pt.y, hwnd, &params);
			if (id) {
				assert(id == psh13 || id == psh14 || id == psh15);
				::PostMessageW(hwnd, WM_COMMAND, id, 0);
			}
			::DestroyMenu(hMenu);
		}
		break;

		case psh13: // LOOKS情報のインポート。
			OnImportLooks(hwnd);
			break;

		case psh14: // LOOKS情報のエクスポート。
			OnExportLooks(hwnd);
			break;

		case psh15: // LOOKS情報のリセット。
			m_display.Reset2();
			RefreshUI(hwnd);
			break;

		case cmb1: // 黒マス画像。
			if (codeNotify == CBN_SELCHANGE || codeNotify == CBN_SELENDOK || codeNotify == CBN_EDITCHANGE) {
				UpdateBlockPreview(hwnd);
			}
			break;

		case chx1: // スケルトンビュー。
			if (codeNotify == BN_CLICKED) {
				if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED) {
					if (!m_bUpdating) {
						m_bUpdating = TRUE;
						{
							// 黒マス画像なし。
							HWND hCmb1 = GetDlgItem(hwnd, cmb1);
							ComboBox_SetCurSel(hCmb1, 0);
							UpdateBlockPreview(hwnd);
						}
						m_bUpdating = FALSE;
					}
				}
			}
			break;

		case chx2: // 外枠。
			if (codeNotify == BN_CLICKED) {
				BOOL bChecked = IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED;
				if (bChecked) {
					::EnableWindow(::GetDlgItem(hwnd, edt7), TRUE);
				} else { // 外枠がないなら、外枠の幅を指定しなくてもいいか。
					::EnableWindow(::GetDlgItem(hwnd, edt7), FALSE);
				}
			}
			break;

		default:
			break;
		}
	}

	// WM_NOTIFY
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) {
		switch (pnmhdr->code) {
		case PSN_APPLY: // 適用
			if (!OnOK(hwnd)) {
				return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
			}
			if (m_fnApply)
				m_fnApply();
			return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
		}
		return 0;
	}

	// WM_DROPFILES
	void OnDropFiles(HWND hwnd, HDROP hdrop) {
		WCHAR file[MAX_PATH];
		DragQueryFileW(hdrop, 0, file, _countof(file));
		if (lstrcmpiW(PathFindExtensionW(file), L".LOOKS") == 0) {
			if (XgImportLooksFile(m_display, file)) {
				RefreshUI(hwnd);
			}
		}
	}

	// WM_DRAWITEM
	void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem) {
		if (lpDrawItem->hwndItem == m_hwndWhite) {
			m_hwndWhite.OnOwnerDrawItem(lpDrawItem);
			return;
		}
		if (lpDrawItem->hwndItem == m_hwndBlack) {
			m_hwndBlack.OnOwnerDrawItem(lpDrawItem);
			return;
		}
		if (lpDrawItem->hwndItem == m_hwndMarked) {
			m_hwndMarked.OnOwnerDrawItem(lpDrawItem);
			return;
		}
	}

	void DoImportLooks(HWND hwnd, LPCWSTR filename) {
		XgImportLooksFile(m_display, filename);
		RefreshUI(hwnd);
	}

	void DoExportLooks(HWND hwnd, LPCWSTR filename) {
		XgExportLooksFile(m_display, filename);
	}

	void OnImportLooks(HWND hwnd) {
		WCHAR szFile[MAX_PATH] = L"";
		OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
		ofn.lpstrFilter = L"LOOKS File (*.looks)\0*.looks\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = _countof(szFile);
		auto title = XgLoadString(IDS_IMPORTLOOKSFILE);
		ofn.lpstrTitle = &title[0];
		ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		OFN_HIDEREADONLY;
		ofn.lpstrDefExt = L"LOOKS";
		if (::GetOpenFileNameW(&ofn)) {
			DoImportLooks(hwnd, szFile);
		}
	}
	void OnExportLooks(HWND hwnd) {
		WCHAR szFile[MAX_PATH] = L"";
		OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
		ofn.lpstrFilter = L"LOOKS File (*.looks)\0*.looks\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = _countof(szFile);
		auto title = XgLoadString(IDS_EXPORTLOOKSFILE);
		ofn.lpstrTitle = &title[0];
		ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt = L"LOOKS";
		if (::GetSaveFileNameW(&ofn)) {
			DoExportLooks(hwnd, szFile);
		}
	}

	BOOL OnOK(HWND hwnd) {
		// セルの文字の大きさ。
		BOOL bTranslated = FALSE;
		int nValue1 = GetDlgItemInt(hwnd, edt4, &bTranslated, FALSE);
		if (!(bTranslated && 0 <= nValue1 && nValue1 <= 100)) {
			// エラー。
			HWND hEdt4 = GetDlgItem(hwnd, edt4);
			Edit_SetSel(hEdt4, 0, -1);
			SetFocus(hEdt4);
			ErrorBoxDx(IDS_INVALIDVALUE);
			return FALSE;
		}

		// 小さい文字の大きさ。
		bTranslated = FALSE;
		int nValue2 = GetDlgItemInt(hwnd, edt5, &bTranslated, FALSE);
		if (!(bTranslated && 0 <= nValue2 && nValue2 <= 100)) {
			// エラー。
			HWND hEdt5 = GetDlgItem(hwnd, edt5);
			Edit_SetSel(hEdt5, 0, -1);
			SetFocus(hEdt5);
			ErrorBoxDx(IDS_INVALIDVALUE);
			return FALSE;
		}

		// 黒マス画像を取得し、実際に読み込む。
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		MStringW name = GetBlackImageName(hwnd);
		if (XgIsNoneImageName(name))
			name.clear();
		if (!m_display.SetBlackCellImage(name, nullptr)) {
			SetFocus(hCmb1);
			ErrorBoxDx(IDS_INVALIDVALUE);
			return FALSE;
		}

		// 文字の大きさの設定。
		m_display.m_nCellCharPercents = nValue1;
		m_display.m_nSmallCharPercents = nValue2;

		// スケルトンビューか？
		if (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
			m_display.m_nViewMode = XG_VIEW_SKELETON;
		else
			m_display.m_nViewMode = XG_VIEW_NORMAL;

		// ナンクロか？
		m_display.m_bNumCroMode = (::IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
		if (m_display.m_bNumCroMode) {
			XgMakeItNumCro(m_display);
		}

		// 太枠をつけるか？
		m_display.m_bAddThickFrame = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
		// 二重マスに枠をつけるか？
		m_display.m_bDrawFrameForMarkedCell = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
		// 英小文字か？
		m_display.m_bLowercase = (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
		// ひらがなか？
		m_display.m_bHiragana = (::IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);

		// 色を設定する。
		m_display.m_rgbWhiteCellColor = m_hwndWhite.GetColor();
		m_display.m_rgbBlackCellColor = m_hwndBlack.GetColor();
		m_display.m_rgbMarkedCellColor = m_hwndMarked.GetColor();

		// 二重マス文字。
		HWND hCmb2 = GetDlgItem(hwnd, cmb2);
		WCHAR szText[512];
		ComboBox_GetText(hCmb2, szText, _countof(szText));
		m_display.m_strDoubleFrameLetters = szText;

		// 線の太さ。
		GetDlgItemTextW(hwnd, edt6, szText, _countof(szText));
		MStringW str = szText;
		XgStrTrim(str);
		float value = wcstof(str.c_str(), nullptr);
		if (value > XG_MAX_LINEWIDTH)
			value = XG_MAX_LINEWIDTH;
		if (value < XG_MIN_LINEWIDTH)
			value = XG_MIN_LINEWIDTH;
		m_display.m_nLineWidthInPt = value;

		// 外枠の幅。
		GetDlgItemTextW(hwnd, edt7, szText, _countof(szText));
		str = szText;
		XgStrTrim(str);
		value = wcstof(str.c_str(), nullptr);
		if (value > XG_MAX_OUTERFRAME)
			value = XG_MAX_OUTERFRAME;
		if (value < XG_MIN_OUTERFRAME)
			value = XG_MIN_OUTERFRAME;
		m_display.m_nOuterFrameInPt = value;

		return TRUE;
	}

	// WM_DESTROY
	void OnDestroy(HWND hwnd) {
		ClearBlockPreview(hwnd);
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
			HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		}
		return 0;
	}

	INT_PTR DoModal(HWND hwnd) {
		m_bSheet = FALSE;
		return DialogBoxDx(hwnd, IDD_CONFIG);
	}

	void Changed(HWND hwnd) {
		if (m_bSheet)
			PropSheet_Changed(GetParent(hwnd), hwnd);
	}

	// プレビューを外して破棄する(STM_SETIMAGEは直前のハンドルを返す)。
	void ClearBlockPreview(HWND hwnd) {
		if (HWND hIco1 = GetDlgItem(hwnd, ico1)) {
			auto hbm = (HBITMAP)SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, 0);
			ShowWindow(hIco1, SW_HIDE);
			if (hbm)
				DeleteObject(hbm);
		}
		if (HWND hIco2 = GetDlgItem(hwnd, ico2)) {
			auto hEMF = (HENHMETAFILE)SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, 0);
			ShowWindow(hIco2, SW_HIDE);
			if (hEMF)
				DeleteEnhMetaFile(hEMF);
		}
	}

	static HBITMAP MakeThumbnail(HBITMAP hbm, INT cx, INT cy) {
		BITMAP bm;
		if (!GetObject(hbm, sizeof(bm), &bm))
			return nullptr;
		HDC hdc = GetDC(nullptr);
		HDC hdcSrc = CreateCompatibleDC(hdc);
		HDC hdcDst = CreateCompatibleDC(hdc);
		HBITMAP hbmNew = CreateCompatibleBitmap(hdc, cx, cy);
		HGDIOBJ hOld1 = SelectObject(hdcSrc, hbm);
		HGDIOBJ hOld2 = SelectObject(hdcDst, hbmNew);
		SetStretchBltMode(hdcDst, STRETCH_HALFTONE);
		SetBrushOrgEx(hdcDst, 0, 0, nullptr);
		StretchBlt(hdcDst, 0, 0, cx, cy, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		SelectObject(hdcSrc, hOld1);
		SelectObject(hdcDst, hOld2);
		DeleteDC(hdcSrc);
		DeleteDC(hdcDst);
		ReleaseDC(nullptr, hdc);
		return hbmNew;
	}

	// BLOCKのプレビュー。
	void UpdateBlockPreview(HWND hwnd) {
		ClearBlockPreview(hwnd);

		MStringW name = GetBlackImageName(hwnd);
		if (XgIsNoneImageName(name))
			return;

		HBITMAP hbm = nullptr;
		HENHMETAFILE hEMF = nullptr;
		if (!m_display.CopyImageHandles(name, nullptr, hbm, hEMF))
			return; // 読み込めなければ何も表示しない。

		constexpr INT PREVIEW_SIZE = 32;
		if (hbm) {
			HBITMAP hbmThumb = MakeThumbnail(hbm, PREVIEW_SIZE, PREVIEW_SIZE);
			DeleteObject(hbm);
			if (hbmThumb) {
				HWND hIco1 = GetDlgItem(hwnd, ico1);
				SetWindowPos(hIco1, nullptr, 0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
				             SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
				SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbmThumb);
			}
			if (hEMF) {
				DeleteEnhMetaFile(hEMF); // ビットマップ優先(描画側と同じ)。
				hEMF = nullptr;
			}
		}
		if (hEMF) {
			HWND hIco2 = GetDlgItem(hwnd, ico2);
			SetWindowPos(hIco2, nullptr, 0, 0, PREVIEW_SIZE, PREVIEW_SIZE,
			             SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
			SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)hEMF);
		}
	}

	// コンボボックスから黒マス画像名を取得する(「なし」は空文字)。
	MStringW GetBlackImageName(HWND hwnd, BOOL bFromList = FALSE) {
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		WCHAR szText[MAX_PATH] = L"";
		INT iSel = ComboBox_GetCurSel(hCmb1);
		INT len = (iSel != CB_ERR) ? ComboBox_GetLBTextLen(hCmb1, iSel) : CB_ERR;
		if (bFromList && len >= 0 && len < MAX_PATH)
			ComboBox_GetLBText(hCmb1, iSel, szText);
		else
			ComboBox_GetText(hCmb1, szText, _countof(szText));
		MStringW name = szText;
		XgStrTrim(name);
		if (name == XgLoadString(IDS_NONE))
			name.clear();
		return name;
	}

protected:
	BOOL m_bSheet = TRUE;
	BOOL m_bUpdating = FALSE;
};
