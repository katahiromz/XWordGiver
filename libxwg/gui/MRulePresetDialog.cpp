// MRulePresetDialog.cpp --- [ルール プリセット]ダイアログ。
// Author: katahiromz
// License: MIT

#include "MRulePresetDialog.h"
#include "file-io.h"

#include <cerrno>

// 排他ルールを正規化する(SetCheckValue と同じ優先順位: 5>6, 7>8>9)。
static INT XgNormalizeRules(INT v) noexcept {
	v |= RULE_DONTDIVIDE;
	if ((v & RULE_5) && (v & RULE_6))
		v &= ~RULE_6;
	if (v & RULE_7)
		v &= ~(RULE_8 | RULE_9);
	else if (v & RULE_8)
		v &= ~RULE_9;
	return v;
}

BOOL MRulePresetDialog::GetComboValue(HWND hwnd, INT& value, BOOL bUseListSel) {
	WCHAR text[256] = L"";
	HWND hCmb1 = GetDlgItem(hwnd, cmb1);

	if (bUseListSel) { // CBN_SELCHANGE 中だけリスト項目を読む。
		const INT iItem = ComboBox_GetCurSel(hCmb1);
		if (iItem == CB_ERR || ComboBox_GetLBTextLen(hCmb1, iItem) >= (INT)_countof(text))
			return FALSE;
		ComboBox_GetLBText(hCmb1, iItem, text);
	} else { // それ以外は常に編集欄を信頼する。
		if (!GetWindowTextW(hCmb1, text, _countof(text)))
			return FALSE;
	}

	MStringW str = text;
	XgStrTrim(str);

	WCHAR half[256];
	if (!LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH, str.c_str(), -1, half, _countof(half)))
		return FALSE;

	LPWSTR pch = nullptr;
	errno = 0;
	const unsigned long n = wcstoul(half, &pch, 0);
	if (pch == half || errno == ERANGE)
		return FALSE;
	while (*pch == L' ' || *pch == L'\t')
		++pch;
	if (*pch != 0 && *pch != L':') // "0x0028: ..." も許す
		return FALSE;
	if (n & ~(unsigned long)XG_VALID_RULES)
		return FALSE;

	value = XgNormalizeRules((INT)n);
	return TRUE;
}

void MRulePresetDialog::SetComboValue(HWND hwnd, INT value) {
	HWND hCmb1 = GetDlgItem(hwnd, cmb1);

	WCHAR text[256];
	StringCchPrintfW(text, _countof(text), L"0x%04X:", value);
	const INT iItem = ComboBox_FindString(hCmb1, -1, text);
	if (iItem != CB_ERR) {
		ComboBox_SetCurSel(hCmb1, iItem); // 編集欄も自動で更新される。
	} else {
		ComboBox_SetCurSel(hCmb1, -1);    // 古い選択を必ず解除する。
		StringCchPrintfW(text, _countof(text), L"0x%04X", value);
		ComboBox_SetText(hCmb1, text);
	}
}

BOOL MRulePresetDialog::GetCheckValue(HWND hwnd, INT& value) noexcept {
	value = 0;
	if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED) value |= RULE_1;
	if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) value |= RULE_2;
	if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED) value |= RULE_3;
	if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED) value |= RULE_4;
	if (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED) value |= RULE_5;
	if (IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED) value |= RULE_6;
	if (IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED) value |= RULE_7;
	if (IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED) value |= RULE_8;
	if (IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED) value |= RULE_9;
	value |= RULE_DONTDIVIDE; // 強制。
	return TRUE;
}

void MRulePresetDialog::SetCheckValue(HWND hwnd, INT value) noexcept {
	value = XgNormalizeRules(value);
	CheckDlgButton(hwnd, chx1, (value & RULE_1) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx2, (value & RULE_2) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx3, (value & RULE_3) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx4, (value & RULE_4) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx5, (value & RULE_5) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx6, (value & RULE_6) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx7, (value & RULE_7) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx8, (value & RULE_8) ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(hwnd, chx9, (value & RULE_9) ? BST_CHECKED : BST_UNCHECKED);
}

void MRulePresetDialog::OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) noexcept {
	if (lpMeasureItem->CtlType != ODT_COMBOBOX)
		return;

	HDC hDC = CreateCompatibleDC(nullptr);
	SelectObject(hDC, GetWindowFont(hwnd));
	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);
	DeleteDC(hDC);

	lpMeasureItem->itemHeight = tm.tmHeight + 8;
}

void MRulePresetDialog::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem) noexcept {
	if (lpDrawItem->CtlType != ODT_COMBOBOX)
		return;

	HDC hDC = lpDrawItem->hDC;
	RECT rcItem = lpDrawItem->rcItem;
	const INT iItem = lpDrawItem->itemID;

	// 選択状態に応じて色の取得。背景を塗りつぶす。
	INT iBackColor;
	const BOOL bSelected = (lpDrawItem->itemState & ODS_SELECTED);
	if (bSelected) {
		iBackColor = COLOR_HIGHLIGHT;
		SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	} else {
		iBackColor = COLOR_WINDOW;
		SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
	}
	FillRect(hDC, &rcItem, GetSysColorBrush(iBackColor));

	// テキストを取得。
	WCHAR text[MAX_PATH];
	text[0] = 0;
	if (iItem >= 0 && ComboBox_GetLBTextLen(lpDrawItem->hwndItem, iItem) < (INT)_countof(text))
		ComboBox_GetLBText(lpDrawItem->hwndItem, iItem, text);

	InflateRect(&rcItem, -3, -3); // 項目を小さくする。

	// テキスト描画。
	SetBkMode(hDC, TRANSPARENT);
	constexpr auto uFormat = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX;
	DrawTextW(hDC, text, -1, &rcItem, uFormat);

	// フォーカスを描画。
	if (lpDrawItem->itemState & ODS_FOCUS) {
		InflateRect(&rcItem, 2, 2);
		DrawFocusRect(hDC, &rcItem);
	}
}

BOOL MRulePresetDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
	// ダイアログを中央へ移動する。
	CenterWindowDx(hwnd);

	// 項目を追加する。
	HWND hCmb1 = GetDlgItem(hwnd, cmb1);

	// POLICY.txtを開いて全部読む。
	MStringW strText;
	std::vector<ENTRY> entries;
	if (XgReadTextFileAll(XgFindLocalFile(L"POLICY.txt").c_str(), strText)) {
		// 行に分割する。
		std::vector<MStringW> lines;
		XgStrSplit(lines, strText, L"\n");

		// 各行について。
		for (auto& line : lines) {
			// 各行の前後の空白を除去する。
			XgStrTrim(line);

			// コメントは除去する。
			const size_t ich = line.find(L';');
			if (ich != line.npos) {
				line = line.substr(0, ich);
			}

			// タブで分割。
			std::vector<MStringW> fields;
			XgStrSplit(fields, line, L"\t");
			for (auto& field : fields) {
				XgStrTrim(field);
			}

			// エントリを追加する。
			if (fields.size() == 2) {
				ENTRY entry;
				entry.value = wcstoul(fields[0].c_str(), nullptr, 0);
				entry.name = fields[1];
				if (entry.value & ~XG_VALID_RULES)
					continue;
				entries.push_back(entry);
			} else if (fields.size() == 3) {
				ENTRY entry;
				entry.language = fields[0];
				entry.value = wcstoul(fields[1].c_str(), nullptr, 0);
				entry.name = fields[2];
				if (entry.value & ~XG_VALID_RULES)
					continue;
				entries.push_back(entry);
			}
		}
	}

	// エントリをコンボボックスに追加。
	for (auto& entry : entries) {
		if (entry.language.size()) {
			// 僕らは分かち合えない。
			if (XgIsUserJapanese()) {
				if (entry.language != L"JPN")
					continue;
			} else {
				if (entry.language == L"JPN")
					continue;
			}
		}

		WCHAR sz[32];
		StringCchPrintfW(sz, _countof(sz), L"0x%04X", entry.value);
		MStringW str = sz;
		str += L": ";
		str += entry.name;
		ComboBox_AddString(hCmb1, str.c_str());
	}

	// 1件も追加されなかったらリソースから追加。
	if (ComboBox_GetCount(hCmb1) == 0) {
		// リソースからコンボボックス項目を追加。
		for (INT id = IDS_POLICYPRESET_SKELETON_0; id <= IDS_POLICYPRESET_JPN_LOOSE_3; ++id) {
			ComboBox_AddString(hCmb1, XgLoadString(id).c_str());
		}
	}

	// 値をセットする。
	m_bUpdating = TRUE;
	{
		SetComboValue(hwnd, m_nRules);
		SetCheckValue(hwnd, m_nRules);
	}
	m_bUpdating = FALSE;

	EnableWindow(GetDlgItem(hwnd, chx4), FALSE);
	return TRUE;
}

void MRulePresetDialog::OnCmb1(HWND hwnd, UINT codeNotify) {
	const BOOL bList = (codeNotify == CBN_SELCHANGE || codeNotify == CBN_SELENDOK);
	INT value;
	if (!GetComboValue(hwnd, value, bList))
		return; // 入力途中の不正値は無視する(編集欄は触らない)。

	m_bUpdating = TRUE;
	SetCheckValue(hwnd, value);
	m_bUpdating = FALSE;

	Changed(hwnd);
}

void MRulePresetDialog::OnCheckBox(HWND hwnd, INT id) {
	// 排他処理: 押した方を優先して、他方を外す。
	m_bUpdating = TRUE;
	if (IsDlgButtonChecked(hwnd, id) == BST_CHECKED) {
		switch (id) {
		case chx5:
			CheckDlgButton(hwnd, chx6, BST_UNCHECKED);
			break;
		case chx6:
			CheckDlgButton(hwnd, chx5, BST_UNCHECKED);
			break;
		case chx7:
			CheckDlgButton(hwnd, chx8, BST_UNCHECKED);
			CheckDlgButton(hwnd, chx9, BST_UNCHECKED);
			break;
		case chx8:
			CheckDlgButton(hwnd, chx7, BST_UNCHECKED);
			CheckDlgButton(hwnd, chx9, BST_UNCHECKED);
			break;
		case chx9:
			CheckDlgButton(hwnd, chx7, BST_UNCHECKED);
			CheckDlgButton(hwnd, chx8, BST_UNCHECKED);
			break;
		default:
			break;
		}
	}

	INT value;
	GetCheckValue(hwnd, value);
	value = XgNormalizeRules(value);
	SetComboValue(hwnd, value); // CURSEL も同時に更新される
	m_bUpdating = FALSE;

	Changed(hwnd);
}

void MRulePresetDialog::OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify)
{
	if (m_bUpdating)
		return;

	switch (id) {
	case IDOK:
		if (!m_bSheet && OnOK(hwnd))
			EndDialog(hwnd, id);
		break;
	case IDCANCEL:
		if (!m_bSheet)
			EndDialog(hwnd, id);
		break;
	case psh1:
		if (codeNotify == BN_CLICKED) {
			if (!m_bSheet) {
				// テスト用。
				if (OnOK(hwnd))
					EndDialog(hwnd, IDOK);
			} else {
				// ルールの説明を開く。
				PostMessageW(xg_hMainWnd, WM_COMMAND, ID_OPENRULESTXT, 0);
			}
		}
		break;
	case cmb1:
		if (codeNotify == CBN_EDITCHANGE || codeNotify == CBN_SELCHANGE || codeNotify == CBN_SELENDOK) {
			OnCmb1(hwnd, codeNotify);
		} else if (codeNotify == CBN_KILLFOCUS) {
			INT value;
			if (GetComboValue(hwnd, value, FALSE)) { // 不正値のときは触らない
				m_bUpdating = TRUE;
				SetComboValue(hwnd, value);
				m_bUpdating = FALSE;
			}
		}
		break;
	case chx1:
	case chx2:
	case chx3:
	case chx4:
	case chx5:
	case chx6:
	case chx7:
	case chx8:
	case chx9:
		if (codeNotify == BN_CLICKED)
			OnCheckBox(hwnd, id);
		break;
	default:
		break;
	}
}

LRESULT MRulePresetDialog::OnNotify(HWND hwnd, INT idFrom, LPNMHDR pnmhdr)
{
	switch (pnmhdr->code) {
	case PSN_APPLY:
		if (!OnOK(hwnd)) // 入力が不正ならページを離れさせない
			return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
		if (m_fnApply)
			m_fnApply(m_nRules);
		return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
	}
	return 0;
}

BOOL MRulePresetDialog::OnOK(HWND hwnd) {
	INT value;
	if (!GetComboValue(hwnd, value, FALSE)) { // 編集欄の文字を信頼する
		ErrorBoxDx(IDS_ENTERPOSITIVE);
		SetFocus(GetDlgItem(hwnd, cmb1));
		return FALSE;
	}
	m_nRules = value;
	return TRUE;
}

void MRulePresetDialog::UnitTest() {
	MRulePresetDialog dialog(0);
	auto ret = dialog.DoModal(nullptr);
	PrintfW(L"ret: %d, rules: %d\n", ret, dialog.m_nRules);
	INT rules = dialog.m_nRules;
	if (rules & RULE_1) PrintfW(L"連黒禁。\n"); // RULE_DONTDOUBLEBLACK
	if (rules & RULE_2) PrintfW(L"四隅黒禁。\n"); // RULE_DONTCORNERBLACK
	if (rules & RULE_3) PrintfW(L"三方黒禁。\n"); // RULE_DONTTRIDIRECTIONS
	if (rules & RULE_4) PrintfW(L"分断禁。\n"); // RULE_DONTDIVIDE
	if (rules & RULE_5) PrintfW(L"黒斜三連禁。\n"); // RULE_DONTTHREEDIAGONALS
	if (rules & RULE_6) PrintfW(L"黒斜四連禁。\n"); // RULE_DONTFOURDIAGONALS
	if (rules & RULE_7) PrintfW(L"黒マス点対称。\n"); // RULE_POINTSYMMETRY
	if (rules & RULE_8) PrintfW(L"黒マス線対称（タテ）。\n"); // RULE_LINESYMMETRYV
	if (rules & RULE_9) PrintfW(L"黒マス線対称（ヨコ）。\n"); // RULE_LINESYMMETRYH
}
