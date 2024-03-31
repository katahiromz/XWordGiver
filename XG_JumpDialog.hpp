#pragma once

#include "XG_Window.hpp"

// 「ジャンプ」ダイアログ。
class XG_JumpDialog : public XG_Dialog
{
public:
    INT m_nType = 0; // ジャンプの種類。
    INT m_jCol = 1, m_iRow = 1; // マス位置。1ベース。
    BOOL m_bVert = FALSE; // タテか？
    INT m_nNumber = 1; // カギの番号。

    XG_JumpDialog() noexcept
    {
    }

    void UpdateUI(HWND hwnd)
    {
        if (!xg_bSolved) {
            m_nType = 0;
            m_nNumber = 1;
        }

        if (m_nType == 0) {
            EnableWindow(GetDlgItem(hwnd, edt1), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt2), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, rad3), FALSE);
            EnableWindow(GetDlgItem(hwnd, rad4), FALSE);
            CheckRadioButton(hwnd, rad1, rad2, rad1);
        } else {
            EnableWindow(GetDlgItem(hwnd, edt1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt2), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
            EnableWindow(GetDlgItem(hwnd, rad3), TRUE);
            EnableWindow(GetDlgItem(hwnd, rad4), TRUE);
            CheckRadioButton(hwnd, rad1, rad2, rad2);
        }

        CheckRadioButton(hwnd, rad3, rad4, (m_bVert ? rad4 : rad3));

        SetDlgItemInt(hwnd, edt1, m_jCol, FALSE);
        SetDlgItemInt(hwnd, edt2, m_iRow, FALSE);
        SetDlgItemInt(hwnd, edt3, m_nNumber, FALSE);
    }

    BOOL OnOK(HWND hwnd)
    {
        m_nType = (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED);
        m_bVert = (IsDlgButtonChecked(hwnd, rad4) == BST_CHECKED);
        m_jCol = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
        m_iRow = GetDlgItemInt(hwnd, edt2, NULL, FALSE);
        m_nNumber = GetDlgItemInt(hwnd, edt3, NULL, FALSE);

        INT nFixed = 0;
        if (m_jCol <= 0) {
            m_jCol = 1;
            nFixed = edt1;
        } else if (m_jCol > xg_nCols) {
            m_jCol = xg_nCols;
            nFixed = edt1;
        }

        if (m_iRow <= 0) {
            m_iRow = 1;
            nFixed = edt2;
        } else if (m_iRow > xg_nRows) {
            m_iRow = xg_nRows;
            nFixed = edt2;
        }

        if (m_nType == 1) {
            if (m_nNumber < 1) {
                m_nNumber = 1;
                nFixed = edt3;
            } else if (m_bVert) {
                BOOL bFound = FALSE;
                for (auto& tate : xg_vVertInfo) {
                    if (m_nNumber == tate.m_number) {
                        bFound = TRUE;
                        break;
                    }
                }
                if (!bFound) {
                    INT nMax = 1;
                    for (auto& tate : xg_vVertInfo) {
                        if (nMax < tate.m_number)
                            nMax = tate.m_number;
                    }
                    m_nNumber = nMax;
                    nFixed = edt3;
                }
            } else {
                BOOL bFound = FALSE;
                for (auto& yoko : xg_vHorzInfo) {
                    if (m_nNumber == yoko.m_number) {
                        bFound = TRUE;
                        break;
                    }
                }
                if (!bFound) {
                    INT nMax = 1;
                    for (auto& yoko : xg_vHorzInfo) {
                        if (nMax < yoko.m_number)
                            nMax = yoko.m_number;
                    }
                    m_nNumber = nMax;
                    nFixed = edt3;
                }
            }
        } else {
            m_nNumber = 1;
        }

        if (nFixed) {
            SetFocus(GetDlgItem(hwnd, nFixed));
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_OUTOFRANGE), nullptr, MB_ICONERROR);
            UpdateUI(hwnd);
            return FALSE;
        }

        return TRUE;
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);

        m_nType = 0;
        m_jCol = xg_caret_pos.m_j + 1;
        m_iRow = xg_caret_pos.m_i + 1;
        m_bVert = FALSE;
        m_nNumber = 1;

        if (xg_bSolved) {
            for (auto& tate : xg_vVertInfo) {
                if (tate.m_iRow == m_iRow - 1 && tate.m_jCol == m_jCol - 1) {
                    m_nNumber = tate.m_number;
                    m_bVert = TRUE;
                    break;
                }
            }
            for (auto& yoko : xg_vHorzInfo) {
                if (yoko.m_iRow == m_iRow - 1 && yoko.m_jCol == m_jCol - 1) {
                    m_nNumber = yoko.m_number;
                    m_bVert = FALSE;
                    break;
                }
            }
            EnableWindow(GetDlgItem(hwnd, rad2), TRUE);
        } else {
            EnableWindow(GetDlgItem(hwnd, rad2), FALSE);
        }

        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(xg_nCols, 1));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(xg_nRows, 1));
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(9999, 1));

        UpdateUI(hwnd);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept
    {
        switch(id)
        {
        case IDOK:
            if (OnOK(hwnd))
            {
                ::EndDialog(hwnd, id);
            }
            break;

        case IDCANCEL:
            ::EndDialog(hwnd, id);
            break;

        case rad1:
            if (IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED) {
                m_nType = 0;
                UpdateUI(hwnd);
            }
            break;
        case rad2:
            if (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED) {
                m_nType = 1;
                UpdateUI(hwnd);
            }
            break;
        default:
            break;
        }
    }

    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_JUMP);
    }
};
