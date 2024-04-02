// [表示]設定。
INT_PTR CALLBACK
XgViewSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static XGStringW s_strFit;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        xg_ahSettingsWnds[1] = hwnd;
        // チェックボックスを初期化。
        if (xg_bShowToolBar)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        if (s_bShowStatusBar)
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        if (::IsWindow(xg_hwndInputPalette))
            ::CheckDlgButton(hwnd, chx3, BST_CHECKED);
        if (::IsWindow(xg_hHintsWnd))
            ::CheckDlgButton(hwnd, chx4, BST_CHECKED);
        if (xg_bShowAnswer)
            ::CheckDlgButton(hwnd, chx5, BST_CHECKED);
        if (xg_bShowNumbering)
            ::CheckDlgButton(hwnd, chx6, BST_CHECKED);
        if (xg_bShowCaret)
            ::CheckDlgButton(hwnd, chx7, BST_CHECKED);
        if (xg_bShowDoubleFrame)
            ::CheckDlgButton(hwnd, chx8, BST_CHECKED);
        if (xg_bShowDoubleFrameLetters)
            ::CheckDlgButton(hwnd, chx9, BST_CHECKED);
        if (xg_nViewMode == XG_VIEW_SKELETON)
            ::CheckDlgButton(hwnd, chx10, BST_CHECKED);
        if (xg_bNumCroMode)
            ::CheckDlgButton(hwnd, chx11, BST_CHECKED);
        if (xg_bLowercase)
            ::CheckDlgButton(hwnd, chx12, BST_CHECKED);
        if (xg_bHiragana)
            ::CheckDlgButton(hwnd, chx13, BST_CHECKED);
        if (xg_bCheckingAnswer)
            ::CheckDlgButton(hwnd, chx14, BST_CHECKED);
        if (!xg_bSolved) {
            ::EnableWindow(::GetDlgItem(hwnd, chx4), FALSE);
            ::EnableWindow(::GetDlgItem(hwnd, chx5), FALSE);
            ::EnableWindow(::GetDlgItem(hwnd, chx14), FALSE);
        }
        // ズームを初期化。
        s_strFit = XgLoadStringDx1(IDS_FITWHOLE);
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)s_strFit.c_str());
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"10 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"30 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"50 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"65 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"80 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"90 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"100 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"200 %");
        ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)L"300 %");
        ::SetDlgItemTextW(hwnd, cmb1, (to_XGStringW(xg_nZoomRate) + L" %").c_str());
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case chx1:
        case chx2:
        case chx3:
        case chx4:
        case chx5:
        case chx6:
        case chx7:
        case chx8:
        case chx9:
        case chx10:
        case chx11:
        case chx12:
        case chx13:
        case chx14:
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
        }
        // 他のダイアログと同期する。
        switch (LOWORD(wParam))
        {
        case chx10: // スケルトンビュー。
            if (HIWORD(wParam) == BN_CLICKED)
            {
                BOOL bChecked = IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED;
                ::CheckDlgButton(xg_ahSettingsWnds[2], chx1, bChecked ? BST_CHECKED : BST_UNCHECKED);
            }
            break;
        case chx12: // 英小文字。
            if (HIWORD(wParam) == BN_CLICKED)
            {
                BOOL bChecked = IsDlgButtonChecked(hwnd, chx12) == BST_CHECKED;
                ::CheckDlgButton(xg_ahSettingsWnds[2], chx4, bChecked ? BST_CHECKED : BST_UNCHECKED);
            }
            break;
        case chx13: // ひらがな。
            if (HIWORD(wParam) == BN_CLICKED)
            {
                BOOL bChecked = IsDlgButtonChecked(hwnd, chx13) == BST_CHECKED;
                ::CheckDlgButton(xg_ahSettingsWnds[2], chx5, bChecked ? BST_CHECKED : BST_UNCHECKED);
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
                    // 元に戻す情報を取得。
                    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
                    sa1->Get();

                    // チェックボックスから設定を取得する。
                    xg_bShowToolBar = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
                    s_bShowStatusBar = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
                    BOOL bInputPalette = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
                    BOOL bHintsWnd = (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
                    xg_bShowAnswer = (::IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
                    xg_bShowNumbering = (::IsDlgButtonChecked(hwnd, chx6) == BST_CHECKED);
                    xg_bShowCaret = (::IsDlgButtonChecked(hwnd, chx7) == BST_CHECKED);
                    xg_bShowDoubleFrame = (::IsDlgButtonChecked(hwnd, chx8) == BST_CHECKED);
                    xg_bShowDoubleFrameLetters = (::IsDlgButtonChecked(hwnd, chx9) == BST_CHECKED);
                    xg_nViewMode = ((::IsDlgButtonChecked(hwnd, chx10) == BST_CHECKED) ? XG_VIEW_SKELETON : XG_VIEW_NORMAL);
                    xg_bNumCroMode = (::IsDlgButtonChecked(hwnd, chx11) == BST_CHECKED);
                    xg_bLowercase = (::IsDlgButtonChecked(hwnd, chx12) == BST_CHECKED);
                    xg_bHiragana = (::IsDlgButtonChecked(hwnd, chx13) == BST_CHECKED);
                    xg_bCheckingAnswer = (::IsDlgButtonChecked(hwnd, chx14) == BST_CHECKED);

                    // コンボボックスの設定を適用する。
                    WCHAR szText[128];
                    ::GetDlgItemTextW(hwnd, cmb1, szText, _countof(szText));
                    if (s_strFit == szText) {
                        ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_FITZOOM, 0);
                    } else {
                        WCHAR szText2[128];
                        ::LCMapStringW(::GetUserDefaultLCID(), LCMAP_HALFWIDTH, szText, _countof(szText),
                                       szText2, _countof(szText2));
                        StrTrimW(szText2, L" \t\r\n\x3000");
                        INT nRate = _wtoi(szText2);
                        if (nRate == 0)
                            nRate = 100;
                        if (nRate < 10)
                            nRate = 10;
                        if (nRate > 300)
                            nRate = 300;
                        XgSetZoomRate(xg_hMainWnd, nRate);
                    }

                    if (xg_bShowToolBar)
                        ::ShowWindow(xg_hToolBar, SW_SHOWNOACTIVATE);
                    else
                        ::ShowWindow(xg_hToolBar, SW_HIDE);

                    if (s_bShowStatusBar)
                        ShowWindow(xg_hStatusBar, SW_SHOWNOACTIVATE);
                    else
                        ShowWindow(xg_hStatusBar, SW_HIDE);

                    if (bInputPalette) {
                        if (!::IsWindow(xg_hwndInputPalette))
                            XgCreateInputPalette(xg_hMainWnd);
                    } else {
                        if (::IsWindow(xg_hwndInputPalette))
                            XgDestroyInputPalette();
                    }

                    if (bHintsWnd) {
                        if (!::IsWindow(xg_hHintsWnd)) {
                            XgShowHints(xg_hMainWnd);
                            xg_bShowClues = TRUE;
                        }
                    } else {
                        if (::IsWindow(xg_hHintsWnd)) {
                            ::DestroyWindow(xg_hHintsWnd);
                            xg_hHintsWnd = nullptr;
                        }
                        xg_bShowClues = FALSE;
                    }

                    if (xg_bNumCroMode) {
                        XgMakeItNumCro(hwnd);
                    } else {
                        xg_mapNumCro1.clear();
                        xg_mapNumCro2.clear();
                    }

                    // 元に戻す情報を確定。
                    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
                    sa2->Get();
                    xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);

                    // 表示を更新。
                    XgUpdateImage(xg_hMainWnd);
                    SendMessageW(xg_hMainWnd, WM_SIZE, 0, 0);
                }
                break;
            }
        }
        break;
    }
    return 0;
}
