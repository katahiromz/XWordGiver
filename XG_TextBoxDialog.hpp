#pragma once

#include "XG_Window.hpp"
#include "XG_ColorBox.hpp"

class XG_TextBoxDialog : public XG_Dialog
{
public:
    std::wstring m_strText;
    XG_ColorBox m_hwndTextColor;
    XG_ColorBox m_hwndBgColor;
    BOOL m_bTextColor = FALSE;
    BOOL m_bBgColor = FALSE;

    BOOL GetTextColor()
    {
        if (m_bTextColor)
            return m_hwndTextColor.GetColor();
        else
            return CLR_INVALID;
    }
    void SetTextColor(COLORREF rgbText, BOOL bEnable)
    {
        m_hwndTextColor.SetColor(rgbText);
        m_bTextColor = bEnable;
    }

    BOOL GetBgColor()
    {
        if (m_bBgColor)
            return m_hwndBgColor.GetColor();
        else
            return CLR_INVALID;
    }
    void SetBgColor(COLORREF rgbBg, BOOL bEnable)
    {
        m_hwndBgColor.SetColor(rgbBg);
        m_bBgColor = bEnable;
    }

    XG_TextBoxDialog()
    {
        m_hwndTextColor.SetColor(CLR_INVALID);
        m_hwndBgColor.SetColor(CLR_INVALID);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        SetWindowText(hEdt1, m_strText.c_str());

        m_hwndTextColor.m_hWnd = GetDlgItem(hwnd, psh1);
        m_hwndBgColor.m_hWnd = GetDlgItem(hwnd, psh2);

        if (m_bTextColor)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (m_bBgColor)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        return TRUE;
    }

    BOOL OnOK(HWND hwnd)
    {
        HWND hEdt1 = GetDlgItem(hwnd, edt1);

        WCHAR szText[MAX_PATH] = L"";
        GetWindowText(hEdt1, szText, _countof(szText));
        m_strText = szText;
        xg_str_trim_right(m_strText);

        m_bTextColor = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        m_bBgColor = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);

        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            if (OnOK(hwnd)) {
                // ダイアログを終了。
                ::EndDialog(hwnd, id);
            }
            break;
        case IDCANCEL:
            // ダイアログを終了。
            ::EndDialog(hwnd, id);
            break;
        case psh1:
            m_hwndTextColor.DoChooseColor();
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
            break;
        case psh2:
            m_hwndBgColor.DoChooseColor();
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
            break;
        case chx1:
            if (IsDlgButtonChecked(hwnd, chx1) != BST_CHECKED)
            {
                m_hwndTextColor.SetColor(xg_rgbBlackCellColor);
            }
            break;
        case chx2:
            if (IsDlgButtonChecked(hwnd, chx2) != BST_CHECKED)
            {
                m_hwndBgColor.SetColor(xg_rgbWhiteCellColor);
            }
            break;
        }
    }

    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
    {
        if (lpDrawItem->hwndItem == m_hwndTextColor) {
            m_hwndTextColor.OnOwnerDrawItem(lpDrawItem);
            return;
        }
        if (lpDrawItem->hwndItem == m_hwndBgColor) {
            m_hwndBgColor.OnOwnerDrawItem(lpDrawItem);
            return;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_TEXTBOX);
    }
};
