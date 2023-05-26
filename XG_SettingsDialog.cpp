#include "XWordGiver.hpp"
#include "XG_SettingsDialog.hpp"

// マスのフォント。
WCHAR xg_szCellFont[LF_FACESIZE] = L"";
// 小さな文字のフォント。
WCHAR xg_szSmallFont[LF_FACESIZE] = L"";
// UIフォント。
WCHAR xg_szUIFont[LF_FACESIZE] = L"";

// 文字の大きさ（％）。
INT xg_nCellCharPercents = XG_DEF_CELL_CHAR_SIZE;

// 小さい文字の大きさ（％）。
INT xg_nSmallCharPercents = XG_DEF_SMALL_CHAR_SIZE;

// 黒マス画像。
HBITMAP xg_hbmBlackCell = nullptr;
HENHMETAFILE xg_hBlackCellEMF = nullptr;
std::wstring xg_strBlackCellImage;

// ビューモード。
XG_VIEW_MODE xg_nViewMode = XG_VIEW_NORMAL;

// 線の太さ（pt）。
float xg_nLineWidthInPt = XG_LINE_WIDTH_DEFAULT;

// 外枠の幅（pt）。
float xg_nOuterFrameInPt = XG_OUTERFRAME_DEFAULT;

// ひらがな表示か？
BOOL xg_bHiragana = FALSE;

// Lowercase表示か？
BOOL xg_bLowercase = FALSE;

// ツールバーを表示するか？
bool xg_bShowToolBar = true;

// 色。
COLORREF xg_rgbWhiteCellColor = RGB(255, 255, 255);
COLORREF xg_rgbBlackCellColor = RGB(0x33, 0x33, 0x33);
COLORREF xg_rgbMarkedCellColor = RGB(255, 255, 255);

// 二重マス文字。
std::wstring xg_strDoubleFrameLetters;

//////////////////////////////////////////////////////////////////////////////

// [設定]ダイアログの初期化。
BOOL XG_SettingsDialog::OnInitDialog(HWND hwnd)
{
    // ドロップを受け付ける。
    ::DragAcceptFiles(hwnd, TRUE);

    // 画面の中央に寄せる。
    XgCenterDialog(hwnd);

    // ハンドルをセットする。
    m_hwndWhite.m_hWnd = GetDlgItem(hwnd, psh7);
    m_hwndBlack.m_hWnd = GetDlgItem(hwnd, psh8);
    m_hwndMarked.m_hWnd = GetDlgItem(hwnd, psh9);

    // フォント名を格納する。
    ::SetDlgItemTextW(hwnd, edt1, xg_szCellFont);
    ::SetDlgItemTextW(hwnd, edt2, xg_szSmallFont);
    ::SetDlgItemTextW(hwnd, edt3, xg_szUIFont);

    // スケルトンビューか？
    ::CheckDlgButton(hwnd, chx1,
        ((xg_nViewMode == XG_VIEW_SKELETON) ? BST_CHECKED : BST_UNCHECKED));

    // 外枠をつけるか？
    ::CheckDlgButton(hwnd, chx2, (xg_bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));
    ::EnableWindow(GetDlgItem(hwnd, edt7), xg_bAddThickFrame);

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
    XgGetFileManager()->get_list(items);

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
        ComboBox_RealSetText(hCmb1, xg_strBlackCellImage.c_str());
    }

    // 二重マス文字。
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    for (INT i = IDS_DBLFRAME_LETTERS_1; i <= IDS_DBLFRAME_LETTERS_10; ++i) {
        ComboBox_AddString(hCmb2, XgLoadStringDx1(i));
    }
    ComboBox_RealSetText(hCmb2, xg_strDoubleFrameLetters.c_str());

    UpdateBlockPreview(hwnd);

    // 可能ならば自動で適用。
    if (m_strAutoFile.size())
    {
        if (m_bImport) {
            DoImportLooks(hwnd, m_strAutoFile.c_str());
        } else {
            DoExportLooks(hwnd, m_strAutoFile.c_str());
        }
        SendMessageW(hwnd, WM_COMMAND, IDOK, 0);
    }

    WCHAR szText[MAX_PATH];

    // 線の太さ。
    StringCchPrintfW(szText, _countof(szText), XG_LINE_WIDTH_FORMAT, xg_nLineWidthInPt);
    SetDlgItemTextW(hwnd, edt6, szText);

    // 外枠の幅。
    StringCchPrintfW(szText, _countof(szText), XG_OUTERFRAME_FORMAT, xg_nOuterFrameInPt);
    SetDlgItemTextW(hwnd, edt7, szText);

    return TRUE;
}

// [設定]ダイアログで[OK]ボタンを押された。
void XG_SettingsDialog::OnOK(HWND hwnd)
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
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_INVALIDVALUE), nullptr, MB_ICONERROR);
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
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_INVALIDVALUE), nullptr, MB_ICONERROR);
        return;
    }

    // 文字の大きさの設定。
    xg_nCellCharPercents = nValue1;
    xg_nSmallCharPercents = nValue2;

    // フォント名を取得する。
    WCHAR szName[LF_FACESIZE];

    // セルフォント。
    ::GetDlgItemTextW(hwnd, edt1, szName, _countof(szName));
    StringCbCopy(xg_szCellFont, sizeof(xg_szCellFont), szName);

    // 小さい文字のフォント。
    ::GetDlgItemTextW(hwnd, edt2, szName, _countof(szName));
    StringCbCopy(xg_szSmallFont, sizeof(xg_szSmallFont), szName);

    // UIフォント。
    ::GetDlgItemTextW(hwnd, edt3, szName, _countof(szName));
    StringCbCopy(xg_szUIFont, sizeof(xg_szUIFont), szName);

    // 黒マス画像を取得。
    WCHAR szText[MAX_PATH];
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    ComboBox_RealGetText(hCmb1, szText, _countof(szText));
    std::wstring strBlock = szText;
    xg_str_trim(strBlock);
    if (XgGetFileManager()->load_block_image(strBlock))
    {
        xg_strBlackCellImage = XgGetFileManager()->get_canonical(strBlock);
    }
    else
    {
        xg_strBlackCellImage.clear();
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
    xg_rgbWhiteCellColor = m_hwndWhite.GetColor();
    xg_rgbBlackCellColor = m_hwndBlack.GetColor();
    xg_rgbMarkedCellColor = m_hwndMarked.GetColor();

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

    // 線の太さ。
    GetDlgItemTextW(hwnd, edt6, szText, _countof(szText));
    std::wstring str = szText;
    xg_str_trim(str);
    float value = wcstof(str.c_str(), nullptr);
    if (value > XG_MAX_LINEWIDTH)
        value = XG_MAX_LINEWIDTH;
    if (value < XG_MIN_LINEWIDTH)
        value = XG_MIN_LINEWIDTH;
    xg_nLineWidthInPt = value;

    // 外枠の幅。
    GetDlgItemTextW(hwnd, edt7, szText, _countof(szText));
    str = szText;
    xg_str_trim(str);
    value = wcstof(str.c_str(), nullptr);
    if (value > XG_MAX_OUTERFRAME)
        value = XG_MAX_OUTERFRAME;
    if (value < XG_MIN_OUTERFRAME)
        value = XG_MIN_OUTERFRAME;
    xg_nOuterFrameInPt = value;

    // イメージを更新する。
    XgUpdateImage(xg_hMainWnd);

    XG_FILE_MODIFIED(TRUE);
}

// LOOKSファイルのインポート。
BOOL XG_SettingsDialog::DoImportLooks(HWND hwnd, LPCWSTR pszFileName)
{
    if (!PathFileExistsW(pszFileName))
        return FALSE;

    XgGetFileManager()->set_looks(pszFileName);

    WCHAR szText[1024], szText2[MAX_PATH];

    HWND hCmb1 = GetDlgItem(hwnd, cmb1);

    // 色。
    GetPrivateProfileStringW(L"Looks", L"WhiteCellColor", XG_WHITE_COLOR_DEFAULT, szText, _countof(szText), pszFileName);
    m_hwndWhite.SetColor(_wtoi(szText));
    InvalidateRect(m_hwndWhite, nullptr, TRUE);

    GetPrivateProfileStringW(L"Looks", L"BlackCellColor", XG_BLACK_COLOR_DEFAULT, szText, _countof(szText), pszFileName);
    m_hwndBlack.SetColor(_wtoi(szText));
    InvalidateRect(m_hwndBlack, nullptr, TRUE);

    GetPrivateProfileStringW(L"Looks", L"MarkedCellColor", XG_MARKED_COLOR_DEFAULT, szText, _countof(szText), pszFileName);
    m_hwndMarked.SetColor(_wtoi(szText));
    InvalidateRect(m_hwndMarked, nullptr, TRUE);

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

    // 外枠をつけるか？
    GetPrivateProfileStringW(L"Looks", L"AddThickFrame", L"1", szText, _countof(szText), pszFileName);
    BOOL bAddThickFrame = _wtoi(szText);
    ::CheckDlgButton(hwnd, chx2, (bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));
    ::EnableWindow(GetDlgItem(hwnd, edt7), bAddThickFrame);

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
    StringCbPrintfW(szText2, sizeof(szText2), L"%d", XG_DEF_CELL_CHAR_SIZE);
    GetPrivateProfileStringW(L"Looks", L"CellCharPercents", szText2, szText, _countof(szText), pszFileName);
    BOOL nCellCharPercents = _wtoi(szText);
    ::SetDlgItemInt(hwnd, edt4, nCellCharPercents, FALSE);

    StringCbPrintfW(szText2, sizeof(szText2), L"%d", XG_DEF_SMALL_CHAR_SIZE);
    GetPrivateProfileStringW(L"Looks", L"SmallCharPercents", szText2, szText, _countof(szText), pszFileName);
    BOOL nSmallCharPercents = _wtoi(szText);
    ::SetDlgItemInt(hwnd, edt5, nSmallCharPercents, FALSE);

    // 黒マス画像。
    GetPrivateProfileStringW(L"Looks", L"BlackCellImage", L"", szText, _countof(szText), pszFileName);
    if (!szText[0])
    {
        // 黒マス画像なし。
        ComboBox_RealSetText(hCmb1, XgLoadStringDx1(IDS_NONE));
    }
    else
    {
        // 黒マス画像あり。
        ComboBox_RealSetText(hCmb1, szText);
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

    // 線の幅。
    GetPrivateProfileStringW(L"Looks", L"LineWidthInPt", XG_LINE_WIDTH_DEFAULT2, szText, _countof(szText), pszFileName);
    ::SetDlgItemTextW(hwnd, edt6, szText);

    // 外枠の幅。
    GetPrivateProfileStringW(L"Looks", L"OuterFrameInPt", XG_OUTERFRAME_DEFAULT2, szText, _countof(szText), pszFileName);
    ::SetDlgItemTextW(hwnd, edt7, szText);

    UpdateBlockPreview(hwnd);
    return TRUE;
}

// LOOKSファイルのエクスポート。
BOOL XG_SettingsDialog::DoExportLooks(HWND hwnd, LPCWSTR pszFileName)
{
    INT nValue1, nValue2;
    BOOL bTranslated;

    // 書く前にファイルを消す。
    DeleteFileW(pszFileName);
    // LOOKSファイル名をセットする。
    XgGetFileManager()->set_looks(pszFileName);

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
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_INVALIDVALUE), nullptr, MB_ICONERROR);
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
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_INVALIDVALUE), nullptr, MB_ICONERROR);
        return FALSE;
    }

    // 文字の大きさの設定。
    WritePrivateProfileStringW(L"Looks", L"CellCharPercents", XgIntToStr(nValue1), pszFileName);
    WritePrivateProfileStringW(L"Looks", L"SmallCharPercents", XgIntToStr(nValue2), pszFileName);

    // フォント名を取得する。
    WCHAR szName[LF_FACESIZE];

    // セルフォント。
    ::GetDlgItemTextW(hwnd, edt1, szName, _countof(szName));
    WritePrivateProfileStringW(L"Looks", L"CellFont", szName, pszFileName);

    // 小さい文字のフォント。
    ::GetDlgItemTextW(hwnd, edt2, szName, _countof(szName));
    WritePrivateProfileStringW(L"Looks", L"SmallFont", szName, pszFileName);

    // UIフォント。
    ::GetDlgItemTextW(hwnd, edt3, szName, _countof(szName));
    WritePrivateProfileStringW(L"Looks", L"UIFont", szName, pszFileName);

    // 黒マス画像の名前を取得。
    WCHAR szText[MAX_PATH];
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    ComboBox_RealGetText(hCmb1, szText, _countof(szText));

    // もし黒マス画像が指定されていれば
    std::wstring str = szText;
    xg_str_trim(str);
    if (str.size() && str != XgLoadStringDx1(IDS_NONE))
    {
        std::wstring converted = str;
        XgGetFileManager()->convert(converted);
        WritePrivateProfileStringW(L"Looks", L"BlackCellImage", converted.c_str(), pszFileName);

        // さらに画像ファイルをエクスポートする。
        XgGetFileManager()->save_image(str);
    }
    else
    {
        WritePrivateProfileStringW(L"Looks", L"BlackCellImage", L"", pszFileName);
    }

    // スケルトンビューか？
    BOOL bSkeltonView = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
    WritePrivateProfileStringW(L"Looks", L"SkeletonView", XgIntToStr(bSkeltonView), pszFileName);

    // 外枠をつけるか？
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
    COLORREF rgb1 = m_hwndWhite.GetColor();
    COLORREF rgb2 = m_hwndBlack.GetColor();
    COLORREF rgb3 = m_hwndMarked.GetColor();
    WritePrivateProfileStringW(L"Looks", L"WhiteCellColor", XgIntToStr(rgb1), pszFileName);
    WritePrivateProfileStringW(L"Looks", L"BlackCellColor", XgIntToStr(rgb2), pszFileName);
    WritePrivateProfileStringW(L"Looks", L"MarkedCellColor", XgIntToStr(rgb3), pszFileName);

    // 二重マス文字。
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    ComboBox_RealGetText(hCmb2, szText, _countof(szText));
    {
        str = XgBinToHex(szText, lstrlenW(szText) * sizeof(WCHAR));
        WritePrivateProfileStringW(L"Looks", L"DoubleFrameLetters", str.c_str(), pszFileName);
    }

    // 線の幅。
    ::GetDlgItemTextW(hwnd, edt6, szText, _countof(szText));
    WritePrivateProfileStringW(L"Looks", L"LineWidthInPt", szText, pszFileName);

    // 外枠の幅。
    ::GetDlgItemTextW(hwnd, edt7, szText, _countof(szText));
    WritePrivateProfileStringW(L"Looks", L"OuterFrameInPt", szText, pszFileName);

    // フラッシュ！
    return WritePrivateProfileStringW(nullptr, nullptr, nullptr, pszFileName);
}

// 設定のインポート。
BOOL XG_SettingsDialog::OnImportLooks(HWND hwnd)
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
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
BOOL XG_SettingsDialog::OnExportLooks(HWND hwnd)
{
    WCHAR szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { OPENFILENAME_SIZE_VERSION_400W, hwnd };
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
void XG_SettingsDialog::OnResetLooks(HWND hwnd)
{
    // 色。
    m_hwndWhite.SetColor(_wtoi(XG_WHITE_COLOR_DEFAULT));
    InvalidateRect(m_hwndWhite, nullptr, TRUE);

    m_hwndBlack.SetColor(_wtoi(XG_BLACK_COLOR_DEFAULT));
    InvalidateRect(m_hwndBlack, nullptr, TRUE);

    m_hwndMarked.SetColor(_wtoi(XG_MARKED_COLOR_DEFAULT));
    InvalidateRect(m_hwndMarked, nullptr, TRUE);

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
    BOOL nCellCharPercents = XG_DEF_CELL_CHAR_SIZE;
    ::SetDlgItemInt(hwnd, edt4, nCellCharPercents, FALSE);

    BOOL nSmallCharPercents = XG_DEF_SMALL_CHAR_SIZE;
    ::SetDlgItemInt(hwnd, edt5, nSmallCharPercents, FALSE);

    // 黒マス画像。
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    // 黒マス画像なし。
    ComboBox_SetCurSel(hCmb1, ComboBox_FindStringExact(hCmb1, -1, XgLoadStringDx1(IDS_NONE)));
    ComboBox_SetText(hCmb1, XgLoadStringDx1(IDS_NONE));

    // 二重マス文字。
    HWND hCmb2 = GetDlgItem(hwnd, cmb2);
    ComboBox_SetText(hCmb2, XgLoadStringDx1(IDS_DBLFRAME_LETTERS_1));

    // 線の幅。
    ::SetDlgItemTextW(hwnd, edt6, XG_LINE_WIDTH_DEFAULT2);

    // 外枠の幅。
    ::SetDlgItemTextW(hwnd, edt7, XG_OUTERFRAME_DEFAULT2);
    ::EnableWindow(GetDlgItem(hwnd, edt7), TRUE);

    UpdateBlockPreview(hwnd);
}

// UIフォントの論理オブジェクトを設定する。
void XG_SettingsDialog::SetUIFont(HWND hwnd, const LOGFONTW *plf)
{
    if (plf == nullptr) {
        SetDlgItemTextW(hwnd, edt3, nullptr);
        return;
    }

    HDC hdc = ::CreateCompatibleDC(nullptr);
    int point_size = -MulDiv(plf->lfHeight, 72, ::GetDeviceCaps(hdc, LOGPIXELSY));
    ::DeleteDC(hdc);

    WCHAR szData[128];
    StringCbPrintf(szData, sizeof(szData), L"%s, %dpt", plf->lfFaceName, point_size);
    ::SetDlgItemTextW(hwnd, edt3, szData);
}

// [設定]ダイアログで[変更...]ボタンを押された。
void XG_SettingsDialog::OnChange(HWND hwnd, int i)
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

    default:
        break;
    }
}

// [設定]ダイアログのオーナードロー。
void XG_SettingsDialog::OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPDRAWITEMSTRUCT pdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
    if (pdis->hwndItem == m_hwndWhite) {
        m_hwndWhite.OnOwnerDrawItem(pdis);
        return;
    }
    if (pdis->hwndItem == m_hwndBlack) {
        m_hwndBlack.OnOwnerDrawItem(pdis);
        return;
    }
    if (pdis->hwndItem == m_hwndMarked) {
        m_hwndMarked.OnOwnerDrawItem(pdis);
        return;
    }
}

// ファイルがドロップされた？
void XG_SettingsDialog::OnDropFiles(HWND hwnd, HDROP hdrop)
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
INT_PTR CALLBACK
XG_SettingsDialog::DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        return OnInitDialog(hwnd);

    case WM_DRAWITEM:
        OnDrawItem(hwnd, wParam, lParam);
        break;

    case WM_DROPFILES:
        OnDropFiles(hwnd, reinterpret_cast<HDROP>(wParam));
        break;

    case WM_NOTIFY:
        {
            auto pUpDown = reinterpret_cast<NM_UPDOWN *>(lParam);
            WCHAR szText[MAX_PATH];
            if (pUpDown->hdr.code == UDN_DELTAPOS)
            {
                if (pUpDown->hdr.idFrom == scr3)
                {
                    GetDlgItemTextW(hwnd, edt6, szText, _countof(szText));
                    std::wstring str = szText;
                    xg_str_trim(str);
                    float value = wcstof(str.c_str(), nullptr);
                    if (pUpDown->iDelta < 0)
                        value += XG_LINE_WIDTH_DELTA;
                    if (pUpDown->iDelta > 0)
                        value -= XG_LINE_WIDTH_DELTA;
                    if (value > XG_MAX_LINEWIDTH)
                        value = XG_MAX_LINEWIDTH;
                    if (value < XG_MIN_LINEWIDTH)
                        value = XG_MIN_LINEWIDTH;
                    StringCchPrintfW(szText, _countof(szText), XG_LINE_WIDTH_FORMAT, value);
                    SetDlgItemTextW(hwnd, edt6, szText);
                    return TRUE;
                }
                if (pUpDown->hdr.idFrom == scr4)
                {
                    GetDlgItemTextW(hwnd, edt7, szText, _countof(szText));
                    std::wstring str = szText;
                    xg_str_trim(str);
                    float value = wcstof(str.c_str(), nullptr);
                    if (pUpDown->iDelta < 0)
                        value += XG_OUTERFRAME_DELTA;
                    if (pUpDown->iDelta > 0)
                        value -= XG_OUTERFRAME_DELTA;
                    if (value > XG_MAX_OUTERFRAME)
                        value = XG_MAX_OUTERFRAME;
                    if (value < XG_MIN_OUTERFRAME)
                        value = XG_MIN_OUTERFRAME;
                    StringCchPrintfW(szText, _countof(szText), XG_OUTERFRAME_FORMAT, value);
                    SetDlgItemTextW(hwnd, edt7, szText);
                    return TRUE;
                }
            }
        }
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
            ::SetDlgItemTextW(hwnd, edt1, L"");
            break;

        case psh4:
            ::SetDlgItemTextW(hwnd, edt2, L"");
            break;

        case psh5:
            OnChange(hwnd, 2);
            break;

        case psh6:
            ::SetDlgItemTextW(hwnd, edt3, L"");
            break;

        case psh7:
            m_hwndWhite.DoChooseColor();
            break;

        case psh8:
            m_hwndBlack.DoChooseColor();
            break;

        case psh9:
            m_hwndMarked.DoChooseColor();
            break;

        case psh10:
            SetDlgItemInt(hwnd, edt4, XG_DEF_CELL_CHAR_SIZE, FALSE);
            break;

        case psh11:
            SetDlgItemInt(hwnd, edt5, XG_DEF_SMALL_CHAR_SIZE, FALSE);
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
            break;

        case chx2:
            if (IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                ::EnableWindow(::GetDlgItem(hwnd, edt7), TRUE);
            } else {
                ::EnableWindow(::GetDlgItem(hwnd, edt7), FALSE);
            }

        default:
            break;
        }
        break;

    default:
        break;
    }
    return 0;
}

// BLOCKのプレビュー。
void XG_SettingsDialog::UpdateBlockPreview(HWND hwnd)
{
    HWND hIco1 = GetDlgItem(hwnd, ico1);
    HWND hIco2 = GetDlgItem(hwnd, ico2);
    SetWindowPos(hIco1, nullptr, 0, 0, 32, 32, SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);
    SetWindowPos(hIco2, nullptr, 0, 0, 32, 32, SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);
    auto hbmOld = reinterpret_cast<HBITMAP>(::SendMessageW(hIco1, STM_GETIMAGE, IMAGE_BITMAP, 0));
    auto hOldEMF = reinterpret_cast<HENHMETAFILE>(SendMessageW(hIco2, STM_GETIMAGE, IMAGE_ENHMETAFILE, 0));

    WCHAR szText[MAX_PATH];
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    ComboBox_RealGetText(hCmb1, szText, _countof(szText));
    std::wstring path = szText;
    xg_str_trim(path);

    HBITMAP hbm1 = nullptr;
    HENHMETAFILE hEMF1 = nullptr;
    if (XgGetFileManager()->load_block_image(szText, hbm1, hEMF1))
    {
        if (!m_bUpdating)
        {
            m_bUpdating = TRUE;
            CheckDlgButton(hwnd, chx1, BST_UNCHECKED);
            m_bUpdating = FALSE;
        }

        if (hbm1)
        {
            HBITMAP hbm2 = reinterpret_cast<HBITMAP>(::CopyImage(hbm1, IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION));
            DeleteObject(hbm1);
            SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hbm2));
            SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, 0);
            ShowWindow(hIco1, SW_SHOWNOACTIVATE);
            DeleteObject(hbmOld);
            DeleteEnhMetaFile(hOldEMF);
            return;
        }
        if (hEMF1)
        {
            SendMessageW(hIco1, STM_SETIMAGE, IMAGE_BITMAP, 0);
            SendMessageW(hIco2, STM_SETIMAGE, IMAGE_ENHMETAFILE, reinterpret_cast<LPARAM>(hEMF1));
            ShowWindow(hIco2, SW_SHOWNOACTIVATE);
            DeleteObject(hbmOld);
            DeleteEnhMetaFile(hOldEMF);
            return;
        }
    }

    SendDlgItemMessageW(hwnd, ico1, STM_SETIMAGE, IMAGE_BITMAP, 0);
    SendDlgItemMessageW(hwnd, ico2, STM_SETIMAGE, IMAGE_ENHMETAFILE, 0);
    DeleteObject(hbmOld);
    DeleteEnhMetaFile(hOldEMF);
}
