#pragma once

// 「隠し機能」ダイアログ。
class XG_HiddenDialog
{
public:
    XG_HiddenDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1: // PAT.txtに追加。
            if (codeNotify == BN_CLICKED) {
                HCURSOR hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
                if (XgPatEdit(hwnd, TRUE)) {
                    // 成功メッセージ。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_WROTEPAT),
                                        XgLoadStringDx2(IDS_APPNAME), MB_ICONINFORMATION);
                } else {
                    // 失敗メッセージ。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTWRITEPAT),
                                        nullptr, MB_ICONERROR);
                }
                ::SetCursor(hOldCursor);
            }
            break;
        case psh2: // PAT.txtから削除。
            if (codeNotify == BN_CLICKED) {
                HCURSOR hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
                if (XgPatEdit(hwnd, FALSE)) {
                    // 成功メッセージ。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_WROTEPAT),
                                        XgLoadStringDx2(IDS_APPNAME), MB_ICONINFORMATION);
                } else {
                    // 失敗メッセージ。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTWRITEPAT),
                                        nullptr, MB_ICONERROR);
                }
                ::SetCursor(hOldCursor);
            }
            break;
        case psh3: // アプリのフォルダを開く。
            if (codeNotify == BN_CLICKED) {
                WCHAR szPath[MAX_PATH];
                GetModuleFileNameW(NULL, szPath, _countof(szPath));
                PathRemoveFileSpecW(szPath);
                ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
            }
            break;
        case psh4: // カギを使って辞書を更新する。
            if (codeNotify == BN_CLICKED) {
                WCHAR szText[MAX_PATH * 2];
                HCURSOR hOldCursor = ::SetCursor(::LoadCursor(NULL, IDC_WAIT));
                XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
                if (XgUpdateDictionaryUsingClues(hwnd, xg_dict_name)) {
                    // 成功メッセージ。
                    StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_UPDATEDICTOK),
                                     PathFindFileNameW(xg_dict_name.c_str()));
                    XgCenterMessageBoxW(hwnd, szText, XgLoadStringDx2(IDS_APPNAME), MB_ICONINFORMATION);
                } else {
                    // 失敗メッセージ。
                    StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_UPDATEDICTFAIL),
                                     PathFindFileNameW(xg_dict_name.c_str()));
                    XgCenterMessageBoxW(hwnd, szText, nullptr, MB_ICONERROR);
                }
                ::SetCursor(hOldCursor);
            }
            break;
        }
    }

    INT_PTR CALLBACK
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

    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static XG_HiddenDialog* s_pThis = NULL;
        if (uMsg == WM_INITDIALOG)
        {
            s_pThis = (XG_HiddenDialog*)lParam;
            SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)lParam);
        }
        else
        {
            s_pThis = (XG_HiddenDialog*)GetWindowLongPtrW(hwnd, DWLP_USER);
        }

        if (!s_pThis)
            return 0;

        return s_pThis->DialogProcDx(hwnd, uMsg, wParam, lParam);
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxParamW(xg_hInstance, MAKEINTRESOURCEW(IDD_HIDDEN), hwnd, DialogProc, (LPARAM)this);
    }
};
