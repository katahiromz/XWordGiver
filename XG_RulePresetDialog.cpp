
BOOL XG_RulePresetDialog::GetComboValue(HWND hwnd, int& value)
{
    WCHAR szText[64];
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    ComboBox_RealGetText(hCmb1, szText, _countof(szText));

    // 前後の空白を取り除く。
    XGStringW str = szText;
    xg_str_trim(str);

    // 半角にする。
    LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH, str.c_str(), -1, szText, _countof(szText));

    // 値にする。
    LPWSTR pch;
    value = wcstoul(szText, &pch, 0);

    // 不正な値があるか？
    if (value & ~XG_VALID_RULES) {
        return FALSE;
    }

    value |= RULE_DONTDIVIDE; // 例外。

    return TRUE;
}

void XG_RulePresetDialog::SetComboValue(HWND hwnd, int value)
{
    WCHAR szText[256];
    StringCchPrintfW(szText, _countof(szText), L"0x%04X:", value);

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    const int iItem = ComboBox_FindString(hCmb1, -1, szText);
    if (iItem != CB_ERR)
    {
        ComboBox_GetLBText(hCmb1, iItem, szText);
    }
    else
    {
        StringCchPrintfW(szText, _countof(szText), L"0x%04X", value);
    }
    ComboBox_RealSetText(hCmb1, szText);
}

BOOL XG_RulePresetDialog::GetCheckValue(HWND hwnd, int& value) noexcept
{
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
    value |= RULE_DONTDIVIDE; // 例外。
    return TRUE;
}

void XG_RulePresetDialog::SetCheckValue(HWND hwnd, int value) noexcept
{
    value |= RULE_DONTDIVIDE; // 例外。
    if (value & RULE_1) CheckDlgButton(hwnd, chx1, BST_CHECKED);
    if (value & RULE_2) CheckDlgButton(hwnd, chx2, BST_CHECKED);
    if (value & RULE_3) CheckDlgButton(hwnd, chx3, BST_CHECKED);
    if (value & RULE_4) CheckDlgButton(hwnd, chx4, BST_CHECKED);
    if (value & RULE_5) CheckDlgButton(hwnd, chx5, BST_CHECKED);
    else if (value & RULE_6) CheckDlgButton(hwnd, chx6, BST_CHECKED);
    if (value & RULE_7) CheckDlgButton(hwnd, chx7, BST_CHECKED);
    if (value & RULE_8) CheckDlgButton(hwnd, chx8, BST_CHECKED);
    if (value & RULE_9) CheckDlgButton(hwnd, chx9, BST_CHECKED);
}

void XG_RulePresetDialog::OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) noexcept
{
    if (lpMeasureItem->CtlType != ODT_COMBOBOX)
        return;

    HDC hDC = CreateCompatibleDC(nullptr);
    SelectObject(hDC, GetWindowFont(hwnd));
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);
    DeleteDC(hDC);

    lpMeasureItem->itemHeight = tm.tmHeight + 8;
}

void XG_RulePresetDialog::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem) noexcept
{
    if (lpDrawItem->CtlType != ODT_COMBOBOX)
        return;

    HDC hDC = lpDrawItem->hDC;
    RECT rcItem = lpDrawItem->rcItem;
    const int iItem = lpDrawItem->itemID;

    // 選択状態に応じて色の取得。背景を塗りつぶす。
    int iBackColor;
    const BOOL bSelected = (lpDrawItem->itemState & ODS_SELECTED);
    if (bSelected)
    {
        iBackColor = COLOR_HIGHLIGHT;
        SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        iBackColor = COLOR_WINDOW;
        SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }
    FillRect(hDC, &rcItem, GetSysColorBrush(iBackColor));

    // テキストを取得。
    WCHAR szText[MAX_PATH];
    szText[0] = 0;
    if (iItem >= 0)
        ComboBox_GetLBText(lpDrawItem->hwndItem, iItem, szText);

    InflateRect(&rcItem, -3, -3); // 項目を小さくする。

    // テキスト描画。
    SetBkMode(hDC, TRANSPARENT);
    constexpr auto uFormat = DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX;
    DrawTextW(hDC, szText, -1, &rcItem, uFormat);

    // フォーカスを描画。
    if (lpDrawItem->itemState & ODS_FOCUS)
    {
        InflateRect(&rcItem, 2, 2);
        DrawFocusRect(hDC, &rcItem);
    }
}

BOOL XG_RulePresetDialog::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    xg_ahSyncedDialogs[I_SYNCED_RULES] = hwnd;
    // ダイアログを中央へ移動する。
    XgCenterDialog(hwnd);

    // 項目を追加する。
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);

    // POLICY.txtを開いて全部読む。
    WCHAR szPath[MAX_PATH];
    XGStringW strText;
    std::vector<ENTRY> entries;
    if (XgFindLocalFile(szPath, _countof(szPath), L"POLICY.txt") &&
        XgReadTextFileAll(szPath, strText))
    {
        // 行に分割する。
        std::vector<XGStringW> lines;
        mstr_split(lines, strText, L"\n");

        // 各行について。
        for (auto& line : lines) {
            // 各行の前後の空白を除去する。
            mstr_trim(line, L" \r\n");

            // コメントは除去する。
            const size_t ich = line.find(L';');
            if (ich != line.npos) {
                line = line.substr(0, ich);
            }

            // タブで分割。
            std::vector<XGStringW> fields;
            mstr_split(fields, line, L"\t");
            for (auto& field : fields) {
                mstr_trim(field, L" ");
            }

            // エントリを追加する。
            if (fields.size() == 2) {
                ENTRY entry;
                entry.value = wcstoul(fields[0].c_str(), nullptr, 0);
                entry.name = fields[1];
                entries.push_back(entry);
            } else if (fields.size() == 3) {
                ENTRY entry;
                entry.language = fields[0];
                entry.value = wcstoul(fields[1].c_str(), nullptr, 0);
                entry.name = fields[2];
                entries.push_back(entry);
            }
        }
    }

    if (entries.empty()) { // エントリがなければ
        // リソースからコンボボックス項目を追加。
        for (int id = IDS_POLICYPRESET_SKELETON_0; id <= IDS_POLICYPRESET_JPN_LOOSE_3; ++id) {
            ComboBox_AddString(hCmb1, XgLoadStringDx1(id));
        }
    } else {
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
            XGStringW str = sz;
            str += L": ";
            str += entry.name;
            ComboBox_AddString(hCmb1, str.c_str());
        }
    }

    // 値をセットする。
    m_bUpdating = TRUE;
    {
        SetComboValue(hwnd, xg_nRules);
        SetCheckValue(hwnd, xg_nRules);
    }
    m_bUpdating = FALSE;

    EnableWindow(GetDlgItem(hwnd, chx4), FALSE);
    return TRUE;
}

void XG_RulePresetDialog::OnCmb1(HWND hwnd)
{
    int value;
    if (GetComboValue(hwnd, value))
    {
        m_bUpdating = TRUE;
        {
            CheckDlgButton(hwnd, chx1, (value & RULE_1) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, chx2, (value & RULE_2) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, chx3, (value & RULE_3) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, chx4, (value & RULE_4) ? BST_CHECKED : BST_UNCHECKED);
            if (value & RULE_5)
            {
                CheckDlgButton(hwnd, chx5, BST_CHECKED);
                CheckDlgButton(hwnd, chx6, BST_UNCHECKED);
            }
            else
            {
                CheckDlgButton(hwnd, chx5, BST_UNCHECKED);
                if (value & RULE_6)
                    CheckDlgButton(hwnd, chx6, BST_CHECKED);
                else
                    CheckDlgButton(hwnd, chx6, BST_UNCHECKED);
            }
            CheckDlgButton(hwnd, chx7, (value & RULE_7) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, chx8, (value & RULE_8) ? BST_CHECKED : BST_UNCHECKED);
            CheckDlgButton(hwnd, chx9, (value & RULE_9) ? BST_CHECKED : BST_UNCHECKED);
        }
        m_bUpdating = FALSE;
    }
    PropSheet_Changed(GetParent(hwnd), hwnd);
}

void XG_RulePresetDialog::OnCheckBox(HWND hwnd, int rule, int id)
{
    m_bUpdating = TRUE;
    if (IsDlgButtonChecked(hwnd, id) == BST_CHECKED)
    {
        switch (id)
        {
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
    m_bUpdating = FALSE;

    int value;
    if (!GetCheckValue(hwnd, value))
        return;

    m_bUpdating = TRUE;
    {
        SetComboValue(hwnd, value);
    }
    m_bUpdating = FALSE;

    PropSheet_Changed(GetParent(hwnd), hwnd);
}

void XG_RulePresetDialog::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_bUpdating)
        return;

    switch (id)
    {
    case psh1:
        // ルールの説明を開く。
        XgOpenRulesTxt(hwnd);
        break;
    case cmb1:
        OnCmb1(hwnd);
        break;
    case chx1:
        OnCheckBox(hwnd, RULE_DONTDOUBLEBLACK, id);
        break;
    case chx2:
        OnCheckBox(hwnd, RULE_DONTCORNERBLACK, id);
        break;
    case chx3:
        OnCheckBox(hwnd, RULE_DONTTRIDIRECTIONS, id);
        break;
    case chx4:
        OnCheckBox(hwnd, RULE_DONTDIVIDE, id);
        break;
    case chx5:
        OnCheckBox(hwnd, RULE_DONTTHREEDIAGONALS, id);
        break;
    case chx6:
        OnCheckBox(hwnd, RULE_DONTFOURDIAGONALS, id);
        break;
    case chx7:
        OnCheckBox(hwnd, RULE_POINTSYMMETRY, id);
        break;
    case chx8:
        OnCheckBox(hwnd, RULE_LINESYMMETRYV, id);
        break;
    case chx9:
        OnCheckBox(hwnd, RULE_LINESYMMETRYH, id);
        break;
    default:
        break;
    }
}

LRESULT XG_RulePresetDialog::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    switch (pnmhdr->code)
    {
    case PSN_APPLY:
        if (!OnOK(hwnd)) {
            return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
        }
        break;
    }
    return 0;
}

BOOL XG_RulePresetDialog::OnOK(HWND hwnd)
{
    int value;
    if (!GetComboValue(hwnd, value))
    {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), nullptr, MB_ICONERROR);
        return FALSE;
    }

    // ルールを適用。
    xg_nRules = (value & XG_VALID_RULES);
    xg_nRules |= RULE_DONTDIVIDE; // 例外。
    return TRUE;
}
