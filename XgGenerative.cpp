// [生成]設定。
INT_PTR CALLBACK
XgGenerativeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static XGStringW s_strFit;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        xg_ahSyncedDialogs[I_SYNCED_GENERATIVE] = hwnd;
        // チェックボックスを初期化。
        if (xg_bNoGeneratedMsg)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (xg_bNoCanceledMsg)
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (xg_bAutoRetry)
            ::CheckDlgButton(hwnd, chx3, BST_CHECKED);
        return TRUE;

    case WM_COMMAND:
        // 変更があれば「更新」ボタンを有効にする。
        switch (LOWORD(wParam))
        {
        case chx1:
        case chx2:
        case chx3:
            if (HIWORD(wParam) == BN_CLICKED)
                PropSheet_Changed(GetParent(hwnd), hwnd);
            break;
        }
        break;

    case WM_NOTIFY:
        {
            NMHDR *pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code) {
            case PSN_APPLY: // 適用
                // チェックボックスから設定を取得する。
                xg_bNoGeneratedMsg = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
                xg_bNoCanceledMsg = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
                xg_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
                break;
            }
        }
        break;
    }
    return 0;
}
