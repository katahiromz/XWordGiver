// MRulePresetDialog.h --- [ルール プリセット]ダイアログ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MPropSheet.h"
#include <functional>

#define RULE_1 RULE_DONTDOUBLEBLACK
#define RULE_2 RULE_DONTCORNERBLACK
#define RULE_3 RULE_DONTTRIDIRECTIONS
#define RULE_4 RULE_DONTDIVIDE
#define RULE_5 RULE_DONTTHREEDIAGONALS
#define RULE_6 RULE_DONTFOURDIAGONALS
#define RULE_7 RULE_POINTSYMMETRY
#define RULE_8 RULE_LINESYMMETRYV
#define RULE_9 RULE_LINESYMMETRYH

// [ルール プリセット]ダイアログ。
class MRulePresetDialog : public MPropSheetPage {
public:
	INT m_nRules = 0;
	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void(INT)> m_fnApply;

	MRulePresetDialog(INT nRules) : MPropSheetPage(IDD_RULEPRESET) {
		m_nRules = nRules | RULE_DONTDIVIDE;
	}

	BOOL GetComboValue(HWND hwnd, INT& value, BOOL bUseListSel = FALSE);
	void SetComboValue(HWND hwnd, INT value);

	BOOL GetCheckValue(HWND hwnd, INT& value) noexcept;
	void SetCheckValue(HWND hwnd, INT value) noexcept;

	// コンボボックスの項目の高さ。
	void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) noexcept;
	// コンボボックスの項目を描画する。
	void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem) noexcept;

	struct ENTRY {
		MStringW language;
		DWORD value;
		MStringW name;
	};

	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) override;
	void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) override;
	LRESULT OnNotify(HWND hwnd, INT idFrom, LPNMHDR pnmhdr) override;
	BOOL OnOK(HWND hwnd);

	void OnCmb1(HWND hwnd, UINT codeNotify);
	void OnCheckBox(HWND hwnd, INT id);

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
		case WM_MEASUREITEM:
			OnMeasureItem(hwnd, (MEASUREITEMSTRUCT*)lParam);
			return SetDlgMsgResult(hwnd, uMsg, TRUE);
		case WM_DRAWITEM:
			OnDrawItem(hwnd, (const DRAWITEMSTRUCT*)lParam);
			return SetDlgMsgResult(hwnd, uMsg, TRUE);
		default:
			break;
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
	BOOL m_bUpdating = FALSE;
	BOOL m_bSheet = TRUE;

public:
	static void UnitTest();
};
