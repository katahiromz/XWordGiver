// MMarksDialog.h --- [二重マス単語の候補と配置]ダイアログ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "dict.h"
#include "marks.h"

#define MARKED_INTERVAL (3 * 1000)

// [二重マス単語の候補と配置]ダイアログ。
class MMarksDialog : public MDialogBase {
public:
	enum { REFRESH_TIMER_ID = 999 };
	BOOL m_bUpdating = FALSE;
	BOOL m_bInitted = FALSE;
	MBoardDisplay& m_display;
	MBoard& m_solution;
	MDict& m_dict;
	std::vector<MBoardPos>& m_vMarks;
	INT m_iMarkedCand = -1;
	MStringListW m_vMarkedCands;
	MStringW m_strMarked;
	INT m_nWindowX = CW_USEDEFAULT;
	INT m_nWindowY = CW_USEDEFAULT;
	std::vector<MMarkLayout> m_layouts; // 現在の単語の配置候補(良い順)。
	INT m_iLayout = -1;                 // 現在の配置のインデックス。
	MStringW m_strLayoutWord;           // m_layouts を作った単語。

	MMarksDialog(MDict& dict, MBoardDisplay& display) noexcept
		: m_display(display)
		, m_solution(display.m_solution)
		, m_dict(dict)
		, m_vMarks(display.m_vMarks)
	{
	}

	BOOL RefreshCandidates(HWND hwnd) {
		// 二重マス単語の候補を取得する。
		GetMarkedCandidates(m_vMarkedCands);

		// リストのクリアする。
		m_bUpdating = TRUE;
		::SendDlgItemMessageW(hwnd, lst1, LB_RESETCONTENT, 0, 0);
		m_bUpdating = FALSE;

		// リストに候補を追加する。
		for (auto& item : m_vMarkedCands) {
			::SendDlgItemMessageW(hwnd, lst1, LB_ADDSTRING, 0, (LPARAM)item.c_str());
		}

		RefreshMarks();
		return TRUE;
	}

	void RefreshMarks() {
		// 二重マス単語を取得。
		XgGetMarkWord(m_display, m_strMarked);

		// 選択を更新する。
		INT iItem = INT(::SendDlgItemMessageW(m_hwnd, lst1, LB_FINDSTRINGEXACT, -1, (LPARAM)m_strMarked.c_str()));
		m_bUpdating = TRUE;
		::SendDlgItemMessageW(m_hwnd, lst1, LB_SETCURSEL, ((iItem == LB_ERR) ? -1 : iItem), 0);
		m_bUpdating = FALSE;

		// テキストを更新。
		m_bUpdating = TRUE;
		::SetDlgItemTextW(m_hwnd, edt1, m_strMarked.c_str());
		m_bUpdating = FALSE;

		// ボタンの有効・無効を更新。
		UpdateButtons(m_hwnd);
	};

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// ダイアログを中央へ移動する。
		CenterWindowDx(hwnd);
		// 移動。
		if (m_nWindowX != CW_USEDEFAULT || m_nWindowY != CW_USEDEFAULT) {
			RECT rc;
			GetWindowRect(hwnd, &rc);
			MoveWindow(hwnd, m_nWindowX, m_nWindowY, rc.right - rc.left, rc.bottom - rc.top, TRUE);
			SendMessageW(hwnd, DM_REPOSITION, 0, 0);
		}
		// 候補を初期化する。
		RefreshCandidates(hwnd);
		// 初期化終わり。
		m_bInitted = TRUE;
		return TRUE;
	}

	// テキストの取得。
	MStringW GetText(HWND hwnd, BOOL bEdit, BOOL bNormalize = TRUE) {
		WCHAR szText[256];
		szText[0] = 0;
		if (bEdit) {
			::GetDlgItemTextW(hwnd, edt1, szText, _countof(szText));
		} else {
			const auto i = GetCurSel(hwnd);
			if (i != LB_ERR)
				::SendDlgItemMessageW(hwnd, lst1, LB_GETTEXT, i, reinterpret_cast<LPARAM>(szText));
		}
		MStringW str = szText;
		if (bNormalize) {
			XgStrTrim(str);
			str = XgNormalizeString(str);
		}
		return str;
	}

	BOOL SetText(HWND hwnd, const MStringW& str, BOOL bFromEdit) {
		if (!bFromEdit) {
			m_bUpdating = TRUE;
			INT iStart, iEnd;
			::SendDlgItemMessageW(hwnd, edt1, EM_GETSEL, reinterpret_cast<WPARAM>(&iStart), reinterpret_cast<LPARAM>(&iEnd));
			::SetDlgItemTextW(hwnd, edt1, str.c_str());
			::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, iStart, iEnd);
			m_bUpdating = FALSE;
		}

		BOOL bDone = FALSE;
		WCHAR chNotFound = 0;

		if (str.empty()) {
			// 空ならマークをクリア。
			SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_MARKS, 0);
			m_vMarks.clear();
			SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_MARKS, 0);
			ClearLayouts();
			bDone = TRUE;
		} else if (str == m_strLayoutWord && !m_layouts.empty()) {
			// 同じ単語なら配置候補を作り直さず、現在の配置を保つ。
			ApplyLayout(hwnd, m_iLayout);
			bDone = TRUE;
		} else {
			std::vector<MMarkLayout> layouts;
			if (XgGenerateMarkLayouts(m_display, str, layouts, &chNotFound)) {
				m_layouts = std::move(layouts);
				m_strLayoutWord = str;
				ApplyLayout(hwnd, 0); // 最も分散の良い配置から。
				bDone = TRUE;
			} else {
				ClearLayouts();
			}
		}

		if (bDone) {
			if (m_layouts.empty())
				::SetDlgItemTextW(hwnd, stc1, XgLoadString(IDS_MARKINGEMPTY).c_str());
			// 配置がある場合は ApplyLayout がステータスを表示済み。
		} else {
			if (str.size() && chNotFound != 0) {
				WCHAR szText[256];
				StringCchPrintfW(szText, _countof(szText),
								 XgLoadString(IDS_MARKINGTYPE).c_str(), chNotFound);
				::SetDlgItemTextW(hwnd, stc1, szText);
			} else {
				::SetDlgItemTextW(hwnd, stc1, XgLoadString(IDS_MARKINGEMPTY).c_str());
			}
		}

		if (bFromEdit) {
			const INT i = INT(::SendDlgItemMessageW(hwnd, lst1, LB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(str.c_str())));
			m_bUpdating = TRUE;
			::SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, (i != LB_ERR) ? i : -1, 0);
			m_bUpdating = FALSE;
		}

		// ボタンの有効・無効を更新。
		UpdateButtons(hwnd);
		return bDone;
	}

	// 配置 index を適用する(循環)。
	void ApplyLayout(HWND hwnd, INT iLayout) {
		const INT n = INT(m_layouts.size());
		if (n == 0)
			return;
		m_iLayout = ((iLayout % n) + n) % n;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_MARKS, 0);
		m_vMarks = m_layouts[m_iLayout];
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_MARKS, 0);
		UpdateLayoutStatus(hwnd);
	}

	// 「配置 3 / 64」を表示する。
	void UpdateLayoutStatus(HWND hwnd) {
		WCHAR szText[64];
		StringCchPrintfW(szText, _countof(szText),
			XgIsUserJapanese() ? L"配置 %d / %d" : L"Layout %d / %d",
			m_iLayout + 1, INT(m_layouts.size()));
		::SetDlgItemTextW(hwnd, stc1, szText);
	}

	void ClearLayouts() {
		m_layouts.clear();
		m_iLayout = -1;
		m_strLayoutWord.clear();
	}

	// ボタンを有効化・無効化する。
	// フォーカスのあるボタンを無効にするとキー操作が効かなくなるので、先に編集ボックスへ逃がす。
	void EnableButton(HWND hwnd, INT id, BOOL bEnable) noexcept {
		HWND hCtrl = ::GetDlgItem(hwnd, id);
		if (!hCtrl)
			return;
		if (!bEnable && ::GetFocus() == hCtrl)
			::SetFocus(::GetDlgItem(hwnd, edt1));
		::EnableWindow(hCtrl, bEnable);
	}

	// 状態に応じて、単語・配置・クリアのボタンを更新する。
	void UpdateButtons(HWND hwnd) {
		// 単語ボタン: 候補があれば有効。
		const BOOL bHasCands = !m_vMarkedCands.empty();
		EnableButton(hwnd, psh1, bHasCands);
		EnableButton(hwnd, psh2, bHasCands);

		// 配置ボタン: 単語があり、切り替え先がある(または未生成で生成できる)とき有効。
		const auto str = GetText(hwnd, TRUE);
		const BOOL bCanLayout = !str.empty() &&
			(str != m_strLayoutWord || m_layouts.size() >= 2);
		EnableButton(hwnd, psh3, bCanLayout);
		EnableButton(hwnd, psh4, bCanLayout);

		// クリアボタン: 二重マスか入力があれば有効。
		EnableButton(hwnd, psh5, !m_vMarks.empty() || !str.empty());
	}

	// 候補リストの項目を選んで適用する。
	void SelectCandidate(HWND hwnd, INT iCand) {
		auto str = m_vMarkedCands[iCand];
		m_bUpdating = TRUE;
		::SetDlgItemTextW(hwnd, edt1, str.c_str());
		m_bUpdating = FALSE;
		SetText(hwnd, str, TRUE);
	}

	VOID OnLst1(HWND hwnd) {
		const auto i = GetCurSel(hwnd);
		if (i == LB_ERR)
			return;

		// テキストを取得する。
		auto str = GetText(hwnd, FALSE);
		SetText(hwnd, str, FALSE);

		::KillTimer(hwnd, REFRESH_TIMER_ID);
		::SetTimer(hwnd, REFRESH_TIMER_ID, MARKED_INTERVAL, nullptr);
	}

	VOID OnEdt1(HWND hwnd) {
		// テキストを取得する。
		auto str = GetText(hwnd, TRUE);
		SetText(hwnd, str, TRUE);

		::KillTimer(hwnd, REFRESH_TIMER_ID);
		::SetTimer(hwnd, REFRESH_TIMER_ID, MARKED_INTERVAL, nullptr);
	}

	INT GetCurSel(HWND hwnd) noexcept {
		return INT(::SendDlgItemMessageW(hwnd, lst1, LB_GETCURSEL, 0, 0));
	}

	void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
		case IDCANCEL:
			// ダイアログを閉じる。
			if (m_bModal)
				::EndDialog(hwnd, id);
			else
				::DestroyWindow(hwnd);
			break;
		case edt1: // 二重マス単語
			if (codeNotify == EN_CHANGE) {
				if (!m_bUpdating)
					OnEdt1(hwnd);
			}
			break;
		case lst1: // 二重マス単語の候補リスト
			if (codeNotify == LBN_SELCHANGE) {
				if (!m_bUpdating)
					OnLst1(hwnd);
			}
			break;
		case psh1: // 前の単語
			if (m_vMarkedCands.size()) {
				const INT n = INT(m_vMarkedCands.size());
				const INT iItem = GetCurSel(hwnd);
				m_iMarkedCand = (iItem == LB_ERR) ? (n - 1) : ((iItem - 1 + n) % n);
				SelectCandidate(hwnd, m_iMarkedCand);
			}
			break;
		case psh2: // 次の単語
			if (m_vMarkedCands.size()) {
				const INT n = INT(m_vMarkedCands.size());
				const INT iItem = GetCurSel(hwnd);
				m_iMarkedCand = (iItem == LB_ERR) ? 0 : ((iItem + 1) % n);
				SelectCandidate(hwnd, m_iMarkedCand);
			}
			break;
		case psh3: // 前の配置。
		case psh4: // 次の配置。
			{
				auto str = GetText(hwnd, TRUE);
				if (str.empty())
					break;
				if (m_layouts.empty() || str != m_strLayoutWord) {
					// まだ配置候補がなければ生成して最良の配置を適用する。
					SetText(hwnd, str, TRUE);
				} else {
					ApplyLayout(hwnd, m_iLayout + (id == psh4 ? 1 : -1));
				}
			}
			break;
		case psh5: // クリア。
			SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_MARKS, 0);
			m_vMarks.clear();
			m_iMarkedCand = -1;
			SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_MARKS, 0);
			ClearLayouts();
			m_strMarked.clear();
			m_bUpdating = TRUE;
			::SetDlgItemTextW(hwnd, edt1, L"");
			::SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, (WPARAM)-1, 0); // リストの選択を解除
			m_bUpdating = FALSE;
			::SetDlgItemTextW(hwnd, stc1, XgLoadString(IDS_MARKINGEMPTY).c_str());
			UpdateButtons(hwnd);
			break;
		default:
			break;
		}
	}

	// WM_MOVE
	void OnMove(HWND hwnd, INT x, INT y) noexcept {
		if (m_bInitted) {
			RECT rc;
			::GetWindowRect(hwnd, &rc);
			m_nWindowX = rc.left;
			m_nWindowY = rc.top;
		}
	}

	// WM_TIMER
	void OnTimer(HWND hwnd, UINT id) {
		if (id != REFRESH_TIMER_ID)
			return;

		::KillTimer(hwnd, REFRESH_TIMER_ID);

		auto str1 = GetText(hwnd, TRUE, FALSE);
		auto str2 = GetText(hwnd, TRUE, TRUE);
		if (str1 != str2) {
			m_bUpdating = TRUE;
			INT iStart, iEnd;
			::SendDlgItemMessageW(hwnd, edt1, EM_GETSEL, reinterpret_cast<WPARAM>(&iStart), reinterpret_cast<LPARAM>(&iEnd));
			::SetDlgItemTextW(hwnd, edt1, str2.c_str());
			::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, iStart, iEnd);
			m_bUpdating = FALSE;
		}
	}

	// WM_DESTROY
	void OnDestroy(HWND hwnd) noexcept {
		::KillTimer(hwnd, REFRESH_TIMER_ID);
		m_bInitted = FALSE;
	}

	// 二重マス単語候補を取得する。
	bool __fastcall GetMarkedCandidates(MStringListW& cands) {
		// 解がなければ失敗。
		if (!m_display.m_bSolved)
			return false;

		// m_solutionから候補群を取得。
		bool bRet = XgGetMarkedCandidates(m_display, cands, m_dict);

		m_iMarkedCand = -1;
		return bRet;
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_MOVE, OnMove);
			HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		default:
			break;
		}
		return 0;
	}

	BOOL CreateDx(HWND hwnd)  {
		if (CreateDialogDx(hwnd, IDD_MARKING)) {
			::ShowWindow(*this, SW_SHOWNOACTIVATE);
			::UpdateWindow(*this);
			return TRUE;
		}
		return FALSE;
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_MARKING);
	}
};
