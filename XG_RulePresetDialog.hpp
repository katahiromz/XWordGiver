﻿#pragma once

#include "XG_Dialog.hpp"

// [ルール プリセット]ダイアログ。
class XG_RulePresetDialog : public XG_Dialog
{
public:
    XG_RulePresetDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);

        WCHAR szText[64];
        StringCbPrintfW(szText, sizeof(szText), L"%u", xg_nRules);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealSetText(hCmb1, szText);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            {
                WCHAR szText[64];
                HWND hCmb1 = GetDlgItem(hwnd, cmb1);
                ComboBox_RealGetText(hCmb1, szText, _countof(szText));

                // 前後の空白を取り除く。
                std::wstring str = szText;
                xg_str_trim(str);

                // ルールを適用。
                xg_nRules = (wcstoul(str.c_str(), NULL, 0) & VALID_RULES);
                xg_nRules |= RULE_DONTDIVIDE; // 例外。

                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            }
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
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
