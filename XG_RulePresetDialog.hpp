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
class XG_RulePresetDialog
{
protected:
    BOOL m_bUpdating;

public:
    XG_RulePresetDialog() noexcept : m_bUpdating(FALSE)
    {
    }

    BOOL GetComboValue(HWND hwnd, int& value);
    void SetComboValue(HWND hwnd, int value);

    BOOL GetCheckValue(HWND hwnd, int& value) noexcept;
    void SetCheckValue(HWND hwnd, int value) noexcept;

    // コンボボックスの項目の高さ。
    void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) noexcept;
    // コンボボックスの項目を描画する。
    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem) noexcept;

    struct ENTRY
    {
        XGStringW language;
        DWORD value;
        XGStringW name;
    };

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

    void OnCmb1(HWND hwnd);
    void OnCheckBox(HWND hwnd, int rule, int id);
    BOOL OnOK(HWND hwnd);
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);

    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_MEASUREITEM, OnMeasureItem);
            HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        default:
            break;
        }
        return 0;
    }

    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static XG_RulePresetDialog* s_pThis = NULL;
        if (uMsg == WM_INITDIALOG)
        {
            s_pThis = (XG_RulePresetDialog*)lParam;
            SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)lParam);
        }
        else
        {
            s_pThis = (XG_RulePresetDialog*)GetWindowLongPtrW(hwnd, DWLP_USER);
        }

        if (!s_pThis)
            return 0;

        return s_pThis->DialogProcDx(hwnd, uMsg, wParam, lParam);
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxParamW(xg_hInstance, MAKEINTRESOURCEW(IDD_RULEPRESET), hwnd, DialogProc, (LPARAM)this);
    }
};
