// 音声コンボボックスを初期化。
BOOL XgGenerative_InitSound(HWND hwnd, INT nComboID, INT nCheckID, LPCWSTR pszPathName)
{
    HWND hCmb = GetDlgItem(hwnd, nComboID);

    if (pszPathName[0])
        ::CheckDlgButton(hwnd, nCheckID, BST_CHECKED);
    else
        ::EnableWindow(hCmb, FALSE);

    // 「(なし)」を追加して選択。
    ComboBox_AddString(hCmb, XgLoadStringDx1(IDS_NONE));
    ComboBox_SetCurSel(hCmb, 0);

    // SOUNDファイルがなければ音声は無効。
    WCHAR szPath[MAX_PATH];
    if (!XgFindLocalFile(szPath, _countof(szPath), L"SOUND"))
        return FALSE;

    // WAVファイルの列挙を開始する。
    WIN32_FIND_DATAW find;
    PathAppendW(szPath, L"*.wav");
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        // コンボボックスに追加。
        INT iItem = ComboBox_AddString(hCmb, find.cFileName);

        // 一致すれば選択。
        if (lstrcmpiW(find.cFileName, PathFindFileNameW(pszPathName)) == 0)
            ComboBox_SetCurSel(hCmb, iItem);
    } while (FindNextFileW(hFind, &find));

    FindClose(hFind);
    return TRUE;
}

// 音を鳴らす。
void XgGenerative_PlaySound(HWND hwnd, INT nID)
{
    // SOUNDフォルダがなければ鳴らさない。
    WCHAR szPath[MAX_PATH];
    if (!XgFindLocalFile(szPath, _countof(szPath), L"SOUND"))
        return;

    // コンボボックスからテキストを取得。
    WCHAR szText[MAX_PATH];
    ComboBox_GetText(GetDlgItem(hwnd, nID), szText, _countof(szText));

    // 「(なし)」は鳴らさない。
    if (lstrcmpiW(szText, XgLoadStringDx1(IDS_NONE)) == 0)
        return;

    // パス名を構築して、音声を鳴らす。
    PathAppendW(szPath, szText);
    ::PlaySoundW(szPath, NULL, SND_ASYNC | SND_FILENAME);
}

// [生成]設定。
INT_PTR CALLBACK
XgGenerativeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        xg_ahSyncedDialogs[I_SYNCED_GENERATIVE] = hwnd;
        // チェックボックスとコンボボックスを初期化。
        if (xg_bNoGeneratedMsg)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (xg_bNoCanceledMsg)
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (xg_bAutoRetry)
            ::CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if (!XgGenerative_InitSound(hwnd, cmb1, chx4, xg_aszSoundFiles[0]))
            EnableWindow(GetDlgItem(hwnd, chx4), FALSE);
        if (!XgGenerative_InitSound(hwnd, cmb2, chx6, xg_aszSoundFiles[1]))
            EnableWindow(GetDlgItem(hwnd, chx5), FALSE);
        if (!XgGenerative_InitSound(hwnd, cmb3, chx5, xg_aszSoundFiles[2]))
            EnableWindow(GetDlgItem(hwnd, chx6), FALSE);
        return TRUE;

    case WM_COMMAND:
        // 変更があれば「更新」ボタンを有効にする。
        switch (LOWORD(wParam))
        {
        case chx1:
        case chx2:
        case chx3:
        case chx4:
        case chx5:
        case chx6:
            if (HIWORD(wParam) == BN_CLICKED)
                PropSheet_Changed(GetParent(hwnd), hwnd);
            break;
        case cmb1:
        case cmb2:
        case cmb3:
            if (HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_SELENDOK)
            {
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;
        }
        // チェックボックスの状態によりコンボボックスの有効化・無効化を切り替える。
        // 「再生」ボタンで音を出す。
        switch (LOWORD(wParam))
        {
        case chx4:
            if (HIWORD(wParam) == BN_CLICKED)
                EnableWindow(GetDlgItem(hwnd, cmb1), IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
            break;
        case chx5:
            if (HIWORD(wParam) == BN_CLICKED)
                EnableWindow(GetDlgItem(hwnd, cmb2), IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
            break;
        case chx6:
            if (HIWORD(wParam) == BN_CLICKED)
                EnableWindow(GetDlgItem(hwnd, cmb3), IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
            break;
        case psh1:
            if (HIWORD(wParam) == BN_CLICKED) {
                XgGenerative_PlaySound(hwnd, cmb1);
            }
            break;
        case psh2:
            if (HIWORD(wParam) == BN_CLICKED) {
                XgGenerative_PlaySound(hwnd, cmb2);
            }
            break;
        case psh3:
            if (HIWORD(wParam) == BN_CLICKED) {
                XgGenerative_PlaySound(hwnd, cmb3);
            }
            break;
        }
        break;

    case WM_NOTIFY:
        {
            NMHDR *pnmhdr = (NMHDR *)lParam;
            switch (pnmhdr->code) {
            case PSN_APPLY: // 適用
                {
                    // 設定を取得する。
                    xg_bNoGeneratedMsg = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
                    xg_bNoCanceledMsg = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
                    xg_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);

                    // いったん音声ファイルの情報をクリアする。
                    xg_aszSoundFiles[0][0] = 0;
                    xg_aszSoundFiles[1][0] = 0;
                    xg_aszSoundFiles[2][0] = 0;

                    // SOUNDフォルダがなければ無効のまま。
                    WCHAR szPath[MAX_PATH];
                    if (!XgFindLocalFile(szPath, _countof(szPath), L"SOUND"))
                        break;

                    WCHAR szText[MAX_PATH];

                    // コンボボックスとチェックボックスの状態に応じて音声ファイルを設定する。
                    ComboBox_GetText(GetDlgItem(hwnd, cmb1), szText, _countof(szText));
                    if (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED && szText[0]) {
                        StringCchCopyW(xg_aszSoundFiles[0], _countof(xg_aszSoundFiles[0]), szPath);
                        PathAppendW(xg_aszSoundFiles[0], szText);
                    }
                    ComboBox_GetText(GetDlgItem(hwnd, cmb2), szText, _countof(szText));
                    if (::IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED && szText[0]) {
                        StringCchCopyW(xg_aszSoundFiles[1], _countof(xg_aszSoundFiles[0]), szPath);
                        PathAppendW(xg_aszSoundFiles[1], szText);
                    }
                    ComboBox_GetText(GetDlgItem(hwnd, cmb3), szText, _countof(szText));
                    if (::IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED && szText[0]) {
                        StringCchCopyW(xg_aszSoundFiles[2], _countof(xg_aszSoundFiles[0]), szPath);
                        PathAppendW(xg_aszSoundFiles[2], szText);
                    }
                }
                break;
            }
        }
        break;
    }
    return 0;
}
