#pragma once

#include "XG_Window.hpp"

// [ヘッダーと備考欄]ダイアログ。
class XG_NotesDialog : public XG_Dialog
{
public:
    XG_NotesDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // ヘッダーを設定する。
        if (xg_strHeader.empty()) {
            xg_strHeader = L"Title: (Untitled)\r\nAuthor: \r\nEditor: \r\nCopyright: \r\nDate: YYYY-MM-DD";
        }
        ::SetDlgItemTextW(hwnd, edt1, xg_strHeader.data());
        // 備考欄を設定する。
        std::wstring str = xg_strNotes;
        xg_str_trim(str);
        LPWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
        if (str.find(psz) == 0) {
            str = str.substr(::lstrlenW(psz));
        }
        ::SetDlgItemTextW(hwnd, edt2, str.data());
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        WCHAR sz[512];
        std::wstring str;

        switch (id)
        {
        case IDOK:
            // ヘッダーを取得する。
            ::GetDlgItemTextW(hwnd, edt1, sz, static_cast<int>(_countof(sz)));
            str = sz;
            xg_str_trim(str);
            xg_strHeader = str;

            // 備考欄を取得する。
            ::GetDlgItemTextW(hwnd, edt2, sz, static_cast<int>(_countof(sz)));
            str = sz;
            xg_str_trim(str);
            xg_strNotes = str;

            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        default:
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
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_NOTES);
    }
};
