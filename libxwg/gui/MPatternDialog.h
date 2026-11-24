// MPatternDialog.h --- 黒マスパターンダイアログ
// Author: katahiromz
// License: MIT
// Q/C: OK

#pragma once

#include "xwg.h"
#include "pat-file-io.h"
#include "image-io.h"
#include "board-text-io.h"
#include "MWindowBase.h"
#include "layout.h"
#include <algorithm>
#include <vector>

#define XG_MAX_PAT_SIZE 20

// 黒マスパターンダイアログ。
class MPatternDialog : public MDialogBase {
public:
	BOOL m_bShowAnswerOnGenerate = TRUE; // xg_bShowAnswerOnGenerate
	MBoard m_selected; // 選択された盤面パターン。

	// セルのサイズ。
	static constexpr INT cxCell1 = 5, cyCell1 = 5; // 小さなセル。
	static constexpr INT cxCell2 = 3, cyCell2 = 3; // さらに小さなセル。
	static constexpr INT cxCell3 = 2, cyCell3 = 2; // さらにさらに小さなセル。

	// 黒マスパターンのデータ。
	inline static std::vector<MBoard> s_patterns;
	inline static LAYOUT_DATA *s_pLayout = nullptr;

	// 「黒マスパターン」ダイアログの位置とサイズ。
	inline static INT s_nWindowX = CW_USEDEFAULT;
	inline static INT s_nWindowY = CW_USEDEFAULT;
	inline static INT s_nWindowCX = CW_USEDEFAULT;
	inline static INT s_nWindowCY = CW_USEDEFAULT;

	explicit MPatternDialog(const MBoard& board) : m_selected(board) { }

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_BLOCKPATTERN);
	}

	// 「黒マスパターン」ダイアログプロシージャ。
	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_MEASUREITEM, OnMeasureItem);
			HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
			HANDLE_MSG(hwnd, WM_MOVE, OnMove);
			HANDLE_MSG(hwnd, WM_SIZE, OnSize);
			HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		default:
			break;
		}
		return 0;
	}

protected:
	//////////////////////////////////////////////////////////////////////////
	// フィルター

	// 大きさ(type0: rad1〜rad3)と形(type1: rad4〜rad6)で絞り込む。
	static BOOL FilterPatBySize(const MBoard& pat, INT type0, INT type1) noexcept {
		const INT cx = pat.Columns(), cy = pat.Rows();
		if (cx > XG_MAX_PAT_SIZE || cy > XG_MAX_PAT_SIZE)
			return FALSE;

		switch (type0) {
		case rad1: // 大きい。
			if (cx + cy < 28)
				return FALSE;
			break;
		case rad2: // 普通。
			if (!(cx + cy < 28 && cx + cy > 16))
				return FALSE;
			break;
		case rad3: // 小さい。
			if (!(cx + cy <= 16))
				return FALSE;
			break;
		default:
			assert(0);
			break;
		}

		switch (type1) {
		case rad4: // ヨコ長。
			if (cx <= cy)
				return FALSE;
			break;
		case rad5: // タテ長。
			if (cx >= cy)
				return FALSE;
			break;
		case rad6: // 正方形。
			if (cx != cy)
				return FALSE;
			break;
		default:
			assert(0);
			break;
		}

		return TRUE;
	}

	// 選択中の大きさのラジオボタンID。
	static INT GetType0(HWND hwnd) noexcept {
		if (IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED)
			return rad1;
		if (IsDlgButtonChecked(hwnd, rad3) == BST_CHECKED)
			return rad3;
		return rad2;
	}

	// 選択中の形のラジオボタンID。
	static INT GetType1(HWND hwnd) noexcept {
		if (IsDlgButtonChecked(hwnd, rad4) == BST_CHECKED)
			return rad4;
		if (IsDlgButtonChecked(hwnd, rad5) == BST_CHECKED)
			return rad5;
		return rad6;
	}

	//////////////////////////////////////////////////////////////////////////
	// リストの内容

	// パターンを読み込み、抽出する。
	BOOL PopulatePatterns(INT type0, INT type1) {
		s_patterns.clear();

		// PAT.txtからパターンを読み込む。
		MStringListW comments;
		HRESULT hr = XgLoadPatFile(XgFindLocalFile(L"PAT.txt").c_str(), s_patterns,
			                       m_selected.Rules(), comments);
		if (FAILED(hr)) {
			// TODO: エラーメッセージ
			return FALSE;
		}

		// ルールと条件に適合するものを抽出する。
		std::vector<MBoard> pats;
		for (auto& pat : s_patterns) {
			if (!pat.AreBlacksOK())
				continue;
			if (FilterPatBySize(pat, type0, type1))
				pats.push_back(pat);
		}
		s_patterns = std::move(pats);

		// かき混ぜる。
		XgRandomShuffle(s_patterns.begin(), s_patterns.end());
		return TRUE;
	}

	// パターンファイルを読み込み、条件に合うものをシャッフルしてリストに入れる。
	BOOL RefreshContents(HWND hwnd, INT type0, INT type1) {
		HWND hLst1 = GetDlgItem(hwnd, lst1);

		// リストボックスをクリアする。選択は無くなるのでボタンも無効にする。
		ListBox_ResetContent(hLst1);
		EnableWindow(GetDlgItem(hwnd, psh1), FALSE);
		EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);

		PopulatePatterns(type0, type1);

		// owner-drawのため、文字列は空で追加し、itemDataにインデックスを設定する。
		for (size_t i = 0; i < s_patterns.size(); ++i) {
			const int idx = ListBox_AddString(hLst1, L"");
			if (idx != LB_ERR)
				ListBox_SetItemData(hLst1, idx, static_cast<LPARAM>(i));
		}

		return TRUE;
	}

	// 選択中のパターンを取得する。なければnullptr。
	const MBoard *GetSelectedPattern(HWND hwnd) const {
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		const int iItem = ListBox_GetCurSel(hLst1);
		if (iItem == LB_ERR)
			return nullptr;
		const auto index = static_cast<size_t>(ListBox_GetItemData(hLst1, iItem));
		if (index >= s_patterns.size())
			return nullptr;
		return &s_patterns[index];
	}

	//////////////////////////////////////////////////////////////////////////
	// メッセージ ハンドラ

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		if (s_pLayout) {
			LayoutDestroy(s_pLayout);
			s_pLayout = nullptr;
		}

		if (m_bShowAnswerOnGenerate)
			CheckDlgButton(hwnd, chx1, BST_CHECKED);

		CheckRadioButton(hwnd, rad1, rad3, rad2);
		CheckRadioButton(hwnd, rad4, rad6, rad6);

		RefreshContents(hwnd, rad2, rad6);

		// リサイズ時のレイアウト。
		static const LAYOUT_INFO layouts[] = {
			{ stc1, BF_LEFT | BF_TOP },
			{ rad1, BF_LEFT | BF_TOP },
			{ rad2, BF_LEFT | BF_TOP },
			{ rad3, BF_LEFT | BF_TOP },
			{ rad4, BF_LEFT | BF_TOP },
			{ rad5, BF_LEFT | BF_TOP },
			{ rad6, BF_LEFT | BF_TOP },
			{ stc2, BF_LEFT | BF_TOP },
			{ lst1, BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM },
			{ psh1, BF_LEFT | BF_BOTTOM },
			{ chx1, BF_LEFT | BF_BOTTOM },
			{ IDOK, BF_RIGHT | BF_BOTTOM },
			{ IDCANCEL, BF_RIGHT | BF_BOTTOM },
		};
		s_pLayout = LayoutInit(hwnd, layouts, _countof(layouts));
		LayoutEnableResize(s_pLayout, TRUE);

		// 前回の位置とサイズを復元する。初回は中央に置いて記録する。
		if (s_nWindowX != CW_USEDEFAULT && s_nWindowCX != CW_USEDEFAULT) {
			MoveWindow(hwnd, s_nWindowX, s_nWindowY, s_nWindowCX, s_nWindowCY, TRUE);
			PostMessageW(hwnd, DM_REPOSITION, 0, 0);
		} else {
			CenterWindowDx(hwnd);
			RECT rc;
			GetWindowRect(hwnd, &rc);
			s_nWindowX = rc.left;
			s_nWindowY = rc.top;
			s_nWindowCX = rc.right - rc.left;
			s_nWindowCY = rc.bottom - rc.top;
		}

		SetFocus(GetDlgItem(hwnd, lst1));
		return FALSE; // フォーカスを自分で設定した。
	}

	// IDOK / psh1(コピー)。選択したパターンを確定してダイアログを閉じる。
	BOOL OnOKOrCopy(HWND hwnd, INT id) {
		const MBoard *pPat = GetSelectedPattern(hwnd);
		if (!pPat)
			return FALSE;

		m_selected = *pPat;
		m_bShowAnswerOnGenerate = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);

		::EndDialog(hwnd, id);
		return TRUE;
	}

	// ダウンロードする。
	void OnDownload(HWND hwnd) noexcept {
		ShellExecuteW(hwnd, nullptr, L"https://katahiromz.web.fc2.com/xword/patterns",
		              nullptr, nullptr, SW_SHOWNORMAL);
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
			OnOKOrCopy(hwnd, IDOK);
			break;
		case IDCANCEL:
			::EndDialog(hwnd, IDCANCEL);
			break;
		case psh1:
			OnOKOrCopy(hwnd, psh1);
			break;
		case psh2:
			OnDownload(hwnd);
			break;
		case lst1:
			if (codeNotify == LBN_DBLCLK) {
				OnOKOrCopy(hwnd, IDOK);
			} else if (codeNotify == LBN_SELCHANGE) {
				EnableWindow(GetDlgItem(hwnd, psh1), TRUE);
				EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
			}
			break;
		case rad1: case rad2: case rad3:
		case rad4: case rad5: case rad6:
			RefreshContents(hwnd, GetType0(hwnd), GetType1(hwnd));
			break;
		default:
			break;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// 描画

	// WM_MEASUREITEM
	void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT *lpMeasureItem) noexcept {
		if (lpMeasureItem->CtlType != ODT_LISTBOX || lpMeasureItem->CtlID != lst1)
			return;

		TEXTMETRIC tm = {};
		if (HDC hDC = CreateCompatibleDC(nullptr)) {
			HGDIOBJ hFontOld = SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
			GetTextMetrics(hDC, &tm);
			SelectObject(hDC, hFontOld);
			DeleteDC(hDC);
		}

		lpMeasureItem->itemWidth = cxCell1 * (XG_MAX_PAT_SIZE + 1) + 3;
		lpMeasureItem->itemHeight = cyCell1 * (XG_MAX_PAT_SIZE + 1) + 3 + tm.tmHeight;
	}

	// パターンの1マスのサイズを決める(大きすぎる場合は縮める)。
	static void GetCellSize(const MBoard& pat, INT& cxCell, INT& cyCell) noexcept {
		cxCell = cxCell1;
		cyCell = cyCell1;
		if (pat.Columns() > XG_MAX_PAT_SIZE || pat.Rows() > XG_MAX_PAT_SIZE) {
			cxCell = (cxCell1 * XG_MAX_PAT_SIZE) / pat.Columns();
			cyCell = (cyCell1 * XG_MAX_PAT_SIZE) / pat.Rows();
			cxCell = cyCell = (std::min)(cxCell, cyCell);
		}
	}

	// パターンの絵を、アイテム矩形の中央(テキストの下)に描く。
	static void DrawPattern(HDC hDC, const RECT& rcItem, INT yTop, const MBoard& pat) {
		INT cxCell, cyCell;
		GetCellSize(pat, cxCell, cyCell);

		const INT cx = cxCell * pat.Columns(), cy = cyCell * pat.Rows(); // 全体のサイズ。

		HDC hdcMem = CreateCompatibleDC(hDC);
		if (!hdcMem)
			return;

		if (HBITMAP hbm = XgCreate24BppBitmap(hdcMem, cx + 3, cy + 3)) {
			HGDIOBJ hbmOld = SelectObject(hdcMem, hbm);

			// 外枠(黒いペン、白いブラシ)。
			SelectObject(hdcMem, GetStockPen(BLACK_PEN));
			SelectObject(hdcMem, GetStockBrush(WHITE_BRUSH));
			Rectangle(hdcMem, 0, 0, cx + 2, cy + 2);

			// 黒マスを描画する。
			for (INT y = 0; y < pat.Rows(); ++y) {
				for (INT x = 0; x < pat.Columns(); ++x) {
					if (pat.GetCell(x, y) != ZEN_BLACK)
						continue;
					RECT rc;
					rc.left = 1 + x * cxCell;
					rc.top = 1 + y * cyCell;
					rc.right = rc.left + cxCell;
					rc.bottom = rc.top + cyCell;
					FillRect(hdcMem, &rc, GetStockBrush(BLACK_BRUSH));
				}
			}

			// 境界線を描画する。
			for (INT y = 0; y <= pat.Rows(); ++y) {
				MoveToEx(hdcMem, 1, 1 + y * cyCell, nullptr);
				LineTo(hdcMem, 1 + cx, 1 + y * cyCell);
			}
			for (INT x = 0; x <= pat.Columns(); ++x) {
				MoveToEx(hdcMem, 1 + x * cxCell, 1, nullptr);
				LineTo(hdcMem, 1 + x * cxCell, 1 + cy);
			}

			// ビットマップイメージをhDCに転送する。
			const INT cxItem = rcItem.right - rcItem.left;
			BitBlt(hDC, rcItem.left + (cxItem - (cx + 3)) / 2, yTop,
			       cx + 3, cy + 3, hdcMem, 0, 0, SRCCOPY);

			SelectObject(hdcMem, hbmOld);
			DeleteObject(hbm);
		}

		DeleteDC(hdcMem);
	}

	// WM_DRAWITEM
	void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem) {
		// リストボックスのlst1か？
		if (lpDrawItem->CtlType != ODT_LISTBOX || lpDrawItem->CtlID != lst1)
			return;

		// 空のリストにフォーカスがあるときはitemIDが-1になる。
		if (lpDrawItem->itemID == (UINT)-1)
			return;

		// itemDataはパターンのインデックス。
		const ULONG_PTR index = lpDrawItem->itemData;
		if (index >= s_patterns.size())
			return;
		const MBoard& pat = s_patterns[index];

		HDC hDC = lpDrawItem->hDC;
		RECT rcItem = lpDrawItem->rcItem;

		// フォーカスだけが変わった場合は、フォーカス枠だけ切り替える。
		if (lpDrawItem->itemAction & ODA_FOCUS)
			::DrawFocusRect(hDC, &rcItem);

		// 選択状態や全体の再描画でなければ何もしない。
		if (!(lpDrawItem->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
			return;

		// DCの状態(フォント、色、背景モード)を後で元に戻す。
		const int nSaved = SaveDC(hDC);

		SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
		SetBkMode(hDC, TRANSPARENT);
		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);

		// 背景と色。
		if (lpDrawItem->itemState & ODS_SELECTED) {
			FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
			SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		} else {
			FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
			SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
		}

		// サイズを表すテキスト。
		WCHAR szText[64];
		StringCchPrintfW(szText, _countof(szText), L"%d x %d = %d",
			pat.Columns(), pat.Rows(), pat.Columns() * pat.Rows());
		DrawTextW(hDC, szText, -1, &rcItem, DT_CENTER | DT_TOP | DT_SINGLELINE);

		// パターンの絵。
		DrawPattern(hDC, rcItem, rcItem.top + tm.tmHeight, pat);

		RestoreDC(hDC, nSaved);

		// フォーカス枠。
		if (lpDrawItem->itemState & ODS_FOCUS)
			DrawFocusRect(hDC, &rcItem);
	}

	//////////////////////////////////////////////////////////////////////////
	// ウィンドウの状態

	// WM_DESTROY
	void OnDestroy(HWND hwnd) {
		if (s_pLayout) {
			LayoutDestroy(s_pLayout);
			s_pLayout = nullptr;
		}
		s_patterns.clear();
	}

	// WM_MOVE
	void OnMove(HWND hwnd, INT x, INT y) noexcept {
		if (!IsMinimized(hwnd) && !IsMaximized(hwnd)) {
			RECT rc;
			GetWindowRect(hwnd, &rc);
			s_nWindowX = rc.left;
			s_nWindowY = rc.top;
		}
	}

	// WM_SIZE
	void OnSize(HWND hwnd, UINT state, INT cx, INT cy) {
		if (s_pLayout)
			LayoutUpdate(hwnd, s_pLayout, nullptr, 0);

		if (!IsMinimized(hwnd) && !IsMaximized(hwnd)) {
			RECT rc;
			GetWindowRect(hwnd, &rc);
			s_nWindowCX = rc.right - rc.left;
			s_nWindowCY = rc.bottom - rc.top;
		}
	}

	// WM_GETMINMAXINFO
	void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept {
		lpMinMaxInfo->ptMinTrackSize.x = 600;
		lpMinMaxInfo->ptMinTrackSize.y = 300;
	}

public:
	// 単体テスト。
	static void UnitTest(void) {
		constexpr INT test_rules = RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK |
		                           RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE |
		                           RULE_DONTTHREEDIAGONALS;
		MBoard b = XgMakeBoard({
			L".#..#...",
			L"..#...#.",
			L"....#...",
			L".#.#..#." }, test_rules);

		MPatternDialog dialog(b);
		auto ret = dialog.DoModal(nullptr);
		PrintfW(L"ret: %d\n", INT(ret));
		if (ret == IDCANCEL)
			return;

		MStringW text;
		XgTextFromBoard(text, dialog.m_selected);
		PrintfW(L"%s\n", text.c_str());
	}
};
