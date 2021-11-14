#pragma once

#include "XG_Dialog.hpp"

// 盤を特定の文字で埋め尽くす。
void XgNewCells(HWND hwnd, WCHAR ch, INT nRows, INT nCols);
// 盤のサイズを変更する。
void XgResizeCells(HWND hwnd, INT nNewRows, INT nNewCols);

// [新規作成]ダイアログのダイアログ プロシージャ。
class XG_NewDialog : public XG_Dialog
{
public:
    XG_NewDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, xg_nCols, FALSE);
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
        CheckRadioButton(hwnd, rad1, rad3, rad1);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        INT n1, n2;
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
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            // 処理を行う。
            if (IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED) {
                // 盤を特定の文字で埋め尽くす。
                XgNewCells(xg_hMainWnd, ZEN_SPACE, n1, n2);
            } else if (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED) {
                // 盤を特定の文字で埋め尽くす。
                XgNewCells(xg_hMainWnd, ZEN_BLACK, n1, n2);
            } else if (IsDlgButtonChecked(hwnd, rad3) == BST_CHECKED) {
                // 盤のサイズを変更する。
                XgResizeCells(xg_hMainWnd, n1, n2);
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
        return DialogBoxDx(hwnd, IDD_NEW);
    }
};
