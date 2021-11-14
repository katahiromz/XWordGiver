#pragma once

#include "XG_Dialog.hpp"

// UIフォント。
extern WCHAR xg_szUIFont[LF_FACESIZE];

// ツールバーを表示するか？
extern bool xg_bShowToolBar;

// マスのフォント。
extern WCHAR xg_szCellFont[LF_FACESIZE];

// 小さな文字のフォント。
extern WCHAR xg_szSmallFont[LF_FACESIZE];

// UIフォント。
extern WCHAR xg_szUIFont[LF_FACESIZE];

// 「単語リストから生成」ダイアログ。
class XG_SettingsDialog : public XG_Dialog
{
public:
    inline static COLORREF s_rgbColorTable[] = {
        RGB(0, 0, 0),
        RGB(0x33, 0x33, 0x33),
        RGB(0x66, 0x66, 0x66),
        RGB(0x99, 0x99, 0x99),
        RGB(0xCC, 0xCC, 0xCC),
        RGB(0xFF, 0xFF, 0xFF),
        RGB(0xFF, 0xFF, 0xCC),
        RGB(0xFF, 0xCC, 0xFF),
        RGB(0xFF, 0xCC, 0xCC),
        RGB(0xCC, 0xFF, 0xFF),
        RGB(0xCC, 0xFF, 0xCC),
        RGB(0xCC, 0xCC, 0xFF),
        RGB(0xCC, 0xCC, 0xCC),
        RGB(0, 0, 0xCC),
        RGB(0, 0xCC, 0),
        RGB(0xCC, 0, 0),
    };
    // 一時的に保存する色のデータ。
    inline static COLORREF s_rgbColors[3];

    XG_SettingsDialog()
    {
    }

    // [設定]ダイアログの初期化。
    BOOL OnInitDialog(HWND hwnd)
    {
        // 色を一時的なデータとしてセットする。
        s_rgbColors[0] = xg_rgbWhiteCellColor;
        s_rgbColors[1] = xg_rgbBlackCellColor;
        s_rgbColors[2] = xg_rgbMarkedCellColor;

        // 画面の中央に寄せる。
        XgCenterDialog(hwnd);

        // フォント名を格納する。
        ::SetDlgItemTextW(hwnd, edt1, xg_szCellFont);
        ::SetDlgItemTextW(hwnd, edt2, xg_szSmallFont);
        ::SetDlgItemTextW(hwnd, edt3, xg_szUIFont);

        // ツールバーを表示するか？
        ::CheckDlgButton(hwnd, chx1,
            (xg_bShowToolBar ? BST_CHECKED : BST_UNCHECKED));
        // 太枠をつけるか？
        ::CheckDlgButton(hwnd, chx2,
            (xg_bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));
        // 二重マスに枠をつけるか？
        ::CheckDlgButton(hwnd, chx3,
            (xg_bDrawFrameForMarkedCell ? BST_CHECKED : BST_UNCHECKED));

        // 文字の大きさ。
        ::SetDlgItemInt(hwnd, edt4, xg_nCellCharPercents, FALSE);
        ::SetDlgItemInt(hwnd, edt5, xg_nSmallCharPercents, FALSE);
        // 大きさの範囲を指定。
        ::SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(100, 3));
        ::SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(100, 3));

        WCHAR szPath[MAX_PATH];
        std::vector<std::wstring> items;
        WIN32_FIND_DATA find;
        HANDLE hFind;

        // BLOCK\*.bmp
        GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath));
        PathRemoveFileSpec(szPath);
        PathAppend(szPath, L"BLOCK\\*.bmp");
        hFind = FindFirstFile(szPath, &find);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                items.push_back(find.cFileName);
            } while (FindNextFile(hFind, &find));

            FindClose(hFind);
        }

        // BLOCK\*.emf
        GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath));
        PathRemoveFileSpec(szPath);
        PathAppend(szPath, L"BLOCK\\*.emf");
        hFind = FindFirstFile(szPath, &find);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                items.push_back(find.cFileName);
            } while (FindNextFile(hFind, &find));

            FindClose(hFind);
        }

        // ソートする。
        std::sort(items.begin(), items.end());

        // コンボボックスに項目を追加する。
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_AddString(hCmb1, XgLoadStringDx1(IDS_NONE));
        for (auto& item : items)
        {
            ComboBox_AddString(hCmb1, item.c_str());
        }

        if (xg_strBlackCellImage.empty())
        {
            // 黒マス画像なし。
            ComboBox_SetText(hCmb1, XgLoadStringDx1(IDS_NONE));
            ComboBox_SetCurSel(hCmb1, ComboBox_FindStringExact(hCmb1, -1, XgLoadStringDx1(IDS_NONE)));
        }
        else
        {
            // 黒マス画像あり。
            LPCWSTR psz = PathFindFileName(xg_strBlackCellImage.c_str());
            ComboBox_SetText(hCmb1, psz);
            ComboBox_SetCurSel(hCmb1, ComboBox_FindStringExact(hCmb1, -1, psz));
        }

        // 二重マス文字。
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        for (INT i = IDS_DBLFRAME_LETTERS_1; i <= IDS_DBLFRAME_LETTERS_10; ++i) {
            ComboBox_AddString(hCmb2, XgLoadStringDx1(i));
        }
        ComboBox_SetText(hCmb2, xg_strDoubleFrameLetters.c_str());

        UpdateBlockPreview(hwnd);

        return TRUE;
    }

    // [設定]ダイアログで[OK]ボタンを押された。
    void OnOK(HWND hwnd)
    {
        INT nValue1, nValue2;
        BOOL bTranslated;

        // セルの文字の大きさ。
        bTranslated = FALSE;
        nValue1 = GetDlgItemInt(hwnd, edt4, &bTranslated, FALSE);
        if (bTranslated && 0 <= nValue1 && nValue1 <= 100)
        {
            ;
        }
        else
        {
            // エラー。
            HWND hEdt4 = GetDlgItem(hwnd, edt4);
            Edit_SetSel(hEdt4, 0, -1);
            SetFocus(hEdt4);
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_INVALIDVALUE), NULL, MB_ICONERROR);
            return;
        }

        // 小さい文字の大きさ。
        bTranslated = FALSE;
        nValue2 = GetDlgItemInt(hwnd, edt5, &bTranslated, FALSE);
        if (bTranslated && 0 <= nValue2 && nValue2 <= 100)
        {
            ;
        }
        else
        {
            // エラー。
            HWND hEdt5 = GetDlgItem(hwnd, edt5);
            Edit_SetSel(hEdt5, 0, -1);
            SetFocus(hEdt5);
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_INVALIDVALUE), NULL, MB_ICONERROR);
            return;
        }

        // 文字の大きさの設定。
        xg_nCellCharPercents = nValue1;
        xg_nSmallCharPercents = nValue2;

        // フォント名を取得する。
        WCHAR szName[LF_FACESIZE];

        // セルフォント。
        ::GetDlgItemTextW(hwnd, edt1, szName, ARRAYSIZE(szName));
        StringCbCopy(xg_szCellFont, sizeof(xg_szCellFont), szName);

        // 小さい文字のフォント。
        ::GetDlgItemTextW(hwnd, edt2, szName, ARRAYSIZE(szName));
        StringCbCopy(xg_szSmallFont, sizeof(xg_szSmallFont), szName);

        // UIフォント。
        ::GetDlgItemTextW(hwnd, edt3, szName, ARRAYSIZE(szName));
        StringCbCopy(xg_szUIFont, sizeof(xg_szUIFont), szName);

        // 黒マス画像の名前を取得。
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_GetText(hCmb1, szName, ARRAYSIZE(szName));

        // 黒マス画像の初期化。
        xg_strBlackCellImage.clear();
        ::DeleteObject(xg_hbmBlackCell);
        xg_hbmBlackCell = NULL;
        DeleteEnhMetaFile(xg_hBlackCellEMF);
        xg_hBlackCellEMF = NULL;

        // もし黒マス画像が指定されていれば
        if (szName[0])
        {
            // パス名をセット。
            WCHAR szPath[MAX_PATH];
            GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath));
            PathRemoveFileSpec(szPath);
            PathAppend(szPath, L"BLOCK");
            PathAppend(szPath, szName);

            if (PathFileExists(szPath))
            {
                // ファイルが存在すれば、画像を読み込む。
                xg_strBlackCellImage = szPath;
                xg_hbmBlackCell = LoadBitmapFromFile(xg_strBlackCellImage.c_str());
                if (!xg_hbmBlackCell)
                {
                    xg_hBlackCellEMF = GetEnhMetaFile(xg_strBlackCellImage.c_str());
                }
            }

            if (!xg_hbmBlackCell && !xg_hBlackCellEMF) {
                // 画像が無効なら、パスも無効化。
                xg_strBlackCellImage.clear();
            } else if (xg_nViewMode == XG_VIEW_SKELETON) {
                // 画像が有効ならスケルトンビューを通常ビューに戻す。
                xg_nViewMode = XG_VIEW_NORMAL;
            }
        }

        // ツールバーを表示するか？
        xg_bShowToolBar = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        if (xg_bShowToolBar)
            ::ShowWindow(xg_hToolBar, SW_SHOWNOACTIVATE);
        else
            ::ShowWindow(xg_hToolBar, SW_HIDE);

        // 太枠をつけるか？
        xg_bAddThickFrame = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);

        // 色を設定する。
        xg_rgbWhiteCellColor = s_rgbColors[0];
        xg_rgbBlackCellColor = s_rgbColors[1];
        xg_rgbMarkedCellColor = s_rgbColors[2];

        // 二重マスに枠をつけるか？
        xg_bDrawFrameForMarkedCell = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);

        // レイアウトを調整する。
        ::PostMessageW(xg_hMainWnd, WM_SIZE, 0, 0);
        if (::IsWindow(xg_hHintsWnd)) {
            XG_HintsWnd::UpdateHintData();
            XgOpenHintsByWindow(xg_hHintsWnd);
            ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
        }

        // 二重マス文字。
        WCHAR szText[MAX_PATH];
        ComboBox_GetText(GetDlgItem(hwnd, cmb2), szText, _countof(szText));
        xg_strDoubleFrameLetters = szText;

        // イメージを更新する。
        XgUpdateImage(xg_hMainWnd);
    }

    // UIフォントの論理オブジェクトを設定する。
    void SetUIFont(HWND hwnd, const LOGFONTW *plf)
    {
        if (plf == NULL) {
            SetDlgItemTextW(hwnd, edt3, NULL);
            return;
        }

        HDC hdc = ::CreateCompatibleDC(NULL);
        int point_size = -MulDiv(plf->lfHeight, 72, ::GetDeviceCaps(hdc, LOGPIXELSY));
        ::DeleteDC(hdc);

        WCHAR szData[128];
        StringCbPrintf(szData, sizeof(szData), L"%s, %upt", plf->lfFaceName, point_size);
        ::SetDlgItemTextW(hwnd, edt3, szData);
    }

    // [設定]ダイアログで[変更...]ボタンを押された。
    void OnChange(HWND hwnd, int i)
    {
        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfQuality = ANTIALIASED_QUALITY;

        // ユーザーにフォント名を問い合わせる。
        CHOOSEFONTW cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.lStructSize = sizeof(cf);
        cf.hwndOwner = hwnd;
        cf.lpLogFont = &lf;
        cf.nFontType = SCREEN_FONTTYPE | SIMULATED_FONTTYPE | REGULAR_FONTTYPE;
        lf.lfCharSet = DEFAULT_CHARSET;

        switch (i) {
        case 0:
            cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCALABLEONLY |
                       CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
            StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);
            if (::ChooseFontW(&cf)) {
                // 取得したフォントをダイアログへ格納する。
                ::SetDlgItemTextW(hwnd, edt1, lf.lfFaceName);
            }
            break;

        case 1:
            cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCALABLEONLY |
                       CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
            StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szSmallFont);
            if (::ChooseFontW(&cf)) {
                // 取得したフォントをダイアログへ格納する。
                ::SetDlgItemTextW(hwnd, edt2, lf.lfFaceName);
            }
            break;

        case 2:
            cf.lpLogFont = XgGetUIFont();
            cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCREENFONTS |
                       CF_INITTOLOGFONTSTRUCT | CF_LIMITSIZE;
            cf.nSizeMin = 8;
            cf.nSizeMax = 20;
            if (::ChooseFontW(&cf)) {
                // 取得したフォントをダイアログへ格納する。
                SetUIFont(hwnd, cf.lpLogFont);
            }
            break;
        }
    }

    // [設定]ダイアログで[リセット]ボタンを押された。
    void OnReset(HWND hwnd, int i)
    {
        switch (i) {
        case 0:
            ::SetDlgItemTextW(hwnd, edt1, L"");
            break;

        case 1:
            ::SetDlgItemTextW(hwnd, edt2, L"");
            break;

        case 2:
            ::SetDlgItemTextW(hwnd, edt3, L"");
            break;
        }
    }

    // [設定]ダイアログのオーナードロー。
    void OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        HDC hdc = pdis->hDC;

        if (pdis->CtlType != ODT_BUTTON) {
            return;
        }

        BOOL bSelected = !!(pdis->itemState & ODS_SELECTED);
        BOOL bFocus = !!(pdis->itemState & ODS_FOCUS);
        RECT& rcItem = pdis->rcItem;

        ::DrawFrameControl(hdc, &rcItem, DFC_BUTTON,
            DFCS_BUTTONPUSH |
            (bSelected ? DFCS_PUSHED : 0)
        );

        HBRUSH hbr = NULL;
        switch (pdis->CtlID) {
        case psh7:
            hbr = ::CreateSolidBrush(s_rgbColors[0]);
            break;

        case psh8:
            hbr = ::CreateSolidBrush(s_rgbColors[1]);
            break;

        case psh9:
            hbr = ::CreateSolidBrush(s_rgbColors[2]);
            break;

        default:
            return;
        }

        ::InflateRect(&rcItem, -4, -4);
        ::FillRect(hdc, &rcItem, hbr);
        ::DeleteObject(hbr);

        if (bFocus) {
            ::InflateRect(&rcItem, 2, 2);
            ::DrawFocusRect(hdc, &rcItem);
        }
    }

    // 色を指定する。
    void OnSetColor(HWND hwnd, int nIndex)
    {
        COLORREF clr;
        switch (nIndex) {
        case 0:
            clr = s_rgbColors[0];
            break;

        case 1:
            clr = s_rgbColors[1];
            break;

        case 2:
            clr = s_rgbColors[2];
            break;

        default:
            return;
        }

        CHOOSECOLORW cc;
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hwnd;
        cc.rgbResult = clr;
        cc.lpCustColors = s_rgbColorTable;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (ChooseColorW(&cc)) {
            switch (nIndex) {
            case 0:
                s_rgbColors[0] = cc.rgbResult;
                ::InvalidateRect(::GetDlgItem(hwnd, psh7), NULL, TRUE);
                break;

            case 1:
                s_rgbColors[1] = cc.rgbResult;
                ::InvalidateRect(::GetDlgItem(hwnd, psh8), NULL, TRUE);
                break;

            case 2:
                s_rgbColors[2] = cc.rgbResult;
                ::InvalidateRect(::GetDlgItem(hwnd, psh9), NULL, TRUE);
                break;

            default:
                return;
            }

        }
    }

    // [設定]ダイアログのダイアログ プロシージャー。
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
        case WM_INITDIALOG:
            return OnInitDialog(hwnd);

        case WM_DRAWITEM:
            OnDrawItem(hwnd, wParam, lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
            case IDOK:
                OnOK(hwnd);
                ::EndDialog(hwnd, IDOK);
                break;

            case IDCANCEL:
                ::EndDialog(hwnd, IDCANCEL);
                break;

            case psh1:
                OnChange(hwnd, 0);
                break;

            case psh2:
                OnChange(hwnd, 1);
                break;

            case psh3:
                OnReset(hwnd, 0);
                break;

            case psh4:
                OnReset(hwnd, 1);
                break;

            case psh5:
                OnChange(hwnd, 2);
                break;

            case psh6:
                OnReset(hwnd, 2);
                break;

            case psh7:
                OnSetColor(hwnd, 0);
                break;

            case psh8:
                OnSetColor(hwnd, 1);
                break;

            case psh9:
                OnSetColor(hwnd, 2);
                break;

            case psh10:
                SetDlgItemInt(hwnd, edt4, DEF_CELL_CHAR_SIZE, FALSE);
                break;

            case psh11:
                SetDlgItemInt(hwnd, edt5, DEF_SMALL_CHAR_SIZE, FALSE);
                break;

            case cmb1:
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    UpdateBlockPreview(hwnd);
                }
            }
            break;
        }
        return 0;
    }

    // BLOCKのプレビュー。
    void UpdateBlockPreview(HWND hwnd)
    {
        HWND hIco1 = GetDlgItem(hwnd, ico1);
        HWND hIco2 = GetDlgItem(hwnd, ico2);
        SetWindowPos(hIco1, NULL, 0, 0, 32, 32, SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);
        SetWindowPos(hIco2, NULL, 0, 0, 32, 32, SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);
        HBITMAP hbmOld = (HBITMAP)SendMessageW(hIco1, STM_GETIMAGE, IMAGE_BITMAP, 0);
        HENHMETAFILE hOldEMF = (HENHMETAFILE)SendMessageW(hIco2, STM_GETIMAGE, IMAGE_ENHMETAFILE, 0);

        WCHAR szPath[MAX_PATH], szName[128];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_GetText(hCmb1, szName, ARRAYSIZE(szName));

        GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath));
        PathRemoveFileSpec(szPath);
        PathAppend(szPath, L"BLOCK");
        PathAppend(szPath, szName);

        if (PathFileExistsW(szPath))
        {
            if (lstrcmpiW(PathFindExtensionW(szPath), L".bmp") == 0)
            {
                HBITMAP hbm1 = LoadBitmapFromFile(szPath);
                if (hbm1)
                {
                    HBITMAP hbm2 = (HBITMAP)CopyImage(hbm1, IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION);
                    DeleteObject(hbm1);
                    SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbm2);
                    SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)NULL);
                    ShowWindow(hIco1, SW_SHOWNOACTIVATE);
                    DeleteObject(hbmOld);
                    DeleteEnhMetaFile(hOldEMF);
                    return;
                }
            }
            else if (lstrcmpiW(PathFindExtensionW(szPath), L".emf") == 0)
            {
                HENHMETAFILE hEMF = GetEnhMetaFile(szPath);
                if (hEMF)
                {
                    SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
                    SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)hEMF);
                    ShowWindow(hIco2, SW_SHOWNOACTIVATE);
                    DeleteObject(hbmOld);
                    DeleteEnhMetaFile(hOldEMF);
                    return;
                }
            }
        }

        SendDlgItemMessageW(hwnd, ico1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)0);
        SendDlgItemMessageW(hwnd, ico2, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)0);
        DeleteObject(hbmOld);
        DeleteEnhMetaFile(hOldEMF);
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_CONFIG);
    }
};
