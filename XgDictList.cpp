// ラジオボタン風の状態を表示するためのイメージリストを作成する。
HIMAGELIST XgDictList_CreateRadioButtonImageList(HWND hwnd)
{
    HIMAGELIST himl;
    HDC hdc_wnd, hdc;
    HBITMAP hbm_im, hbm_mask;
    HGDIOBJ hbm_orig;
    HBRUSH hbr_white = GetStockBrush(WHITE_BRUSH), hbr_black = GetStockBrush(BLACK_BRUSH);
    RECT rc;

    // 小さいイメージリストを作成する。
    himl = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                            ILC_COLOR | ILC_MASK, 2, 2);
    // いろいろ準備。
    hdc_wnd = GetDC(hwnd);
    hdc = CreateCompatibleDC(hdc_wnd);
    hbm_im = CreateCompatibleBitmap(hdc_wnd, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    hbm_mask = CreateBitmap(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 1, 1, NULL);
    ReleaseDC(hwnd, hdc_wnd);

    // マスクビットマップを描画。
    SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    hbm_orig = SelectObject(hdc, hbm_mask);
    FillRect(hdc, &rc, hbr_white);
    InflateRect(&rc, -1, -1);
    SelectObject(hdc, hbr_black);
    Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);

    // チェックしていないアイコンを追加。
    SelectObject(hdc, hbm_im);
    SelectObject(hdc, GetSysColorBrush(COLOR_WINDOW));
    Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, hbm_orig);
    ImageList_Add(himl, hbm_im, hbm_mask); 

    // チェック済みのアイコンを追加。
    SelectObject(hdc, hbm_im);
    SelectObject(hdc, GetSysColorBrush(COLOR_WINDOWTEXT));
    Ellipse(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, hbm_orig);
    ImageList_Add(himl, hbm_im, hbm_mask);

    // 後片付け。
    DeleteObject(hbm_mask);
    DeleteObject(hbm_im);
    DeleteDC(hdc);

    return himl;
}

// リストを再読み込みする。
void XgDictList_ReloadList(HWND hwnd)
{
    HWND hwndLst1 = GetDlgItem(hwnd, lst1);

    // すべての項目を消去する。
    ListView_DeleteAllItems(hwndLst1);
    SendDlgItemMessageW(hwnd, cmb1, CB_RESETCONTENT, 0, 0);

    // 辞書リストを読み込む。
    XgLoadDictsAll();

    // リストビューを埋める。
    INT iItem = 0;
    for (auto& entry : xg_dicts) {
        LV_ITEMW item = { LVIF_TEXT };
        item.pszText = const_cast<LPWSTR>(PathFindFileNameW(entry.m_filename.c_str()));
        item.iItem = iItem;
        item.iSubItem = 0;
        ListView_InsertItem(hwndLst1, &item);
        item.iItem = iItem;
        item.iSubItem = 1;
        item.pszText = const_cast<LPWSTR>(entry.m_friendly_name.c_str());
        ListView_SetItem(hwndLst1, &item);
        ++iItem;
    }

    // コンボボックスを埋める。
    for (auto& item : xg_dicts) {
        LPCWSTR pszText = PathFindFileNameW(item.m_filename.c_str());
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)pszText);
    }

    // コンボボックスの項目を選択する。
    {
        LPCWSTR pszText = PathFindFileNameW(xg_dict_name.c_str());
        INT iItem = ::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, -1, (LPARAM)pszText);
        if (iItem != CB_ERR) {
            ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, iItem, 0);
        }
    }

    // リストビューの項目を選択する。
    LV_FINDINFO Find = { LVFI_STRING, PathFindFileNameW(xg_dict_name.c_str()) };
    iItem = ListView_FindItem(hwndLst1, -1, &Find);
    ListView_SetItemState(hwndLst1, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
    ListView_EnsureVisible(hwndLst1, iItem, FALSE);
}

// リストビューのチェック位置を取得する。
INT XgDictList_GetCurSel(HWND hwnd, HWND hwndLst1)
{
    INT cItems = ListView_GetItemCount(hwndLst1);
    for (INT iItem = 0; iItem < cItems; ++iItem) {
        UINT uState = ListView_GetItemState(hwndLst1, iItem, LVIS_STATEIMAGEMASK);
        if (uState & INDEXTOSTATEIMAGEMASK(2))
            return iItem;
    }
    return -1;
}

// リストビューのチェック情報を更新する。
void XgDictList_SetCurSel(HWND hwnd, HWND hwndLst1, INT iSelect)
{
    INT cItems = ListView_GetItemCount(hwndLst1);
    for (INT iItem = 0; iItem < cItems; ++iItem) {
        if (iItem == iSelect) {
            ListView_SetItemState(hwndLst1, iItem, INDEXTOSTATEIMAGEMASK(2), LVIS_STATEIMAGEMASK);
        } else {
            ListView_SetItemState(hwndLst1, iItem, INDEXTOSTATEIMAGEMASK(1), LVIS_STATEIMAGEMASK);
        }
    }

    ListView_SetItemState(hwndLst1, iSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hwndLst1, iSelect, FALSE);
}

// [辞書]設定。
INT_PTR CALLBACK
XgDictListDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static INT m_bUpdating = 0;
    HWND hwndLst1 = GetDlgItem(hwnd, lst1);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            xg_ahSyncedDialogs[I_SYNCED_DICTLIST] = hwnd;

            // 拡張リストビュースタイルを設定。
            ListView_SetExtendedListViewStyle(hwndLst1,
                LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES);

            // 辞書リストのヘッダーを初期化。
            LV_COLUMNW column = { LVCF_TEXT | LVCF_WIDTH };
            column.pszText = XgLoadStringDx1(IDS_FILENAME);
            column.cx = 200;
            ListView_InsertColumn(hwndLst1, 0, &column);
            column.pszText = XgLoadStringDx1(IDS_DISPLAYNAME);
            column.cx = 250;
            ListView_InsertColumn(hwndLst1, 1, &column);

            // イメージリストを設定。
            HIMAGELIST himl = XgDictList_CreateRadioButtonImageList(hwnd);
            ListView_SetImageList(hwndLst1, himl, LVSIL_STATE);

            // リストを再読み込みする。
            ++m_bUpdating;
            XgDictList_ReloadList(hwnd);
            --m_bUpdating;
        }
        return TRUE;

    case WM_COMMAND:
        // 変更があれば「更新」ボタンを有効にする。
        switch (LOWORD(wParam))
        {
        case psh1:
            if (HIWORD(wParam) == BN_CLICKED)
                PropSheet_Changed(GetParent(hwnd), hwnd);
            break;
        case cmb1:
            if (HIWORD(wParam) == CBN_EDITCHANGE ||
                HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_SELENDOK)
            {
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;
        }
        // 実際の処理。
        switch (LOWORD(wParam))
        {
        case psh1: // ファイルを開く。
            if (HIWORD(wParam) == BN_CLICKED) {
                INT iItem = XgDictList_GetCurSel(hwnd, hwndLst1);

                // コンボボックスからテキストを取得。
                WCHAR szText[MAX_PATH];
                szText[0] = 0;
                SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
                szText[_countof(szText) - 1] = 0;

                // パスファイル名を構築する。
                WCHAR szPath[MAX_PATH];
                GetModuleFileNameW(NULL, szPath, _countof(szPath));
                PathRemoveFileSpecW(szPath);
                PathAppendW(szPath, L"DICT");
                PathAppendW(szPath, szText);

                // シェルで開く。開けなければメモ帳で開く。
                SHELLEXECUTEINFOW info = { sizeof(info), SEE_MASK_FLAG_NO_UI, hwnd };
                info.lpFile = szPath;
                if (!ShellExecuteExW(&info)) {
                    WCHAR szQuotedPath[MAX_PATH];
                    StringCchPrintfW(szQuotedPath, _countof(szQuotedPath), L"\"%s\"", szPath);
                    info.lpFile = L"notepad.exe";
                    info.lpParameters = szQuotedPath;
                    info.nShow = SW_SHOWNORMAL;
                    ShellExecuteExW(&info);
                }
            }
            break;
        case psh2: // フォルダを開く。
            if (HIWORD(wParam) == BN_CLICKED) {
                WCHAR szPath[MAX_PATH];
                GetModuleFileNameW(NULL, szPath, _countof(szPath));
                PathRemoveFileSpecW(szPath);
                PathAppendW(szPath, L"DICT");
                ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
            }
            break;
        case psh4: // 再読み込み。
            XgDictList_ReloadList(hwnd);
            break;
        case psh5: // テーマ。
            // 変更点を適用する。
            PropSheet_Apply(GetParent(hwnd));
            // テーマを開く。
            XgTheme(hwnd);
            break;
        case cmb1:
            if (HIWORD(wParam) == CBN_EDITCHANGE ||
                HIWORD(wParam) == CBN_SELCHANGE ||
                HIWORD(wParam) == CBN_SELENDOK)
            {
                if (m_bUpdating)
                    break;
                // コンボボックスで現在選択されているテキストを取得。
                INT iItem = (INT)::SendDlgItemMessageW(hwnd, cmb1, CB_GETCURSEL, 0, 0);
                if (iItem == CB_ERR)
                    return 0;

                // コンボボックスからテキストを取得。
                WCHAR szText[MAX_PATH];
                szText[0] = 0;
                SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
                szText[_countof(szText) - 1] = 0;

                // リストビューからテキストを探す。
                LV_FINDINFO Find = { LVFI_STRING, szText };
                iItem = ListView_FindItem(hwndLst1, -1, &Find);

                // リストビューの選択を更新する。
                m_bUpdating++;
                XgDictList_SetCurSel(hwnd, hwndLst1, iItem);
                m_bUpdating--;
            }
            break;
        }
        break;

    case WM_NOTIFY:
        {
            NMHDR *pnmhdr = (NMHDR *)lParam;
            auto pListView = (NM_LISTVIEW *)lParam;
            switch (pnmhdr->code) {
            case PSN_APPLY: // 適用
                {
                    INT iItem = XgDictList_GetCurSel(hwnd, hwndLst1);

                    // コンボボックスからテキストを取得。
                    WCHAR szText[MAX_PATH];
                    szText[0] = 0;
                    SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, iItem, (LPARAM)szText);
                    szText[_countof(szText) - 1] = 0;

                    // パスファイル名を構築する。
                    WCHAR szPath[MAX_PATH];
                    GetModuleFileNameW(NULL, szPath, _countof(szPath));
                    PathRemoveFileSpecW(szPath);
                    PathAppendW(szPath, L"DICT");
                    PathAppendW(szPath, szText);

                    // 辞書を指定する。
                    xg_dict_name = szPath;

                    // ステータスバーを更新。
                    XgUpdateStatusBar(xg_hMainWnd);

                    // 再読み込み。
                    XgDictList_ReloadList(hwnd);
                }
                break;

            case LVN_ITEMCHANGED:
                if (pnmhdr->idFrom == lst1 && !m_bUpdating && (pListView->uNewState & LVIS_SELECTED)) {
                    // リストビューの選択が変わった。
                    HWND hwndLst1 = GetDlgItem(hwnd, lst1);

                    // テキストを取得。
                    WCHAR szText[MAX_PATH];
                    LV_ITEMW item = { LVIF_TEXT };
                    item.pszText = szText;
                    item.cchTextMax = _countof(szText);
                    item.iItem = pListView->iItem;
                    item.iSubItem = 0;
                    ListView_GetItem(hwndLst1, &item);

                    // リストビューのチェック状態を更新する。
                    m_bUpdating++;
                    XgDictList_SetCurSel(hwnd, hwndLst1, pListView->iItem);
                    m_bUpdating--;

                    // コンボボックスからテキストを探す。
                    INT iItem = (INT)::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, -1, (LPARAM)szText);
                    if (iItem != CB_ERR) {
                        // コンボボックスの選択を変更する。
                        m_bUpdating++;
                        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, iItem, 0);
                        m_bUpdating--;

                        // 更新ボタンを有効にする。
                        PropSheet_Changed(GetParent(hwnd), hwnd);
                    }
                }
                break;
            }
        }
        break;
    }
    return 0;
}
