#pragma once

#include "XG_Dialog.hpp"

// [問題の作成]ダイアログ。
class XG_GenDialog : public XG_Dialog
{
public:
    XG_GenDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, xg_nCols, FALSE);

        // 現在の状態で好ましいと思われる単語の最大長を取得する。
        INT n3 = XgGetPreferredMaxLength();
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);

        // 自動で再計算をするか？
        if (xg_bAutoRetry)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        // スマート解決か？
        if (xg_bSmartResolution)
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        // IMEをOFFにする。
        {
            HWND hwndCtrl;

            hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);

            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_WORD_LEN, XG_MIN_WORD_LEN));
        if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
            EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
            EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
        } else {
            EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
        }
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        INT n1, n2, n3;
        switch (id)
        {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < XG_MIN_SIZE || n1 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < XG_MIN_SIZE || n2 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return;
            }
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < XG_MIN_WORD_LEN || n3 > XG_MAX_WORD_LEN) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return;
            }
            xg_nMaxWordLen = n3;
            // 自動で再計算をするか？
            xg_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // スマート解決か？
            xg_bSmartResolution = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.ResetAndSetSize(n1, n2);
                xg_nRows = n1;
                xg_nCols = n2;
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
                // 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
                if (!(xg_nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENROWLINESYMV), nullptr, MB_ICONERROR);
                    break;
                }
                // 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
                if (!(xg_nCols & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENCOLLINESYMH), nullptr, MB_ICONERROR);
                    break;
                }
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case chx2:
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
                EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
                EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
            } else {
                EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
                EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
                EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
            }
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
        return DialogBoxDx(hwnd, IDD_CREATE);
    }
};
