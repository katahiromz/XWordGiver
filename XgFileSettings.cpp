// [ファイル]設定。
INT_PTR CALLBACK
XgFileSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hCmb2, hCmb3;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        xg_ahSyncedDialogs[I_SYNCED_FILE_SETTINGS] = hwnd;
        // チェックボックスを初期化。
        if (xg_bShowAnswerOnOpen)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (xg_bAutoSave)
            CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (xg_bShowAnswerOnGenerate)
            CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if (xg_bNoReadLooks)
            CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if (xg_bNoWriteLooks)
            CheckDlgButton(hwnd, chx5, BST_CHECKED);
        // 保存先を初期化。
        for (const auto& dir : xg_dirs_save_to) {
            SendDlgItemMessage(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)dir.c_str());
        }
        SendDlgItemMessage(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // 連番ファイル名1。
        hCmb2 = GetDlgItem(hwnd, cmb2);
        ComboBox_AddString(hCmb2, L"Crossword-%6N.xd");
        ComboBox_AddString(hCmb2, L"Crossword-%Wx%H-%4N.xd");
        ComboBox_AddString(hCmb2, L"Cross-%Y%M%D-%h%m%s-%N.xd");
        ComboBox_AddString(hCmb2, L"Cross-%Y%M%D-%h%m%s-%Wx%H-%N.xd");
        ComboBox_AddString(hCmb2, L"Cross-%Y_%M_%D-%h_%m_%s-%N.xd");
        ComboBox_AddString(hCmb2, L"Cross-%Y_%M_%D-%h_%m_%s-%Wx%H-%N.xd");
        ComboBox_SetText(hCmb2, xg_szNumberingFileName1);
        // 連番ファイル名2。
        hCmb3 = GetDlgItem(hwnd, cmb3);
        ComboBox_AddString(hCmb3, L"Pat-%6N.xd");
        ComboBox_AddString(hCmb3, L"Pat-%Wx%H-%4N.xd");
        ComboBox_AddString(hCmb3, L"Pat-%Y%M%D-%h%m%s-%N.xd");
        ComboBox_AddString(hCmb3, L"Pat-%Y%M%D-%h%m%s-%Wx%H-%N.xd");
        ComboBox_AddString(hCmb3, L"Pat-%Y_%M_%D-%h_%m_%s-%N.xd");
        ComboBox_AddString(hCmb3, L"Pat-%Y_%M_%D-%h_%m_%s-%Wx%H-%N.xd");
        ComboBox_SetText(hCmb3, xg_szNumberingFileName2);
        // ドラッグ＆ドロップを受け付ける。
        DragAcceptFiles(hwnd, TRUE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case chx1:
        case chx2:
        case chx3:
        case chx4:
        case chx5:
            if (HIWORD(wParam) == BN_CLICKED) {
                // 変更された。更新ボタンを有効にする。
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;
        case cmb1:
        case cmb2:
        case cmb3:
            if (HIWORD(wParam) == CBN_EDITCHANGE ||
                HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_SELENDOK)
            {
                // 変更された。更新ボタンを有効にする。
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;
        case psh1: // 「参照」ボタン。
            {
                // ユーザーにフォルダ位置を問い合わせる。
                BROWSEINFOW bi = { hwnd };
                bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
                bi.ulFlags = BIF_RETURNONLYFSDIRS;
                bi.lpfn = XgBrowseCallbackProc; // 初期位置設定用のフック。
                ::GetDlgItemTextW(hwnd, cmb1, xg_szDir, _countof(xg_szDir));
                LPITEMIDLIST pidl = ::SHBrowseForFolderW(&bi);
                if (pidl) {
                    // テキストを設定。
                    WCHAR szFile[MAX_PATH];
                    ::SHGetPathFromIDListW(pidl, szFile);
                    ::SetDlgItemTextW(hwnd, cmb1, szFile);
                    // 忘れずPIDLを解放。
                    ::CoTaskMemFree(pidl);
                    // 変更された。更新ボタンを有効にする。
                    PropSheet_Changed(GetParent(hwnd), hwnd);
                }
            }
            break;
        case psh2: // 「フォルダを開く」ボタン。
            {
                // フォルダパス名を取得。
                WCHAR szDir[MAX_PATH];
                ::GetDlgItemTextW(hwnd, cmb1, szDir, _countof(szDir));
                // なければ作成。
                XgMakePathW(szDir);
                // シェルで開く。
                ::ShellExecuteW(hwnd, NULL, szDir, NULL, NULL, SW_SHOWNORMAL);
            }
            break;
        }
        break;

    case WM_DROPFILES: // ファイルがドロップされた。
        {
            HDROP hDrop = (HDROP)wParam;
            DragQueryFileW(hDrop, 0, xg_szDir, _countof(xg_szDir));
            if (PathIsDirectoryW(xg_szDir)) // フォルダか？
            {
                // テキストを設定。
                ::SetDlgItemTextW(hwnd, cmb1, xg_szDir);
                // 変更された。更新ボタンを有効にする。
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
        }
        break;

    case WM_NOTIFY:
        {
            NMHDR *pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code) {
            case PSN_APPLY: // 適用
                {
                    // チェックボックスから設定を取得する。
                    xg_bShowAnswerOnOpen = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
                    xg_bAutoSave = (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
                    xg_bShowAnswerOnGenerate = (IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
                    xg_bNoReadLooks = (IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
                    xg_bNoWriteLooks = (IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);

                    // テキストを取得する。
                    WCHAR szFile[MAX_PATH];
                    ::GetDlgItemTextW(hwnd, cmb1, szFile, _countof(szFile));
                    // 前後の空白を取り除く。
                    StrTrimW(szFile, XG_WHITE_SPACES);

                    // 一致する項目を削除する。
                    INT i = 0;
                    for (const auto& dir : xg_dirs_save_to) {
                        if (lstrcmpiW(dir.c_str(), szFile) == 0) {
                            xg_dirs_save_to.erase(xg_dirs_save_to.begin() + i);
                            break;
                        }
                        ++i;
                    }

                    // 先頭に挿入。
                    xg_dirs_save_to.insert(xg_dirs_save_to.begin(), szFile);

                    // 連番ファイル名のテキストを取得する。
                    ::GetDlgItemTextW(hwnd, cmb2, xg_szNumberingFileName1, _countof(xg_szNumberingFileName1));
                    ::GetDlgItemTextW(hwnd, cmb3, xg_szNumberingFileName2, _countof(xg_szNumberingFileName2));

                    // 前後の空白を取り除く。
                    StrTrimW(xg_szNumberingFileName1, XG_WHITE_SPACES);
                    StrTrimW(xg_szNumberingFileName2, XG_WHITE_SPACES);

                    // 空文字列・無効なファイル名ならデフォルトに戻す。
                    if (!xg_szNumberingFileName1[0] ||
                        lstrcmpiW(xg_szNumberingFileName1, L".") == 0 ||
                        lstrcmpiW(xg_szNumberingFileName1, L"..") == 0)
                    {
                        StringCchCopyW(xg_szNumberingFileName1, _countof(xg_szNumberingFileName1),
                                       L"Crossword-%Wx%H-%4N.xd");
                    }
                    if (!xg_szNumberingFileName2[0] ||
                        lstrcmpiW(xg_szNumberingFileName2, L".") == 0 ||
                        lstrcmpiW(xg_szNumberingFileName2, L"..") == 0)
                    {
                        StringCchCopyW(xg_szNumberingFileName2, _countof(xg_szNumberingFileName2),
                                       L"Pat-%Wx%H-%4N.xd");
                    }

                    // 必要なら拡張子を付ける。
                    LPWSTR pch;
                    pch = PathFindExtensionW(xg_szNumberingFileName1);
                    if (!pch || !*pch)
                        PathAddExtensionW(xg_szNumberingFileName1, L".xd");
                    pch = PathFindExtensionW(xg_szNumberingFileName2);
                    if (!pch || !*pch)
                        PathAddExtensionW(xg_szNumberingFileName2, L".xd");

                    // 不正な文字を置き換える。
                    for (LPWSTR pch = xg_szNumberingFileName1; *pch; ++pch)
                    {
                        switch (*pch)
                        {
                        case L'/': case L'\\': case L':': case L'*': case L'?':
                        case L'<': case L'>': case L'|':
                            *pch = L'_';
                            break;
                        }
                    }
                    for (LPWSTR pch = xg_szNumberingFileName2; *pch; ++pch)
                    {
                        switch (*pch)
                        {
                        case L'/': case L'\\': case L':': case L'*': case L'?':
                        case L'<': case L'>': case L'|':
                            *pch = L'_';
                            break;
                        }
                    }
                }
                break;
            }
        }
        break;
    }
    return 0;
}
