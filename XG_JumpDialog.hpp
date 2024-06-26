﻿#pragma once

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
            if (m_nType == 1)
                m_nType = 0;
        }

        if (m_nType == 0) {
            EnableWindow(GetDlgItem(hwnd, edt1), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt2), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt4), FALSE);
            EnableWindow(GetDlgItem(hwnd, rad3), FALSE);
            EnableWindow(GetDlgItem(hwnd, rad4), FALSE);
            CheckRadioButton(hwnd, rad1, rad2, rad1);
            SetDlgItemInt(hwnd, edt1, m_jCol, FALSE);
            SetDlgItemInt(hwnd, edt2, m_iRow, FALSE);
        } else if (m_nType == 1) {
            EnableWindow(GetDlgItem(hwnd, edt1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt2), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt4), FALSE);
            EnableWindow(GetDlgItem(hwnd, rad3), TRUE);
            EnableWindow(GetDlgItem(hwnd, rad4), TRUE);
            CheckRadioButton(hwnd, rad1, rad2, rad2);
            SetDlgItemInt(hwnd, edt3, m_nNumber, FALSE);
            CheckRadioButton(hwnd, rad3, rad4, (m_bVert ? rad4 : rad3));
        } else if (m_nType == 2) {
            EnableWindow(GetDlgItem(hwnd, edt1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt2), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt4), TRUE);
            EnableWindow(GetDlgItem(hwnd, rad3), FALSE);
            EnableWindow(GetDlgItem(hwnd, rad4), FALSE);
        }
    }

    BOOL OnOK(HWND hwnd)
    {
        if (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED)
            m_nType = 1;
        else if (IsDlgButtonChecked(hwnd, rad5) == BST_CHECKED)
            m_nType = 2;
        else
            m_nType = 0;

        m_bVert = (IsDlgButtonChecked(hwnd, rad4) == BST_CHECKED);
        m_jCol = GetDlgItemInt(hwnd, edt1, NULL, FALSE);
        m_iRow = GetDlgItemInt(hwnd, edt2, NULL, FALSE);
        m_nNumber = GetDlgItemInt(hwnd, edt3, NULL, FALSE);

        INT nFixed = 0;

        if (m_nType == 0) {
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
        }

        if (m_nType == 1) {
            if (m_nNumber < 1) {
                m_nNumber = 1;
                nFixed = edt3;
            } else if (m_bVert) {
                BOOL bFound = FALSE;
                for (auto& vert : xg_vVertInfo) {
                    if (m_nNumber == vert.m_number) {
                        bFound = TRUE;
                        break;
                    }
                }
                if (!bFound) {
                    INT nMax = 1;
                    for (auto& vert : xg_vVertInfo) {
                        if (nMax < vert.m_number)
                            nMax = vert.m_number;
                    }
                    m_nNumber = nMax;
                    nFixed = edt3;
                }
            } else {
                BOOL bFound = FALSE;
                for (auto& horz : xg_vHorzInfo) {
                    if (m_nNumber == horz.m_number) {
                        bFound = TRUE;
                        break;
                    }
                }
                if (!bFound) {
                    INT nMax = 1;
                    for (auto& horz : xg_vHorzInfo) {
                        if (nMax < horz.m_number)
                            nMax = horz.m_number;
                    }
                    m_nNumber = nMax;
                    nFixed = edt3;
                }
            }
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
            for (auto& vert : xg_vVertInfo) {
                if (vert.m_iRow == m_iRow - 1 && vert.m_jCol == m_jCol - 1) {
                    m_nNumber = vert.m_number;
                    m_bVert = TRUE;
                    break;
                }
            }
            for (auto& horz : xg_vHorzInfo) {
                if (horz.m_iRow == m_iRow - 1 && horz.m_jCol == m_jCol - 1) {
                    m_nNumber = horz.m_number;
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
                switch (m_nType)
                {
                case 0: // マス位置。
                    xg_caret_pos.m_j = m_jCol - 1;
                    xg_caret_pos.m_i = m_iRow - 1;
                    // 表示を更新する。
                    XgEnsureCaretVisible(hwnd);
                    XgUpdateStatusBar(hwnd);
                    // すぐに入力できるようにする。
                    SetFocus(hwnd);
                    break;
                case 1: // カギ位置。
                    XgJumpNumber(hwnd, m_nNumber, m_bVert);
                    break;
                case 2: // 文字または単語。
                    {
                        // テキストを取得する。
                        WCHAR szText[64];
                        GetDlgItemText(hwnd, edt4, szText, _countof(szText));
                        StrTrimW(szText, XG_WHITE_SPACES);
                        if (!szText[0])
                            return;

                        auto str = XgNormalizeString(szText);
                        XG_Board& xw = (xg_bSolved && xg_bShowAnswer) ? xg_solution : xg_xword;

                        // 次に進む関数。
                        auto GetNext = [&](INT& jCol, INT& iRow) {
                            ++jCol;
                            if (jCol == xg_nCols) {
                                jCol = 0;
                                ++iRow;
                                if (iRow == xg_nRows) {
                                    iRow = 0;
                                }
                            }
                        };
                        // ヨコの単語を取得する関数。
                        auto GetHorzWord = [&](INT jCol, INT iRow) -> XGStringW {
                            if (xw.GetAt(iRow, jCol) == ZEN_BLACK) {
                                return { ZEN_BLACK };
                            }
                            if (jCol == 0 || xw.GetAt(iRow, jCol - 1) == ZEN_BLACK) {
                                XGStringW ret;
                                for (INT j = jCol; j < xg_nCols; ++j) {
                                    WCHAR ch = xw.GetAt(iRow, j);
                                    if (ch == ZEN_BLACK)
                                        break;
                                    ret += ch;
                                }
                                return ret;
                            }
                            return { xw.GetAt(iRow, jCol) };
                        };
                        // タテの単語を取得する関数。
                        auto GetVertWord = [&](INT jCol, INT iRow) -> XGStringW {
                            if (xw.GetAt(iRow, jCol) == ZEN_BLACK) {
                                return { ZEN_BLACK };
                            }
                            if (iRow == 0 || xw.GetAt(iRow - 1, jCol) == ZEN_BLACK) {
                                XGStringW ret;
                                for (INT i = iRow; i < xg_nRows; ++i) {
                                    WCHAR ch = xw.GetAt(i, jCol);
                                    if (ch == ZEN_BLACK)
                                        break;
                                    ret += ch;
                                }
                                return ret;
                            }
                            return { xw.GetAt(iRow, jCol) };
                        };

                        // 検索する。
                        INT j = xg_caret_pos.m_j, i = xg_caret_pos.m_i;
                        for (INT n = 0; n < xg_nRows * xg_nCols; ++n) {
                            GetNext(j, i);
                            auto word0 = GetHorzWord(j, i);
                            auto j0 = word0.find(str);
                            if (j0 != word0.npos) {
                                xg_caret_pos.m_j = j + j0;
                                xg_caret_pos.m_i = i;
                                break;
                            }
                            auto word1 = GetVertWord(j, i);
                            auto i0 = word1.find(str);
                            if (i0 != word1.npos) {
                                xg_caret_pos.m_j = j;
                                xg_caret_pos.m_i = i + i0;
                                break;
                            }
                        }
                    }
                    // 表示を更新する。
                    XgEnsureCaretVisible(hwnd);
                    XgUpdateStatusBar(hwnd);
                    // すぐに入力できるようにする。
                    SetFocus(hwnd);
                    break;
                }
            }
            break;

        case IDCANCEL:
            EndDialog(IDCANCEL);
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
        case rad5:
            if (IsDlgButtonChecked(hwnd, rad5) == BST_CHECKED) {
                m_nType = 2;
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

    BOOL CreateDx(HWND hwnd) noexcept
    {
        return CreateDialogDx(hwnd, IDD_JUMP);
    }

    BOOL EndDialog(INT result)
    {
        return ::DestroyWindow(m_hWnd);
    }
};
