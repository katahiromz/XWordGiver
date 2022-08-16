﻿#pragma once

#include "XG_Window.hpp"
#include "XG_ColorBox.hpp"

#define DEFAULT_TEXTBOX_POINTSIZE 14

class XG_TextBoxDialog : public XG_Dialog
{
public:
    std::wstring m_strText;
    XG_ColorBox m_hwndTextColor;
    XG_ColorBox m_hwndBgColor;
    BOOL m_bTextColor = FALSE;
    BOOL m_bBgColor = FALSE;
    std::wstring m_strFontName;
    INT m_nFontSizeInPoints = 0;

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

    static INT CALLBACK EnumFontsProc(
        const LOGFONTW *lplf,
        const TEXTMETRICW *lptm,
        DWORD dwType,
        LPARAM lParam)
    {
        if (dwType != TRUETYPE_FONTTYPE)
            return TRUE;
        HWND hwnd = (HWND)lParam;
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_AddString(hCmb1, lplf->lfFaceName);
        return TRUE;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        SetDlgItemTextW(hwnd, edt1, m_strText.c_str());

        m_hwndTextColor.m_hWnd = GetDlgItem(hwnd, psh1);
        m_hwndBgColor.m_hWnd = GetDlgItem(hwnd, psh2);

        if (m_bTextColor)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (m_bBgColor)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);

        HDC hDC = CreateCompatibleDC(NULL);
        EnumFontsW(hDC, NULL, EnumFontsProc, (LPARAM)hwnd);
        DeleteDC(hDC);

        if (m_strFontName.size())
        {
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
            ComboBox_RealSetText(GetDlgItem(hwnd, cmb1), m_strFontName.c_str());
        }
        else
        {
            SetDlgItemTextW(hwnd, cmb1, xg_szCellFont);
            EnableWindow(GetDlgItem(hwnd, cmb1), FALSE);
        }

        if (m_nFontSizeInPoints)
        {
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
            SetDlgItemInt(hwnd, edt2, m_nFontSizeInPoints, FALSE);
        }
        else
        {
            SetDlgItemInt(hwnd, edt2, DEFAULT_TEXTBOX_POINTSIZE, FALSE);
            EnableWindow(GetDlgItem(hwnd, edt2), FALSE);
        }

        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(64, 8));

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

        if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            ComboBox_RealGetText(hCmb1, szText, _countof(szText));
            m_strFontName = szText;
            xg_str_trim(m_strFontName);
        }
        else
        {
            m_strFontName.clear();
        }

        if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
        {
            m_nFontSizeInPoints = GetDlgItemInt(hwnd, edt2, NULL, FALSE);
        }
        else
        {
            m_nFontSizeInPoints = 0;
        }

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
        case chx3:
            if (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED)
            {
                EnableWindow(GetDlgItem(hwnd, cmb1), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, cmb1), FALSE);
            }
            break;
        case chx4:
            if (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED)
            {
                EnableWindow(GetDlgItem(hwnd, edt2), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hwnd, edt2), FALSE);
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
