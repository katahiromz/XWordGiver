//////////////////////////////////////////////////////////////////////////////
// GUI.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#define NOMINMAX
#include "XWordGiver.hpp"
#include "layout.h"

// クロスワードのサイズの制限。
#define XG_MIN_SIZE         3
#define XG_MAX_SIZE         22

#ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL 0x020E
#endif

// 辞書の最大数。
#define MAX_DICTS 16

#undef HANDLE_WM_MOUSEWHEEL     // might be wrong
#define HANDLE_WM_MOUSEWHEEL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), \
        (int)(short)HIWORD(wParam), (UINT)(short)LOWORD(wParam)), 0)

#undef FORWARD_WM_MOUSEWHEEL    // might be wrong
#define FORWARD_WM_MOUSEWHEEL(hwnd, xPos, yPos, zDelta, fwKeys, fn) \
    (void)(fn)((hwnd), WM_MOUSEWHEEL, MAKEWPARAM((fwKeys),(zDelta)), MAKELPARAM((xPos),(yPos)))

void __fastcall MainWnd_OnChar(HWND hwnd, TCHAR ch, int cRepeat);
void __fastcall MainWnd_OnKey(HWND hwnd, UINT vk, bool fDown, int /*cRepeat*/, UINT /*flags*/);
void __fastcall MainWnd_OnImeChar(HWND hwnd, WCHAR ch, LPARAM /*lKeyData*/);

// 「黒マスパターン」ダイアログの位置とサイズ。
INT xg_nPatWndX = CW_USEDEFAULT;
INT xg_nPatWndY = CW_USEDEFAULT;
INT xg_nPatWndCX = CW_USEDEFAULT;
INT xg_nPatWndCY = CW_USEDEFAULT;

//////////////////////////////////////////////////////////////////////////////
// global variables

// 入力モード。
XG_InputMode    xg_imode = xg_im_KANA;

// インスタンスのハンドル。
HINSTANCE       xg_hInstance = nullptr;

// メインウィンドウのハンドル。
HWND            xg_hMainWnd = nullptr;

// ヒントウィンドウのハンドル。
HWND            xg_hHintsWnd = nullptr;

// 候補ウィンドウのハンドル。
HWND            xg_hCandsWnd = nullptr;

// 入力パレット。
HWND            xg_hwndInputPalette = nullptr;

// スクロールバー。
HWND xg_hVScrollBar          = nullptr;
HWND xg_hHScrollBar          = nullptr;
HWND xg_hSizeGrip            = nullptr;

// ツールバーのハンドル。
HWND            xg_hToolBar  = nullptr;

// ステータスバーのハンドル
HWND            xg_hStatusBar  = nullptr;

// ツールバーのイメージリスト。
HIMAGELIST      xg_hImageList     = nullptr;
HIMAGELIST      xg_hGrayedImageList = nullptr;

// 辞書ファイルの場所（パス）。
std::wstring xg_dict_name;
std::deque<std::wstring>  xg_dict_files;

// ヒントに追加があったか？
bool            xg_bHintsAdded = false;

// JSONファイルとして保存するか？
bool            xg_bSaveAsJsonFile = true;

// 太枠をつけるか？
bool            xg_bAddThickFrame = true;

// マスのフォント。
WCHAR xg_szCellFont[LF_FACESIZE] = L"";

// 小さな文字のフォント。
WCHAR xg_szSmallFont[LF_FACESIZE] = L"";

// UIフォント。
WCHAR xg_szUIFont[LF_FACESIZE] = L"";

// 「元に戻す」ためのバッファ。
XG_UndoBuffer                        xg_ubUndoBuffer;

// 直前に押したキーを覚えておく。
WCHAR xg_prev_vk = 0;

// 「入力パレット」縦置き？
bool xg_bTateOki = true;

// 表示用に描画するか？（XgGetXWordExtentとXgDrawXWordとXgCreateXWordImageで使う）。
INT xg_nForDisplay = 0;

// ズーム比率(%)。
INT xg_nZoomRate = 100;

// スケルトンモードか？
BOOL xg_bSkeltonMode = FALSE;

// 番号を表示するか？
BOOL xg_bShowNumbering = TRUE;
// キャレットを表示するか？
BOOL xg_bShowCaret = TRUE;

//////////////////////////////////////////////////////////////////////////////
// static variables

// 保存先のパスのリスト。
static std::deque<std::wstring>  s_dirs_save_to;

// メインウィンドウの位置とサイズ。
static int s_nMainWndX = CW_USEDEFAULT, s_nMainWndY = CW_USEDEFAULT;
static int s_nMainWndCX = CW_USEDEFAULT, s_nMainWndCY = CW_USEDEFAULT;

// ヒントウィンドウの位置とサイズ。
static int s_nHintsWndX = CW_USEDEFAULT, s_nHintsWndY = CW_USEDEFAULT;
static int s_nHintsWndCX = CW_USEDEFAULT, s_nHintsWndCY = CW_USEDEFAULT;

// 候補ウィンドウの位置とサイズ。
static int s_nCandsWndX = CW_USEDEFAULT, s_nCandsWndY = CW_USEDEFAULT;
static int s_nCandsWndCX = CW_USEDEFAULT, s_nCandsWndCY = CW_USEDEFAULT;

// 入力パレットの位置。
INT xg_nInputPaletteWndX = CW_USEDEFAULT;
INT xg_nInputPaletteWndY = CW_USEDEFAULT;

// ひらがな表示か？
BOOL xg_bHiragana = FALSE;
// Lowercase表示か？
BOOL xg_bLowercase = FALSE;

// 会社名。
static const LPCWSTR
    s_pszSoftwareCompanyName = L"Software\\Katayama Hirofumi MZ";

// アプリ名。
#ifdef _WIN64
    static const LPCWSTR s_pszAppName = L"XWord64";
#else
    static const LPCWSTR s_pszAppName = L"XWord32";
#endif

// 会社名とアプリ名。
#ifdef _WIN64
    static const LPCWSTR
        s_pszSoftwareCompanyAndApp = L"Software\\Katayama Hirofumi MZ\\XWord64";
#else
    static const LPCWSTR
        s_pszSoftwareCompanyAndApp = L"Software\\Katayama Hirofumi MZ\\XWord32";
#endif

// 再計算するか？
static bool s_bAutoRetry = true;

// 古いパソコンであることを通知したか？
static bool s_bOldNotice = false;

// 辞書ファイルの保存モード（0: 確認する、1:自動的に保存する、2:保存しない）。
static int s_nDictSaveMode = 2;

// ショートカットの拡張子。
static const LPCWSTR s_szShellLinkDotExt = L".LNK";

// メインウィンドウクラス名。
static const LPCWSTR s_pszMainWndClass = L"XWord Giver Main Window";

// ヒントウィンドウクラス名。
static const LPCWSTR s_pszHintsWndClass = L"XWord Giver Hints Window";

// 候補ウィンドウクラス名。
static const LPCWSTR s_pszCandsWndClass = L"XWord Giver Candidates Window";

// アクセラレータのハンドル。
static HACCEL       s_hAccel = nullptr;

// プロセッサの数。
static DWORD        s_dwNumberOfProcessors = 1;

// 計算時間測定用。
static DWORD        s_dwTick0;    // 開始時間。
static DWORD        s_dwTick1;    // 再計算時間。
static DWORD        s_dwTick2;    // 終了時間。
static DWORD        s_dwWait;     // 待ち時間。

// ディスク容量が足りないか？
static bool         s_bOutOfDiskSpace = false;

// 連続生成の場合、問題を生成する数。
static int          s_nNumberToGenerate = 16;

// 連続生成の場合、問題を生成した数。
static int          s_nNumberGenerated = 0;

// 再計算の回数。
static LONG         s_nRetryCount;

// ツールバーを表示するか？
static bool         s_bShowToolBar = true;

// ステータスバーを表示するか？
static bool         s_bShowStatusBar = true;

// サイズを指定して画像をコピーするときのサイズ。
static int          s_nImageCopyWidth = 250;
static int          s_nImageCopyHeight = 250;
static bool         s_bImageCopyByHeight = false;
static int          s_nMarksHeight = 40;

// 黒マスパターンで答えを表示する。
BOOL xg_bShowAnswerOnPattern = TRUE;

// ルール群。
INT xg_nRules = DEFAULT_RULES_JAPANESE;

//////////////////////////////////////////////////////////////////////////////
// スクロール関連。

// 水平スクロールの位置を取得する。
int __fastcall XgGetHScrollPos(void)
{
    return ::GetScrollPos(xg_hHScrollBar, SB_CTL);
}

// 垂直スクロールの位置を取得する。
int __fastcall XgGetVScrollPos(void)
{
    return ::GetScrollPos(xg_hVScrollBar, SB_CTL);
}

// 水平スクロールの位置を設定する。
int __fastcall XgSetHScrollPos(int nPos, BOOL bRedraw)
{
    return ::SetScrollPos(xg_hHScrollBar, SB_CTL, nPos, bRedraw);
}

// 垂直スクロールの位置を設定する。
int __fastcall XgSetVScrollPos(int nPos, BOOL bRedraw)
{
    return ::SetScrollPos(xg_hVScrollBar, SB_CTL, nPos, bRedraw);
}

// 水平スクロールの情報を取得する。
BOOL __fastcall XgGetHScrollInfo(LPSCROLLINFO psi)
{
    return ::GetScrollInfo(xg_hHScrollBar, SB_CTL, psi);
}

// 垂直スクロールの情報を取得する。
BOOL __fastcall XgGetVScrollInfo(LPSCROLLINFO psi)
{
    return ::GetScrollInfo(xg_hVScrollBar, SB_CTL, psi);
}

// 水平スクロールの情報を設定する。
BOOL __fastcall XgSetHScrollInfo(LPSCROLLINFO psi, BOOL bRedraw)
{
    return ::SetScrollInfo(xg_hHScrollBar, SB_CTL, psi, bRedraw);
}

// 垂直スクロールの情報を設定する。
BOOL __fastcall XgSetVScrollInfo(LPSCROLLINFO psi, BOOL bRedraw)
{
    return ::SetScrollInfo(xg_hVScrollBar, SB_CTL, psi, bRedraw);
}

// 本当のクライアント領域を計算する。
void __fastcall XgGetRealClientRect(HWND hwnd, LPRECT prcClient)
{
    MRect rc, rcClient;
    ::GetClientRect(hwnd, &rcClient);

    if (::IsWindowVisible(xg_hToolBar)) {
        ::GetWindowRect(xg_hToolBar, &rc);
        rcClient.top += rc.Height();
    }

    if (::IsWindowVisible(xg_hStatusBar)) {
        ::GetWindowRect(xg_hStatusBar, &rc);
        rcClient.bottom -= rc.Height();
    }

    rcClient.right -= ::GetSystemMetrics(SM_CXVSCROLL);
    rcClient.bottom -= ::GetSystemMetrics(SM_CYHSCROLL);

    assert(prcClient);
    *prcClient = rcClient;
}

// スクロール情報を設定する。
void __fastcall XgUpdateScrollInfo(HWND hwnd, int x, int y)
{
    SIZE siz;
    MRect rcClient;
    SCROLLINFO si;

    // クロスワードの大きさを取得する。
    ForDisplay for_display;
    XgGetXWordExtent(&siz);

    // クライアント療育を取得する。
    XgGetRealClientRect(hwnd, &rcClient);

    // 横スクロール情報を設定する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    XgGetHScrollInfo(&si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = siz.cx;
    si.nPage = rcClient.Width();
    si.nPos = x;
    XgSetHScrollInfo(&si, TRUE);

    // 縦スクロール情報を設定する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    XgGetVScrollInfo(&si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = siz.cy;
    si.nPage = rcClient.Height();
    si.nPos = y;
    XgSetVScrollInfo(&si, TRUE);
}

// キャレットが見えるように、必要ならばスクロールする。
void __fastcall XgEnsureCaretVisible(HWND hwnd)
{
    MRect rc, rcClient;
    SCROLLINFO si;
    bool bNeedRedraw = false;

    // クライアント領域を取得する。
    ::GetClientRect(hwnd, &rcClient);

    // ツールバーが見えるなら、クライアント領域を補正する。
    if (::IsWindowVisible(xg_hToolBar)) {
        ::GetWindowRect(xg_hToolBar, &rc);
        rcClient.top += rc.Height();
    }

    if (::IsWindowVisible(xg_hStatusBar)) {
        ::GetWindowRect(xg_hStatusBar, &rc);
        rcClient.bottom -= rc.Height();
    }

    INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    // キャレットの矩形を設定する。
    ::SetRect(&rc,
        static_cast<int>(xg_nMargin + xg_caret_pos.m_j * nCellSize), 
        static_cast<int>(xg_nMargin + xg_caret_pos.m_i * nCellSize), 
        static_cast<int>(xg_nMargin + (xg_caret_pos.m_j + 1) * nCellSize), 
        static_cast<int>(xg_nMargin + (xg_caret_pos.m_i + 1) * nCellSize));

    // 横スクロール情報を修正する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    XgGetHScrollInfo(&si);
    if (rc.left < si.nPos) {
        XgSetHScrollPos(rc.left, TRUE);
        bNeedRedraw = true;
    } else if (rc.right > static_cast<int>(si.nPos + si.nPage)) {
        XgSetHScrollPos(rc.right - si.nPage, TRUE);
        bNeedRedraw = true;
    }

    // 縦スクロール情報を修正する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    XgGetVScrollInfo(&si);
    if (rc.top < si.nPos) {
        XgSetVScrollPos(rc.top, TRUE);
        bNeedRedraw = true;
    } else if (rc.bottom > static_cast<int>(si.nPos + si.nPage)) {
        XgSetVScrollPos(rc.bottom - si.nPage, TRUE);
        bNeedRedraw = true;
    }

    // 変換ウィンドウの位置を設定する。
    COMPOSITIONFORM CompForm;
    CompForm.dwStyle = CFS_POINT;
    CompForm.ptCurrentPos.x = rcClient.left + rc.left - XgGetHScrollPos();
    CompForm.ptCurrentPos.y = rcClient.top + rc.top - XgGetVScrollPos();
    HIMC hIMC = ::ImmGetContext(hwnd);
    ::ImmSetCompositionWindow(hIMC, &CompForm);
    ::ImmReleaseContext(hwnd, hIMC);

    // 必要ならば再描画する。
    if (bNeedRedraw) {
        int x = XgGetHScrollPos();
        int y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
    }
}

//////////////////////////////////////////////////////////////////////////////

// ツールバーのUIを更新する。
void __fastcall XgUpdateToolBarUI(HWND hwnd)
{
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_SOLVE, !xg_bSolved);
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_SOLVENOADDBLACK, !xg_bSolved);
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_SOLVEREPEATEDLY, !xg_bSolved);
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_SOLVEREPEATEDLYNOADDBLACK, !xg_bSolved);
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_PRINTANSWER, xg_bSolved);
}

//////////////////////////////////////////////////////////////////////////////

BOOL XgLoadDictsFromDir(LPWSTR pszDir)
{
    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW find;
    HANDLE hFind;

    // ファイル *.dic を列挙する。
    StringCbCopy(szPath, sizeof(szPath), pszDir);
    PathAppend(szPath, L"*.dic");
    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            StringCbCopy(szPath, sizeof(szPath), pszDir);
            PathAppend(szPath, find.cFileName);
            xg_dict_files.emplace_back(szPath);
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    // ファイル *.tsv を列挙する。
    StringCbCopy(szPath, sizeof(szPath), pszDir);
    PathAppend(szPath, L"*.tsv");
    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            StringCbCopy(szPath, sizeof(szPath), pszDir);
            PathAppend(szPath, find.cFileName);
            xg_dict_files.emplace_back(szPath);
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    return !xg_dict_files.empty();
}

// 辞書ファイルをすべて読み込む。
BOOL XgLoadDictsAll(void)
{
    xg_dict_files.clear();

    // 実行ファイルのパスを取得。
    WCHAR sz[MAX_PATH];
    ::GetModuleFileNameW(nullptr, sz, sizeof(sz));

    // 実行ファイルの近くにある.dic/.tsvファイルを列挙する。
    PathRemoveFileSpec(sz);
    PathAppend(sz, L"DICT");
    if (!XgLoadDictsFromDir(sz))
    {
        PathRemoveFileSpec(sz);
        PathRemoveFileSpec(sz);
        PathAppend(sz, L"DICT");
        if (!XgLoadDictsFromDir(sz))
        {
            PathRemoveFileSpec(sz);
            PathAppend(sz, L"DICT");
            XgLoadDictsFromDir(sz);
        }
    }

    // 読み込んだ中から見つかるか？
    bool bFound = false;
    for (auto& file : xg_dict_files)
    {
        if (lstrcmpiW(file.c_str(), xg_dict_name.c_str()) == 0)
        {
            bFound = true;
            break;
        }
    }

    if (xg_dict_name.empty() || !bFound)
    {
        LPCWSTR pszNormal = XgLoadStringDx1(IDS_NORMAL_DICT);
        LPCWSTR pszBasicDict = XgLoadStringDx2(IDS_BASICDICTDATA);
        for (auto& file : xg_dict_files)
        {
            if (file.find(pszBasicDict) != std::wstring::npos &&
                file.find(pszNormal) != std::wstring::npos &&
                PathFileExistsW(file.c_str()))
            {
                xg_dict_name = file;
                break;
            }
        }
        if (xg_dict_name.empty() && xg_dict_files.size())
        {
            xg_dict_name = xg_dict_files[0];
        }
    }

    // ファイルが実際に存在するかチェックし、存在しない項目は消す。
    for (size_t i = 0; i < xg_dict_files.size(); ++i) {
        auto& file = xg_dict_files[i];
        if (!PathFileExistsW(file.c_str())) {
            xg_dict_files.erase(xg_dict_files.begin() + i);
            --i;
        }
    }

    return !xg_dict_files.empty();
}

// 設定を読み込む。
bool __fastcall XgLoadSettings(void)
{
    int i, nDirCount = 0;
    WCHAR sz[MAX_PATH];
    WCHAR szFormat[32];
    DWORD dwValue;

    // 初期化する。
    s_nMainWndX = CW_USEDEFAULT;
    s_nMainWndY = CW_USEDEFAULT;
    s_nMainWndCX = 475;
    s_nMainWndCY = 515;

    s_nHintsWndX = CW_USEDEFAULT;
    s_nHintsWndY = CW_USEDEFAULT;
    s_nHintsWndCX = 420;
    s_nHintsWndCY = 250;

    s_nCandsWndX = CW_USEDEFAULT;
    s_nCandsWndY = CW_USEDEFAULT;
    s_nCandsWndCX = 420;
    s_nCandsWndCY = 250;

    xg_nInputPaletteWndX = CW_USEDEFAULT;
    xg_nInputPaletteWndY = CW_USEDEFAULT;

    xg_bTateInput = false;
    xg_dict_name.clear();
    s_dirs_save_to.clear();
    s_bAutoRetry = true;
    xg_nRows = xg_nCols = 7;
    xg_szCellFont[0] = 0;
    xg_szSmallFont[0] = 0;
    xg_szUIFont[0] = 0;
    s_nDictSaveMode = 2;
    s_bShowToolBar = true;
    s_bShowStatusBar = true;
    xg_bShowInputPalette = false;
    xg_bSaveAsJsonFile = true;
    s_nImageCopyWidth = 250;
    s_nImageCopyHeight = 250;
    s_bImageCopyByHeight = false;
    s_nMarksHeight = 40;
    xg_bAddThickFrame = true;
    xg_bTateOki = true;
    xg_bCharFeed = true;
    xg_rgbWhiteCellColor = RGB(255, 255, 255);
    xg_rgbBlackCellColor = RGB(0x33, 0x33, 0x33);
    xg_rgbMarkedCellColor = RGB(255, 255, 255);
    xg_bDrawFrameForMarkedCell = TRUE;
    xg_bSmartResolution = TRUE;
    xg_imode = xg_im_KANA;
    xg_nZoomRate = 100;
    xg_bSkeltonMode = FALSE;
    xg_bShowNumbering = TRUE;
    xg_bShowCaret = TRUE;

    xg_bHiragana = FALSE;
    xg_bLowercase = FALSE;

    xg_nCellCharPercents = DEF_CELL_CHAR_SIZE;
    xg_nSmallCharPercents = DEF_SMALL_CHAR_SIZE;

    xg_strBlackCellImage.clear();

    xg_nPatWndX = CW_USEDEFAULT;
    xg_nPatWndY = CW_USEDEFAULT;
    xg_nPatWndCX = CW_USEDEFAULT;
    xg_nPatWndCY = CW_USEDEFAULT;
    xg_bShowAnswerOnPattern = TRUE;

    if (XgIsUserJapanese())
        xg_nRules = DEFAULT_RULES_JAPANESE;
    else
        xg_nRules = DEFAULT_RULES_ENGLISH;

    s_nNumberToGenerate = 16;

    // 会社名キーを開く。
    MRegKey company_key(HKEY_CURRENT_USER, s_pszSoftwareCompanyName, FALSE);
    if (company_key) {
        // アプリ名キーを開く。
        MRegKey app_key(company_key, s_pszAppName, FALSE);
        if (app_key) {
            if (!app_key.QueryDword(L"WindowX", dwValue)) {
                s_nMainWndX = dwValue;
            }
            if (!app_key.QueryDword(L"WindowY", dwValue)) {
                s_nMainWndY = dwValue;
            }
            if (!app_key.QueryDword(L"WindowCX", dwValue)) {
                s_nMainWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"WindowCY", dwValue)) {
                s_nMainWndCY = dwValue;
            }

            if (!app_key.QueryDword(L"TateInput", dwValue)) {
                xg_bTateInput = !!dwValue;
            }

            if (!app_key.QueryDword(L"HintsX", dwValue)) {
                s_nHintsWndX = dwValue;
            }
            if (!app_key.QueryDword(L"HintsY", dwValue)) {
                s_nHintsWndY = dwValue;
            }
            if (!app_key.QueryDword(L"HintsCX", dwValue)) {
                s_nHintsWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"HintsCY", dwValue)) {
                s_nHintsWndCY = dwValue;
            }

            if (!app_key.QueryDword(L"CandsX", dwValue)) {
                s_nCandsWndX = dwValue;
            }
            if (!app_key.QueryDword(L"CandsY", dwValue)) {
                s_nCandsWndY = dwValue;
            }
            if (!app_key.QueryDword(L"CandsCX", dwValue)) {
                s_nCandsWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"CandsCY", dwValue)) {
                s_nCandsWndCY = dwValue;
            }

            if (!app_key.QueryDword(L"IPaletteX", dwValue)) {
                xg_nInputPaletteWndX = dwValue;
            }
            if (!app_key.QueryDword(L"IPaletteY", dwValue)) {
                xg_nInputPaletteWndY = dwValue;
            }

            if (!app_key.QueryDword(L"OldNotice", dwValue)) {
                s_bOldNotice = !!dwValue;
            }
            if (!app_key.QueryDword(L"AutoRetry", dwValue)) {
                s_bAutoRetry = !!dwValue;
            }
            if (!app_key.QueryDword(L"Rows", dwValue)) {
                xg_nRows = dwValue;
            }
            if (!app_key.QueryDword(L"Cols", dwValue)) {
                xg_nCols = dwValue;
            }

            if (!app_key.QuerySz(L"CellFont", sz, ARRAYSIZE(sz))) {
                StringCbCopy(xg_szCellFont, sizeof(xg_szCellFont), sz);
            }
            if (!app_key.QuerySz(L"SmallFont", sz, ARRAYSIZE(sz))) {
                StringCbCopy(xg_szSmallFont, sizeof(xg_szSmallFont), sz);
            }
            if (!app_key.QuerySz(L"UIFont", sz, ARRAYSIZE(sz))) {
                StringCbCopy(xg_szUIFont, sizeof(xg_szUIFont), sz);
            }

            if (!app_key.QueryDword(L"ShowToolBar", dwValue)) {
                s_bShowToolBar = !!dwValue;
            }
            if (!app_key.QueryDword(L"ShowStatusBar", dwValue)) {
                s_bShowStatusBar = !!dwValue;
            }
            if (!app_key.QueryDword(L"ShowInputPalette", dwValue)) {
                xg_bShowInputPalette = !!dwValue;
            }

            xg_bSaveAsJsonFile = true;

            if (!app_key.QueryDword(L"NumberToGenerate", dwValue)) {
                s_nNumberToGenerate = dwValue;
            }
            if (!app_key.QueryDword(L"ImageCopyWidth", dwValue)) {
                s_nImageCopyWidth = dwValue;
            }
            if (!app_key.QueryDword(L"ImageCopyHeight", dwValue)) {
                s_nImageCopyHeight = dwValue;
            }
            if (!app_key.QueryDword(L"ImageCopyByHeight", dwValue)) {
                s_bImageCopyByHeight = dwValue;
            }
            if (!app_key.QueryDword(L"MarksHeight", dwValue)) {
                s_nMarksHeight = dwValue;
            }
            if (!app_key.QueryDword(L"AddThickFrame", dwValue)) {
                xg_bAddThickFrame = !!dwValue;
            }

            if (!app_key.QueryDword(L"CharFeed", dwValue)) {
                xg_bCharFeed = !!dwValue;
            }

            if (!app_key.QueryDword(L"TateOki", dwValue)) {
                xg_bTateOki = !!dwValue;
            }
            if (!app_key.QueryDword(L"WhiteCellColor", dwValue)) {
                xg_rgbWhiteCellColor = dwValue;
            }
            if (!app_key.QueryDword(L"BlackCellColor", dwValue)) {
                xg_rgbBlackCellColor = dwValue;
            }
            if (!app_key.QueryDword(L"MarkedCellColor", dwValue)) {
                xg_rgbMarkedCellColor = dwValue;
            }
            if (!app_key.QueryDword(L"DrawFrameForMarkedCell", dwValue)) {
                xg_bDrawFrameForMarkedCell = dwValue;
            }
            if (!app_key.QueryDword(L"SmartResolution", dwValue)) {
                xg_bSmartResolution = dwValue;
            }
            if (!app_key.QueryDword(L"InputMode", dwValue)) {
                xg_imode = (XG_InputMode)dwValue;
            }
            if (!app_key.QueryDword(L"ZoomRate", dwValue)) {
                xg_nZoomRate = dwValue;
            }
            if (!app_key.QueryDword(L"SkeltonMode", dwValue)) {
                xg_bSkeltonMode = dwValue;
            }
            if (!app_key.QueryDword(L"ShowNumbering", dwValue)) {
                xg_bShowNumbering = dwValue;
            }
            if (!app_key.QueryDword(L"ShowCaret", dwValue)) {
                xg_bShowCaret = dwValue;
            }

            if (!app_key.QueryDword(L"Hiragana", dwValue)) {
                xg_bHiragana = !!dwValue;
            }
            if (!app_key.QueryDword(L"Lowercase", dwValue)) {
                xg_bLowercase = !!dwValue;
            }

            if (!app_key.QueryDword(L"CellCharPercents", dwValue)) {
                xg_nCellCharPercents = dwValue;
            }
            if (!app_key.QueryDword(L"SmallCharPercents", dwValue)) {
                xg_nSmallCharPercents = dwValue;
            }

            if (!app_key.QueryDword(L"PatWndX", dwValue)) {
                xg_nPatWndX = dwValue;
            }
            if (!app_key.QueryDword(L"PatWndY", dwValue)) {
                xg_nPatWndY = dwValue;
            }
            if (!app_key.QueryDword(L"PatWndCX", dwValue)) {
                xg_nPatWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"PatWndCY", dwValue)) {
                xg_nPatWndCY = dwValue;
            }
            if (!app_key.QueryDword(L"ShowAnsOnPat", dwValue)) {
                xg_bShowAnswerOnPattern = dwValue;
            }
            if (!app_key.QueryDword(L"Rules", dwValue)) {
                xg_nRules = dwValue;
            }

            if (!app_key.QuerySz(L"Recent", sz, ARRAYSIZE(sz))) {
                xg_dict_name = sz;
                if (!PathFileExists(xg_dict_name.c_str()))
                {
                    xg_dict_name.clear();
                }
            }

            if (!app_key.QuerySz(L"BlackCellImage", sz, ARRAYSIZE(sz))) {
                xg_strBlackCellImage = sz;
                if (!PathFileExists(xg_strBlackCellImage.c_str()))
                {
                    xg_strBlackCellImage.clear();
                }
            }

            // 保存先のリストを取得する。
            if (!app_key.QueryDword(L"SaveToCount", dwValue)) {
                nDirCount = dwValue;
                for (i = 0; i < nDirCount; i++) {
                    StringCbPrintf(szFormat, sizeof(szFormat), L"SaveTo %d", i + 1);
                    if (!app_key.QuerySz(szFormat, sz, ARRAYSIZE(sz))) {
                        s_dirs_save_to.emplace_back(sz);
                    } else {
                        nDirCount = i;
                        break;
                    }
                }
            }
        }
    }

    // 保存先リストが空だったら、初期化する。
    if (nDirCount == 0 || s_dirs_save_to.empty()) {
        LPITEMIDLIST pidl;
        WCHAR szPath[MAX_PATH];
        ::SHGetSpecialFolderLocation(nullptr, CSIDL_PERSONAL, &pidl);
        ::SHGetPathFromIDListW(pidl, szPath);
        ::CoTaskMemFree(pidl);
        s_dirs_save_to.emplace_back(szPath);
    }

    ::DeleteObject(xg_hbmBlackCell);
    xg_hbmBlackCell = NULL;

    DeleteEnhMetaFile(xg_hBlackCellEMF);
    xg_hBlackCellEMF = NULL;

    if (!xg_strBlackCellImage.empty())
    {
        xg_hbmBlackCell = LoadBitmapFromFile(xg_strBlackCellImage.c_str());
        if (!xg_hbmBlackCell)
        {
            xg_hBlackCellEMF = GetEnhMetaFile(xg_strBlackCellImage.c_str());
        }

        if (!xg_hbmBlackCell && !xg_hBlackCellEMF)
            xg_strBlackCellImage.clear();
    }

    return true;
}

// 設定を保存する。
bool __fastcall XgSaveSettings(void)
{
    int i, nCount;
    WCHAR szFormat[32];

    // 会社名キーを開く。キーがなければ作成する。
    MRegKey company_key(HKEY_CURRENT_USER, s_pszSoftwareCompanyName, TRUE);
    if (company_key) {
        // アプリ名キーを開く。キーがなければ作成する。
        MRegKey app_key(company_key, s_pszAppName, TRUE);
        if (app_key) {
            app_key.SetDword(L"OldNotice", s_bOldNotice);
            app_key.SetDword(L"AutoRetry", s_bAutoRetry);
            app_key.SetDword(L"Rows", xg_nRows);
            app_key.SetDword(L"Cols", xg_nCols);

            app_key.SetSz(L"CellFont", xg_szCellFont, ARRAYSIZE(xg_szCellFont));
            app_key.SetSz(L"SmallFont", xg_szSmallFont, ARRAYSIZE(xg_szSmallFont));
            app_key.SetSz(L"UIFont", xg_szUIFont, ARRAYSIZE(xg_szUIFont));

            app_key.SetDword(L"ShowToolBar", s_bShowToolBar);
            app_key.SetDword(L"ShowStatusBar", s_bShowStatusBar);
            app_key.SetDword(L"ShowInputPalette", xg_bShowInputPalette);

            app_key.SetDword(L"SaveAsJsonFile", xg_bSaveAsJsonFile);
            app_key.SetDword(L"NumberToGenerate", s_nNumberToGenerate);
            app_key.SetDword(L"ImageCopyWidth", s_nImageCopyWidth);
            app_key.SetDword(L"ImageCopyHeight", s_nImageCopyHeight);
            app_key.SetDword(L"ImageCopyByHeight", s_bImageCopyByHeight);
            app_key.SetDword(L"MarksHeight", s_nMarksHeight);
            app_key.SetDword(L"AddThickFrame", xg_bAddThickFrame);
            app_key.SetDword(L"CharFeed", xg_bCharFeed);
            app_key.SetDword(L"TateOki", xg_bTateOki);

            app_key.SetDword(L"WhiteCellColor", xg_rgbWhiteCellColor);
            app_key.SetDword(L"BlackCellColor", xg_rgbBlackCellColor);
            app_key.SetDword(L"MarkedCellColor", xg_rgbMarkedCellColor);

            app_key.SetDword(L"DrawFrameForMarkedCell", xg_bDrawFrameForMarkedCell);
            app_key.SetDword(L"SmartResolution", xg_bSmartResolution);
            app_key.SetDword(L"InputMode", (DWORD)xg_imode);
            app_key.SetDword(L"ZoomRate", xg_nZoomRate);
            app_key.SetDword(L"SkeltonMode", xg_bSkeltonMode);
            app_key.SetDword(L"ShowNumbering", xg_bShowNumbering);
            app_key.SetDword(L"ShowCaret", xg_bShowCaret);

            app_key.SetDword(L"Hiragana", xg_bHiragana);
            app_key.SetDword(L"Lowercase", xg_bLowercase);

            app_key.SetDword(L"CellCharPercents", xg_nCellCharPercents);
            app_key.SetDword(L"SmallCharPercents", xg_nSmallCharPercents);

            app_key.SetDword(L"PatWndX", xg_nPatWndX);
            app_key.SetDword(L"PatWndY", xg_nPatWndY);
            app_key.SetDword(L"PatWndCX", xg_nPatWndCX);
            app_key.SetDword(L"PatWndCY", xg_nPatWndCY);
            app_key.SetDword(L"ShowAnsOnPat", xg_bShowAnswerOnPattern);
            app_key.SetDword(L"Rules", xg_nRules);

            app_key.SetSz(L"Recent", xg_dict_name.c_str());
            app_key.SetSz(L"BlackCellImage", xg_strBlackCellImage.c_str());

            // 保存先のリストを設定する。
            nCount = static_cast<int>(s_dirs_save_to.size());
            app_key.SetDword(L"SaveToCount", nCount);
            for (i = 0; i < nCount; i++)
            {
                StringCbPrintf(szFormat, sizeof(szFormat), L"SaveTo %d", i + 1);
                app_key.SetSz(szFormat, s_dirs_save_to[i].c_str());
            }

            app_key.SetDword(L"HintsX", s_nHintsWndX);
            app_key.SetDword(L"HintsY", s_nHintsWndY);
            app_key.SetDword(L"HintsCX", s_nHintsWndCX);
            app_key.SetDword(L"HintsCY", s_nHintsWndCY);

            app_key.SetDword(L"CandsX", s_nCandsWndX);
            app_key.SetDword(L"CandsY", s_nCandsWndY);
            app_key.SetDword(L"CandsCX", s_nCandsWndCX);
            app_key.SetDword(L"CandsCY", s_nCandsWndCY);

            app_key.SetDword(L"IPaletteX", xg_nInputPaletteWndX);
            app_key.SetDword(L"IPaletteY", xg_nInputPaletteWndY);

            app_key.SetDword(L"WindowX", s_nMainWndX);
            app_key.SetDword(L"WindowY", s_nMainWndY);
            app_key.SetDword(L"WindowCX", s_nMainWndCX);
            app_key.SetDword(L"WindowCY", s_nMainWndCY);

            app_key.SetDword(L"TateInput", xg_bTateInput);
        }
    }

    return true;
}

// 設定を消去する。
bool __fastcall XgEraseSettings(void)
{
    // 会社名キーを開く。
    RegDeleteTreeDx(HKEY_CURRENT_USER, s_pszSoftwareCompanyAndApp);

    return true;
}

//////////////////////////////////////////////////////////////////////////////

// 「ヒントの入力」ダイアログ。
extern "C"
INT_PTR CALLBACK
XgInputHintDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR sz[512];
    static std::wstring s_word;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央に寄せる。
        XgCenterDialog(hwnd);

        // ダイアログを初期化する。
        s_word = *reinterpret_cast<std::wstring *>(lParam);
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_REGISTERWORD), s_word.data(), s_word.data());
        ::SetDlgItemTextW(hwnd, stc1, sz);

        // ヒントが追加された。
        xg_bHintsAdded = true;
        return true;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // テキストを取得する。
            ::GetDlgItemTextW(hwnd, edt1, sz, ARRAYSIZE(sz));

            // 辞書データに追加する。
            xg_dict_1.emplace_back(s_word, sz);

            // 二分探索のために、並び替えておく。
            XgSortAndUniqueDictData(xg_dict_1);

            // ダイアログを終了。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを終了。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;
    }

    return false;
}

// クロスワードをチェックする。
bool __fastcall XgCheckCrossWord(HWND hwnd, bool check_words = true)
{
    // 四隅黒禁。
    if ((xg_nRules & RULE_DONTCORNERBLACK) && xg_xword.CornerBlack()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CORNERBLOCK), nullptr, MB_ICONERROR);
        return false;
    }

    // 連黒禁。
    if ((xg_nRules & RULE_DONTDOUBLEBLACK) && xg_xword.DoubleBlack()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADJACENTBLOCK), nullptr, MB_ICONERROR);
        return false;
    }

    // 三方黒禁。
    if ((xg_nRules & RULE_DONTTRIDIRECTIONS) && xg_xword.TriBlackAround()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_TRIBLOCK), nullptr, MB_ICONERROR);
        return false;
    }

    // 分断禁。
    if ((xg_nRules & RULE_DONTDIVIDE) && xg_xword.DividedByBlack()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_DIVIDED), nullptr, MB_ICONERROR);
        return false;
    }

    // 黒斜三連禁。
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (xg_xword.ThreeDiagonals()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_THREEDIAGONALS), nullptr, MB_ICONERROR);
            return false;
        }
    } else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
        // 黒斜四連禁。
        if (xg_xword.FourDiagonals()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_FOURDIAGONALS), nullptr, MB_ICONERROR);
            return false;
        }
    }

    // 黒マス点対称。
    if ((xg_nRules & RULE_POINTSYMMETRY) && !xg_xword.IsPointSymmetry()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTPOINTSYMMETRY), nullptr, MB_ICONERROR);
        return false;
    }

    // クロスワードに含まれる単語のチェック。
    XG_Pos pos;
    std::vector<std::wstring> vNotFoundWords;
    XG_EpvCode code = xg_xword.EveryPatternValid1(vNotFoundWords, pos, xg_bNoAddBlack);
    if (code == xg_epv_PATNOTMATCH) {
        if (check_words) {
            // パターンにマッチしないマスがあった。
            WCHAR sz[128];
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_NOCANDIDATE), pos.m_i + 1, pos.m_j + 1);
            XgCenterMessageBoxW(hwnd, sz, nullptr, MB_ICONERROR);
            return false;
        }
    } else if (code == xg_epv_DOUBLEWORD) {
        // すでに使用した単語があった。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_DOUBLEDWORD), nullptr, MB_ICONERROR);
        return false;
    } else if (code == xg_epv_LENGTHMISMATCH) {
        if (check_words) {
            // 登録されている単語と長さの一致しないスペースがあった。
            WCHAR sz[128];
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_TOOLONGSPACE), pos.m_i + 1, pos.m_j + 1);
            XgCenterMessageBoxW(hwnd, sz, nullptr, MB_ICONERROR);
            return false;
        }
    }

    // 見つからなかった単語があるか？
    if (!vNotFoundWords.empty()) {
        if (check_words) {
            // 単語が登録されていない。
            for (auto& word : vNotFoundWords) {
                // ヒントの入力を促す。
                if (::DialogBoxParamW(xg_hInstance, MAKEINTRESOURCE(IDD_INPUTHINT),
                                      hwnd, XgInputHintDlgProc,
                                      reinterpret_cast<LPARAM>(&word)) != IDOK)
                {
                    // キャンセルされた。
                    return false;
                }
            }
        }
    }

    // 成功。
    return true;
}

// 辞書名をセットする。
void XgSetDict(const std::wstring& strFile)
{
    // 辞書名を格納。
    xg_dict_name = strFile;

    // 辞書として追加、ソート、一意にする。
    if (xg_dict_files.size() < MAX_DICTS)
    {
        xg_dict_files.emplace_back(strFile);
        std::sort(xg_dict_files.begin(), xg_dict_files.end());
        auto last = std::unique(xg_dict_files.begin(), xg_dict_files.end());
        xg_dict_files.erase(last, xg_dict_files.end());
    }
}

//////////////////////////////////////////////////////////////////////////////

// 盤を特定の文字で埋め尽くす。
void __fastcall XgNewCells(HWND hwnd, WCHAR ch, INT nRows, INT nCols)
{
    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 初期化する。
    xg_bSolved = false;
    xg_nRows = nRows;
    xg_nCols = nCols;
    xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
    xg_bHintsAdded = false;
    xg_bShowAnswer = false;
    xg_caret_pos.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_strFileName.clear();
    xg_vMarks.clear();
    xg_vMarkedCands.clear();
    for (INT iRow = 0; iRow < xg_nRows; ++iRow) {
        for (INT jCol = 0; jCol < xg_nCols; ++jCol) {
            xg_xword.SetAt(iRow, jCol, ch);
        }
    }
    // イメージを更新する。
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);
    // 「元に戻す」情報を設定する。
    sa2->Get();
    xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
}

// 盤のサイズを変更する。
void __fastcall XgResizeCells(HWND hwnd, INT nNewRows, INT nNewCols)
{
    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // マークの更新を通知する。
    XgMarkUpdate();
    // サイズを変更する。
    INT nOldRows = xg_nRows, nOldCols = xg_nCols;
    INT iMin = std::min(xg_nRows, nNewRows);
    INT jMin = std::min(xg_nCols, nNewCols);
    XG_Board copy;
    copy.ResetAndSetSize(nNewRows, nNewCols);
    for (INT i = 0; i < iMin; ++i) {
        for (INT j = 0; j < jMin; ++j) {
            xg_nRows = nOldRows;
            xg_nCols = nOldCols;
            WCHAR ch = xg_xword.GetAt(i, j);
            xg_nRows = nNewRows;
            xg_nCols = nNewCols;
            copy.SetAt(i, j, ch);
        }
    }
    xg_nRows = nNewRows;
    xg_nCols = nNewCols;
    xg_xword = copy;
    xg_bSolved = false;
    xg_bHintsAdded = false;
    xg_bShowAnswer = false;
    xg_caret_pos.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_strFileName.clear();
    xg_vMarks.clear();
    xg_vMarkedCands.clear();
    // イメージを更新する。
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);
    // 「元に戻す」情報を設定する。
    sa2->Get();
    xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
}

// [新規作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgNewDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    INT n1, n2;
    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, xg_nCols, FALSE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl;

            hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);

            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        CheckRadioButton(hwnd, rad1, rad3, rad1);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < XG_MIN_SIZE || n1 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return 0;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < XG_MIN_SIZE || n2 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return 0;
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            // 処理を行う。
            if (IsDlgButtonChecked(hwnd, rad1) == BST_CHECKED) {
                // 盤を特定の文字で埋め尽くす。
                XgNewCells(xg_hMainWnd, ZEN_SPACE, n1, n2);
            } else if (IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED) {
                // 盤を特定の文字で埋め尽くす。
                XgNewCells(xg_hMainWnd, ZEN_BLACK, n1, n2);
            } else if (IsDlgButtonChecked(hwnd, rad3) == BST_CHECKED) {
                // 盤のサイズを変更する。
                XgResizeCells(xg_hMainWnd, n1, n2);
            }
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// [問題の作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgGenerateDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    INT n1, n2, n3;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, xg_nCols, FALSE);

        // スケルトンモードと入力モードに応じて単語の最大長を設定する。
        if (xg_imode == xg_im_KANJI) {
            n3 = 4;
        } else if (xg_bSkeltonMode) {
            n3 = 6;
        } else if (xg_imode == xg_im_RUSSIA || xg_imode == xg_im_ABC) {
            n3 = 5;
        } else if (xg_imode == xg_im_DIGITS) {
            n3 = 7;
        } else {
            n3 = 4;
        }
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);

        // 自動で再計算をするか？
        if (s_bAutoRetry)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        // スマート解決か？
        if (xg_bSmartResolution)
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        // IMEをOFFにする。
        {
            HWND hwndCtrl;

            hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);

            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(10, 4));
        if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
            EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
            EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
        } else {
            EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < XG_MIN_SIZE || n1 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return 0;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < XG_MIN_SIZE || n2 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return 0;
            }
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < 4 || n3 > 10) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return 0;
            }
            xg_nMaxWordLen = n3;
            // 自動で再計算をするか？
            s_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // スマート解決か？
            xg_bSmartResolution = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.ResetAndSetSize(n1, n2);
                xg_nRows = n1;
                xg_nCols = n2;
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case chx2:
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
                EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
                EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
            } else {
                EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
                EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
                EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
            }
            break;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// 保存先。
WCHAR xg_szDir[MAX_PATH] = L"";

// 「保存先」参照。
extern "C"
int CALLBACK XgBrowseCallbackProc(
    HWND hwnd,
    UINT uMsg,
    LPARAM /*lParam*/,
    LPARAM /*lpData*/)
{
    if (uMsg == BFFM_INITIALIZED) {
        // 初期化の際に、フォルダーの場所を指定する。
        ::SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(xg_szDir));
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// [問題の連続作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgGenerateRepeatedlyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    INT n1, n2, n3;
    WCHAR szFile[MAX_PATH];
    std::wstring strDir;
    COMBOBOXEXITEMW item;
    BROWSEINFOW bi;
    DWORD attrs;
    LPITEMIDLIST pidl;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, xg_nCols, FALSE);
        // スケルトンモードと入力モードに応じて単語の最大長を設定する。
        if (xg_imode == xg_im_KANJI) {
            n3 = 4;
        } else if (xg_bSkeltonMode) {
            n3 = 6;
        } else if (xg_imode == xg_im_RUSSIA || xg_imode == xg_im_ABC) {
            n3 = 5;
        } else if (xg_imode == xg_im_DIGITS) {
            n3 = 7;
        } else {
            n3 = 4;
        }
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);
        // スマート解決か？
        if (xg_bSmartResolution) {
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        } else {
            ::CheckDlgButton(hwnd, chx2, BST_UNCHECKED);
        }
        // 保存先を設定する。
        for (const auto& dir : s_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCbCopy(szFile, sizeof(szFile), dir.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);
        // 自動で再計算をするか？
        if (s_bAutoRetry)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        // 生成する数を設定する。
        ::SetDlgItemInt(hwnd, edt4, s_nNumberToGenerate, FALSE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);
            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
            hwndCtrl = ::GetDlgItem(hwnd, edt3);
            ::ImmAssociateContext(hwndCtrl, NULL);
            hwndCtrl = ::GetDlgItem(hwnd, edt4);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(10, 4));
        SendDlgItemMessageW(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(100, 1));
        if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
            EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
            EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
        } else {
            EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < XG_MIN_SIZE || n1 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return 0;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < XG_MIN_SIZE || n2 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return 0;
            }
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < 4 || n3 > 10) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return 0;
            }
            xg_nMaxWordLen = n3;
            // 自動で再計算をするか？
            s_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // スマート解決か？
            xg_bSmartResolution = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
            // 保存先のパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb2, szFile, ARRAYSIZE(szFile));
            attrs = ::GetFileAttributesW(szFile);
            if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                // パスがなければ作成する。
                if (!XgMakePathW(szFile)) {
                    // 作成に失敗。
                    ::SendDlgItemMessageW(hwnd, cmb2, CB_SETEDITSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_STORAGEINVALID), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, cmb2));
                    return 0;
                }
            }
            // 保存先をセットする。
            strDir = szFile;
            xg_str_trim(strDir);
            {
                auto end = s_dirs_save_to.end();
                for (auto it = s_dirs_save_to.begin(); it != end; it++) {
                    if (_wcsicmp((*it).data(), strDir.data()) == 0) {
                        s_dirs_save_to.erase(it);
                        break;
                    }
                }
                s_dirs_save_to.emplace_front(strDir);
            }
            // 問題の数を取得する。
            {
                // 無制限ではない。
                BOOL bTranslated;
                s_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt4, &bTranslated, FALSE);
                if (!bTranslated || s_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt4, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt4));
                    return 0;
                }
            }
            // JSON形式として保存するか？
            xg_bSaveAsJsonFile = true;
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.ResetAndSetSize(n1, n2);
                xg_nRows = n1;
                xg_nCols = n2;
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case chx2:
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
                EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
                EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
            } else {
                EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
                EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
                EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
            }
            break;

        case psh2:
            // ユーザーに保存先の場所を問い合わせる。
            ZeroMemory(&bi, sizeof(bi));
            bi.hwndOwner = hwnd;
            bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
            bi.ulFlags = BIF_RETURNONLYFSDIRS;
            bi.lpfn = XgBrowseCallbackProc;
            ::GetDlgItemTextW(hwnd, cmb2, xg_szDir, ARRAYSIZE(xg_szDir));
            pidl = ::SHBrowseForFolderW(&bi);
            if (pidl) {
                // パスをコンボボックスに設定。
                ::SHGetPathFromIDListW(pidl, szFile);
                ::SetDlgItemTextW(hwnd, cmb2, szFile);
                ::CoTaskMemFree(pidl);
            }
            break;
        }
    }
    return 0;
}

// [黒マスパターンの連続作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgGenerateBlacksRepeatedlyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    INT n3;
    WCHAR szFile[MAX_PATH];
    std::wstring strDir;
    COMBOBOXEXITEMW item;
    BROWSEINFOW bi;
    DWORD attrs;
    LPITEMIDLIST pidl;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // スケルトンモードと入力モードに応じて単語の最大長を設定する。
        if (xg_imode == xg_im_KANJI) {
            n3 = 4;
        } else if (xg_bSkeltonMode) {
            n3 = 6;
        } else if (xg_imode == xg_im_RUSSIA || xg_imode == xg_im_ABC) {
            n3 = 5;
        } else if (xg_imode == xg_im_DIGITS) {
            n3 = 7;
        } else {
            n3 = 4;
        }
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);
        // 保存先を設定する。
        for (const auto& dir : s_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCbCopy(szFile, sizeof(szFile), dir.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);
        // 生成する数を設定する。
        ::SetDlgItemInt(hwnd, edt4, s_nNumberToGenerate, FALSE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl = ::GetDlgItem(hwnd, edt3);
            ::ImmAssociateContext(hwndCtrl, NULL);
            hwndCtrl = ::GetDlgItem(hwnd, edt4);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(10, 4));
        SendDlgItemMessageW(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(100, 1));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < 4 || n3 > 10) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return 0;
            }
            xg_nMaxWordLen = n3;
            // 保存先のパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb2, szFile, ARRAYSIZE(szFile));
            attrs = ::GetFileAttributesW(szFile);
            if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                // パスがなければ作成する。
                if (!XgMakePathW(szFile)) {
                    // 作成に失敗。
                    ::SendDlgItemMessageW(hwnd, cmb2, CB_SETEDITSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_STORAGEINVALID), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, cmb2));
                    return 0;
                }
            }
            // 保存先をセットする。
            strDir = szFile;
            xg_str_trim(strDir);
            {
                auto end = s_dirs_save_to.end();
                for (auto it = s_dirs_save_to.begin(); it != end; it++) {
                    if (_wcsicmp((*it).data(), strDir.data()) == 0) {
                        s_dirs_save_to.erase(it);
                        break;
                    }
                }
                s_dirs_save_to.emplace_front(strDir);
            }
            // 問題の数を取得する。
            {
                // 無制限ではない。
                BOOL bTranslated;
                s_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt4, &bTranslated, FALSE);
                if (!bTranslated || s_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt4, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt4));
                    return 0;
                }
            }
            // JSON形式として保存するか？
            xg_bSaveAsJsonFile = true;
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.clear();
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh2:
            // ユーザーに保存先の場所を問い合わせる。
            ZeroMemory(&bi, sizeof(bi));
            bi.hwndOwner = hwnd;
            bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
            bi.ulFlags = BIF_RETURNONLYFSDIRS;
            bi.lpfn = XgBrowseCallbackProc;
            ::GetDlgItemTextW(hwnd, cmb2, xg_szDir, ARRAYSIZE(xg_szDir));
            pidl = ::SHBrowseForFolderW(&bi);
            if (pidl) {
                // パスをコンボボックスに設定。
                ::SHGetPathFromIDListW(pidl, szFile);
                ::SetDlgItemTextW(hwnd, cmb2, szFile);
                ::CoTaskMemFree(pidl);
            }
            break;
        }
    }
    return 0;
}

// [黒マスパターンの作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgGenerateBlacksDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    INT n3;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // スケルトンモードと入力モードに応じて単語の最大長を設定する。
        if (xg_imode == xg_im_KANJI) {
            n3 = 4;
        } else if (xg_bSkeltonMode) {
            n3 = 6;
        } else if (xg_imode == xg_im_RUSSIA || xg_imode == xg_im_ABC) {
            n3 = 5;
        } else if (xg_imode == xg_im_DIGITS) {
            n3 = 7;
        } else {
            n3 = 4;
        }
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(10, 4));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < 4 || n3 > 10) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return 0;
            }
            xg_nMaxWordLen = n3;
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.clear();
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        }
    }
    return 0;
}

// [解の連続作成]ダイアログのダイアログ プロシージャー。
extern "C" INT_PTR CALLBACK
XgSolveRepeatedlyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    int i;
    WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
    std::wstring strFile, strDir;
    COMBOBOXEXITEMW item;
    HDROP hDrop;
    BROWSEINFOW bi;
    DWORD attrs;
    LPITEMIDLIST pidl;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 保存先を設定する。
        for (const auto& dir : s_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCbCopy(szFile, sizeof(szFile), dir.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // 自動で再計算をするか？
        if (s_bAutoRetry) {
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        }
        // 無制限か？
        // 生成する数を設定する。
        ::SetDlgItemInt(hwnd, edt1, s_nNumberToGenerate, FALSE);
        // ファイルドロップを有効にする。
        ::DragAcceptFiles(hwnd, TRUE);
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(100, 2));
        return TRUE;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile, ARRAYSIZE(szFile));
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile, szTarget)) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            StringCbCopy(szFile, sizeof(szFile), szTarget);
        }

        // ファイルの属性を確認する。
        attrs = ::GetFileAttributesW(szFile);
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            // ディレクトリーだった。
            // 同じ項目がすでにあれば、削除する。
            i = static_cast<int>(::SendDlgItemMessageW(
                hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                reinterpret_cast<LPARAM>(szFile)));
            if (i != CB_ERR) {
                ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
            }
            // コンボボックスの最初に挿入する。
            item.mask = CBEIF_TEXT;
            item.iItem = 0;
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                reinterpret_cast<LPARAM>(&item));
            // コンボボックスの最初の項目を選択する。
            ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        } else {
            // ディレクトリーではなかった。
            ::MessageBeep(0xFFFFFFFF);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // 保存先のパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb1, szFile, ARRAYSIZE(szFile));
            attrs = ::GetFileAttributesW(szFile);
            if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                // パスがなければ作成する。
                if (!XgMakePathW(szFile)) {
                    // 作成に失敗。
                    ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_STORAGEINVALID), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, cmb1));
                    return 0;
                }
            }
            // 保存先をセットする。
            strDir = szFile;
            xg_str_trim(strDir);
            {
                auto end = s_dirs_save_to.end();
                for (auto it = s_dirs_save_to.begin(); it != end; it++) {
                    if (_wcsicmp((*it).data(), strDir.data()) == 0) {
                        s_dirs_save_to.erase(it);
                        break;
                    }
                }
                s_dirs_save_to.emplace_front(strDir);
            }
            // 自動で再計算をするか？
            s_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // 問題の数を取得する。
            {
                BOOL bTranslated;
                s_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
                if (!bTranslated || s_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt1));
                    return 0;
                }
            }
            // JSON形式で保存するか？
            xg_bSaveAsJsonFile = true;
            // 初期化する。
            xg_bSolved = false;
            xg_bShowAnswer = false;
            xg_vTateInfo.clear();
            xg_vYokoInfo.clear();
            xg_vMarks.clear();
            xg_vMarkedCands.clear();
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh1:
            // ユーザーに保存先を問い合わせる。
            ZeroMemory(&bi, sizeof(bi));
            bi.hwndOwner = hwnd;
            bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
            bi.ulFlags = BIF_RETURNONLYFSDIRS;
            bi.lpfn = XgBrowseCallbackProc;
            ::GetDlgItemTextW(hwnd, cmb1, xg_szDir, ARRAYSIZE(xg_szDir));
            pidl = ::SHBrowseForFolderW(&bi);
            if (pidl) {
                // コンボボックスにパスを設定する。
                ::SHGetPathFromIDListW(pidl, szFile);
                ::SetDlgItemTextW(hwnd, cmb1, szFile);
                ::CoTaskMemFree(pidl);
            }
            break;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// ヒントの内容をメモ帳で開く。
bool __fastcall XgOpenHintsByNotepad(HWND /*hwnd*/, bool bShowAnswer)
{
    WCHAR szPath[MAX_PATH];
    WCHAR szCmdLine[MAX_PATH * 2];
    std::wstring str;
    DWORD cb;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;

    // 解がないときは、ヒントはない。
    if (!xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

    // 一時ファイルを作成する。
    ::GetTempPathW(MAX_PATH, szPath);
    StringCbCat(szPath, sizeof(szPath), XgLoadStringDx1(IDS_HINTSTXT));
    HANDLE hFile = ::CreateFileW(szPath, GENERIC_WRITE, FILE_SHARE_READ,
        nullptr, CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    // BOMとヒントの文字列をファイルに書き込む。
    str = reinterpret_cast<LPCWSTR>("\xFF\xFE\x00");
    str += xg_pszNewLine;
    xg_solution.GetHintsStr(xg_strHints, 2, true);
    str += xg_strHints;
    cb = static_cast<DWORD>(str.size() * sizeof(WCHAR));
    if (::WriteFile(hFile, str.data(), cb, &cb, nullptr)) {
        // ファイルを閉じる。
        ::CloseHandle(hFile);

        // メモ帳でファイルを開く。
        StringCbPrintf(szCmdLine, sizeof(szCmdLine),
                       XgLoadStringDx1(IDS_NOTEPAD), szPath);
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        if (::CreateProcessW(nullptr, szCmdLine, nullptr, nullptr, FALSE,
                             0, nullptr, nullptr, &si, &pi))
        {
            // メモ帳が待ち状態になるまで待つ。
            if (::WaitForInputIdle(pi.hProcess, 5 * 1000) != WAIT_TIMEOUT) {
                // 0.2秒待つ。
                ::Sleep(200);
                // ファイルを削除する。
                ::DeleteFileW(szPath);
            }
            // ハンドルを閉じる。
            ::CloseHandle(pi.hProcess);
            ::CloseHandle(pi.hThread);
            return true;
        }
    }
    // ファイルを閉じる。
    ::CloseHandle(hFile);
    return false;
}

// ヒントの内容をヒントウィンドウで開く。
bool __fastcall XgOpenHintsByWindow(HWND hwnd)
{
    // もしヒントウィンドウが存在すれば破棄する。
    if (xg_hHintsWnd) {
        HWND hwnd = xg_hHintsWnd;
        xg_hHintsWnd = NULL;
        ::DestroyWindow(hwnd);
    }

    // ヒントウィンドウを作成する。
    if (XgCreateHintsWnd(xg_hMainWnd)) {
        ::ShowWindow(xg_hHintsWnd, SW_SHOWNOACTIVATE);
        ::SetForegroundWindow(xg_hMainWnd);
        return true;
    }
    return false;
}

// ヒントを更新する。
void __fastcall XgUpdateHints(HWND hwnd)
{
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_strHints.clear();
    if (xg_bSolved) {
        xg_solution.GetHintsStr(xg_strHints, 2, true);
        if (!XgParseHintsStr(hwnd, xg_strHints)) {
            xg_strHints.clear();
        }
    }
}

// ヒントを表示する。
void __fastcall XgShowHints(HWND hwnd)
{
    #if 1
        XgOpenHintsByWindow(hwnd);
    #else
        XgOpenHintsByNotepad(hwnd, xg_bShowAnswer);
    #endif
}

// スレッドを閉じる。
void __fastcall XgCloseThreads(void)
{
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        ::CloseHandle(xg_ahThreads[i]);
        xg_ahThreads[i] = nullptr;
    }
}

// スレッドを待つ。
inline void __fastcall XgWaitForThreads(void)
{
    ::WaitForMultipleObjects(xg_dwThreadCount, xg_ahThreads.data(), true, 1000);
}

// スレッドが終了したか？
bool __fastcall XgIsAnyThreadTerminated(void)
{
    DWORD dwExitCode;
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        ::GetExitCodeThread(xg_ahThreads[i], &dwExitCode);
        if (dwExitCode != STILL_ACTIVE)
            return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////

// プログレスバーの更新頻度。
#define xg_dwTimerInterval 500

// 再計算までの時間を概算する。
inline DWORD XgGetRetryInterval(void)
{
    return 8 * (xg_nRows + xg_nCols) * (xg_nRows + xg_nCols) + 1000;
}

// キャンセルダイアログ。
extern "C" INT_PTR CALLBACK
XgCancelSolveDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        #ifdef NO_RANDOM
        {
            extern int xg_random_seed;
            xg_random_seed = 100;
        }
        #endif
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 再試行カウントをクリアする。
        s_nRetryCount = 0;
        // プログレスバーの範囲をセットする。
        ::SendDlgItemMessageW(hwnd, ctl1, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        ::SendDlgItemMessageW(hwnd, ctl2, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        // 計算時間を求めるために、開始時間を取得する。
        s_dwTick0 = s_dwTick1 = ::GetTickCount();
        // 再計算までの時間を概算する。
        s_dwWait = XgGetRetryInterval();
        // 解を求めるのを開始。
        XgStartSolve_AddBlack();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        // 生成した問題の個数を表示する。
        if (s_nNumberGenerated > 0) {
            WCHAR sz[MAX_PATH];
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMAKING),
                           s_nNumberGenerated);
            ::SetDlgItemTextW(hwnd, stc2, sz);
        }
        return false;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case psh1:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh2:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 再計算しなおす。
            xg_bRetrying = true;
            XgWaitForThreads();
            XgCloseThreads();
            ::InterlockedIncrement(&s_nRetryCount);
            s_dwTick1 = ::GetTickCount();
            XgStartSolve_AddBlack();
            // タイマーをセットする。
            ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            break;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE) {
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
        }
        break;

    case WM_TIMER:
        // プログレスバーを更新する。
        //for (DWORD i = 0; i < xg_dwThreadCount; i++)
        for (DWORD i = 0; i < 2; i++) {
            ::SendDlgItemMessageW(hwnd, ctl1 + i, PBM_SETPOS,
                static_cast<WPARAM>(xg_aThreadInfo[i].m_count), 0);
        }

        {
            // 経過時間を表示する。
            WCHAR sz[MAX_PATH];
            DWORD dwTick = ::GetTickCount();
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_NOWSOLVING),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10, s_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }

        // 終了したスレッドがあるか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
        } else {
            // 再計算が必要か？
            if (s_bAutoRetry && ::GetTickCount() - s_dwTick1 > s_dwWait) {
                // タイマーを解除する。
                ::KillTimer(hwnd, 999);
                // 再計算しなおす。
                xg_bRetrying = true;
                XgWaitForThreads();
                XgCloseThreads();
                ::InterlockedIncrement(&s_nRetryCount);
                s_dwTick1 = ::GetTickCount();
                XgStartSolve_AddBlack();
                // タイマーをセットする。
                ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            }
        }
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////

// キャンセルダイアログ（黒マス追加なし）。
extern "C" INT_PTR CALLBACK
XgCancelSolveDlgProcNoAddBlack(
    HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        #ifdef NO_RANDOM
        {
            extern int xg_random_seed;
            xg_random_seed = 100;
        }
        #endif
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 初期化する。
        s_nRetryCount = 0;
        // プログレスバーの範囲をセットする。
        ::SendDlgItemMessageW(hwnd, ctl1, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        ::SendDlgItemMessageW(hwnd, ctl2, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        // 計算時間を求めるために、開始時間を取得する。
        s_dwTick0 = s_dwTick1 = ::GetTickCount();
        // 再計算までの時間を概算する。
        s_dwWait = XgGetRetryInterval();
        // 解を求めるのを開始。
        XgStartSolve_NoAddBlack();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        // 生成した問題の個数を表示する。
        if (s_nNumberGenerated > 0) {
            WCHAR sz[MAX_PATH];
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMAKING),
                           s_nNumberGenerated);
            ::SetDlgItemTextW(hwnd, stc2, sz);
        }
        return false;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case psh1:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh2:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 再計算しなおす。
            xg_bRetrying = true;
            XgWaitForThreads();
            XgCloseThreads();
            ::InterlockedIncrement(&s_nRetryCount);
            s_dwTick1 = ::GetTickCount();
            XgStartSolve_NoAddBlack();
            // タイマーをセットする。
            ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            break;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE) {
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
        }
        break;

    case WM_TIMER:
        // プログレスバーを更新する。
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            ::SendDlgItemMessageW(hwnd, ctl1 + i, PBM_SETPOS,
                static_cast<WPARAM>(xg_aThreadInfo[i].m_count), 0);
        }
        // 経過時間を表示する。
        {
            WCHAR sz[MAX_PATH];
            DWORD dwTick = ::GetTickCount();
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_NOWSOLVING),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10, s_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }
        // 一つ以上のスレッドが終了したか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
        } else {
            // 再計算が必要か？
            if (s_bAutoRetry && ::GetTickCount() - s_dwTick1 > s_dwWait) {
                // タイマーを解除する。
                ::KillTimer(hwnd, 999);
                // 再計算しなおす。
                xg_bRetrying = true;
                XgWaitForThreads();
                XgCloseThreads();
                ::InterlockedIncrement(&s_nRetryCount);
                s_dwTick1 = ::GetTickCount();
                // スマート解決なら、黒マスを生成する。
                XgStartSolve_NoAddBlack();
                // タイマーをセットする。
                ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            }
        }
        break;
    }
    return false;
}

// キャンセルダイアログ（スマート解決）。
extern "C" INT_PTR CALLBACK
XgCancelSolveDlgProcSmart(
    HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        #ifdef NO_RANDOM
        {
            extern int xg_random_seed;
            xg_random_seed = 100;
        }
        #endif
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 初期化する。
        s_nRetryCount = 0;
        // プログレスバーの範囲をセットする。
        ::SendDlgItemMessageW(hwnd, ctl1, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        ::SendDlgItemMessageW(hwnd, ctl2, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        // 計算時間を求めるために、開始時間を取得する。
        s_dwTick0 = s_dwTick1 = ::GetTickCount();
        // 再計算までの時間を概算する。
        s_dwWait = XgGetRetryInterval();
        // 解を求めるのを開始。
        XgStartSolve_Smart();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        // 生成した問題の個数を表示する。
        if (s_nNumberGenerated > 0) {
            WCHAR sz[MAX_PATH];
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMAKING),
                           s_nNumberGenerated);
            ::SetDlgItemTextW(hwnd, stc2, sz);
        }
        return false;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case psh1:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh2:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 再計算しなおす。
            xg_bRetrying = true;
            XgWaitForThreads();
            XgCloseThreads();
            ::InterlockedIncrement(&s_nRetryCount);
            s_dwTick1 = ::GetTickCount();
            XgStartSolve_Smart();
            // タイマーをセットする。
            ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            break;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE) {
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
        }
        break;

    case WM_TIMER:
        // プログレスバーを更新する。
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            ::SendDlgItemMessageW(hwnd, ctl1 + i, PBM_SETPOS,
                static_cast<WPARAM>(xg_aThreadInfo[i].m_count), 0);
        }
        // 経過時間を表示する。
        {
            WCHAR sz[MAX_PATH];
            DWORD dwTick = ::GetTickCount();
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_NOWSOLVING),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10, s_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }
        // 一つ以上のスレッドが終了したか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
        } else {
            // 再計算が必要か？
            if (s_bAutoRetry && ::GetTickCount() - s_dwTick1 > s_dwWait) {
                // タイマーを解除する。
                ::KillTimer(hwnd, 999);
                // 再計算しなおす。
                xg_bRetrying = true;
                XgWaitForThreads();
                XgCloseThreads();
                ::InterlockedIncrement(&s_nRetryCount);
                s_dwTick1 = ::GetTickCount();
                XgStartSolve_Smart();
                // タイマーをセットする。
                ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            }
        }
        break;
    }
    return false;
}

// キャンセルダイアログ。
extern "C" INT_PTR CALLBACK
XgCancelGenBlacksDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 解を求めるのを開始。
        XgStartGenerateBlacks(!!lParam);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        // 開始時間。
        s_dwTick0 = ::GetTickCount();
        s_nRetryCount = 0;
        // 生成したパターンの個数を表示する。
        if (s_nNumberGenerated > 0) {
            WCHAR sz[MAX_PATH];
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PATMAKING), s_nNumberGenerated);
            ::SetDlgItemTextW(hwnd, stc2, sz);
        }
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        return false;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case psh1:
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_CLOSE) {
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // キャンセルしてスレッドを待つ。
            xg_bCancelled = true;
            XgWaitForThreads();
            // スレッドを閉じる。
            XgCloseThreads();
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
        }
        break;

    case WM_TIMER:
        {
            // 経過時間を表示する。
            WCHAR sz[MAX_PATH];
            DWORD dwTick = ::GetTickCount();
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CALCULATING),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }

        // 終了したスレッドがあるか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 計算時間を求めるために、終了時間を取得する。
            s_dwTick2 = ::GetTickCount();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
        }
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// 印刷。

// 印刷する。
void __fastcall XgPrintIt(HDC hdc, PRINTDLGW* ppd, bool bPrintAnswer)
{
    LOGFONTW lf;
    HFONT hFont;
    HGDIOBJ hFontOld;
    RECT rc;

    const int nTate = ::GetDeviceCaps(hdc, VERTSIZE);
    const int nYoko = ::GetDeviceCaps(hdc, HORZSIZE);
    if (nTate < nYoko) {
        // 印刷用紙が横長。メッセージを表示して終了。
        ::XgCenterMessageBoxW(xg_hMainWnd, XgLoadStringDx1(IDS_SETPORTRAIT), nullptr,
                            MB_ICONERROR);
        ::DeleteDC(hdc);
        ::GlobalFree(ppd->hDevMode);
        ::GlobalFree(ppd->hDevNames);
        return;
    }

    DOCINFOW di;
    ZeroMemory(&di, sizeof(di));
    di.cbSize = sizeof(di);
    di.lpszDocName = XgLoadStringDx1(IDS_CROSSWORD);

    // 指定された部数を印刷する。
    for (int i = 0; i < ppd->nCopies; i++) {
        // 文書を開始する。
        if (::StartDocW(hdc, &di) <= 0)
            continue;

        size_t ich, ichOld;
        std::wstring str, strMarkWord, strHints;
        int yLast = 0;

        // 論理ピクセルのアスペクト比を取得する。
        const int nLogPixelX = ::GetDeviceCaps(hdc, LOGPIXELSX);
        const int nLogPixelY = ::GetDeviceCaps(hdc, LOGPIXELSY);

        // 用紙のピクセルサイズを取得する。
        int cxPaper = ::GetDeviceCaps(hdc, HORZRES);
        int cyPaper = ::GetDeviceCaps(hdc, VERTRES);

        // ページ開始。
        if (::StartPage(hdc) > 0) {
            // 二重マス単語を描画する。
            if (bPrintAnswer && XgGetMarkWord(&xg_solution, strMarkWord)) {
                // 既定のフォント情報を取得する。
                hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
                ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);

                // フォント名を取得する。
                StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
                if (xg_szCellFont[0])
                    StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);

                // フォント情報を設定する。
                lf.lfHeight = cyPaper / 2 / 15;
                lf.lfWidth = 0;
                lf.lfWeight = FW_NORMAL;
                lf.lfQuality = ANTIALIASED_QUALITY;

                // 二重マス単語を描画する。
                hFont = ::CreateFontIndirectW(&lf);
                hFontOld = ::SelectObject(hdc, hFont);
                ::SetRect(&rc, cxPaper / 8, cyPaper / 16,
                    cxPaper * 7 / 8, cyPaper / 8);
                str = XgLoadStringDx1(IDS_ANSWER);
                str += strMarkWord;
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);
                ::SelectObject(hdc, hFontOld);
                ::DeleteFont(hFont);
            }

            // EMFオブジェクトを作成する。
            HDC hdcEMF = ::CreateEnhMetaFileW(hdc, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
            if (hdcEMF != nullptr) {
                // EMFオブジェクトにクロスワードを描画する。
                SIZE siz;
                XgGetXWordExtent(&siz);
                if (bPrintAnswer)
                    XgDrawXWord(xg_solution, hdcEMF, &siz, false);
                else
                    XgDrawXWord(xg_xword, hdcEMF, &siz, false);

                // EMFオブジェクトを閉じる。
                HENHMETAFILE hEMF = ::CloseEnhMetaFile(hdcEMF);
                if (hEMF != nullptr) {
                    // EMFを描画する領域を計算する。
                    int x, y, cx, cy;
                    if (xg_nCols >= 10) {
                        x = cxPaper / 6;
                        y = cyPaper / 8;
                        cx = cxPaper * 2 / 3;
                        cy = cxPaper * nLogPixelY * siz.cy / nLogPixelX / siz.cx * 2 / 3;
                    } else {
                        x = cxPaper / 4;
                        y = cyPaper / 8;
                        cx = cxPaper / 2;
                        cy = cxPaper * nLogPixelY * siz.cy / nLogPixelX / siz.cx / 2;
                    }
                    ::SetRect(&rc, x, y, x + cx, y + cy);

                    // EMFを描画する。
                    ::PlayEnhMetaFile(hdc, hEMF, &rc);
                    ::DeleteEnhMetaFile(hEMF);
                    yLast = y + cy;
                }
            }

            // ヒントを取得する。
            if (xg_bSolved) {
                xg_solution.GetHintsStr(xg_strHints, 2, bPrintAnswer);
                strHints = xg_strHints;
            } else {
                strHints.clear();
            }

            // 文字色をセットする。
            ::SetTextColor(hdc, RGB(0, 0, 0));

            // 既定のフォント情報を取得する。
            hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
            ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);

            // フォント名を取得する。
            StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
            if (xg_szCellFont[0])
                StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);

            // フォント情報を設定する。
            lf.lfHeight = cyPaper / 2 / 45;
            lf.lfWidth = 0;
            lf.lfWeight = FW_NORMAL;
            lf.lfQuality = ANTIALIASED_QUALITY;
            hFont = ::CreateFontIndirectW(&lf);
            hFontOld = ::SelectObject(hdc, hFont);

            // ヒントを一行ずつ描画する。
            ::SetRect(&rc, cxPaper / 8, cyPaper / 2,
                cxPaper * 7 / 8, cyPaper * 8 / 9);
            for (ichOld = ich = 0; rc.bottom <= cyPaper * 8 / 9; ichOld = ich + 1) {
                ich = strHints.find(L"\n", ichOld);
                if (ich == std::wstring::npos)
                    break;
                str = strHints.substr(0, ich);  // 一行取り出す。
                ::SetRect(&rc, cxPaper / 8, yLast,
                    cxPaper * 7 / 8, cyPaper * 8 / 9);
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK);
            }
            // 最後の行を描画する。
            ::SetRect(&rc, cxPaper / 8, yLast,
                cxPaper * 7 / 8, cyPaper * 8 / 9);
            if (ich == std::wstring::npos) {
                str = strHints;
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK);
                strHints.clear();
            } else {
                str = strHints.substr(0, ichOld);
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK);
                strHints = strHints.substr(ichOld);
            }

            // 文字のフォントの選択を解除して削除する。
            ::SelectObject(hdc, hFontOld);
            ::DeleteFont(hFont);

            // ページ終了。
            ::EndPage(hdc);
        }

        // ヒントの残りを描画する。
        if (!strHints.empty()) {
            // ページ開始。
            if (::StartPage(hdc) > 0) {
                // 文字のフォントを作成する。
                hFont = reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
                ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);
                StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
                lf.lfHeight = cyPaper / 2 / 45;
                lf.lfWidth = 0;
                lf.lfWeight = FW_NORMAL;
                lf.lfQuality = ANTIALIASED_QUALITY;

                // ヒントの残りを描画する。
                hFont = ::CreateFontIndirectW(&lf);
                hFontOld = ::SelectObject(hdc, hFont);
                str = strHints;
                ::SetRect(&rc, cxPaper / 8, cyPaper / 9,
                    cxPaper * 7 / 8, cyPaper * 8 / 9);
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK);
                ::SelectObject(hdc, hFontOld);
                ::DeleteFont(hFont);

                // ページ終了。
                ::EndPage(hdc);
            }
        }
        // 文書終了。
        ::EndDoc(hdc);
    }

    ::DeleteDC(hdc);
    ::GlobalFree(ppd->hDevMode);
    ::GlobalFree(ppd->hDevNames);
}

// 問題のみを印刷する。
void __fastcall XgPrintProblem(void)
{
    PRINTDLGW pd;

    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = xg_hMainWnd;
    pd.Flags = PD_ALLPAGES | PD_HIDEPRINTTOFILE |
        PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC;
    pd.nCopies = 1;
    if (::PrintDlgW(&pd)) {
        XgPrintIt(pd.hDC, &pd, false);
    }
}

// 問題と解答を印刷する。
void __fastcall XgPrintAnswer(void)
{
    PRINTDLGW pd;

    ZeroMemory(&pd, sizeof(pd));
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = xg_hMainWnd;
    pd.Flags = PD_ALLPAGES | PD_HIDEPRINTTOFILE |
        PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC;
    pd.nCopies = 1;
    if (::PrintDlgW(&pd)) {
        XgPrintIt(pd.hDC, &pd, true);
    }
}

//////////////////////////////////////////////////////////////////////////////

// バージョン情報を表示する。
void __fastcall XgOnAbout(HWND hwnd)
{
    MSGBOXPARAMS params;
    memset(&params, 0, sizeof(params));
    params.cbSize = sizeof(params);
    params.hwndOwner = hwnd;
    params.hInstance = xg_hInstance;
    params.lpszText = XgLoadStringDx1(IDS_VERSION);
    params.lpszCaption = XgLoadStringDx2(IDS_APPNAME);
    params.dwStyle = MB_USERICON;
    params.lpszIcon = MAKEINTRESOURCE(1);
    XgCenterMessageBoxIndirectW(&params);
}

// 新規作成ダイアログ。
bool __fastcall XgOnNew(HWND hwnd)
{
    INT nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    nID = INT(::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_NEW), hwnd, XgNewDlgProc));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK)
        return false;

    return true;
}

// 特定の場所にファイルを保存する。
bool __fastcall XgDoSaveToLocation(HWND hwnd)
{
    WCHAR szPath[MAX_PATH], szDir[MAX_PATH];
    WCHAR szName[32];

    // パスを生成する。
    StringCbCopy(szDir, sizeof(szDir), s_dirs_save_to[0].data());
    PathAddBackslashW(szDir);

    // ファイル名を生成する。
    UINT u;
    for (u = 1; u <= 0xFFFF; u++) {
        StringCbPrintf(szName, sizeof(szName), L"%dx%d-%04u.xwj",
                  xg_nRows, xg_nCols, u);
        StringCbCopy(szPath, sizeof(szPath), szDir);
        StringCbCat(szPath, sizeof(szPath), szName);
        if (::GetFileAttributesW(szPath) == 0xFFFFFFFF)
            break;
    }

    bool bOK = false;
    if (u != 0x10000) {
        // ファイル名が作成できた。排他制御しながら保存する。
        ::EnterCriticalSection(&xg_cs);
        {
            // 解あり？
            if (xg_bSolved) {
                // カギ番号を更新する。
                xg_solution.DoNumberingNoCheck();

                // ヒントを更新する。
                XgUpdateHints(hwnd);
            }

            // ファイルに保存する。
            bOK = XgDoSave(hwnd, szPath);
        }
        ::LeaveCriticalSection(&xg_cs);
    }

    // ディスク容量を確認する。
    ULARGE_INTEGER ull1, ull2, ull3;
    if (::GetDiskFreeSpaceExW(szPath, &ull1, &ull2, &ull3)) {
        if (ull1.QuadPart < 0x1000000)  // 1MB
        {
            s_bOutOfDiskSpace = true;
        }
    }

    return bOK;
}

// 問題の作成。
bool __fastcall XgOnGenerate(HWND hwnd, bool show_answer, bool multiple = false)
{
    INT nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    if (multiple) {
        // [問題の連続作成]ダイアログ。
        nID = INT(::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_SEQCREATE), hwnd,
                               XgGenerateRepeatedlyDlgProc));
    } else {
        // [問題の作成]ダイアログ。
        nID = INT(::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CREATE), hwnd,
                               XgGenerateDlgProc));
    }
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        return false;
    }

    xg_bSolvingEmpty = true;
    xg_bNoAddBlack = false;
    s_nNumberGenerated = 0;
    s_bOutOfDiskSpace = false;
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_strFileName.clear();

    // 辞書を読み込む。
    XgLoadDictFile(xg_dict_name.c_str());
    XgSetInputModeFromDict(hwnd);

    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    do {
        if (xg_imode == xg_im_KANJI) {
            // 漢字の場合はスマート解決を使用しない。
            nID = ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd, XgCancelSolveDlgProc);
        } else if (xg_bSmartResolution && xg_nRows >= 7 && xg_nCols >= 7) {
            nID = ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd, XgCancelSolveDlgProcSmart);
        } else if (xg_nRules & RULE_POINTSYMMETRY) {
            nID = ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd, XgCancelSolveDlgProcSmart);
        } else {
            nID = ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd, XgCancelSolveDlgProc);
        }
        // 生成成功のときはs_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bSolved) {
            ++s_nNumberGenerated;
            if (multiple) {
                if (!XgDoSaveToLocation(hwnd)) {
                    s_bOutOfDiskSpace = true;
                    break;
                }
                // 初期化。
                xg_bSolved = false;
                xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
                xg_bSolvingEmpty = true;
                xg_strHeader.clear();
                xg_strNotes.clear();
                xg_strFileName.clear();
                // 辞書を読み込む。
                XgLoadDictFile(xg_dict_name.c_str());
                XgSetInputModeFromDict(hwnd);
            }
        }
    } while (nID == IDOK && multiple && s_nNumberGenerated < s_nNumberToGenerate);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    // 初期化する。
    if (multiple) {
        xg_xword.clear();
        xg_vMarkedCands.clear();
        xg_vMarks.clear();
        xg_vTateInfo.clear();
        xg_vYokoInfo.clear();
        xg_vecTateHints.clear();
        xg_vecYokoHints.clear();
        xg_bSolved = false;
        xg_bShowAnswer = false;
    } else {
        xg_bShowAnswer = show_answer;
        if (xg_bSmartResolution && xg_bCancelled) {
            xg_xword.clear();
        }
    }

    return true;
}

// 黒マスパターンを連続生成する。
bool __fastcall XgOnGenerateBlacksRepeatedly(HWND hwnd)
{
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    INT nID = DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(IDD_SEQPATGEN), hwnd,
                         XgGenerateBlacksRepeatedlyDlgProc);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        return false;
    }

    // 初期化する。
    xg_xword.clear();
    xg_caret_pos.clear();
    xg_vMarks.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_bSolved = false;
    xg_bHintsAdded = false;
    xg_bShowAnswer = false;
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_strFileName.clear();
    xg_bBlacksGenerated = false;
    s_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、生成を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    do
    {
        nID = ::DialogBoxParamW(xg_hInstance, MAKEINTRESOURCEW(IDD_CALCULATING),
                                hwnd, XgCancelGenBlacksDlgProc,
                                !!(xg_nRules & RULE_POINTSYMMETRY));
        // 生成成功のときはs_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bBlacksGenerated) {
            ++s_nNumberGenerated;
            if (!XgDoSaveToLocation(hwnd)) {
                s_bOutOfDiskSpace = true;
                break;
            }
            // 初期化。
            xg_bSolved = false;
            xg_xword.clear();
            xg_vTateInfo.clear();
            xg_vYokoInfo.clear();
            xg_vMarks.clear();
            xg_vMarkedCands.clear();
            xg_strHeader.clear();
            xg_strNotes.clear();
            xg_strFileName.clear();
            xg_bBlacksGenerated = false;
        }
    } while (nID == IDOK && s_nNumberGenerated < s_nNumberToGenerate);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    xg_caret_pos.clear();
    XgUpdateImage(hwnd, 0, 0);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_BLOCKSGENERATED),
            s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    }

    // 保存先フォルダを開く。
    if (s_nNumberGenerated && !s_dirs_save_to.empty()) {
        ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                      nullptr, nullptr, SW_SHOWNORMAL);
    }
    return true;
}

// 黒マスパターンを生成する。
bool __fastcall XgOnGenerateBlacks(HWND hwnd, bool sym)
{
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    INT nID = DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(IDD_PATGEN), hwnd,
                         XgGenerateBlacksDlgProc);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        return false;
    }

    // 初期化する。
    xg_caret_pos.clear();
    xg_vMarks.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_bSolved = false;
    xg_bHintsAdded = false;
    xg_bShowAnswer = false;
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_strFileName.clear();
    xg_bBlacksGenerated = false;
    s_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、生成を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    ::DialogBoxParamW(xg_hInstance, MAKEINTRESOURCEW(IDD_CALCULATING),
                      hwnd, XgCancelGenBlacksDlgProc,
                      !!(xg_nRules & RULE_POINTSYMMETRY));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    xg_caret_pos.clear();
    XgUpdateImage(hwnd, 0, 0);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_BLOCKSGENERATED), 1,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    }
    return true;
}

// 解を求める。
bool __fastcall XgOnSolve_AddBlack(HWND hwnd)
{
    // すでに解かれている場合は、実行を拒否する。
    if (xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

    // 辞書を読み込む。
    XgLoadDictFile(xg_dict_name.c_str());
    XgSetInputModeFromDict(hwnd);

    // 黒マスルールなどをチェックする。
    xg_bNoAddBlack = false;
    if (!XgCheckCrossWord(hwnd)) {
        return false;
    }

    // 初期化する。
    xg_bSolvingEmpty = xg_xword.IsEmpty();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vMarks.clear();
    xg_vMarkedCands.clear();
    xg_strFileName.clear();
    // 生成した数と生成する数。
    s_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd, XgCancelSolveDlgProc);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        // キャンセルされた。
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 空マスがないか？
        if (xg_xword.IsFulfilled()) {
            // 空マスがない。クリア。
            xg_xword.clear();
            // 解に合わせて、問題に黒マスを置く。
            for (int i = 0; i < xg_nRows; i++) {
                for (int j = 0; j < xg_nCols; j++) {
                    if (xg_solution.GetAt(i, j) == ZEN_BLACK)
                        xg_xword.SetAt(i, j, ZEN_BLACK);
                }
            }
        }

        // 解あり。表示を更新する。
        xg_bShowAnswer = true;
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 成功メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_SOLVED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints(hwnd);
        XgShowHints(hwnd);
    } else {
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // 失敗メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANTSOLVE),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        ::InvalidateRect(hwnd, nullptr, FALSE);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONERROR);
    }
    return true;
}

// 解を求める（黒マス追加なし）。
bool __fastcall XgOnSolve_NoAddBlack(HWND hwnd, bool bShowAnswer = true)
{
    // すでに解かれている場合は、実行を拒否する。
    if (xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

    // 辞書を読み込む。
    XgLoadDictFile(xg_dict_name.c_str());
    XgSetInputModeFromDict(hwnd);

    // 黒マスルールなどをチェックする。
    xg_bNoAddBlack = true;
    if (!XgCheckCrossWord(hwnd)) {
        return false;
    }

    // 初期化する。
    xg_bSolvingEmpty = xg_xword.IsEmpty();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vMarks.clear();
    xg_vMarkedCands.clear();
    xg_strFileName.clear();
    // 生成した数と生成する数。
    s_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd,
                 XgCancelSolveDlgProcNoAddBlack);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        // キャンセルされた。
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 終了メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 空マスがないか？
        if (xg_xword.IsFulfilled()) {
            // 空マスがない。クリア。
            xg_xword.clear();
            // 解に合わせて、問題に黒マスを置く。
            for (int i = 0; i < xg_nRows; i++) {
                for (int j = 0; j < xg_nCols; j++) {
                    if (xg_solution.GetAt(i, j) == ZEN_BLACK)
                        xg_xword.SetAt(i, j, ZEN_BLACK);
                }
            }
        }

        // 解あり。表示を更新する。
        xg_bShowAnswer = bShowAnswer;
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 成功メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_SOLVED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints(hwnd);
        XgShowHints(hwnd);
    } else {
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // 失敗メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANTSOLVE),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        ::InvalidateRect(hwnd, nullptr, FALSE);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONERROR);
    }

    return true;
}

// 連続で解を求める。
bool __fastcall XgOnSolveRepeatedly(HWND hwnd)
{
    // すでに解かれている場合は、実行を拒否する。
    if (xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

    // 辞書を読み込む。
    XgLoadDictFile(xg_dict_name.c_str());
    XgSetInputModeFromDict(hwnd);

    // 黒マスルールなどをチェックする。
    xg_bNoAddBlack = false;
    if (!XgCheckCrossWord(hwnd)) {
        return false;
    }

    // 実行前のマスの状態を保存する。
    XG_Board xword_save(xg_xword);

    // [解の連続作成]ダイアログ。
    INT nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    nID = INT(::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_SEQSOLVE), hwnd,
                           XgSolveRepeatedlyDlgProc));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        // イメージを更新する。
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        return false;
    }

    // 初期化する。
    xg_strFileName.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    s_nNumberGenerated = 0;
    s_bOutOfDiskSpace = false;

    xg_bSolvingEmpty = xg_xword.IsEmpty();
    xg_bNoAddBlack = false;
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();

    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    do
    {
        nID = ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd,
                           XgCancelSolveDlgProc);
        // 生成成功のときはs_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bSolved) {
            ++s_nNumberGenerated;
            if (!XgDoSaveToLocation(hwnd)) {
                s_bOutOfDiskSpace = true;
                break;
            }
            // 初期化。
            xg_bSolved = false;
            xg_xword = xword_save;
            xg_vTateInfo.clear();
            xg_vYokoInfo.clear();
            xg_vMarks.clear();
            xg_vMarkedCands.clear();
            xg_strHeader.clear();
            xg_strNotes.clear();
            xg_strFileName.clear();
            // 辞書を読み込む。
            XgLoadDictFile(xg_dict_name.c_str());
            XgSetInputModeFromDict(hwnd);
        }
    } while (nID == IDOK && s_nNumberGenerated < s_nNumberToGenerate);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    // 初期化する。
    xg_xword = xword_save;
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_strFileName.clear();

    // イメージを更新する。
    xg_bSolved = false;
    xg_bShowAnswer = false;
    xg_caret_pos.clear();
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);

    // 終了メッセージを表示する。
    WCHAR sz[MAX_PATH];
    if (s_bOutOfDiskSpace) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
    }
    XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

    // 保存先フォルダを開く。
    if (s_nNumberGenerated && !s_dirs_save_to.empty()) {
        ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                      nullptr, nullptr, SW_SHOWNORMAL);
    }

    return true;
}

// 連続で解を求める（黒マス追加なし）。
bool __fastcall XgOnSolveRepeatedlyNoAddBlack(HWND hwnd)
{
    // すでに解かれている場合は、実行を拒否する。
    if (xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

    // 辞書を読み込む。
    XgLoadDictFile(xg_dict_name.c_str());
    XgSetInputModeFromDict(hwnd);

    // 黒マスルールなどをチェックする。
    xg_bNoAddBlack = true;
    if (!XgCheckCrossWord(hwnd)) {
        return false;
    }

    // 実行前のマスの状態を保存する。
    XG_Board xword_save(xg_xword);

    // [解の連続作成]ダイアログ。
    INT nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    nID = INT(::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_SEQSOLVE), hwnd,
                           XgSolveRepeatedlyDlgProc));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        // イメージを更新する。
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        return false;
    }

    // 初期化する。
    xg_strFileName.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    s_nNumberGenerated = 0;
    s_bOutOfDiskSpace = false;

    xg_bSolvingEmpty = xg_xword.IsEmpty();
    xg_bNoAddBlack = true;
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();

    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    do
    {
        nID = ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CALCULATING), hwnd,
                           XgCancelSolveDlgProcNoAddBlack);
        // 生成成功のときはs_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bSolved) {
            ++s_nNumberGenerated;
            if (!XgDoSaveToLocation(hwnd)) {
                s_bOutOfDiskSpace = true;
                break;
            }
            // 初期化。
            xg_bSolved = false;
            xg_xword = xword_save;
            xg_vTateInfo.clear();
            xg_vYokoInfo.clear();
            xg_vMarks.clear();
            xg_vMarkedCands.clear();
            xg_strHeader.clear();
            xg_strNotes.clear();
            xg_strFileName.clear();
            // 辞書を読み込む。
            XgLoadDictFile(xg_dict_name.c_str());
            XgSetInputModeFromDict(hwnd);
        }
    } while (nID == IDOK && s_nNumberGenerated < s_nNumberToGenerate);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    // 初期化する。
    xg_xword = xword_save;
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_strFileName.clear();

    // イメージを更新する。
    xg_bSolved = false;
    xg_bShowAnswer = false;
    xg_caret_pos.clear();
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);

    // 終了メッセージを表示する。
    WCHAR sz[MAX_PATH];
    if (s_bOutOfDiskSpace) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
    }
    XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

    // 保存先フォルダを開く。
    if (s_nNumberGenerated && !s_dirs_save_to.empty()) {
        ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                      nullptr, nullptr, SW_SHOWNORMAL);
    }
    return true;
}

// 黒マス線対称チェック。
void XgOnLineSymmetryCheck(HWND hwnd)
{
    if (xg_xword.IsLineSymmetry()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_LINESYMMETRY), XgLoadStringDx2(IDS_APPNAME),
                          MB_ICONINFORMATION);
    } else {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRY), XgLoadStringDx2(IDS_APPNAME),
                          MB_ICONINFORMATION);
    }
}

// 黒マス点対称チェック。
void XgOnPointSymmetryCheck(HWND hwnd)
{
    if (xg_xword.IsPointSymmetry()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_POINTSYMMETRY), XgLoadStringDx2(IDS_APPNAME),
                          MB_ICONINFORMATION);
    } else {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTPOINTSYMMETRY), XgLoadStringDx2(IDS_APPNAME),
                          MB_ICONINFORMATION);
    }
}

//////////////////////////////////////////////////////////////////////////////

// 24BPPビットマップを作成。
HBITMAP XgCreate24BppBitmap(HDC hDC, LONG width, LONG height)
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    return CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
}

// BITMAPINFOEX構造体。
typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR * LPBITMAPINFOEX;

BOOL
PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm)
{
    vecData.clear();

    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm))
        return FALSE;

    BITMAPINFOEX bi;
    BITMAPINFOHEADER *pbmih;
    DWORD cColors, cbColors;

    pbmih = &bi.bmiHeader;
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
    pbmih->biSize             = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth            = bm.bmWidth;
    pbmih->biHeight           = bm.bmHeight;
    pbmih->biPlanes           = 1;
    pbmih->biBitCount         = bm.bmBitsPixel;
    pbmih->biCompression      = BI_RGB;
    pbmih->biSizeImage        = bm.bmWidthBytes * bm.bmHeight;

    if (bm.bmBitsPixel < 16)
        cColors = 1 << bm.bmBitsPixel;
    else
        cColors = 0;
    cbColors = cColors * sizeof(RGBQUAD);

    std::vector<BYTE> Bits(pbmih->biSizeImage);
    HDC hDC = CreateCompatibleDC(NULL);
    if (hDC == NULL)
        return FALSE;

    LPBITMAPINFO pbi = LPBITMAPINFO(&bi);
    if (!GetDIBits(hDC, hbm, 0, bm.bmHeight, &Bits[0], pbi, DIB_RGB_COLORS))
    {
        DeleteDC(hDC);
        return FALSE;
    }

    DeleteDC(hDC);

    std::string stream;
    stream.append((const char *)pbmih, sizeof(*pbmih));
    stream.append((const char *)bi.bmiColors, cbColors);
    stream.append((const char *)&Bits[0], Bits.size());
    vecData.assign(stream.begin(), stream.end());
    return TRUE;
}

// クリップボードにクロスワードをコピー。
void __fastcall XgCopyBoard(HWND hwnd)
{
    std::wstring str;

    // コピーする盤を選ぶ。
    XG_Board *pxw = (xg_bSolved && xg_bShowAnswer) ? &xg_solution : &xg_xword;

    // クロスワードの文字列を取得する。
    pxw->GetString(str);

    // ヒープからメモリを確保する。
    DWORD cb = static_cast<DWORD>((str.size() + 1) * sizeof(WCHAR));
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (hGlobal) {
        // メモリをロックする。
        LPWSTR psz = reinterpret_cast<LPWSTR>(::GlobalLock(hGlobal));
        if (psz) {
            // メモリに文字列をコピーする。
            ::CopyMemory(psz, str.data(), cb);
            // メモリのロックを解除する。
            ::GlobalUnlock(hGlobal);

            // クリップボードを開く。
            if (::OpenClipboard(hwnd)) {
                // クリップボードを空にする。
                if (::EmptyClipboard()) {
                    // Unicodeテキストを設定。
                    ::SetClipboardData(CF_UNICODETEXT, hGlobal);

                    // 描画サイズを取得する。
                    SIZE siz;
                    XgGetXWordExtent(&siz);

                    // EMFを作成する。
                    HDC hdcRef = ::GetDC(hwnd);
                    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
                    if (hdc) {
                        // EMFに描画する。
                        XgDrawXWord(*pxw, hdc, &siz, false);

                        // EMFを設定。
                        HENHMETAFILE hEMF = ::CloseEnhMetaFile(hdc);
                        ::SetClipboardData(CF_ENHMETAFILE, hEMF);

                        // DIBを設定。
                        if (HDC hDC = CreateCompatibleDC(NULL))
                        {
                            HBITMAP hbm = XgCreate24BppBitmap(hDC, siz.cx, siz.cy);
                            HGDIOBJ hbmOld = SelectObject(hDC, hbm);
                            XgDrawXWord(*pxw, hDC, &siz, false);
                            SelectObject(hDC, hbmOld);
                            ::DeleteDC(hDC);

                            std::vector<BYTE> data;
                            if (PackedDIB_CreateFromHandle(data, hbm))
                            {
                                HGLOBAL hGlobal2 = GlobalAlloc(GHND | GMEM_SHARE, data.size());
                                LPVOID pv = GlobalLock(hGlobal2);
                                memcpy(pv, &data[0], data.size());
                                GlobalUnlock(hGlobal2);
                                ::SetClipboardData(CF_DIB, hGlobal2);
                            }
                            ::DeleteObject(hbm);
                        }
                    }
                    ::ReleaseDC(hwnd, hdcRef);
                }
                // クリップボードを閉じる。
                ::CloseClipboard();
                return;
            }
        }
        // 確保したメモリを解放する。
        ::GlobalFree(hGlobal);
    }
}

// クリップボードにクロスワードを画像としてコピー。
void __fastcall XgCopyBoardAsImage(HWND hwnd)
{
    XG_Board *pxw = (xg_bSolved && xg_bShowAnswer) ? &xg_solution : &xg_xword;

    // 描画サイズを取得する。
    SIZE siz;
    XgGetXWordExtent(&siz);

    // EMFを作成する。
    HENHMETAFILE hEMF = NULL;
    HDC hdcRef = ::GetDC(hwnd);
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    if (hdc) {
        // EMFに描画する。
        XgDrawXWord(*pxw, hdc, &siz, false);
        hEMF = ::CloseEnhMetaFile(hdc);
    }

    // BMPを作成する。
    HBITMAP hbm = NULL;
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = siz.cx;
        bi.bmiHeader.biHeight = siz.cy;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        LPVOID pvBits;
        hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        if (HGDIOBJ hbmOld = SelectObject(hDC, hbm))
        {
            RECT rc;
            SetRect(&rc, 0, 0, siz.cx, siz.cy);
            PlayEnhMetaFile(hDC, hEMF, &rc);
            SelectObject(hDC, hbmOld);
        }
        DeleteDC(hDC);
    }
    HGLOBAL hGlobal = NULL;
    if (hbm)
    {
        std::vector<BYTE> data;
        PackedDIB_CreateFromHandle(data, hbm);

        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, DWORD(data.size()));
        if (hGlobal)
        {
            if (LPVOID pv = GlobalLock(hGlobal))
            {
                memcpy(pv, &data[0], data.size());
                GlobalUnlock(hGlobal);
            }
        }
        DeleteObject(hbm);
        hbm = NULL;
    }

    // クリップボードを開く。
    if (::OpenClipboard(hwnd)) {
        // クリップボードを空にする。
        if (::EmptyClipboard()) {
            // BMPを設定。
            ::SetClipboardData(CF_DIB, hGlobal);
            // EMFを設定。
            ::SetClipboardData(CF_ENHMETAFILE, hEMF);

            ::ReleaseDC(hwnd, hdcRef);
        }
        // クリップボードを閉じる。
        ::CloseClipboard();
    }
}

// 「サイズを指定」ダイアログプロシージャー。
INT_PTR CALLBACK
ImageSize_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);

        ::SetDlgItemInt(hwnd, edt1, s_nImageCopyWidth, FALSE);
        ::SetDlgItemInt(hwnd, edt2, s_nImageCopyHeight, FALSE);
        if (s_bImageCopyByHeight) {
            ::CheckRadioButton(hwnd, rad1, rad2, rad2);
            ::EnableWindow(::GetDlgItem(hwnd, edt1), FALSE);
            ::EnableWindow(::GetDlgItem(hwnd, edt2), TRUE);
        } else {
            ::CheckRadioButton(hwnd, rad1, rad2, rad1);
            ::EnableWindow(::GetDlgItem(hwnd, edt1), TRUE);
            ::EnableWindow(::GetDlgItem(hwnd, edt2), FALSE);
        }
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case rad1:
            if (HIWORD(wParam) == BN_CLICKED) {
                ::EnableWindow(::GetDlgItem(hwnd, edt1), TRUE);
                ::EnableWindow(::GetDlgItem(hwnd, edt2), FALSE);
            }
            break;

        case rad2:
            if (HIWORD(wParam) == BN_CLICKED) {
                ::EnableWindow(::GetDlgItem(hwnd, edt1), FALSE);
                ::EnableWindow(::GetDlgItem(hwnd, edt2), TRUE);
            }
            break;

        case IDOK:
            s_nImageCopyWidth = ::GetDlgItemInt(hwnd, edt1, NULL, FALSE);
            if (s_nImageCopyWidth <= 0) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                SetFocus(::GetDlgItem(hwnd, edt1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), NULL,
                                    MB_ICONERROR);
                break;
            }
            s_nImageCopyHeight = ::GetDlgItemInt(hwnd, edt2, NULL, FALSE);
            if (s_nImageCopyHeight <= 0) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                SetFocus(::GetDlgItem(hwnd, edt2));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), NULL,
                                    MB_ICONERROR);
                break;
            }
            if (::IsDlgButtonChecked(hwnd, rad2) == BST_CHECKED) {
                s_bImageCopyByHeight = true;
            } else {
                s_bImageCopyByHeight = false;
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            ::EndDialog(hwnd, IDCANCEL);
            break;
        }
    }
    return 0;
}

// サイズを指定してクリップボードにクロスワードを画像としてコピー。
void __fastcall XgCopyBoardAsImageSized(HWND hwnd)
{
    if (IDOK != ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_SIZES), hwnd,
                             ImageSize_DlgProc))
    {
        return;
    }

    HENHMETAFILE hEMF = NULL, hEMF2 = NULL;
    XG_Board *pxw = (xg_bSolved && xg_bShowAnswer) ? &xg_solution : &xg_xword;

    // 描画サイズを取得する。
    SIZE siz;
    XgGetXWordExtent(&siz);

    // EMFを作成する。
    HDC hdcRef = ::GetDC(hwnd);
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    // EMFに描画する。
    XgDrawXWord(*pxw, hdc, &siz, false);
    hEMF = ::CloseEnhMetaFile(hdc);

    MRect rc;
    if (s_bImageCopyByHeight) {
        int cx = siz.cx * s_nImageCopyHeight / siz.cy;
        ::SetRect(&rc, 0, 0, cx, s_nImageCopyHeight);
    } else {
        int cy = siz.cy * s_nImageCopyWidth / siz.cx;
        ::SetRect(&rc, 0, 0, s_nImageCopyWidth, cy);
    }
    siz.cx = rc.right - rc.left;
    siz.cy = rc.bottom - rc.top;

    HDC hdc2 = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    ::PlayEnhMetaFile(hdc2, hEMF, &rc);
    hEMF2 = ::CloseEnhMetaFile(hdc2);
    ::DeleteEnhMetaFile(hEMF);

    // BMPを作成する。
    HBITMAP hbm = NULL;
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = siz.cx;
        bi.bmiHeader.biHeight = siz.cy;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        LPVOID pvBits;
        hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        if (HGDIOBJ hbmOld = SelectObject(hDC, hbm))
        {
            PlayEnhMetaFile(hDC, hEMF2, &rc);
            SelectObject(hDC, hbmOld);
        }
        DeleteDC(hDC);
    }
    HGLOBAL hGlobal = NULL;
    if (hbm)
    {
        std::vector<BYTE> data;
        PackedDIB_CreateFromHandle(data, hbm);

        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, DWORD(data.size()));
        if (hGlobal)
        {
            if (LPVOID pv = GlobalLock(hGlobal))
            {
                memcpy(pv, &data[0], data.size());
                GlobalUnlock(hGlobal);
            }
        }
        DeleteObject(hbm);
        hbm = NULL;
    }

    // クリップボードを開く。
    if (::OpenClipboard(hwnd)) {
        // クリップボードを空にする。
        if (::EmptyClipboard()) {
            // BMPを設定。
            ::SetClipboardData(CF_DIB, hGlobal);
            // EMFを設定。
            ::SetClipboardData(CF_ENHMETAFILE, hEMF2);
        }
        // クリップボードを閉じる。
        ::CloseClipboard();
    }

    ::ReleaseDC(hwnd, hdcRef);
}

// 二重マス単語を画像としてコピー。
void __fastcall XgCopyMarkWordAsImage(HWND hwnd)
{
    // マークがなければ、実行を拒否する。
    if (xg_vMarks.empty()) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // 描画サイズを取得する。
    SIZE siz;
    int nCount = static_cast<int>(xg_vMarks.size());
    XgGetMarkWordExtent(nCount, &siz);

    // EMFを作成する。
    HENHMETAFILE hEMF = NULL;
    HDC hdcRef = ::GetDC(hwnd);
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    if (hdc) {
        // EMFに描画する。
        XgDrawMarkWord(hdc, &siz);
        ::ReleaseDC(hwnd, hdcRef);
        hEMF = ::CloseEnhMetaFile(hdc);
    }

    // クリップボードを開く。
    if (::OpenClipboard(hwnd)) {
        // クリップボードを空にする。
        if (::EmptyClipboard()) {
            // EMFを設定。
            ::SetClipboardData(CF_ENHMETAFILE, hEMF);
        }
        // クリップボードを閉じる。
        ::CloseClipboard();
    }
}

// 「高さを指定」ダイアログプロシージャー。
INT_PTR CALLBACK
MarksHeight_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);

        ::SetDlgItemInt(hwnd, edt1, s_nMarksHeight, FALSE);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            s_nMarksHeight = ::GetDlgItemInt(hwnd, edt1, NULL, FALSE);
            if (s_nMarksHeight <= 0) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                SetFocus(::GetDlgItem(hwnd, edt1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), NULL,
                                    MB_ICONERROR);
                break;
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            ::EndDialog(hwnd, IDCANCEL);
            break;
        }
    }
    return 0;
}

// 高さを指定して二重マス単語を画像としてコピー。
void __fastcall XgCopyMarkWordAsImageSized(HWND hwnd)
{
    // マークがなければ、実行を拒否する。
    if (xg_vMarks.empty()) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    if (IDOK != ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_HEIGHT), hwnd,
                             MarksHeight_DlgProc))
    {
        return;
    }

    HENHMETAFILE hEMF = NULL, hEMF2 = NULL;

    // 描画サイズを取得する。
    SIZE siz;
    int nCount = static_cast<int>(xg_vMarks.size());
    XgGetMarkWordExtent(nCount, &siz);

    // EMFを作成する。
    HDC hdcRef = ::GetDC(hwnd);
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    // EMFに描画する。
    XgDrawMarkWord(hdc, &siz);
    hEMF = ::CloseEnhMetaFile(hdc);

    MRect rc;
    int cx = siz.cx * s_nMarksHeight / siz.cy;
    ::SetRect(&rc, 0, 0, cx, s_nMarksHeight);

    // EMFを作成する。
    HDC hdc2 = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    ::PlayEnhMetaFile(hdc2, hEMF, &rc);
    hEMF2 = ::CloseEnhMetaFile(hdc2);

    ::DeleteEnhMetaFile(hEMF);
    ::ReleaseDC(hwnd, hdcRef);

    // BMPを作成する。
    HBITMAP hbm = NULL;
    if (HDC hDC = CreateCompatibleDC(NULL))
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = rc.right - rc.left;
        bi.bmiHeader.biHeight = rc.bottom - rc.top;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        LPVOID pvBits;
        hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
        if (HGDIOBJ hbmOld = SelectObject(hDC, hbm))
        {
            PlayEnhMetaFile(hDC, hEMF2, &rc);
            SelectObject(hDC, hbmOld);
        }
        DeleteDC(hDC);
    }
    HGLOBAL hGlobal = NULL;
    if (hbm)
    {
        std::vector<BYTE> data;
        PackedDIB_CreateFromHandle(data, hbm);

        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, DWORD(data.size()));
        if (hGlobal)
        {
            if (LPVOID pv = GlobalLock(hGlobal))
            {
                memcpy(pv, &data[0], data.size());
                GlobalUnlock(hGlobal);
            }
        }
        DeleteObject(hbm);
        hbm = NULL;
    }

    // クリップボードを開く。
    if (::OpenClipboard(hwnd)) {
        // クリップボードを空にする。
        if (::EmptyClipboard()) {
            // EMFを設定。
            ::SetClipboardData(CF_ENHMETAFILE, hEMF2);
            // BMPを設定。
            ::SetClipboardData(CF_DIB, hGlobal);
        }
        // クリップボードを閉じる。
        ::CloseClipboard();
    }
}

std::wstring XgGetClipboardUnicodeText(HWND hwnd)
{
    std::wstring str;

    // クリップボードを開く。
    HGLOBAL hGlobal;
    if (::OpenClipboard(hwnd)) {
        // Unicode文字列を取得。
        hGlobal = ::GetClipboardData(CF_UNICODETEXT);
        LPWSTR psz = reinterpret_cast<LPWSTR>(::GlobalLock(hGlobal));
        if (psz) {
            str = psz;
            ::GlobalUnlock(hGlobal);
        }
        // クリップボードを閉じる。
        ::CloseClipboard();
    }

    return str;
}

// クリップボードから貼り付け。
void __fastcall XgPasteBoard(HWND hwnd, const std::wstring& str)
{
    // 文字列が空じゃないか？
    if (!str.empty()) {
        // 文字列がクロスワードを表していると仮定する。
        // クロスワードに文字列を設定。
        if (xg_xword.SetString(str)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();

            xg_bSolved = false;
            xg_bShowAnswer = false;
            xg_caret_pos.clear();
            xg_vMarks.clear();
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        } else {
            ::MessageBeep(0xFFFFFFFF);
        }
    }
}

// クリップボードにテキストをコピーする。
BOOL XgSetClipboardUnicodeText(HWND hwnd, const std::wstring& str)
{
    // ヒープからメモリを確保する。
    DWORD cb = static_cast<DWORD>((str.size() + 1) * sizeof(WCHAR));
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (hGlobal) {
        // メモリをロックする。
        LPWSTR psz = reinterpret_cast<LPWSTR>(::GlobalLock(hGlobal));
        if (psz) {
            // メモリに文字列をコピーする。
            ::CopyMemory(psz, str.data(), cb);
            // メモリのロックを解除する。
            ::GlobalUnlock(hGlobal);

            // クリップボードを開く。
            if (::OpenClipboard(hwnd)) {
                // クリップボードを空にする。
                if (::EmptyClipboard()) {
                    // Unicodeテキストを設定。
                    ::SetClipboardData(CF_UNICODETEXT, hGlobal);
                }
                // クリップボードを閉じる。
                ::CloseClipboard();
                return TRUE;
            }
        }
        // 確保したメモリを解放する。
        ::GlobalFree(hGlobal);
    }

    return FALSE;
}

// クリップボードにヒントをコピー（スタイルゼロ）。
void __fastcall XgCopyHintsStyle0(HWND hwnd, int hint_type)
{
    // 解かれていなければ処理を拒否する。
    if (!xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // クロスワードの文字列を取得する。
    std::wstring str;
    xg_solution.GetHintsStr(str, hint_type, false);
    xg_str_trim(str);

    // クリップボードにテキストをコピーする。
    XgSetClipboardUnicodeText(hwnd, str);
}

// クリップボードにヒントをコピー（スタイルワン）。
void __fastcall XgCopyHintsStyle1(HWND hwnd, int hint_type)
{
    // 解かれていないときは、処理を拒否する。
    if (!xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // クロスワードの文字列を取得する。
    std::wstring str;
    xg_solution.GetHintsStr(str, hint_type, false);
    xg_str_trim(str);

    // スタイルワンでは要らない部分を削除する。
    xg_str_replace_all(str, XgLoadStringDx1(IDS_DOWNLEFT), L"");
    xg_str_replace_all(str, XgLoadStringDx1(IDS_ACROSSLEFT), L"");
    xg_str_replace_all(str, XgLoadStringDx1(IDS_KEYRIGHT), XgLoadStringDx2(IDS_DOT));

    // HTMLデータ (UTF-8)を用意する。
    std::wstring html;
    xg_solution.GetHintsStr(html, hint_type + 3, false);
    xg_str_trim(html);
    std::string htmldata = XgMakeClipHtmlData(html,
        L"p, ol, li { margin-top: 0px; margin-bottom: 0px; }\r\n");

    // クリップボードのHTML形式を登録する。
    UINT CF_HTML = ::RegisterClipboardFormatW(L"HTML Format");

    // ヒープからメモリを確保する。
    DWORD cb = static_cast<DWORD>((str.size() + 1) * sizeof(WCHAR));
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (hGlobal) {
        // メモリをロックする。
        LPWSTR psz = reinterpret_cast<LPWSTR>(::GlobalLock(hGlobal));
        if (psz) {
            // メモリに文字列をコピーする。
            ::CopyMemory(psz, str.data(), cb);
            // メモリのロックを解除する。
            ::GlobalUnlock(hGlobal);

            // ヒープからメモリを確保する。
            cb = static_cast<DWORD>((htmldata.size() + 1) * sizeof(char));
            HGLOBAL hGlobal2 = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
            if (hGlobal2) {
                // メモリをロックする。
                LPVOID p2 = ::GlobalLock(hGlobal2);
                if (p2) {
                    // メモリに文字列をコピーする。
                    LPBYTE pb2 = reinterpret_cast<LPBYTE>(p2);
                    CopyMemory(pb2, htmldata.data(), htmldata.size());
                    pb2[htmldata.size()] = 0;
                    // メモリのロックを解除する。
                    ::GlobalUnlock(hGlobal);

                    // クリップボードを開く。
                    if (::OpenClipboard(hwnd)) {
                        // クリップボードを空にする。
                        if (::EmptyClipboard()) {
                            // Unicodeテキストを設定。
                            ::SetClipboardData(CF_UNICODETEXT, hGlobal);
                            // HTMLデータを設定。
                            ::SetClipboardData(CF_HTML, hGlobal2);
                        }
                        // クリップボードを閉じる。
                        ::CloseClipboard();
                        return;
                    }
                }
                // 確保したメモリを解放する。
                ::GlobalFree(hGlobal2);
            }
        }
        // 確保したメモリを解放する。
        ::GlobalFree(hGlobal);
    }
}

//////////////////////////////////////////////////////////////////////////////

// ウィンドウが破棄された。
void __fastcall MainWnd_OnDestroy(HWND /*hwnd*/)
{
    // イメージリストを破棄する。
    ::ImageList_Destroy(xg_hImageList);
    xg_hImageList = NULL;
    ::ImageList_Destroy(xg_hGrayedImageList);
    xg_hGrayedImageList = NULL;

    // ウィンドウを破棄する。
    ::DestroyWindow(xg_hToolBar);
    xg_hToolBar = NULL;
    ::DestroyWindow(xg_hHScrollBar);
    xg_hHScrollBar = NULL;
    ::DestroyWindow(xg_hVScrollBar);
    xg_hVScrollBar = NULL;
    ::DestroyWindow(xg_hSizeGrip);
    xg_hSizeGrip = NULL;
    ::DestroyWindow(xg_hCandsWnd);
    xg_hCandsWnd = NULL;
    ::DestroyWindow(xg_hHintsWnd);
    xg_hHintsWnd = NULL;
    ::DestroyWindow(xg_hwndInputPalette);
    xg_hwndInputPalette = NULL;

    // アプリを終了する。
    ::PostQuitMessage(0);

    xg_hMainWnd = NULL;
}

// ウィンドウを描画する。
void __fastcall MainWnd_OnPaint(HWND hwnd)
{
    // ツールバーがなければ、初期化の前なので、無視する。
    if (xg_hToolBar == NULL)
        return;

    // クロスワードの描画サイズを取得する。
    SIZE siz;
    ForDisplay for_display;
    XgGetXWordExtent(&siz);

    // 描画を開始する。
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(hwnd, &ps);
    assert(hdc);
    if (hdc) {
        // イメージがない場合は、イメージを取得する。
        if (xg_hbmImage == nullptr) {
            if (xg_bSolved && xg_bShowAnswer)
                xg_hbmImage = XgCreateXWordImage(xg_solution, &siz, true);
            else
                xg_hbmImage = XgCreateXWordImage(xg_xword, &siz, true);
        }

        // クライアント領域を得る。
        MRect rcClient;
        XgGetRealClientRect(hwnd, &rcClient);

        // スクロール位置を取得する。
        int x = XgGetHScrollPos();
        int y = XgGetVScrollPos();

        // ビットマップ イメージをウィンドウに転送する。
        HDC hdcMem = ::CreateCompatibleDC(hdc);
        ::IntersectClipRect(hdc, rcClient.left, rcClient.top,
            rcClient.right, rcClient.bottom);
        HGDIOBJ hbmOld = ::SelectObject(hdcMem, xg_hbmImage);
        ::BitBlt(hdc, rcClient.left - x, rcClient.top - y,
            siz.cx, siz.cy, hdcMem, 0, 0, SRCCOPY);
        ::SelectObject(hdcMem, hbmOld);
        ::DeleteDC(hdcMem);

        // 描画を終了する。
        ::EndPaint(hwnd, &ps);
    }
}

// 横スクロールする。
void __fastcall MainWnd_OnHScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, int pos)
{
    SCROLLINFO si;

    // 横スクロール情報を取得する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    XgGetHScrollInfo(&si);

    // コードに応じて位置情報を設定する。
    switch (code) {
    case SB_LEFT:
        si.nPos = si.nMin;
        break;

    case SB_RIGHT:
        si.nPos = si.nMax;
        break;

    case SB_LINELEFT:
        si.nPos -= 10;
        if (si.nPos < si.nMin)
            si.nPos = si.nMin;
        break;

    case SB_LINERIGHT:
        si.nPos += 10;
        if (si.nPos > si.nMax)
            si.nPos = si.nMax;
        break;

    case SB_PAGELEFT:
        si.nPos -= si.nPage;
        if (si.nPos < si.nMin)
            si.nPos = si.nMin;
        break;

    case SB_PAGERIGHT:
        si.nPos += si.nPage;
        if (si.nPos > si.nMax)
            si.nPos = si.nMax;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        si.nPos = pos;
        break;
    }

    // スクロール情報を設定し、イメージを更新する。
    XgSetHScrollInfo(&si, TRUE);
    XgUpdateImage(hwnd, si.nPos, XgGetVScrollPos());
}

// 縦スクロールする。
void __fastcall MainWnd_OnVScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, int pos)
{
    SCROLLINFO si;

    // 縦スクロール情報を取得する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    XgGetVScrollInfo(&si);

    // コードに応じて位置情報を設定する。
    switch (code) {
    case SB_TOP:
        si.nPos = si.nMin;
        break;

    case SB_BOTTOM:
        si.nPos = si.nMax;
        break;

    case SB_LINEUP:
        si.nPos -= 10;
        if (si.nPos < si.nMin)
            si.nPos = si.nMin;
        break;

    case SB_LINEDOWN:
        si.nPos += 10;
        if (si.nPos > si.nMax)
            si.nPos = si.nMax;
        break;

    case SB_PAGEUP:
        si.nPos -= si.nPage;
        if (si.nPos < si.nMin)
            si.nPos = si.nMin;
        break;

    case SB_PAGEDOWN:
        si.nPos += si.nPage;
        if (si.nPos > si.nMax)
            si.nPos = si.nMax;
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        si.nPos = pos;
        break;
    }

    // スクロール情報を設定し、イメージを更新する。
    XgSetVScrollPos(si.nPos, TRUE);
    XgUpdateImage(hwnd, XgGetHScrollPos(), si.nPos);
}

// 「辞書」メニューを取得する。
HMENU DoFindDictMenu(HMENU hMenu)
{
    WCHAR szText[128];
    LPCWSTR pszDict = XgLoadStringDx1(IDS_DICTIONARY);
    for (INT i = 0; i < 16; ++i)
    {
        if (GetMenuStringW(hMenu, i, szText, ARRAYSIZE(szText), MF_BYPOSITION))
        {
            if (wcsstr(szText, pszDict) != NULL)
            {
                return GetSubMenu(hMenu, i);
            }
        }
    }
    return NULL;
}

// 「辞書」メニューを更新する。
void DoUpdateDictMenu(HMENU hDictMenu)
{
    while (RemoveMenu(hDictMenu, 4, MF_BYPOSITION))
    {
        ;
    }

    if (xg_dict_files.empty())
    {
        AppendMenuW(hDictMenu, MF_STRING | MF_GRAYED, -1, XgLoadStringDx1(IDS_NONE));
        return;
    }

    INT index = 4, count = 0, id = ID_DICTIONARY00;
    WCHAR szText[MAX_PATH];
    for (const auto& file : xg_dict_files)
    {
        LPCWSTR pszFileTitle = PathFindFileNameW(file.c_str());
        StringCbPrintfW(szText, sizeof(szText), L"&%c ", L"0123456789ABCDEF"[count]);
        StringCbCatW(szText, sizeof(szText), pszFileTitle);
        AppendMenuW(hDictMenu, MF_STRING | MF_ENABLED, id, szText);
        ++index;
        ++count;
        ++id;
        if (count >= MAX_DICTS)
            break;
    }

    index = 4;
    for (const auto& file : xg_dict_files)
    {
        if (lstrcmpiW(file.c_str(), xg_dict_name.c_str()) == 0)
        {
            INT nCount = GetMenuItemCount(hDictMenu);
            CheckMenuRadioItem(hDictMenu, 2, nCount - 1, index, MF_BYPOSITION);
            break;
        }
        ++index;
    }
}

// メニューを初期化する。
void __fastcall MainWnd_OnInitMenu(HWND /*hwnd*/, HMENU hMenu)
{
    if (HMENU hDictMenu = DoFindDictMenu(hMenu))
    {
        // 辞書メニューを更新。
        DoUpdateDictMenu(hDictMenu);
    }

    // スケルトンモード。
    if (xg_bSkeltonMode) {
        CheckMenuItem(hMenu, ID_SKELTONMODE, MF_BYCOMMAND | MF_CHECKED);
    } else {
        CheckMenuItem(hMenu, ID_SKELTONMODE, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // 数字を表示するか？
    if (xg_bShowNumbering) {
        CheckMenuItem(hMenu, ID_SHOWHIDENUMBERING, MF_BYCOMMAND | MF_CHECKED);
    } else {
        CheckMenuItem(hMenu, ID_SHOWHIDENUMBERING, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // キャレットを表示するか？
    if (xg_bShowCaret) {
        CheckMenuItem(hMenu, ID_SHOWHIDECARET, MF_BYCOMMAND | MF_CHECKED);
    } else {
        CheckMenuItem(hMenu, ID_SHOWHIDECARET, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // 一定時間が過ぎたらリトライ。
    if (s_bAutoRetry) {
        CheckMenuItem(hMenu, ID_RETRYIFTIMEOUT, MF_BYCOMMAND | MF_CHECKED);
    } else {
        CheckMenuItem(hMenu, ID_RETRYIFTIMEOUT, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // テーマ。
    if (!xg_bThemeModified) {
        CheckMenuItem(hMenu, ID_THEME, MF_BYCOMMAND | MF_UNCHECKED);
        EnableMenuItem(hMenu, ID_RESETTHEME, MF_BYCOMMAND | MF_GRAYED);
    } else {
        CheckMenuItem(hMenu, ID_THEME, MF_BYCOMMAND | MF_CHECKED);
        if (xg_tag_histgram.empty()) {
            EnableMenuItem(hMenu, ID_RESETTHEME, MF_BYCOMMAND | MF_GRAYED);
        } else {
            EnableMenuItem(hMenu, ID_RESETTHEME, MF_BYCOMMAND | MF_ENABLED);
        }
    }

    // 連黒禁。
    if (xg_nRules & RULE_DONTDOUBLEBLACK)
        ::CheckMenuItem(hMenu, ID_RULE_DONTDOUBLEBLACK, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_DONTDOUBLEBLACK, MF_UNCHECKED);
    // 四隅黒禁。
    if (xg_nRules & RULE_DONTCORNERBLACK)
        ::CheckMenuItem(hMenu, ID_RULE_DONTCORNERBLACK, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_DONTCORNERBLACK, MF_UNCHECKED);
    // 三方黒禁。
    if (xg_nRules & RULE_DONTTRIDIRECTIONS)
        ::CheckMenuItem(hMenu, ID_RULE_DONTTRIDIRECTIONS, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_DONTTRIDIRECTIONS, MF_UNCHECKED);
    // 分断禁。
    if (xg_nRules & RULE_DONTDIVIDE)
        ::CheckMenuItem(hMenu, ID_RULE_DONTDIVIDE, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_DONTDIVIDE, MF_UNCHECKED);
    // 黒斜三連禁。
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        ::CheckMenuItem(hMenu, ID_RULE_DONTTHREEDIAGONALS, MF_CHECKED);
        ::CheckMenuItem(hMenu, ID_RULE_DONTFOURDIAGONALS, MF_UNCHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_RULE_DONTTHREEDIAGONALS, MF_UNCHECKED);
        // 黒斜四連禁。
        if (xg_nRules & RULE_DONTFOURDIAGONALS)
            ::CheckMenuItem(hMenu, ID_RULE_DONTFOURDIAGONALS, MF_CHECKED);
        else
            ::CheckMenuItem(hMenu, ID_RULE_DONTFOURDIAGONALS, MF_UNCHECKED);
    }
    // 黒マス点対称。
    if (xg_nRules & RULE_POINTSYMMETRY)
        ::CheckMenuItem(hMenu, ID_RULE_POINTSYMMETRY, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_POINTSYMMETRY, MF_UNCHECKED);

    // 入力モード。
    switch (xg_imode) {
    case xg_im_KANA:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_DIGITINPUT, ID_KANAINPUT, MF_BYCOMMAND);
        break;

    case xg_im_ABC:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_DIGITINPUT, ID_ABCINPUT, MF_BYCOMMAND);
        break;

    case xg_im_KANJI:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_DIGITINPUT, ID_KANJIINPUT, MF_BYCOMMAND);
        break;

    case xg_im_RUSSIA:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_DIGITINPUT, ID_RUSSIAINPUT, MF_BYCOMMAND);
        break;

    case xg_im_DIGITS:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_DIGITINPUT, ID_DIGITINPUT, MF_BYCOMMAND);
        break;
    }

    // 「元に戻す」「やり直し」メニュー更新。
    if (xg_ubUndoBuffer.CanUndo()) {
        ::EnableMenuItem(hMenu, ID_UNDO, MF_BYCOMMAND | MF_ENABLED);
    } else {
        ::EnableMenuItem(hMenu, ID_UNDO, MF_BYCOMMAND | MF_GRAYED);
    }
    if (xg_ubUndoBuffer.CanRedo()) {
        ::EnableMenuItem(hMenu, ID_REDO, MF_BYCOMMAND | MF_ENABLED);
    } else {
        ::EnableMenuItem(hMenu, ID_REDO, MF_BYCOMMAND | MF_GRAYED);
    }

    if (xg_bSolved) {
        ::EnableMenuItem(hMenu, ID_SOLVE, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_SOLVENOADDBLACK, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_SOLVEREPEATEDLY, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_SOLVEREPEATEDLYNOADDBLACK, MF_BYCOMMAND | MF_GRAYED);
        if (xg_bShowAnswer) {
            ::EnableMenuItem(hMenu, ID_SHOWSOLUTION, MF_BYCOMMAND | MF_GRAYED);
            ::EnableMenuItem(hMenu, ID_NOSOLUTION, MF_BYCOMMAND | MF_ENABLED);
            ::CheckMenuRadioItem(hMenu, ID_SHOWSOLUTION, ID_NOSOLUTION, ID_SHOWSOLUTION, MF_BYCOMMAND);
            ::CheckMenuItem(hMenu, ID_SHOWHIDESOLUTION, MF_BYCOMMAND | MF_CHECKED);
        } else {
            ::EnableMenuItem(hMenu, ID_SHOWSOLUTION, MF_BYCOMMAND | MF_ENABLED);
            ::EnableMenuItem(hMenu, ID_NOSOLUTION, MF_BYCOMMAND | MF_GRAYED);
            ::CheckMenuRadioItem(hMenu, ID_SHOWSOLUTION, ID_NOSOLUTION, ID_NOSOLUTION, MF_BYCOMMAND);
            ::CheckMenuItem(hMenu, ID_SHOWHIDESOLUTION, MF_BYCOMMAND | MF_UNCHECKED);
        }
        ::EnableMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_MARKSNEXT, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_MARKSPREV, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_SAVEANSASIMAGE, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_PRINTANSWER, MF_BYCOMMAND | MF_ENABLED);

        ::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE0, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE1, MF_BYCOMMAND | MF_ENABLED);

        ::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE0, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE0, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE1, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE1, MF_BYCOMMAND | MF_ENABLED);

        ::EnableMenuItem(hMenu, ID_OPENCANDSWNDHORZ, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_OPENCANDSWNDVERT, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_MOSTLEFT, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_MOSTRIGHT, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_MOSTUPPER, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_MOSTLOWER, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_LEFT, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_RIGHT, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_UP, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_DOWN, MF_BYCOMMAND | MF_GRAYED);
    } else {
        ::EnableMenuItem(hMenu, ID_SOLVE, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_SOLVENOADDBLACK, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_SOLVEREPEATEDLY, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_SOLVEREPEATEDLYNOADDBLACK, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_SHOWSOLUTION, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_NOSOLUTION, MF_BYCOMMAND | MF_GRAYED);
        ::CheckMenuRadioItem(hMenu, ID_SHOWSOLUTION, ID_NOSOLUTION, ID_NOSOLUTION, MF_BYCOMMAND);
        ::EnableMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_MARKSNEXT, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_MARKSPREV, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_SAVEANSASIMAGE, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_PRINTANSWER, MF_BYCOMMAND | MF_GRAYED);

        ::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE0, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE1, MF_BYCOMMAND | MF_GRAYED);

        ::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE0, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE0, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE1, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE1, MF_BYCOMMAND | MF_GRAYED);

        ::EnableMenuItem(hMenu, ID_OPENCANDSWNDHORZ, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_OPENCANDSWNDVERT, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_MOSTLEFT, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_MOSTRIGHT, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_MOSTUPPER, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_MOSTLOWER, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_LEFT, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_RIGHT, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_UP, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_DOWN, MF_BYCOMMAND | MF_ENABLED);
    }

    // 二重マスメニュー更新。
    if (xg_vMarks.empty()) {
        ::EnableMenuItem(hMenu, ID_KILLMARKS, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGE, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGESIZED, MF_BYCOMMAND | MF_GRAYED);
    } else {
        ::EnableMenuItem(hMenu, ID_KILLMARKS, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGE, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGESIZED, MF_BYCOMMAND | MF_ENABLED);
    }

    // 「解を削除して盤のロックを解除」
    if (xg_bSolved) {
        ::EnableMenuItem(hMenu, ID_ERASESOLUTIONANDUNLOCKEDIT, MF_BYCOMMAND | MF_ENABLED);
    } else {
        ::EnableMenuItem(hMenu, ID_ERASESOLUTIONANDUNLOCKEDIT, MF_BYCOMMAND | MF_GRAYED);
    }

    // ステータスバーのメニュー更新。
    if (s_bShowStatusBar) {
        ::CheckMenuItem(hMenu, ID_STATUS, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_STATUS, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // 入力パレットのメニュー更新。
    if (xg_hwndInputPalette) {
        ::CheckMenuItem(hMenu, ID_PALETTE, MF_BYCOMMAND | MF_CHECKED);
        ::CheckMenuItem(hMenu, ID_PALETTE2, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_PALETTE, MF_BYCOMMAND | MF_UNCHECKED);
        ::CheckMenuItem(hMenu, ID_PALETTE2, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // ツールバーのメニュー更新。
    if (s_bShowToolBar) {
        ::CheckMenuItem(hMenu, ID_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // ヒントウィンドウのメニュー更新。
    if (xg_hHintsWnd) {
        ::CheckMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // ひらがなウィンドウのメニュー更新。
    if (xg_bHiragana) {
        ::CheckMenuRadioItem(hMenu, ID_HIRAGANA, ID_KATAKANA, ID_HIRAGANA, MF_BYCOMMAND);
    } else {
        ::CheckMenuRadioItem(hMenu, ID_HIRAGANA, ID_KATAKANA, ID_KATAKANA, MF_BYCOMMAND);
    }

    // Lowercaseウィンドウのメニュー更新。
    if (xg_bLowercase) {
        ::CheckMenuRadioItem(hMenu, ID_UPPERCASE, ID_LOWERCASE, ID_LOWERCASE, MF_BYCOMMAND);
    } else {
        ::CheckMenuRadioItem(hMenu, ID_UPPERCASE, ID_LOWERCASE, ID_UPPERCASE, MF_BYCOMMAND);
    }

    // タテヨコ入力のメニュー更新。
    if (xg_bTateInput) {
        ::CheckMenuRadioItem(hMenu, ID_INPUTH, ID_INPUTV, ID_INPUTV, MF_BYCOMMAND);
    } else {
        ::CheckMenuRadioItem(hMenu, ID_INPUTH, ID_INPUTV, ID_INPUTH, MF_BYCOMMAND);
    }

    // 文字送りのメニュー更新。
    if (xg_bCharFeed) {
        ::CheckMenuItem(hMenu, ID_CHARFEED, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_CHARFEED, MF_BYCOMMAND | MF_UNCHECKED);
    }
}

// マウスの左ボタンが離された。
void __fastcall MainWnd_OnLButtonUp(HWND hwnd, int x, int y, UINT /*keyFlags*/)
{
    int i, j;
    RECT rc;
    POINT pt;
    INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    // 左ボタンが離された位置を求める。
    pt.x = x + XgGetHScrollPos();
    pt.y = y + XgGetVScrollPos();

    // ツールバーが表示されていたら、位置を補正する。
    if (::IsWindowVisible(xg_hToolBar)) {
        ::GetWindowRect(xg_hToolBar, &rc);
        pt.y -= (rc.bottom - rc.top);
    }

    // それぞれのマスについて調べる。
    for (i = 0; i < xg_nRows; i++) {
        for (j = 0; j < xg_nCols; j++) {
            // マスの矩形を求める。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize),
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize),
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            // マスの中か？
            if (::PtInRect(&rc, pt)) {
                // キャレットを移動して、イメージを更新する。
                xg_caret_pos.m_i = i;
                xg_caret_pos.m_j = j;
                XgEnsureCaretVisible(hwnd);
                XgUpdateStatusBar(hwnd);

                // イメージを更新する。
                x = XgGetHScrollPos();
                y = XgGetVScrollPos();
                XgUpdateImage(hwnd, x, y);
                return;
            }
        }
    }
}

// ダブルクリックされた。
void __fastcall MainWnd_OnLButtonDown(HWND hwnd, bool fDoubleClick, int x, int y, UINT /*keyFlags*/)
{
    int i, j;
    RECT rc;
    POINT pt;
    INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    // ダブルクリックは無視。
    if (!fDoubleClick)
        return;

    // 左ボタンが離された位置を求める。
    pt.x = x + XgGetHScrollPos();
    pt.y = y + XgGetVScrollPos();

    // ツールバーが表示されていたら、位置を補正する。
    if (::IsWindowVisible(xg_hToolBar)) {
        ::GetWindowRect(xg_hToolBar, &rc);
        pt.y -= (rc.bottom - rc.top);
    }

    // それぞれのマスについて調べる。
    for (i = 0; i < xg_nRows; i++) {
        for (j = 0; j < xg_nCols; j++) {
            // マスの矩形を求める。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize),
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize),
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            // マスの中か？
            if (::PtInRect(&rc, pt)) {
                // キャレットを移動して、イメージを更新する。
                xg_caret_pos.m_i = i;
                xg_caret_pos.m_j = j;
                XgEnsureCaretVisible(hwnd);
                XgUpdateStatusBar(hwnd);

                // マークされていないか？
                if (XgGetMarked(i, j) == -1) {
                    // マークされていないマス。マークをセットする。
                    XG_Board *pxw;

                    if (xg_bSolved && xg_bShowAnswer)
                        pxw = &xg_solution;
                    else
                        pxw = &xg_xword;

                    if (pxw->GetAt(i, j) != ZEN_BLACK)
                        XgSetMark(i, j);
                    else
                        ::MessageBeep(0xFFFFFFFF);
                } else {
                    // マークがセットされているマス。マークを解除する。
                    XgDeleteMark(i, j);
                }

                // イメージを更新する。
                x = XgGetHScrollPos();
                y = XgGetVScrollPos();
                XgUpdateImage(hwnd, x, y);
                return;
            }
        }
    }
}

// ステータスバーを更新する。
void __fastcall XgUpdateStatusBar(HWND hwnd)
{
    // クライアント領域を取得する。
    RECT rc;
    GetClientRect(hwnd, &rc);

    // パーツのサイズを決定する。
    INT anWidth[] = { rc.right - 200, rc.right - 100, rc.right };

    // ステータスバーをパーツに分ける。
    SendMessageW(xg_hStatusBar, SB_SETPARTS, 3, (LPARAM)anWidth);

    // 状態文字列を設定。
    std::wstring str;
    if (xg_bTateInput) {
        str = XgLoadStringDx1(IDS_VINPUT3);
    } else {
        str = XgLoadStringDx1(IDS_HINPUT3);
    }
    str += L" - ";

    switch (xg_imode) {
    case xg_im_ABC: str += XgLoadStringDx1(IDS_ABC); break;
    case xg_im_KANA: str += XgLoadStringDx1(IDS_KANA); break;
    case xg_im_KANJI: str += XgLoadStringDx1(IDS_KANJI); break;
    case xg_im_RUSSIA: str += XgLoadStringDx1(IDS_RUSSIA); break;
    case xg_im_DIGITS: str += XgLoadStringDx1(IDS_DIGITS); break;
    default:
        break;
    }

    if (xg_bCharFeed) {
        str += L" - ";
        str += XgLoadStringDx1(IDS_CHARFEED);
    }

    // 状態を表示。
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 0, (LPARAM)str.c_str());

    // キャレット位置。
    WCHAR szText[64];
    StringCbPrintf(szText, sizeof(szText), L"(%u, %u)", xg_caret_pos.m_j + 1, xg_caret_pos.m_i + 1);
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 1, (LPARAM)szText);

    // 盤のサイズ。
    StringCbPrintf(szText, sizeof(szText), L"%u x %u", xg_nCols, xg_nRows);
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 2, (LPARAM)szText);
}

// サイズが変更された。
void __fastcall MainWnd_OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/)
{
    int x, y;

    // ツールバーが作成されていなければ、初期化前なので、無視。
    if (xg_hToolBar == NULL)
        return;

    // ステータスバーの高さを取得。
    INT cyStatus = 0;
    if (s_bShowStatusBar) {
        // ステータスバーの位置を自動で修正。
        ::SendMessageW(xg_hStatusBar, WM_SIZE, 0, 0);

        MRect rcStatus;
        ::GetWindowRect(xg_hStatusBar, &rcStatus);
        cyStatus = rcStatus.Height();

        XgUpdateStatusBar(hwnd);

        ::ShowWindow(xg_hSizeGrip, SW_HIDE);
    } else {
        ::ShowWindow(xg_hSizeGrip, SW_SHOWNOACTIVATE);
    }

    MRect rc, rcClient;

    // クライアント領域を計算する。
    ::GetClientRect(hwnd, &rcClient);
    x = rcClient.left;
    y = rcClient.top;
    INT cx = rcClient.Width(), cy = rcClient.Height();

    // ツールバーの高さを取得。
    INT cyToolBar = 0;
    if (s_bShowToolBar) {
        // ツールバーの位置を自動で修正。
        ::SendMessageW(xg_hToolBar, WM_SIZE, 0, 0);

        ::GetWindowRect(xg_hToolBar, &rc);
        cyToolBar = rc.Height();
    }

    // ツールバーが表示されていたら、位置を補正する。
    y += cyToolBar;
    cy -= cyToolBar;
    cy -= cyStatus;

    // スクロールバーのサイズを取得する。
    int cyHScrollBar = ::GetSystemMetrics(SM_CYHSCROLL);
    int cxVScrollBar = ::GetSystemMetrics(SM_CXVSCROLL);

    // 複数のウィンドウの位置とサイズをいっぺんに変更する。
    HDWP hDwp = ::BeginDeferWindowPos(3);
    if (hDwp) {
        ::DeferWindowPos(hDwp, xg_hHScrollBar, NULL,
            x, y + cy - cyHScrollBar,
            cx - cxVScrollBar, cyHScrollBar,
            SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        ::DeferWindowPos(hDwp, xg_hVScrollBar, NULL,
            cx - cxVScrollBar, y,
            cxVScrollBar, cy - cyHScrollBar,
            SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        if (::IsWindowVisible(xg_hSizeGrip)) {
            ::DeferWindowPos(hDwp, xg_hSizeGrip, NULL,
                x + cx - cxVScrollBar, y + cy - cyHScrollBar,
                cxVScrollBar, cyHScrollBar,
                SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        ::EndDeferWindowPos(hDwp);
    }

    // 再描画する。
    ::InvalidateRect(xg_hToolBar, NULL, TRUE);
    ::InvalidateRect(xg_hStatusBar, NULL, TRUE);
    ::InvalidateRect(xg_hHScrollBar, NULL, TRUE);
    ::InvalidateRect(xg_hVScrollBar, NULL, TRUE);
    ::InvalidateRect(xg_hSizeGrip, NULL, TRUE);

    // スクロール位置を取得し、スクロール情報を更新する。
    x = XgGetHScrollPos();
    y = XgGetVScrollPos();
    XgUpdateImage(hwnd, x, y);

    if (!IsZoomed(hwnd) && !IsIconic(hwnd))
    {
        // 設定の保存のために、ウィンドウの通常状態のときの幅と高さを格納しておく。
        WINDOWPLACEMENT wndpl;
        wndpl.length = sizeof(wndpl);
        ::GetWindowPlacement(hwnd, &wndpl);
        s_nMainWndCX = wndpl.rcNormalPosition.right - wndpl.rcNormalPosition.left;
        s_nMainWndCY = wndpl.rcNormalPosition.bottom - wndpl.rcNormalPosition.top;
    }
}

// 位置が変更された。
void __fastcall MainWnd_OnMove(HWND hwnd, int /*x*/, int /*y*/)
{
    if (!IsZoomed(hwnd) && !IsIconic(hwnd)) {
        MRect rc;
        ::GetWindowRect(hwnd, &rc);
        s_nMainWndX = rc.left;
        s_nMainWndY = rc.top;
    }
}

// 一時的に保存する色のデータ。
COLORREF s_rgbColors[3];

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

// [設定]ダイアログの初期化。
BOOL SettingsDlg_OnInitDialog(HWND hwnd)
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
        (s_bShowToolBar ? BST_CHECKED : BST_UNCHECKED));
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

    UpdateBlockPreview(hwnd);

    return TRUE;
}

// [設定]ダイアログで[OK]ボタンを押された。
void SettingsDlg_OnOK(HWND hwnd)
{
    // 辞書ファイルの保存モードを取得する。
    //s_nDictSaveMode = 
    //    static_cast<int>(::SendDlgItemMessageW(hwnd, cmb1, CB_GETCURSEL, 0, 0));
    s_nDictSaveMode = 2;

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

        if (!xg_hbmBlackCell && !xg_hBlackCellEMF)
        {
            // 画像が無効なら、パスも無効化。
            xg_strBlackCellImage.clear();
        }
    }

    // ツールバーを表示するか？
    s_bShowToolBar = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
    if (s_bShowToolBar)
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
        XgUpdateHintsData();
        XgOpenHintsByWindow(xg_hHintsWnd);
        ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
    }

    // イメージを更新する。
    int x = XgGetHScrollPos();
    int y = XgGetVScrollPos();
    XgUpdateImage(xg_hMainWnd, x, y);
}

// UIフォントの論理オブジェクトを取得する。
LOGFONTW *XgGetUIFont(void)
{
    static LOGFONTW s_lf;
    ::GetObjectW(::GetStockObject(DEFAULT_GUI_FONT), sizeof(s_lf), &s_lf);
    if (xg_szUIFont[0]) {
        WCHAR szData[LF_FACESIZE];
        StringCbCopy(szData, sizeof(szData), xg_szUIFont);
        LPWSTR pch = wcsrchr(szData, L',');
        if (pch) {
            *pch++ = 0;
            std::wstring name(szData);
            std::wstring size(pch);
            xg_str_trim(name);
            xg_str_trim(size);

            StringCbCopy(s_lf.lfFaceName, sizeof(s_lf.lfFaceName), name.data());

            HDC hdc = ::CreateCompatibleDC(NULL);
            int point_size = _wtoi(size.data());
            s_lf.lfHeight = -MulDiv(point_size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ::DeleteDC(hdc);
        } else {
            std::wstring name(szData);
            xg_str_trim(name);

            StringCbCopy(s_lf.lfFaceName, sizeof(s_lf.lfFaceName), name.data());
        }
    }
    return &s_lf;
}

// UIフォントの論理オブジェクトを設定する。
void SettingsDlg_SetUIFont(HWND hwnd, const LOGFONTW *plf)
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
void SettingsDlg_OnChange(HWND hwnd, int i)
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
            SettingsDlg_SetUIFont(hwnd, cf.lpLogFont);
        }
        break;
    }
}

// [設定]ダイアログで[リセット]ボタンを押された。
void SettingsDlg_OnReset(HWND hwnd, int i)
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
void SettingDlg_OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
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

COLORREF s_rgbColorTable[] = {
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

// 色を指定する。
void SettingsDlg_OnSetColor(HWND hwnd, int nIndex)
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
INT_PTR CALLBACK
XgSettingsDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        return SettingsDlg_OnInitDialog(hwnd);

    case WM_DRAWITEM:
        SettingDlg_OnDrawItem(hwnd, wParam, lParam);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            SettingsDlg_OnOK(hwnd);
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh1:
            SettingsDlg_OnChange(hwnd, 0);
            break;

        case psh2:
            SettingsDlg_OnChange(hwnd, 1);
            break;

        case psh3:
            SettingsDlg_OnReset(hwnd, 0);
            break;

        case psh4:
            SettingsDlg_OnReset(hwnd, 1);
            break;

        case psh5:
            SettingsDlg_OnChange(hwnd, 2);
            break;

        case psh6:
            SettingsDlg_OnReset(hwnd, 2);
            break;

        case psh7:
            SettingsDlg_OnSetColor(hwnd, 0);
            break;

        case psh8:
            SettingsDlg_OnSetColor(hwnd, 1);
            break;

        case psh9:
            SettingsDlg_OnSetColor(hwnd, 2);
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

// 設定。
void MainWnd_OnSettings(HWND hwnd)
{
    XgDestroyCandsWnd();
    ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_CONFIG), hwnd, XgSettingsDlgProc);
}

// テーマが変更された。
void XgUpdateTheme(HWND hwnd)
{
    std::unordered_set<std::wstring> priority, forbidden;
    XgParseTheme(priority, forbidden, xg_strDefaultTheme);
    xg_bThemeModified = (priority != xg_priority_tags || forbidden != xg_forbidden_tags);

    HMENU hMenu = ::GetMenu(hwnd);
    INT nCount = ::GetMenuItemCount(hMenu);
    assert(nCount > 0);
    WCHAR szText[32];
    MENUITEMINFOW info = { sizeof(info) };
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    for (INT i = 0; i < nCount; ++i) {
        szText[0] = 0;
        ::GetMenuStringW(hMenu, i, szText, ARRAYSIZE(szText), MF_BYPOSITION);
        if (wcsstr(szText, XgLoadStringDx1(IDS_DICT)) != NULL) {
            if (xg_bThemeModified) {
                StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_MODIFIEDDICT));
            } else {
                StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_DEFAULTDICT));
            }
            info.dwTypeData = szText;
            SetMenuItemInfoW(hMenu, i, TRUE, &info);
            break;
        }
    }
    // メニューバーを再描画。
    ::DrawMenuBar(hwnd);
}

// ルールが変更された。
void XgUpdateRules(HWND hwnd)
{
    HMENU hMenu = ::GetMenu(hwnd);
    INT nCount = ::GetMenuItemCount(hMenu);
    WCHAR szText[32];
    MENUITEMINFOW info = { sizeof(info) };
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    for (INT i = nCount - 1; i >= 0; --i) {
        szText[0] = 0;
        ::GetMenuStringW(hMenu, i, szText, ARRAYSIZE(szText), MF_BYPOSITION);
        if (wcsstr(szText, XgLoadStringDx1(IDS_RULES)) != NULL) {
            if (XgIsUserJapanese()) {
                if (xg_nRules == DEFAULT_RULES_JAPANESE) {
                    StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_STANDARDRULES));
                } else {
                    StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_MODIFIEDRULES));
                }
            } else {
                if (xg_nRules == DEFAULT_RULES_ENGLISH) {
                    StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_STANDARDRULES));
                } else {
                    StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_MODIFIEDRULES));
                }
            }
            info.dwTypeData = szText;
            SetMenuItemInfoW(hMenu, i, TRUE, &info);
            break;
        }
    }
    // メニューバーを再描画。
    ::DrawMenuBar(hwnd);
}

// 設定を消去する。
void MainWnd_OnEraseSettings(HWND hwnd)
{
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // 消去するのか確認。
    if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_QUERYERASESETTINGS), XgLoadStringDx2(IDS_APPNAME),
                            MB_ICONWARNING | MB_YESNO) != IDYES)
    {
        return;
    }

    // 設定を消去する。
    bool bSuccess = XgEraseSettings();

    // 初期化する。
    XgLoadSettings();
    XgUpdateRules(hwnd);
    // 辞書ファイルの名前を読み込む。
    XgLoadDictsAll();

    xg_bSolved = false;
    xg_bHintsAdded = false;
    xg_bShowAnswer = false;
    xg_xword.clear();
    xg_solution.clear();
    xg_caret_pos.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_strFileName.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    xg_vMarks.clear();
    xg_vMarkedCands.clear();
    XgMarkUpdate();

    // ツールバーの表示を切り替える。
    if (s_bShowToolBar)
        ::ShowWindow(xg_hToolBar, SW_SHOWNOACTIVATE);
    else
        ::ShowWindow(xg_hToolBar, SW_HIDE);

    // ツールバーの表示を切り替える。
    if (s_bShowStatusBar)
        ::ShowWindow(xg_hStatusBar, SW_SHOWNOACTIVATE);
    else
        ::ShowWindow(xg_hStatusBar, SW_HIDE);

    // イメージを更新する。
    XgUpdateImage(hwnd, 0, 0);
    // テーマを初期化する。
    XgResetTheme(hwnd);
    XgUpdateTheme(hwnd);

    if (bSuccess) {
        // メッセージを表示する。
        XgCenterMessageBoxW(hwnd,
            XgLoadStringDx1(IDS_ERASEDSETTINGS), XgLoadStringDx2(IDS_APPNAME),
            MB_ICONINFORMATION);
    } else {
        // メッセージを表示する。
        XgCenterMessageBoxW(hwnd,
            XgLoadStringDx1(IDS_FAILERASESETTINGS), XgLoadStringDx2(IDS_APPNAME),
            MB_ICONINFORMATION);
    }
}

// 辞書を読み込む。
extern "C"
INT_PTR CALLBACK
XgLoadDictDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int i;
    WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
    std::wstring strFile;
    HWND hCmb1;
    COMBOBOXEXITEMW item;
    OPENFILENAMEW ofn;
    HDROP hDrop;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 辞書ファイルをすべて読み込む。
        XgLoadDictsAll();
        // 辞書ファイルのパス名のベクターをコンボボックスに設定する。
        hCmb1 = GetDlgItem(hwnd, cmb1);
        for (const auto& dict_file : xg_dict_files) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCbCopy(szFile, sizeof(szFile), dict_file.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendMessageW(hCmb1, CBEM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item));
        }
        ComboBox_SetText(hCmb1, xg_dict_name.c_str());

        // ドラッグ＆ドロップを受け付ける。
        ::DragAcceptFiles(hwnd, TRUE);
        return TRUE;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile, MAX_PATH);
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile, szTarget)) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            StringCbCopy(szFile, sizeof(szFile), szTarget);
        }

        // 同じ項目がすでにあれば、削除する。
        i = static_cast<int>(::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                                                 reinterpret_cast<LPARAM>(szFile)));
        if (i != CB_ERR) {
            ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
        }
        // コンボボックスの最初に挿入する。
        item.mask = CBEIF_TEXT;
        item.iItem = 0;
        item.pszText = szFile;
        item.cchTextMax = -1;
        ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                            reinterpret_cast<LPARAM>(&item));
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // 辞書ファイルのパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb1, szFile, ARRAYSIZE(szFile));
            strFile = szFile;
            xg_str_trim(strFile);
            // 正しく読み込めるか？
            if (XgLoadDictFile(strFile.data())) {
                // 読み込めた。
                XgSetDict(strFile);

                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            } else {
                // 読み込めなかったのでエラーを表示する。
                ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, cmb1));
            }
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh1:  // [参照]ボタン。
            // ユーザーに辞書ファイルの場所を問い合わせる。
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
            ofn.hwndOwner = hwnd;
            ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_DICTFILTER));
            szFile[0] = 0;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = XgLoadStringDx1(IDS_OPENDICTDATA);
            ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = L"dic";
            if (::GetOpenFileNameW(&ofn))
            {
                // コンボボックスにテキストを設定。
                ::SetDlgItemTextW(hwnd, cmb1, szFile);
            }
            break;
        }
    }
    return 0;
}

// [ヘッダーと備考欄]ダイアログ。
extern "C"
INT_PTR CALLBACK
XgNotesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WCHAR sz[512];
    std::wstring str;
    LPWSTR psz;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // ヘッダーを設定する。
        ::SetDlgItemTextW(hwnd, edt1, xg_strHeader.data());
        // 備考欄を設定する。
        str = xg_strNotes;
        xg_str_trim(str);
        psz = XgLoadStringDx1(IDS_BELOWISNOTES);
        if (str.find(psz) == 0) {
            str = str.substr(::lstrlenW(psz));
        }
        ::SetDlgItemTextW(hwnd, edt2, str.data());
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // ヘッダーを取得する。
            ::GetDlgItemTextW(hwnd, edt1, sz, static_cast<int>(ARRAYSIZE(sz)));
            str = sz;
            xg_str_trim(str);
            xg_strHeader = str;

            // 備考欄を取得する。
            ::GetDlgItemTextW(hwnd, edt2, sz, static_cast<int>(ARRAYSIZE(sz)));
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
        }
    }
    return 0;
}

// 縦と横を入れ替える。
void __fastcall MainWnd_OnFlipVH(HWND hwnd)
{
    xg_xword.SwapXandY();
    if (xg_bSolved) {
        xg_solution.SwapXandY();
    }
    std::swap(xg_nRows, xg_nCols);
    if (xg_bSolved) {
        auto old_dict_1 = xg_dict_1;
        auto old_dict_2 = xg_dict_2;
        xg_dict_1 = XgCreateMiniDict();
        xg_dict_2.clear();
        xg_solution.DoNumbering();
        xg_solution.GetHintsStr(xg_strHints, 2, true);
        if (!XgParseHintsStr(hwnd, xg_strHints)) {
            xg_strHints.clear();
        }
        std::swap(xg_dict_1, old_dict_1);
        std::swap(xg_dict_2, old_dict_2);
    }
    std::swap(xg_caret_pos.m_i, xg_caret_pos.m_j);
    for (auto& mark : xg_vMarks) {
        std::swap(mark.m_i, mark.m_j);
    }
    // イメージを更新する。
    XgUpdateImage(hwnd, 0, 0);
}

std::wstring URL_encode(const std::wstring& url)
{
    std::string str;

    size_t len = url.size() * 4;
    str.resize(len);
    if (len > 0)
        WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, &str[0], (INT)len, NULL, NULL);

    len = strlen(str.c_str());
    str.resize(len);

    std::wstring ret;
    WCHAR buf[4];
    static const WCHAR s_hex[] = L"0123456789ABCDEF";
    for (size_t i = 0; i < str.size(); ++i)
    {
        using namespace std;
        if (str[i] == ' ')
        {
            ret += L'+';
        }
        else if (isalnum(str[i]))
        {
            ret += (char)str[i];
        }
        else
        {
            switch (str[i])
            {
            case L'.':
            case L'-':
            case L'_':
            case L'*':
                ret += (char)str[i];
                break;
            default:
                buf[0] = L'%';
                buf[1] = s_hex[(str[i] >> 4) & 0xF];
                buf[2] = s_hex[str[i] & 0xF];
                buf[3] = 0;
                ret += buf;
                break;
            }
        }
    }

    return ret;
}

// ウェブ検索。
void DoWebSearch(HWND hwnd, LPCWSTR str)
{
    std::wstring query = XgLoadStringDx1(IDS_GOOGLESEARCH);
    std::wstring raw = str;
    switch (xg_imode)
    {
    case xg_im_ABC:
        raw += L" ";
        raw += XgLoadStringDx2(IDS_ABC);
        break;
    case xg_im_KANA:
    case xg_im_KANJI:
        raw += L" ";
        raw += XgLoadStringDx2(IDS_DICTIONARY);
        break;
    case xg_im_RUSSIA:
    case xg_im_DIGITS:
        break;
    default:
        break;
    }
    std::wstring encoded = URL_encode(raw.c_str());
    query += encoded;

    ::ShellExecuteW(hwnd, NULL, query.c_str(), NULL, NULL, SW_SHOWNORMAL);
}

void __fastcall MainWnd_OnCopyPattern(HWND hwnd, BOOL bTate)
{
    XG_Board *pxword;
    if (xg_bSolved && xg_bShowAnswer) {
        pxword = &xg_solution;
    } else {
        pxword = &xg_xword;
    }

    // パターンを取得する。
    std::wstring pattern;
    if (bTate) {
        pattern = pxword->GetPatternV(xg_caret_pos);
    } else {
        pattern = pxword->GetPatternH(xg_caret_pos);
    }

    XgSetClipboardUnicodeText(hwnd, pattern);
}

void __fastcall MainWnd_OnCopyPatternHorz(HWND hwnd)
{
    MainWnd_OnCopyPattern(hwnd, FALSE);
}

void __fastcall MainWnd_OnCopyPatternVert(HWND hwnd)
{
    MainWnd_OnCopyPattern(hwnd, TRUE);
}

// オンライン辞書を引く。
void __fastcall XgOnlineDict(HWND hwnd, BOOL bTate)
{
    XG_Board *pxword;
    if (xg_bSolved && xg_bShowAnswer) {
        pxword = &xg_solution;
    } else {
        pxword = &xg_xword;
    }

    // パターンを取得する。
    std::wstring pattern;
    if (bTate) {
        pattern = pxword->GetPatternV(xg_caret_pos);
    } else {
        pattern = pxword->GetPatternH(xg_caret_pos);
    }

    // 空白を含んでいたら、無視。
    if (pattern.find(ZEN_SPACE) != std::wstring::npos) {
        return;
    }

    DoWebSearch(hwnd, pattern.c_str());
}

bool __fastcall MainWnd_OnCommand2(HWND hwnd, INT id)
{
    bool bOK = false;

    bool bOldFeed = xg_bCharFeed;
    xg_bCharFeed = false;
    switch (id)
    {
    // kana
    case 10000: MainWnd_OnImeChar(hwnd, ZEN_A, 0); bOK = true; break;
    case 10001: MainWnd_OnImeChar(hwnd, ZEN_I, 0); bOK = true; break;
    case 10002: MainWnd_OnImeChar(hwnd, ZEN_U, 0); bOK = true; break;
    case 10003: MainWnd_OnImeChar(hwnd, ZEN_E, 0); bOK = true; break;
    case 10004: MainWnd_OnImeChar(hwnd, ZEN_O, 0); bOK = true; break;
    case 10010: MainWnd_OnImeChar(hwnd, ZEN_KA, 0); bOK = true; break;
    case 10011: MainWnd_OnImeChar(hwnd, ZEN_KI, 0); bOK = true; break;
    case 10012: MainWnd_OnImeChar(hwnd, ZEN_KU, 0); bOK = true; break;
    case 10013: MainWnd_OnImeChar(hwnd, ZEN_KE, 0); bOK = true; break;
    case 10014: MainWnd_OnImeChar(hwnd, ZEN_KO, 0); bOK = true; break;
    case 10020: MainWnd_OnImeChar(hwnd, ZEN_SA, 0); bOK = true; break;
    case 10021: MainWnd_OnImeChar(hwnd, ZEN_SI, 0); bOK = true; break;
    case 10022: MainWnd_OnImeChar(hwnd, ZEN_SU, 0); bOK = true; break;
    case 10023: MainWnd_OnImeChar(hwnd, ZEN_SE, 0); bOK = true; break;
    case 10024: MainWnd_OnImeChar(hwnd, ZEN_SO, 0); bOK = true; break;
    case 10030: MainWnd_OnImeChar(hwnd, ZEN_TA, 0); bOK = true; break;
    case 10031: MainWnd_OnImeChar(hwnd, ZEN_CHI, 0); bOK = true; break;
    case 10032: MainWnd_OnImeChar(hwnd, ZEN_TSU, 0); bOK = true; break;
    case 10033: MainWnd_OnImeChar(hwnd, ZEN_TE, 0); bOK = true; break;
    case 10034: MainWnd_OnImeChar(hwnd, ZEN_TO, 0); bOK = true; break;
    case 10040: MainWnd_OnImeChar(hwnd, ZEN_NA, 0); bOK = true; break;
    case 10041: MainWnd_OnImeChar(hwnd, ZEN_NI, 0); bOK = true; break;
    case 10042: MainWnd_OnImeChar(hwnd, ZEN_NU, 0); bOK = true; break;
    case 10043: MainWnd_OnImeChar(hwnd, ZEN_NE, 0); bOK = true; break;
    case 10044: MainWnd_OnImeChar(hwnd, ZEN_NO, 0); bOK = true; break;
    case 10050: MainWnd_OnImeChar(hwnd, ZEN_HA, 0); bOK = true; break;
    case 10051: MainWnd_OnImeChar(hwnd, ZEN_HI, 0); bOK = true; break;
    case 10052: MainWnd_OnImeChar(hwnd, ZEN_FU, 0); bOK = true; break;
    case 10053: MainWnd_OnImeChar(hwnd, ZEN_HE, 0); bOK = true; break;
    case 10054: MainWnd_OnImeChar(hwnd, ZEN_HO, 0); bOK = true; break;
    case 10060: MainWnd_OnImeChar(hwnd, ZEN_MA, 0); bOK = true; break;
    case 10061: MainWnd_OnImeChar(hwnd, ZEN_MI, 0); bOK = true; break;
    case 10062: MainWnd_OnImeChar(hwnd, ZEN_MU, 0); bOK = true; break;
    case 10063: MainWnd_OnImeChar(hwnd, ZEN_ME, 0); bOK = true; break;
    case 10064: MainWnd_OnImeChar(hwnd, ZEN_MO, 0); bOK = true; break;
    case 10070: MainWnd_OnImeChar(hwnd, ZEN_YA, 0); bOK = true; break;
    case 10071: MainWnd_OnImeChar(hwnd, ZEN_YU, 0); bOK = true; break;
    case 10072: MainWnd_OnImeChar(hwnd, ZEN_YO, 0); bOK = true; break;
    case 10080: MainWnd_OnImeChar(hwnd, ZEN_RA, 0); bOK = true; break;
    case 10081: MainWnd_OnImeChar(hwnd, ZEN_RI, 0); bOK = true; break;
    case 10082: MainWnd_OnImeChar(hwnd, ZEN_RU, 0); bOK = true; break;
    case 10083: MainWnd_OnImeChar(hwnd, ZEN_RE, 0); bOK = true; break;
    case 10084: MainWnd_OnImeChar(hwnd, ZEN_RO, 0); bOK = true; break;
    case 10090: MainWnd_OnImeChar(hwnd, ZEN_WA, 0); bOK = true; break;
    case 10091: MainWnd_OnImeChar(hwnd, ZEN_NN, 0); bOK = true; break;
    case 10092: MainWnd_OnImeChar(hwnd, ZEN_PROLONG, 0); bOK = true; break;
    case 10100: MainWnd_OnImeChar(hwnd, ZEN_GA, 0); bOK = true; break;
    case 10101: MainWnd_OnImeChar(hwnd, ZEN_GI, 0); bOK = true; break;
    case 10102: MainWnd_OnImeChar(hwnd, ZEN_GU, 0); bOK = true; break;
    case 10103: MainWnd_OnImeChar(hwnd, ZEN_GE, 0); bOK = true; break;
    case 10104: MainWnd_OnImeChar(hwnd, ZEN_GO, 0); bOK = true; break;
    case 10110: MainWnd_OnImeChar(hwnd, ZEN_ZA, 0); bOK = true; break;
    case 10111: MainWnd_OnImeChar(hwnd, ZEN_JI, 0); bOK = true; break;
    case 10112: MainWnd_OnImeChar(hwnd, ZEN_ZU, 0); bOK = true; break;
    case 10113: MainWnd_OnImeChar(hwnd, ZEN_ZE, 0); bOK = true; break;
    case 10114: MainWnd_OnImeChar(hwnd, ZEN_ZO, 0); bOK = true; break;
    case 10120: MainWnd_OnImeChar(hwnd, ZEN_DA, 0); bOK = true; break;
    case 10121: MainWnd_OnImeChar(hwnd, ZEN_DI, 0); bOK = true; break;
    case 10122: MainWnd_OnImeChar(hwnd, ZEN_DU, 0); bOK = true; break;
    case 10123: MainWnd_OnImeChar(hwnd, ZEN_DE, 0); bOK = true; break;
    case 10124: MainWnd_OnImeChar(hwnd, ZEN_DO, 0); bOK = true; break;
    case 10130: MainWnd_OnImeChar(hwnd, ZEN_BA, 0); bOK = true; break;
    case 10131: MainWnd_OnImeChar(hwnd, ZEN_BI, 0); bOK = true; break;
    case 10132: MainWnd_OnImeChar(hwnd, ZEN_BU, 0); bOK = true; break;
    case 10133: MainWnd_OnImeChar(hwnd, ZEN_BE, 0); bOK = true; break;
    case 10134: MainWnd_OnImeChar(hwnd, ZEN_BO, 0); bOK = true; break;
    case 10140: MainWnd_OnImeChar(hwnd, ZEN_PA, 0); bOK = true; break;
    case 10141: MainWnd_OnImeChar(hwnd, ZEN_PI, 0); bOK = true; break;
    case 10142: MainWnd_OnImeChar(hwnd, ZEN_PU, 0); bOK = true; break;
    case 10143: MainWnd_OnImeChar(hwnd, ZEN_PE, 0); bOK = true; break;
    case 10144: MainWnd_OnImeChar(hwnd, ZEN_PO, 0); bOK = true; break;
    case 10150: MainWnd_OnImeChar(hwnd, ZEN_PROLONG, 0); bOK = true; break;
    // ABC
    case 20000: MainWnd_OnChar(hwnd, L'A', 1); bOK = true; break;
    case 20001: MainWnd_OnChar(hwnd, L'B', 1); bOK = true; break;
    case 20002: MainWnd_OnChar(hwnd, L'C', 1); bOK = true; break;
    case 20003: MainWnd_OnChar(hwnd, L'D', 1); bOK = true; break;
    case 20004: MainWnd_OnChar(hwnd, L'E', 1); bOK = true; break;
    case 20005: MainWnd_OnChar(hwnd, L'F', 1); bOK = true; break;
    case 20006: MainWnd_OnChar(hwnd, L'G', 1); bOK = true; break;
    case 20007: MainWnd_OnChar(hwnd, L'H', 1); bOK = true; break;
    case 20008: MainWnd_OnChar(hwnd, L'I', 1); bOK = true; break;
    case 20009: MainWnd_OnChar(hwnd, L'J', 1); bOK = true; break;
    case 20010: MainWnd_OnChar(hwnd, L'K', 1); bOK = true; break;
    case 20011: MainWnd_OnChar(hwnd, L'L', 1); bOK = true; break;
    case 20012: MainWnd_OnChar(hwnd, L'M', 1); bOK = true; break;
    case 20013: MainWnd_OnChar(hwnd, L'N', 1); bOK = true; break;
    case 20014: MainWnd_OnChar(hwnd, L'O', 1); bOK = true; break;
    case 20015: MainWnd_OnChar(hwnd, L'P', 1); bOK = true; break;
    case 20016: MainWnd_OnChar(hwnd, L'Q', 1); bOK = true; break;
    case 20017: MainWnd_OnChar(hwnd, L'R', 1); bOK = true; break;
    case 20018: MainWnd_OnChar(hwnd, L'S', 1); bOK = true; break;
    case 20019: MainWnd_OnChar(hwnd, L'T', 1); bOK = true; break;
    case 20020: MainWnd_OnChar(hwnd, L'U', 1); bOK = true; break;
    case 20021: MainWnd_OnChar(hwnd, L'V', 1); bOK = true; break;
    case 20022: MainWnd_OnChar(hwnd, L'W', 1); bOK = true; break;
    case 20023: MainWnd_OnChar(hwnd, L'X', 1); bOK = true; break;
    case 20024: MainWnd_OnChar(hwnd, L'Y', 1); bOK = true; break;
    case 20025: MainWnd_OnChar(hwnd, L'Z', 1); bOK = true; break;
    // Russian
    case 30000: MainWnd_OnImeChar(hwnd, 0x0410, 0); bOK = true; break;
    case 30001: MainWnd_OnImeChar(hwnd, 0x0411, 0); bOK = true; break;
    case 30002: MainWnd_OnImeChar(hwnd, 0x0412, 0); bOK = true; break;
    case 30003: MainWnd_OnImeChar(hwnd, 0x0413, 0); bOK = true; break;
    case 30004: MainWnd_OnImeChar(hwnd, 0x0414, 0); bOK = true; break;
    case 30005: MainWnd_OnImeChar(hwnd, 0x0415, 0); bOK = true; break;
    case 30006: MainWnd_OnImeChar(hwnd, 0x0401, 0); bOK = true; break;
    case 30007: MainWnd_OnImeChar(hwnd, 0x0416, 0); bOK = true; break;
    case 30008: MainWnd_OnImeChar(hwnd, 0x0417, 0); bOK = true; break;
    case 30009: MainWnd_OnImeChar(hwnd, 0x0418, 0); bOK = true; break;
    case 30010: MainWnd_OnImeChar(hwnd, 0x0419, 0); bOK = true; break;
    case 30011: MainWnd_OnImeChar(hwnd, 0x041A, 0); bOK = true; break;
    case 30012: MainWnd_OnImeChar(hwnd, 0x041B, 0); bOK = true; break;
    case 30013: MainWnd_OnImeChar(hwnd, 0x041C, 0); bOK = true; break;
    case 30014: MainWnd_OnImeChar(hwnd, 0x041D, 0); bOK = true; break;
    case 30015: MainWnd_OnImeChar(hwnd, 0x041E, 0); bOK = true; break;
    case 30016: MainWnd_OnImeChar(hwnd, 0x041F, 0); bOK = true; break;
    case 30017: MainWnd_OnImeChar(hwnd, 0x0420, 0); bOK = true; break;
    case 30018: MainWnd_OnImeChar(hwnd, 0x0421, 0); bOK = true; break;
    case 30019: MainWnd_OnImeChar(hwnd, 0x0422, 0); bOK = true; break;
    case 30020: MainWnd_OnImeChar(hwnd, 0x0423, 0); bOK = true; break;
    case 30021: MainWnd_OnImeChar(hwnd, 0x0424, 0); bOK = true; break;
    case 30022: MainWnd_OnImeChar(hwnd, 0x0425, 0); bOK = true; break;
    case 30023: MainWnd_OnImeChar(hwnd, 0x0426, 0); bOK = true; break;
    case 30024: MainWnd_OnImeChar(hwnd, 0x0427, 0); bOK = true; break;
    case 30025: MainWnd_OnImeChar(hwnd, 0x0428, 0); bOK = true; break;
    case 30026: MainWnd_OnImeChar(hwnd, 0x0429, 0); bOK = true; break;
    case 30027: MainWnd_OnImeChar(hwnd, 0x042A, 0); bOK = true; break;
    case 30028: MainWnd_OnImeChar(hwnd, 0x042B, 0); bOK = true; break;
    case 30029: MainWnd_OnImeChar(hwnd, 0x042C, 0); bOK = true; break;
    case 30030: MainWnd_OnImeChar(hwnd, 0x042D, 0); bOK = true; break;
    case 30031: MainWnd_OnImeChar(hwnd, 0x042E, 0); bOK = true; break;
    case 30032: MainWnd_OnImeChar(hwnd, 0x042F, 0); bOK = true; break;
    // Digits
    case 40000: MainWnd_OnImeChar(hwnd, L'0', 0); bOK = true; break;
    case 40001: MainWnd_OnImeChar(hwnd, L'1', 0); bOK = true; break;
    case 40002: MainWnd_OnImeChar(hwnd, L'2', 0); bOK = true; break;
    case 40003: MainWnd_OnImeChar(hwnd, L'3', 0); bOK = true; break;
    case 40004: MainWnd_OnImeChar(hwnd, L'4', 0); bOK = true; break;
    case 40005: MainWnd_OnImeChar(hwnd, L'5', 0); bOK = true; break;
    case 40006: MainWnd_OnImeChar(hwnd, L'6', 0); bOK = true; break;
    case 40007: MainWnd_OnImeChar(hwnd, L'7', 0); bOK = true; break;
    case 40008: MainWnd_OnImeChar(hwnd, L'8', 0); bOK = true; break;
    case 40009: MainWnd_OnImeChar(hwnd, L'9', 0); bOK = true; break;
    default:
        break;
    }
    xg_bCharFeed = bOldFeed;

    return bOK;
}

// 黒マスパターンの構造体。
struct PATDATA
{
    int num_columns;
    int num_rows;
    std::wstring data;
};

// 黒マスパターンのデータ。
static std::vector<PATDATA> s_patterns;
static LAYOUT_DATA *s_pLayout = NULL;

// パターンのテキストデータを扱いやすいよう、加工する。
static void
XgConvertPatternData(std::vector<WCHAR>& data, std::wstring text, INT cx, INT cy)
{
    xg_str_replace_all(text, L"\r\n", L"\n");
    xg_str_replace_all(text, L"\u2501", L"");
    xg_str_replace_all(text, L"\u250F\u2513\n", L"");
    xg_str_replace_all(text, L"\u2517\u251B\n", L"");
    data.resize(cx * cy);
    int x = 0, y = 0;
    for (auto& ch : text)
    {
        switch (ch)
        {
        case ZEN_BLACK:
            data[x + cx * y] = ZEN_BLACK;
            ++x;
            break;
        case ZEN_SPACE:
            data[x + cx * y] = ZEN_SPACE;
            ++x;
            break;
        case L'\n':
            x = 0;
            ++y;
            break;
        }
        if (y == cy)
            break;
    }
}

static BOOL
XgPattern_RefreshContents(HWND hwnd, INT type)
{
    // パターンデータをクリアする。
    s_patterns.clear();
    ListBox_ResetContent(GetDlgItem(hwnd, lst1));

    // パターンデータを読み込む。
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    PathRemoveFileSpecW(szPath);
    WCHAR szFile[MAX_PATH];
    StringCbCopyW(szFile, sizeof(szFile), szPath);
    PathAppendW(szFile, L"pat\\data.json");
    if (!PathFileExistsW(szFile))
    {
        StringCbCopyW(szFile, sizeof(szFile), szPath);
        PathAppendW(szFile, L"..\\pat\\data.json");
        if (!PathFileExistsW(szFile))
        {
            StringCbCopyW(szFile, sizeof(szFile), szPath);
            PathAppendW(szFile, L"..\\..\\pat\\data.json");
        }
    }
    std::string utf8;
    CHAR buf[256];
    if (FILE *fp = _wfopen(szFile, L"rb"))
    {
        while (fgets(buf, 256, fp))
        {
            utf8 += buf;
        }
        fclose(fp);
    }
    else
    {
        return FALSE;
    }

    json j = json::parse(utf8);
    for (auto& item : j)
    {
        PATDATA pat;
        pat.num_columns = item["num_columns"];
        pat.num_rows = item["num_rows"];

        // タイプによりフィルターを行う。
        switch (type)
        {
        case rad1:
            if (pat.num_columns != pat.num_rows)
                continue;
            if (!(pat.num_columns >= 13 && pat.num_rows >= 13))
                continue;
            break;
        case rad2:
            if (pat.num_columns != pat.num_rows)
                continue;
            if (!(8 <= pat.num_columns && pat.num_columns <= 12 &&
                  8 <= pat.num_rows && pat.num_rows <= 12))
            {
                continue;
            }
            break;
        case rad3:
            if (pat.num_columns != pat.num_rows)
                continue;
            if (!(pat.num_columns <= 8 && pat.num_rows <= 8))
                continue;
            break;
        case rad4:
            if (pat.num_columns <= pat.num_rows)
                continue;
            break;
        case rad5:
            if (pat.num_columns >= pat.num_rows)
                continue;
            break;
        case rad6:
            if (pat.num_columns != pat.num_rows)
                continue;
            break;
        }

        // dataをテキストデータにする。
        std::string str;
        for (auto& subitem : item["data"])
        {
            str += subitem;
            str += "\r\n";
        }
        pat.data = XgUtf8ToUnicode(str);

        // パターンのテキストデータを扱いやすいよう、加工する。
        std::vector<WCHAR> data;
        XgConvertPatternData(data, pat.data, pat.num_columns, pat.num_rows);

        // 黒マスルールを適合する。
#define GET_DATA(x, y) data[(y) * pat.num_columns + (x)]
        if (xg_nRules & RULE_DONTDOUBLEBLACK) {
            BOOL bFound = FALSE;
            for (INT y = 0; y < pat.num_rows; ++y) {
                for (INT x = 0; x < pat.num_columns - 1; ++x) {
                    if (GET_DATA(x, y) == ZEN_BLACK && GET_DATA(x + 1, y) == ZEN_BLACK) {
                        x = pat.num_columns;
                        y = pat.num_rows;
                        bFound = TRUE;
                    }
                }
            }
            if (bFound)
                continue;
            bFound = FALSE;
            for (INT x = 0; x < pat.num_columns; ++x) {
                for (INT y = 0; y < pat.num_rows - 1; ++y) {
                    if (GET_DATA(x, y) == ZEN_BLACK && GET_DATA(x, y + 1) == ZEN_BLACK) {
                        x = pat.num_columns;
                        y = pat.num_rows;
                        bFound = TRUE;
                    }
                }
            }
            if (bFound)
                continue;
        }
        if (xg_nRules & RULE_DONTCORNERBLACK) {
            if (GET_DATA(0, 0) == ZEN_BLACK)
                continue;
            if (GET_DATA(pat.num_columns - 1, 0) == ZEN_BLACK)
                continue;
            if (GET_DATA(pat.num_columns - 1, pat.num_rows - 1) == ZEN_BLACK)
                continue;
            if (GET_DATA(0, pat.num_rows - 1) == ZEN_BLACK)
                continue;
        }
        //if (xg_nRules & RULE_DONTDIVIDE)
        if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
            BOOL bFound = FALSE;
            for (INT y = 0; y < pat.num_rows - 2; ++y) {
                for (INT x = 0; x < pat.num_columns - 2; ++x) {
                    if (GET_DATA(x, y) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x + 1, y + 1) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x + 2, y + 2) != ZEN_BLACK)
                        continue;
                    x = pat.num_columns;
                    y = pat.num_rows;
                    bFound = TRUE;
                }
            }
            if (bFound)
                continue;
            bFound = FALSE;
            for (INT y = 0; y < pat.num_rows - 2; ++y) {
                for (INT x = 2; x < pat.num_columns; ++x) {
                    if (GET_DATA(x, y) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x - 1, y + 1) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x - 2, y + 2) != ZEN_BLACK)
                        continue;
                    x = pat.num_columns;
                    y = pat.num_rows;
                    bFound = TRUE;
                }
            }
            if (bFound)
                continue;
        } else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
            BOOL bFound = FALSE;
            for (INT y = 0; y < pat.num_rows - 3; ++y) {
                for (INT x = 0; x < pat.num_columns - 3; ++x) {
                    if (GET_DATA(x, y) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x + 1, y + 1) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x + 2, y + 2) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x + 3, y + 3) != ZEN_BLACK)
                        continue;
                    x = pat.num_columns;
                    y = pat.num_rows;
                    bFound = TRUE;
                }
            }
            if (bFound)
                continue;
            bFound = FALSE;
            for (INT y = 0; y < pat.num_rows - 3; ++y) {
                for (INT x = 3; x < pat.num_columns; ++x) {
                    if (GET_DATA(x, y) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x - 1, y + 1) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x - 2, y + 2) != ZEN_BLACK)
                        continue;
                    if (GET_DATA(x - 3, y + 3) != ZEN_BLACK)
                        continue;
                    x = pat.num_columns;
                    y = pat.num_rows;
                    bFound = TRUE;
                }
            }
            if (bFound)
                continue;
        }
        if (xg_nRules & RULE_DONTTRIDIRECTIONS) {
            BOOL bFound = FALSE;
            for (INT y = 0; y < pat.num_rows; ++y) {
                for (INT x = 0; x < pat.num_columns; ++x) {
                    INT nCount = 0;
                    if (x > 0 && GET_DATA(x - 1, y) == ZEN_BLACK)
                        ++nCount;
                    if (y > 0 && GET_DATA(x, y - 1) == ZEN_BLACK)
                        ++nCount;
                    if (x + 1 < pat.num_columns && GET_DATA(x + 1, y) == ZEN_BLACK)
                        ++nCount;
                    if (y + 1 < pat.num_rows && GET_DATA(x, y + 1) == ZEN_BLACK)
                        ++nCount;
                    if (nCount >= 3) {
                        x = pat.num_columns;
                        y = pat.num_rows;
                        bFound = TRUE;
                    }
                }
            }
            if (bFound)
                continue;
        }
        if (xg_nRules & RULE_POINTSYMMETRY) {
            BOOL bFound = FALSE;
            for (INT y = 0; y < pat.num_rows; ++y) {
                for (INT x = 0; x < pat.num_columns; ++x) {
                    if ((GET_DATA(x, y) == ZEN_BLACK) !=
                        (GET_DATA(pat.num_columns - (x + 1), pat.num_rows - (y + 1)) == ZEN_BLACK))
                    {
                        x = pat.num_columns;
                        y = pat.num_rows;
                        bFound = TRUE;
                    }
                }
            }
            if (bFound)
                continue;
        }
#undef GET_DATA

        s_patterns.push_back(pat);
    }

    // かき混ぜる。
    std::random_shuffle(s_patterns.begin(), s_patterns.end());

    // インデックスとして追加する。
    for (size_t i = 0; i < s_patterns.size(); ++i)
    {
        SendDlgItemMessageW(hwnd, lst1, LB_ADDSTRING, 0, i);
    }

    return TRUE;
}

// WM_INITDIALOG
static BOOL
XgPattern_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    if (s_pLayout)
    {
        LayoutDestroy(s_pLayout);
        s_pLayout = NULL;
    }

    if (xg_bShowAnswerOnPattern)
        CheckDlgButton(hwnd, chx1, BST_CHECKED);
    else
        CheckDlgButton(hwnd, chx1, BST_UNCHECKED);

    CheckRadioButton(hwnd, rad1, rad6, rad6);
    XgPattern_RefreshContents(hwnd, rad6);

    static const LAYOUT_INFO layouts[] =
    {
        { stc1, BF_LEFT | BF_TOP },
        { rad1, BF_LEFT | BF_TOP },
        { rad2, BF_LEFT | BF_TOP },
        { rad3, BF_LEFT | BF_TOP },
        { rad4, BF_LEFT | BF_TOP },
        { rad5, BF_LEFT | BF_TOP },
        { rad6, BF_LEFT | BF_TOP },
        { stc2, BF_LEFT | BF_TOP },
        { lst1, BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM },
        { psh1, BF_LEFT | BF_BOTTOM },
        { chx1, BF_LEFT | BF_BOTTOM },
        { IDOK, BF_RIGHT | BF_BOTTOM },
        { IDCANCEL, BF_RIGHT | BF_BOTTOM },
    };
    s_pLayout = LayoutInit(hwnd, layouts, ARRAYSIZE(layouts));
    LayoutEnableResize(s_pLayout, TRUE);

    if (xg_nPatWndX != CW_USEDEFAULT && xg_nPatWndCX != CW_USEDEFAULT)
    {
        MoveWindow(hwnd, xg_nPatWndX, xg_nPatWndY, xg_nPatWndCX, xg_nPatWndCY, TRUE);
    }
    else
    {
        RECT rc;
        XgCenterDialog(hwnd);
        GetWindowRect(hwnd, &rc);
        xg_nPatWndX = rc.left;
        xg_nPatWndY = rc.top;
        xg_nPatWndCX = rc.right - rc.left;
        xg_nPatWndCY = rc.bottom - rc.top;
    }

    SetFocus(GetDlgItem(hwnd, lst1));
    return FALSE;
}

// 黒マスパターンのコピー。
static void XgPattern_OnCopy(HWND hwnd)
{
    HWND hLst1 = GetDlgItem(hwnd, lst1);
    INT i = ListBox_GetCurSel(hLst1);
    if (i == LB_ERR || i >= INT(s_patterns.size()))
        return;

    auto& pat = s_patterns[i];
    {
        auto sa1 = std::make_shared<XG_UndoData_SetAll>();
        auto sa2 = std::make_shared<XG_UndoData_SetAll>();
        sa1->Get();
        {
            XgPasteBoard(xg_hMainWnd, pat.data);
            XgCopyBoard(xg_hMainWnd);
        }
        sa2->Get();
        // 元に戻す情報を設定する。
        xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
    }
    XgUpdateImage(xg_hMainWnd, 0, 0);

    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(xg_hMainWnd);

    EndDialog(hwnd, IDCANCEL);
}

// 黒マスパターンで「OK」ボタンを押した。
static void XgPattern_OnOK(HWND hwnd)
{
    HWND hLst1 = GetDlgItem(hwnd, lst1);
    INT i = ListBox_GetCurSel(hLst1);
    if (i == LB_ERR || i >= INT(s_patterns.size()))
        return;

    auto& pat = s_patterns[i];
    {
        auto sa1 = std::make_shared<XG_UndoData_SetAll>();
        auto sa2 = std::make_shared<XG_UndoData_SetAll>();
        sa1->Get();
        {
            // コピー＆貼り付け。
            XgPasteBoard(xg_hMainWnd, pat.data);
            XgCopyBoard(xg_hMainWnd);
        }
        sa2->Get();
        // 元に戻す情報を設定する。
        xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
    }

    // ダイアログを閉じる。
    EndDialog(hwnd, IDOK);

    // 答えを表示するか？
    xg_bShowAnswerOnPattern = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);

    {
        auto sa1 = std::make_shared<XG_UndoData_SetAll>();
        auto sa2 = std::make_shared<XG_UndoData_SetAll>();
        sa1->Get();
        {
            // 解を求める（黒マス追加なし）。
            XgOnSolve_NoAddBlack(xg_hMainWnd, xg_bShowAnswerOnPattern);
        }
        sa2->Get();
        // 元に戻す情報を設定する。
        xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
    }

    // 表示を更新する。
    XgUpdateImage(xg_hMainWnd, 0, 0);

    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(xg_hMainWnd);
}

// WM_COMMAND
static void
XgPattern_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        XgPattern_OnOK(hwnd);
        break;
    case IDCANCEL:
        EndDialog(hwnd, IDCANCEL);
        break;
    case psh1:
        XgPattern_OnCopy(hwnd);
        break;
    case lst1:
        if (codeNotify == LBN_DBLCLK)
        {
            XgPattern_OnOK(hwnd);
        }
        break;
    case rad1:
    case rad2:
    case rad3:
    case rad4:
    case rad5:
    case rad6:
        XgPattern_RefreshContents(hwnd, id);
        break;
    }
}

static INT cxCell = 6, cyCell = 6; // 小さなセルのサイズ。

// WM_MEASUREITEM
static void
XgPattern_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem)
{
    // リストボックスの lst1 か？
    if (lpMeasureItem->CtlType != ODT_LISTBOX || lpMeasureItem->CtlID != lst1)
        return;

    HDC hDC = CreateCompatibleDC(NULL);
    SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);
    DeleteDC(hDC);
    lpMeasureItem->itemWidth = cxCell * 19 + 3;
    lpMeasureItem->itemHeight = cyCell * 18 + 3 + tm.tmHeight;
}

// WM_DRAWITEM
static void
XgPattern_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
    // リストボックスの lst1 か？
    if (lpDrawItem->CtlType != ODT_LISTBOX || lpDrawItem->CtlID != lst1)
        return;

    // データがパターンのインデックスか？
    LPARAM lParam = lpDrawItem->itemData;
    if ((int)lParam >= (int)s_patterns.size())
        return;

    // インデックスに対応するパターンを取得。
    const auto& pat = s_patterns[(int)lParam];
    HDC hDC = lpDrawItem->hDC;
    RECT rcItem = lpDrawItem->rcItem;

    // 必要ならフォーカス枠を描く。
    if (lpDrawItem->itemAction & ODA_FOCUS)
    {
        DrawFocusRect(hDC, &rcItem);
    }

    // その他は無視。
    if (!(lpDrawItem->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
        return;

    // サイズを表すテキスト。
    WCHAR szText[64];
    StringCbPrintfW(szText, sizeof(szText), L"%u x %u", int(pat.num_columns), int(pat.num_rows));

    // 背景とテキストを描画する。
    SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
    SetBkMode(hDC, TRANSPARENT);
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);
    if (lpDrawItem->itemState & ODS_SELECTED)
    {
        FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetTextColor(hDC, COLOR_HIGHLIGHTTEXT);
    }
    else
    {
        FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
        SetTextColor(hDC, COLOR_WINDOWTEXT);
    }
    DrawTextW(hDC, szText, -1, &rcItem, DT_CENTER | DT_TOP | DT_SINGLELINE);

    // 必要ならフォーカス枠を描く。
    if (lpDrawItem->itemState & ODS_FOCUS)
    {
        DrawFocusRect(hDC, &rcItem);
    }

    // パターンのテキストデータを扱いやすいよう、加工する。
    std::vector<WCHAR> data;
    XgConvertPatternData(data, pat.data, pat.num_columns, pat.num_rows);

    // 描画項目のサイズ。
    INT cxItem = rcItem.right - rcItem.left;

    // メモリーデバイスコンテキストを作成。
    if (HDC hdcMem = CreateCompatibleDC(hDC))
    {
        INT cx = cxCell * pat.num_columns, cy = cyCell * pat.num_rows; // 全体のサイズ。
        // ビットマップを作成する。
        if (HBITMAP hbm = XgCreate24BppBitmap(hdcMem, cx + 3, cy + 3))
        {
            // ビットマップを選択。
            HGDIOBJ hbmOld = SelectObject(hdcMem, hbm);
            // 四角形を描く。
            SelectObject(hdcMem, GetStockPen(BLACK_PEN)); // 黒いペン
            SelectObject(hdcMem, GetStockBrush(WHITE_BRUSH)); // 白いブラシ
            Rectangle(hdcMem, 0, 0, cx + 2, cy + 2);
            // 黒マスを描画する。
            for (INT y = 0; y < pat.num_rows; ++y)
            {
                RECT rc;
                for (INT x = 0; x < pat.num_columns; ++x)
                {
                    rc.left = 1 + x * cxCell;
                    rc.top = 1 + y * cyCell;
                    rc.right = rc.left + cxCell;
                    rc.bottom = rc.top + cyCell;
                    if (data[x + pat.num_columns * y] == ZEN_BLACK)
                        FillRect(hdcMem, &rc, GetStockBrush(BLACK_BRUSH));
                }
            }
            // 境界線を描画する。
            for (INT y = 0; y < pat.num_rows + 1; ++y)
            {
                MoveToEx(hdcMem, 1, 1 + y * cyCell, NULL);
                LineTo(hdcMem, 1 + pat.num_columns * cxCell, 1 + y * cyCell);
            }
            for (INT x = 0; x < pat.num_columns + 1; ++x)
            {
                MoveToEx(hdcMem, 1 + x * cxCell, 1, NULL);
                LineTo(hdcMem, 1 + x * cxCell, 1 + pat.num_rows * cyCell);
            }
            // ビットマップイメージをhDCに転送する。
            BitBlt(hDC,
                   rcItem.left + (cxItem - (cx + 3)) / 2,
                   rcItem.top + tm.tmHeight,
                   cx + 3, cy + 3, hdcMem, 0, 0, SRCCOPY);
            // ビットマップの選択を解除する。
            SelectObject(hdcMem, hbmOld);
            // ビットマップを破棄する。
            DeleteObject(hbm);
        }
        // メモリーデバイスコンテキストを破棄する。
        DeleteDC(hdcMem);
    }
}

static void XgPattern_OnDestroy(HWND hwnd)
{
    if (s_pLayout)
    {
        LayoutDestroy(s_pLayout);
        s_pLayout = NULL;
    }
}

static void XgPattern_OnMove(HWND hwnd, int x, int y)
{
    RECT rc;
    if (!IsMinimized(hwnd) && !IsMaximized(hwnd))
    {
        GetWindowRect(hwnd, &rc);
        xg_nPatWndX = rc.left;
        xg_nPatWndY = rc.top;
    }
}

static void XgPattern_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;

    LayoutUpdate(hwnd, s_pLayout, NULL, 0);

    if (!IsMinimized(hwnd) && !IsMaximized(hwnd))
    {
        GetWindowRect(hwnd, &rc);
        xg_nPatWndCX = rc.right - rc.left;
        xg_nPatWndCY = rc.bottom - rc.top;
    }
}

static void XgPattern_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMinTrackSize.x = 600;
    lpMinMaxInfo->ptMinTrackSize.y = 300;
}

// 「黒マスパターン」ダイアログプロシージャ。
INT_PTR CALLBACK
XgPatternDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, XgPattern_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, XgPattern_OnCommand);
        HANDLE_MSG(hwnd, WM_MEASUREITEM, XgPattern_OnMeasureItem);
        HANDLE_MSG(hwnd, WM_DRAWITEM, XgPattern_OnDrawItem);
        HANDLE_MSG(hwnd, WM_MOVE, XgPattern_OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, XgPattern_OnSize);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO, XgPattern_OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_DESTROY, XgPattern_OnDestroy);
    }
    return 0;
}

// 「黒マスパターン」を開く。
void __fastcall XgOpenPatterns(HWND hwnd)
{
    DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(IDD_BLOCKPATTERN), hwnd, XgPatternDlgProc);
}

// 辞書を切り替える。
void MainWnd_DoDictionary(HWND hwnd, size_t iDict)
{
    // 範囲外は無視。
    if (iDict >= xg_dict_files.size())
        return;

    // 辞書を読み込み、セットする。
    const auto& file = xg_dict_files[iDict];
    if (XgLoadDictFile(file.c_str()))
    {
        XgSetDict(file.c_str());
        XgSetInputModeFromDict(hwnd);
    }

    // 二重マス単語の候補をクリアする。
    xg_vMarkedCands.clear();
}

// 「黒マスルールの説明.txt」を開く。
static void OnOpenRulesTxt(HWND hwnd)
{
    WCHAR szPath[MAX_PATH], szDir[MAX_PATH];
    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    PathRemoveFileSpecW(szPath);
    StringCbCopyW(szDir, sizeof(szDir), szPath);
    StringCbCopyW(szPath, sizeof(szPath), szDir);
    PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
    if (!PathFileExistsW(szPath)) {
        StringCbCopyW(szPath, sizeof(szPath), szDir);
        PathAppendW(szPath, L"..");
        PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
        if (!PathFileExistsW(szPath)) {
            StringCbCopyW(szPath, sizeof(szPath), szDir);
            PathAppendW(szPath, L"..\\..");
            PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
            if (!PathFileExistsW(szPath)) {
                StringCbCopyW(szPath, sizeof(szPath), szDir);
                PathAppendW(szPath, L"..\\..\\..");
                PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
            }
        }
    }
    ShellExecuteW(hwnd, NULL, szPath, NULL, NULL, SW_SHOWNORMAL);
}

// 黒マスルールをチェックする。
void __fastcall XgRuleCheck(HWND hwnd)
{
    XG_Board& board = (xg_bShowAnswer ? xg_solution : xg_xword);
    // 連黒禁。
    if (xg_nRules & RULE_DONTDOUBLEBLACK) {
        if (board.DoubleBlack()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADJACENTBLOCK), nullptr, MB_ICONERROR);
            return;
        }
    }
    // 四隅黒禁。
    if (xg_nRules & RULE_DONTCORNERBLACK) {
        if (board.CornerBlack()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CORNERBLOCK), nullptr, MB_ICONERROR);
            return;
        }
    }
    // 三方黒禁。
    if (xg_nRules & RULE_DONTTRIDIRECTIONS) {
        if (board.TriBlackAround()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_TRIBLOCK), nullptr, MB_ICONERROR);
            return;
        }
    }
    // 分断禁。
    if (xg_nRules & RULE_DONTDIVIDE) {
        if (board.DividedByBlack()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_DIVIDED), nullptr, MB_ICONERROR);
            return;
        }
    }
    // 黒斜三連禁。
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (board.ThreeDiagonals()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_THREEDIAGONALS), nullptr, MB_ICONERROR);
            return;
        }
    } else {
        // 黒斜四連禁。
        if (xg_nRules & RULE_DONTFOURDIAGONALS) {
            if (board.FourDiagonals()) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_FOURDIAGONALS), nullptr, MB_ICONERROR);
                return;
            }
        }
    }
    // 黒マス点対称。
    if (xg_nRules & RULE_POINTSYMMETRY) {
        if (!board.IsPointSymmetry()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTPOINTSYMMETRY), nullptr, MB_ICONERROR);
            return;
        }
    }

    // 合格。
    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_RULESPASSED),
                        XgLoadStringDx2(IDS_PASSED), MB_ICONINFORMATION);
}

// タグリストボックスを初期化。
static void XgInitTagListView(HWND hwndLV)
{
    DWORD exstyle = LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_GRIDLINES;
    ListView_SetExtendedListViewStyleEx(hwndLV, exstyle, exstyle);

    LV_COLUMN column = { LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT };

    column.pszText = XgLoadStringDx1(IDS_TAGS);
    column.fmt = LVCFMT_LEFT;
    column.cx = 90;
    column.iSubItem = 0;
    ListView_InsertColumn(hwndLV, 0, &column);

    column.pszText = XgLoadStringDx1(IDS_TAGCOUNT);
    column.fmt = LVCFMT_RIGHT;
    column.cx = 84;
    column.iSubItem = 1;
    ListView_InsertColumn(hwndLV, 1, &column);
}

template <typename T_STR_CONTAINER>
inline void
mstr_split(T_STR_CONTAINER& container,
           const typename T_STR_CONTAINER::value_type& str,
           const typename T_STR_CONTAINER::value_type& chars)
{
    container.clear();
    size_t i = 0, k = str.find_first_of(chars);
    while (k != T_STR_CONTAINER::value_type::npos)
    {
        container.push_back(str.substr(i, k - i));
        i = k + 1;
        k = str.find_first_of(chars, i);
    }
    container.push_back(str.substr(i));
}

static void XgTheme_SetPreset(HWND hwnd, LPCWSTR pszText)
{
    HWND hLst2 = GetDlgItem(hwnd, lst2);
    HWND hLst3 = GetDlgItem(hwnd, lst3);
    ListView_DeleteAllItems(hLst2);
    ListView_DeleteAllItems(hLst3);

    std::vector<std::wstring> strs;
    std::wstring strText = pszText;

    xg_str_replace_all(strText, L" ", L"");
    mstr_split(strs, strText, L",");

    WCHAR szText[64];

    for (auto& str : strs) {
        if (str.empty())
            continue;

        bool minus = false;
        if (str[0] == L'-') {
            minus = true;
            str = str.substr(1);
        }
        if (str[0] == L'+') {
            str = str.substr(1);
        }

        LV_ITEM item = { LVIF_TEXT };
        INT iItem = ListView_GetItemCount(hLst3);
        StringCbCopyW(szText, sizeof(szText), str.c_str());
        item.iItem = iItem;
        item.pszText = szText;
        item.iSubItem = 0;
        if (minus)
            ListView_InsertItem(hLst3, &item);
        else
            ListView_InsertItem(hLst2, &item);

        StringCbCopyW(szText, sizeof(szText), std::to_wstring(xg_tag_histgram[str]).c_str());
        item.iItem = iItem;
        item.pszText = szText;
        item.iSubItem = 1;
        if (minus)
            ListView_SetItem(hLst3, &item);
        else
            ListView_SetItem(hLst2, &item);
    }

    // 最初の項目を選択する。
    LV_ITEM item;
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItem(hLst2, &item);

    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItem(hLst3, &item);
}

// タグ群の最大長。
#define MAX_TAGSLEN 256

static void XgTheme_SetPreset(HWND hwnd)
{
    HWND hCmb1 = GetDlgItem(hwnd, cmb1);
    INT iItem = ComboBox_GetCurSel(hCmb1);

    WCHAR szText[MAX_TAGSLEN];
    if (iItem == CB_ERR) {
        GetDlgItemTextW(hwnd, cmb1, szText, ARRAYSIZE(szText));
    } else {
        ComboBox_GetLBText(hCmb1, iItem, szText);
    }

    XgTheme_SetPreset(hwnd, szText);
}

static BOOL xg_bUpdatingPreset = FALSE;

static void XgTheme_UpdatePreset(HWND hwnd)
{
    HWND hLst2 = GetDlgItem(hwnd, lst2);
    HWND hLst3 = GetDlgItem(hwnd, lst3);

    std::wstring str;
    WCHAR szText[64];
    INT nCount2 = ListView_GetItemCount(hLst2);
    INT nCount3 = ListView_GetItemCount(hLst3);
    for (INT i = 0; i < nCount2; ++i) {
        if (str.size()) {
            str += L",";
        }
        ListView_GetItemText(hLst2, i, 0, szText, ARRAYSIZE(szText));
        str += L"+";
        str += szText;
    }
    for (INT i = 0; i < nCount3; ++i) {
        if (str.size()) {
            str += L",";
        }
        ListView_GetItemText(hLst3, i, 0, szText, ARRAYSIZE(szText));
        str += L"-";
        str += szText;
    }

    // 長さ制限。
    if (str.size() > MAX_TAGSLEN - 1)
        str.resize(MAX_TAGSLEN - 1);

    xg_bUpdatingPreset = TRUE;
    SetDlgItemTextW(hwnd, cmb1, str.c_str());
    xg_bUpdatingPreset = FALSE;
}

// 「テーマ」ダイアログの初期化。
static BOOL XgTheme_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    // ダイアログを中央寄せする。
    XgCenterDialog(hwnd);

    // 長さを制限する。
    SendDlgItemMessageW(hwnd, cmb1, CB_LIMITTEXT, MAX_TAGSLEN - 1, 0);

    // リストビューを初期化。
    HWND hLst1 = GetDlgItem(hwnd, lst1);
    HWND hLst2 = GetDlgItem(hwnd, lst2);
    HWND hLst3 = GetDlgItem(hwnd, lst3);
    XgInitTagListView(hLst1);
    XgInitTagListView(hLst2);
    XgInitTagListView(hLst3);

    // ヒストグラムを取得。
    std::vector<std::pair<size_t, std::wstring> > histgram;
    for (auto& pair : xg_tag_histgram) {
        histgram.emplace_back(std::make_pair(pair.second, pair.first));
    }
    // 出現回数の逆順でソート。
    std::sort(histgram.begin(), histgram.end(),
        [](const std::pair<size_t, std::wstring>& a, const std::pair<size_t, std::wstring>& b) {
            return a.first > b.first;
        }
    );

    // リストビューを逆順のヒストグラムで埋める。
    INT iItem = 0;
    LV_ITEM item = { LVIF_TEXT };
    WCHAR szText[64];
    for (auto& pair : histgram) {
        StringCbCopyW(szText, sizeof(szText), pair.second.c_str());
        item.iItem = iItem;
        item.pszText = szText;
        item.iSubItem = 0;
        ListView_InsertItem(hLst1, &item);

        StringCbCopyW(szText, sizeof(szText), std::to_wstring(pair.first).c_str());
        item.iItem = iItem;
        item.pszText = szText;
        item.iSubItem = 1;
        ListView_SetItem(hLst1, &item);

        ++iItem;
    }

    // コンボボックスにテキストを設定する。
    SetDlgItemTextW(hwnd, cmb1, xg_strTheme.c_str());
    // プリセットを設定する。
    XgTheme_SetPreset(hwnd, xg_strTheme.c_str());

    // 最初の項目を選択。
    item.mask = LVIF_STATE;
    item.iItem = 0;
    item.iSubItem = 0;
    item.state = item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItem(hLst1, &item);

    return TRUE;
}

// リストビューにタグ項目を追加する。
static void XgTheme_AddTag(HWND hwnd, BOOL bPriority)
{
    // 選択中のテキストを取得する。
    HWND hLst1 = GetDlgItem(hwnd, lst1);
    INT iItem = ListView_GetNextItem(hLst1, -1, LVNI_ALL | LVNI_SELECTED);
    if (iItem < 0)
        return; // 選択なし。
    WCHAR szText1[64], szText2[64];
    ListView_GetItemText(hLst1, iItem, 0, szText1, ARRAYSIZE(szText1));
    ListView_GetItemText(hLst1, iItem, 1, szText2, ARRAYSIZE(szText2));

    LV_FINDINFO find = { LVFI_STRING, szText1 };
    if (bPriority) {
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        iItem = ListView_FindItem(hLst2, -1, &find);
        if (iItem >= 0)
            return; // すでにあった。

        // タグ項目を追加。
        INT cItems = ListView_GetItemCount(hLst2);
        LV_ITEM item = { LVIF_TEXT };
        item.iItem = cItems;
        item.iSubItem = 0;
        item.pszText = szText1;
        iItem = ListView_InsertItem(hLst2, &item);
        item.iItem = iItem;
        item.iSubItem = 1;
        item.pszText = szText2;
        ListView_SetItem(hLst2, &item);

        // カウンターを更新。
        size_t count = 0;
        cItems = ListView_GetItemCount(hLst2);
        for (iItem = 0; iItem < cItems; ++iItem) {
            ListView_GetItemText(hLst2, iItem, 1, szText2, ARRAYSIZE(szText2));
            count += _wtoi(szText2);
        }
        SetDlgItemInt(hwnd, stc1, INT(count), FALSE);
    } else {
        HWND hLst3 = GetDlgItem(hwnd, lst3);
        iItem = ListView_FindItem(hLst3, -1, &find);
        if (iItem >= 0)
            return; // すでにあった。

        // タグ項目を追加。
        INT cItems = ListView_GetItemCount(hLst3);
        LV_ITEM item = { LVIF_TEXT };
        item.iItem = cItems;
        item.iSubItem = 0;
        item.pszText = szText1;
        iItem = ListView_InsertItem(hLst3, &item);
        item.iItem = iItem;
        item.iSubItem = 1;
        item.pszText = szText2;
        ListView_SetItem(hLst3, &item);

        // カウンターを更新。
        size_t count = 0;
        cItems = ListView_GetItemCount(hLst3);
        for (iItem = 0; iItem < cItems; ++iItem) {
            ListView_GetItemText(hLst3, iItem, 1, szText2, ARRAYSIZE(szText2));
            count += _wtoi(szText2);
        }
        SetDlgItemInt(hwnd, stc2, INT(count), FALSE);
    }

    // プリセットを更新。
    XgTheme_UpdatePreset(hwnd);
}

// リストビューからタグ項目を削除する。
static void XgTheme_RemoveTag(HWND hwnd, BOOL bPriority)
{
    WCHAR szText[64];
    if (bPriority) {
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        INT iItem = ListView_GetNextItem(hLst2, -1, LVNI_ALL | LVNI_SELECTED);
        ListView_DeleteItem(hLst2, iItem);

        // カウンターを更新。
        INT cItems = ListView_GetItemCount(hLst2);
        size_t count = 0;
        for (iItem = 0; iItem < cItems; ++iItem) {
            ListView_GetItemText(hLst2, iItem, 1, szText, ARRAYSIZE(szText));
            count += _wtoi(szText);
        }
        SetDlgItemInt(hwnd, stc1, INT(count), FALSE);
    } else {
        HWND hLst3 = GetDlgItem(hwnd, lst3);
        INT iItem = ListView_GetNextItem(hLst3, -1, LVNI_ALL | LVNI_SELECTED);
        ListView_DeleteItem(hLst3, iItem);

        // カウンターを更新。
        INT cItems = ListView_GetItemCount(hLst3);
        size_t count = 0;
        for (iItem = 0; iItem < cItems; ++iItem) {
            ListView_GetItemText(hLst3, iItem, 1, szText, ARRAYSIZE(szText));
            count += _wtoi(szText);
        }
        SetDlgItemInt(hwnd, stc2, INT(count), FALSE);
    }

    // プリセットを更新。
    XgTheme_UpdatePreset(hwnd);
}

// 「テーマ」ダイアログで「OK」ボタンが押された。
static BOOL XgTheme_OnOK(HWND hwnd)
{
    HWND hLst2 = GetDlgItem(hwnd, lst2);
    HWND hLst3 = GetDlgItem(hwnd, lst3);

    xg_priority_tags.clear();
    xg_forbidden_tags.clear();

    std::wstring strTheme;
    WCHAR szText[MAX_TAGSLEN];
    INT cItems;

    cItems = ListView_GetItemCount(hLst2);
    for (INT iItem = 0; iItem < cItems; ++iItem) {
        ListView_GetItemText(hLst2, iItem, 0, szText, ARRAYSIZE(szText));
        xg_priority_tags.emplace(szText);
        if (strTheme.size())
            strTheme += L',';
        strTheme += L'+';
        strTheme += szText;
    }

    cItems = ListView_GetItemCount(hLst3);
    for (INT iItem = 0; iItem < cItems; ++iItem) {
        ListView_GetItemText(hLst3, iItem, 0, szText, ARRAYSIZE(szText));
        xg_forbidden_tags.emplace(szText);
        if (strTheme.size())
            strTheme += L',';
        strTheme += L'-';
        strTheme += szText;
    }

    XgSetThemeString(strTheme);

    return TRUE;
}

// タグの検索。
static void XgTheme_OnEdt1(HWND hwnd)
{
    WCHAR szText[64];
    GetDlgItemTextW(hwnd, edt1, szText, ARRAYSIZE(szText));

    HWND hLst1 = GetDlgItem(hwnd, lst1);

    LV_FINDINFO find = { LVFI_STRING | LVFI_PARTIAL };
    find.psz = szText;
    INT iItem = ListView_FindItem(hLst1, -1, &find);
    UINT state = LVIS_FOCUSED | LVIS_SELECTED;
    ListView_SetItemState(hLst1, iItem, state, state);
    ListView_EnsureVisible(hLst1, iItem, FALSE);
}

// 「テーマ」ダイアログのコマンド処理。
static void XgTheme_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDOK:
        if (XgTheme_OnOK(hwnd)) {
            EndDialog(hwnd, IDOK);
        }
        break;
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case psh1: // →
        XgTheme_AddTag(hwnd, TRUE);
        break;
    case psh2: // ←
        XgTheme_RemoveTag(hwnd, TRUE);
        break;
    case psh3: // →
        XgTheme_AddTag(hwnd, FALSE);
        break;
    case psh4: // ←
        XgTheme_RemoveTag(hwnd, FALSE);
        break;
    case psh5: // リセット
        XgSetThemeString(xg_strDefaultTheme);
        EndDialog(hwnd, IDOK);
        break;
    case edt1:
        if (codeNotify == EN_CHANGE) {
            XgTheme_OnEdt1(hwnd);
        }
        break;
    case cmb1:
        if (codeNotify == CBN_EDITCHANGE && !xg_bUpdatingPreset) {
            XgTheme_SetPreset(hwnd);
        }
        break;
    }
}

LRESULT XgTheme_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    LV_KEYDOWN *pKeyDown;
    switch (idFrom) {
    case lst1:
        if (pnmhdr->code == NM_DBLCLK) {
            XgTheme_AddTag(hwnd, TRUE);
        }
        break;
    case lst2:
        if (pnmhdr->code == NM_DBLCLK) {
            XgTheme_RemoveTag(hwnd, TRUE);
        } else if (pnmhdr->code == LVN_KEYDOWN) {
            pKeyDown = reinterpret_cast<LV_KEYDOWN *>(pnmhdr);
            if (pKeyDown->wVKey == VK_DELETE)
                XgTheme_RemoveTag(hwnd, TRUE);
        }
        break;
    case lst3:
        if (pnmhdr->code == NM_DBLCLK) {
            XgTheme_RemoveTag(hwnd, FALSE);
        } else if (pnmhdr->code == LVN_KEYDOWN) {
            pKeyDown = reinterpret_cast<LV_KEYDOWN *>(pnmhdr);
            if (pKeyDown->wVKey == VK_DELETE)
                XgTheme_RemoveTag(hwnd, FALSE);
        }
        break;
    }
    return 0;
}

// 「テーマ」ダイアログプロシージャ。
static INT_PTR CALLBACK
XgThemeDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, XgTheme_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, XgTheme_OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, XgTheme_OnNotify);
    }
    return 0;
}

// 「テーマ」ダイアログを表示する。
void __fastcall XgTheme(HWND hwnd)
{
    if (xg_tag_histgram.empty()) {
        // タグがありません。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTAGS), NULL, MB_ICONERROR);
        return;
    }

    INT id = DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(IDD_THEME), hwnd, XgThemeDlgProc);
    if (id == IDOK) {
        XgUpdateTheme(hwnd);
    }
}

// テーマをリセットする。
void __fastcall XgResetTheme(HWND hwnd, BOOL bQuery)
{
    if (bQuery) {
        INT id = XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_RESETTHEME),
                                     XgLoadStringDx2(IDS_APPNAME),
                                     MB_ICONINFORMATION | MB_YESNOCANCEL);
        if (id != IDYES)
            return;
    }
    XgResetTheme(hwnd);
    XgUpdateTheme(hwnd);
}

void __fastcall XgShowResults(HWND hwnd)
{
    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        // キャンセルされた。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 成功メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_MADEPROBLEM),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints(hwnd);
        XgShowHints(hwnd);
    } else {
        // 失敗メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANTMAKEPROBLEM),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONERROR);
    }
}

// メッセージボックスを表示する。
void __fastcall XgShowResultsRepeatedly(HWND hwnd)
{
    WCHAR sz[MAX_PATH];

    // ディスクに空きがあるか？
    if (s_bOutOfDiskSpace) {
        // なかった。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
    } else {
        // あった。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), s_nNumberGenerated,
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
    }

    // 終了メッセージを表示する。
    XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

    // 保存先フォルダを開く。
    if (s_nNumberGenerated && !s_dirs_save_to.empty())
        ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                        nullptr, nullptr, SW_SHOWNORMAL);
}

static void XgSetZoomRate(HWND hwnd, INT nZoomRate)
{
    xg_nZoomRate = nZoomRate;
    INT x = XgGetHScrollPos();
    INT y = XgGetVScrollPos();
    XgUpdateScrollInfo(hwnd, x, y);
    XgUpdateImage(hwnd, x, y);
}

// コマンドを実行する。
void __fastcall MainWnd_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT /*codeNotify*/)
{
    WCHAR sz[MAX_PATH];
    OPENFILENAMEW ofn;
    int x, y;

    switch (id) {
    case ID_LEFT:
        // キャレットを左へ移動。
        if (xg_caret_pos.m_j > 0) {
            xg_caret_pos.m_j--;
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番左のキャレットなら、左端へ移動。
            x = 0;
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_RIGHT:
        // キャレットを右へ移動。
        if (xg_caret_pos.m_j + 1 < xg_nCols) {
            xg_caret_pos.m_j++;
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番右のキャレットなら、右端へ移動。
            SIZE siz;
            ForDisplay for_display;
            XgGetXWordExtent(&siz);
            MRect rcClient;
            XgGetRealClientRect(hwnd, &rcClient);
            x = siz.cx - rcClient.Width();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_UP:
        // キャレットを上へ移動。
        if (xg_caret_pos.m_i > 0) {
            xg_caret_pos.m_i--;
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番上のキャレットなら、上端へ移動。
            x = XgGetHScrollPos();
            y = 0;
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_DOWN:
        // キャレットを下へ移動。
        if (xg_caret_pos.m_i + 1 < xg_nRows) {
            xg_caret_pos.m_i++;
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番下のキャレットなら、下端へ移動。
            SIZE siz;
            ForDisplay for_display;
            XgGetXWordExtent(&siz);
            MRect rcClient;
            XgGetRealClientRect(hwnd, &rcClient);
            x = XgGetHScrollPos();
            y = siz.cy - rcClient.Height();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTLEFT:
        // Ctrl+←。
        if (xg_caret_pos.m_j > 0) {
            xg_caret_pos.m_j = 0;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番左のキャレットなら、左端へ移動。
            x = 0;
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTRIGHT:
        // Ctrl+→。
        if (xg_caret_pos.m_j + 1 < xg_nCols) {
            xg_caret_pos.m_j = xg_nCols - 1;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番右のキャレットなら、右端へ移動。
            SIZE siz;
            ForDisplay for_display;
            XgGetXWordExtent(&siz);
            MRect rcClient;
            XgGetRealClientRect(hwnd, &rcClient);
            x = siz.cx - rcClient.Width();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTUPPER:
        // Ctrl+↑。
        if (xg_caret_pos.m_i > 0) {
            xg_caret_pos.m_i = 0;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番上のキャレットなら、上端へ移動。
            x = XgGetHScrollPos();
            y = 0;
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTLOWER:
        // Ctrl+↓。
        if (xg_caret_pos.m_i + 1 < xg_nRows) {
            xg_caret_pos.m_i = xg_nRows - 1;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else {
            // 一番下のキャレットなら、下端へ移動。
            SIZE siz;
            ForDisplay for_display;
            XgGetXWordExtent(&siz);
            MRect rcClient;
            XgGetRealClientRect(hwnd, &rcClient);
            x = XgGetHScrollPos();
            y = siz.cy - rcClient.Height();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
        break;
    case ID_OPENCANDSWNDHORZ:
        XgOpenCandsWnd(hwnd, false);
        xg_prev_vk = 0;
        break;
    case ID_OPENCANDSWNDVERT:
        XgOpenCandsWnd(hwnd, true);
        xg_prev_vk = 0;
        break;

    case ID_HEADERANDNOTES:
        ::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_NOTES), hwnd, XgNotesDlgProc);
        break;

    case ID_COPYHINTSSTYLE0:
        XgCopyHintsStyle0(hwnd, 2);
        break;

    case ID_COPYHINTSSTYLE1:
        XgCopyHintsStyle1(hwnd, 2);
        break;

    case ID_COPYVHINTSSTYLE0:
        XgCopyHintsStyle0(hwnd, 0);
        break;

    case ID_COPYHHINTSSTYLE0:
        XgCopyHintsStyle0(hwnd, 1);
        break;

    case ID_COPYVHINTSSTYLE1:
        XgCopyHintsStyle1(hwnd, 0);
        break;

    case ID_COPYHHINTSSTYLE1:
        XgCopyHintsStyle1(hwnd, 1);
        break;

    case ID_LOADDICTFILE:
        if (::DialogBoxW(xg_hInstance, MAKEINTRESOURCE(IDD_READDICT),
                         hwnd, XgLoadDictDlgProc) == IDOK)
        {
            // 二重マス単語をクリアする。
            SendMessageW(hwnd, WM_COMMAND, ID_KILLMARKS, 0);
        }
        break;

    case ID_SETTINGS:   // 設定。
        MainWnd_OnSettings(hwnd);
        break;

    case ID_ERASESETTINGS:  // 設定の削除。
        MainWnd_OnEraseSettings(hwnd);
        break;

    case ID_FLIPVH: // 縦と横を入れ替える。
        {
            bool flag = !!::IsWindow(xg_hHintsWnd);
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 縦と横を入れ替える。
            MainWnd_OnFlipVH(hwnd);
            if (flag) {
                XgShowHints(hwnd);
            }
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_NEW:    // 新規作成。
        {
            bool flag = false;
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 新規作成ダイアログ。
            if (XgOnNew(hwnd)) {
                flag = true;
                sa2->Get();
            }
            if (flag) {
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            } else {
                sa1->Apply();
            }
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_GENERATE:   // 問題を自動生成する。
        {
            bool flag = false;
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 問題の作成。
            if (XgOnGenerate(hwnd, false, false)) {
                flag = true;
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
                // イメージを更新する。
                xg_caret_pos.clear();
                XgMarkUpdate();
                XgUpdateImage(hwnd, 0, 0);
                // メッセージボックスを表示する。
                XgShowResults(hwnd);
            }
            if (!flag) {
                sa1->Apply();
            }
            // イメージを更新する。
            xg_caret_pos.clear();
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_GENERATEANSWER:   // 問題を自動生成する（答え付き）。
        {
            bool flag = false;
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 問題の作成。
            if (XgOnGenerate(hwnd, true, false)) {
                flag = true;
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
                // イメージを更新する。
                xg_caret_pos.clear();
                XgMarkUpdate();
                XgUpdateImage(hwnd, 0, 0);
                // メッセージボックスを表示する。
                XgShowResults(hwnd);
            }
            if (!flag) {
                sa1->Apply();
            }
            // イメージを更新する。
            xg_caret_pos.clear();
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_GENERATEREPEATEDLY:     // 問題を連続自動生成する
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 連続生成ダイアログ。
            if (XgOnGenerate(hwnd, false, true)) {
                // クリアする。
                xg_bShowAnswer = false;
                xg_bSolved = false;
                xg_xword.clear();
                xg_solution.clear();
                // 元に戻す情報を残す。
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
                // イメージを更新する。
                xg_caret_pos.clear();
                XgMarkUpdate();
                XgUpdateImage(hwnd, 0, 0);
                // メッセージボックスを表示する。
                XgShowResultsRepeatedly(hwnd);
            }
            // イメージを更新する。
            xg_caret_pos.clear();
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_OPEN:   // ファイルを開く。
        // ユーザーにファイルの場所を問い合わせる。
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_CROSSFILTER));
        sz[0] = 0;
        ofn.lpstrFile = sz;
        ofn.nMaxFile = static_cast<DWORD>(ARRAYSIZE(sz));
        ofn.lpstrTitle = XgLoadStringDx1(IDS_OPENCROSSDATA);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
            OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"xwd";
        if (::GetOpenFileNameW(&ofn)) {
            // JSON形式か？
            bool is_json = false;
            bool is_builder = false;
            if (::lstrcmpiW(PathFindExtensionW(sz), L".xwj") == 0 ||
                ::lstrcmpiW(PathFindExtensionW(sz), L".json") == 0)
            {
                is_json = true;
            }
            if (::lstrcmpiW(PathFindExtensionW(sz), L".crp") == 0 ||
                ::lstrcmpiW(PathFindExtensionW(sz), L".crx") == 0)
            {
                is_builder = true;
            }
            // 開く。
            if (is_builder) {
                if (!XgDoLoadCrpFile(hwnd, sz)) {
                    // 失敗。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
                } else {
                    // 成功。
                    xg_ubUndoBuffer.Empty();
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            } else {
                if (!XgDoLoadFile(hwnd, sz, is_json)) {
                    // 失敗。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
                } else {
                    // 成功。
                    xg_ubUndoBuffer.Empty();
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                    // テーマを更新する。
                    XgSetThemeString(xg_strTheme);
                    XgUpdateTheme(hwnd);
                }
            }
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        // ルールを更新する。
        XgUpdateRules(hwnd);
        break;

    case ID_SAVEAS: // ファイルを保存する。
        if (xg_dict_files.empty()) {
            // 辞書ファイルの名前を読み込む。
            XgLoadDictsAll();
        }
        // ユーザーにファイルの場所を問い合わせる準備。
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_SAVEFILTER));
        StringCbCopy(sz, sizeof(sz), xg_strFileName.data());
        ofn.lpstrFile = sz;
        ofn.nMaxFile = static_cast<DWORD>(ARRAYSIZE(sz));
        ofn.lpstrTitle = XgLoadStringDx1(IDS_SAVECROSSDATA);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT |
            OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        // JSON only
        ofn.nFilterIndex = 1;
        ofn.lpstrDefExt = L"xwj";
        if (lstrcmpiW(PathFindExtensionW(sz), L".xwd") == 0)
        {
            PathRemoveExtensionW(sz);
        }
        // ユーザーにファイルの場所を問い合わせる。
        if (::GetSaveFileNameW(&ofn)) {
            // 保存する。
            if (!XgDoSave(hwnd, sz)) {
                // 保存に失敗。
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            }
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_SAVEPROBASIMAGE:    // 問題を画像ファイルとして保存する。
        XgSaveProbAsImage(hwnd);
        break;

    case ID_SAVEANSASIMAGE:     // 解答を画像ファイルとして保存する。
        XgSaveAnsAsImage(hwnd);
        break;

    case ID_LINESYMMETRYCHECK:  // 線対称チェック。
        XgOnLineSymmetryCheck(hwnd);
        break;

    case ID_POINTSYMMETRYCHECK: // 点対称チェック。
        XgOnPointSymmetryCheck(hwnd);
        break;

    case ID_EXIT:   // 終了する。
        DestroyWindow(hwnd);
        break;

    case ID_UNDO:   // 元に戻す。
        if (::GetForegroundWindow() != xg_hMainWnd) {
            HWND hwnd = ::GetFocus();
            ::SendMessageW(hwnd, WM_UNDO, 0, 0);
        } else {
            if (xg_ubUndoBuffer.CanUndo()) {
                xg_ubUndoBuffer.Undo();
                if (::IsWindow(xg_hHintsWnd)) {
                    XgSetHintsData();
                }
                // イメージを更新する。
                XgUpdateImage(hwnd, 0, 0);
            } else {
                ::MessageBeep(0xFFFFFFFF);
            }
        }
        break;

    case ID_REDO:   // やり直す。
        if (::GetForegroundWindow() != xg_hMainWnd) {
            ;
        } else {
            if (xg_ubUndoBuffer.CanRedo()) {
                xg_ubUndoBuffer.Redo();
                if (::IsWindow(xg_hHintsWnd)) {
                    XgSetHintsData();
                }
                // イメージを更新する。
                XgUpdateImage(hwnd, 0, 0);
            } else {
                ::MessageBeep(0xFFFFFFFF);
            }
        }
        break;

    case ID_GENERATEBLACKS: // 黒マスパターンを生成。
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            if (XgOnGenerateBlacks(hwnd, false)) {
                sa2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            }
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_GENERATEBLACKSSYMMETRIC2: // 黒マスパターンを生成（点対称）。
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            if (XgOnGenerateBlacks(hwnd, true)) {
                sa2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            }
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_SOLVE:  // 解を求める。
        // ルール「黒マス線対称」では黒マス追加ありの解を求めることはできません。
        if (xg_nRules & RULE_POINTSYMMETRY) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVESYMMETRY), NULL, MB_ICONERROR);
            return;
        }
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                // 解を求める。
                XgOnSolve_AddBlack(hwnd);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_SOLVENOADDBLACK:    // 解を求める（黒マス追加なし）。
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                // 解を求める（黒マス追加なし）。
                XgOnSolve_NoAddBlack(hwnd);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_SOLVEREPEATEDLY:    // 連続で解を求める
        // ルール「黒マス線対称」では黒マス追加ありの解を求めることはできません。
        if (xg_nRules & RULE_POINTSYMMETRY) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVESYMMETRY), NULL, MB_ICONERROR);
            return;
        }
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 連続で解を求める。
            if (XgOnSolveRepeatedly(hwnd)) {
                sa1->Apply();
            }
        }
        // イメージを更新する。
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_SOLVEREPEATEDLYNOADDBLACK:  // 連続で解を求める(黒マス追加なし)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 連続で解を求める。
            if (XgOnSolveRepeatedlyNoAddBlack(hwnd)) {
                sa1->Apply();
            }
        }
        // イメージを更新する。
        xg_caret_pos.clear();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_SHOWSOLUTION:   // 解を表示する。
        xg_bShowAnswer = true;
        XgMarkUpdate();
        // イメージを更新する。
        XgUpdateImage(hwnd, 0, 0);
        break;

    case ID_NOSOLUTION: // 解を表示しない。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        // イメージを更新する。
        XgUpdateImage(hwnd, 0, 0);
        break;

    case ID_SHOWHIDESOLUTION:   // 解の表示を切り替える。
        xg_bShowAnswer = !xg_bShowAnswer;
        XgMarkUpdate();
        // イメージを更新する。
        XgUpdateImage(hwnd, 0, 0);
        break;

    case ID_OPENREADME:     // ReadMeを開く。
        XgOpenReadMe(hwnd);
        break;

    case ID_OPENLICENSE:    // Licenseを開く。
        XgOpenLicense(hwnd);
        break;

    case ID_OPENPATTERNS:    // パターンを開く。
        XgOpenPatterns(hwnd);
        break;

    case ID_ABOUT:      // バージョン情報。
        XgOnAbout(hwnd);
        break;

    case ID_PANENEXT:   // 次のペーン。
        {
            HWND ahwnd[] = {
                xg_hMainWnd,
                xg_hHintsWnd,
                xg_hCandsWnd,
                xg_hwndInputPalette,
            };

            size_t i = 0, k, m, count = ARRAYSIZE(ahwnd);
            for (i = 0; i < count; ++i) {
                if (ahwnd[i] == ::GetForegroundWindow()) {
                    for (k = 1; k < count; ++k) {
                        m = (i + k) % count;
                        if (::IsWindow(ahwnd[m])) {
                            ::SetForegroundWindow(ahwnd[m]);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        break;

    case ID_PANEPREV:   // 前のペーン。
        {
            HWND ahwnd[] = {
                xg_hwndInputPalette,
                xg_hCandsWnd,
                xg_hHintsWnd,
                xg_hMainWnd,
            };

            size_t i = 0, k, m, count = ARRAYSIZE(ahwnd);
            for (i = 0; i < count; ++i) {
                if (ahwnd[i] == ::GetForegroundWindow()) {
                    for (k = 1; k < count; ++k) {
                        m = (i + k) % count;
                        if (::IsWindow(ahwnd[m])) {
                            ::SetForegroundWindow(ahwnd[m]);
                            break;
                        }
                    }
                    break;
                }
            }
        }
        break;

    case ID_CUT:    // 切り取り。
        if (::GetForegroundWindow() == xg_hMainWnd) {
            ;
        } else {
            HWND hwnd = ::GetFocus();
            ::SendMessageW(hwnd, WM_CUT, 0, 0);
        }
        break;

    case ID_COPY:   // コピー。
        if (::GetForegroundWindow() == xg_hMainWnd) {
            XgCopyBoard(hwnd);
        } else {
            HWND hwnd = ::GetFocus();
            ::SendMessageW(hwnd, WM_COPY, 0, 0);
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_COPYASIMAGE:    // 画像をコピー。
        if (::GetForegroundWindow() == xg_hMainWnd) {
            XgCopyBoardAsImage(hwnd);
        }
        break;

    case ID_COPYASIMAGESIZED:   // サイズを指定して画像をコピー。
        if (::GetForegroundWindow() == xg_hMainWnd) {
            XgCopyBoardAsImageSized(hwnd);
        }
        break;

    case ID_COPYMARKWORDASIMAGE: // 二重マス単語の画像をコピー。
        XgCopyMarkWordAsImage(hwnd);
        break;

    case ID_COPYMARKWORDASIMAGESIZED:   // 高さを指定して二重マス単語の画像をコピー。
        XgCopyMarkWordAsImageSized(hwnd);
        break;

    case ID_PASTE:  // 貼り付け。
        if (::GetForegroundWindow() == xg_hMainWnd) {
            std::wstring str = XgGetClipboardUnicodeText(hwnd);
            if (str.find(ZEN_ULEFT) != std::wstring::npos &&
                str.find(ZEN_LRIGHT) != std::wstring::npos)
            {
                auto sa1 = std::make_shared<XG_UndoData_SetAll>();
                auto sa2 = std::make_shared<XG_UndoData_SetAll>();
                sa1->Get();
                {
                    // 盤の貼り付け。
                    XgPasteBoard(hwnd, str);
                }
                sa2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            } else {
                // 単語の貼り付け。
                for (auto& ch : str) {
                    MainWnd_OnImeChar(hwnd, ch, 0);
                }
            }
        } else {
            HWND hwnd = ::GetFocus();
            ::SendMessageW(hwnd, WM_PASTE, 0, 0);
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_KANAINPUT:  // カナ入力モード。
        XgSetInputMode(hwnd, xg_im_KANA);
        break;

    case ID_ABCINPUT:   // 英字入力モード。
        XgSetInputMode(hwnd, xg_im_ABC);
        break;

    case ID_KANJIINPUT: // 漢字入力モード。
        XgSetInputMode(hwnd, xg_im_KANJI);
        break;

    case ID_RUSSIAINPUT: // ロシア入力モード。
        XgSetInputMode(hwnd, xg_im_RUSSIA);
        break;

    case ID_DIGITINPUT: // 数字入力モード。
        XgSetInputMode(hwnd, xg_im_DIGITS);
        break;

    case ID_SHOWHIDEHINTS:
        if (IsWindow(xg_hHintsWnd)) {
            ::DestroyWindow(xg_hHintsWnd);
            xg_hHintsWnd = NULL;
        } else {
            XgShowHints(hwnd);
            ::SetForegroundWindow(xg_hHintsWnd);
        }
        break;

    case ID_MARKSNEXT:  // 次の二重マス単語
        {
            auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
            auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
            mu1->Get();
            {
                XgGetNextMarkedWord();
            }
            mu2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
        }
        break;

    case ID_MARKSPREV:  // 前の二重マス単語
        {
            auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
            auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
            mu1->Get();
            {
                XgGetPrevMarkedWord();
            }
            mu2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
        }
        break;

    case ID_KILLMARKS:  // 二重マスをすべて解除する
        {
            auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
            auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
            mu1->Get();
            {
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
                XgMarkUpdate();
                // イメージを更新する。
                XgUpdateImage(hwnd, 0, 0);
            }
            mu2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
        }
        break;

    case ID_PRINTPROBLEM:   // 問題のみを印刷する。
        XgPrintProblem();
        break;

    case ID_PRINTANSWER:    // 問題と解答を印刷する。
        if (xg_bSolved)
            XgPrintAnswer();
        else
            ::MessageBeep(0xFFFFFFFF);
        break;

    case ID_OPENHOMEPAGE:   // ホームページを開く。
        {
            static LPCWSTR s_pszHomepage = L"http://katahiromz.web.fc2.com/";
            ::ShellExecuteW(hwnd, NULL, s_pszHomepage, NULL, NULL, SW_SHOWNORMAL);
        }
        break;

    case ID_OPENBBS:        // 掲示板を開く。
        {
            static LPCWSTR s_pszBBS = L"http://katahiromz.bbs.fc2.com/";
            ::ShellExecuteW(hwnd, NULL, s_pszBBS, NULL, NULL, SW_SHOWNORMAL);
        }
        break;

    case ID_CHARFEED:
        XgSetCharFeed(hwnd, -1);
        break;

    case ID_INPUTH:
        XgInputDirection(hwnd, 0);
        break;

    case ID_INPUTV:
        XgInputDirection(hwnd, 1);
        break;

    case ID_STATUS:
        s_bShowStatusBar = !s_bShowStatusBar;
        if (s_bShowStatusBar)
            ShowWindow(xg_hStatusBar, SW_SHOWNOACTIVATE);
        else
            ShowWindow(xg_hStatusBar, SW_HIDE);
        {
            int x = XgGetHScrollPos();
            int y = XgGetVScrollPos();
            XgUpdateScrollInfo(hwnd, x, y);
        }
        PostMessageW(hwnd, WM_SIZE, 0, 0);
        break;
    case ID_PALETTE:
    case ID_PALETTE2:
        if (xg_hwndInputPalette) {
            XgDestroyInputPalette();
        } else {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_VIEW50PER:
        break;
    case ID_VIEW75PER:
        break;
    case ID_VIEW100PER:
        break;
    case ID_VIEW150PER:
        break;
    case ID_VIEW200PER:
        break;
    case ID_BACK:
        XgCharBack(hwnd);
        break;
    case ID_INPUTHV:
        XgInputDirection(hwnd, -1);
        break;
    case ID_RETURN:
        XgReturn(hwnd);
        break;
    case ID_BLOCK:
        PostMessageW(hwnd, WM_CHAR, L'#', 0);
        break;
    case ID_SPACE:
        PostMessageW(hwnd, WM_CHAR, L'_', 0);
        break;
    case ID_CLEARNONBLOCKS:
        XgClearNonBlocks(hwnd);
        break;
    case ID_ONLINEDICT:
        XgOnlineDict(hwnd, xg_bTateInput);
        break;
    case ID_ONLINEDICTV:
        XgOnlineDict(hwnd, TRUE);
        break;
    case ID_ONLINEDICTH:
        XgOnlineDict(hwnd, FALSE);
        break;
    case ID_TOGGLEMARK:
        XgToggleMark(hwnd);
        break;
    case ID_TOOLBAR:
        s_bShowToolBar = !s_bShowToolBar;
        if (s_bShowToolBar) {
            ::ShowWindow(xg_hToolBar, SW_SHOWNOACTIVATE);
        } else {
            ::ShowWindow(xg_hToolBar, SW_HIDE);
        }
        PostMessageW(hwnd, WM_SIZE, 0, 0);
        break;
    case ID_HELPDICTSITE:
        ShellExecuteW(hwnd, NULL, XgLoadStringDx1(IDS_MATRIXSEARCH), NULL, NULL, SW_SHOWNORMAL);
        break;
    case ID_BLOCKNOFEED:
        {
            bool bOldFeed = xg_bCharFeed;
            xg_bCharFeed = false;
            SendMessageW(hwnd, WM_CHAR, L'#', 0);
            xg_bCharFeed = bOldFeed;
        }
        break;
    case ID_SPACENOFEED:
        {
            bool bOldFeed = xg_bCharFeed;
            xg_bCharFeed = false;
            SendMessageW(hwnd, WM_CHAR, L'_', 0);
            xg_bCharFeed = bOldFeed;
        }
        break;
    case ID_SHOWHIDENUMBERING:
        xg_bShowNumbering = !xg_bShowNumbering;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
        break;
    case ID_SHOWHIDECARET:
        xg_bShowCaret = !xg_bShowCaret;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
        break;
    case ID_ZOOMIN:
        if (xg_nZoomRate < 50) {
            xg_nZoomRate += 5;
        } else if (xg_nZoomRate < 100) {
            xg_nZoomRate += 10;
        } else if (xg_nZoomRate < 200) {
            xg_nZoomRate += 20;
        }
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateScrollInfo(hwnd, x, y);
        XgUpdateImage(hwnd, x, y);
        break;
    case ID_ZOOMOUT:
        if (xg_nZoomRate > 200) {
            xg_nZoomRate -= 20;
        } else if (xg_nZoomRate > 100) {
            xg_nZoomRate -= 10;
        } else if (xg_nZoomRate > 50) {
            xg_nZoomRate -= 5;
        }
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateScrollInfo(hwnd, x, y);
        XgUpdateImage(hwnd, x, y);
        break;
    case ID_ZOOM100:
        XgSetZoomRate(hwnd, 100);
        break;
    case ID_ZOOM30:
        XgSetZoomRate(hwnd, 30);
        break;
    case ID_ZOOM50:
        XgSetZoomRate(hwnd, 50);
        break;
    case ID_ZOOM65:
        XgSetZoomRate(hwnd, 65);
        break;
    case ID_ZOOM80:
        XgSetZoomRate(hwnd, 80);
        break;
    case ID_ZOOM90:
        XgSetZoomRate(hwnd, 90);
        break;
    case ID_COPYWORDHORZ:
        MainWnd_OnCopyPatternHorz(hwnd);
        break;
    case ID_COPYWORDVERT:
        MainWnd_OnCopyPatternVert(hwnd);
        break;
    case ID_UPPERCASE:
        xg_bLowercase = FALSE;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_LOWERCASE:
        xg_bLowercase = TRUE;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_HIRAGANA:
        xg_bHiragana = TRUE;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_KATAKANA:
        xg_bHiragana = FALSE;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateImage(hwnd, x, y);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_DICTIONARY00:
    case ID_DICTIONARY01:
    case ID_DICTIONARY02:
    case ID_DICTIONARY03:
    case ID_DICTIONARY04:
    case ID_DICTIONARY05:
    case ID_DICTIONARY06:
    case ID_DICTIONARY07:
    case ID_DICTIONARY08:
    case ID_DICTIONARY09:
    case ID_DICTIONARY10:
    case ID_DICTIONARY11:
    case ID_DICTIONARY12:
    case ID_DICTIONARY13:
    case ID_DICTIONARY14:
    case ID_DICTIONARY15:
        MainWnd_DoDictionary(hwnd, id - ID_DICTIONARY00);
        XgResetTheme(hwnd, FALSE);
        break;
    case ID_RESETRULES:
        if (XgIsUserJapanese())
            xg_nRules = DEFAULT_RULES_JAPANESE;
        else
            xg_nRules = DEFAULT_RULES_ENGLISH;
        xg_bSkeltonMode = FALSE;
        XgUpdateRules(hwnd);
        break;
    case ID_OPENRULESTXT:
        OnOpenRulesTxt(hwnd);
        break;
    case ID_RULE_DONTDOUBLEBLACK:
        if (xg_nRules & RULE_DONTDOUBLEBLACK) {
            xg_nRules &= ~RULE_DONTDOUBLEBLACK;
        } else {
            xg_nRules |= RULE_DONTDOUBLEBLACK;
            if (xg_bSkeltonMode) {
                xg_bSkeltonMode = FALSE;
            }
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTCORNERBLACK:
        if (xg_nRules & RULE_DONTCORNERBLACK) {
            xg_nRules &= ~RULE_DONTCORNERBLACK;
        } else {
            xg_nRules |= RULE_DONTCORNERBLACK;
            if (xg_bSkeltonMode) {
                xg_bSkeltonMode = FALSE;
            }
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTTRIDIRECTIONS:
        if (xg_nRules & RULE_DONTTRIDIRECTIONS) {
            xg_nRules &= ~RULE_DONTTRIDIRECTIONS;
        } else {
            xg_nRules |= RULE_DONTTRIDIRECTIONS;
            if (xg_bSkeltonMode) {
                xg_bSkeltonMode = FALSE;
            }
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTDIVIDE:
        if (xg_nRules & RULE_DONTDIVIDE) {
            xg_nRules &= ~RULE_DONTDIVIDE;
        } else {
            xg_nRules |= RULE_DONTDIVIDE;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTTHREEDIAGONALS:
        if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~(RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
        } else if (!(xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules |= RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS;
            xg_bSkeltonMode = FALSE;
        } else if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && !(xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTTHREEDIAGONALS;
        } else {
            xg_nRules |= (RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
            xg_bSkeltonMode = FALSE;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTFOURDIAGONALS:
        if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTTHREEDIAGONALS;
            xg_nRules |= RULE_DONTFOURDIAGONALS;
            xg_bSkeltonMode = FALSE;
        } else if (!(xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~(RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
        } else if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && !(xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTFOURDIAGONALS;
        } else {
            xg_nRules |= RULE_DONTFOURDIAGONALS;
            xg_bSkeltonMode = FALSE;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_POINTSYMMETRY:
        if (xg_nRules & RULE_POINTSYMMETRY) {
            xg_nRules &= ~RULE_POINTSYMMETRY;
        } else {
            xg_nRules |= RULE_POINTSYMMETRY;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULECHECK:
        XgRuleCheck(hwnd);
        break;
    case ID_THEME:
        XgTheme(hwnd);
        break;
    case ID_RESETTHEME:
        XgResetTheme(hwnd, TRUE);
        break;
    case ID_RETRYIFTIMEOUT:
        s_bAutoRetry = !s_bAutoRetry;
        break;
    case ID_SKELTONMODE:
        if (xg_bSkeltonMode) {
            xg_bSkeltonMode = FALSE;
            xg_nRules |= (RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | RULE_DONTTRIDIRECTIONS | RULE_DONTFOURDIAGONALS);
        } else {
            xg_bSkeltonMode = TRUE;
            xg_nRules &= ~(RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | RULE_DONTTRIDIRECTIONS | RULE_DONTFOURDIAGONALS | RULE_DONTTHREEDIAGONALS);
        }
        XgUpdateRules(hwnd);
        break;
    case ID_SEQPATGEN:
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            if (XgOnGenerateBlacksRepeatedly(hwnd)) {
                // クリアする。
                xg_bShowAnswer = false;
                xg_bSolved = false;
                xg_xword.clear();
                xg_solution.clear();
                // 元に戻す情報を残す。
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            }
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;
    case ID_FILLBYBLOCKS:
        XgNewCells(hwnd, ZEN_BLACK, xg_nRows, xg_nCols);
        break;
    case ID_FILLBYWHITES:
        XgNewCells(hwnd, ZEN_SPACE, xg_nRows, xg_nCols);
        break;
    case ID_ERASESOLUTIONANDUNLOCKEDIT:
        {
            std::wstring str;
            XG_Board *pxw = (xg_bSolved && xg_bShowAnswer) ? &xg_solution : &xg_xword;
            pxw->GetString(str);
            XgPasteBoard(hwnd, str);
        }
        break;
    case ID_RELOADDICTS:
        XgLoadDictsAll();
        break;
    default:
        if (!MainWnd_OnCommand2(hwnd, id)) {
            ::MessageBeep(0xFFFFFFFF);
        }
        break;
    }

    XgUpdateStatusBar(hwnd);
}

// ファイルがドロップされた。
void __fastcall MainWnd_OnDropFiles(HWND hwnd, HDROP hDrop)
{
    // 最初のファイルのパス名を取得する。
    WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
    ::DragQueryFileW(hDrop, 0, szFile, ARRAYSIZE(szFile));
    ::DragFinish(hDrop);

    // ショートカットだった場合は、ターゲットのパスを取得する。
    if (XgGetPathOfShortcutW(szFile, szTarget))
        StringCbCopy(szFile, sizeof(szFile), szTarget);

    // 拡張子を取得する。
    LPWSTR pch = PathFindExtensionW(szFile);

    if (::lstrcmpiW(pch, L".xwd") == 0) {
        // 拡張子が.xwdだった。ファイルを開く。
        if (!XgDoLoadFile(hwnd, szFile, false)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
        } else {
            xg_caret_pos.clear();
            // テーマを更新する。
            XgSetThemeString(xg_strTheme);
            XgUpdateTheme(hwnd);
            // イメージを更新する。
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
    } else if (::lstrcmpiW(pch, L".xwj") == 0 || lstrcmpiW(pch, L".json") == 0) {
        // 拡張子が.xwjか.jsonだった。ファイルを開く。
        if (!XgDoLoadFile(hwnd, szFile, true)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
        } else {
            xg_caret_pos.clear();
            // テーマを更新する。
            XgSetThemeString(xg_strTheme);
            XgUpdateTheme(hwnd);
            // イメージを更新する。
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
    } else if (::lstrcmpiW(pch, L".crp") == 0 || ::lstrcmpiW(pch, L".crx") == 0) {
        if (!XgDoLoadCrpFile(hwnd, szFile)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
        } else {
            xg_caret_pos.clear();
            // テーマを更新する。
            XgSetThemeString(xg_strTheme);
            XgUpdateTheme(hwnd);
            // イメージを更新する。
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
    } else {
        ::MessageBeep(0xFFFFFFFF);
    }

    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
    // ルールを更新する。
    XgUpdateRules(hwnd);
}

// 無効状態のビットマップを作成する。
HBITMAP XgCreateGrayedBitmap(HBITMAP hbm, COLORREF crMask = CLR_INVALID)
{
    HDC hdc = ::GetDC(NULL);
    if (::GetDeviceCaps(hdc, BITSPIXEL) < 24) {
        HPALETTE hPal = reinterpret_cast<HPALETTE>(::GetCurrentObject(hdc, OBJ_PAL));
        UINT index = ::GetNearestPaletteIndex(hPal, crMask);
        if (index != CLR_INVALID)
            crMask = PALETTEINDEX(index);
    }
    ::ReleaseDC(NULL, hdc);

    BITMAP bm;
    ::GetObject(hbm, sizeof(bm), &bm);

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = bm.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = bm.bmWidth;
    rc.bottom = bm.bmHeight;

    HDC hdc1 = ::CreateCompatibleDC(NULL);
    HDC hdc2 = ::CreateCompatibleDC(NULL);

    LPVOID pvBits;
    HBITMAP hbmNew = ::CreateDIBSection(
        NULL, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    assert(hbmNew);
    if (hbmNew) {
        HGDIOBJ hbm1Old = ::SelectObject(hdc1, hbm);
        HGDIOBJ hbm2Old = ::SelectObject(hdc2, hbmNew);
        if (crMask == CLR_INVALID) {
            HBRUSH hbr = ::CreateSolidBrush(crMask);
            ::FillRect(hdc2, &rc, hbr);
            ::DeleteObject(hbr);
        }

        BYTE by;
        COLORREF cr;
        if (crMask == CLR_INVALID) {
            for (int y = 0; y < bm.bmHeight; ++y) {
                for (int x = 0; x < bm.bmWidth; ++x) {
                    cr = ::GetPixel(hdc1, x, y);
                    by = BYTE(
                        95 + (
                            GetRValue(cr) * 3 +
                            GetGValue(cr) * 6 +
                            GetBValue(cr)
                        ) / 20
                    );
                    ::SetPixelV(hdc2, x, y, RGB(by, by, by));
                }
            }
        } else {
            for (int y = 0; y < bm.bmHeight; ++y) {
                for (int x = 0; x < bm.bmWidth; ++x) {
                    cr = ::GetPixel(hdc1, x, y);
                    if (cr != crMask)
                    {
                        by = BYTE(
                            95 + (
                                GetRValue(cr) * 3 +
                                GetGValue(cr) * 6 +
                                GetBValue(cr)
                            ) / 20
                        );
                        ::SetPixelV(hdc2, x, y, RGB(by, by, by));
                    }
                }
            }
        }
        ::SelectObject(hdc1, hbm1Old);
        ::SelectObject(hdc2, hbm2Old);
    }

    ::DeleteDC(hdc1);
    ::DeleteDC(hdc2);

    return hbmNew;
}

// ウィンドウの作成の際に呼ばれる。
bool __fastcall MainWnd_OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
{
    xg_hMainWnd = hwnd;

    // 水平スクロールバーを作る。
    xg_hHScrollBar = ::CreateWindowW(
        L"SCROLLBAR", NULL,
        WS_CHILD | WS_VISIBLE | SBS_HORZ,
        0, 0, 0, 0,
        hwnd, NULL, xg_hInstance, NULL);
    if (xg_hHScrollBar == NULL)
        return false;

    // 垂直スクロールバーを作る。
    xg_hVScrollBar = ::CreateWindowW(
        L"SCROLLBAR", NULL,
        WS_CHILD | WS_VISIBLE | SBS_VERT,
        0, 0, 0, 0,
        hwnd, NULL, xg_hInstance, NULL);
    if (xg_hVScrollBar == NULL)
        return false;

    // サイズグリップを作る。
    xg_hSizeGrip = ::CreateWindowW(
        L"SCROLLBAR", NULL,
        WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP,
        0, 0, 0, 0,
        hwnd, NULL, xg_hInstance, NULL);
    if (xg_hSizeGrip == NULL)
        return false;

    // イメージリストの準備をする。
    xg_hImageList = ::ImageList_Create(32, 32, ILC_COLOR24, 0, 0);
    if (xg_hImageList == NULL)
        return FALSE;
    xg_hGrayedImageList = ::ImageList_Create(32, 32, ILC_COLOR24, 0, 0);
    if (xg_hGrayedImageList == NULL)
        return FALSE;

    ::ImageList_SetBkColor(xg_hImageList, CLR_NONE);
    ::ImageList_SetBkColor(xg_hGrayedImageList, CLR_NONE);

    HBITMAP hbm = reinterpret_cast<HBITMAP>(::LoadImageW(
        xg_hInstance, MAKEINTRESOURCE(1),
        IMAGE_BITMAP,
        0, 0,
        LR_COLOR));
    ::ImageList_Add(xg_hImageList, hbm, NULL);
    HBITMAP hbmGrayed = XgCreateGrayedBitmap(hbm);
    ::ImageList_Add(xg_hGrayedImageList, hbmGrayed, NULL);
    ::DeleteObject(hbm);
    ::DeleteObject(hbmGrayed);

    // ツールバーのボタン情報。
    static const TBBUTTON atbb[] = {
        {0, ID_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {1, ID_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {2, ID_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {3, ID_GENERATE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {4, ID_GENERATEANSWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {5, ID_GENERATEREPEATEDLY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {6, ID_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {7, ID_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {8, ID_SOLVE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {9, ID_SOLVENOADDBLACK, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {10, ID_SOLVEREPEATEDLY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {11, ID_SOLVEREPEATEDLYNOADDBLACK, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {12, ID_PRINTPROBLEM, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {13, ID_PRINTANSWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
    };

    xg_hStatusBar = ::CreateStatusWindow(WS_CHILD | WS_VISIBLE, L"", hwnd, 256);
    if (xg_hStatusBar == NULL)
        return FALSE;

    XgUpdateStatusBar(hwnd);

    // ツールバーを作成する。
    const int c_IDW_TOOLBAR = 1;
    xg_hToolBar = ::CreateWindowW(
        TOOLBARCLASSNAMEW, NULL, 
        WS_CHILD | CCS_TOP | TBSTYLE_TOOLTIPS,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(UINT_PTR(c_IDW_TOOLBAR)),
        xg_hInstance,
        NULL);
    if (xg_hToolBar == NULL)
        return FALSE;

    // ツールバーを初期化する。
    ::SendMessageW(xg_hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    ::SendMessageW(xg_hToolBar, TB_SETBITMAPSIZE, 0, MAKELPARAM(32, 32));
    ::SendMessageW(xg_hToolBar, TB_SETIMAGELIST, 0, (LPARAM)xg_hImageList);
    ::SendMessageW(xg_hToolBar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)xg_hGrayedImageList);
    ::SendMessageW(xg_hToolBar, TB_ADDBUTTONS, ARRAYSIZE(atbb), (LPARAM)atbb);
    ::SendMessageW(xg_hToolBar, WM_SIZE, 0, 0);

    if (s_bShowToolBar)
        ::ShowWindow(xg_hToolBar, SW_SHOWNOACTIVATE);
    else
        ::ShowWindow(xg_hToolBar, SW_HIDE);
    
    if (s_bShowStatusBar)
        ::ShowWindow(xg_hStatusBar, SW_SHOWNOACTIVATE);
    else
        ::ShowWindow(xg_hStatusBar, SW_HIDE);

    if (xg_bShowInputPalette)
        XgCreateInputPalette(hwnd);

    // パソコンが古い場合、警告を表示する。
    if (s_dwNumberOfProcessors <= 1 && !s_bOldNotice) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_TOOOLDPC), XgLoadStringDx2(IDS_APPNAME),
                          MB_ICONWARNING | MB_OK);
        s_bOldNotice = true;
    }

    // クロスワードを初期化する。
    xg_xword.clear();

    // 辞書ファイルを読み込む。
    if (xg_dict_name.size())
        XgLoadDictFile(xg_dict_name.c_str());

    // ファイルドロップを受け付ける。
    ::DragAcceptFiles(hwnd, TRUE);

    // イメージを更新する。
    XgUpdateImage(hwnd, 0, 0);

    int argc;
    LPWSTR *wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argc >= 2) {
        WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
        StringCbCopy(szFile, sizeof(szFile), wargv[1]);

        // コマンドライン引数があれば、それを開く。
        bool bSuccess = true;
        if (::lstrcmpiW(PathFindExtensionW(szFile), s_szShellLinkDotExt) == 0)
        {
            // ショートカットだった場合は、ターゲットのパスを取得する。
            if (XgGetPathOfShortcutW(szFile, szTarget)) {
                StringCbCopy(szFile, sizeof(szFile), szTarget);
            } else {
                bSuccess = false;
                MessageBeep(0xFFFFFFFF);
            }
        }
        bool is_json = false;
        if (::lstrcmpiW(PathFindExtensionW(szFile), L".xwj") == 0 ||
            ::lstrcmpiW(PathFindExtensionW(szFile), L".json") == 0)
        {
            is_json = true;
        }
        bool is_builder = false;
        if (::lstrcmpiW(PathFindExtensionW(szFile), L".crp") == 0 ||
            ::lstrcmpiW(PathFindExtensionW(szFile), L".crx") == 0)
        {
            is_builder = true;
        }
        if (bSuccess) {
            if (is_builder) {
                if (!XgDoLoadCrpFile(hwnd, szFile)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
                } else {
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            } else {
                if (!XgDoLoadFile(hwnd, szFile, is_json)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
                } else {
                    xg_caret_pos.clear();
                    // テーマを更新する。
                    XgSetThemeString(xg_strTheme);
                    XgUpdateTheme(hwnd);
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            }
            // ルールを更新する。
            XgUpdateRules(hwnd);
        }
    }
    GlobalFree(wargv);

    ::PostMessageW(hwnd, WM_SIZE, 0, 0);
    // ルールを更新する。
    XgUpdateRules(hwnd);
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);

    return true;
}

// マウスホイールが回転した。
void __fastcall
MainWnd_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
    POINT pt = {xPos, yPos};

    RECT rc;
    if (::GetWindowRect(xg_hHintsWnd, &rc) && ::PtInRect(&rc, pt)) {
        FORWARD_WM_MOUSEWHEEL(xg_hHintsWnd, rc.left, rc.top,
            zDelta, fwKeys, ::SendMessageW);
    } else {
        if (::GetAsyncKeyState(VK_CONTROL) < 0) {
            if (zDelta < 0)
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMOUT, 0);
            else if (zDelta > 0)
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMIN, 0);
        } else if (::GetAsyncKeyState(VK_SHIFT) < 0) {
            if (zDelta < 0)
                ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            else if (zDelta > 0)
                ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
        } else {
            if (zDelta < 0)
                ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            else if (zDelta > 0)
                ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
        }
    }
}

// ポップアップメニューを読み込む。
HMENU XgLoadPopupMenu(HWND hwnd, INT nPos)
{
    HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCE(2));
    HMENU hSubMenu = GetSubMenu(hMenu, nPos);

    // POPUP "カナ"がインデックス5。
    // POPUP "英字"がインデックス6。
    // POPUP "ロシア"がインデックス7。
    // POPUP "数字"がインデックス8。
    // 大きいインデックスのメニュー項目から削除。
    switch (xg_imode)
    {
    case xg_im_ABC:
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_KANA:
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        break;
    case xg_im_KANJI:
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_RUSSIA:
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_DIGITS:
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    default:
        break;
    }

    return hMenu;
}

// 右クリックされた。
void
MainWnd_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    MainWnd_OnLButtonUp(hwnd, x, y, keyFlags);

    HMENU hMenu = XgLoadPopupMenu(hwnd, 0);
    HMENU hSubMenu = GetSubMenu(hMenu, 0);

    // スクリーン座標へ変換する。
    POINT pt;
    pt.x = x;
    pt.y = y;
    ::ClientToScreen(hwnd, &pt);

    // 右クリックメニューを表示する。
    ::SetForegroundWindow(hwnd);
    ::TrackPopupMenu(
        hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN,
        pt.x, pt.y, 0, hwnd, NULL);
    ::PostMessageW(hwnd, WM_NULL, 0, 0);

    ::DestroyMenu(hMenu);
}

// 通知。
void MainWnd_OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    if (pnmh->code == TTN_NEEDTEXT) {
        // ツールチップの情報をセットする。
        LPTOOLTIPTEXT pttt;
        pttt = reinterpret_cast<LPTOOLTIPTEXT>(pnmh);
        pttt->hinst = xg_hInstance;
        pttt->lpszText = MAKEINTRESOURCE(pttt->hdr.idFrom + ID_TT_BASE);
        assert(IDS_TT_NEW == ID_TT_BASE + ID_NEW);
        assert(IDS_TT_GENERATE == ID_TT_BASE + ID_GENERATE);
        assert(IDS_TT_GENERATEANSWER == ID_TT_BASE + ID_GENERATEANSWER);
        assert(IDS_TT_OPEN == ID_TT_BASE + ID_OPEN);
        assert(IDS_TT_SAVEAS == ID_TT_BASE + ID_SAVEAS);
        assert(IDS_TT_SOLVE == ID_TT_BASE + ID_SOLVE);
        assert(IDS_TT_COPY == ID_TT_BASE + ID_COPY);
        assert(IDS_TT_PASTE == ID_TT_BASE + ID_PASTE);
        assert(IDS_TT_PRINTPROBLEM == ID_TT_BASE + ID_PRINTPROBLEM);
        assert(IDS_TT_PRINTANSWER == ID_TT_BASE + ID_PRINTANSWER);
        assert(IDS_TT_SOLVENOADDBLACK == ID_TT_BASE + ID_SOLVENOADDBLACK);
        assert(IDS_TT_GENERATEREPEATEDLY == ID_TT_BASE + ID_GENERATEREPEATEDLY);
        assert(IDS_TT_SOLVEREPEATEDLY == ID_TT_BASE + ID_SOLVEREPEATEDLY);
        assert(IDS_TT_SOLVEREPEATEDLYNOADDBLACK == ID_TT_BASE + ID_SOLVEREPEATEDLYNOADDBLACK);
    }
}

// ウィンドウのサイズを制限する。
void MainWnd_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMinTrackSize.x = 300;
    lpMinMaxInfo->ptMinTrackSize.y = 150;
}

//////////////////////////////////////////////////////////////////////////////

// ウィンドウプロシージャ。
extern "C"
LRESULT CALLBACK
XgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, MainWnd_OnCreate);
    HANDLE_MSG(hWnd, WM_DESTROY, MainWnd_OnDestroy);
    HANDLE_MSG(hWnd, WM_PAINT, MainWnd_OnPaint);
    HANDLE_MSG(hWnd, WM_HSCROLL, MainWnd_OnHScroll);
    HANDLE_MSG(hWnd, WM_VSCROLL, MainWnd_OnVScroll);
    HANDLE_MSG(hWnd, WM_MOVE, MainWnd_OnMove);
    HANDLE_MSG(hWnd, WM_SIZE, MainWnd_OnSize);
    HANDLE_MSG(hWnd, WM_KEYDOWN, MainWnd_OnKey);
    HANDLE_MSG(hWnd, WM_KEYUP, MainWnd_OnKey);
    HANDLE_MSG(hWnd, WM_CHAR, MainWnd_OnChar);
    HANDLE_MSG(hWnd, WM_LBUTTONDBLCLK, MainWnd_OnLButtonDown);
    HANDLE_MSG(hWnd, WM_LBUTTONUP, MainWnd_OnLButtonUp);
    HANDLE_MSG(hWnd, WM_RBUTTONDOWN, MainWnd_OnRButtonDown);
    HANDLE_MSG(hWnd, WM_COMMAND, MainWnd_OnCommand);
    HANDLE_MSG(hWnd, WM_INITMENU, MainWnd_OnInitMenu);
    HANDLE_MSG(hWnd, WM_DROPFILES, MainWnd_OnDropFiles);
    HANDLE_MSG(hWnd, WM_MOUSEWHEEL, MainWnd_OnMouseWheel);
    HANDLE_MSG(hWnd, WM_GETMINMAXINFO, MainWnd_OnGetMinMaxInfo);
    case WM_NOTIFY:
        MainWnd_OnNotify(hWnd, static_cast<int>(wParam), reinterpret_cast<LPNMHDR>(lParam));
        break;

    case WM_IME_CHAR:
        MainWnd_OnImeChar(hWnd, static_cast<WCHAR>(wParam), lParam);
        break;

    default:
        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// ヒントウィンドウ。

// ヒントウィンドウのスクロールビュー。
MScrollView         xg_svHintsScrollView;

// ヒントウィンドウのUIフォント。
HFONT               xg_hHintsUIFont = NULL;

// 縦のカギのコントロール群。
HWND                xg_hwndTateCaptionStatic = NULL;
std::vector<HWND>   xg_ahwndTateStatics;
std::vector<HWND>   xg_ahwndTateEdits;

// 横のカギのコントロール群。
HWND                xg_hwndYokoCaptionStatic = NULL;
std::vector<HWND>   xg_ahwndYokoStatics;
std::vector<HWND>   xg_ahwndYokoEdits;

// ヒントウィンドウのサイズが変わった。
void HintsWnd_OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/)
{
    if (xg_hwndTateCaptionStatic == NULL)
        return;

    xg_svHintsScrollView.clear();

    MRect rcClient;
    ::GetClientRect(hwnd, &rcClient);

    MSize size1, size2;
    {
        HDC hdc = ::CreateCompatibleDC(NULL);
        WCHAR label[64];
        StringCbPrintf(label, sizeof(label), XgLoadStringDx1(IDS_DOWNNUMBER), 100);
        std::wstring strLabel = label;
        ::SelectObject(hdc, ::GetStockObject(SYSTEM_FIXED_FONT));
        ::GetTextExtentPoint32W(hdc, strLabel.data(), int(strLabel.size()), &size1);
        ::SelectObject(hdc, xg_hHintsUIFont);
        ::GetTextExtentPoint32W(hdc, strLabel.data(), int(strLabel.size()), &size2);
        ::DeleteDC(hdc);
    }

    WCHAR szText[512];
    HDC hdc = ::CreateCompatibleDC(NULL);
    HGDIOBJ hFontOld = ::SelectObject(hdc, xg_hHintsUIFont);
    int y = 0;

    // タテのカギ。
    {
        MRect rcCtrl(MPoint(0, y + 4), 
                     MSize(rcClient.Width(), size1.cy + 4));
        xg_svHintsScrollView.AddCtrlInfo(xg_hwndTateCaptionStatic, rcCtrl);
        y += size1.cy + 8;
    }
    if (rcClient.Width() - size2.cx - 8 > 0) {
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            ::GetWindowTextW(xg_ahwndTateEdits[i], szText,
                             ARRAYSIZE(szText));
            MRect rcCtrl(MPoint(size2.cx, y),
                         MSize(rcClient.Width() - size2.cx - 8, 0));
            if (szText[0] == 0) {
                szText[0] = L' ';
                szText[1] = 0;
            }
            ::DrawTextW(hdc, szText, -1, &rcCtrl,
                DT_LEFT | DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK);
            rcCtrl.right = rcClient.right;
            rcCtrl.bottom += 8;
            xg_svHintsScrollView.AddCtrlInfo(xg_ahwndTateEdits[i], rcCtrl);
            rcCtrl.left = 0;
            rcCtrl.right = size2.cx;
            xg_svHintsScrollView.AddCtrlInfo(xg_ahwndTateStatics[i], rcCtrl);
            y += rcCtrl.Height();
        }
    }
    // ヨコのカギ。
    {
        MRect rcCtrl(MPoint(0, y + 4),
                     MSize(rcClient.Width(), size1.cy + 4));
        xg_svHintsScrollView.AddCtrlInfo(xg_hwndYokoCaptionStatic, rcCtrl);
        y += size1.cy + 8;
    }
    if (rcClient.Width() - size2.cx - 8 > 0) {
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            ::GetWindowTextW(xg_ahwndYokoEdits[i], szText, ARRAYSIZE(szText));
            MRect rcCtrl(MPoint(size2.cx, y),
                         MSize(rcClient.Width() - size2.cx - 8, 0));
            if (szText[0] == 0) {
                szText[0] = L' ';
                szText[1] = 0;
            }
            ::DrawTextW(hdc, szText, -1, &rcCtrl,
                DT_LEFT | DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK);
            rcCtrl.right = rcClient.right;
            rcCtrl.bottom += 8;
            xg_svHintsScrollView.AddCtrlInfo(xg_ahwndYokoEdits[i], rcCtrl);
            rcCtrl.left = 0;
            rcCtrl.right = size2.cx;
            xg_svHintsScrollView.AddCtrlInfo(xg_ahwndYokoStatics[i], rcCtrl);
            y += rcCtrl.Height();
        }
    }

    ::SelectObject(hdc, hFontOld);
    ::DeleteDC(hdc);

    xg_svHintsScrollView.SetExtentForAllCtrls();
    xg_svHintsScrollView.UpdateAll();
}

struct XG_HintEditData
{
    WNDPROC m_fnOldWndProc;
    bool    m_fTate;
};

extern "C"
LRESULT CALLBACK
XgHintEdit_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC fn;
    XG_HintEditData *data =
        reinterpret_cast<XG_HintEditData *>(
            ::GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg) {
    case WM_CHAR:
        if (wParam == L'\r' || wParam == L'\n') {
            // 改行が押された。必要ならばデータを更新する。
            if (XgAreHintsModified()) {
                auto hu1 = std::make_shared<XG_UndoData_HintsUpdated>();
                auto hu2 = std::make_shared<XG_UndoData_HintsUpdated>();
                hu1->Get();
                {
                    // ヒントを更新する。
                    XgUpdateHintsData();
                }
                hu2->Get();
                xg_ubUndoBuffer.Commit(UC_HINTS_UPDATED, hu1, hu2);
            }
        }
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);

    case WM_SETFOCUS: // フォーカスを得た。
        if (wParam) {
            // フォーカスを失うコントロールの選択を解除する。
            HWND hwndLoseFocus = reinterpret_cast<HWND>(wParam);
            ::SendMessageW(hwndLoseFocus, EM_SETSEL, 0, 0);
        }
        // フォーカスのあるコントロールが見えるように移動する。
        xg_svHintsScrollView.EnsureCtrlVisible(hwnd);
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);

    case WM_KILLFOCUS:  // フォーカスを失った。
        // ヒントに更新があれば、データを更新する。
        if (XgAreHintsModified()) {
            auto hu1 = std::make_shared<XG_UndoData_HintsUpdated>();
            auto hu2 = std::make_shared<XG_UndoData_HintsUpdated>();
            hu1->Get();
            {
                // ヒントを更新する。
                XgUpdateHintsData();
            }
            hu2->Get();
            xg_ubUndoBuffer.Commit(UC_HINTS_UPDATED, hu1, hu2);
        }
        // レイアウトを調整する。
        ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);

    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
            ::SetFocus(NULL);
            break;
        }

        if (wParam == VK_TAB) {
            HWND hwndNext;
            if (::GetAsyncKeyState(VK_SHIFT) < 0)
                hwndNext = ::GetNextDlgTabItem(xg_hHintsWnd, hwnd, TRUE);
            else
                hwndNext = ::GetNextDlgTabItem(xg_hHintsWnd, hwnd, FALSE);
            ::SendMessageW(hwnd, EM_SETSEL, 0, 0);
            ::SendMessageW(hwndNext, EM_SETSEL, 0, -1);
            ::SetFocus(hwndNext);
            break;
        }

        if (wParam == VK_PRIOR || wParam == VK_NEXT) {
            HWND hwndParent = ::GetParent(hwnd);
            ::SendMessageW(hwndParent, uMsg, wParam, lParam);
            break;
        }

        if (wParam == VK_F6) {
            if (::GetAsyncKeyState(VK_SHIFT) < 0)
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
            else
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
            break;
        }

        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);

    case WM_NCDESTROY:
        fn = data->m_fnOldWndProc;
        ::LocalFree(data);
        return ::CallWindowProc(fn, hwnd, uMsg, wParam, lParam);

    case WM_MOUSEWHEEL:
        {
            HWND hwndParent = ::GetParent(hwnd);
            ::SendMessageW(hwndParent, uMsg, wParam, lParam);
        }
        break;

    default:
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// ヒントウィンドウが作成された。
BOOL HintsWnd_OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
{
    xg_hHintsWnd = hwnd;

    // 初期化。
    xg_ahwndTateStatics.clear();
    xg_ahwndTateEdits.clear();
    xg_ahwndYokoStatics.clear();
    xg_ahwndYokoEdits.clear();

    xg_svHintsScrollView.SetParent(hwnd);
    xg_svHintsScrollView.ShowScrollBars(FALSE, TRUE);

    if (xg_hHintsUIFont) {
        ::DeleteObject(xg_hHintsUIFont);
    }
    xg_hHintsUIFont = ::CreateFontIndirectW(XgGetUIFont());
    if (xg_hHintsUIFont == NULL) {
        xg_hHintsUIFont =
            reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    }

    HWND hwndCtrl;

    hwndCtrl = ::CreateWindowW(
        TEXT("STATIC"), XgLoadStringDx1(IDS_DOWN),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY |
        SS_CENTER | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
    if (hwndCtrl == NULL)
        return FALSE;
    xg_hwndTateCaptionStatic = hwndCtrl;
    ::SendMessageW(hwndCtrl, WM_SETFONT,
        reinterpret_cast<WPARAM>(::GetStockObject(SYSTEM_FIXED_FONT)),
        TRUE);

    hwndCtrl = ::CreateWindowW(
        TEXT("STATIC"), XgLoadStringDx1(IDS_ACROSS),
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY |
        SS_CENTER | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
    if (hwndCtrl == NULL)
        return FALSE;
    xg_hwndYokoCaptionStatic = hwndCtrl;
    ::SendMessageW(hwndCtrl, WM_SETFONT,
        reinterpret_cast<WPARAM>(::GetStockObject(SYSTEM_FIXED_FONT)),
        TRUE);

    XG_HintEditData *data;
    WCHAR sz[256];
    for (const auto& hint : xg_vecTateHints) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_DOWNNUMBER), hint.m_number);
        hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz,
            WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        assert(hwndCtrl);
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(xg_hHintsUIFont),
            TRUE);
        if (hwndCtrl == NULL)
            return FALSE;

        xg_ahwndTateStatics.emplace_back(hwndCtrl);

        hwndCtrl = ::CreateWindowExW(
            WS_EX_CLIENTEDGE,
            TEXT("EDIT"), hint.m_strHint.data(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        assert(hwndCtrl);
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(xg_hHintsUIFont),
            TRUE);
        if (hwndCtrl == NULL)
            return FALSE;

        data = reinterpret_cast<XG_HintEditData *>(
            ::LocalAlloc(LMEM_FIXED, sizeof(XG_HintEditData)));
        data->m_fTate = true;
        data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(XgHintEdit_WndProc)));
        ::SetWindowLongPtr(hwndCtrl, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(data));
        xg_ahwndTateEdits.emplace_back(hwndCtrl);
    }
    for (const auto& hint : xg_vecYokoHints) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_ACROSSNUMBER), hint.m_number);
        hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz,
            WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        assert(hwndCtrl);
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(xg_hHintsUIFont),
            TRUE);
        if (hwndCtrl == NULL)
            return FALSE;

        xg_ahwndYokoStatics.emplace_back(hwndCtrl);

        hwndCtrl = ::CreateWindowExW(
            WS_EX_CLIENTEDGE,
            TEXT("EDIT"), hint.m_strHint.data(),
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        assert(hwndCtrl);
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(xg_hHintsUIFont),
            TRUE);
        if (hwndCtrl == NULL)
            return FALSE;

        data = reinterpret_cast<XG_HintEditData *>(
            ::LocalAlloc(LMEM_FIXED, sizeof(XG_HintEditData)));
        data->m_fTate = false;
        data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(XgHintEdit_WndProc)));
        ::SetWindowLongPtr(hwndCtrl, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(data));
        xg_ahwndYokoEdits.emplace_back(hwndCtrl);
    }

    if (xg_ahwndTateEdits.size())
        ::SetFocus(xg_ahwndTateEdits[0]);

    ::PostMessageW(hwnd, WM_SIZE, 0, 0);

    return TRUE;
}

// ヒントウィンドウが縦にスクロールされた。
void HintsWnd_OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
{
    xg_svHintsScrollView.Scroll(SB_VERT, code, pos);
}

// ヒントが変更されたか？
bool __fastcall XgAreHintsModified(void)
{
    if (xg_bHintsAdded) {
        return true;
    }

    if (::IsWindow(xg_hHintsWnd)) {
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            if (::SendMessageW(xg_ahwndTateEdits[i], EM_GETMODIFY, 0, 0)) {
                return true;
            }
        }
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            if (::SendMessageW(xg_ahwndYokoEdits[i], EM_GETMODIFY, 0, 0)) {
                return true;
            }
        }
    }
    return false;
}

// ヒントデータを設定する。
void __fastcall XgSetHintsData(void)
{
    for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
        ::SetWindowTextW(xg_ahwndTateEdits[i], xg_vecTateHints[i].m_strHint.data());
    }
    for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
        ::SetWindowTextW(xg_ahwndYokoEdits[i], xg_vecYokoHints[i].m_strHint.data());
    }
}

// ヒントデータを更新する。
bool __fastcall XgUpdateHintsData(void)
{
    bool updated = false;
    if (::IsWindow(xg_hHintsWnd)) {
        WCHAR sz[512];
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            if (::SendMessageW(xg_ahwndTateEdits[i], EM_GETMODIFY, 0, 0)) {
                updated = true;
                ::GetWindowTextW(xg_ahwndTateEdits[i], sz, 
                                 static_cast<int>(ARRAYSIZE(sz)));
                xg_vecTateHints[i].m_strHint = sz;
                ::SendMessageW(xg_ahwndTateEdits[i], EM_SETMODIFY, FALSE, 0);
            }
        }
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            if (::SendMessageW(xg_ahwndYokoEdits[i], EM_GETMODIFY, 0, 0)) {
                updated = true;
                ::GetWindowTextW(xg_ahwndYokoEdits[i], sz, 
                                 static_cast<int>(ARRAYSIZE(sz)));
                xg_vecYokoHints[i].m_strHint = sz;
                ::SendMessageW(xg_ahwndTateEdits[i], EM_SETMODIFY, FALSE, 0);
            }
        }
    }
    return updated;
}

// ヒントウィンドウが破棄された。
void HintsWnd_OnDestroy(HWND hwnd)
{
    if (xg_hHintsWnd) {
        // ヒントデータを更新する。
        XgUpdateHintsData();
    }

    // 現在の位置とサイズを保存する。
    MRect rc;
    ::GetWindowRect(hwnd, &rc);
    s_nHintsWndX = rc.left;
    s_nHintsWndY = rc.top;
    s_nHintsWndCX = rc.Width();
    s_nHintsWndCY = rc.Height();

    xg_hHintsWnd = NULL;
    xg_hwndTateCaptionStatic = NULL;
    xg_hwndYokoCaptionStatic = NULL;
    xg_ahwndTateStatics.clear();
    xg_ahwndTateEdits.clear();
    xg_ahwndYokoStatics.clear();
    xg_ahwndYokoEdits.clear();
    xg_svHintsScrollView.clear();

    ::DeleteObject(xg_hHintsUIFont);
    xg_hHintsUIFont = NULL;
}

void HintsWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify == STN_CLICKED) {
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            if (xg_ahwndTateStatics[i] == hwndCtl) {
                ::SendMessageW(xg_ahwndTateEdits[i], EM_SETSEL, 0, -1);
                ::SetFocus(xg_ahwndTateEdits[i]);
                return;
            }
        }
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            if (xg_ahwndYokoStatics[i] == hwndCtl) {
                ::SendMessageW(xg_ahwndYokoEdits[i], EM_SETSEL, 0, -1);
                ::SetFocus(xg_ahwndYokoEdits[i]);
                return;
            }
        }
    }
}

// キーが押された。
void HintsWnd_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (!fDown)
        return;

    switch (vk) {
    case VK_PRIOR:
        ::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEUP, 0), 0);
        break;

    case VK_NEXT:
        ::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEDOWN, 0), 0);
        break;

    case VK_TAB:
        if (xg_ahwndTateEdits.size())
            ::SetFocus(xg_ahwndTateEdits[0]);
        break;

    case VK_F6:
        if (::GetAsyncKeyState(VK_SHIFT) < 0)
            ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
        else
            ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
        break;
    }
}


// マウスホイールが回転した。
void __fastcall
HintsWnd_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
    if (::GetAsyncKeyState(VK_SHIFT) < 0) {
        if (zDelta < 0)
            ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
        else if (zDelta > 0)
            ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
    } else {
        if (zDelta < 0)
            ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
        else if (zDelta > 0)
            ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
    }
}

// ヒント ウィンドウのサイズを制限する。
void HintsWnd_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMinTrackSize.x = 256;
    lpMinMaxInfo->ptMinTrackSize.y = 128;
}

// 「ヒント」ウィンドウのコンテキストメニュー。
void HintsWnd_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    WCHAR szClass[8];
    szClass[0] = 0;
    GetClassNameW(hwndContext, szClass, ARRAYSIZE(szClass));
    if (lstrcmpiW(szClass, L"EDIT") == 0)
        return;

    HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(2));
    HMENU hSubMenu = GetSubMenu(hMenu, 1);

    // 右クリックメニューを表示する。
    ::SetForegroundWindow(hwnd);
    INT nCmd = ::TrackPopupMenu(
        hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD,
        xPos, yPos, 0, hwnd, NULL);
    ::PostMessageW(hwnd, WM_NULL, 0, 0);
    if (nCmd)
        ::PostMessageW(xg_hMainWnd, WM_COMMAND, nCmd, 0);

    ::DestroyMenu(hMenu);
}

// ヒント ウィンドウのウィンドウ プロシージャー。
extern "C"
LRESULT CALLBACK
XgHintsWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, HintsWnd_OnCreate);
    HANDLE_MSG(hWnd, WM_SIZE, HintsWnd_OnSize);
    HANDLE_MSG(hWnd, WM_VSCROLL, HintsWnd_OnVScroll);
    HANDLE_MSG(hWnd, WM_KEYDOWN, HintsWnd_OnKey);
    HANDLE_MSG(hWnd, WM_KEYUP, HintsWnd_OnKey);
    HANDLE_MSG(hWnd, WM_DESTROY, HintsWnd_OnDestroy);
    HANDLE_MSG(hWnd, WM_COMMAND, HintsWnd_OnCommand);
    HANDLE_MSG(hWnd, WM_MOUSEWHEEL, HintsWnd_OnMouseWheel);
    HANDLE_MSG(hWnd, WM_GETMINMAXINFO, HintsWnd_OnGetMinMaxInfo);
    HANDLE_MSG(hWnd, WM_CONTEXTMENU, HintsWnd_OnContextMenu);

    default:
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// ヒントウィンドウを作成する。
BOOL XgCreateHintsWnd(HWND hwnd)
{
    const DWORD style = WS_OVERLAPPED | WS_CAPTION |
        WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
    CreateWindowExW(WS_EX_TOOLWINDOW,
        s_pszHintsWndClass, XgLoadStringDx1(IDS_HINTS), style,
        s_nHintsWndX, s_nHintsWndY, s_nHintsWndCX, s_nHintsWndCY,
        hwnd, nullptr, xg_hInstance, nullptr);
    if (xg_hHintsWnd) {
        ShowWindow(xg_hHintsWnd, SW_SHOWNORMAL);
        UpdateWindow(xg_hHintsWnd);
        return TRUE;
    }
    return FALSE;
}

// ヒントウィンドウを破棄する。
void XgDestroyHintsWnd(void)
{
    // ヒントウィンドウが存在するか？
    if (xg_hHintsWnd && ::IsWindow(xg_hHintsWnd)) {
        // 更新を無視・破棄する。
        HWND hwnd = xg_hHintsWnd;
        xg_hHintsWnd = NULL;
        ::DestroyWindow(hwnd);
    }
}

//////////////////////////////////////////////////////////////////////////////

// hook for Ctrl+A
HHOOK xg_hCtrlAHook = NULL;

// hook proc for Ctrl+A
LRESULT CALLBACK XgCtrlAMessageProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return ::CallNextHookEx(xg_hCtrlAHook, nCode, wParam, lParam);

    MSG *pMsg = reinterpret_cast<MSG *>(lParam);
    WCHAR szClassName[64];

    HWND hWnd;
    if (pMsg->message == WM_KEYDOWN) {
        if (static_cast<int>(pMsg->wParam) == 'A' &&
            ::GetAsyncKeyState(VK_CONTROL) < 0 &&
            ::GetAsyncKeyState(VK_SHIFT) >= 0 &&
            ::GetAsyncKeyState(VK_MENU) >= 0)
        {
            // Ctrl+A is pressed
            hWnd = ::GetFocus();
            if (hWnd) {
                ::GetClassNameW(hWnd, szClassName, 64);
                if (::lstrcmpiW(szClassName, L"EDIT") == 0) {
                    ::SendMessageW(hWnd, EM_SETSEL, 0, -1);
                    return 1;
                }
            }
        }
    }

    return ::CallNextHookEx(xg_hCtrlAHook, nCode, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////////
// 候補ウィンドウ。

// 候補ウィンドウのスクロールビュー。
MScrollView                 xg_svCandsScrollView;

// 候補ウィンドウのUIフォント。
HFONT                       xg_hCandsUIFont = NULL;

// 候補。
std::vector<std::wstring>   xg_vecCandidates;

// 候補ボタン。
std::vector<HWND>           xg_ahwndCandButtons;

// 候補を求める位置。
int  xg_jCandPos;
int  xg_iCandPos;

// 候補はタテかヨコか。
bool xg_bCandVertical;

struct XG_CandsButtonData
{
    WNDPROC m_fnOldWndProc;
};

extern "C"
LRESULT CALLBACK
XgCandsButton_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC fn;
    XG_CandsButtonData *data =
        reinterpret_cast<XG_CandsButtonData *>(
            ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    switch (uMsg) {
    case WM_CHAR:
        #if 0
            if (wParam == L'\t' || wParam == L'\r' || wParam == L'\n')
                break;
        #endif
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);

    case WM_SETFOCUS:
        xg_svCandsScrollView.EnsureCtrlVisible(hwnd);
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);

    case WM_KEYDOWN:
        if (wParam == VK_RETURN) {
            ::SetFocus(NULL);
            break;
        }

        if (wParam == VK_PRIOR || wParam == VK_NEXT) {
            HWND hwndParent = ::GetParent(hwnd);
            ::SendMessageW(hwndParent, uMsg, wParam, lParam);
            break;
        }

        if (wParam == VK_F6) {
            if (::GetAsyncKeyState(VK_SHIFT) < 0)
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
            else
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
            break;
        }

        if (wParam == VK_ESCAPE) {
            DestroyWindow(GetParent(hwnd));
            break;
        }

        return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

    case WM_NCDESTROY:
        fn = data->m_fnOldWndProc;
        ::LocalFree(data);
        return ::CallWindowProc(fn, hwnd, uMsg, wParam, lParam);

    case WM_MOUSEWHEEL:
        {
            HWND hwndParent = ::GetParent(hwnd);
            ::SendMessageW(hwndParent, uMsg, wParam, lParam);
        }
        break;

    default:
        return ::CallWindowProc(data->m_fnOldWndProc,
            hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 候補ウィンドウのサイズが変わった。
void CandsWnd_OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/)
{
    if (xg_ahwndCandButtons.empty())
        return;

    xg_svCandsScrollView.clear();

    MRect rcClient;
    ::GetClientRect(hwnd, &rcClient);

    HDC hdc = ::CreateCompatibleDC(NULL);
    HGDIOBJ hFontOld = ::SelectObject(hdc, xg_hCandsUIFont);
    {
        MPoint pt;
        for (size_t i = 0; i < xg_vecCandidates.size(); ++i) {
            const std::wstring& strLabel = xg_vecCandidates[i];

            MSize siz;
            ::GetTextExtentPoint32W(hdc, strLabel.data(),
                                    static_cast<int>(strLabel.size()), &siz);

            if (pt.x != 0 && pt.x + siz.cx + 16 > rcClient.Width()) {
                pt.x = 0;
                pt.y += siz.cy + 16;
            }

            MRect rcCtrl(MPoint(pt.x + 4, pt.y + 4), MSize(siz.cx + 8, siz.cy + 8));
            xg_svCandsScrollView.AddCtrlInfo(xg_ahwndCandButtons[i], rcCtrl);

            pt.x += siz.cx + 16;
        }
    }
    ::SelectObject(hdc, hFontOld);
    ::DeleteDC(hdc);

    xg_svCandsScrollView.SetExtentForAllCtrls();
    xg_svCandsScrollView.Extent().cy += 4;
    xg_svCandsScrollView.EnsureCtrlVisible(::GetFocus(), false);
    xg_svCandsScrollView.UpdateAll();
}

// 候補ウィンドウが作成された。
BOOL CandsWnd_OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
{
    xg_hCandsWnd = hwnd;

    if (xg_hCandsUIFont) {
        ::DeleteObject(xg_hCandsUIFont);
    }
    xg_hCandsUIFont = ::CreateFontIndirectW(XgGetUIFont());
    if (xg_hCandsUIFont == NULL) {
        xg_hCandsUIFont = reinterpret_cast<HFONT>(
            ::GetStockObject(DEFAULT_GUI_FONT));
    }

    // 初期化。
    xg_ahwndCandButtons.clear();
    xg_svCandsScrollView.clear();

    xg_svCandsScrollView.SetParent(hwnd);
    xg_svCandsScrollView.ShowScrollBars(FALSE, TRUE);

    HWND hwndCtrl;
    XG_CandsButtonData *data;
    for (size_t i = 0; i < xg_vecCandidates.size(); ++i) {
        WCHAR szText[64];
        if (xg_bHiragana) {
            LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                         xg_vecCandidates[i].data(), -1, szText, ARRAYSIZE(szText));
            xg_vecCandidates[i] = szText;
        }
        if (xg_bLowercase) {
            LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE,
                         xg_vecCandidates[i].data(), -1, szText, ARRAYSIZE(szText));
            xg_vecCandidates[i] = szText;
        }

        hwndCtrl = ::CreateWindowW(
            TEXT("BUTTON"), xg_vecCandidates[i].data(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        assert(hwndCtrl);
        if (hwndCtrl == NULL)
            return FALSE;

        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(xg_hCandsUIFont),
            FALSE);
        xg_ahwndCandButtons.emplace_back(hwndCtrl);

        data = reinterpret_cast<XG_CandsButtonData *>(
            ::LocalAlloc(LMEM_FIXED, sizeof(XG_CandsButtonData)));
        data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
            ::SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                reinterpret_cast<LONG_PTR>(XgCandsButton_WndProc)));
        ::SetWindowLongPtr(hwndCtrl, GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(data));
    }
    CandsWnd_OnSize(hwnd, 0, 0, 0);

    if (xg_ahwndCandButtons.size())
        ::SetFocus(xg_ahwndCandButtons[0]);
    else
        ::SetFocus(hwnd);

    return TRUE;
}

// 候補ウィンドウが縦にスクロールされた。
void CandsWnd_OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
{
    xg_svCandsScrollView.Scroll(SB_VERT, code, pos);
}

// キーが押された。
void CandsWnd_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (!fDown)
        return;

    switch (vk) {
    case VK_PRIOR:
        ::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEUP, 0), 0);
        break;

    case VK_NEXT:
        ::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEDOWN, 0), 0);
        break;

    case VK_TAB:
        if (xg_ahwndCandButtons.size())
            ::SetFocus(xg_ahwndCandButtons[0]);
        break;

    case VK_F6:
        if (::GetAsyncKeyState(VK_SHIFT) < 0)
            ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
        else
            ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
        break;

    case VK_ESCAPE:
        DestroyWindow(hwnd);
        break;
    }
}

// 候補ウィンドウが破棄された。
void CandsWnd_OnDestroy(HWND hwnd)
{
    // 現在の位置とサイズを保存する。
    MRect rc;
    ::GetWindowRect(hwnd, &rc);
    s_nCandsWndX = rc.left;
    s_nCandsWndY = rc.top;
    s_nCandsWndCX = rc.Width();
    s_nCandsWndCY = rc.Height();

    xg_hCandsWnd = NULL;
    xg_ahwndCandButtons.clear();
    xg_svCandsScrollView.clear();

    ::DeleteObject(xg_hCandsUIFont);
    xg_hCandsUIFont = NULL;

    SetForegroundWindow(xg_hMainWnd);
}

// 候補ウィンドウを破棄する。
void XgDestroyCandsWnd(void)
{
    // 候補ウィンドウが存在するか？
    if (xg_hCandsWnd && ::IsWindow(xg_hCandsWnd)) {
        // 更新を無視・破棄する。
        HWND hwnd = xg_hCandsWnd;
        xg_hCandsWnd = NULL;
        ::DestroyWindow(hwnd);
    }
}

// 候補を適用する。
void XgApplyCandidate(XG_Board& xword, const std::wstring& strCand)
{
    std::wstring cand = XgNormalizeString(strCand);

    int lo, hi;
    if (xg_bCandVertical) {
        for (lo = xg_iCandPos; lo > 0; --lo) {
            if (xword.GetAt(lo - 1, xg_jCandPos) == ZEN_BLACK)
                break;
        }
        for (hi = xg_iCandPos; hi + 1 < xg_nRows; ++hi) {
            if (xword.GetAt(hi + 1, xg_jCandPos) == ZEN_BLACK)
                break;
        }

        int m = 0;
        for (int k = lo; k <= hi; ++k, ++m) {
            xword.SetAt(k, xg_jCandPos, cand[m]);
        }
    }
    else
    {
        for (lo = xg_jCandPos; lo > 0; --lo) {
            if (xword.GetAt(xg_iCandPos, lo - 1) == ZEN_BLACK)
                break;
        }
        for (hi = xg_jCandPos; hi + 1 < xg_nCols; ++hi) {
            if (xword.GetAt(xg_iCandPos, hi + 1) == ZEN_BLACK)
                break;
        }

        int m = 0;
        for (int k = lo; k <= hi; ++k, ++m) {
            xword.SetAt(xg_iCandPos, k, cand[m]);
        }
    }
}

void CandsWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    if (codeNotify == BN_CLICKED) {
        for (size_t i = 0; i < xg_ahwndCandButtons.size(); ++i) {
            if (xg_ahwndCandButtons[i] == hwndCtl)
            {
                // 候補を適用する。
                XgApplyCandidate(xg_xword, xg_vecCandidates[i]);

                // 候補ウィンドウを破棄する。
                XgDestroyCandsWnd();

                // イメージを更新する。
                int x = XgGetHScrollPos();
                int y = XgGetVScrollPos();
                XgUpdateImage(xg_hMainWnd, x, y);
                return;
            }
        }
    }
}

// マウスホイールが回転した。
void __fastcall
CandsWnd_OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
{
    if (::GetAsyncKeyState(VK_SHIFT) < 0) {
        if (zDelta < 0)
            ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
        else if (zDelta > 0)
            ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
    } else {
        if (zDelta < 0)
            ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
        else if (zDelta > 0)
            ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
    }
}

// 候補ウィンドウのサイズを制限する。
void CandsWnd_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMinTrackSize.x = 256;
    lpMinMaxInfo->ptMinTrackSize.y = 128;
}

// 候補ウィンドウを描画する。
void CandsWnd_OnPaint(HWND hwnd)
{
    if (xg_vecCandidates.empty()) {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);
        if (hdc) {
            MRect rcClient;
            ::GetClientRect(hwnd, &rcClient);
            ::SetTextColor(hdc, RGB(0, 0, 0));
            ::SetBkMode(hdc, TRANSPARENT);
            ::DrawTextW(hdc, XgLoadStringDx1(IDS_NOCANDIDATES), -1,
                &rcClient,
                DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
            ::EndPaint(hwnd, &ps);
        }
    } else {
        FORWARD_WM_PAINT(hwnd, DefWindowProcW);
    }
}

extern "C"
LRESULT CALLBACK
XgCandsWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    HANDLE_MSG(hwnd, WM_CREATE, CandsWnd_OnCreate);
    HANDLE_MSG(hwnd, WM_SIZE, CandsWnd_OnSize);
    HANDLE_MSG(hwnd, WM_VSCROLL, CandsWnd_OnVScroll);
    HANDLE_MSG(hwnd, WM_KEYDOWN, CandsWnd_OnKey);
    HANDLE_MSG(hwnd, WM_KEYUP, CandsWnd_OnKey);
    HANDLE_MSG(hwnd, WM_DESTROY, CandsWnd_OnDestroy);
    HANDLE_MSG(hwnd, WM_COMMAND, CandsWnd_OnCommand);
    HANDLE_MSG(hwnd, WM_MOUSEWHEEL, CandsWnd_OnMouseWheel);
    HANDLE_MSG(hwnd, WM_GETMINMAXINFO, CandsWnd_OnGetMinMaxInfo);
    HANDLE_MSG(hwnd, WM_PAINT, CandsWnd_OnPaint);

    default:
        return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

// 候補ウィンドウを作成する。
BOOL XgCreateCandsWnd(HWND hwnd)
{
    const DWORD style = WS_OVERLAPPED | WS_CAPTION |
        WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
    ::CreateWindowExW(WS_EX_TOOLWINDOW,
        s_pszCandsWndClass, XgLoadStringDx1(IDS_CANDIDATES), style,
        s_nCandsWndX, s_nCandsWndY, s_nCandsWndCX, s_nCandsWndCY,
        hwnd, nullptr, xg_hInstance, nullptr);
    if (xg_hCandsWnd) {
        return TRUE;
    }

    return FALSE;
}

// 候補の内容を候補ウィンドウで開く。
bool __fastcall XgOpenCandsWnd(HWND hwnd, bool vertical)
{
    // もし候補ウィンドウが存在すれば破棄する。
    if (xg_hCandsWnd) {
        HWND hwnd = xg_hCandsWnd;
        xg_hCandsWnd = NULL;
        DestroyWindow(hwnd);
    }

    // 候補を作成する。
    xg_iCandPos = xg_caret_pos.m_i;
    xg_jCandPos = xg_caret_pos.m_j;
    xg_bCandVertical = vertical;
    if (xg_xword.GetAt(xg_iCandPos, xg_jCandPos) == ZEN_BLACK) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

    // パターンを取得する。
    int lo, hi;
    std::wstring pattern;
    bool left_black, right_black;
    if (xg_bCandVertical) {
        lo = hi = xg_iCandPos;
        while (lo > 0) {
            if (xg_xword.GetAt(lo - 1, xg_jCandPos) != ZEN_BLACK)
                --lo;
            else
                break;
        }
        while (hi + 1 < xg_nRows) {
            if (xg_xword.GetAt(hi + 1, xg_jCandPos) != ZEN_BLACK)
                ++hi;
            else
                break;
        }

        for (int i = lo; i <= hi; ++i) {
            pattern += xg_xword.GetAt(i, xg_jCandPos);
        }

        right_black = (hi + 1 != xg_nRows);
    } else {
        lo = hi = xg_jCandPos;
        while (lo > 0) {
            if (xg_xword.GetAt(xg_iCandPos, lo - 1) != ZEN_BLACK)
                --lo;
            else
                break;
        }
        while (hi + 1 < xg_nCols) {
            if (xg_xword.GetAt(xg_iCandPos, hi + 1) != ZEN_BLACK)
                ++hi;
            else
                break;
        }

        for (int j = lo; j <= hi; ++j) {
            pattern += xg_xword.GetAt(xg_iCandPos, j);
        }

        right_black = (hi + 1 != xg_nCols);
    }
    left_black = (lo != 0);

    // 候補を取得する。
    int nSkip = 0;
    std::vector<std::wstring> cands, cands2;
    XgGetCandidatesAddBlack<false>(cands, pattern, nSkip, left_black, right_black);
    XgGetCandidatesAddBlack<true>(cands2, pattern, nSkip, left_black, right_black);
    cands.insert(cands.end(), cands2.begin(), cands2.end());

    // 仮に適用して、正当かどうか確かめ、正当なものだけを
    // 最終的な候補とする。
    xg_vecCandidates.clear();
    for (const auto& cand : cands) {
        XG_Board xword(xg_xword);
        XgApplyCandidate(xword, cand);
        if (xword.CornerBlack() || xword.DoubleBlack() ||
            xword.TriBlackAround() || xword.DividedByBlack())
        {
            ;
        } else {
            xg_vecCandidates.emplace_back(cand);
        }
    }

    // 個数制限。
    if (xg_vecCandidates.size() > xg_nMaxCandidates)
        xg_vecCandidates.resize(xg_nMaxCandidates);

    if (xg_vecCandidates.empty()) {
        if (XgCheckCrossWord(hwnd, false)) {
            ::MessageBeep(0xFFFFFFFF);
        } else {
            return false;
        }
    }

    // ヒントウィンドウを作成する。
    if (XgCreateCandsWnd(xg_hMainWnd)) {
        ::ShowWindow(xg_hCandsWnd, SW_SHOWNORMAL);
        ::UpdateWindow(xg_hCandsWnd);
        return true;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////

// Windowsアプリのメイン関数。
extern "C"
int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE /*hPrevInstance*/,
    LPSTR /*pszCmdLine*/,
    int nCmdShow)
{
    // アプリのインスタンスを保存する。
    xg_hInstance = hInstance;

    // 設定を読み込む。
    XgLoadSettings();

    // 辞書ファイルの名前を読み込む。
    XgLoadDictsAll();

    // 乱数モジュールを初期化する。
    srand(::GetTickCount() ^ ::GetCurrentThreadId());

    // プロセッサの数を取得する。
    SYSTEM_INFO si;
    ::GetSystemInfo(&si);
    s_dwNumberOfProcessors = si.dwNumberOfProcessors;

    // プロセッサの数に合わせてスレッドの数を決める。
    if (s_dwNumberOfProcessors <= 3)
        xg_dwThreadCount = 2;
    else
        xg_dwThreadCount = s_dwNumberOfProcessors - 1;

    xg_aThreadInfo.resize(xg_dwThreadCount);
    xg_ahThreads.resize(xg_dwThreadCount);

    // コモン コントロールを初期化する。
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof(iccx);
    iccx.dwICC = ICC_USEREX_CLASSES | ICC_PROGRESS_CLASS;
    ::InitCommonControlsEx(&iccx);

    // アクセラレータを読み込む。
    s_hAccel = ::LoadAcceleratorsW(hInstance, MAKEINTRESOURCE(1));
    if (s_hAccel == nullptr) {
        // アクセラレータ作成失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTACCEL), nullptr, MB_ICONERROR);
        return 3;
    }

    // ウィンドウクラスを登録する。
    WNDCLASSEXW wcx;
    wcx.cbSize = sizeof(wcx);
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = XgWindowProc;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(1));
    wcx.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(INT_PTR(COLOR_3DFACE + 1));
    wcx.lpszMenuName = MAKEINTRESOURCE(1);
    wcx.lpszClassName = s_pszMainWndClass;
    wcx.hIconSm = nullptr;
    if (!::RegisterClassExW(&wcx)) {
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
        return 1;
    }
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = XgHintsWndProc;
    wcx.hIcon = NULL;
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_3DFACE + 1));
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = s_pszHintsWndClass;
    if (!::RegisterClassExW(&wcx)) {
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
        return 1;
    }
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = XgCandsWndProc;
    wcx.hIcon = NULL;
    wcx.hbrBackground = ::CreateSolidBrush(RGB(255, 255, 192));
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = s_pszCandsWndClass;
    if (!::RegisterClassExW(&wcx)) {
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
        return 1;
    }

    // クリティカルセクションを初期化する。
    ::InitializeCriticalSection(&xg_cs);

    // メインウィンドウを作成する。
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS;
    ::CreateWindowW(s_pszMainWndClass, XgLoadStringDx1(IDS_APPINFO), style,
        s_nMainWndX, s_nMainWndY, s_nMainWndCX, s_nMainWndCY,
        nullptr, nullptr, hInstance, nullptr);
    if (xg_hMainWnd == nullptr) {
        // ウィンドウ作成失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTMAKEWND), nullptr, MB_ICONERROR);
        return 2;
    }

    // ウィンドウを表示する。
    ::ShowWindow(xg_hMainWnd, nCmdShow);
    ::UpdateWindow(xg_hMainWnd);

    // Ctrl+Aの機能を有効にする。
    xg_hCtrlAHook = ::SetWindowsHookEx(WH_MSGFILTER,
        XgCtrlAMessageProc, NULL, ::GetCurrentThreadId());

    // メッセージループ。
    MSG msg;
    while (::GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F6) {
            if (GetAsyncKeyState(VK_SHIFT) < 0)
                PostMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
            else
                PostMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
            continue;
        }

        if (xg_hHintsWnd && GetParent(msg.hwnd) == xg_hHintsWnd && 
            msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) 
        {
            msg.wParam = VK_TAB;
        }

        if (msg.message == WM_KEYDOWN &&
            msg.wParam == L'L' && GetAsyncKeyState(VK_CONTROL) < 0)
        {
            msg.hwnd = xg_hMainWnd;
        }

        if (msg.message == WM_KEYDOWN &&
            msg.wParam == L'U' && GetAsyncKeyState(VK_CONTROL) < 0)
        {
            msg.hwnd = xg_hMainWnd;
        }

        if (xg_hHintsWnd && ::IsDialogMessageW(xg_hHintsWnd, &msg))
            continue;

        if (xg_hCandsWnd) {
            if (msg.message != WM_KEYDOWN || msg.wParam != VK_ESCAPE) {
                if (::IsDialogMessageW(xg_hCandsWnd, &msg))
                    continue;
            }
        }

        if (xg_hwndInputPalette) {
            if (::IsDialogMessageW(xg_hwndInputPalette, &msg))
                continue;
        }

        if (!::TranslateAcceleratorW(xg_hMainWnd, s_hAccel, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    // Ctrl+Aの機能を解除する。
    ::UnhookWindowsHookEx(xg_hCtrlAHook);
    xg_hCtrlAHook = NULL;

    // クリティカルセクションを破棄する。
    ::DeleteCriticalSection(&xg_cs);

    // 設定を保存。
    XgSaveSettings();

    return static_cast<int>(msg.wParam);
}

//////////////////////////////////////////////////////////////////////////////
