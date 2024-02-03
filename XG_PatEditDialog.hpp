#pragma once

#include "XG_Window.hpp"

void XgShowPatInfo(HWND hwndInfo);

// 「パターン編集」ダイアログ。
class XG_PatEditDialog : public XG_Dialog
{
public:
    enum TYPE {
        TYPE_ADD,
        TYPE_DELETE,
        TYPE_SHOWINFO
    };
    static inline TYPE s_nType = TYPE_ADD;
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
        ComboBox_AddString(hCmb1, XgLoadStringDx1(IDS_SHOWINFO));

        switch (s_nType) {
        case TYPE_ADD:
            ComboBox_SetCurSel(hCmb1, 0);
            break;
        case TYPE_DELETE:
            ComboBox_SetCurSel(hCmb1, 1);
            break;
        case TYPE_SHOWINFO:
            ComboBox_SetCurSel(hCmb1, 2);
            break;
        }

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
            ::ShowWindow(::GetDlgItem(m_hWnd, edt1), SW_HIDE);
            ::ShowWindow(::GetDlgItem(m_hWnd, stc1), SW_SHOWNOACTIVATE);
            hbm = ::LoadBitmapW(xg_hInstance, MAKEINTRESOURCEW(IDB_ADDPAT));
            ::DeleteObject(s_hbm);
            s_hbm = hbm;
            SendDlgItemMessageW(m_hWnd, stc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)s_hbm);
            break;
        case 1:
            ::ShowWindow(::GetDlgItem(m_hWnd, edt1), SW_HIDE);
            ::ShowWindow(::GetDlgItem(m_hWnd, stc1), SW_SHOWNOACTIVATE);
            hbm = ::LoadBitmapW(xg_hInstance, MAKEINTRESOURCEW(IDB_DELETEPAT));
            ::DeleteObject(s_hbm);
            s_hbm = hbm;
            SendDlgItemMessageW(m_hWnd, stc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)s_hbm);
            break;
        case 2:
            ::ShowWindow(::GetDlgItem(m_hWnd, edt1), SW_SHOWNOACTIVATE);
            ::ShowWindow(::GetDlgItem(m_hWnd, stc1), SW_HIDE);
            XgShowPatInfo(::GetDlgItem(m_hWnd, edt1));
            break;
        default:
            break;
        }

    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
        case IDCANCEL:
            {
                HWND hCmb1 = ::GetDlgItem(hwnd, cmb1);
                s_nType = (TYPE)ComboBox_GetCurSel(hCmb1);
                ::DeleteObject(s_hbm);
                s_hbm = NULL;
                ::EndDialog(hwnd, id);
            }
            break;
        case cmb1:
            if (codeNotify == CBN_SELCHANGE) {
                OnCmb1(hwnd);
            }
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
