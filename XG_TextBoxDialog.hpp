#pragma once

#include "XG_Dialog.hpp"

class XG_TextBoxDialog : public XG_Dialog
{
public:
    std::wstring m_strText;

    XG_TextBoxDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        SetWindowText(hEdt1, m_strText.c_str());
        return TRUE;
    }

    BOOL OnOK(HWND hwnd)
    {
        HWND hEdt1 = GetDlgItem(hwnd, edt1);

        WCHAR szText[MAX_PATH] = L"";
        GetWindowText(hEdt1, szText, _countof(szText));
        m_strText = szText;
        xg_str_trim_right(m_strText);
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
        return DialogBoxDx(hwnd, IDD_TEXTBOX);
    }
};
