//////////////////////////////////////////////////////////////////////////////
// GUI.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#define NOMINMAX
#include "XWordGiver.hpp"
#include "GUI.hpp"
#include "XG_UndoBuffer.hpp"    // 「元に戻す」情報。
#include <algorithm>

// ダイアログとウィンドウ。
#include "XG_CancelFromWordsDialog.hpp"
#include "XG_CancelGenBlacksDialog.hpp"
#include "XG_CancelSmartSolveDialog.hpp"
#include "XG_CancelSolveDialog.hpp"
#include "XG_CancelSolveNoAddBlackDialog.hpp"
#include "XG_CandsWnd.hpp"
#include "XG_GenDialog.hpp"
#include "XG_HintsWnd.hpp"
#include "XG_NewDialog.hpp"
#include "XG_NotesDialog.hpp"
#include "XG_PatGenDialog.hpp"
#include "XG_PatternDialog.hpp"
#include "XG_SeqGenDialog.hpp"
#include "XG_SeqPatGenDialog.hpp"
#include "XG_SeqSolveDialog.hpp"
#include "XG_SettingsDialog.hpp"
#include "XG_ThemeDialog.hpp"
#include "XG_WordListDialog.hpp"
#include "XG_RulePresetDialog.hpp"
#include "XG_BoxWindow.hpp"
#include "XG_CanvasWindow.hpp"
#include "XG_PictureBoxDialog.hpp"
#include "XG_TextBoxDialog.hpp"
#include "XG_BoxWindow.hpp"
#include "XG_CanvasWindow.hpp"

// ボックスをすべて削除する。
void XgDeleteBoxes(void)
{
    for (auto& box : xg_boxes) {
        DestroyWindow(*box);
    }
    xg_boxes.clear();
}

// ボックスJSONを読み込む。
BOOL XgDoLoadBoxJson(const json& boxes)
{
    try
    {
        for (size_t i = 0; i < boxes.size(); ++i) {
            auto& box = boxes[i];
            if (box["type"] == "pic") {
                auto ptr = new XG_PictureBoxWindow();
                ptr->SetData(0, XgUtf8ToUnicode(box["data0"]));
                ptr->SetData(1, XgUtf8ToUnicode(box["data1"]));
                if (ptr->CreateDx(xg_canvasWnd)) {
                    xg_boxes.emplace_back(ptr);
                    continue;
                } else {
                    delete ptr;
                }
            } else if (box["type"] == "text") {
                auto ptr = new XG_TextBoxWindow();
                ptr->SetData(0, XgUtf8ToUnicode(box["data0"]));
                ptr->SetData(1, XgUtf8ToUnicode(box["data1"]));
                if (ptr->CreateDx(xg_canvasWnd)) {
                    xg_boxes.emplace_back(ptr);
                    continue;
                } else {
                    delete ptr;
                }
            }
        }
    }
    catch(...)
    {
        return FALSE;
    }

    return TRUE;
}

// ボックスJSONを保存。
BOOL XgDoSaveBoxJson(json& j)
{
    try
    {
        for (size_t i = 0; i < xg_boxes.size(); ++i) {
            auto& box = *xg_boxes[i];
            std::wstring type, data0, data1;
            type = box.m_type;
            box.GetData(0, data0);
            box.GetData(1, data1);
            json info;
            info["type"] = XgUnicodeToUtf8(type);
            info["data0"] = XgUnicodeToUtf8(data0);
            info["data1"] = XgUnicodeToUtf8(data1);
            j["boxes"].push_back(info);
        }
    }
    catch(...)
    {
        return FALSE;
    }

    return TRUE;
}

#undef HANDLE_WM_MOUSEWHEEL     // might be wrong
#define HANDLE_WM_MOUSEWHEEL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), \
        (int)(short)HIWORD(wParam), (UINT)(short)LOWORD(wParam)), 0)

#undef FORWARD_WM_MOUSEWHEEL    // might be wrong
#define FORWARD_WM_MOUSEWHEEL(hwnd, xPos, yPos, zDelta, fwKeys, fn) \
    (void)(fn)((hwnd), WM_MOUSEWHEEL, MAKEWPARAM((fwKeys),(zDelta)), MAKELPARAM((xPos),(yPos)))

//////////////////////////////////////////////////////////////////////////////
// global variables

// 入力モード。
XG_InputMode xg_imode = xg_im_KANA;

// インスタンスのハンドル。
HINSTANCE xg_hInstance = nullptr;

// メインウィンドウのハンドル。
HWND xg_hMainWnd = nullptr;

// ヒントウィンドウのハンドル。
HWND xg_hHintsWnd = nullptr;
XG_HintsWnd xg_hints_wnd;

// 候補ウィンドウ。
XG_CandsWnd xg_cands_wnd;

// 入力パレット。
HWND xg_hwndInputPalette = nullptr;

// マスのフォント。
WCHAR xg_szCellFont[LF_FACESIZE] = L"";

// 小さな文字のフォント。
WCHAR xg_szSmallFont[LF_FACESIZE] = L"";

// UIフォント。
WCHAR xg_szUIFont[LF_FACESIZE] = L"";

// スクロールバー。
HWND xg_hSizeGrip = nullptr;

// ツールバーのハンドル。
HWND xg_hToolBar  = nullptr;

// ステータスバーのハンドル
HWND xg_hStatusBar  = nullptr;

// ツールバーのイメージリスト。
HIMAGELIST xg_hImageList = nullptr;
HIMAGELIST xg_hGrayedImageList = nullptr;

// 辞書ファイルの場所（パス）。
std::wstring xg_dict_name;
std::deque<DICT_ENTRY>  xg_dict_files;

// ヒントに追加があったか？
bool xg_bHintsAdded = false;

// JSONファイルとして保存するか？
bool xg_bSaveAsJsonFile = true;

// 太枠をつけるか？
bool xg_bAddThickFrame = true;

// 「元に戻す」ためのバッファ。
XG_UndoBuffer xg_ubUndoBuffer;

// 直前に押したキーを覚えておく。
WCHAR xg_prev_vk = 0;

// 「入力パレット」縦置き？
bool xg_bTateOki = true;

// 表示用に描画するか？（XgGetXWordExtentとXgDrawXWordとXgCreateXWordImageで使う）。
INT xg_nForDisplay = 0;

// ズーム比率(%)。
INT xg_nZoomRate = 100;

// 番号を表示するか？
BOOL xg_bShowNumbering = TRUE;
// キャレットを表示するか？
BOOL xg_bShowCaret = TRUE;

// 二重マス文字。
std::wstring xg_strDoubleFrameLetters;

// 二重マス文字を表示するか？
BOOL xg_bShowDoubleFrameLetters = TRUE;

// 保存先のパスのリスト。
std::deque<std::wstring> xg_dirs_save_to;

// 連続生成の場合、問題を生成する数。
INT xg_nNumberToGenerate = 16;

// マウスの中央ボタンの処理に使う変数。
BOOL xg_bMButtonDragging = FALSE;
POINT xg_ptMButtonDragging;

// ファイルの種類。
XG_FILETYPE xg_nFileType = XG_FILETYPE_XWJ;

// ハイライト情報。
XG_HighLight xg_highlight = { -1, FALSE };

// ボックス。
std::vector<std::unique_ptr<XG_BoxWindow> > xg_boxes;

// キャンバスウィンドウ。
XG_CanvasWindow xg_canvasWnd;

//////////////////////////////////////////////////////////////////////////////
// static variables

// メインウィンドウの位置とサイズ。
static int s_nMainWndX = CW_USEDEFAULT, s_nMainWndY = CW_USEDEFAULT;
static int s_nMainWndCX = CW_USEDEFAULT, s_nMainWndCY = CW_USEDEFAULT;

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
bool xg_bAutoRetry = true;

// 古いパソコンであることを通知したか？
static bool s_bOldNotice = false;

// メインウィンドウクラス名。
static const LPCWSTR s_pszMainWndClass = L"XWord Giver Main Window";

// アクセラレータのハンドル。
static HACCEL s_hAccel = nullptr;

// プロセッサの数。
static DWORD s_dwNumberOfProcessors = 1;

// 計算時間測定用。
DWORDLONG xg_dwlTick0;    // 開始時間。
DWORDLONG xg_dwlTick1;    // 再計算時間。
DWORDLONG xg_dwlTick2;    // 終了時間。
DWORDLONG xg_dwlWait;     // 待ち時間。

// ディスク容量が足りないか？
static bool s_bOutOfDiskSpace = false;

// 連続生成の場合、問題を生成した数。
INT xg_nNumberGenerated = 0;

// 再計算の回数。
LONG xg_nRetryCount;

// ツールバーを表示するか？
bool xg_bShowToolBar = true;

// ステータスバーを表示するか？
static bool s_bShowStatusBar = true;

// ルール群。
INT xg_nRules = DEFAULT_RULES_JAPANESE;

//////////////////////////////////////////////////////////////////////////////
// スクロール関連。

// 水平スクロールの位置を取得する。
int __fastcall XgGetHScrollPos(void)
{
    return ::GetScrollPos(xg_canvasWnd, SB_HORZ);
}

// 垂直スクロールの位置を取得する。
int __fastcall XgGetVScrollPos(void)
{
    return ::GetScrollPos(xg_canvasWnd, SB_VERT);
}

// 水平スクロールの位置を設定する。
int __fastcall XgSetHScrollPos(int nPos, BOOL bRedraw)
{
    return ::SetScrollPos(xg_canvasWnd, SB_HORZ, nPos, bRedraw);
}

// 垂直スクロールの位置を設定する。
int __fastcall XgSetVScrollPos(int nPos, BOOL bRedraw)
{
    return ::SetScrollPos(xg_canvasWnd, SB_VERT, nPos, bRedraw);
}

// 水平スクロールの情報を取得する。
BOOL __fastcall XgGetHScrollInfo(LPSCROLLINFO psi)
{
    return ::GetScrollInfo(xg_canvasWnd, SB_HORZ, psi);
}

// 垂直スクロールの情報を取得する。
BOOL __fastcall XgGetVScrollInfo(LPSCROLLINFO psi)
{
    return ::GetScrollInfo(xg_canvasWnd, SB_VERT, psi);
}

// 水平スクロールの情報を設定する。
BOOL __fastcall XgSetHScrollInfo(LPSCROLLINFO psi, BOOL bRedraw)
{
    return ::SetScrollInfo(xg_canvasWnd, SB_HORZ, psi, bRedraw);
}

// 垂直スクロールの情報を設定する。
BOOL __fastcall XgSetVScrollInfo(LPSCROLLINFO psi, BOOL bRedraw)
{
    return ::SetScrollInfo(xg_canvasWnd, SB_VERT, psi, bRedraw);
}

// マス位置を取得する。
VOID XgGetCellPosition(RECT& rc, INT i1, INT j1, INT i2, INT j2)
{
    INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    ::SetRect(&rc,
        static_cast<int>(xg_nMargin + j1 * nCellSize), 
        static_cast<int>(xg_nMargin + i1 * nCellSize),
        static_cast<int>(xg_nMargin + j2 * nCellSize), 
        static_cast<int>(xg_nMargin + i2 * nCellSize));

    ::OffsetRect(&rc, -XgGetHScrollPos(), -XgGetVScrollPos());
}

// マス位置を設定する。
VOID XgSetCellPosition(LONG& x, LONG& y, INT& i, INT& j, BOOL bEnd)
{
    INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    y += XgGetVScrollPos();
    y -= xg_nMargin;
    i = (y + nCellSize / 2) / nCellSize;
    if (i < 0) {
        i = bEnd ? 1 : 0;
    } else if (i >= xg_nRows) {
        i = bEnd ? xg_nRows : (xg_nRows - 1);
    }
    y = i * nCellSize;
    y += xg_nMargin;

    x += XgGetHScrollPos();
    x -= xg_nMargin;
    j = (x + nCellSize / 2) / nCellSize;
    if (j < 0) {
        j = bEnd ? 1 : 0;
    } else if (j >= xg_nCols) {
        j = bEnd ? xg_nCols : (xg_nCols - 1);
    }
    x = j * nCellSize;
    x += xg_nMargin;
}

// 本当のクライアント領域を計算する。
void __fastcall XgGetRealClientRect(HWND hwnd, LPRECT prcClient)
{
    MRect rcClient;
    ::GetClientRect(xg_canvasWnd, &rcClient);

    assert(prcClient);
    *prcClient = rcClient;
}

// キャレット位置を更新する。
void __fastcall XgUpdateCaretPos(void)
{
    // 現在のセルサイズを計算する。
    INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    // 真・クライアント領域を取得する。
    RECT rc;
    XgGetRealClientRect(xg_hMainWnd, &rc);

    // 未確定文字列の表示位置を計算する。
    POINT pt;
    pt.x = xg_nMargin + xg_caret_pos.m_j * nCellSize;
    pt.y = xg_nMargin + xg_caret_pos.m_i * nCellSize;
    pt.x -= XgGetHScrollPos();
    pt.y -= XgGetVScrollPos();

    // 未確定文字列の設定。
    LOGFONTW lf; // 論理フォント。
    ZeroMemory(&lf, sizeof(lf));
    if (xg_bTateInput && XgIsUserCJK()) {
        lf.lfFaceName[0] = L'@';
        lf.lfFaceName[1] = 0;
        lf.lfEscapement = 2700;
        lf.lfOrientation = 2700;
        pt.x += nCellSize - 4;
    } else {
        pt.x += 4;
    }
    if (xg_szCellFont[0])
        StringCbCatW(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);
    else
        StringCbCatW(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
    lf.lfHeight = -nCellSize * xg_nCellCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;

    COMPOSITIONFORM CompForm = { CFS_POINT };
    pt.y += 4;
    MapWindowPoints(xg_canvasWnd, xg_hMainWnd, &pt, 1);
    CompForm.ptCurrentPos = pt; // 未確定文字列の表示位置。

    // IMEの制御。
    HIMC hIMC = ImmGetContext(xg_canvasWnd);
    ImmSetCompositionWindow(hIMC, &CompForm); // 未確定文字列の表示位置を設定。
    ImmSetCompositionFont(hIMC, &lf); // 未確定文字列のフォントを設定。
    ImmReleaseContext(xg_canvasWnd, hIMC);
}

// キャレット位置をセットする。
void __fastcall XgSetCaretPos(INT iRow, INT jCol)
{
    xg_caret_pos.m_i = iRow;
    xg_caret_pos.m_j = jCol;
    XgUpdateCaretPos();
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

    // ボックスの位置を更新。
    PostMessage(hwnd, WM_COMMAND, ID_MOVEBOXES, 0);
}

// キャレットが見えるように、必要ならばスクロールする。
void __fastcall XgEnsureCaretVisible(HWND hwnd)
{
    MRect rc, rcClient;
    SCROLLINFO si;
    bool bNeedRedraw = false;

    // クライアント領域を取得する。
    XgGetRealClientRect(hwnd, &rcClient);

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
    CompForm.ptCurrentPos.x = rc.left - XgGetHScrollPos();
    CompForm.ptCurrentPos.y = rc.top - XgGetVScrollPos();
    MapWindowPoints(xg_canvasWnd, hwnd, &CompForm.ptCurrentPos, 1);
    HIMC hIMC = ::ImmGetContext(xg_canvasWnd);
    ::ImmSetCompositionWindow(hIMC, &CompForm);
    ::ImmReleaseContext(xg_canvasWnd, hIMC);

    // 必要ならば再描画する。
    if (bNeedRedraw) {
        XgUpdateImage(hwnd);
    }

    XgUpdateCaretPos();
}

// 現在の状態で好ましいと思われる単語の最大長を取得する。
// 旧来のスマート解決では最大長を指定する必要があった。
INT __fastcall XgGetPreferredMaxLength(void)
{
    INT ret = 7;

    if (xg_dict_1.size()) {
        auto& word = xg_dict_1[0].m_word;
        auto ch = word[0];
        if (XgIsCharKanjiW(ch)) {
            // 漢字の熟語は4文字以下が多い。
            ret = 4;
        } else if (XgIsCharKanaW(ch)) {
            // 日本語の単語は4文字程度が多い。
            ret = 5;
        } else {
            // ロシア語や英語は日本語の単語より長い傾向にある。
            ret = 7;
        }
    }

    if (xg_nRules & (RULE_LINESYMMETRYH | RULE_LINESYMMETRYV)) {
        // 線対称のときに偶数長はまずいと思われる。
        if (ret % 2 == 0)
            ret += 1;
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////

// ツールバーのUIを更新する。
void XgUpdateToolBarUI(HWND hwnd)
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
    int i, nDirCount = 0;
    WCHAR sz[MAX_PATH];
    WCHAR szFormat[32];
    DWORD dwValue;

    // 初期化する。
    s_nMainWndX = CW_USEDEFAULT;
    s_nMainWndY = CW_USEDEFAULT;
    s_nMainWndCX = 475;
    s_nMainWndCY = 515;

    XG_HintsWnd::s_nHintsWndX = CW_USEDEFAULT;
    XG_HintsWnd::s_nHintsWndY = CW_USEDEFAULT;
    XG_HintsWnd::s_nHintsWndCX = 420;
    XG_HintsWnd::s_nHintsWndCY = 250;

    XG_CandsWnd::s_nCandsWndX = CW_USEDEFAULT;
    XG_CandsWnd::s_nCandsWndY = CW_USEDEFAULT;
    XG_CandsWnd::s_nCandsWndCX = 420;
    XG_CandsWnd::s_nCandsWndCY = 250;

    xg_nInputPaletteWndX = CW_USEDEFAULT;
    xg_nInputPaletteWndY = CW_USEDEFAULT;

    xg_bTateInput = false;
    xg_dict_name.clear();
    xg_dirs_save_to.clear();
    xg_bAutoRetry = true;
    xg_szCellFont[0] = 0;
    xg_szSmallFont[0] = 0;
    xg_szUIFont[0] = 0;
    xg_bShowToolBar = true;
    s_bShowStatusBar = true;
    xg_bShowInputPalette = false;
    xg_bSaveAsJsonFile = true;
    xg_bAddThickFrame = true;
    xg_bTateOki = true;
    xg_bCharFeed = true;
    xg_rgbWhiteCellColor = RGB(255, 255, 255);
    xg_rgbBlackCellColor = RGB(0x33, 0x33, 0x33);
    xg_rgbMarkedCellColor = RGB(255, 255, 255);
    xg_bDrawFrameForMarkedCell = TRUE;
    xg_bSmartResolution = TRUE;
    xg_nZoomRate = 100;
    xg_bShowNumbering = TRUE;
    xg_bShowCaret = TRUE;
    xg_bShowDoubleFrameLetters = TRUE;

    xg_bHiragana = FALSE;
    xg_bLowercase = FALSE;

    xg_nCellCharPercents = DEF_CELL_CHAR_SIZE;
    xg_nSmallCharPercents = DEF_SMALL_CHAR_SIZE;

    xg_strBlackCellImage.clear();

    XG_PatternDialog::xg_nPatWndX = CW_USEDEFAULT;
    XG_PatternDialog::xg_nPatWndY = CW_USEDEFAULT;
    XG_PatternDialog::xg_nPatWndCX = CW_USEDEFAULT;
    XG_PatternDialog::xg_nPatWndCY = CW_USEDEFAULT;
    XG_PatternDialog::xg_bShowAnswerOnPattern = TRUE;

    if (XgIsUserJapanese()) {
        xg_nRules = DEFAULT_RULES_JAPANESE;
        xg_nViewMode = XG_VIEW_NORMAL;
        xg_nFileType = XG_FILETYPE_XWJ;
    } else {
        xg_nRules = DEFAULT_RULES_ENGLISH;
        xg_nViewMode = XG_VIEW_SKELETON;
        xg_nFileType = XG_FILETYPE_XD;
    }
    xg_imode = xg_im_ANY; // 自由入力。

    xg_nNumberToGenerate = 16;

    xg_strDoubleFrameLetters = XgLoadStringDx1(IDS_DBLFRAME_LETTERS_1);

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
                XG_HintsWnd::s_nHintsWndX = dwValue;
            }
            if (!app_key.QueryDword(L"HintsY", dwValue)) {
                XG_HintsWnd::s_nHintsWndY = dwValue;
            }
            if (!app_key.QueryDword(L"HintsCX", dwValue)) {
                XG_HintsWnd::s_nHintsWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"HintsCY", dwValue)) {
                XG_HintsWnd::s_nHintsWndCY = dwValue;
            }

            if (!app_key.QueryDword(L"CandsX", dwValue)) {
                XG_CandsWnd::s_nCandsWndX = dwValue;
            }
            if (!app_key.QueryDword(L"CandsY", dwValue)) {
                XG_CandsWnd::s_nCandsWndY = dwValue;
            }
            if (!app_key.QueryDword(L"CandsCX", dwValue)) {
                XG_CandsWnd::s_nCandsWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"CandsCY", dwValue)) {
                XG_CandsWnd::s_nCandsWndCY = dwValue;
            }
            if (!app_key.QueryDword(L"FileType", dwValue)) {
                switch ((XG_FILETYPE)dwValue)
                {
                case XG_FILETYPE_XWD:
                case XG_FILETYPE_XWJ:
                    xg_nFileType = XG_FILETYPE_XWJ;
                    break;
                case XG_FILETYPE_CRP:
                    xg_nFileType = XG_FILETYPE_CRP;
                    break;
                case XG_FILETYPE_XD:
                    xg_nFileType = XG_FILETYPE_XD;
                    break;
                default:
                    if (XgIsUserJapanese())
                        xg_nFileType = XG_FILETYPE_XWJ;
                    else
                        xg_nFileType = XG_FILETYPE_XD;
                    break;
                }
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
                xg_bAutoRetry = !!dwValue;
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
                xg_bShowToolBar = !!dwValue;
            }
            if (!app_key.QueryDword(L"ShowStatusBar", dwValue)) {
                s_bShowStatusBar = !!dwValue;
            }
            if (!app_key.QueryDword(L"ShowInputPalette", dwValue)) {
                xg_bShowInputPalette = !!dwValue;
            }

            xg_bSaveAsJsonFile = true;

            if (!app_key.QueryDword(L"NumberToGenerate", dwValue)) {
                xg_nNumberToGenerate = dwValue;
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
                XG_PatternDialog::xg_nPatWndX = dwValue;
            }
            if (!app_key.QueryDword(L"PatWndY", dwValue)) {
                XG_PatternDialog::xg_nPatWndY = dwValue;
            }
            if (!app_key.QueryDword(L"PatWndCX", dwValue)) {
                XG_PatternDialog::xg_nPatWndCX = dwValue;
            }
            if (!app_key.QueryDword(L"PatWndCY", dwValue)) {
                XG_PatternDialog::xg_nPatWndCY = dwValue;
            }
            if (!app_key.QueryDword(L"ShowAnsOnPat", dwValue)) {
                XG_PatternDialog::xg_bShowAnswerOnPattern = dwValue;
            }
            if (!app_key.QueryDword(L"Rules", dwValue)) {
                xg_nRules = dwValue | RULE_DONTDIVIDE;
            }
            if (!app_key.QueryDword(L"ShowDoubleFrameLetters", dwValue)) {
                xg_bShowDoubleFrameLetters = !!dwValue;
            }
            if (!app_key.QueryDword(L"ViewMode", dwValue)) {
                xg_nViewMode = static_cast<XG_VIEW_MODE>(dwValue);
                if (xg_nViewMode != XG_VIEW_NORMAL && xg_nViewMode != XG_VIEW_SKELETON) {
                    xg_nViewMode = XG_VIEW_NORMAL;
                }
            }

            if (!app_key.QuerySz(L"Recent", sz, ARRAYSIZE(sz))) {
                xg_dict_name = sz;
                if (!PathFileExists(xg_dict_name.c_str()))
                {
                    xg_dict_name.clear();
                }
            }

            if (!app_key.QuerySz(L"BlackCellImage", sz, ARRAYSIZE(sz))) {
                WCHAR szFullPath[MAX_PATH];
                if (XgGetLoadImagePath(szFullPath, sz))
                {
                    xg_strBlackCellImage = szFullPath;
                }
                else
                {
                    xg_strBlackCellImage.clear();
                }
            }

            if (!app_key.QuerySz(L"DoubleFrameLetters", sz, ARRAYSIZE(sz))) {
                xg_strDoubleFrameLetters = sz;
            }

            // 保存先のリストを取得する。
            if (!app_key.QueryDword(L"SaveToCount", dwValue)) {
                nDirCount = dwValue;
                for (i = 0; i < nDirCount; i++) {
                    StringCbPrintf(szFormat, sizeof(szFormat), L"SaveTo %d", i + 1);
                    if (!app_key.QuerySz(szFormat, sz, ARRAYSIZE(sz))) {
                        xg_dirs_save_to.emplace_back(sz);
                    } else {
                        nDirCount = i;
                        break;
                    }
                }
            }
        }
    }

    // 保存先リストが空だったら、初期化する。
    if (nDirCount == 0 || xg_dirs_save_to.empty()) {
        LPITEMIDLIST pidl;
        WCHAR szPath[MAX_PATH];
        ::SHGetSpecialFolderLocation(nullptr, CSIDL_PERSONAL, &pidl);
        ::SHGetPathFromIDListW(pidl, szPath);
        ::CoTaskMemFree(pidl);
        xg_dirs_save_to.emplace_back(szPath);
    }

    ::DeleteObject(xg_hbmBlackCell);
    xg_hbmBlackCell = NULL;

    DeleteEnhMetaFile(xg_hBlackCellEMF);
    xg_hBlackCellEMF = NULL;

    if (!xg_strBlackCellImage.empty())
    {
        HBITMAP hbm = NULL;
        HENHMETAFILE hEMF = NULL;
        if (XgLoadImage(xg_strBlackCellImage.c_str(), hbm, hEMF))
        {
            // ファイルが存在すれば、画像を読み込む。
            xg_hbmBlackCell = hbm;
            xg_hBlackCellEMF = hEMF;
            if (xg_nViewMode == XG_VIEW_SKELETON)
            {
                // 画像が有効ならスケルトンビューを通常ビューに戻す。
                xg_nViewMode = XG_VIEW_NORMAL;
            }
        }
        else
        {
            // 画像が無効なら、パスも無効化。
            xg_strBlackCellImage.clear();
        }
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
            app_key.SetDword(L"AutoRetry", xg_bAutoRetry);
            app_key.SetDword(L"Rows", xg_nRows);
            app_key.SetDword(L"Cols", xg_nCols);

            app_key.SetSz(L"CellFont", xg_szCellFont, ARRAYSIZE(xg_szCellFont));
            app_key.SetSz(L"SmallFont", xg_szSmallFont, ARRAYSIZE(xg_szSmallFont));
            app_key.SetSz(L"UIFont", xg_szUIFont, ARRAYSIZE(xg_szUIFont));

            app_key.SetDword(L"ShowToolBar", xg_bShowToolBar);
            app_key.SetDword(L"ShowStatusBar", s_bShowStatusBar);
            app_key.SetDword(L"ShowInputPalette", xg_bShowInputPalette);

            app_key.SetDword(L"SaveAsJsonFile", xg_bSaveAsJsonFile);
            app_key.SetDword(L"NumberToGenerate", xg_nNumberToGenerate);
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
            app_key.SetDword(L"ShowNumbering", xg_bShowNumbering);
            app_key.SetDword(L"ShowCaret", xg_bShowCaret);

            app_key.SetDword(L"Hiragana", xg_bHiragana);
            app_key.SetDword(L"Lowercase", xg_bLowercase);

            app_key.SetDword(L"CellCharPercents", xg_nCellCharPercents);
            app_key.SetDword(L"SmallCharPercents", xg_nSmallCharPercents);

            app_key.SetDword(L"PatWndX", XG_PatternDialog::xg_nPatWndX);
            app_key.SetDword(L"PatWndY", XG_PatternDialog::xg_nPatWndY);
            app_key.SetDword(L"PatWndCX", XG_PatternDialog::xg_nPatWndCX);
            app_key.SetDword(L"PatWndCY", XG_PatternDialog::xg_nPatWndCY);
            app_key.SetDword(L"ShowAnsOnPat", XG_PatternDialog::xg_bShowAnswerOnPattern);

            app_key.SetDword(L"Rules", xg_nRules);
            app_key.SetDword(L"ShowDoubleFrameLetters", xg_bShowDoubleFrameLetters);
            app_key.SetDword(L"ViewMode", xg_nViewMode);

            app_key.SetSz(L"Recent", xg_dict_name.c_str());
            app_key.SetSz(L"BlackCellImage", xg_strBlackCellImage.c_str());
            app_key.SetSz(L"DoubleFrameLetters", xg_strDoubleFrameLetters.c_str());

            // 保存先のリストを設定する。
            nCount = static_cast<int>(xg_dirs_save_to.size());
            app_key.SetDword(L"SaveToCount", nCount);
            for (i = 0; i < nCount; i++)
            {
                StringCbPrintf(szFormat, sizeof(szFormat), L"SaveTo %d", i + 1);
                app_key.SetSz(szFormat, xg_dirs_save_to[i].c_str());
            }

            app_key.SetDword(L"HintsX", XG_HintsWnd::s_nHintsWndX);
            app_key.SetDword(L"HintsY", XG_HintsWnd::s_nHintsWndY);
            app_key.SetDword(L"HintsCX", XG_HintsWnd::s_nHintsWndCX);
            app_key.SetDword(L"HintsCY", XG_HintsWnd::s_nHintsWndCY);

            app_key.SetDword(L"CandsX", XG_CandsWnd::s_nCandsWndX);
            app_key.SetDword(L"CandsY", XG_CandsWnd::s_nCandsWndY);
            app_key.SetDword(L"CandsCX", XG_CandsWnd::s_nCandsWndCX);
            app_key.SetDword(L"CandsCY", XG_CandsWnd::s_nCandsWndCY);

            app_key.SetDword(L"IPaletteX", xg_nInputPaletteWndX);
            app_key.SetDword(L"IPaletteY", xg_nInputPaletteWndY);

            app_key.SetDword(L"WindowX", s_nMainWndX);
            app_key.SetDword(L"WindowY", s_nMainWndY);
            app_key.SetDword(L"WindowCX", s_nMainWndCX);
            app_key.SetDword(L"WindowCY", s_nMainWndCY);

            app_key.SetDword(L"TateInput", xg_bTateInput);

            app_key.SetDword(L"FileType", (DWORD)xg_nFileType);
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

    // 黒マス線対称。
    if ((xg_nRules & RULE_LINESYMMETRYV) && !xg_xword.IsLineSymmetryV()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRYV), nullptr, MB_ICONERROR);
        return false;
    }
    if ((xg_nRules & RULE_LINESYMMETRYH) && !xg_xword.IsLineSymmetryH()) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRYH), nullptr, MB_ICONERROR);
        return false;
    }

    // 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
    if (!(xg_nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENROWLINESYMV), nullptr, MB_ICONERROR);
        return false;
    }
    // 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
    if (!(xg_nCols & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENCOLLINESYMH), nullptr, MB_ICONERROR);
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
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_NOCANDIDATE), pos.m_j + 1, pos.m_i + 1);
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
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_TOOLONGSPACE), pos.m_j + 1, pos.m_i + 1);
            XgCenterMessageBoxW(hwnd, sz, nullptr, MB_ICONERROR);
            return false;
        }
    }

    // 見つからなかった単語があるか？
    if (!vNotFoundWords.empty() && check_words) {
        auto str = mstr_join(vNotFoundWords, L", ");
        if (str.size() > 128)
            str.resize(128);

        WCHAR szText[256];
        StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_NOTREGDWORD), str.c_str());

        // 未登録単語があることを1回だけ警告。
        if (XgCenterMessageBoxW(hwnd, szText, XgLoadStringDx2(IDS_WARNING),
                                MB_ICONWARNING | MB_YESNOCANCEL) != IDYES)
        {
            // キャンセルされた。
            return false;
        }

        // 辞書データにヒントなしで追加する。
        for (auto& word : vNotFoundWords) {
            xg_dict_1.emplace_back(word, L"");
        }

        // 二分探索のために、並び替えておく。
        XgSortAndUniqueDictData(xg_dict_1);
    }

    // 成功。
    return true;
}

//////////////////////////////////////////////////////////////////////////////
// 候補ウィンドウ。

// 候補ウィンドウを破棄する。
void XgDestroyCandsWnd(void)
{
    // 候補ウィンドウが存在するか？
    if (::IsWindow(xg_cands_wnd))
    {
        // 更新を無視・破棄する。
        ::DestroyWindow(xg_cands_wnd);
    }
}

// 候補の内容を候補ウィンドウで開く。
bool XgOpenCandsWnd(HWND hwnd, bool vertical)
{
    // もし候補ウィンドウが存在すれば破棄する。
    if (IsWindow(xg_cands_wnd)) {
        DestroyWindow(xg_cands_wnd);
    }

    return xg_cands_wnd.Open(hwnd, vertical);
}

// ヒントを更新する。
void __fastcall XgUpdateHints(void)
{
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_strHints.clear();
    if (xg_bSolved) {
        XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
        XgGetHintsStr(xg_solution, xg_strHints, 2, true);
        if (!XgParseHintsStr(xg_strHints)) {
            xg_strHints.clear();
        }
    }
}

// ヒントの内容をメモ帳で開く。
bool XgOpenHintsByNotepad(HWND /*hwnd*/, bool bShowAnswer)
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
    XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
    XgGetHintsStr(xg_solution, xg_strHints, 2, true);
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

//////////////////////////////////////////////////////////////////////////////

// 盤を特定の文字で埋め尽くす。
void XgNewCells(HWND hwnd, WCHAR ch, INT nRows, INT nCols)
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
    XgSetCaretPos();
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

// 単語リストのコピー。
void XgCopyWordList(HWND hwnd)
{
    const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

    // 全単語を取得。空白を含む単語は無視。
    std::vector<std::wstring> words;
    for (INT iRow = 0; iRow < xg_nRows; ++iRow) {
        for (INT jCol = 0; jCol < xg_nCols; ++jCol) {
            std::wstring str;
            str = xw->GetPatternV(XG_Pos(iRow, jCol));
            if (str.size() >= 2 && str.find(ZEN_SPACE) == str.npos) {
                words.push_back(str);
            }
            str = xw->GetPatternH(XG_Pos(iRow, jCol));
            if (str.size() >= 2 && str.find(ZEN_SPACE) == str.npos) {
                words.push_back(str);
            }
        }
    }

    // ソートして一意化。
    std::sort(words.begin(), words.end());
    auto last = std::unique(words.begin(), words.end());
    words.erase(last, words.end());

    // 改行区切りにする。
    std::wstring str = mstr_join(words, L"\r\n");
    str += L"\r\n";

    // 全角英数を半角英数にする。
    for (auto& wch : str) {
        if (ZEN_LARGE_A <= wch && wch <= ZEN_LARGE_Z)
            wch = L'a' + (wch - ZEN_LARGE_A);
        else if (ZEN_SMALL_A <= wch && wch <= ZEN_SMALL_Z)
            wch = L'a' + (wch - ZEN_SMALL_A);
    }

    // クリップボードにコピー。
    size_t cb = (str.size() + 1) * sizeof(WCHAR);
    if (HGLOBAL hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, cb)) {
        if (LPWSTR psz = reinterpret_cast<LPWSTR>(::GlobalLock(hGlobal))) {
            StringCbCopyW(psz, cb, str.c_str());
            ::GlobalUnlock(hGlobal);

            if (::OpenClipboard(hwnd)) {
                ::EmptyClipboard();
                ::SetClipboardData(CF_UNICODETEXT, hGlobal);
                ::CloseClipboard();
                return;
            }
        }
        ::GlobalFree(hGlobal);
    }
}

// 盤のサイズを変更する。
void XgResizeCells(HWND hwnd, INT nNewRows, INT nNewCols)
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
    INT iMin = std::min((INT)xg_nRows, (INT)nNewRows);
    INT jMin = std::min((INT)xg_nCols, (INT)nNewCols);
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
    XgSetCaretPos();
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

//////////////////////////////////////////////////////////////////////////////

// 保存先。
WCHAR xg_szDir[MAX_PATH] = L"";

// 「保存先」参照。
INT CALLBACK XgBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM /*lpData*/)
{
    if (uMsg == BFFM_INITIALIZED) {
        // 初期化の際に、フォルダーの場所を指定する。
        ::SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(xg_szDir));
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// 印刷。

// 印刷する。
static void XgPrintIt(HDC hdc, PRINTDLGW* ppd, bool bPrintAnswer)
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
                XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
                XgGetHintsStr(xg_solution, xg_strHints, 2, bPrintAnswer);
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
static void XgPrintProblem(void)
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
static void XgPrintAnswer(void)
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
    XG_NewDialog dialog;
    nID = dialog.DoModal(hwnd);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    return (nID == IDOK);
}

// 特定の場所にファイルを保存する。
bool __fastcall XgDoSaveToLocation(HWND hwnd)
{
    WCHAR szPath[MAX_PATH], szDir[MAX_PATH];
    WCHAR szName[32];

    // パスを生成する。
    StringCbCopy(szDir, sizeof(szDir), xg_dirs_save_to[0].data());
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
                XgUpdateHints();
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

// ズームを実際のウィンドウに合わせる。
void __fastcall XgFitZoom(HWND hwnd)
{
    // クライアント領域のサイズを取得する。
    RECT rc;
    XgGetRealClientRect(hwnd, &rc);
    SIZE sizClient = { rc.right - rc.left, rc.bottom - rc.top };

    // イメージを更新。
    XgUpdateImage(hwnd);

    // 描画サイズを取得。
    SIZE siz;
    XgGetXWordExtent(&siz);

    // サイズをクライアント領域にフィットさせる。
    INT nZoomRate;
    if (sizClient.cx * siz.cy > siz.cx * sizClient.cy) {
        nZoomRate = sizClient.cy * 100 / siz.cy;
    } else {
        nZoomRate = sizClient.cx * 100 / siz.cx;
    }

    // ズーム倍率を修正。
    if (nZoomRate == 0)
        nZoomRate = 1;
    if (nZoomRate > 100)
        nZoomRate = 100;
    xg_nZoomRate = nZoomRate;
    XgUpdateScrollInfo(hwnd, 0, 0);

    // 再描画。
    XgUpdateImage(hwnd, 0, 0);
    XgUpdateStatusBar(hwnd);
}

// 問題の作成。
bool __fastcall XgOnGenerate(HWND hwnd, bool show_answer, bool multiple = false)
{
    INT nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    if (multiple) {
        // [問題の連続作成]ダイアログ。
        XG_SeqGenDialog dialog;
        nID = INT(dialog.DoModal(hwnd));
    } else {
        // [問題の作成]ダイアログ。
        XG_GenDialog dialog;
        dialog.m_bShowAnswer = show_answer;
        nID = INT(dialog.DoModal(hwnd));
        show_answer = dialog.m_bShowAnswer;
    }
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        return false;
    }

    xg_bSolvingEmpty = true;
    xg_bNoAddBlack = false;
    xg_nNumberGenerated = 0;
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
        if (xg_bSmartResolution && xg_nRows >= 7 && xg_nCols >= 7) {
            XG_CancelSmartSolveDialog dialog;
            nID = dialog.DoModal(hwnd);
        } else if (xg_nRules & (RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH)) {
            XG_CancelSmartSolveDialog dialog;
            nID = dialog.DoModal(hwnd);
        } else {
            XG_CancelSolveDialog dialog;
            nID = dialog.DoModal(hwnd);
        }
        // 生成成功のときはxg_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bSolved) {
            ++xg_nNumberGenerated;
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
    } while (nID == IDOK && multiple && xg_nNumberGenerated < xg_nNumberToGenerate);
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

        // ズームを全体に合わせる。
        XgFitZoom(hwnd);
    }

    return true;
}

// 黒マスパターンを連続生成する。
bool __fastcall XgOnGenerateBlacksRepeatedly(HWND hwnd)
{
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    XG_SeqPatGenDialog dialog;
    INT nID = INT(dialog.DoModal(hwnd));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        return false;
    }

    // 初期化する。
    xg_xword.clear();
    XgSetCaretPos();
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
    xg_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、生成を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    do
    {
        XG_CancelGenBlacksDialog dialog;
        nID = dialog.DoModal(hwnd);
        // 生成成功のときはxg_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bBlacksGenerated) {
            ++xg_nNumberGenerated;
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
    } while (nID == IDOK && xg_nNumberGenerated < xg_nNumberToGenerate);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    XgSetCaretPos();
    XgUpdateImage(hwnd, 0, 0);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_BLOCKSGENERATED),
            xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    }

    // 保存先フォルダを開く。
    if (xg_nNumberGenerated && !xg_dirs_save_to.empty()) {
        ::ShellExecuteW(hwnd, nullptr, xg_dirs_save_to[0].data(),
                      nullptr, nullptr, SW_SHOWNORMAL);
    }
    return true;
}

// 黒マスパターンを生成する。
bool __fastcall XgOnGenerateBlacks(HWND hwnd, bool sym)
{
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    XG_PatGenDialog dialog;
    INT nID = dialog.DoModal(hwnd);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        return false;
    }

    // 初期化する。
    XgSetCaretPos();
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
    xg_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、生成を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    {
        XG_CancelGenBlacksDialog dialog;
        dialog.DoModal(hwnd);
    }
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    XgSetCaretPos();
    XgUpdateImage(hwnd, 0, 0);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_BLOCKSGENERATED), 1,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
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
    xg_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    {
        XG_CancelSolveDialog dialog;
        dialog.DoModal(hwnd);
    }
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        // キャンセルされた。
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
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
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 成功メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_SOLVED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints();
        XgShowHints(hwnd);
    } else {
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // 失敗メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANTSOLVE),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        ::InvalidateRect(hwnd, nullptr, FALSE);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONERROR);
    }
    return true;
}

// 解を求める（黒マス追加なし）。
bool __fastcall XgOnSolve_NoAddBlack(HWND hwnd, bool bShowAnswer/* = true*/)
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
    xg_nNumberGenerated = 0;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // キャンセルダイアログを表示し、実行を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    {
        XG_CancelSolveNoAddBlackDialog dialog;
        dialog.DoModal(hwnd);
    }
    ::EnableWindow(xg_hwndInputPalette, TRUE);

    WCHAR sz[MAX_PATH];
    if (xg_bCancelled) {
        // キャンセルされた。
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 終了メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
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
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);

        // 成功メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_SOLVED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints();
        XgShowHints(hwnd);
    } else {
        // 解なし。表示を更新する。
        xg_bShowAnswer = false;
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // 失敗メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANTSOLVE),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
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
    XG_SeqSolveDialog dialog;
    nID = INT(dialog.DoModal(hwnd));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        // イメージを更新する。
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        return false;
    }

    // 初期化する。
    xg_strFileName.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_nNumberGenerated = 0;
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
        XG_CancelSolveDialog dialog;
        nID = dialog.DoModal(hwnd);
        // 生成成功のときはxg_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bSolved) {
            ++xg_nNumberGenerated;
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
    } while (nID == IDOK && xg_nNumberGenerated < xg_nNumberToGenerate);
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
    XgSetCaretPos();
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);

    // 終了メッセージを表示する。
    WCHAR sz[MAX_PATH];
    if (s_bOutOfDiskSpace) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    }
    XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

    // 保存先フォルダを開く。
    if (xg_nNumberGenerated && !xg_dirs_save_to.empty()) {
        ::ShellExecuteW(hwnd, nullptr, xg_dirs_save_to[0].data(),
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
    XG_SeqSolveDialog dialog;
    nID = INT(dialog.DoModal(hwnd));
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK) {
        // イメージを更新する。
        XgSetCaretPos();
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        return false;
    }

    // 初期化する。
    xg_strFileName.clear();
    xg_strHeader.clear();
    xg_strNotes.clear();
    xg_nNumberGenerated = 0;
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
        XG_CancelSolveNoAddBlackDialog dialog;
        nID = dialog.DoModal(hwnd);
        // 生成成功のときはxg_nNumberGeneratedを増やす。
        if (nID == IDOK && xg_bSolved) {
            ++xg_nNumberGenerated;
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
    } while (nID == IDOK && xg_nNumberGenerated < xg_nNumberToGenerate);
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
    XgSetCaretPos();
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);

    // 終了メッセージを表示する。
    WCHAR sz[MAX_PATH];
    if (s_bOutOfDiskSpace) {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    } else {
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    }
    XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

    // 保存先フォルダを開く。
    if (xg_nNumberGenerated && !xg_dirs_save_to.empty()) {
        ::ShellExecuteW(hwnd, nullptr, xg_dirs_save_to[0].data(),
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

// クリップボードにクロスワードをコピー。
void XgCopyBoard(HWND hwnd)
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
                            RECT rc;
                            SetRect(&rc, 0, 0, siz.cx, siz.cy);
                            FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));
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
void XgCopyBoardAsImage(HWND hwnd)
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
            FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));
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

// 二重マス単語をコピー。
void __fastcall XgCopyMarkWord(HWND hwnd)
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
        hEMF = ::CloseEnhMetaFile(hdc);
    }
    ::ReleaseDC(hwnd, hdcRef);

    // すでに解があるかどうかによって切り替え。
    const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

    // 二重マス単語のテキストを取得。
    HGLOBAL hGlobal = NULL;
    std::wstring strMarkWord;
    if (XgGetMarkWord(xw, strMarkWord)) {
        for (auto& wch : strMarkWord) {
            if (ZEN_LARGE_A <= wch && wch <= ZEN_LARGE_Z)
                wch = L'A' + (wch - ZEN_LARGE_A);
            else if (ZEN_SMALL_A <= wch && wch <= ZEN_SMALL_Z)
                wch = L'A' + (wch - ZEN_SMALL_A);
        }

        // CF_UNICODETEXTのデータを用意。
        DWORD cbGlobal = (strMarkWord.size() + 1) * sizeof(WCHAR);
        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cbGlobal);
        if (LPWSTR psz = (LPWSTR)GlobalLock(hGlobal)) {
            StringCbCopyW(psz, cbGlobal, strMarkWord.c_str());
            GlobalUnlock(hGlobal);
        } else {
            GlobalFree(hGlobal);
            hGlobal = NULL;
        }
    }

    // クリップボードを開く。
    if (::OpenClipboard(hwnd)) {
        // クリップボードを空にする。
        if (::EmptyClipboard()) {
            // テキストを設定。
            ::SetClipboardData(CF_UNICODETEXT, hGlobal);
            // EMFを設定。
            ::SetClipboardData(CF_ENHMETAFILE, hEMF);
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
void XgPasteBoard(HWND hwnd, const std::wstring& str)
{
    // 文字列が空じゃないか？
    if (str.empty())
        return;

    // 文字列がクロスワードを表していると仮定する。
    // クロスワードに文字列を設定。
    if (!xg_xword.SetString(str)) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    xg_bSolved = false;
    xg_bShowAnswer = false;
    XgSetCaretPos();
    xg_vMarks.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);
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
    XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
    XgGetHintsStr(xg_solution, str, hint_type, false);
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
    XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
    XgGetHintsStr(xg_solution, str, hint_type, false);
    xg_str_trim(str);

    // スタイルワンでは要らない部分を削除する。
    xg_str_replace_all(str, XgLoadStringDx1(IDS_DOWNLEFT), L"");
    xg_str_replace_all(str, XgLoadStringDx1(IDS_ACROSSLEFT), L"");
    xg_str_replace_all(str, XgLoadStringDx1(IDS_KEYRIGHT), XgLoadStringDx2(IDS_DOT));

    // HTMLデータ (UTF-8)を用意する。
    std::wstring html;
    XgGetHintsStr(xg_solution, html, hint_type + 3, false);
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
    ::DestroyWindow(xg_hSizeGrip);
    xg_hSizeGrip = NULL;
    ::DestroyWindow(xg_cands_wnd);
    ::DestroyWindow(xg_hHintsWnd);
    xg_hHintsWnd = NULL;
    ::DestroyWindow(xg_hwndInputPalette);
    xg_hwndInputPalette = NULL;

    // アプリを終了する。
    ::PostQuitMessage(0);

    xg_hMainWnd = NULL;
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
    for (const auto& entry : xg_dict_files)
    {
        std::wstring text;
        if (entry.m_friendly_name == entry.m_filename)
        {
            text = PathFindFileNameW(entry.m_filename.c_str());
        }
        else
        {
            text = PathFindFileNameW(entry.m_filename.c_str());
            text += L"\t";
            text += entry.m_friendly_name;
        }
        StringCbPrintfW(szText, sizeof(szText), L"&%c ",
            L"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"[count]);
        StringCbCatW(szText, sizeof(szText), text.c_str());
        AppendMenuW(hDictMenu, MF_STRING | MF_ENABLED, id, szText);
        ++index;
        ++count;
        ++id;
        if (count >= MAX_DICTS)
            break;
    }

    index = 4;
    for (const auto& entry : xg_dict_files)
    {
        auto& file = entry.m_filename;
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
    if (xg_bAutoRetry) {
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
    // 黒マス線対称。
    if (xg_nRules & RULE_LINESYMMETRYV)
        ::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYV, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYV, MF_UNCHECKED);
    if (xg_nRules & RULE_LINESYMMETRYH)
        ::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYH, MF_CHECKED);
    else
        ::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYH, MF_UNCHECKED);

    // 入力モード。
    switch (xg_imode) {
    case xg_im_KANA:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_KANAINPUT, MF_BYCOMMAND);
        break;

    case xg_im_ABC:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_ABCINPUT, MF_BYCOMMAND);
        break;

    case xg_im_KANJI:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_KANJIINPUT, MF_BYCOMMAND);
        break;

    case xg_im_RUSSIA:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_RUSSIAINPUT, MF_BYCOMMAND);
        break;

    case xg_im_GREEK:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_GREEKINPUT, MF_BYCOMMAND);
        break;

    case xg_im_DIGITS:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_DIGITINPUT, MF_BYCOMMAND);
        break;

    case xg_im_ANY:
        ::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_ANYINPUT, MF_BYCOMMAND);
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
        ::EnableMenuItem(hMenu, ID_COPYMARKWORD, MF_BYCOMMAND | MF_GRAYED);
    } else {
        ::EnableMenuItem(hMenu, ID_KILLMARKS, MF_BYCOMMAND | MF_ENABLED);
        ::EnableMenuItem(hMenu, ID_COPYMARKWORD, MF_BYCOMMAND | MF_ENABLED);
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
    if (xg_bShowToolBar) {
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

    // 二重マス文字表示のメニュー更新。
    if (xg_bShowDoubleFrameLetters) {
        ::CheckMenuItem(hMenu, ID_VIEW_DOUBLEFRAME_LETTERS, MF_BYCOMMAND | MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_VIEW_DOUBLEFRAME_LETTERS, MF_BYCOMMAND | MF_UNCHECKED);
    }

    // ビューモード。
    switch (xg_nViewMode) {
    case XG_VIEW_NORMAL:
    default:
        // 通常ビュー。
        ::CheckMenuRadioItem(hMenu, ID_VIEW_NORMAL_VIEW, ID_VIEW_SKELETON_VIEW,
                             ID_VIEW_NORMAL_VIEW, MF_BYCOMMAND);
        break;
    case XG_VIEW_SKELETON:
        // スケルトンビュー。
        ::CheckMenuRadioItem(hMenu, ID_VIEW_NORMAL_VIEW, ID_VIEW_SKELETON_VIEW,
                             ID_VIEW_SKELETON_VIEW, MF_BYCOMMAND);
        break;
    }

    // 行と列の削除と追加。
    if (xg_bSolved || xg_nRows - 1 < XG_MIN_SIZE) {
        ::DeleteMenu(hMenu, ID_DELETE_ROW, MF_BYCOMMAND);
    }
    if (xg_bSolved || xg_nCols - 1 < XG_MIN_SIZE) {
        ::DeleteMenu(hMenu, ID_DELETE_COLUMN, MF_BYCOMMAND);
    }
    if (xg_bSolved || xg_nRows + 1 > XG_MAX_SIZE) {
        ::DeleteMenu(hMenu, ID_INSERT_ROW_ABOVE, MF_BYCOMMAND);
        ::DeleteMenu(hMenu, ID_INSERT_ROW_BELOW, MF_BYCOMMAND);
    }
    BOOL bDeleteSepOK = FALSE;
    if (xg_bSolved || xg_nCols + 1 > XG_MAX_SIZE) {
        ::DeleteMenu(hMenu, ID_LEFT_INSERT_COLUMN, MF_BYCOMMAND);
        bDeleteSepOK = ::DeleteMenu(hMenu, ID_RIGHT_INSERT_COLUMN, MF_BYCOMMAND);
    }
    if (xg_bSolved && bDeleteSepOK) {
        INT cItems = ::GetMenuItemCount(hMenu);
        ::DeleteMenu(hMenu, cItems - 1, MF_BYPOSITION);
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
    case xg_im_GREEK: str += XgLoadStringDx1(IDS_GREEK); break;
    case xg_im_DIGITS: str += XgLoadStringDx1(IDS_DIGITS); break;
    case xg_im_ANY: str += XgLoadStringDx1(IDS_ANYINPUT); break;
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
    if (xg_bShowToolBar) {
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
    HDWP hDwp = ::BeginDeferWindowPos(2);
    if (hDwp) {
        if (::IsWindowVisible(xg_hSizeGrip)) {
            hDwp = ::DeferWindowPos(hDwp, xg_hSizeGrip, NULL,
                x + cx - cxVScrollBar, y + cy - cyHScrollBar,
                cxVScrollBar, cyHScrollBar,
                SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        hDwp = ::DeferWindowPos(hDwp, xg_canvasWnd, NULL,
            x, y, cx, cy, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        ::EndDeferWindowPos(hDwp);
    }

    // 再描画する。
    ::InvalidateRect(xg_hToolBar, NULL, TRUE);
    ::InvalidateRect(xg_hStatusBar, NULL, TRUE);
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

    // キャレット位置を更新。
    XgUpdateCaretPos();

    // ボックスの位置を更新。
    PostMessage(hwnd, WM_COMMAND, ID_MOVEBOXES, 0);
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

    // ボックスの位置を更新。
    PostMessage(hwnd, WM_COMMAND, ID_MOVEBOXES, 0);
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

// 設定。
void MainWnd_OnSettings(HWND hwnd)
{
    XgDestroyCandsWnd();
    XG_SettingsDialog dialog;
    dialog.DoModal(hwnd);
}

// テーマが変更された。
void XgUpdateTheme(HWND hwnd)
{
    std::unordered_set<std::wstring> priority, forbidden;
    XgParseTheme(priority, forbidden, xg_strDefaultTheme);
    xg_bThemeModified = (priority != xg_priority_tags || forbidden != xg_forbidden_tags);

    // メニュー項目の個数を取得。
    HMENU hMenu = ::GetMenu(hwnd);
    INT nCount = ::GetMenuItemCount(hMenu);
    assert(nCount > 0);
    // 辞書の文字列から「辞書」メニューのインデックスiMenuを取得。
    INT iMenu;
    WCHAR szText[32];
    MENUITEMINFOW info = { sizeof(info) };
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    for (iMenu = 0; iMenu < nCount; ++iMenu) {
        szText[0] = 0;
        ::GetMenuStringW(hMenu, iMenu, szText, ARRAYSIZE(szText), MF_BYPOSITION);
        if (wcsstr(szText, XgLoadStringDx1(IDS_DICT)) != NULL) {
            break;
        }
    }
    assert(iMenu != nCount);
    // 辞書の状態に対して文字列を指定。
    if (xg_bThemeModified || xg_dict_name.empty() ||
        xg_dict_name.find(L"\\SubDict") != xg_dict_name.npos)
    {
        StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_MODIFIEDDICT));
    } else {
        StringCbCopyW(szText, sizeof(szText), XgLoadStringDx1(IDS_DEFAULTDICT));
    }
    // メニュー文字列を変更。
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    info.dwTypeData = szText;
    ::SetMenuItemInfoW(hMenu, iMenu, TRUE, &info);
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
    // キャレット位置を初期化する。
    XgSetCaretPos();

    // ツールバーの表示を切り替える。
    if (xg_bShowToolBar)
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
        XG_HintsWnd::UpdateHintData(); // ヒントに変更があれば、更新する。
        XgGetHintsStr(xg_solution, xg_strHints, 2, true);
        if (!XgParseHintsStr(xg_strHints)) {
            xg_strHints.clear();
        }
        std::swap(xg_dict_1, old_dict_1);
        std::swap(xg_dict_2, old_dict_2);
    }
    std::swap(xg_caret_pos.m_i, xg_caret_pos.m_j);
    for (auto& mark : xg_vMarks) {
        std::swap(mark.m_i, mark.m_j);
    }
    XgUpdateCaretPos();
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

    for (auto& wch : raw) {
        if (ZEN_LARGE_A <= wch && wch <= ZEN_LARGE_Z)
            wch = L'a' + (wch - ZEN_LARGE_A);
        else if (ZEN_SMALL_A <= wch && wch <= ZEN_SMALL_Z)
            wch = L'a' + (wch - ZEN_SMALL_A);
    }

    switch (xg_imode)
    {
    case xg_im_ABC:
    case xg_im_KANA:
    case xg_im_KANJI:
        raw += L" ";
        raw += XgLoadStringDx2(IDS_DICTIONARY);
        break;
    case xg_im_RUSSIA:
    case xg_im_GREEK:
    case xg_im_DIGITS:
        break;
    case xg_im_ANY:
        raw += L" ";
        raw += XgLoadStringDx2(IDS_DICTIONARY);
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

    // 半角英大文字にする。
    for (auto& ch : pattern) {
        if (ZEN_LARGE_A <= ch && ch <= ZEN_LARGE_Z)
            ch += L'A' - ZEN_LARGE_A;
    }

    XgSetClipboardUnicodeText(hwnd, pattern);
}

void __fastcall MainWnd_OnCopyPatternHorz(HWND hwnd)
{
    MainWnd_OnCopyPattern(hwnd, FALSE);
}

// 文字集合をコピーする。
void MainWnd_OnCopyCharSet(HWND hwnd)
{
    const XG_Board *pxword;
    if (xg_bSolved) {
        pxword = &xg_solution;
    } else {
        pxword = &xg_xword;
    }

    std::multiset<WCHAR> multiset;
    for (INT i = 0; i < xg_nRows; ++i) {
        for (INT j = 0; j < xg_nCols; ++j) {
            WCHAR ch = pxword->GetAt(i, j);
            if (ch == ZEN_SPACE || ch == ZEN_BLACK)
                continue;

            multiset.insert(ch);
        }
    }

    std::wstring str;
    for (auto ch : multiset) {
        if (str.size())
            str += L" ";
        // 半角英大文字にする。
        if (ZEN_LARGE_A <= ch && ch <= ZEN_LARGE_Z)
            ch += L'A' - ZEN_LARGE_A;
        str += ch;
    }

    // クリップボードにテキストをコピーする。
    XgSetClipboardUnicodeText(hwnd, str);
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

// 「黒マスパターン」を開く。
void __fastcall XgOpenPatterns(HWND hwnd)
{
    XG_PatternDialog dialog;
    dialog.DoModal(hwnd);
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
    // 黒マス線対称。
    if (xg_nRules & RULE_LINESYMMETRYV) {
        if (!board.IsLineSymmetryV()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRYV), nullptr, MB_ICONERROR);
            return;
        }
    }
    if (xg_nRules & RULE_LINESYMMETRYH) {
        if (!board.IsLineSymmetryH()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRYH), nullptr, MB_ICONERROR);
            return;
        }
    }

    // 合格。
    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_RULESPASSED),
                        XgLoadStringDx2(IDS_PASSED), MB_ICONINFORMATION);
}

// 「テーマ」ダイアログを表示する。
void __fastcall XgTheme(HWND hwnd)
{
    if (xg_tag_histgram.empty()) {
        // タグがありません。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTAGS), NULL, MB_ICONERROR);
        return;
    }

    XG_ThemeDialog dialog;
    INT_PTR id = dialog.DoModal(hwnd);
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
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 成功メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_MADEPROBLEM),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints();
        XgShowHints(hwnd);
    } else {
        // 失敗メッセージを表示する。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANTMAKEPROBLEM),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
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
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    } else {
        // あった。
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), xg_nNumberGenerated,
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    }

    // 終了メッセージを表示する。
    XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

    // 保存先フォルダを開く。
    if (xg_nNumberGenerated && !xg_dirs_save_to.empty())
        ::ShellExecuteW(hwnd, nullptr, xg_dirs_save_to[0].data(),
                        nullptr, nullptr, SW_SHOWNORMAL);
}

// ズーム倍率を設定する。
static void XgSetZoomRate(HWND hwnd, INT nZoomRate)
{
    xg_nZoomRate = nZoomRate;
    INT x = XgGetHScrollPos();
    INT y = XgGetVScrollPos();
    XgUpdateScrollInfo(hwnd, x, y);
    XgUpdateImage(hwnd, x, y);
}

// ルール プリセット。
void XgOnRulePreset(HWND hwnd)
{
    XG_RulePresetDialog dialog;
    if (dialog.DoModal(hwnd) == IDOK)
    {
        XgUpdateRules(hwnd);
    }
}

// 単語リストから生成。
void XgGenerateFromWordList(HWND hwnd)
{
    using namespace crossword_generation;

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();

    // ダイアログを表示。
    INT nID;
    {
        XG_WordListDialog dialog;
        nID = INT(dialog.DoModal(hwnd));
        if (nID != IDOK)
            return;
    }

    // 計算時間を求めるために、開始時間を取得する。
    xg_dwlTick0 = xg_dwlTick1 = ::GetTickCount64();
    // 再計算までの時間を概算する。
    auto size = XG_WordListDialog::s_wordset.size();
    xg_dwlWait = size * size / 3 + 100; // ミリ秒。

    // 単語リストから生成する。
    {
        XG_CancelFromWordsDialog dialog;
        nID = INT(dialog.DoModal(hwnd));
    }
    if (nID == IDCANCEL) {
        // キャンセルされた。
        WCHAR sz[256];
        StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_CANCELLED),
            (xg_dwlTick2 - xg_dwlTick0) / 1000,
            (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
        return;
    }

    if (!s_generated) {
        // 生成できなかった。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTGENERATE), NULL, MB_ICONERROR);
        return;
    }

    // 「元に戻す」情報を取得する。
    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();
    // ルールを補正する。
    xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
    xg_nRules &= ~(RULE_DONTCORNERBLACK | RULE_DONTDOUBLEBLACK | RULE_DONTTRIDIRECTIONS);
    xg_nRules &= ~(RULE_DONTFOURDIAGONALS | RULE_DONTTHREEDIAGONALS);
    xg_nRules |= RULE_DONTDIVIDE;
    XgUpdateRules(xg_hMainWnd);
    // 解をセットする。
    auto& solution = from_words_t<wchar_t, false>::s_solution;
    xg_bSolved = true;
    xg_bShowAnswer = true;
    xg_nRows = solution.m_cy;
    xg_nCols = solution.m_cx;
    xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
    xg_solution.ResetAndSetSize(xg_nRows, xg_nCols);
    xg_dict_1.clear();
    xg_dict_2.clear();
    xg_dict_name.clear();
    for (int y = 0; y < solution.m_cy; ++y) {
        for (int x = 0; x < solution.m_cx; ++x) {
            auto ch = solution.get_at(x, y);
            if (ch == L'#') {
                xg_solution.SetAt(y, x, ZEN_BLACK);
                xg_xword.SetAt(y, x, ZEN_BLACK);
            } else {
                xg_solution.SetAt(y, x, ch);
                xg_xword.SetAt(y, x, ZEN_SPACE);
            }
        }
    }
    // 一時的な辞書をセットする。
    for (auto& word : XG_WordListDialog::s_words) {
        xg_dict_1.emplace_back(word, L"");
    }
    // 番号とヒントを付ける。
    xg_solution.DoNumberingNoCheck();
    XgUpdateHints();
    // テーマをリセットする。
    XgResetTheme(xg_hMainWnd);
    XgUpdateTheme(xg_hMainWnd);
    // 二重マスをリセットする。
    xg_vMarks.clear();
    xg_vMarkedCands.clear();
    XgMarkUpdate();
    // 単語リストを保存して後で使う。
    for (auto& word : XG_WordListDialog::s_words) {
        for (auto& wch : word) {
            if (ZEN_LARGE_A <= wch && wch <= ZEN_LARGE_Z)
                wch = L'a' + (wch - ZEN_LARGE_A);
            else if (ZEN_SMALL_A <= wch && wch <= ZEN_SMALL_Z)
                wch = L'a' + (wch - ZEN_SMALL_A);
        }
        CharLowerBuffW(&word[0], word.size());
    }
    XG_WordListDialog::s_str_word_list = mstr_join(XG_WordListDialog::s_words, L" ");
    // 「元に戻す」情報を設定する。
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa2->Get();
    xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);

    // ズームを実際のウィンドウに合わせる。
    XgFitZoom(hwnd);

    // 成功メッセージ。
    XgShowResults(hwnd);

    // ヒントを表示する。
    XgShowHints(hwnd);

    // クリア。
    XG_WordListDialog::s_words.clear();
    XG_WordListDialog::s_wordset.clear();
}

// ボックスを追加する。
BOOL XgAddBox(HWND hwnd, UINT id)
{
    INT i1 = xg_caret_pos.m_i, j1 = xg_caret_pos.m_j;
    INT i2 = i1 + 2, j2 = j1 + 2;
    if (i2 >= xg_nRows) {
        i1 = xg_nRows - 1;
        i2 = xg_nRows;
    }
    if (j2 >= xg_nCols) {
        j1 = xg_nCols - 1;
        j2 = xg_nCols;
    }

    XG_TextBoxDialog dialog1;
    XG_PictureBoxDialog dialog2;
    switch (id)
    {
    case ID_ADDTEXTBOX:
        if (dialog1.DoModal(hwnd) == IDOK) {
            auto ptr = new XG_TextBoxWindow(i1, j1, i2, j2);
            ptr->SetText(dialog1.m_strText);
            if (ptr->CreateDx(xg_canvasWnd)) {
                xg_boxes.emplace_back(ptr);
                return TRUE;
            } else {
                delete ptr;
            }
        }
        break;
    case ID_ADDPICTUREBOX:
        if (dialog2.DoModal(hwnd) == IDOK) {
            auto ptr = new XG_PictureBoxWindow(i1, j1, i2, j2);
            ptr->SetFile(dialog2.m_strFile);
            if (ptr->CreateDx(xg_canvasWnd)) {
                xg_boxes.emplace_back(ptr);
                return TRUE;
            } else {
                delete ptr;
            }
        }
        break;
    }

    return FALSE;
}

// コマンドを実行する。
void __fastcall MainWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT /*codeNotify*/)
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
            XgUpdateImage(hwnd);
        } else {
            // 一番左のキャレットなら、左端へ移動。
            x = 0;
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_RIGHT:
        // キャレットを右へ移動。
        if (xg_caret_pos.m_j + 1 < xg_nCols) {
            xg_caret_pos.m_j++;
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
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
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_UP:
        // キャレットを上へ移動。
        if (xg_caret_pos.m_i > 0) {
            xg_caret_pos.m_i--;
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        } else {
            // 一番上のキャレットなら、上端へ移動。
            x = XgGetHScrollPos();
            y = 0;
            XgUpdateImage(hwnd, x, y);
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_DOWN:
        // キャレットを下へ移動。
        if (xg_caret_pos.m_i + 1 < xg_nRows) {
            xg_caret_pos.m_i++;
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
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
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTLEFT:
        // Ctrl+←。
        if (xg_caret_pos.m_j > 0) {
            xg_caret_pos.m_j = 0;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        } else {
            // 一番左のキャレットなら、左端へ移動。
            x = 0;
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTRIGHT:
        // Ctrl+→。
        if (xg_caret_pos.m_j + 1 < xg_nCols) {
            xg_caret_pos.m_j = xg_nCols - 1;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
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
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTUPPER:
        // Ctrl+↑。
        if (xg_caret_pos.m_i > 0) {
            xg_caret_pos.m_i = 0;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        } else {
            // 一番上のキャレットなら、上端へ移動。
            x = XgGetHScrollPos();
            y = 0;
            XgUpdateImage(hwnd, x, y);
            XgUpdateCaretPos();
        }
        xg_prev_vk = 0;
        break;
    case ID_MOSTLOWER:
        // Ctrl+↓。
        if (xg_caret_pos.m_i + 1 < xg_nRows) {
            xg_caret_pos.m_i = xg_nRows - 1;
            XgUpdateStatusBar(hwnd);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
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
            XgUpdateCaretPos();
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
        {
            XG_NotesDialog dialog;
            dialog.DoModal(hwnd);
        }
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
        // ボックスをすべて削除する。
        XgDeleteBoxes();
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
                XgSetCaretPos();
                XgMarkUpdate();
                XgUpdateImage(hwnd, 0, 0);
                // メッセージボックスを表示する。
                XgShowResults(hwnd);
            }
            if (!flag) {
                sa1->Apply();
            }
            // イメージを更新する。
            XgSetCaretPos();
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
                XgSetCaretPos();
                XgMarkUpdate();
                XgUpdateImage(hwnd, 0, 0);
                // メッセージボックスを表示する。
                XgShowResults(hwnd);
            }
            if (!flag) {
                sa1->Apply();
            }
            // イメージを更新する。
            XgSetCaretPos();
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
                XgSetCaretPos();
                XgMarkUpdate();
                XgUpdateImage(hwnd, 0, 0);
                // メッセージボックスを表示する。
                XgShowResultsRepeatedly(hwnd);
            }
            // イメージを更新する。
            XgSetCaretPos();
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
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // ボックスをすべて削除する。
            XgDeleteBoxes();
            // 開く。
            if (!XgDoLoad(hwnd, sz)) {
                // 失敗。
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
            } else {
                // 成功。
                xg_ubUndoBuffer.Empty();
                XgSetCaretPos();
                // ズームを実際のウィンドウに合わせる。
                XgFitZoom(hwnd);
                // イメージを更新する。
                XgUpdateImage(hwnd, 0, 0);
                // テーマを更新する。
                XgSetThemeString(xg_strTheme);
                XgUpdateTheme(hwnd);
                // ヒントを表示する。
                XgShowHints(hwnd);
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
        ofn.lpstrDefExt = L"xwj";

        // ファイルの種類を決定する。
        if (lstrcmpiW(PathFindExtensionW(sz), L".xwj") == 0 ||
            lstrcmpiW(PathFindExtensionW(sz), L".json") == 0 ||
            lstrcmpiW(PathFindExtensionW(sz), L".jso") == 0)
        {
            PathRemoveExtensionW(sz);
            ofn.nFilterIndex = 1;
        }
        else if (lstrcmpiW(PathFindExtensionW(sz), L".crp") == 0)
        {
            PathRemoveExtensionW(sz);
            ofn.nFilterIndex = 2;
        }
        else if (lstrcmpiW(PathFindExtensionW(sz), L".xd") == 0)
        {
            PathRemoveExtensionW(sz);
            ofn.nFilterIndex = 3;
        }
        else
        {
            switch (xg_nFileType) {
            case XG_FILETYPE_XWD:
            case XG_FILETYPE_XWJ:
                ofn.nFilterIndex = 1;
                break;
            case XG_FILETYPE_CRP:
                ofn.nFilterIndex = 2;
                break;
            case XG_FILETYPE_XD:
                ofn.nFilterIndex = 3;
                break;
            default:
                ofn.nFilterIndex = 0;
                break;
            }
        }

        // ユーザーにファイルの場所を問い合わせる。
        if (::GetSaveFileNameW(&ofn)) {
            // 保存する。
            if (!XgDoSave(hwnd, sz)) {
                // 保存に失敗。
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            } else {
                // ファイルの種類を保存する。
                if (lstrcmpiW(PathFindExtensionW(sz), L".xwj") == 0 ||
                    lstrcmpiW(PathFindExtensionW(sz), L".json") == 0 ||
                    lstrcmpiW(PathFindExtensionW(sz), L".jso") == 0)
                {
                    xg_nFileType = XG_FILETYPE_XWJ;
                }
                else if (lstrcmpiW(PathFindExtensionW(sz), L".crp") == 0)
                {
                    xg_nFileType = XG_FILETYPE_CRP;
                }
                else if (lstrcmpiW(PathFindExtensionW(sz), L".xd") == 0)
                {
                    xg_nFileType = XG_FILETYPE_XD;
                }
                else
                {
                    if (XgIsUserJapanese())
                        xg_nFileType = XG_FILETYPE_XWJ;
                    else
                        xg_nFileType = XG_FILETYPE_XD;
                }
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
                    XG_HintsWnd::SetHintsData();
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
                    XG_HintsWnd::SetHintsData();
                }
                // イメージを更新する。
                XgUpdateImage(hwnd, 0, 0);
            } else {
                ::MessageBeep(0xFFFFFFFF);
            }
        }
        break;

    case ID_GENERATEBLACKS: // 黒マスパターンを生成。
        // 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
        if (!(xg_nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENROWLINESYMV), nullptr, MB_ICONERROR);
            break;
        }
        // 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
        if (!(xg_nCols & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENCOLLINESYMH), nullptr, MB_ICONERROR);
            break;
        }
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

    case ID_SOLVE:  // 解を求める。
        if (!xg_bSolved && xg_xword.IsFulfilled())
        {
            // 空白マスがない場合は「解を求める」を制限しない。
        }
        else
        {
            // ルール「黒マス点対称」では黒マス追加ありの解を求めることはできません。
            if (xg_nRules & RULE_POINTSYMMETRY) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVESYMMETRY), NULL, MB_ICONERROR);
                return;
            }
            // ルール「黒マス線対称」では黒マス追加ありの解を求めることはできません。
            if (xg_nRules & RULE_LINESYMMETRYV) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), NULL, MB_ICONERROR);
                return;
            }
            if (xg_nRules & RULE_LINESYMMETRYH) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), NULL, MB_ICONERROR);
                return;
            }
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
            sa1->Get();

            // 必要ならルールに従って対称にする。
            XG_Board copy = xg_xword;
            copy.Mirror();
            if (!std::equal(xg_xword.m_vCells.cbegin(), xg_xword.m_vCells.cend(),
                            copy.m_vCells.cbegin()))
            {
                if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_SHALLIMIRROR), NULL,
                                        MB_ICONINFORMATION | MB_YESNO) == IDYES)
                {
                    xg_xword.m_vCells = copy.m_vCells;
                    XgUpdateImage(hwnd, 0, 0);
                }
            }

            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
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
        // ルール「黒マス点対称」では黒マス追加ありの解を求めることはできません。
        if (xg_nRules & RULE_POINTSYMMETRY) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVESYMMETRY), NULL, MB_ICONERROR);
            return;
        }
        // ルール「黒マス線対称」では黒マス追加ありの解を求めることはできません。
        if (xg_nRules & RULE_LINESYMMETRYV) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), NULL, MB_ICONERROR);
            return;
        }
        if (xg_nRules & RULE_LINESYMMETRYH) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), NULL, MB_ICONERROR);
            return;
        }
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();

            // 必要ならルールに従って対称にする。
            XG_Board copy = xg_xword;
            copy.Mirror();
            if (!std::equal(xg_xword.m_vCells.cbegin(), xg_xword.m_vCells.cend(),
                            copy.m_vCells.cbegin()))
            {
                if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_SHALLIMIRROR), NULL,
                                        MB_ICONINFORMATION | MB_YESNO) == IDYES)
                {
                    xg_xword.m_vCells = copy.m_vCells;
                    XgUpdateImage(hwnd, 0, 0);
                }
            }

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
        XgSetCaretPos();
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
        XgSetCaretPos();
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
                xg_cands_wnd,
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
                xg_cands_wnd,
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

    case ID_COPYMARKWORD: // 二重マス単語をコピー。
        XgCopyMarkWord(hwnd);
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
                    XgOnImeChar(hwnd, ch, 0);
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
        XgSetInputMode(hwnd, xg_im_KANA, TRUE);
        break;

    case ID_ABCINPUT:   // 英字入力モード。
        XgSetInputMode(hwnd, xg_im_ABC, TRUE);
        break;

    case ID_KANJIINPUT: // 漢字入力モード。
        XgSetInputMode(hwnd, xg_im_KANJI, TRUE);
        break;

    case ID_RUSSIAINPUT: // ロシア入力モード。
        XgSetInputMode(hwnd, xg_im_RUSSIA, TRUE);
        break;

    case ID_GREEKINPUT: // ギリシャ文字入力モード。
        XgSetInputMode(hwnd, xg_im_GREEK, TRUE);
        break;

    case ID_DIGITINPUT: // 数字入力モード。
        XgSetInputMode(hwnd, xg_im_DIGITS, TRUE);
        break;

    case ID_ANYINPUT: // 自由入力モード。
        XgSetInputMode(hwnd, xg_im_ANY, TRUE);
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
                // イメージを更新する。
                XgUpdateImage(xg_hMainWnd, 0, 0);
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
                // イメージを更新する。
                XgUpdateImage(xg_hMainWnd, 0, 0);
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
        ::ShellExecuteW(hwnd, NULL, XgLoadStringDx1(IDS_HOMEPAGE), NULL, NULL, SW_SHOWNORMAL);
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
        {
            // 「元に戻す」情報を読み込む。
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 文字マスを消す。
            XgClearNonBlocks();
            // 「元に戻す」情報を確定。
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            // イメージを更新する。
            XgUpdateImage(hwnd);
        }
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
        xg_bShowToolBar = !xg_bShowToolBar;
        if (xg_bShowToolBar) {
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
        XgUpdateImage(hwnd);
        break;
    case ID_SHOWHIDECARET:
        xg_bShowCaret = !xg_bShowCaret;
        XgUpdateImage(hwnd);
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
        } else if (xg_nZoomRate > 1) {
            xg_nZoomRate -= 1;
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
    case ID_COPYCHARSET:
        MainWnd_OnCopyCharSet(hwnd);
        break;
    case ID_UPPERCASE:
        xg_bLowercase = FALSE;
        XgUpdateImage(hwnd);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_LOWERCASE:
        xg_bLowercase = TRUE;
        XgUpdateImage(hwnd);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_HIRAGANA:
        xg_bHiragana = TRUE;
        XgUpdateImage(hwnd);
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        break;
    case ID_KATAKANA:
        xg_bHiragana = FALSE;
        XgUpdateImage(hwnd);
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
    case ID_DICTIONARY16:
    case ID_DICTIONARY17:
    case ID_DICTIONARY18:
    case ID_DICTIONARY19:
    case ID_DICTIONARY20:
    case ID_DICTIONARY21:
    case ID_DICTIONARY22:
    case ID_DICTIONARY23:
    case ID_DICTIONARY24:
    case ID_DICTIONARY25:
    case ID_DICTIONARY26:
    case ID_DICTIONARY27:
    case ID_DICTIONARY28:
    case ID_DICTIONARY29:
    case ID_DICTIONARY30:
    case ID_DICTIONARY31:
    case ID_DICTIONARY32:
    case ID_DICTIONARY33:
    case ID_DICTIONARY34:
    case ID_DICTIONARY35:
    case ID_DICTIONARY36:
    case ID_DICTIONARY37:
    case ID_DICTIONARY38:
    case ID_DICTIONARY39:
    case ID_DICTIONARY40:
    case ID_DICTIONARY41:
    case ID_DICTIONARY42:
    case ID_DICTIONARY43:
    case ID_DICTIONARY44:
    case ID_DICTIONARY45:
    case ID_DICTIONARY46:
    case ID_DICTIONARY47:
    case ID_DICTIONARY48:
    case ID_DICTIONARY49:
    case ID_DICTIONARY50:
    case ID_DICTIONARY51:
    case ID_DICTIONARY52:
    case ID_DICTIONARY53:
    case ID_DICTIONARY54:
    case ID_DICTIONARY55:
    case ID_DICTIONARY56:
    case ID_DICTIONARY57:
    case ID_DICTIONARY58:
    case ID_DICTIONARY59:
    case ID_DICTIONARY60:
    case ID_DICTIONARY61:
    case ID_DICTIONARY62:
    case ID_DICTIONARY63:
        XgSelectDict(hwnd, id - ID_DICTIONARY00);
        XgResetTheme(hwnd, FALSE);
        XgUpdateTheme(hwnd);
        break;
    case ID_RESETRULES:
        if (XgIsUserJapanese())
            xg_nRules = DEFAULT_RULES_JAPANESE;
        else
            xg_nRules = DEFAULT_RULES_ENGLISH;
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
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTCORNERBLACK:
        if (xg_nRules & RULE_DONTCORNERBLACK) {
            xg_nRules &= ~RULE_DONTCORNERBLACK;
        } else {
            xg_nRules |= RULE_DONTCORNERBLACK;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTTRIDIRECTIONS:
        if (xg_nRules & RULE_DONTTRIDIRECTIONS) {
            xg_nRules &= ~RULE_DONTTRIDIRECTIONS;
        } else {
            xg_nRules |= RULE_DONTTRIDIRECTIONS;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTDIVIDE:
        //if (xg_nRules & RULE_DONTDIVIDE) {
        //    xg_nRules &= ~RULE_DONTDIVIDE;
        //} else {
        //    xg_nRules |= RULE_DONTDIVIDE;
        //}
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTTHREEDIAGONALS:
        if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~(RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
        } else if (!(xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules |= RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS;
        } else if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && !(xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTTHREEDIAGONALS;
        } else {
            xg_nRules |= (RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_DONTFOURDIAGONALS:
        if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTTHREEDIAGONALS;
            xg_nRules |= RULE_DONTFOURDIAGONALS;
        } else if (!(xg_nRules & RULE_DONTTHREEDIAGONALS) && (xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~(RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
        } else if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && !(xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTFOURDIAGONALS;
        } else {
            xg_nRules |= RULE_DONTFOURDIAGONALS;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_POINTSYMMETRY:
        if (xg_nRules & RULE_POINTSYMMETRY) {
            xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
        } else {
            xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
            xg_nRules |= RULE_POINTSYMMETRY;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_LINESYMMETRYV:
        if (xg_nRules & RULE_LINESYMMETRYV) {
            xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
        } else {
            xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
            xg_nRules |= RULE_LINESYMMETRYV;
        }
        XgUpdateRules(hwnd);
        break;
    case ID_RULE_LINESYMMETRYH:
        if (xg_nRules & RULE_LINESYMMETRYH) {
            xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
        } else {
            xg_nRules &= ~(RULE_POINTSYMMETRY | RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
            xg_nRules |= RULE_LINESYMMETRYH;
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
        xg_bAutoRetry = !xg_bAutoRetry;
        break;
    case ID_SEQPATGEN:
        // 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
        if (!(xg_nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENROWLINESYMV), nullptr, MB_ICONERROR);
            break;
        }
        // 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
        if (!(xg_nCols & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENCOLLINESYMH), nullptr, MB_ICONERROR);
            break;
        }
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
    case ID_VIEW_NORMAL_VIEW:
        // 通常ビューを設定。
        xg_nViewMode = XG_VIEW_NORMAL;
        XgUpdateImage(hwnd);
        break;
    case ID_VIEW_SKELETON_VIEW:
        // 黒マス画像をクリアする。
        xg_strBlackCellImage.clear();
        if (xg_hbmBlackCell) {
            ::DeleteObject(xg_hbmBlackCell);
            xg_hbmBlackCell = NULL;
        }
        if (xg_hBlackCellEMF) {
            DeleteEnhMetaFile(xg_hBlackCellEMF);
            xg_hBlackCellEMF = NULL;
        }
        // スケルトンビューを設定。
        xg_nViewMode = XG_VIEW_SKELETON;
        XgUpdateImage(hwnd);
        break;
    case ID_COPY_WORD_LIST:
        // 単語リストのコピー。
        XgCopyWordList(hwnd);
        break;
    case ID_VIEW_DOUBLEFRAME_LETTERS:
        // 二重マス文字を表示するか？
        xg_bShowDoubleFrameLetters = !xg_bShowDoubleFrameLetters;
        XgUpdateImage(hwnd);
        break;
    case ID_DELETE_ROW:
        if (!xg_bSolved)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            xg_xword.DeleteRow(xg_caret_pos.m_i);
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            XgUpdateImage(hwnd);
        }
        break;
    case ID_DELETE_COLUMN:
        if (!xg_bSolved)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            xg_xword.DeleteColumn(xg_caret_pos.m_j);
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            XgUpdateImage(hwnd);
        }
        break;
    case ID_INSERT_ROW_ABOVE:
        if (!xg_bSolved)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            xg_xword.InsertRow(xg_caret_pos.m_i);
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            XgUpdateImage(hwnd);
        }
        break;
    case ID_INSERT_ROW_BELOW:
        if (!xg_bSolved)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            xg_xword.InsertRow(xg_caret_pos.m_i + 1);
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            XgUpdateImage(hwnd);
        }
        break;
    case ID_LEFT_INSERT_COLUMN:
        if (!xg_bSolved)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            xg_xword.InsertColumn(xg_caret_pos.m_j);
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            XgUpdateImage(hwnd);
        }
        break;
    case ID_RIGHT_INSERT_COLUMN:
        if (!xg_bSolved)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            xg_xword.InsertColumn(xg_caret_pos.m_j + 1);
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
            XgUpdateImage(hwnd);
        }
        break;
    case ID_GENERATEFROMWORDLIST:
        XgGenerateFromWordList(hwnd);
        break;
    case ID_FITZOOM:
        XgFitZoom(hwnd);
        break;
    case ID_RULEPRESET:
        XgOnRulePreset(hwnd);
        break;
    case ID_MOVEBOXES:
        {
            auto it = std::remove_if(xg_boxes.begin(), xg_boxes.end(), [](const std::unique_ptr<XG_BoxWindow>& box){
                return !::IsWindow(*box);
            });
            xg_boxes.erase(it, xg_boxes.end());
        }
        for (auto& box : xg_boxes) {
            box->Bound();
        }
        XgUpdateCaretPos();
        break;
    case ID_ADDTEXTBOX:
    case ID_ADDPICTUREBOX:
        XgAddBox(hwnd, id);
        break;
    case ID_DELETEBOX:
        for (auto it = xg_boxes.begin(); it != xg_boxes.end(); ++it) {
            if ((*it)->m_hWnd == hwndCtl) {
                DestroyWindow(hwndCtl);
                xg_boxes.erase(it);
                break;
            }
        }
        break;
    case ID_BOXPROP:
        for (auto it = xg_boxes.begin(); it != xg_boxes.end(); ++it) {
            if ((*it)->m_hWnd == hwndCtl) {
                (*it)->Prop(hwndCtl);
                break;
            }
        }
        break;
    default:
        if (!XgOnCommandExtra(hwnd, id)) {
            ::MessageBeep(0xFFFFFFFF);
        }
        break;
    }

    XgUpdateStatusBar(hwnd);
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

BOOL xg_bNoGUI = FALSE;

// ウィンドウの作成の際に呼ばれる。
bool __fastcall MainWnd_OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
{
    xg_hMainWnd = hwnd;

    // キャンバスウィンドウを作成する。
    DWORD style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN;
    if (!xg_canvasWnd.CreateWindowDx(hwnd, NULL, style, WS_EX_ACCEPTFILES))
    {
        return false;
    }

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

    if (xg_bShowToolBar)
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
        // 変換コマンドラインか？
        if (argc == 4 && lstrcmpiW(wargv[1], L"--convert") == 0) {
            // 読み込んで名前を付けて保存する。
            auto input = wargv[2];
            auto output = wargv[3];
            if (XgDoLoad(hwnd, input) && XgDoSave(hwnd, output)) {
                PostQuitMessage(0);
            } else {
                PostQuitMessage(-1);
            }
            xg_bNoGUI = TRUE;
            return true;
        }

        WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
        StringCbCopy(szFile, sizeof(szFile), wargv[1]);

        // コマンドライン引数があれば、それを開く。
        bool bSuccess = true;
        if (::lstrcmpiW(PathFindExtensionW(szFile), L".LNK") == 0)
        {
            // ショートカットだった場合は、ターゲットのパスを取得する。
            if (XgGetPathOfShortcutW(szFile, szTarget)) {
                StringCbCopy(szFile, sizeof(szFile), szTarget);
            } else {
                bSuccess = false;
                MessageBeep(0xFFFFFFFF);
            }
        }
        if (bSuccess) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // ファイルを開く。
            if (!XgDoLoad(hwnd, szFile)) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
            } else {
                XgSetCaretPos();
                // ズームを実際のウィンドウに合わせる。
                XgFitZoom(hwnd);
                // テーマを更新する。
                XgSetThemeString(xg_strTheme);
                XgUpdateTheme(hwnd);
                // イメージを更新する。
                XgUpdateImage(hwnd, 0, 0);
                // ヒントを表示する。
                XgShowHints(hwnd);
            }
            // ルールを更新する。
            XgUpdateRules(hwnd);
        }
    }
    GlobalFree(wargv);

    // コントロールのレイアウトを更新する。
    ::SendMessageW(hwnd, WM_SIZE, 0, 0);
    // ルールを更新する。
    XgUpdateRules(hwnd);
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
    // 辞書メニューの表示を更新。
    XgUpdateTheme(hwnd);

    return true;
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
        DeleteMenu(hSubMenu, 9, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_KANA:
        DeleteMenu(hSubMenu, 9, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        break;
    case xg_im_KANJI:
        DeleteMenu(hSubMenu, 9, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_RUSSIA:
        DeleteMenu(hSubMenu, 9, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_GREEK:
        DeleteMenu(hSubMenu, 8, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_DIGITS:
        DeleteMenu(hSubMenu, 9, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 7, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 6, MF_BYPOSITION);
        DeleteMenu(hSubMenu, 5, MF_BYPOSITION);
        break;
    case xg_im_ANY:
        break;
    default:
        break;
    }

    return hMenu;
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

// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd, INT x, INT y)
{
    ForDisplay for_display;

    // イメージがあれば破棄する。
    if (xg_hbmImage)
        ::DeleteObject(xg_hbmImage);

    // 描画サイズを取得し、イメージを作成する。
    SIZE siz;
    XgGetXWordExtent(&siz);
    if (xg_bSolved && xg_bShowAnswer)
        xg_hbmImage = XgCreateXWordImage(xg_solution, &siz, true);
    else
        xg_hbmImage = XgCreateXWordImage(xg_xword, &siz, true);

    // スクロール情報を更新する。
    XgUpdateScrollInfo(hwnd, x, y);

    // 再描画する。
    MRect rcClient;
    XgGetRealClientRect(hwnd, &rcClient);
    ::InvalidateRect(xg_canvasWnd, &rcClient, TRUE);
}

// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd)
{
    XgUpdateImage(hwnd, XgGetHScrollPos(), XgGetVScrollPos());
}

// ファイルがドロップされた。
void MainWnd_OnDropFiles(HWND hwnd, HDROP hdrop)
{
    POINT pt;
    DragQueryPoint(hdrop, &pt);
    ClientToScreen(hwnd, &pt);

    xg_canvasWnd.DoDropFile(xg_canvasWnd, hdrop, pt);
}

// ウィンドウプロシージャ。
extern "C"
LRESULT CALLBACK
XgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, MainWnd_OnCreate);
    HANDLE_MSG(hWnd, WM_DESTROY, MainWnd_OnDestroy);
    HANDLE_MSG(hWnd, WM_MOVE, MainWnd_OnMove);
    HANDLE_MSG(hWnd, WM_SIZE, MainWnd_OnSize);
    HANDLE_MSG(hWnd, WM_KEYDOWN, XgOnKey);
    HANDLE_MSG(hWnd, WM_KEYUP, XgOnKey);
    HANDLE_MSG(hWnd, WM_CHAR, XgOnChar);
    HANDLE_MSG(hWnd, WM_COMMAND, MainWnd_OnCommand);
    HANDLE_MSG(hWnd, WM_INITMENU, MainWnd_OnInitMenu);
    HANDLE_MSG(hWnd, WM_DROPFILES, MainWnd_OnDropFiles);
    HANDLE_MSG(hWnd, WM_GETMINMAXINFO, MainWnd_OnGetMinMaxInfo);
    case WM_NOTIFY:
        MainWnd_OnNotify(hWnd, static_cast<int>(wParam), reinterpret_cast<LPNMHDR>(lParam));
        break;

    case WM_IME_CHAR:
        XgOnImeChar(hWnd, static_cast<WCHAR>(wParam), lParam);
        break;

    case XGWM_HIGHLIGHT:
        if (wParam) {
            xg_highlight.m_number = (SHORT)LOWORD(lParam);
            xg_highlight.m_vertical = !!HIWORD(lParam);
            XgUpdateImage(hWnd);
        }
        break;

    default:
        return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// ヒントウィンドウ。

// ヒントウィンドウを作成する。
BOOL XgCreateHintsWnd(HWND hwnd)
{
    auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
    auto exstyle = WS_EX_TOOLWINDOW;
    auto text = XgLoadStringDx1(IDS_HINTS);

    // ヒントウィンドウの初期位置を改良する。
    if (XG_HintsWnd::s_nHintsWndX == CW_USEDEFAULT) {
        RECT rcMain;
        GetWindowRect(xg_hMainWnd, &rcMain);
        HMONITOR hMon = ::MonitorFromWindow(xg_hMainWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO info = { sizeof(info) };
        RECT rcWork;
        if (::GetMonitorInfoW(hMon, &info))
            rcWork = info.rcWork;
        else
            ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
        if (rcMain.right + XG_HintsWnd::s_nHintsWndCX < rcWork.right) {
            XG_HintsWnd::s_nHintsWndX = rcMain.right;
            XG_HintsWnd::s_nHintsWndY = rcMain.top;
        } else if (rcWork.left + XG_HintsWnd::s_nHintsWndCX <= rcMain.left) {
            XG_HintsWnd::s_nHintsWndX = rcMain.left - XG_HintsWnd::s_nHintsWndCX;
            XG_HintsWnd::s_nHintsWndY = rcMain.top;
        } else if (rcWork.top + XG_HintsWnd::s_nHintsWndCY < rcMain.top) {
            XG_HintsWnd::s_nHintsWndX = rcMain.left;
            XG_HintsWnd::s_nHintsWndY = rcMain.top - XG_HintsWnd::s_nHintsWndCY;
        } else if (rcMain.bottom + XG_HintsWnd::s_nHintsWndCY < rcWork.bottom) {
            XG_HintsWnd::s_nHintsWndX = rcMain.left;
            XG_HintsWnd::s_nHintsWndY = rcMain.bottom;
        }
    }

    xg_hints_wnd.CreateWindowDx(hwnd, text, style, exstyle,
                                XG_HintsWnd::s_nHintsWndX, XG_HintsWnd::s_nHintsWndY,
                                XG_HintsWnd::s_nHintsWndCX, XG_HintsWnd::s_nHintsWndCY);
    if (xg_hHintsWnd)
    {
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

// ヒントの内容をヒントウィンドウで開く。
bool XgOpenHintsByWindow(HWND hwnd)
{
    // もしヒントウィンドウが存在すれば破棄する。
    if (xg_hHintsWnd) {
        HWND hwnd = xg_hHintsWnd;
        xg_hHintsWnd = NULL;
        ::DestroyWindow(hwnd);
    }

    // ヒントが空なら開かない。
    if (!xg_bSolved)
        return false;

    // ヒントウィンドウを作成する。
    if (XgCreateHintsWnd(xg_hMainWnd)) {
        ::ShowWindow(xg_hHintsWnd, SW_SHOWNOACTIVATE);
        ::SetForegroundWindow(xg_hMainWnd);
        return true;
    }
    return false;
}

// ヒントを表示する。
void XgShowHints(HWND hwnd)
{
    #if 1
        XgOpenHintsByWindow(hwnd);
    #else
        XgOpenHintsByNotepad(hwnd, xg_bShowAnswer);
    #endif
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
// テスト

void XgDoTests(void)
{
#ifndef NDEBUG
    std::wstring str;

    // xg_str_trim
    str = L"  \t";
    xg_str_trim(str);
    assert(str == L"");
    str = L"ABC";
    xg_str_trim(str);
    assert(str == L"ABC");
    str = L" ABC";
    xg_str_trim(str);
    assert(str == L"ABC");
    str = L"ABC ";
    xg_str_trim(str);
    assert(str == L"ABC");
    str = L"  ABC  ";
    xg_str_trim(str);
    assert(str == L"ABC");

    // xg_str_trim_right
    str = L"  \t";
    xg_str_trim_right(str);
    assert(str == L"");
    str = L"ABC";
    xg_str_trim_right(str);
    assert(str == L"ABC");
    str = L" ABC";
    xg_str_trim_right(str);
    assert(str == L" ABC");
    str = L"ABC ";
    xg_str_trim_right(str);
    assert(str == L"ABC");
    str = L"  ABC  ";
    xg_str_trim_right(str);
    assert(str == L"  ABC");
#endif
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
    // テストをする。
    XgDoTests();

    // アプリのインスタンスを保存する。
    xg_hInstance = hInstance;

    // 設定を読み込む。
    XgLoadSettings();

    // 辞書ファイルの名前を読み込む。
    XgLoadDictsAll();

    // 乱数モジュールを初期化する。
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());

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
    if (!xg_hints_wnd.RegisterClassDx())
    {
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
        return 1;
    }
    if (!xg_cands_wnd.RegisterClassDx()) {
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
        return 1;
    }
    if (!xg_canvasWnd.RegisterClassDx()) {
        // ウィンドウ登録失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
        return 1;
    }
    {
        XG_BoxWindow box(L"");
        if (!box.RegisterClassDx()) {
            // ウィンドウ登録失敗メッセージ。
            XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTREGWND), nullptr, MB_ICONERROR);
            return 1;
        }
    }

    // クリティカルセクションを初期化する。
    ::InitializeCriticalSection(&xg_cs);

    // メインウィンドウを作成する。
    DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    ::CreateWindowW(s_pszMainWndClass, XgLoadStringDx1(IDS_APPINFO), style,
        s_nMainWndX, s_nMainWndY, s_nMainWndCX, s_nMainWndCY,
        nullptr, nullptr, hInstance, nullptr);
    if (xg_hMainWnd == nullptr) {
        // ウィンドウ作成失敗メッセージ。
        XgCenterMessageBoxW(nullptr, XgLoadStringDx1(IDS_CANTMAKEWND), nullptr, MB_ICONERROR);
        return 2;
    }

    if (!xg_bNoGUI) {
        // 表示する。
        ::ShowWindow(xg_hMainWnd, nCmdShow);
        ::UpdateWindow(xg_hMainWnd);
    }

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

        if (xg_cands_wnd) {
            if (msg.message != WM_KEYDOWN || msg.wParam != VK_ESCAPE) {
                if (::IsDialogMessageW(xg_cands_wnd, &msg))
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
