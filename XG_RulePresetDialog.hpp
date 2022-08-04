#pragma once

#include "XG_Window.hpp"

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
class XG_RulePresetDialog : public XG_Dialog
{
protected:
    BOOL m_bUpdating;

public:
    XG_RulePresetDialog() : m_bUpdating(FALSE)
    {
    }

    BOOL GetComboValue(HWND hwnd, INT& value)
    {
        WCHAR szText[64];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));

        // 前後の空白を取り除く。
        std::wstring str = szText;
        xg_str_trim(str);

        // 半角にする。
        LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH, str.c_str(), -1, szText, _countof(szText));

        // 値にする。
        LPWSTR pch;
        value = wcstoul(szText, &pch, 0);

        // 不正な値があるか？
        if (*pch || (value & ~VALID_RULES))
        {
            return FALSE;
        }

        value |= RULE_DONTDIVIDE; // 例外。

        return TRUE;
    }

    void SetComboValue(HWND hwnd, INT value)
    {
        WCHAR szText[64];
        StringCbPrintfW(szText, sizeof(szText), L"%u", value);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealSetText(hCmb1, szText);
    }

    BOOL GetCheckValue(HWND hwnd, INT& value)
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

    void SetCheckValue(HWND hwnd, INT value)
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

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);

        m_bUpdating = TRUE;
        {
            SetComboValue(hwnd, xg_nRules);
            SetCheckValue(hwnd, xg_nRules);
        }
        m_bUpdating = FALSE;

        EnableWindow(GetDlgItem(hwnd, chx4), FALSE);
        return TRUE;
    }

    void OnCmb1(HWND hwnd)
    {
        INT value;
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
    }

    void OnCheckBox(HWND hwnd, INT rule, INT id)
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
            }
        }
        m_bUpdating = FALSE;

        INT value;
        if (!GetCheckValue(hwnd, value))
            return;

        m_bUpdating = TRUE;
        {
            SetComboValue(hwnd, value);
        }
        m_bUpdating = FALSE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (m_bUpdating)
            return;

        switch (id)
        {
        case IDOK:
            {
                INT value;
                if (!GetComboValue(hwnd, value))
                {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), NULL, MB_ICONERROR);
                    return;
                }

                // ルールを適用。
                xg_nRules = (value & VALID_RULES);
                xg_nRules |= RULE_DONTDIVIDE; // 例外。

                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            }
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
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
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_RULEPRESET);
    }
};
