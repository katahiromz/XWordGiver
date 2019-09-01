//////////////////////////////////////////////////////////////////////////////
// GUI.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2019 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"

// クロスワードのサイズの制限。
#define xg_nMinSize         3
#define xg_nMaxSize         30

#ifndef WM_MOUSEHWHEEL
    #define WM_MOUSEHWHEEL 0x020E
#endif

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

// 辞書ファイルの場所（パス）のリスト。
std::deque<std::wstring>  xg_dict_files;

// ヒントに追加があったか？
bool            xg_bHintsAdded = false;

// JSONファイルとして保存するか？
bool            xg_bSaveAsJsonFile = false;

// 太枠をつけるか？
bool            xg_bAddThickFrame = true;

// マスのフォント。
std::array<WCHAR,LF_FACESIZE>        xg_szCellFont = { { 0 } };

// 小さな文字のフォント。
std::array<WCHAR,LF_FACESIZE>        xg_szSmallFont = { { 0 } };

// UIフォント。
std::array<WCHAR,LF_FACESIZE>        xg_szUIFont = { { 0 } };

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

//////////////////////////////////////////////////////////////////////////////
// static variables

// クロスワードのサイズの設定。
static int s_nRows = 5, s_nCols = 5;

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

INT xg_nInputPaletteWndX = CW_USEDEFAULT;
INT xg_nInputPaletteWndY = CW_USEDEFAULT;

// ウィンドウ設定の名前。
static const LPCWSTR s_pszMainWndX = L"WindowX";
static const LPCWSTR s_pszMainWndY = L"WindowY";
static const LPCWSTR s_pszMainWndCX = L"WindowCX";
static const LPCWSTR s_pszMainWndCY = L"WindowCY";
static const LPCWSTR s_pszHintsWndX = L"HintsX";
static const LPCWSTR s_pszHintsWndY = L"HintsY";
static const LPCWSTR s_pszHintsWndCX = L"HintsCX";
static const LPCWSTR s_pszHintsWndCY = L"HintsCY";
static const LPCWSTR s_pszCandsWndX = L"CandsX";
static const LPCWSTR s_pszCandsWndY = L"CandsY";
static const LPCWSTR s_pszCandsWndCX = L"CandsCX";
static const LPCWSTR s_pszCandsWndCY = L"CandsCY";
static const LPCWSTR s_pszInputPaletteWndX = L"IPaletteX";
static const LPCWSTR s_pszInputPaletteWndY = L"IPaletteY";

// 会社名。
static const LPCWSTR
    s_pszSoftwareCompanyName = L"Software\\Katayama Hirofumi MZ";

// アプリ名。
static const LPCWSTR s_pszAppName = L"XWord";

// 会社名とアプリ名。
static const LPCWSTR
    s_pszSoftwareCompanyAndApp = L"Software\\Katayama Hirofumi MZ\\XWord";

// レジストリ項目。
static const LPCWSTR s_pszAutoRetry = L"AutoRetry";
static const LPCWSTR s_pszRows = L"Rows";
static const LPCWSTR s_pszCols = L"Cols";
static const LPCWSTR s_pszRecentCount = L"RecentCount";
static const LPCWSTR s_pszRecent = L"Recent %d";
static const LPCWSTR s_pszOldNotice = L"OldNotice";
static const LPCWSTR s_pszSaveToCount = L"SaveToCount";
static const LPCWSTR s_pszSaveTo = L"SaveTo %d";
static const LPCWSTR s_pszInfinite = L"Infinite";
//static const LPCWSTR s_pszDictSaveMode = L"DictSaveMode";
static const LPCWSTR s_pszCellFont = L"CellFont";
static const LPCWSTR s_pszSmallFont = L"SmallFont";
static const LPCWSTR s_pszUIFont = L"UIFont";
static const LPCWSTR s_pszShowToolBar = L"ShowToolBar";
static const LPCWSTR s_pszShowStatusBar = L"ShowStatusBar";
static const LPCWSTR s_pszShowInputPalette = L"ShowInputPalette";
static const LPCWSTR s_pszSaveAsJsonFile = L"SaveAsJsonFile";
static const LPCWSTR s_pszNumberToGenerate = L"NumberToGenerate";
static const LPCWSTR s_pszImageCopyWidth = L"ImageCopyWidth";
static const LPCWSTR s_pszImageCopyHeight = L"ImageCopyHeight";
static const LPCWSTR s_pszImageCopyByHeight = L"ImageCopyByHeight";
static const LPCWSTR s_pszMarksHeight = L"MarksHeight";
static const LPCWSTR s_pszAddThickFrame = L"AddThickFrame";
static const LPCWSTR s_pszTateOki = L"TateOki";
static const LPCWSTR s_pszWhiteCellColor = L"WhiteCellColor";
static const LPCWSTR s_pszBlackCellColor = L"BlackCellColor";
static const LPCWSTR s_pszMarkedCellColor = L"MarkedCellColor";
static const LPCWSTR s_pszDrawFrameForMarkedCell = L"DrawFrameForMarkedCell";
static const LPCWSTR s_pszCharFeed = L"CharFeed";
static const LPCWSTR s_pszTateInput = L"TateInput";
static const LPCWSTR s_pszSmartResolution = L"SmartResolution";
static const LPCWSTR s_pszInputMode = L"InputMode";
static const LPCWSTR s_pszZoomRate = L"ZoomRate";

// 連続生成の場合、無限に生成するか？
static bool s_bInfinite = true;

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

// 多重起動禁止ミューテックス。
static HANDLE       s_hMutex = NULL;

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
static int          s_nNumberToGenerate = 100;

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

// 設定を読み込む。
bool __fastcall XgLoadSettings(void)
{
    int i, nFileCount = 0, nDirCount = 0;
    std::array<WCHAR,MAX_PATH>  sz;
    std::array<WCHAR,32>        szFormat;
    DWORD dwValue;

    // 初期化する。
    s_nMainWndX = CW_USEDEFAULT;
    s_nMainWndY = CW_USEDEFAULT;
    s_nMainWndCX = 475;
    s_nMainWndCY = 490;

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
    xg_dict_files.clear();
    s_dirs_save_to.clear();
    s_bAutoRetry = true;
    s_nRows = s_nCols = 5;
    s_bInfinite = true;
    xg_szCellFont[0] = 0;
    xg_szSmallFont[0] = 0;
    xg_szUIFont[0] = 0;
    s_nDictSaveMode = 2;
    s_bShowToolBar = true;
    s_bShowStatusBar = true;
    xg_bShowInputPalette = false;
    xg_bSaveAsJsonFile = false;
    s_nImageCopyWidth = 250;
    s_nImageCopyHeight = 250;
    s_bImageCopyByHeight = false;
    s_nMarksHeight = 40;
    xg_bAddThickFrame = true;
    xg_bTateOki = true;
    xg_bCharFeed = false;
    xg_rgbWhiteCellColor = RGB(255, 255, 255);
    xg_rgbBlackCellColor = RGB(0x33, 0x33, 0x33);
    xg_rgbMarkedCellColor = RGB(255, 255, 255);
    xg_bDrawFrameForMarkedCell = TRUE;
    xg_bSmartResolution = TRUE;
    xg_imode = xg_im_KANA;
    xg_nZoomRate = 100;

    // 会社名キーを開く。
    MRegKey company_key(HKEY_CURRENT_USER, s_pszSoftwareCompanyName, FALSE);
    if (company_key) {
        // アプリ名キーを開く。
        MRegKey app_key(company_key, s_pszAppName, FALSE);
        if (app_key) {
            if (!app_key.QueryDword(s_pszMainWndX, dwValue)) {
                s_nMainWndX = dwValue;
            }
            if (!app_key.QueryDword(s_pszMainWndY, dwValue)) {
                s_nMainWndY = dwValue;
            }
            if (!app_key.QueryDword(s_pszMainWndCX, dwValue)) {
                s_nMainWndCX = dwValue;
            }
            if (!app_key.QueryDword(s_pszMainWndCY, dwValue)) {
                s_nMainWndCY = dwValue;
            }

            if (!app_key.QueryDword(s_pszTateInput, dwValue)) {
                xg_bTateInput = !!dwValue;
            }

            if (!app_key.QueryDword(s_pszHintsWndX, dwValue)) {
                s_nHintsWndX = dwValue;
            }
            if (!app_key.QueryDword(s_pszHintsWndY, dwValue)) {
                s_nHintsWndY = dwValue;
            }
            if (!app_key.QueryDword(s_pszHintsWndCX, dwValue)) {
                s_nHintsWndCX = dwValue;
            }
            if (!app_key.QueryDword(s_pszHintsWndCY, dwValue)) {
                s_nHintsWndCY = dwValue;
            }

            if (!app_key.QueryDword(s_pszCandsWndX, dwValue)) {
                s_nCandsWndX = dwValue;
            }
            if (!app_key.QueryDword(s_pszCandsWndY, dwValue)) {
                s_nCandsWndY = dwValue;
            }
            if (!app_key.QueryDword(s_pszCandsWndCX, dwValue)) {
                s_nCandsWndCX = dwValue;
            }
            if (!app_key.QueryDword(s_pszCandsWndCY, dwValue)) {
                s_nCandsWndCY = dwValue;
            }

            if (!app_key.QueryDword(s_pszInputPaletteWndX, dwValue)) {
                xg_nInputPaletteWndX = dwValue;
            }
            if (!app_key.QueryDword(s_pszInputPaletteWndY, dwValue)) {
                xg_nInputPaletteWndY = dwValue;
            }

            if (!app_key.QueryDword(s_pszOldNotice, dwValue)) {
                s_bOldNotice = !!dwValue;
            }
            if (!app_key.QueryDword(s_pszAutoRetry, dwValue)) {
                s_bAutoRetry = !!dwValue;
            }
            if (!app_key.QueryDword(s_pszRows, dwValue)) {
                s_nRows = dwValue;
            }
            if (!app_key.QueryDword(s_pszCols, dwValue)) {
                s_nCols = dwValue;
            }

            if (!app_key.QueryDword(s_pszInfinite, dwValue)) {
                s_bInfinite = !!dwValue;
            }

            //if (!app_key.QueryDword(s_pszDictSaveMode, dwValue)) {
            //    s_nDictSaveMode = !!dwValue;
            //}

            if (!app_key.QuerySz(s_pszCellFont, sz.data(), sz.size())) {
                ::lstrcpynW(xg_szCellFont.data(), sz.data(), int(xg_szCellFont.size()));
            }
            if (!app_key.QuerySz(s_pszSmallFont, sz.data(), sz.size())) {
                ::lstrcpynW(xg_szSmallFont.data(), sz.data(), int(xg_szSmallFont.size()));
            }
            if (!app_key.QuerySz(s_pszUIFont, sz.data(), sz.size())) {
                ::lstrcpynW(xg_szUIFont.data(), sz.data(), int(xg_szUIFont.size()));
            }

            if (!app_key.QueryDword(s_pszShowToolBar, dwValue)) {
                s_bShowToolBar = !!dwValue;
            }
            if (!app_key.QueryDword(s_pszShowStatusBar, dwValue)) {
                s_bShowStatusBar = !!dwValue;
            }
            if (!app_key.QueryDword(s_pszShowInputPalette, dwValue)) {
                xg_bShowInputPalette = !!dwValue;
            }

            if (!app_key.QueryDword(s_pszSaveAsJsonFile, dwValue)) {
                xg_bSaveAsJsonFile = !!dwValue;
            }
            if (!app_key.QueryDword(s_pszNumberToGenerate, dwValue)) {
                s_nNumberToGenerate = dwValue;
            }
            if (!app_key.QueryDword(s_pszImageCopyWidth, dwValue)) {
                s_nImageCopyWidth = dwValue;
            }
            if (!app_key.QueryDword(s_pszImageCopyHeight, dwValue)) {
                s_nImageCopyHeight = dwValue;
            }
            if (!app_key.QueryDword(s_pszImageCopyByHeight, dwValue)) {
                s_bImageCopyByHeight = dwValue;
            }
            if (!app_key.QueryDword(s_pszMarksHeight, dwValue)) {
                s_nMarksHeight = dwValue;
            }
            if (!app_key.QueryDword(s_pszAddThickFrame, dwValue)) {
                xg_bAddThickFrame = !!dwValue;
            }

            if (!app_key.QueryDword(s_pszCharFeed, dwValue)) {
                xg_bCharFeed = !!dwValue;
            }

            if (!app_key.QueryDword(s_pszTateOki, dwValue)) {
                xg_bTateOki = !!dwValue;
            }
            if (!app_key.QueryDword(s_pszWhiteCellColor, dwValue)) {
                xg_rgbWhiteCellColor = dwValue;
            }
            if (!app_key.QueryDword(s_pszBlackCellColor, dwValue)) {
                xg_rgbBlackCellColor = dwValue;
            }
            if (!app_key.QueryDword(s_pszMarkedCellColor, dwValue)) {
                xg_rgbMarkedCellColor = dwValue;
            }
            if (!app_key.QueryDword(s_pszDrawFrameForMarkedCell, dwValue)) {
                xg_bDrawFrameForMarkedCell = dwValue;
            }
            if (!app_key.QueryDword(s_pszSmartResolution, dwValue)) {
                xg_bSmartResolution = dwValue;
            }
            if (!app_key.QueryDword(s_pszInputMode, dwValue)) {
                xg_imode = (XG_InputMode)dwValue;
            }
            if (!app_key.QueryDword(s_pszZoomRate, dwValue)) {
                xg_nZoomRate = dwValue;
            }

            // 辞書ファイルのリストを取得する。
            if (!app_key.QueryDword(s_pszRecentCount, dwValue)) {
                nFileCount = dwValue;
                for (i = 0; i < nFileCount; i++) {
                    ::wsprintfW(szFormat.data(), s_pszRecent, i + 1);
                    if (!app_key.QuerySz(szFormat.data(), sz.data(), sz.size())) {
                        xg_dict_files.emplace_back(sz.data());
                    } else {
                        nFileCount = i;
                        break;
                    }
                }
            }

            // 保存先のリストを取得する。
            if (!app_key.QueryDword(s_pszSaveToCount, dwValue)) {
                nDirCount = dwValue;
                for (i = 0; i < nDirCount; i++) {
                    ::wsprintfW(szFormat.data(), s_pszSaveTo, i + 1);
                    if (!app_key.QuerySz(szFormat.data(), sz.data(), sz.size())) {
                        s_dirs_save_to.emplace_back(sz.data());
                    } else {
                        nDirCount = i;
                        break;
                    }
                }
            }
        }
    }

    // ファイルが実際に存在するかチェックし、存在しない項目は消す。
    for (size_t i = 0; i < xg_dict_files.size(); ++i) {
        DWORD attrs = ::GetFileAttributesW(xg_dict_files[i].data());
        if (attrs == 0xFFFFFFFF) {
            xg_dict_files.erase(xg_dict_files.begin() + i);
            --i;
        }
    }

    // 辞書ファイルの個数がゼロならば、初期化する。
    if (nFileCount == 0 || xg_dict_files.empty()) {
        xg_dict_files.clear();

        // 実行ファイルのパスを取得。
        ::GetModuleFileNameW(nullptr, sz.data(), 
                             static_cast<DWORD>(sz.size()));

        // 実行ファイルにある.dicファイルを列挙する。
        HANDLE hFind;
        WIN32_FIND_DATAW find;
        LPWSTR pch;
        pch = wcsrchr(sz.data(), L'\\');
        wcscpy(pch, L"\\*.dic");
        pch++;
        hFind = ::FindFirstFileW(sz.data(), &find);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                ::lstrcpyW(pch, find.cFileName);
                // ファイルが存在するか確認する。
                DWORD attrs = ::GetFileAttributesW(sz.data());
                if (attrs != 0xFFFFFFFF && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    // あった。
                    xg_dict_files.emplace_back(sz.data());
                }
            } while (::FindNextFileW(hFind, &find));
            ::FindClose(hFind);
        }

        if (xg_dict_files.empty()) {
            // 空の場合は一つ上のフォルダで試す。
            pch = wcsrchr(sz.data(), L'\\');
            *pch = 0;
            pch = wcsrchr(sz.data(), L'\\');
            if (pch) {
                wcscpy(pch, L"\\*.dic");
                pch++;
                hFind = ::FindFirstFileW(sz.data(), &find);
                if (hFind != INVALID_HANDLE_VALUE) {
                    do {
                        ::lstrcpyW(pch, find.cFileName);
                        // ファイルが存在するか確認する。
                        DWORD attrs = ::GetFileAttributesW(sz.data());
                        if (attrs != 0xFFFFFFFF && !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                            // あった。
                            xg_dict_files.emplace_back(sz.data());
                        }
                    } while (::FindNextFileW(hFind, &find));
                    ::FindClose(hFind);
                }
            }
        }
    }

    // 保存先リストが空だったら、初期化する。
    if (nDirCount == 0 || s_dirs_save_to.empty()) {
        LPITEMIDLIST pidl;
        std::array<WCHAR,MAX_PATH> szPath;
        ::SHGetSpecialFolderLocation(nullptr, CSIDL_PERSONAL, &pidl);
        ::SHGetPathFromIDListW(pidl, szPath.data());
        ::CoTaskMemFree(pidl);
        s_dirs_save_to.emplace_back(szPath.data());
    }

    return true;
}

// 設定を保存する。
bool __fastcall XgSaveSettings(void)
{
    int i, nCount;
    std::array<WCHAR,32> szFormat;

    // 会社名キーを開く。キーがなければ作成する。
    MRegKey company_key(HKEY_CURRENT_USER, s_pszSoftwareCompanyName, TRUE);
    if (company_key) {
        // アプリ名キーを開く。キーがなければ作成する。
        MRegKey app_key(company_key, s_pszAppName, TRUE);
        if (app_key) {
            app_key.SetDword(s_pszOldNotice, s_bOldNotice);
            app_key.SetDword(s_pszAutoRetry, s_bAutoRetry);
            app_key.SetDword(s_pszRows, s_nRows);
            app_key.SetDword(s_pszCols, s_nCols);
            app_key.SetDword(s_pszInfinite, s_bInfinite);
            //app_key.SetDword(s_pszDictSaveMode, s_nDictSaveMode);

            app_key.SetSz(s_pszCellFont, xg_szCellFont.data(), xg_szCellFont.size());
            app_key.SetSz(s_pszSmallFont, xg_szSmallFont.data(), xg_szSmallFont.size());
            app_key.SetSz(s_pszUIFont, xg_szUIFont.data(), xg_szUIFont.size());

            app_key.SetDword(s_pszShowToolBar, s_bShowToolBar);
            app_key.SetDword(s_pszShowStatusBar, s_bShowStatusBar);
            app_key.SetDword(s_pszShowInputPalette, xg_bShowInputPalette);

            app_key.SetDword(s_pszSaveAsJsonFile, xg_bSaveAsJsonFile);
            app_key.SetDword(s_pszNumberToGenerate, s_nNumberToGenerate);
            app_key.SetDword(s_pszImageCopyWidth, s_nImageCopyWidth);
            app_key.SetDword(s_pszImageCopyHeight, s_nImageCopyHeight);
            app_key.SetDword(s_pszImageCopyByHeight, s_bImageCopyByHeight);
            app_key.SetDword(s_pszMarksHeight, s_nMarksHeight);
            app_key.SetDword(s_pszAddThickFrame, xg_bAddThickFrame);
            app_key.SetDword(s_pszCharFeed, xg_bCharFeed);
            app_key.SetDword(s_pszTateOki, xg_bTateOki);

            app_key.SetDword(s_pszWhiteCellColor, xg_rgbWhiteCellColor);
            app_key.SetDword(s_pszBlackCellColor, xg_rgbBlackCellColor);
            app_key.SetDword(s_pszMarkedCellColor, xg_rgbMarkedCellColor);

            app_key.SetDword(s_pszDrawFrameForMarkedCell, xg_bDrawFrameForMarkedCell);
            app_key.SetDword(s_pszSmartResolution, xg_bSmartResolution);
            app_key.SetDword(s_pszInputMode, (DWORD)xg_imode);
            app_key.SetDword(s_pszZoomRate, xg_nZoomRate);

            // 辞書ファイルのリストを設定する。
            nCount = static_cast<int>(xg_dict_files.size());
            app_key.SetDword(s_pszRecentCount, nCount);
            for (i = 0; i < nCount; i++) {
                ::wsprintfW(szFormat.data(), s_pszRecent, i + 1);
                app_key.SetSz(szFormat.data(), xg_dict_files[i].c_str());
            }

            // 保存先のリストを設定する。
            nCount = static_cast<int>(s_dirs_save_to.size());
            app_key.SetDword(s_pszSaveToCount, nCount);
            for (i = 0; i < nCount; i++)
            {
                ::wsprintfW(szFormat.data(), s_pszSaveTo, i + 1);
                app_key.SetSz(szFormat.data(), s_dirs_save_to[i].c_str());
            }

            app_key.SetDword(s_pszHintsWndX, s_nHintsWndX);
            app_key.SetDword(s_pszHintsWndY, s_nHintsWndY);
            app_key.SetDword(s_pszHintsWndCX, s_nHintsWndCX);
            app_key.SetDword(s_pszHintsWndCY, s_nHintsWndCY);

            app_key.SetDword(s_pszCandsWndX, s_nCandsWndX);
            app_key.SetDword(s_pszCandsWndY, s_nCandsWndY);
            app_key.SetDword(s_pszCandsWndCX, s_nCandsWndCX);
            app_key.SetDword(s_pszCandsWndCY, s_nCandsWndCY);

            app_key.SetDword(s_pszInputPaletteWndX, xg_nInputPaletteWndX);
            app_key.SetDword(s_pszInputPaletteWndY, xg_nInputPaletteWndY);

            app_key.SetDword(s_pszMainWndX, s_nMainWndX);
            app_key.SetDword(s_pszMainWndY, s_nMainWndY);
            app_key.SetDword(s_pszMainWndCX, s_nMainWndCX);
            app_key.SetDword(s_pszMainWndCY, s_nMainWndCY);

            app_key.SetDword(s_pszTateInput, xg_bTateInput);
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
    std::array<WCHAR,512> sz;
    static std::wstring s_word;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央に寄せる。
        XgCenterDialog(hwnd);

        // ダイアログを初期化する。
        s_word = *reinterpret_cast<std::wstring *>(lParam);
        ::wsprintfW(sz.data(), XgLoadStringDx1(42), s_word.data(), s_word.data());
        ::SetDlgItemTextW(hwnd, stc1, sz.data());

        // ヒントが追加された。
        xg_bHintsAdded = true;
        return true;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // テキストを取得する。
            ::GetDlgItemTextW(hwnd, edt1, sz.data(), 
                              static_cast<int>(sz.size()));

            // 辞書データに追加する。
            xg_dict_data.emplace_back(s_word, sz.data());

            // 二分探索のために、並び替えておく。
            XgSortAndUniqueDictData();

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
    // 四隅には黒マスは置けません。
    if (xg_xword.CornerBlack()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(43), nullptr, MB_ICONERROR);
        return false;
    }

    // 連黒禁。
    if (xg_xword.DoubleBlack()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(44), nullptr, MB_ICONERROR);
        return false;
    }

    // 三方向が黒マスで囲まれたマスを作ってはいけません。
    if (xg_xword.TriBlackArround()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(46), nullptr, MB_ICONERROR);
        return false;
    }

    // 分断禁。
    if (xg_xword.DividedByBlack()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(47), nullptr, MB_ICONERROR);
        return false;
    }

    // クロスワードに含まれる単語のチェック。
    XG_Pos pos;
    std::vector<std::wstring> vNotFoundWords;
    XG_EpvCode code = xg_xword.EveryPatternValid1(vNotFoundWords, pos, xg_bNoAddBlack);
    if (code == xg_epv_PATNOTMATCH) {
        if (check_words) {
            // パターンにマッチしないマスがあった。
            std::array<WCHAR,128> sz;
            ::wsprintfW(sz.data(), XgLoadStringDx1(48), pos.m_i + 1, pos.m_j + 1);
            XgCenterMessageBoxW(hwnd, sz.data(), nullptr, MB_ICONERROR);
            return false;
        }
    } else if (code == xg_epv_DOUBLEWORD) {
        // すでに使用した単語があった。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(45), nullptr, MB_ICONERROR);
        return false;
    } else if (code == xg_epv_LENGTHMISMATCH) {
        if (check_words) {
            // 登録されている単語と長さの一致しないスペースがあった。
            std::array<WCHAR,128> sz;
            ::wsprintfW(sz.data(), XgLoadStringDx1(54), pos.m_i + 1, pos.m_j + 1);
            XgCenterMessageBoxW(hwnd, sz.data(), nullptr, MB_ICONERROR);
            return false;
        }
    }

    // 見つからなかった単語があるか？
    if (!vNotFoundWords.empty()) {
        if (check_words) {
            // 単語が登録されていない。
            for (auto& word : vNotFoundWords) {
                // ヒントの入力を促す。
                if (::DialogBoxParamW(xg_hInstance, MAKEINTRESOURCEW(4), hwnd,
                                      XgInputHintDlgProc,
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

//////////////////////////////////////////////////////////////////////////////

// [新規作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgNewDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    int i, nCount, n1, n2;
    std::array<WCHAR,MAX_PATH> szFile, szTarget;
    std::wstring strFile;
    COMBOBOXEXITEMW item;
    OPENFILENAMEW ofn;
    HDROP hDrop;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, s_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, s_nCols, FALSE);
        // 辞書ファイルのパス名のベクターをコンボボックスに設定する。
        nCount = static_cast<int>(xg_dict_files.size());
        for (i = 0; i < nCount; i++) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            ::lstrcpyW(szFile.data(), xg_dict_files[i].data());
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // ドラッグ＆ドロップを受け付ける。
        ::DragAcceptFiles(hwnd, TRUE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl;

            hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);

            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(xg_nMaxSize, xg_nMinSize));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(xg_nMaxSize, xg_nMinSize));
        return TRUE;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile.data(), MAX_PATH);
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile.data(), szTarget.data())) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            ::lstrcpyW(szFile.data(), szTarget.data());
        }

        // 同じ項目がすでにあれば、削除する。
        i = static_cast<int>(::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                                                 reinterpret_cast<LPARAM>(szFile.data())));
        if (i != CB_ERR) {
            ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
        }
        // コンボボックスの最初に挿入する。
        item.mask = CBEIF_TEXT;
        item.iItem = 0;
        item.pszText = szFile.data();
        item.cchTextMax = -1;
        ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                            reinterpret_cast<LPARAM>(&item));
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < xg_nMinSize || n1 > xg_nMaxSize) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(18), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return 0;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < xg_nMinSize || n2 > xg_nMaxSize) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(18), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return 0;
            }
            // 辞書ファイルのパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb1, szFile.data(), 
                            static_cast<int>(szFile.size()));
            strFile = szFile.data();
            xg_str_trim(strFile);
            // 正しく読み込めるか？
            if (XgLoadDictFile(strFile.data())) {
                // 読み込めた。
                // 読み込んだファイル項目を一番上にする。
                auto end = xg_dict_files.end();
                for (auto it = xg_dict_files.begin(); it != end; it++) {
                    if (_wcsicmp((*it).data(), strFile.data()) == 0) {
                        xg_dict_files.erase(it);
                        break;
                    }
                }
                xg_dict_files.emplace_front(strFile);

                // 初期化する。
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.ResetAndSetSize(n1, n2);
                xg_nRows = s_nRows = n1;
                xg_nCols = s_nCols = n2;
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
                // マークの更新を通知する。
                XgMarkUpdate();
                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            } else {
                // 読み込めなかったのでエラーを表示する。
                ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
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
            ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(50));
            szFile[0] = 0;
            ofn.lpstrFile = szFile.data();
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = XgLoadStringDx1(19);
            ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = L"dic";
            if (::GetOpenFileNameW(&ofn)) {
                // コンボボックスにテキストを設定。
                ::SetDlgItemTextW(hwnd, cmb1, szFile.data());
            }
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
    int i, n1, n2;
    std::array<WCHAR,MAX_PATH> szFile, szTarget;
    std::wstring strFile;
    COMBOBOXEXITEMW item;
    OPENFILENAMEW ofn;
    HDROP hDrop;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, s_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, s_nCols, FALSE);
        // 辞書ファイルのパス名のベクターをコンボボックスに設定する。
        for (const auto& dict_file : xg_dict_files) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            ::lstrcpyW(szFile.data(), dict_file.data());
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // 自動で再計算をするか？
        if (s_bAutoRetry)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        // スマート解決か？
        if (xg_bSmartResolution)
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        // ドラッグ＆ドロップを受け付ける。
        ::DragAcceptFiles(hwnd, TRUE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl;

            hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);

            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(xg_nMaxSize, xg_nMinSize));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(xg_nMaxSize, xg_nMinSize));
        return TRUE;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile.data(), static_cast<UINT>(szFile.size()));
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile.data(), szTarget.data())) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            ::lstrcpyW(szFile.data(), szTarget.data());
        }

        // 同じ項目がすでにあれば、削除する。
        i = static_cast<int>(::SendDlgItemMessageW(
            hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
            reinterpret_cast<LPARAM>(szFile.data())));
        if (i != CB_ERR) {
            ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
        }
        // コンボボックスの最初に挿入する。
        item.mask = CBEIF_TEXT;
        item.iItem = 0;
        item.pszText = szFile.data();
        item.cchTextMax = -1;
        ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                              reinterpret_cast<LPARAM>(&item));
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < xg_nMinSize || n1 > xg_nMaxSize) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(18), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return 0;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < xg_nMinSize || n2 > xg_nMaxSize) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(18), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return 0;
            }
            // 自動で再計算をするか？
            s_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // スマート解決か？
            xg_bSmartResolution = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
            // 辞書ファイルのパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb1, szFile.data(), 
                              static_cast<int>(szFile.size()));
            strFile = szFile.data();
            xg_str_trim(strFile);
            // 正しく読み込めるか？
            if (XgLoadDictFile(strFile.data())) {
                // 読み込めた。
                // 読み込んだファイル項目を一番上にする。
                auto end = xg_dict_files.end();
                for (auto it = xg_dict_files.begin(); it != end; it++) {
                    if (_wcsicmp(it->data(), strFile.data()) == 0) {
                        // TODO: なぜかここでMinGW32が停止する。。。
                        xg_dict_files.erase(it);
                        break;
                    }
                }
                xg_dict_files.emplace_front(strFile);

                // 初期化する。
                {
                    xg_bSolved = false;
                    xg_bShowAnswer = false;
                    xg_xword.ResetAndSetSize(n1, n2);
                    xg_nRows = s_nRows = n1;
                    xg_nCols = s_nCols = n2;
                    xg_vTateInfo.clear();
                    xg_vYokoInfo.clear();
                    xg_vMarks.clear();
                    xg_vMarkedCands.clear();
                }
                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            } else {
                // 読み込めなかったのでエラーを表示する。
                ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
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
            ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(50));
            szFile[0] = 0;
            ofn.lpstrFile = szFile.data();
            ofn.nMaxFile = static_cast<DWORD>(szFile.size());
            ofn.lpstrTitle = XgLoadStringDx1(19);
            ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = L"dic";
            if (::GetOpenFileNameW(&ofn)) {
                // コンボボックスにテキストを設定。
                ::SetDlgItemTextW(hwnd, cmb1, szFile.data());
            }
            break;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// 保存先。
std::array<WCHAR,MAX_PATH> xg_szDir = { { 0 } };

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
        ::SendMessageW(hwnd, BFFM_SETSELECTION, TRUE,
                       reinterpret_cast<LPARAM>(xg_szDir.data()));
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// [問題の連続作成]ダイアログのダイアログ プロシージャ。
extern "C" INT_PTR CALLBACK
XgGenerateRepeatedlyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    int i, n1, n2;
    std::array<WCHAR,MAX_PATH> szFile, szTarget;
    std::wstring strFile, strDir;
    COMBOBOXEXITEMW item;
    OPENFILENAMEW ofn;
    HDROP hDrop;
    BROWSEINFOW bi;
    DWORD attrs;
    LPITEMIDLIST pidl;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, s_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, s_nCols, FALSE);
        // 辞書ファイルのパス名のベクターをコンボボックスに設定する。
        for (const auto& dict_file : xg_dict_files) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            ::lstrcpyW(szFile.data(), dict_file.data());
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // 保存先を設定する。
        for (const auto& dir : s_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            ::lstrcpyW(szFile.data(), dir.data());
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);
        // 自動で再計算をするか？
        if (s_bAutoRetry)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        // 無制限に生成するか？
        if (s_bInfinite) {
            // 「無制限に生成」チェックボックスにチェックを入れる。
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
            // 回数を無効化する。
            ::EnableWindow(::GetDlgItem(hwnd, edt3), FALSE);
            ::SetDlgItemTextW(hwnd, edt3, NULL);
        } else {
            // 生成する数を設定する。
            ::SetDlgItemInt(hwnd, edt3, s_nNumberToGenerate, FALSE);
        }
        // JSON形式で保存するか？
        if (xg_bSaveAsJsonFile)
            ::CheckDlgButton(hwnd, chx3, BST_CHECKED);
        // ドラッグ＆ドロップを受け付ける。
        ::DragAcceptFiles(hwnd, TRUE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl;

            hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, NULL);

            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, NULL);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(xg_nMaxSize, xg_nMinSize));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(xg_nMaxSize, xg_nMinSize));
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(2, 100));
        return true;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile.data(), static_cast<UINT>(szFile.size()));
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile.data(), szTarget.data())) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            ::lstrcpyW(szFile.data(), szTarget.data());
        }

        // ファイルの属性を確認する。
        attrs = ::GetFileAttributesW(szFile.data());
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            // ディレクトリーだった。
            // 同じ項目がすでにあれば、削除する。
            i = static_cast<int>(::SendDlgItemMessageW(
                hwnd, cmb2, CB_FINDSTRINGEXACT, 0,
                reinterpret_cast<LPARAM>(szFile.data())));
            if (i != CB_ERR) {
                ::SendDlgItemMessageW(hwnd, cmb2, CB_DELETESTRING, i, 0);
            }
            // コンボボックスの最初に挿入する。
            item.mask = CBEIF_TEXT;
            item.iItem = 0;
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEMW, 0,
                                reinterpret_cast<LPARAM>(&item));
            // コンボボックスの最初の項目を選択する。
            ::SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);
        } else {
            // 普通のファイルだった。
            // 同じ項目がすでにあれば、削除する。
            i = static_cast<int>(::SendDlgItemMessageW(
                hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                reinterpret_cast<LPARAM>(szFile.data())));
            if (i != CB_ERR) {
                ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
            }
            // コンボボックスの最初に挿入する。
            item.mask = CBEIF_TEXT;
            item.iItem = 0;
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                reinterpret_cast<LPARAM>(&item));
            // コンボボックスの最初の項目を選択する。
            ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < xg_nMinSize || n1 > xg_nMaxSize) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(18), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return 0;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < xg_nMinSize || n2 > xg_nMaxSize) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(18), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return 0;
            }
            // 自動で再計算をするか？
            s_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // 辞書ファイルのパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb1, szFile.data(), 
                            static_cast<int>(szFile.size()));
            strFile = szFile.data();
            xg_str_trim(strFile);
            // 保存先のパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb2, szFile.data(), 
                            static_cast<int>(szFile.size()));
            attrs = ::GetFileAttributesW(szFile.data());
            if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                // パスがなければ作成する。
                if (!XgMakePathW(szFile.data())) {
                    // 作成に失敗。
                    ::SendDlgItemMessageW(hwnd, cmb2, CB_SETEDITSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(57), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, cmb2));
                    return 0;
                }
            }
            // 保存先をセットする。
            strDir = szFile.data();
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
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                // 無制限。
                s_nNumberToGenerate = -1;
                s_bInfinite = true;
            } else {
                // 無制限ではない。
                BOOL bTranslated;
                s_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt3, &bTranslated, FALSE);
                if (!bTranslated || s_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(58), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt3));
                    return 0;
                }
                s_bInfinite = false;
            }
            // JSON形式として保存するか？
            if (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED) {
                xg_bSaveAsJsonFile = true;
            } else {
                xg_bSaveAsJsonFile = false;
            }
            // 正しく読み込めるか？
            if (XgLoadDictFile(strFile.data())) {
                // 読み込めた。
                // 読み込んだファイル項目を一番上にする。
                auto end = xg_dict_files.end();
                for (auto it = xg_dict_files.begin(); it != end; it++) {
                    if (_wcsicmp((*it).data(), strFile.data()) == 0) {
                        xg_dict_files.erase(it);
                        break;
                    }
                }
                xg_dict_files.emplace_front(strFile);

                // 初期化する。
                {
                    xg_bSolved = false;
                    xg_bShowAnswer = false;
                    xg_xword.ResetAndSetSize(n1, n2);
                    xg_nRows = s_nRows = n1;
                    xg_nCols = s_nCols = n2;
                    xg_vTateInfo.clear();
                    xg_vYokoInfo.clear();
                    xg_vMarks.clear();
                    xg_vMarkedCands.clear();
                }
                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            } else {
                // 読み込めなかったのでエラーを表示する。
                ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
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
            ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(50));
            szFile[0] = 0;
            ofn.lpstrFile = szFile.data();
            ofn.nMaxFile = static_cast<DWORD>(szFile.size());
            ofn.lpstrTitle = XgLoadStringDx1(19);
            ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = L"dic";
            if (::GetOpenFileNameW(&ofn)) {
                // コンボボックスにテキストを設定。
                ::SetDlgItemTextW(hwnd, cmb1, szFile.data());
            }
            break;

        case psh2:
            // ユーザーに保存先の場所を問い合わせる。
            ZeroMemory(&bi, sizeof(bi));
            bi.hwndOwner = hwnd;
            bi.lpszTitle = XgLoadStringDx1(56);
            bi.ulFlags = BIF_RETURNONLYFSDIRS;
            bi.lpfn = XgBrowseCallbackProc;
            ::GetDlgItemTextW(hwnd, cmb2, xg_szDir.data(), 
                            static_cast<int>(xg_szDir.size()));
            pidl = ::SHBrowseForFolderW(&bi);
            if (pidl) {
                // パスをコンボボックスに設定。
                ::SHGetPathFromIDListW(pidl, szFile.data());
                ::SetDlgItemTextW(hwnd, cmb2, szFile.data());
                ::CoTaskMemFree(pidl);
            }
            break;

        case chx2:
            // 無制限にチェックが入っていれば、回数を無効にする。
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
                ::EnableWindow(::GetDlgItem(hwnd, edt3), FALSE);
            else
                ::EnableWindow(::GetDlgItem(hwnd, edt3), TRUE);
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
    std::array<WCHAR,MAX_PATH> szFile, szTarget;
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
            ::lstrcpyW(szFile.data(), dir.data());
            item.pszText = szFile.data();
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
        if (s_bInfinite) {
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
            ::EnableWindow(::GetDlgItem(hwnd, edt1), FALSE);
            ::SetDlgItemTextW(hwnd, edt1, NULL);
        } else {
            // 生成する数を設定する。
            ::SetDlgItemInt(hwnd, edt1, s_nNumberToGenerate, FALSE);
        }
        // JSONとして保存するか？
        ::CheckDlgButton(hwnd, chx3, (xg_bSaveAsJsonFile ? BST_CHECKED : BST_UNCHECKED));
        // ファイルドロップを有効にする。
        ::DragAcceptFiles(hwnd, TRUE);
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(2, 100));
        return true;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile.data(), static_cast<UINT>(szFile.size()));
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile.data(), szTarget.data())) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            ::lstrcpyW(szFile.data(), szTarget.data());
        }

        // ファイルの属性を確認する。
        attrs = ::GetFileAttributesW(szFile.data());
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            // ディレクトリーだった。
            // 同じ項目がすでにあれば、削除する。
            i = static_cast<int>(::SendDlgItemMessageW(
                hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                reinterpret_cast<LPARAM>(szFile.data())));
            if (i != CB_ERR) {
                ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
            }
            // コンボボックスの最初に挿入する。
            item.mask = CBEIF_TEXT;
            item.iItem = 0;
            item.pszText = szFile.data();
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
            ::GetDlgItemTextW(hwnd, cmb1, szFile.data(), 
                            static_cast<int>(szFile.size()));
            attrs = ::GetFileAttributesW(szFile.data());
            if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                // パスがなければ作成する。
                if (!XgMakePathW(szFile.data())) {
                    // 作成に失敗。
                    ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(57), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, cmb1));
                    return 0;
                }
            }
            // 保存先をセットする。
            strDir = szFile.data();
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
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                s_nNumberToGenerate = -1;
                s_bInfinite = true;
            } else {
                BOOL bTranslated;
                s_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
                if (!bTranslated || s_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(58), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt1));
                    return 0;
                }
                s_bInfinite = false;
            }
            // JSON形式で保存するか？
            if (::IsDlgButtonChecked(hwnd, chx3) == BST_CHECKED) {
                xg_bSaveAsJsonFile = true;
            } else {
                xg_bSaveAsJsonFile = false;
            }
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
            bi.lpszTitle = XgLoadStringDx1(56);
            bi.ulFlags = BIF_RETURNONLYFSDIRS;
            bi.lpfn = XgBrowseCallbackProc;
            ::GetDlgItemTextW(hwnd, cmb1, xg_szDir.data(), 
                            static_cast<int>(xg_szDir.size()));
            pidl = ::SHBrowseForFolderW(&bi);
            if (pidl) {
                // コンボボックスにパスを設定する。
                ::SHGetPathFromIDListW(pidl, szFile.data());
                ::SetDlgItemTextW(hwnd, cmb1, szFile.data());
                ::CoTaskMemFree(pidl);
            }
            break;

        case chx2:
            // 無制限にチェックが入っていれば、回数を無効にする。
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED)
                ::EnableWindow(::GetDlgItem(hwnd, edt1), FALSE);
            else
                ::EnableWindow(::GetDlgItem(hwnd, edt1), TRUE);
            break;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////

// ヒントの内容をメモ帳で開く。
bool __fastcall XgOpenHintsByNotepad(HWND /*hwnd*/, bool bShowAnswer)
{
    std::array<WCHAR,MAX_PATH>       szPath;
    std::array<WCHAR,MAX_PATH*2>     szCmdLine;
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
    ::GetTempPathW(MAX_PATH, szPath.data());
    ::lstrcatW(szPath.data(), XgLoadStringDx1(28));
    HANDLE hFile = ::CreateFileW(szPath.data(), GENERIC_WRITE, FILE_SHARE_READ,
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
        ::wsprintfW(szCmdLine.data(), XgLoadStringDx1(27), szPath.data());
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;
        if (::CreateProcessW(nullptr, szCmdLine.data(), nullptr, nullptr, FALSE,
                             0, nullptr, nullptr, &si, &pi))
        {
            // メモ帳が待ち状態になるまで待つ。
            if (::WaitForInputIdle(pi.hProcess, 5 * 1000) != WAIT_TIMEOUT) {
                // 0.2秒待つ。
                ::Sleep(200);
                // ファイルを削除する。
                ::DeleteFileW(szPath.data());
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

#define xg_dwTimerInterval 500

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
        // 再計算までの時間を求める。
        #ifndef NDEBUG
            s_dwWait = 8 * xg_nRows * xg_nCols *
                           (xg_nRows + xg_nCols) + 800;
        #else
            s_dwWait = 4 * xg_nRows * xg_nCols *
                           (xg_nRows + xg_nCols) + 800;
        #endif
        // 解を求めるのを開始。
        XgStartSolve();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
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
            XgStartSolve();
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
            std::array<WCHAR,MAX_PATH> sz;
            DWORD dwTick = ::GetTickCount();
            ::wsprintfW(sz.data(), XgLoadStringDx1(32),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10, s_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz.data());
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
                XgStartSolve();
                // タイマーをセットする。
                ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            }
        }
        break;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////

// 連続生成キャンセルダイアログ。
extern "C" INT_PTR CALLBACK
XgCancelGenerateRepeatedlyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
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
        // 再計算までの時間を求める。
        #ifndef NDEBUG
            s_dwWait = 8 * xg_nRows * xg_nCols *
                       (xg_nRows + xg_nCols) + 800;
        #else
            s_dwWait = 4 * xg_nRows * xg_nCols *
                       (xg_nRows + xg_nCols) + 800;
        #endif
        // 解を求めるのを開始。
        XgStartSolve();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        return false;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case psh1:  // [キャンセル]ボタン。
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

        case psh2:  // [再計算]ボタン。
            // タイマーを解除する。
            ::KillTimer(hwnd, 999);
            // 再計算しなおす。
            xg_bRetrying = true;
            XgWaitForThreads();
            XgCloseThreads();
            ::InterlockedIncrement(&s_nRetryCount);
            s_dwTick1 = ::GetTickCount();
            XgStartSolve();
            // タイマーをセットする。
            ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            break;
        }
        break;

    case WM_SYSCOMMAND: // ダイアログが閉じられた。
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

        // 連続生成か？
        {
            // 生成した数を表示する。
            std::array<WCHAR,MAX_PATH> sz;
            DWORD dwTick = ::GetTickCount();
            ::wsprintfW(sz.data(), XgLoadStringDx1(59), s_nNumberGenerated,
                (dwTick - s_dwTick0) / 1000,
                (dwTick - s_dwTick0) / 100 % 10);
            ::SetDlgItemTextW(hwnd, stc1, sz.data());
        }

        // 終了したスレッドがあるか？
        if (XgIsAnyThreadTerminated()) {
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // 生成した問題を保存先に保存する。
            if (xg_bSolved) {
                // 解か？
                ::EnterCriticalSection(&xg_cs);
                bool ok = xg_solution.IsSolution();
                ::LeaveCriticalSection(&xg_cs);
                if (!ok)
                    break;

                std::array<WCHAR,MAX_PATH> szPath, szDir;
                std::array<WCHAR,32> szName;

                // パスを生成する。
                ::lstrcpyW(szDir.data(), s_dirs_save_to[0].data());
                PathAddBackslashW(szDir.data());

                // ファイル名を生成する。
                UINT u;
                for (u = 1; u <= 0xFFFF; u++) {
                    if (xg_bSaveAsJsonFile) {
                        ::wsprintfW(szName.data(), L"%dx%d-%04u.xwj",
                                  xg_nRows, xg_nCols, u);
                    } else {
                        ::wsprintfW(szName.data(), L"%dx%d-%04u.xwd",
                                  xg_nRows, xg_nCols, u);
                    }
                    ::lstrcpyW(szPath.data(), szDir.data());
                    ::lstrcatW(szPath.data(), szName.data());
                    if (::GetFileAttributesW(szPath.data()) == 0xFFFFFFFF)
                        break;
                }

                if (u != 0x10000) {
                    // ファイル名が作成できた。排他制御しながら、保存する。
                    ::EnterCriticalSection(&xg_cs);
                    {
                        // カギ番号を更新する。
                        xg_solution.DoNumberingNoCheck();

                        // ヒントを更新する。
                        XgUpdateHints(hwnd);

                        // ファイルに保存する。
                        bool bOK = XgDoSave(hwnd, szPath.data(), xg_bSaveAsJsonFile);
                        if (bOK)
                            s_nNumberGenerated++;
                    }
                    ::LeaveCriticalSection(&xg_cs);
                }
                

                // ディスク容量を確認する。
                ULARGE_INTEGER ull1, ull2, ull3;
                if (::GetDiskFreeSpaceExW(szPath.data(), &ull1, &ull2, &ull3)) {
                    if (ull1.QuadPart < 0x1000000)  // 1MB
                    {
                        s_bOutOfDiskSpace = true;
                    }
                }
                xg_bSolved = false;

                if (s_bOutOfDiskSpace ||
                    (!s_bInfinite && s_nNumberToGenerate <= s_nNumberGenerated))
                {
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
                    // タイマーを解除する。
                    ::KillTimer(hwnd, 999);
                    // 再計算しなおす。
                    xg_bRetrying = true;
                    XgWaitForThreads();
                    XgCloseThreads();
                    ::InterlockedIncrement(&s_nRetryCount);
                    s_dwTick1 = ::GetTickCount();
                    XgStartSolve();
                    // タイマーをセットする。
                    ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
                }
            }
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
                XgStartSolve();
                // タイマーをセットする。
                ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            }
        }
        break;
    }
    return false;
}

// 連続生成キャンセルダイアログ。
extern "C" INT_PTR CALLBACK
XgCancelSolveRepeatedlyDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
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
        // 再計算までの時間を求める。
        #ifndef NDEBUG
            s_dwWait = 8 * xg_nRows * xg_nCols *
                       (xg_nRows + xg_nCols) + 800;
        #else
            s_dwWait = 4 * xg_nRows * xg_nCols *
                       (xg_nRows + xg_nCols) + 800;
        #endif
        // 解を求めるのを開始。
        XgStartSolve();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
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
            XgStartSolve();
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

        // 連続生成か？
        {
            // 生成した数を表示する。
            std::array<WCHAR,MAX_PATH> sz;
            DWORD dwTick = ::GetTickCount();
            ::wsprintfW(sz.data(), XgLoadStringDx1(59), s_nNumberGenerated,
                (dwTick - s_dwTick0) / 1000,
                (dwTick - s_dwTick0) / 100 % 10);
            ::SetDlgItemTextW(hwnd, stc1, sz.data());
        }

        // 終了したスレッドがあるか？
        if (XgIsAnyThreadTerminated()) {
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // 生成した問題を保存先に保存する。
            if (xg_bSolved) {
                ::EnterCriticalSection(&xg_cs);
                bool ok = xg_solution.IsSolution();
                ::LeaveCriticalSection(&xg_cs);

                if (!ok)
                    break;

                std::array<WCHAR,MAX_PATH> szPath, szDir;
                std::array<WCHAR,32> szName;

                // パスを生成する。
                ::lstrcpyW(szDir.data(), s_dirs_save_to[0].data());
                PathAddBackslashW(szDir.data());

                // ファイル名を生成する。
                UINT u;
                for (u = 1; u <= 0xFFFF; u++) {
                    if (xg_bSaveAsJsonFile) {
                        ::wsprintfW(szName.data(), L"%dx%d-%04u.xwj",
                                  xg_nRows, xg_nCols, u);
                    } else {
                        ::wsprintfW(szName.data(), L"%dx%d-%04u.xwd",
                                  xg_nRows, xg_nCols, u);
                    }
                    ::lstrcpyW(szPath.data(), szDir.data());
                    ::lstrcatW(szPath.data(), szName.data());
                    if (::GetFileAttributesW(szPath.data()) == 0xFFFFFFFF)
                        break;
                }

                if (u != 0x10000) {
                    // ファイル名が作成できた。排他制御しながら保存する。
                    ::EnterCriticalSection(&xg_cs);
                    {
                        // カギ番号を更新する。
                        xg_solution.DoNumberingNoCheck();

                        // ヒントを更新する。
                        XgUpdateHints(hwnd);

                        // ファイルに保存する。
                        bool bOK = XgDoSave(hwnd, szPath.data(), xg_bSaveAsJsonFile);
                        if (bOK)
                            s_nNumberGenerated++;
                    }
                    ::LeaveCriticalSection(&xg_cs);
                }

                // ディスク容量を確認する。
                ULARGE_INTEGER ull1, ull2, ull3;
                if (::GetDiskFreeSpaceExW(szPath.data(), &ull1, &ull2, &ull3)) {
                    if (ull1.QuadPart < 0x1000000)  // 1MB
                    {
                        s_bOutOfDiskSpace = true;
                    }
                }
                xg_bSolved = false;
            }

            if (s_bOutOfDiskSpace ||
                (!s_bInfinite && s_nNumberToGenerate <= s_nNumberGenerated))
            {
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
                // タイマーを解除する。
                ::KillTimer(hwnd, 999);
                // 再計算しなおす。
                xg_bRetrying = true;
                XgWaitForThreads();
                XgCloseThreads();
                ::InterlockedIncrement(&s_nRetryCount);
                s_dwTick1 = ::GetTickCount();
                XgStartSolve();
                // タイマーをセットする。
                ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
            }
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
                XgStartSolve();
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
        // 再計算までの時間を求める。
        #ifndef NDEBUG
            s_dwWait = 8 * xg_nRows * xg_nCols *
                       (xg_nRows + xg_nCols) + 800;
        #else
            s_dwWait = 4 * xg_nRows * xg_nCols *
                       (xg_nRows + xg_nCols) + 800;
        #endif
        // 解を求めるのを開始。
        XgStartSolveNoAddBlack();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
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
            XgStartSolveNoAddBlack();
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
            std::array<WCHAR,MAX_PATH> sz;
            DWORD dwTick = ::GetTickCount();
            ::wsprintfW(sz.data(), XgLoadStringDx1(32),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10, s_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz.data());
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
                XgStartSolveNoAddBlack();
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
        // 再計算までの時間を求める。
        #ifndef NDEBUG
            s_dwWait = 7 * xg_nRows * xg_nCols *
                           (xg_nRows + xg_nCols) + 1000;
        #else
            s_dwWait = 3 * xg_nRows * xg_nCols *
                           (xg_nRows + xg_nCols) + 1000;
        #endif
        // 解を求めるのを開始。
        XgStartSolveSmart();
        // タイマーをセットする。
        ::SetTimer(hwnd, 999, xg_dwTimerInterval, nullptr);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
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
            XgStartSolveSmart();
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
            std::array<WCHAR,MAX_PATH> sz;
            DWORD dwTick = ::GetTickCount();
            ::wsprintfW(sz.data(), XgLoadStringDx1(32),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10, s_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz.data());
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
                XgStartSolveSmart();
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
            std::array<WCHAR,MAX_PATH> sz;
            DWORD dwTick = ::GetTickCount();
            ::wsprintfW(sz.data(), XgLoadStringDx1(107),
                    (dwTick - s_dwTick0) / 1000,
                    (dwTick - s_dwTick0) / 100 % 10);
            ::SetDlgItemTextW(hwnd, stc1, sz.data());
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
        ::XgCenterMessageBoxW(xg_hMainWnd, XgLoadStringDx1(41), nullptr,
                            MB_ICONERROR);
        ::DeleteDC(hdc);
        ::GlobalFree(ppd->hDevMode);
        ::GlobalFree(ppd->hDevNames);
        return;
    }

    DOCINFOW di;
    ZeroMemory(&di, sizeof(di));
    di.cbSize = sizeof(di);
    di.lpszDocName = XgLoadStringDx1(39);

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
                if (xg_imode == xg_im_HANGUL)
                    ::lstrcpyW(lf.lfFaceName, XgLoadStringDx1(67)); // ハングルの場合。
                else
                    ::lstrcpyW(lf.lfFaceName, XgLoadStringDx1(35)); // その他の場合。
                if (xg_szCellFont[0])
                    ::lstrcpyW(lf.lfFaceName, xg_szCellFont.data());

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
                str = XgLoadStringDx1(40);
                str += strMarkWord;
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);
                ::SelectObject(hdc, hFontOld);
                ::DeleteFont(hFont);
            }

            // EMFオブジェクトを作成する。
            HDC hdcEMF = ::CreateEnhMetaFileW(hdc, nullptr, nullptr, XgLoadStringDx1(2));
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
            if (xg_imode == xg_im_HANGUL)
                ::lstrcpyW(lf.lfFaceName, XgLoadStringDx1(67)); // ハングルの場合。
            else
                ::lstrcpyW(lf.lfFaceName, XgLoadStringDx1(35)); // その他の場合。
            if (xg_szCellFont[0])
                ::lstrcpyW(lf.lfFaceName, xg_szCellFont.data());

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
                if (xg_imode == xg_im_HANGUL)
                    ::lstrcpyW(lf.lfFaceName, XgLoadStringDx1(67)); // ハングルの場合。
                else
                    ::lstrcpyW(lf.lfFaceName, XgLoadStringDx1(35)); // その他の場合。
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
    params.lpszText = XgLoadStringDx1(1);
    params.lpszCaption = XgLoadStringDx2(2);
    params.dwStyle = MB_USERICON;
    params.lpszIcon = MAKEINTRESOURCEW(1);
    XgCenterMessageBoxIndirectW(&params);
}

// 新規作成ダイアログ。
bool __fastcall XgOnNew(HWND hwnd)
{
    int nID;
    nID = static_cast<int>(::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(1), hwnd,
                                        XgNewDlgProc));
    if (nID == IDOK) {
        // 初期化する。
        xg_bSolved = false;
        xg_bHintsAdded = false;
        xg_bShowAnswer = false;
        xg_caret_pos.clear();
        xg_strHeader.clear();
        xg_strNotes.clear();
        xg_strFileName.clear();

        // イメージを更新する。
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        return true;
    }
    return false;
}

// 問題の作成。
bool __fastcall XgOnGenerate(HWND hwnd, bool show_answer)
{
    std::array<WCHAR,MAX_PATH> sz;

    // [問題の作成]ダイアログ。
    int nID;
    nID = static_cast<int>(::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(2), hwnd,
                                      XgGenerateDlgProc));
    if (nID == IDOK) {
        xg_bSolvingEmpty = true;
        xg_bNoAddBlack = false;
        s_nNumberGenerated = 0;
        s_bOutOfDiskSpace = false;
        xg_strHeader.clear();
        xg_strNotes.clear();
        xg_strFileName.clear();
        XgSetInputModeFromDict(hwnd);

        // キャンセルダイアログを表示し、実行を開始する。
        ::EnableWindow(xg_hwndInputPalette, FALSE);
        if (xg_bSmartResolution && xg_nRows >= 7 && xg_nCols >= 7) {
            ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd, XgCancelSolveDlgProcSmart);
        } else {
            ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd, XgCancelSolveDlgProc);
        }
        ::EnableWindow(xg_hwndInputPalette, TRUE);

        // イメージを更新する。
        xg_bShowAnswer = show_answer;
        if (xg_bSmartResolution && xg_bCancelled) {
            xg_xword.clear();
        }
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        if (xg_bCancelled) {
            // キャンセルされた。
            ::wsprintfW(sz.data(), XgLoadStringDx1(31),
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
            XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);
        } else if (xg_bSolved) {
            // 成功メッセージを表示する。
            ::wsprintfW(sz.data(), XgLoadStringDx1(16),
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
            XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);

            // ヒントを更新して開く。
            XgUpdateHints(hwnd);
            XgShowHints(hwnd);
        } else {
            // 失敗メッセージを表示する。
            ::wsprintfW(sz.data(), XgLoadStringDx1(17),
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
            XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONERROR);
        }
        return true;
    }
    return false;
}

// 連続生成する。
bool __fastcall XgOnGenerateRepeatedly(HWND hwnd)
{
    #ifndef MZC_NO_SHAREWARE
        // シェアウェアの場合、登録されているか？
        if (!g_shareware.IsRegistered()) {
            g_shareware.ThisCommandRequiresRegistering(hwnd);
            if (!g_shareware.UrgeRegister(hwnd)) {
                // 機能を実行しない。
                return false;
            }
        }
    #endif  // ndef MZC_NO_SHAREWARE

    std::array<WCHAR,MAX_PATH> sz;

    // [問題の連続作成]ダイアログ。
    int nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    nID = static_cast<int>(::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(5), hwnd,
                                      XgGenerateRepeatedlyDlgProc));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID == IDOK) {
        // 初期化する。
        xg_bSolvingEmpty = true;
        xg_bNoAddBlack = false;
        xg_strFileName.clear();
        xg_strHeader.clear();
        xg_strNotes.clear();
        s_nNumberGenerated = 0;
        s_bOutOfDiskSpace = false;
        XgSetInputModeFromDict(hwnd);

        // キャンセルダイアログを表示し、実行を開始する。
        ::EnableWindow(xg_hwndInputPalette, FALSE);
        ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd,
                     XgCancelGenerateRepeatedlyDlgProc);
        ::EnableWindow(xg_hwndInputPalette, TRUE);

        // クリアする。
        xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
        xg_vMarkedCands.clear();
        xg_vMarks.clear();
        xg_vTateInfo.clear();
        xg_vYokoInfo.clear();
        xg_vecTateHints.clear();
        xg_vecYokoHints.clear();

        // イメージを更新する。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // ディスクに空きがあるか？
        if (s_bOutOfDiskSpace) {
            // なかった。
            ::wsprintfW(sz.data(), XgLoadStringDx1(61), s_nNumberGenerated,
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
        } else {
            // あった。
            ::wsprintfW(sz.data(), XgLoadStringDx1(60), s_nNumberGenerated,
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
        }

        // 終了メッセージを表示する。
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);

        // 保存先フォルダを開く。
        if (s_nNumberGenerated && !s_dirs_save_to.empty())
            ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                            nullptr, nullptr, SW_SHOWNORMAL);

        return true;
    }
    return false;
}

// 黒マスパターンを生成する。
void XgOnGenerateBlacks(HWND hwnd, bool sym)
{
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

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、生成を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    ::DialogBoxParamW(xg_hInstance,
        MAKEINTRESOURCEW(13), hwnd, XgCancelGenBlacksDlgProc,
        sym);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    XgUpdateImage(hwnd, 0, 0);

    std::array<WCHAR,MAX_PATH> sz;
    if (xg_bCancelled) {
        ::wsprintfW(sz.data(), XgLoadStringDx1(31),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);
    } else {
        ::wsprintfW(sz.data(), XgLoadStringDx1(108),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);
    }
}

// 解を求める。
bool __fastcall XgOnSolveAddBlack(HWND hwnd)
{
    // すでに解かれている場合は、実行を拒否する。
    if (xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

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
    XgSetInputModeFromDict(hwnd);

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd, XgCancelSolveDlgProc);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    std::array<WCHAR,MAX_PATH> sz;
    if (xg_bCancelled) {
        // キャンセルされた。
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        ::wsprintfW(sz.data(), XgLoadStringDx1(31),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 空マスがないか？
        if (xg_xword.IsFulfilled()) {
            // 空マスがない。クリア。
            xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
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
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 成功メッセージを表示する。
        ::wsprintfW(sz.data(), XgLoadStringDx1(8),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints(hwnd);
        XgShowHints(hwnd);
    } else {
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // 失敗メッセージを表示する。
        ::wsprintfW(sz.data(), XgLoadStringDx1(11),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        ::InvalidateRect(hwnd, nullptr, FALSE);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONERROR);
    }
    return true;
}

// 解を求める（黒マス追加なし）。
bool __fastcall XgOnSolveNoAddBlack(HWND hwnd)
{
    // すでに解かれている場合は、実行を拒否する。
    if (xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return false;
    }

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
    XgSetInputModeFromDict(hwnd);

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd,
                 XgCancelSolveDlgProcNoAddBlack);
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    std::array<WCHAR,MAX_PATH> sz;
    if (xg_bCancelled) {
        // キャンセルされた。
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 終了メッセージを表示する。
        ::wsprintfW(sz.data(), XgLoadStringDx1(31),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 空マスがないか？
        if (xg_xword.IsFulfilled()) {
            // 空マスがない。クリア。
            xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
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
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 成功メッセージを表示する。
        ::wsprintfW(sz.data(), XgLoadStringDx1(8),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints(hwnd);
        XgShowHints(hwnd);
    } else {
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // 失敗メッセージを表示する。
        ::wsprintfW(sz.data(), XgLoadStringDx1(11),
            (s_dwTick2 - s_dwTick0) / 1000,
            (s_dwTick2 - s_dwTick0) / 100 % 10);
        ::InvalidateRect(hwnd, nullptr, FALSE);
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONERROR);
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

    #ifndef MZC_NO_SHAREWARE
        // シェアウェアの場合、登録されているか？
        if (!g_shareware.IsRegistered()) {
            g_shareware.ThisCommandRequiresRegistering(hwnd);
            if (!g_shareware.UrgeRegister(hwnd)) {
                // 機能を実行しない。
                return false;
            }
        }
    #endif  // ndef MZC_NO_SHAREWARE

    // 黒マスルールなどをチェックする。
    xg_bNoAddBlack = false;
    if (!XgCheckCrossWord(hwnd)) {
        return false;
    }

    // 実行前のマスの状態を保存する。
    XG_Board xword_save(xg_xword);

    // [解の連続作成]ダイアログ。
    int nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    nID = static_cast<int>(::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(8), hwnd,
                                        XgSolveRepeatedlyDlgProc));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID == IDOK) {
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

        XgSetInputModeFromDict(hwnd);

        // キャンセルダイアログを表示し、実行を開始する。
        ::EnableWindow(xg_hwndInputPalette, FALSE);
        ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd,
                   XgCancelSolveRepeatedlyDlgProc);
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
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 終了メッセージを表示する。
        std::array<WCHAR,MAX_PATH> sz;
        if (s_bOutOfDiskSpace) {
            ::wsprintfW(sz.data(), XgLoadStringDx1(61), s_nNumberGenerated,
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
        } else {
            ::wsprintfW(sz.data(), XgLoadStringDx1(60), s_nNumberGenerated,
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
        }
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);

        // 保存先フォルダを開く。
        if (s_nNumberGenerated && !s_dirs_save_to.empty()) {
            ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                          nullptr, nullptr, SW_SHOWNORMAL);
        }
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

    #ifndef MZC_NO_SHAREWARE
        // シェアウェアの場合、登録されているか？
        if (!g_shareware.IsRegistered()) {
            g_shareware.ThisCommandRequiresRegistering(hwnd);
            if (!g_shareware.UrgeRegister(hwnd)) {
                // 機能を実行しない。
                return false;
            }
        }
    #endif  // ndef MZC_NO_SHAREWARE

    // 黒マスルールなどをチェックする。
    xg_bNoAddBlack = true;
    if (!XgCheckCrossWord(hwnd)) {
        return false;
    }

    // 実行前のマスの状態を保存する。
    XG_Board xword_save(xg_xword);

    // [解の連続作成]ダイアログ。
    int nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    nID = static_cast<int>(::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(8), hwnd,
                                      XgSolveRepeatedlyDlgProc));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID == IDOK) {
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
        XgSetInputModeFromDict(hwnd);

        // キャンセルダイアログを表示し、実行を開始する。
        ::EnableWindow(xg_hwndInputPalette, FALSE);
        ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(3), hwnd,
                     XgCancelSolveRepeatedlyDlgProc);
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
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 終了メッセージを表示する。
        std::array<WCHAR,MAX_PATH> sz;
        if (s_bOutOfDiskSpace) {
            ::wsprintfW(sz.data(), XgLoadStringDx1(61), s_nNumberGenerated,
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
        } else {
            ::wsprintfW(sz.data(), XgLoadStringDx1(60), s_nNumberGenerated,
                (s_dwTick2 - s_dwTick0) / 1000,
                (s_dwTick2 - s_dwTick0) / 100 % 10);
        }
        XgCenterMessageBoxW(hwnd, sz.data(), XgLoadStringDx2(9), MB_ICONINFORMATION);

        // 保存先フォルダを開く。
        if (s_nNumberGenerated && !s_dirs_save_to.empty()) {
            ::ShellExecuteW(hwnd, nullptr, s_dirs_save_to[0].data(),
                          nullptr, nullptr, SW_SHOWNORMAL);
        }
    }
    return true;
}

// 黒マス線対称チェック。
void XgOnLineSymmetryCheck(HWND hwnd)
{
    if (xg_xword.IsLineSymmetry()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(62), XgLoadStringDx2(2),
                          MB_ICONINFORMATION);
    } else {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(63), XgLoadStringDx2(2),
                          MB_ICONINFORMATION);
    }
}

// 黒マス点対称チェック。
void XgOnPointSymmetryCheck(HWND hwnd)
{
    if (xg_xword.IsPointSymmetry()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(64), XgLoadStringDx2(2),
                          MB_ICONINFORMATION);
    } else {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(65), XgLoadStringDx2(2),
                          MB_ICONINFORMATION);
    }
}

//////////////////////////////////////////////////////////////////////////////

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
                    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
                    if (hdc) {
                        // EMFに描画する。
                        XgDrawXWord(*pxw, hdc, &siz, false);

                        // EMFを設定。
                        ::SetClipboardData(CF_ENHMETAFILE, ::CloseEnhMetaFile(hdc));
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

    LPBYTE pb;
    pb = (LPBYTE)pbmih;
    vecData.insert(vecData.end(), pb, pb + sizeof(*pbmih));
    pb = (LPBYTE)bi.bmiColors;
    vecData.insert(vecData.end(), pb, pb + cbColors);
    pb = (LPBYTE)&Bits[0];
    vecData.insert(vecData.end(), pb, pb + Bits.size());

    return TRUE;
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
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
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
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(58), NULL,
                                    MB_ICONERROR);
                break;
            }
            s_nImageCopyHeight = ::GetDlgItemInt(hwnd, edt2, NULL, FALSE);
            if (s_nImageCopyHeight <= 0) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                SetFocus(::GetDlgItem(hwnd, edt2));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(58), NULL,
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
    if (IDOK != ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(11), hwnd,
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
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
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

    HDC hdc2 = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
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
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
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
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(58), NULL,
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

    if (IDOK != ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(12), hwnd,
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
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
    // EMFに描画する。
    XgDrawMarkWord(hdc, &siz);
    hEMF = ::CloseEnhMetaFile(hdc);

    MRect rc;
    int cx = siz.cx * s_nMarksHeight / siz.cy;
    ::SetRect(&rc, 0, 0, cx, s_nMarksHeight);

    // EMFを作成する。
    HDC hdc2 = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(2));
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
                return;
            }
        }
        // 確保したメモリを解放する。
        ::GlobalFree(hGlobal);
    }
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
    xg_str_replace_all(str, XgLoadStringDx1(92), L"");
    xg_str_replace_all(str, XgLoadStringDx1(93), L"");
    xg_str_replace_all(str, XgLoadStringDx1(94), XgLoadStringDx2(95));

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


// メニューを初期化する。
void __fastcall MainWnd_OnInitMenu(HWND /*hwnd*/, HMENU hMenu)
{
    if (xg_bTateInput)
    {
        ::CheckMenuRadioItem(hMenu, ID_INPUTH, ID_INPUTV, ID_INPUTV, MF_BYCOMMAND);
    }
    else
    {
        ::CheckMenuRadioItem(hMenu, ID_INPUTH, ID_INPUTV, ID_INPUTH, MF_BYCOMMAND);
    }

    if (xg_bCharFeed)
    {
        ::CheckMenuItem(hMenu, ID_CHARFEED, MF_BYCOMMAND | MF_CHECKED);
    }
    else
    {
        ::CheckMenuItem(hMenu, ID_CHARFEED, MF_BYCOMMAND | MF_UNCHECKED);
    }

    switch (xg_imode) {
    case xg_im_KANA:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_RUSSIAINPUT, ID_KANAINPUT, MF_BYCOMMAND);
        break;

    case xg_im_ABC:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_RUSSIAINPUT, ID_ABCINPUT, MF_BYCOMMAND);
        break;

    case xg_im_HANGUL:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_RUSSIAINPUT, ID_HANGULINPUT, MF_BYCOMMAND);
        break;

    case xg_im_KANJI:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_RUSSIAINPUT, ID_KANJIINPUT, MF_BYCOMMAND);
        break;

    case xg_im_RUSSIA:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_RUSSIAINPUT, ID_RUSSIAINPUT, MF_BYCOMMAND);
        break;
    }

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

    if (xg_vMarks.empty()) {
        ::EnableMenuItem(hMenu, ID_KILLMARKS, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGE, MF_BYCOMMAND | MF_GRAYED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGESIZED, MF_BYCOMMAND | MF_GRAYED);
    } else {
        ::EnableMenuItem(hMenu, ID_KILLMARKS, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGE, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORDASIMAGESIZED, MF_BYCOMMAND | MF_ENABLED);
    }

    if (s_bShowStatusBar) {
        ::CheckMenuItem(hMenu, ID_STATUS, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_STATUS, MF_BYCOMMAND | MF_UNCHECKED);
    }

    if (xg_hwndInputPalette) {
        ::CheckMenuItem(hMenu, ID_PALETTE, MF_BYCOMMAND | MF_CHECKED);
        ::CheckMenuItem(hMenu, ID_PALETTE2, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_PALETTE, MF_BYCOMMAND | MF_UNCHECKED);
        ::CheckMenuItem(hMenu, ID_PALETTE2, MF_BYCOMMAND | MF_UNCHECKED);
    }

    if (s_bShowToolBar) {
        ::CheckMenuItem(hMenu, ID_TOOLBAR, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_TOOLBAR, MF_BYCOMMAND | MF_UNCHECKED);
    }

    if (xg_hHintsWnd) {
        ::CheckMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_BYCOMMAND | MF_UNCHECKED);
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
    RECT rc;
    GetClientRect(hwnd, &rc);

    INT anWidth[] = { rc.right - 200, rc.right - 100, rc.right };
    WCHAR szText[64];

    SendMessageW(xg_hStatusBar, SB_SETPARTS, 3, (LPARAM)anWidth);

    std::wstring str;
    if (xg_bTateInput) {
        str = XgLoadStringDx1(1184);
    } else {
        str = XgLoadStringDx1(1183);
    }
    str += L" - ";

    switch (xg_imode) {
    case xg_im_ABC: str += XgLoadStringDx1(1172); break;
    case xg_im_KANA: str += XgLoadStringDx1(1180); break;
    case xg_im_KANJI: str += XgLoadStringDx1(1181); break;
    case xg_im_RUSSIA: str += XgLoadStringDx1(1185); break;
    default:
        break;
    }

    if (xg_bCharFeed) {
        str += L" - ";
        str += XgLoadStringDx1(1182);
    }

    SendMessageW(xg_hStatusBar, SB_SETTEXT, 0, (LPARAM)str.c_str());

    wsprintfW(szText, L"(%u, %u)", xg_caret_pos.m_j + 1, xg_caret_pos.m_i + 1);
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 1, (LPARAM)szText);

    wsprintfW(szText, L"%u x %u", xg_nCols, xg_nRows);
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
    ::SetDlgItemTextW(hwnd, edt1, xg_szCellFont.data());
    ::SetDlgItemTextW(hwnd, edt2, xg_szSmallFont.data());
    ::SetDlgItemTextW(hwnd, edt3, xg_szUIFont.data());

    // ツールバーを表示するか？
    ::CheckDlgButton(hwnd, chx1,
        (s_bShowToolBar ? BST_CHECKED : BST_UNCHECKED));
    // 太枠をつけるか？
    ::CheckDlgButton(hwnd, chx2,
        (xg_bAddThickFrame ? BST_CHECKED : BST_UNCHECKED));
    // 二重マスに枠をつけるか？
    ::CheckDlgButton(hwnd, chx3,
        (xg_bDrawFrameForMarkedCell ? BST_CHECKED : BST_UNCHECKED));

    
    return TRUE;
}

// [設定]ダイアログで[OK]ボタンを押された。
void SettingsDlg_OnOK(HWND hwnd)
{
    // 辞書ファイルの保存モードを取得する。
    //s_nDictSaveMode = 
    //    static_cast<int>(::SendDlgItemMessageW(hwnd, cmb1, CB_GETCURSEL, 0, 0));
    s_nDictSaveMode = 2;

    // フォント名を取得する。
    std::array<WCHAR,LF_FACESIZE> szName;
    ::GetDlgItemTextW(hwnd, edt1, szName.data(), LF_FACESIZE);
    ::lstrcpynW(xg_szCellFont.data(), szName.data(), LF_FACESIZE);
    ::GetDlgItemTextW(hwnd, edt2, szName.data(), LF_FACESIZE);
    ::lstrcpynW(xg_szSmallFont.data(), szName.data(), LF_FACESIZE);
    ::GetDlgItemTextW(hwnd, edt3, szName.data(), LF_FACESIZE);
    ::lstrcpynW(xg_szUIFont.data(), szName.data(), LF_FACESIZE);

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
        std::array<WCHAR,LF_FACESIZE> szData;
        ::lstrcpynW(szData.data(), xg_szUIFont.data(), LF_FACESIZE);
        LPWSTR pch = wcsrchr(szData.data(), L',');
        if (pch) {
            *pch++ = 0;
            std::wstring name(szData.data());
            std::wstring size(pch);
            xg_str_trim(name);
            xg_str_trim(size);

            lstrcpynW(s_lf.lfFaceName, name.data(), LF_FACESIZE);

            HDC hdc = ::CreateCompatibleDC(NULL);
            int point_size = _wtoi(size.data());
            s_lf.lfHeight = -MulDiv(point_size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ::DeleteDC(hdc);
        } else {
            std::wstring name(szData.data());
            xg_str_trim(name);

            ::lstrcpynW(s_lf.lfFaceName, name.data(), LF_FACESIZE);
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

    std::array<WCHAR,128> szData;
    ::wsprintfW(szData.data(), L"%s, %upt", plf->lfFaceName, point_size);
    ::SetDlgItemTextW(hwnd, edt3, szData.data());
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
                   CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS |
                   CF_FIXEDPITCHONLY;
        ::lstrcpynW(lf.lfFaceName, xg_szCellFont.data(), LF_FACESIZE);
        lf.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
        if (::ChooseFontW(&cf)) {
            // 取得したフォントをダイアログへ格納する。
            ::SetDlgItemTextW(hwnd, edt1, lf.lfFaceName);
        }
        break;

    case 1:
        cf.Flags = CF_NOSCRIPTSEL | CF_NOVERTFONTS | CF_SCALABLEONLY |
                   CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
        ::lstrcpynW(lf.lfFaceName, xg_szSmallFont.data(), LF_FACESIZE);
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
        }
        break;
    }
    return 0;
}

// 設定。
void MainWnd_OnSettings(HWND hwnd)
{
    XgDestroyCandsWnd();
    ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(7), hwnd, XgSettingsDlgProc);
}

// 設定を消去する。
void MainWnd_OnEraseSettings(HWND hwnd)
{
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // 消去するのか確認。
    if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(76), XgLoadStringDx2(2),
                            MB_ICONWARNING | MB_YESNO) != IDYES)
    {
        return;
    }

    // 設定を消去する。
    bool bSuccess = XgEraseSettings();

    // 初期化する。
    XgLoadSettings();
    xg_bSolved = false;
    xg_bHintsAdded = false;
    xg_bShowAnswer = false;
    xg_nRows = s_nRows;
    xg_nCols = s_nCols;
    xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
    xg_solution.ResetAndSetSize(xg_nRows, xg_nCols);
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

    if (bSuccess) {
        // メッセージを表示する。
        XgCenterMessageBoxW(hwnd,
            XgLoadStringDx1(74), XgLoadStringDx2(2),
            MB_ICONINFORMATION);
    } else {
        // メッセージを表示する。
        XgCenterMessageBoxW(hwnd,
            XgLoadStringDx1(75), XgLoadStringDx2(2),
            MB_ICONINFORMATION);
    }
}

// 辞書を読み込む。
extern "C"
INT_PTR CALLBACK
XgLoadDictDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    int i;
    std::array<WCHAR,MAX_PATH> szFile, szTarget;
    std::wstring strFile;
    COMBOBOXEXITEMW item;
    OPENFILENAMEW ofn;
    HDROP hDrop;

    switch (uMsg) {
    case WM_INITDIALOG:
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 辞書ファイルのパス名のベクターをコンボボックスに設定する。
        for (const auto& dict_file : xg_dict_files) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            ::lstrcpyW(szFile.data(), dict_file.data());
            item.pszText = szFile.data();
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // ドラッグ＆ドロップを受け付ける。
        ::DragAcceptFiles(hwnd, TRUE);
        return TRUE;

    case WM_DROPFILES:
        // ドロップされたファイルのパス名を取得する。
        hDrop = reinterpret_cast<HDROP>(wParam);
        ::DragQueryFileW(hDrop, 0, szFile.data(), MAX_PATH);
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), s_szShellLinkDotExt) == 0) {
            if (!XgGetPathOfShortcutW(szFile.data(), szTarget.data())) {
                ::MessageBeep(0xFFFFFFFF);
                break;
            }
            ::lstrcpyW(szFile.data(), szTarget.data());
        }

        // 同じ項目がすでにあれば、削除する。
        i = static_cast<int>(::SendDlgItemMessageW(hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                                                 reinterpret_cast<LPARAM>(szFile.data())));
        if (i != CB_ERR) {
            ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
        }
        // コンボボックスの最初に挿入する。
        item.mask = CBEIF_TEXT;
        item.iItem = 0;
        item.pszText = szFile.data();
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
            ::GetDlgItemTextW(hwnd, cmb1, szFile.data(), 
                            static_cast<int>(szFile.size()));
            strFile = szFile.data();
            xg_str_trim(strFile);
            // 正しく読み込めるか？
            if (XgLoadDictFile(strFile.data())) {
                // 読み込めた。
                // 読み込んだファイル項目を一番上にする。
                auto end = xg_dict_files.end();
                for (auto it = xg_dict_files.begin(); it != end; it++) {
                    if (_wcsicmp((*it).data(), strFile.data()) == 0) {
                        xg_dict_files.erase(it);
                        break;
                    }
                }
                xg_dict_files.emplace_front(strFile);
                // ダイアログを閉じる。
                ::EndDialog(hwnd, IDOK);
            } else {
                // 読み込めなかったのでエラーを表示する。
                ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
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
            ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(50));
            szFile[0] = 0;
            ofn.lpstrFile = szFile.data();
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrTitle = XgLoadStringDx1(19);
            ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
            ofn.lpstrDefExt = L"dic";
            if (::GetOpenFileNameW(&ofn))
            {
                // コンボボックスにテキストを設定。
                ::SetDlgItemTextW(hwnd, cmb1, szFile.data());
            }
            break;
        }
    }
    return 0;
}

// [ヘッダーと備考欄]ダイアログ。
extern "C"
INT_PTR CALLBACK
XgHeaderNotesDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    std::array<WCHAR,512> sz;
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
        psz = XgLoadStringDx1(83);
        if (str.find(psz) == 0) {
            str = str.substr(::lstrlenW(psz));
        }
        ::SetDlgItemTextW(hwnd, edt2, str.data());
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            // ヘッダーを取得する。
            ::GetDlgItemTextW(hwnd, edt1, sz.data(), static_cast<int>(sz.size()));
            str = sz.data();
            xg_str_trim(str);
            xg_strHeader = str;

            // 備考欄を取得する。
            ::GetDlgItemTextW(hwnd, edt2, sz.data(), static_cast<int>(sz.size()));
            str = sz.data();
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
        xg_solution.DoNumbering();
        xg_solution.GetHintsStr(xg_strHints, 2, true);
        if (!XgParseHintsStr(hwnd, xg_strHints)) {
            xg_strHints.clear();
        }
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
    std::wstring query = XgLoadStringDx1(1171);
    std::wstring raw = str;
    switch (xg_imode)
    {
    case xg_im_ABC:
        raw += L" ";
        raw += XgLoadStringDx2(1172);
        break;
    case xg_im_KANA:
    case xg_im_KANJI:
        raw += L" ";
        raw += XgLoadStringDx2(1174);
        break;
    case xg_im_HANGUL:
    case xg_im_RUSSIA:
        break;
    default:
        break;
    }
    std::wstring encoded = URL_encode(raw.c_str());
    query += encoded;

    ::ShellExecuteW(hwnd, NULL, query.c_str(), NULL, NULL, SW_SHOWNORMAL);
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
    int lo, hi;
    std::wstring pattern;
    if (bTate) {
        lo = hi = xg_caret_pos.m_i;
        while (lo > 0) {
            if (pxword->GetAt(lo - 1, xg_caret_pos.m_j) != ZEN_BLACK)
                --lo;
            else
                break;
        }
        while (hi + 1 < xg_nRows) {
            if (pxword->GetAt(hi + 1, xg_caret_pos.m_j) != ZEN_BLACK)
                ++hi;
            else
                break;
        }

        for (int i = lo; i <= hi; ++i) {
            pattern += pxword->GetAt(i, xg_caret_pos.m_j);
        }
    } else {
        lo = hi = xg_caret_pos.m_j;
        while (lo > 0) {
            if (pxword->GetAt(xg_caret_pos.m_i, lo - 1) != ZEN_BLACK)
                --lo;
            else
                break;
        }
        while (hi + 1 < xg_nCols) {
            if (pxword->GetAt(xg_caret_pos.m_i, hi + 1) != ZEN_BLACK)
                ++hi;
            else
                break;
        }

        for (int j = lo; j <= hi; ++j) {
            pattern += pxword->GetAt(xg_caret_pos.m_i, j);
        }
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
    default:
        break;
    }
    xg_bCharFeed = bOldFeed;

    return bOK;
}

// コマンドを実行する。
void __fastcall MainWnd_OnCommand(HWND hwnd, int id, HWND /*hwndCtl*/, UINT /*codeNotify*/)
{
    std::array<WCHAR,MAX_PATH> sz;
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
        ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(10), hwnd, XgHeaderNotesDlgProc);
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
        ::DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(9), hwnd, XgLoadDictDlgProc);
        break;

#ifndef MZC_NO_SHAREWARE
    case ID_STARTSHAREWARE:
        // シェアウェアを開始する。
        if (!g_shareware.Start(hwnd)) {
            // 開始できないときは、ウィンドウを破棄する。
            ::DestroyWindow(hwnd);
        }
        XgEnsureCaretVisible(hwnd);
        break;

    case ID_REGISTERKEY:
        // シェアウェアの登録を促す。
        g_shareware.UrgeRegister(hwnd);
        break;
#endif  // ndef MZC_NO_SHAREWARE

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
            if (XgOnGenerate(hwnd, false)) {
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
            if (XgOnGenerate(hwnd, true)) {
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

    case ID_GENERATEREPEATEDLY:     // 問題を連続自動生成する
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 連続生成ダイアログ。
            XgOnGenerateRepeatedly(hwnd);

            sa1->Apply();
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_OPEN:   // ファイルを開く。
        // ユーザーにファイルの場所を問い合わせる。
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(51));
        sz[0] = 0;
        ofn.lpstrFile = sz.data();
        ofn.nMaxFile = static_cast<DWORD>(sz.size());
        ofn.lpstrTitle = XgLoadStringDx1(20);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
            OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = L"xwd";
        if (::GetOpenFileNameW(&ofn)) {
            // JSON形式か？
            bool is_json = false;
            bool is_builder = false;
            if (::lstrcmpiW(PathFindExtensionW(sz.data()), L".xwj") == 0 ||
                ::lstrcmpiW(PathFindExtensionW(sz.data()), L".json") == 0)
            {
                is_json = true;
            }
            if (::lstrcmpiW(PathFindExtensionW(sz.data()), L".crp") == 0 ||
                ::lstrcmpiW(PathFindExtensionW(sz.data()), L".crx") == 0)
            {
                is_builder = true;
            }
            // 開く。
            if (is_builder) {
                if (!XgDoLoadBuilderFile(hwnd, sz.data())) {
                    // 失敗。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
                } else {
                    // 成功。
                    xg_ubUndoBuffer.Empty();
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            } else {
                if (!XgDoLoad(hwnd, sz.data(), is_json)) {
                    // 失敗。
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
                } else {
                    // 成功。
                    xg_ubUndoBuffer.Empty();
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            }
        }
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        break;

    case ID_SAVEAS: // ファイルを保存する。
        if (xg_dict_files.empty()) {
            XgLoadSettings();
        }
        // ユーザーにファイルの場所を問い合わせる準備。
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
        ofn.hwndOwner = hwnd;
        ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(52));
        ::lstrcpynW(sz.data(), xg_strFileName.data(), static_cast<int>(sz.size()));
        ofn.lpstrFile = sz.data();
        ofn.nMaxFile = static_cast<DWORD>(sz.size());
        ofn.lpstrTitle = XgLoadStringDx1(21);
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT |
            OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
        // 拡張子に応じて処理を変える。
        if (::lstrcmpW(sz.data(), L"") == 0) {
            if (xg_bSaveAsJsonFile) {
                ofn.nFilterIndex = 2;
                ofn.lpstrDefExt = L"xwj";
            } else {
                ofn.nFilterIndex = 1;
                ofn.lpstrDefExt = L"xwd";
            }
        } else {
            LPTSTR pchDotExt = PathFindExtensionW(sz.data());
            if (::lstrcmpiW(pchDotExt, L".xwj") == 0) {
                ofn.nFilterIndex = 2;
                ofn.lpstrDefExt = L"xwj";
            } else if (::lstrcmpiW(pchDotExt, L".json") == 0) {
                ofn.nFilterIndex = 2;
                ofn.lpstrDefExt = L"json";
            } else if (::lstrcmpiW(pchDotExt, L".crp") == 0 ||
                       ::lstrcmpiW(pchDotExt, L".crx") == 0)
            {
                *pchDotExt = 0;
                ofn.nFilterIndex = 1;
                ofn.lpstrDefExt = L"xwd";
            } else {
                ofn.nFilterIndex = 1;
                ofn.lpstrDefExt = L"xwd";
            }
        }
        // ユーザーにファイルの場所を問い合わせる。
        if (::GetSaveFileNameW(&ofn)) {
            bool dict_updated = false;
            if (XgAreHintsModified()) {
                // ヒントに更新があった。ヒントを更新する。
                XgUpdateHintsData();
                // ヒントに従って辞書を更新する。
                XgUpdateDictData();
                // フラグを立てる。
                dict_updated = true;
            } else if (XgIsDictUpdated()) {
                dict_updated = true;
            }

            // JSON形式で保存するか？
            xg_bSaveAsJsonFile = (ofn.nFilterIndex == 2);

            // 保存する。
            if (!XgDoSave(hwnd, sz.data(), xg_bSaveAsJsonFile)) {
                // 保存に失敗。
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
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
            {
                XgOnGenerateBlacks(hwnd, false);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_GENERATEBLACKSSYMMETRIC2: // 黒マスパターンを生成（点対称）。
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                XgOnGenerateBlacks(hwnd, true);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_SOLVE:  // 解を求める。
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                // 解を求める。
                XgOnSolveAddBlack(hwnd);
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
                XgOnSolveNoAddBlack(hwnd);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // ツールバーのUIを更新する。
            XgUpdateToolBarUI(hwnd);
        }
        break;

    case ID_SOLVEREPEATEDLY:    // 連続で解を求める
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 連続で解を求める。
            XgOnSolveRepeatedly(hwnd);
            sa1->Apply();
        }
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
            XgOnSolveRepeatedlyNoAddBlack(hwnd);
            sa1->Apply();
        }
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

    case ID_HANGULINPUT: // ハングル入力モード。
        XgSetInputMode(hwnd, xg_im_HANGUL);
        break;

    case ID_KANJIINPUT: // 漢字入力モード。
        XgSetInputMode(hwnd, xg_im_KANJI);
        break;

    case ID_RUSSIAINPUT: // ロシア入力モード。
        XgSetInputMode(hwnd, xg_im_RUSSIA);
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

    case ID_SENDMAIL:    // メールを送る。
        {
            std::wstring strTitle, strBody;
            HINSTANCE hInst;

            XgGetMailTitle(hwnd, strTitle);
            XgGetMailBody(hwnd, strBody);
            XgUrlEncodeStr(strTitle);
            XgUrlEncodeStr(strBody);
            #if 1
                std::wstring strURL;
                strURL += L"mailto:";
                strURL += L"?subject=";
                strURL += strTitle;
                strURL += L"&body=";
                strURL += strBody;
                hInst = ::ShellExecuteW(hwnd, NULL,
                    strURL.data(), NULL, NULL, SW_SHOWNORMAL);
            #else
                std::string utf8Title = XgUnicodeToUtf8(strTitle.data());
                std::string utf8Body = XgUnicodeToUtf8(strBody.data());
                std::string encodedTitle = XgUrlEncode(utf8Title);
                std::string encodedBody = XgUrlEncode(utf8Body);
                std::string strURL;
                strURL += "mailto:";
                strURL += "?subject=";
                strURL += encodedTitle;
                strURL += "&body=";
                strURL += encodedBody;
                std::wstring wide = XgAnsiToUnicode(strURL);
                hInst = ::ShellExecuteW(hwnd, NULL,
                    wide.data(), NULL, NULL, SW_SHOWNORMAL);
            #endif
            if (reinterpret_cast<INT_PTR>(hInst) <= 32) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(86),
                    XgLoadStringDx2(2), MB_ICONERROR | MB_OK);
            }
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
        ShellExecuteW(hwnd, NULL, XgLoadStringDx1(1175), NULL, NULL, SW_SHOWNORMAL);
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
        xg_nZoomRate = 100;
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateScrollInfo(hwnd, x, y);
        XgUpdateImage(hwnd, x, y);
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
    std::array<WCHAR,MAX_PATH> szFile, szTarget;
    ::DragQueryFileW(hDrop, 0, szFile.data(), static_cast<UINT>(szFile.size()));
    ::DragFinish(hDrop);

    // ショートカットだった場合は、ターゲットのパスを取得する。
    if (XgGetPathOfShortcutW(szFile.data(), szTarget.data()))
        ::lstrcpyW(szFile.data(), szTarget.data());

    // 拡張子を取得する。
    LPWSTR pch = PathFindExtensionW(szFile.data());

    if (::lstrcmpiW(pch, L".xwd") == 0) {
        // 拡張子が.xwdだった。ファイルを開く。
        if (!XgDoLoad(hwnd, szFile.data(), false)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
        } else {
            xg_caret_pos.clear();
            // イメージを更新する。
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
    } else if (::lstrcmpiW(pch, L".xwj") == 0 || lstrcmpiW(pch, L".json") == 0) {
        // 拡張子が.xwjか.jsonだった。ファイルを開く。
        if (!XgDoLoad(hwnd, szFile.data(), true)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
        } else {
            xg_caret_pos.clear();
            // イメージを更新する。
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
    } else if (::lstrcmpiW(pch, L".crp") == 0 || ::lstrcmpiW(pch, L".crx") == 0) {
        if (!XgDoLoadBuilderFile(hwnd, szFile.data())) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
        } else {
            xg_caret_pos.clear();
            // イメージを更新する。
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
        }
    } else {
        ::MessageBeep(0xFFFFFFFF);
    }

    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
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

    xg_nRows = s_nRows;
    xg_nCols = s_nCols;

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
    const int c_nButtons = 18;
    static const TBBUTTON atbb[c_nButtons] = {
        {0, ID_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {1, ID_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {2, ID_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {3, ID_GENERATE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {4, ID_GENERATEREPEATEDLY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {5, ID_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {6, ID_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {7, ID_SOLVE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {8, ID_SOLVENOADDBLACK, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {9, ID_SOLVEREPEATEDLY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {10, ID_SOLVEREPEATEDLYNOADDBLACK, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {11, ID_PRINTPROBLEM, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {12, ID_PRINTANSWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
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
    ::SendMessageW(xg_hToolBar, TB_SETIMAGELIST, 0,
        reinterpret_cast<LPARAM>(xg_hImageList));
    ::SendMessageW(xg_hToolBar, TB_SETDISABLEDIMAGELIST, 0,
        reinterpret_cast<LPARAM>(xg_hGrayedImageList));
    ::SendMessageW(xg_hToolBar, TB_ADDBUTTONS, c_nButtons,
        reinterpret_cast<LPARAM>(atbb));
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
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(55), XgLoadStringDx2(2),
                          MB_ICONWARNING | MB_OK);
        s_bOldNotice = true;
    }

    // クロスワードを初期化する。
    xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);

    // 辞書ファイルを読み込む。
    if (xg_dict_files.size())
        XgLoadDictFile(xg_dict_files[0].data());

    // ファイルドロップを受け付ける。
    ::DragAcceptFiles(hwnd, TRUE);

    // イメージを更新する。
    XgUpdateImage(hwnd, 0, 0);

    int argc;
    LPWSTR *wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argc >= 2) {
        std::array<WCHAR,MAX_PATH> szFile, szTarget;
        ::lstrcpynW(szFile.data(), wargv[1], static_cast<int>(szFile.size()));

        // コマンドライン引数があれば、それを開く。
        bool bSuccess = true;
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), s_szShellLinkDotExt) == 0)
        {
            // ショートカットだった場合は、ターゲットのパスを取得する。
            if (XgGetPathOfShortcutW(szFile.data(), szTarget.data())) {
                ::lstrcpynW(szFile.data(), szTarget.data(), MAX_PATH);
            } else {
                bSuccess = false;
                MessageBeep(0xFFFFFFFF);
            }
        }
        bool is_json = false;
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), L".xwj") == 0 ||
            ::lstrcmpiW(PathFindExtensionW(szFile.data()), L".json") == 0)
        {
            is_json = true;
        }
        bool is_builder = false;
        if (::lstrcmpiW(PathFindExtensionW(szFile.data()), L".crp") == 0 ||
            ::lstrcmpiW(PathFindExtensionW(szFile.data()), L".crx") == 0)
        {
            is_builder = true;
        }
        if (bSuccess) {
            if (is_builder) {
                if (!XgDoLoadBuilderFile(hwnd, szFile.data())) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
                } else {
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            } else {
                if (!XgDoLoad(hwnd, szFile.data(), is_json)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(3), nullptr, MB_ICONERROR);
                } else {
                    xg_caret_pos.clear();
                    // イメージを更新する。
                    XgUpdateImage(hwnd, 0, 0);
                }
            }
        }
    }
    GlobalFree(wargv);

#ifndef MZC_NO_SHAREWARE
    // シェアウェアを開始する。
    ::PostMessageW(hwnd, WM_COMMAND, ID_STARTSHAREWARE, 0);
#endif

    ::PostMessageW(hwnd, WM_SIZE, 0, 0);

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

// 右クリックされた。
void
MainWnd_OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    MainWnd_OnLButtonUp(hwnd, x, y, keyFlags);

    HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(2));
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
        pttt->lpszText = MAKEINTRESOURCEW(pttt->hdr.idFrom + 1000);
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
        std::array<WCHAR,64> label;
        ::wsprintfW(label.data(), XgLoadStringDx1(24), 100);
        std::wstring strLabel = label.data();
        ::SelectObject(hdc, ::GetStockObject(SYSTEM_FIXED_FONT));
        ::GetTextExtentPoint32W(hdc, strLabel.data(), 
                                static_cast<int>(strLabel.size()), &size1);
        ::SelectObject(hdc, xg_hHintsUIFont);
        ::GetTextExtentPoint32W(hdc, strLabel.data(), 
                                static_cast<int>(strLabel.size()), &size2);
        ::DeleteDC(hdc);
    }

    std::array<WCHAR,512> szText;
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
            ::GetWindowTextW(xg_ahwndTateEdits[i], szText.data(),
                             static_cast<int>(szText.size()));
            MRect rcCtrl(MPoint(size2.cx, y),
                         MSize(rcClient.Width() - size2.cx - 8, 0));
            if (szText[0] == 0) {
                szText[0] = L' ';
                szText[1] = 0;
            }
            ::DrawTextW(hdc, szText.data(), -1, &rcCtrl,
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
            ::GetWindowTextW(xg_ahwndYokoEdits[i], szText.data(),
                             static_cast<int>(szText.size()));
            MRect rcCtrl(MPoint(size2.cx, y),
                         MSize(rcClient.Width() - size2.cx - 8, 0));
            if (szText[0] == 0) {
                szText[0] = L' ';
                szText[1] = 0;
            }
            ::DrawTextW(hdc, szText.data(), -1, &rcCtrl,
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
    xg_svHintsScrollView.EnsureCtrlVisible(::GetFocus(), false);
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
                    // ヒントに従って辞書を更新する。
                    XgUpdateDictData();
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
                // ヒントに従って辞書を更新する。
                XgUpdateDictData();
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
        TEXT("STATIC"), XgLoadStringDx1(22),
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
        TEXT("STATIC"), XgLoadStringDx1(23),
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
    std::array<WCHAR,256> sz;
    for (const auto& hint : xg_vecTateHints) {
        ::wsprintfW(sz.data(), XgLoadStringDx1(24), hint.m_number);
        hwndCtrl = ::CreateWindowW(
            TEXT("STATIC"), sz.data(),
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
        ::wsprintfW(sz.data(), XgLoadStringDx1(25), hint.m_number);
        hwndCtrl = ::CreateWindowW(
            TEXT("STATIC"), sz.data(),
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

// ヒントウィンドウが横にスクロールされた。
void HintsWnd_OnHScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
{
    xg_svHintsScrollView.HScroll(code, pos);
}

// ヒントウィンドウが縦にスクロールされた。
void HintsWnd_OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
{
    xg_svHintsScrollView.VScroll(code, pos);
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
        std::array<WCHAR,512> sz;
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            if (::SendMessageW(xg_ahwndTateEdits[i], EM_GETMODIFY, 0, 0)) {
                updated = true;
                ::GetWindowTextW(xg_ahwndTateEdits[i], sz.data(), 
                                 static_cast<int>(sz.size()));
                xg_vecTateHints[i].m_strHint = sz.data();
                ::SendMessageW(xg_ahwndTateEdits[i], EM_SETMODIFY, FALSE, 0);
            }
        }
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            if (::SendMessageW(xg_ahwndYokoEdits[i], EM_GETMODIFY, 0, 0)) {
                updated = true;
                ::GetWindowTextW(xg_ahwndYokoEdits[i], sz.data(), 
                                 static_cast<int>(sz.size()));
                xg_vecYokoHints[i].m_strHint = sz.data();
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
    xg_svHintsScrollView.ResetScrollPos();

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

// ヒントウィンドウがアクティブ化された。
void HintsWnd_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state == WA_ACTIVE) {
        HWND hwndFocus = ::GetFocus();
        std::array<WCHAR,64> sz;
        ::GetClassNameW(hwndFocus, sz.data(), static_cast<int>(sz.size()));
        if (hwndFocus == NULL || lstrcmpiW(sz.data(), L"EDIT") != 0) {
            if (xg_ahwndTateEdits.size()) {
                ::SetFocus(xg_ahwndTateEdits[0]);
            }
        }
    }
}

// ヒント ウィンドウのサイズを制限する。
void HintsWnd_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
    lpMinMaxInfo->ptMinTrackSize.x = 256;
    lpMinMaxInfo->ptMinTrackSize.y = 128;
}

// ヒント ウィンドウのウィンドウ プロシージャー。
extern "C"
LRESULT CALLBACK
XgHintsWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, HintsWnd_OnCreate);
    HANDLE_MSG(hWnd, WM_SIZE, HintsWnd_OnSize);
    HANDLE_MSG(hWnd, WM_HSCROLL, HintsWnd_OnHScroll);
    HANDLE_MSG(hWnd, WM_VSCROLL, HintsWnd_OnVScroll);
    HANDLE_MSG(hWnd, WM_KEYDOWN, HintsWnd_OnKey);
    HANDLE_MSG(hWnd, WM_KEYUP, HintsWnd_OnKey);
    HANDLE_MSG(hWnd, WM_DESTROY, HintsWnd_OnDestroy);
    HANDLE_MSG(hWnd, WM_COMMAND, HintsWnd_OnCommand);
    HANDLE_MSG(hWnd, WM_MOUSEWHEEL, HintsWnd_OnMouseWheel);
    HANDLE_MSG(hWnd, WM_ACTIVATE, HintsWnd_OnActivate);
    HANDLE_MSG(hWnd, WM_GETMINMAXINFO, HintsWnd_OnGetMinMaxInfo);

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
        s_pszHintsWndClass, XgLoadStringDx1(70), style,
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
    std::array<WCHAR,64> szClassName;

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
                ::GetClassNameW(hWnd, szClassName.data(), 64);
                if (::lstrcmpiW(szClassName.data(), L"EDIT") == 0) {
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
std::vector<XG_Candidate>   xg_vecCandidates;

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
            std::wstring strLabel = xg_vecCandidates[i];

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
    xg_svCandsScrollView.ResetScrollPos();

    xg_svCandsScrollView.SetParent(hwnd);
    xg_svCandsScrollView.ShowScrollBars(FALSE, TRUE);

    HWND hwndCtrl;
    XG_CandsButtonData *data;
    for (size_t i = 0; i < xg_vecCandidates.size(); ++i) {
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

// 候補ウィンドウが横にスクロールされた。
void CandsWnd_OnHScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
{
    xg_svCandsScrollView.HScroll(code, pos);
}

// 候補ウィンドウが縦にスクロールされた。
void CandsWnd_OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
{
    xg_svCandsScrollView.VScroll(code, pos);
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
    xg_svCandsScrollView.ResetScrollPos();

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
            xword.SetAt(k, xg_jCandPos, strCand[m]);
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
            xword.SetAt(xg_iCandPos, k, strCand[m]);
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

void CandsWnd_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state == WA_ACTIVE) {
        HWND hwndFocus = ::GetFocus();
        std::array<WCHAR,64> sz;
        ::GetClassNameW(hwndFocus, sz.data(), 64);
        if (hwndFocus == NULL || lstrcmpiW(sz.data(), L"BUTTON") != 0) {
            if (xg_ahwndCandButtons.size()) {
                ::SetFocus(xg_ahwndCandButtons[0]);
            }
        }
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
            ::DrawTextW(hdc, XgLoadStringDx1(97), -1,
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
    HANDLE_MSG(hwnd, WM_HSCROLL, CandsWnd_OnHScroll);
    HANDLE_MSG(hwnd, WM_VSCROLL, CandsWnd_OnVScroll);
    HANDLE_MSG(hwnd, WM_KEYDOWN, CandsWnd_OnKey);
    HANDLE_MSG(hwnd, WM_KEYUP, CandsWnd_OnKey);
    HANDLE_MSG(hwnd, WM_DESTROY, CandsWnd_OnDestroy);
    HANDLE_MSG(hwnd, WM_COMMAND, CandsWnd_OnCommand);
    HANDLE_MSG(hwnd, WM_MOUSEWHEEL, CandsWnd_OnMouseWheel);
    HANDLE_MSG(hwnd, WM_ACTIVATE, CandsWnd_OnActivate);
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
        s_pszCandsWndClass, XgLoadStringDx1(96), style,
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
    std::vector<std::wstring> cands;
    XgGetCandidatesAddBlack(cands, pattern, nSkip, left_black, right_black);

    // 仮に適用して、正当かどうか確かめ、正当なものだけを
    // 最終的な候補とする。
    xg_vecCandidates.clear();
    for (const auto& cand : cands) {
        XG_Board xword(xg_xword);
        XgApplyCandidate(xword, cand);
        if (xword.CornerBlack() || xword.DoubleBlack() ||
            xword.TriBlackArround() || xword.DividedByBlack())
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
    // 多重起動禁止ミューテックスを作成。
    s_hMutex = ::CreateMutexW(NULL, FALSE, L"XWordGiver Mutex");
    if (::WaitForSingleObject(s_hMutex, 500) != WAIT_OBJECT_0) {
        // ミューテックスを閉じる。
        ::CloseHandle(s_hMutex);
        // 多重起動禁止メッセージ。
        XgCenterMessageBoxW(NULL, XgLoadStringDx1(66), XgLoadStringDx2(2),
                            MB_ICONERROR);
        return 999;
    }

    // アプリのインスタンスを保存する。
    xg_hInstance = hInstance;

    // 設定を読み込む。
    XgLoadSettings();

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
    s_hAccel = ::LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(1));
    if (s_hAccel == nullptr) {
        // ミューテックスを解放。
        ::ReleaseMutex(s_hMutex);
        ::CloseHandle(s_hMutex);
        // アクセラレータ作成失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(12), nullptr, MB_ICONERROR);
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
    wcx.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));
    wcx.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_3DFACE + 1));
    wcx.lpszMenuName = MAKEINTRESOURCEW(1);
    wcx.lpszClassName = s_pszMainWndClass;
    wcx.hIconSm = nullptr;
    if (!::RegisterClassExW(&wcx)) {
        // ミューテックスを解放。
        ::ReleaseMutex(s_hMutex);
        ::CloseHandle(s_hMutex);
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(13), nullptr, MB_ICONERROR);
        return 1;
    }
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = XgHintsWndProc;
    wcx.hIcon = NULL;
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_3DFACE + 1));
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = s_pszHintsWndClass;
    if (!::RegisterClassExW(&wcx)) {
        // ミューテックスを解放。
        ::ReleaseMutex(s_hMutex);
        ::CloseHandle(s_hMutex);
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(13), nullptr, MB_ICONERROR);
        return 1;
    }
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = XgCandsWndProc;
    wcx.hIcon = NULL;
    wcx.hbrBackground = ::CreateSolidBrush(RGB(255, 255, 192));
    wcx.lpszMenuName = NULL;
    wcx.lpszClassName = s_pszCandsWndClass;
    if (!::RegisterClassExW(&wcx)) {
        // ミューテックスを解放。
        ::ReleaseMutex(s_hMutex);
        ::CloseHandle(s_hMutex);
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(13), nullptr, MB_ICONERROR);
        return 1;
    }

    // クリティカルセクションを初期化する。
    ::InitializeCriticalSection(&xg_cs);

    // メインウィンドウを作成する。
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS;
    ::CreateWindowW(s_pszMainWndClass, XgLoadStringDx1(1176), style,
        s_nMainWndX, s_nMainWndY, s_nMainWndCX, s_nMainWndCY,
        nullptr, nullptr, hInstance, nullptr);
    if (xg_hMainWnd == nullptr) {
        // ミューテックスを解放。
        ::ReleaseMutex(s_hMutex);
        ::CloseHandle(s_hMutex);
        // ウィンドウ作成失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(14), nullptr, MB_ICONERROR);
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

    // ミューテックスを解放。
    ::ReleaseMutex(s_hMutex);
    ::CloseHandle(s_hMutex);

    // 設定を保存。
    XgSaveSettings();

    return static_cast<int>(msg.wParam);
}

//////////////////////////////////////////////////////////////////////////////
