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

// 「見た目の設定」ダイアログ。
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

    LPCWSTR m_pszAutoFile = NULL;
    BOOL m_bUpdating = FALSE;

    XG_SettingsDialog()
    {
    }

    // [設定]ダイアログの初期化。
    BOOL OnInitDialog(HWND hwnd)
    {
        // ドロップを受け付ける。
        ::DragAcceptFiles(hwnd, TRUE);

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

        // スケルトンビューか？
        ::CheckDlgButton(hwnd, chx1,
            ((xg_nViewMode == XG_VIEW_SKELETON) ? BST_CHECKED : BST_UNCHECKED));
        // 太枠をつけるか？
        ::CheckDlgButton(hwnd, chx2,
            (xg_bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));
        // 二重マスに枠をつけるか？
        ::CheckDlgButton(hwnd, chx3,
            (xg_bDrawFrameForMarkedCell ? BST_CHECKED : BST_UNCHECKED));
        // 英小文字か？
        ::CheckDlgButton(hwnd, chx4,
            (xg_bLowercase ? BST_CHECKED : BST_UNCHECKED));
        // ひらがなか？
        ::CheckDlgButton(hwnd, chx5,
            (xg_bHiragana ? BST_CHECKED : BST_UNCHECKED));

        // 文字の大きさ。
        ::SetDlgItemInt(hwnd, edt4, xg_nCellCharPercents, FALSE);
        ::SetDlgItemInt(hwnd, edt5, xg_nSmallCharPercents, FALSE);
        // 大きさの範囲を指定。
        ::SendDlgItemMessage(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(100, 3));
        ::SendDlgItemMessage(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(100, 3));

        // 画像ファイルリストを取得する。
        std::vector<std::wstring> items;
        XgGetImageList(items);

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
            ComboBox_RealSetText(hCmb1, XgLoadStringDx1(IDS_NONE));
        }
        else
        {
            // 黒マス画像あり。
            WCHAR szPath[MAX_PATH];
            XgGetCanonicalImagePath(szPath, xg_strBlackCellImage.c_str());
            ComboBox_RealSetText(hCmb1, szPath);
        }

        // 二重マス文字。
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        for (INT i = IDS_DBLFRAME_LETTERS_1; i <= IDS_DBLFRAME_LETTERS_10; ++i) {
            ComboBox_AddString(hCmb2, XgLoadStringDx1(i));
        }
        ComboBox_RealSetText(hCmb2, xg_strDoubleFrameLetters.c_str());

        UpdateBlockPreview(hwnd);

        // 可能ならば自動で適用。
        if (m_pszAutoFile)
        {
            if (DoImportLooks(hwnd, m_pszAutoFile))
            {
                PostMessageW(hwnd, WM_COMMAND, IDOK, 0);
            }
        }

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
        WCHAR szText[MAX_PATH];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));

        // 黒マス画像の初期化。
        xg_strBlackCellImage.clear();
        ::DeleteObject(xg_hbmBlackCell);
        xg_hbmBlackCell = NULL;
        DeleteEnhMetaFile(xg_hBlackCellEMF);
        xg_hBlackCellEMF = NULL;

        // もし黒マス画像が指定されていれば
        if (szText[0])
        {
            HBITMAP hbm = NULL;
            HENHMETAFILE hEMF = NULL;
            if (XgLoadImage(szText, hbm, hEMF))
            {
                // ファイルが存在すれば、画像を読み込む。
                xg_strBlackCellImage = szText;
                xg_hbmBlackCell = hbm;
                xg_hBlackCellEMF = hEMF;
                if (xg_nViewMode == XG_VIEW_SKELETON)
                {
                    // 画像が有効ならスケルトンビューを通常ビューに戻す。
                    xg_nViewMode = XG_VIEW_NORMAL;
                }
            }
        }

        // スケルトンビューか？
        if (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED)
            xg_nViewMode = XG_VIEW_SKELETON;
        else
            xg_nViewMode = XG_VIEW_NORMAL;

        // 太枠をつけるか？
        xg_bAddThickFrame = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        // 二重マスに枠をつけるか？
        xg_bDrawFrameForMarkedCell = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
        // 英小文字か？
        xg_bLowercase = (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
        // ひらがなか？
        xg_bHiragana = (::IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);

        // 色を設定する。
        xg_rgbWhiteCellColor = s_rgbColors[0];
        xg_rgbBlackCellColor = s_rgbColors[1];
        xg_rgbMarkedCellColor = s_rgbColors[2];

        // レイアウトを調整する。
        ::PostMessageW(xg_hMainWnd, WM_SIZE, 0, 0);
        if (::IsWindow(xg_hHintsWnd)) {
            XG_HintsWnd::UpdateHintData();
            XgOpenHintsByWindow(xg_hHintsWnd);
            ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
        }

        // 二重マス文字。
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ComboBox_RealGetText(hCmb2, szText, _countof(szText));
        xg_strDoubleFrameLetters = szText;

        // イメージを更新する。
        XgUpdateImage(xg_hMainWnd);
    }

    // LOOKSファイルのインポート。
    BOOL DoImportLooks(HWND hwnd, LPCWSTR pszFileName)
    {
        if (!PathFileExistsW(pszFileName))
            return FALSE;

        WCHAR szText[1024], szText2[MAX_PATH];

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        GetPrivateProfileStringW(L"Looks", L"BlackCellImage", L"", szText, _countof(szText), pszFileName);
        BOOL bHasImage = FALSE;
        if (szText[0])
        {
            // 黒マス画像あり。事前に存在をチェックする。
            HBITMAP hbm = NULL;
            HENHMETAFILE hEMF = NULL;
            bHasImage = XgLoadImage(szText, hbm, hEMF);
            DeleteObject(hbm);
            DeleteEnhMetaFile(hEMF);
        }

        // 色。
        GetPrivateProfileStringW(L"Looks", L"WhiteCellColor", L"16777215", szText, _countof(szText), pszFileName);
        s_rgbColors[0] = _wtoi(szText);
        InvalidateRect(GetDlgItem(hwnd, psh7), NULL, TRUE);

        GetPrivateProfileStringW(L"Looks", L"BlackCellColor", L"3355443", szText, _countof(szText), pszFileName);
        s_rgbColors[1] = _wtoi(szText);
        InvalidateRect(GetDlgItem(hwnd, psh8), NULL, TRUE);

        GetPrivateProfileStringW(L"Looks", L"MarkedCellColor", L"16777215", szText, _countof(szText), pszFileName);
        s_rgbColors[2] = _wtoi(szText);
        InvalidateRect(GetDlgItem(hwnd, psh9), NULL, TRUE);

        // フォント。
        GetPrivateProfileStringW(L"Looks", L"CellFont", L"", szText, _countof(szText), pszFileName);
        SetDlgItemTextW(hwnd, edt1, szText);

        GetPrivateProfileStringW(L"Looks", L"SmallFont", L"", szText, _countof(szText), pszFileName);
        SetDlgItemTextW(hwnd, edt2, szText);

        GetPrivateProfileStringW(L"Looks", L"UIFont", L"", szText, _countof(szText), pszFileName);
        SetDlgItemTextW(hwnd, edt3, szText);

        // スケルトンビューか？
        if (XgIsUserJapanese())
            GetPrivateProfileStringW(L"Looks", L"SkeletonView", L"0", szText, _countof(szText), pszFileName);
        else
            GetPrivateProfileStringW(L"Looks", L"SkeletonView", L"1", szText, _countof(szText), pszFileName);
        BOOL bSkeltonView = _wtoi(szText);
        ::CheckDlgButton(hwnd, chx1, (bSkeltonView ? BST_CHECKED : BST_UNCHECKED));

        // 太枠をつけるか？
        GetPrivateProfileStringW(L"Looks", L"AddThickFrame", L"1", szText, _countof(szText), pszFileName);
        BOOL bAddThickFrame = _wtoi(szText);
        ::CheckDlgButton(hwnd, chx2, (bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));

        // 二重マスに枠をつけるか？
        GetPrivateProfileStringW(L"Looks", L"DrawFrameForMarkedCell", L"1", szText, _countof(szText), pszFileName);
        BOOL bDrawFrameForMarkedCell = _wtoi(szText);
        ::CheckDlgButton(hwnd, chx3, (bDrawFrameForMarkedCell ? BST_CHECKED : BST_UNCHECKED));

        // 英小文字か？
        GetPrivateProfileStringW(L"Looks", L"Lowercase", L"0", szText, _countof(szText), pszFileName);
        BOOL bLowercase = _wtoi(szText);
        ::CheckDlgButton(hwnd, chx4, (bLowercase ? BST_CHECKED : BST_UNCHECKED));

        // ひらがなか？
        GetPrivateProfileStringW(L"Looks", L"Hiragana", L"0", szText, _countof(szText), pszFileName);
        BOOL bHiragana = _wtoi(szText);
        ::CheckDlgButton(hwnd, chx5, (bHiragana ? BST_CHECKED : BST_UNCHECKED));

        // 文字の大きさ。
        StringCbPrintfW(szText2, sizeof(szText2), L"%d", DEF_CELL_CHAR_SIZE);
        GetPrivateProfileStringW(L"Looks", L"CellCharPercents", szText2, szText, _countof(szText), pszFileName);
        BOOL nCellCharPercents = _wtoi(szText);
        ::SetDlgItemInt(hwnd, edt4, nCellCharPercents, FALSE);

        StringCbPrintfW(szText2, sizeof(szText2), L"%d", DEF_SMALL_CHAR_SIZE);
        GetPrivateProfileStringW(L"Looks", L"SmallCharPercents", szText2, szText, _countof(szText), pszFileName);
        BOOL nSmallCharPercents = _wtoi(szText);
        ::SetDlgItemInt(hwnd, edt5, nSmallCharPercents, FALSE);

        // 黒マス画像。
        GetPrivateProfileStringW(L"Looks", L"BlackCellImage", L"", szText, _countof(szText), pszFileName);
        if (!szText[0] || !bHasImage)
        {
            // 黒マス画像なし。
            ComboBox_RealSetText(hCmb1, XgLoadStringDx1(IDS_NONE));
        }
        else
        {
            // 黒マス画像あり。
            WCHAR szPath[MAX_PATH];
            XgGetCanonicalImagePath(szPath, szText);
            ComboBox_RealSetText(hCmb1, szPath);
        }

        // 二重マス文字。
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        GetPrivateProfileStringW(L"Looks", L"DoubleFrameLetters", XgLoadStringDx1(IDS_DBLFRAME_LETTERS_1), szText, _countof(szText), pszFileName);
        {
            std::vector<BYTE> data;
            XgHexToBin(data, szText);
            std::wstring str;
            str.resize(data.size() / sizeof(WCHAR));
            memcpy(&str[0], data.data(), data.size());
            ComboBox_RealSetText(hCmb2, str.c_str());
        }

        UpdateBlockPreview(hwnd);

        return TRUE;
    }

    // LOOKSファイルのエクスポート。
    BOOL DoExportLooks(HWND hwnd, LPCWSTR pszFileName)
    {
        INT nValue1, nValue2;
        BOOL bTranslated;

        // 書く前にファイルを消す。
        DeleteFileW(pszFileName);

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
            return FALSE;
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
            return FALSE;
        }

        // 文字の大きさの設定。
        WritePrivateProfileStringW(L"Looks", L"CellCharPercents", XgIntToStr(nValue1), pszFileName);
        WritePrivateProfileStringW(L"Looks", L"SmallCharPercents", XgIntToStr(nValue2), pszFileName);

        // フォント名を取得する。
        WCHAR szName[LF_FACESIZE];

        // セルフォント。
        ::GetDlgItemTextW(hwnd, edt1, szName, ARRAYSIZE(szName));
        WritePrivateProfileStringW(L"Looks", L"CellFont", szName, pszFileName);

        // 小さい文字のフォント。
        ::GetDlgItemTextW(hwnd, edt2, szName, ARRAYSIZE(szName));
        WritePrivateProfileStringW(L"Looks", L"SmallFont", szName, pszFileName);

        // UIフォント。
        ::GetDlgItemTextW(hwnd, edt3, szName, ARRAYSIZE(szName));
        WritePrivateProfileStringW(L"Looks", L"UIFont", szName, pszFileName);

        // 黒マス画像の名前を取得。
        WCHAR szText[MAX_PATH];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));

        // もし黒マス画像が指定されていれば
        std::wstring str = szText;
        if (str.size() && str != XgLoadStringDx1(IDS_NONE))
        {
            WCHAR szPath[MAX_PATH];
            XgGetCanonicalImagePath(szPath, szText);
            WritePrivateProfileStringW(L"Looks", L"BlackCellImage", szPath, pszFileName);
        }
        else
        {
            WritePrivateProfileStringW(L"Looks", L"BlackCellImage", L"", pszFileName);
        }

        // スケルトンビューか？
        BOOL bSkeltonView = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
        WritePrivateProfileStringW(L"Looks", L"SkeletonView", XgIntToStr(bSkeltonView), pszFileName);

        // 太枠をつけるか？
        BOOL bAddThickFrame = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
        WritePrivateProfileStringW(L"Looks", L"AddThickFrame", XgIntToStr(bAddThickFrame), pszFileName);

        // 二重マスに枠をつけるか？
        BOOL bDrawFrameForMarkedCell = (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED);
        WritePrivateProfileStringW(L"Looks", L"DrawFrameForMarkedCell", XgIntToStr(bDrawFrameForMarkedCell), pszFileName);

        // 英小文字か？
        BOOL bLowercase = (::IsDlgButtonChecked(hwnd, chx4) == BST_CHECKED);
        WritePrivateProfileStringW(L"Looks", L"Lowercase", XgIntToStr(bLowercase), pszFileName);

        // ひらがなか？
        BOOL bHiragana = (::IsDlgButtonChecked(hwnd, chx5) == BST_CHECKED);
        WritePrivateProfileStringW(L"Looks", L"Hiragana", XgIntToStr(bHiragana), pszFileName);

        // 色を設定する。
        WritePrivateProfileStringW(L"Looks", L"WhiteCellColor", XgIntToStr(s_rgbColors[0]), pszFileName);
        WritePrivateProfileStringW(L"Looks", L"BlackCellColor", XgIntToStr(s_rgbColors[1]), pszFileName);
        WritePrivateProfileStringW(L"Looks", L"MarkedCellColor", XgIntToStr(s_rgbColors[2]), pszFileName);

        // 二重マス文字。
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ComboBox_RealGetText(hCmb2, szText, _countof(szText));
        {
            std::wstring str = XgBinToHex(szText, lstrlenW(szText) * sizeof(WCHAR));
            WritePrivateProfileStringW(L"Looks", L"DoubleFrameLetters", str.c_str(), pszFileName);
        }

        // フラッシュ！
        return WritePrivateProfileStringW(NULL, NULL, NULL, pszFileName);
    }

    // 設定のインポート。
    BOOL OnImportLooks(HWND hwnd)
    {
        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
        ofn.lpstrFilter = L"LOOKS File (*.looks)\0*.looks\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = XgLoadStringDx1(IDS_IMPORTLOOKSFILE);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                    OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"LOOKS";
        if (::GetOpenFileNameW(&ofn))
        {
            return DoImportLooks(hwnd, szFile);
        }
        return FALSE;
    }

    // 設定のエクスポート。
    BOOL OnExportLooks(HWND hwnd)
    {
        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
        ofn.lpstrFilter = L"LOOKS File (*.looks)\0*.looks\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = XgLoadStringDx1(IDS_EXPORTLOOKSFILE);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
        ofn.lpstrDefExt = L"LOOKS";
        if (::GetSaveFileNameW(&ofn))
        {
            return DoExportLooks(hwnd, szFile);
        }
        return FALSE;
    }

    // 設定のリセット。
    void OnResetLooks(HWND hwnd)
    {
        // 色。
        s_rgbColors[0] = _wtoi(L"16777215");
        InvalidateRect(GetDlgItem(hwnd, psh7), NULL, TRUE);

        s_rgbColors[1] = _wtoi(L"3355443");
        InvalidateRect(GetDlgItem(hwnd, psh8), NULL, TRUE);

        s_rgbColors[2] = _wtoi(L"16777215");
        InvalidateRect(GetDlgItem(hwnd, psh9), NULL, TRUE);

        // フォント。
        SetDlgItemTextW(hwnd, edt1, L"");
        SetDlgItemTextW(hwnd, edt2, L"");
        SetDlgItemTextW(hwnd, edt3, L"");

        // スケルトンビューか？
        BOOL bSkeltonView = !XgIsUserJapanese();
        ::CheckDlgButton(hwnd, chx1, (bSkeltonView ? BST_CHECKED : BST_UNCHECKED));

        // 太枠をつけるか？
        BOOL bAddThickFrame = TRUE;
        ::CheckDlgButton(hwnd, chx2, (bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));

        // 二重マスに枠をつけるか？
        BOOL bDrawFrameForMarkedCell = TRUE;
        ::CheckDlgButton(hwnd, chx3, (bDrawFrameForMarkedCell ? BST_CHECKED : BST_UNCHECKED));

        // 英小文字か？
        BOOL bLowercase = FALSE;
        ::CheckDlgButton(hwnd, chx4, (bLowercase ? BST_CHECKED : BST_UNCHECKED));

        // ひらがなか？
        BOOL bHiragana = FALSE;
        ::CheckDlgButton(hwnd, chx5, (bHiragana ? BST_CHECKED : BST_UNCHECKED));

        // 文字の大きさ。
        BOOL nCellCharPercents = DEF_CELL_CHAR_SIZE;
        ::SetDlgItemInt(hwnd, edt4, nCellCharPercents, FALSE);

        BOOL nSmallCharPercents = DEF_SMALL_CHAR_SIZE;
        ::SetDlgItemInt(hwnd, edt5, nSmallCharPercents, FALSE);

        // 黒マス画像。
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        // 黒マス画像なし。
        ComboBox_SetCurSel(hCmb1, ComboBox_FindStringExact(hCmb1, -1, XgLoadStringDx1(IDS_NONE)));
        ComboBox_SetText(hCmb1, XgLoadStringDx1(IDS_NONE));

        // 二重マス文字。
        HWND hCmb2 = GetDlgItem(hwnd, cmb2);
        ComboBox_SetText(hCmb2, XgLoadStringDx1(IDS_DBLFRAME_LETTERS_1));

        UpdateBlockPreview(hwnd);
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

    // ファイルがドロップされた？
    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR szFile[MAX_PATH];
        DragQueryFileW(hdrop, 0, szFile, _countof(szFile));

        LPWSTR pchDotExt =  PathFindExtensionW(szFile);
        if (lstrcmpiW(pchDotExt, L".looks") == 0)
        {
            DoImportLooks(hwnd, szFile);
        }
        else
        {
            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            ComboBox_RealSetText(hCmb1, szFile);
            UpdateBlockPreview(hwnd);
        }

        DragFinish(hdrop);
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

        case WM_DROPFILES:
            OnDropFiles(hwnd, (HDROP)wParam);
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

            case psh12:
                {
                    HWND hPsh12 = GetDlgItem(hwnd, psh12);
                    RECT rc;
                    GetWindowRect(hPsh12, &rc);
                    POINT pt = { rc.left, (rc.top + rc.bottom) / 2 };
                    HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(3));
                    HMENU hSubMenu = GetSubMenu(hMenu, 0);
                    SetForegroundWindow(hwnd);
                    TPMPARAMS params = { sizeof(params), rc };
                    INT id = TrackPopupMenuEx(hSubMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
                                              pt.x, pt.y, hwnd, &params);
                    if (id)
                    {
                        assert(id == psh13 || id == psh14 || id == psh15);
                        ::PostMessageW(hwnd, WM_COMMAND, id, 0);
                    }
                    ::DestroyMenu(hMenu);
                }
                break;

            case psh13:
                OnImportLooks(hwnd);
                break;

            case psh14:
                OnExportLooks(hwnd);
                break;

            case psh15:
                OnResetLooks(hwnd);
                break;

            case cmb1:
                if (HIWORD(wParam) == CBN_SELENDOK || HIWORD(wParam) == CBN_EDITCHANGE)
                {
                    UpdateBlockPreview(hwnd);
                }
                break;

            case chx1:
                if (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED) {
                    if (!m_bUpdating) {
                        m_bUpdating = TRUE;
                        {
                            // 黒マス画像なし。
                            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
                            ComboBox_RealSetText(hCmb1, XgLoadStringDx1(IDS_NONE));
                            UpdateBlockPreview(hwnd);
                        }
                        m_bUpdating = FALSE;
                    }
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

        WCHAR szText[MAX_PATH];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));

        HBITMAP hbm1;
        HENHMETAFILE hEMF1;
        if (XgLoadImage(szText, hbm1, hEMF1))
        {
            if (!m_bUpdating)
            {
                m_bUpdating = TRUE;
                CheckDlgButton(hwnd, chx1, BST_UNCHECKED);
                m_bUpdating = FALSE;
            }

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
            if (hEMF1)
            {
                SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)NULL);
                SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)hEMF1);
                ShowWindow(hIco2, SW_SHOWNOACTIVATE);
                DeleteObject(hbmOld);
                DeleteEnhMetaFile(hOldEMF);
                return;
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
