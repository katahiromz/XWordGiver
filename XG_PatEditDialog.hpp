#pragma once

#include "XG_Window.hpp"

// 「パターン編集」ダイアログ。
class XG_PatEditDialog : public XG_Dialog
{
public:
    static inline BOOL s_bAdd = TRUE;
    static inline HBITMAP s_hbm = NULL;

    XG_PatEditDialog() noexcept
    {
    }

    // [パターン編集]ダイアログの初期化。
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        HWND hCmb1 = ::GetDlgItem(hwnd, cmb1);
        ComboBox_AddString(hCmb1, XgLoadStringDx1(IDS_ADD));
        ComboBox_AddString(hCmb1, XgLoadStringDx1(IDS_DELETE));

        if (s_bAdd)
            ComboBox_SetCurSel(hCmb1, 0);
        else
            ComboBox_SetCurSel(hCmb1, 1);

        OnCmb1(hwnd);

        return TRUE;
    }

    void OnCmb1(HWND hwnd)
    {
        HWND hCmb1 = ::GetDlgItem(hwnd, cmb1);
        INT iItem = ComboBox_GetCurSel(hCmb1);

        HBITMAP hbm = NULL;
        switch (iItem)
        {
        case 0:
            hbm = ::LoadBitmapW(xg_hInstance, MAKEINTRESOURCEW(IDB_ADDPAT));
            break;
        case 1:
            hbm = ::LoadBitmapW(xg_hInstance, MAKEINTRESOURCEW(IDB_DELETEPAT));
            break;
        default:
            break;
        }

        ::DeleteObject(s_hbm);
        s_hbm = hbm;
        SendDlgItemMessageW(m_hWnd, stc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)s_hbm);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
        case IDCANCEL:
            {
                HWND hCmb1 = ::GetDlgItem(hwnd, cmb1);
                INT iItem = ComboBox_GetCurSel(hCmb1);
                s_bAdd = (iItem == 0);
                ::DeleteObject(s_hbm);
                s_hbm = NULL;
                ::EndDialog(hwnd, id);
            }
            break;
        case cmb1:
            OnCmb1(hwnd);
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
        return DialogBoxDx(hwnd, IDD_ADDDELETEPAT);
    }
};
