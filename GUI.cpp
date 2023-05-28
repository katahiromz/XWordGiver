﻿//////////////////////////////////////////////////////////////////////////////
// GUI.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#define NOMINMAX
#include "XWordGiver.hpp"
#include "GUI.hpp"
#include <algorithm>
#include "WonSetThreadUILanguage/WonSetThreadUILanguage.h"

// 「元に戻す」情報。
#include "XG_UndoBuffer.hpp"
#include "XG_UndoBuffer.cpp"

// ダイアログとウィンドウ。
#include "XG_Window.cpp"
#include "XG_CancelFromWordsDialog.hpp"
#include "XG_CancelGenBlacksDialog.hpp"
#include "XG_CancelSmartSolveDialog.hpp"
#include "XG_CancelSolveDialog.hpp"
#include "XG_CancelSolveNoAddBlackDialog.hpp"
#include "XG_CandsWnd.hpp"
#include "XG_GenDialog.hpp"
#include "XG_HintsWnd.hpp"
#include "XG_MarkingDialog.hpp"
#include "XG_NewDialog.hpp"
#include "XG_NotesDialog.hpp"
#include "XG_PatGenDialog.hpp"
#include "XG_PatternDialog.hpp"
#include "XG_SeqGenDialog.hpp"
#include "XG_SeqPatGenDialog.hpp"
#include "XG_SeqSolveDialog.hpp"
#include "XG_SettingsDialog.hpp"
#include "XG_SettingsDialog.cpp"
#include "XG_ThemeDialog.hpp"
#include "XG_WordListDialog.hpp"
#include "XG_RulePresetDialog.hpp"
#include "XG_BoxWindow.hpp"
#include "XG_CanvasWindow.hpp"
#include "XG_PictureBoxDialog.hpp"
#include "XG_TextBoxDialog.hpp"
#include "XG_BoxWindow.hpp"
#include "XG_CanvasWindow.hpp"
#include "XG_UILanguageDialog.hpp"

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

// キャンバスウィンドウ。
HWND xg_hCanvasWnd = nullptr;

// ヒントウィンドウのハンドル。
HWND xg_hHintsWnd = nullptr;
XG_HintsWnd xg_hints_wnd;

// ヒントを表示するかどうか。
BOOL xg_bShowClues = TRUE;

// 候補ウィンドウ。
XG_CandsWnd xg_cands_wnd;

// [二重マス単語の候補と配置]ダイアログ。
XG_MarkingDialog xg_hMarkingDlg;

// 入力パレット。
HWND xg_hwndInputPalette = nullptr;

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
dicts_t xg_dicts;

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
int xg_nForDisplay = 0;

// ズーム比率(%)。
int xg_nZoomRate = 100;

// 番号を表示するか？
BOOL xg_bShowNumbering = TRUE;
// キャレットを表示するか？
BOOL xg_bShowCaret = TRUE;
// 最大化するか？
BOOL xg_bMainWndMaximized = FALSE;

// 二重マス文字を表示するか？
BOOL xg_bShowDoubleFrameLetters = TRUE;

// 保存先のパスのリスト。
std::deque<std::wstring> xg_dirs_save_to;

// 連続生成の場合、問題を生成する数。
int xg_nNumberToGenerate = 16;

// マウスの中央ボタンの処理に使う変数。
BOOL xg_bMButtonDragging = FALSE;
POINT xg_ptMButtonDragging;

// ファイルの種類。
XG_FILETYPE xg_nFileType = XG_FILETYPE_XWJ;

// ハイライト情報。
XG_HighLight xg_highlight = { -1, FALSE };

// ボックス。
boxes_t xg_boxes;

// キャンバスウィンドウ。
XG_CanvasWindow xg_canvasWnd;

// ナンクロモードか？
bool xg_bNumCroMode = false;
// ナンクロモードの場合、写像を保存する。
std::unordered_map<WCHAR, int> xg_mapNumCro1;
std::unordered_map<int, WCHAR> xg_mapNumCro2;

// ファイル変更フラグ。
BOOL xg_bFileModified = FALSE;

// ファイル変更フラグ。
void XgSetModified(BOOL bModified, LPCSTR file, int line)
{
    xg_bFileModified = bModified;

    if (bModified) {
        CHAR szText[MAX_PATH];
        StringCchPrintfA(szText, _countof(szText), "%s: %d", file, line);
        //MessageBoxA(nullptr, szText, nullptr, 0);
    }

    XgMarkUpdate();
}

// UI言語。
LANGID xg_UILangID = 0;

// 最近使ったファイルのリスト。
std::vector<std::wstring> xg_recently_used_files;

#define XG_MAX_RECENT 10 // 最近使ったファイルは10個まで。

// 最近使ったファイルを更新する。
void XgUpdateRecentlyUsed(LPCWSTR pszFile)
{
    // 安全に操作するため、いったん格納。
    std::wstring file = pszFile;
    // 重複は許さない。
    for (size_t i = 0; i < xg_recently_used_files.size(); ++i)
    {
        // 大文字小文字の違いを無視。
        if (lstrcmpiW(xg_recently_used_files[i].c_str(), file.c_str()) == 0)
        {
            xg_recently_used_files.erase(std::cbegin(xg_recently_used_files) + i);
            break;
        }
    }
    if (xg_recently_used_files.size() > XG_MAX_RECENT)
        xg_recently_used_files.resize(XG_MAX_RECENT);
    // 先頭に追加。
    xg_recently_used_files.emplace(xg_recently_used_files.begin(), file.c_str());
}

//////////////////////////////////////////////////////////////////////////////
// static variables

// メインウィンドウの位置とサイズ。
static int s_nMainWndX = CW_USEDEFAULT, s_nMainWndY = CW_USEDEFAULT;
static int s_nMainWndCX = CW_USEDEFAULT, s_nMainWndCY = CW_USEDEFAULT;

// 入力パレットの位置。
int xg_nInputPaletteWndX = CW_USEDEFAULT;
int xg_nInputPaletteWndY = CW_USEDEFAULT;

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
int xg_nNumberGenerated = 0;

// 再計算の回数。
LONG xg_nRetryCount;

// ステータスバーを表示するか？
static bool s_bShowStatusBar = true;

// ルール群。
int xg_nRules = XG_DEFAULT_RULES;

// [二重マス単語の候補と配置]ダイアログの位置。
int xg_nMarkingX = CW_USEDEFAULT;
int xg_nMarkingY = CW_USEDEFAULT;

//////////////////////////////////////////////////////////////////////////////
// スクロール関連。

// マス位置を取得する。
VOID XgGetCellPosition(RECT& rc, int i1, int j1, int i2, int j2, BOOL bScroll) noexcept
{
    const int nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    rc = {
        xg_nMargin + j1 * nCellSize,
        xg_nMargin + i1 * nCellSize,
        xg_nMargin + j2 * nCellSize,
        xg_nMargin + i2 * nCellSize
    };

    if (bScroll)
        ::OffsetRect(&rc, -XgGetHScrollPos(), -XgGetVScrollPos());
}

// マス位置を設定する。
VOID XgSetCellPosition(LONG& x, LONG& y, int& i, int& j, BOOL bEnd) noexcept
{
    int nCellSize = xg_nCellSize * xg_nZoomRate / 100;

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

// ボックスをすべて削除する。
void XgDeleteBoxes(void) noexcept
{
    for (auto& box : xg_boxes) {
        DestroyWindow(*box);
    }
    xg_boxes.clear();
}

// 画像群を読み込む。
bool XgLoadImageFiles(void)
{
    for (auto& box : xg_boxes) {
        if (box->m_type == L"pic") {
            auto pic = dynamic_cast<XG_PictureBoxWindow*>(&*box);
            XgGetFileManager()->load_image(pic->GetText().c_str());
        }
    }
    return true;
}

// パス情報を変換する。
bool XgConvertBoxes(void)
{
    for (auto& box : xg_boxes) {
        if (box->m_type == L"pic") {
            auto pic = dynamic_cast<XG_PictureBoxWindow*>(&*box);
            XgGetFileManager()->convert(pic->m_strText);
        }
    }
    return true;
}

// ボックス群を文字列化する。
std::wstring XgStringifyBoxes(const boxes_t& boxes)
{
    std::wstring ret;

    for (auto& box : xg_boxes) {
        // ボックスの内部データを取り出す。
        XG_BoxWindow::map_t map;
        box->WriteMap(map);

        // 読み書きする。
        bool first = true;
        for (auto& pair : map) {
            if (!first)
                ret += L", ";
            auto quoted = xg_str_quote(pair.second);
            ret += L"{{";
            ret += pair.first.c_str();
            ret += L": ";
            ret += quoted.c_str();
            ret += L"}}";
            first = false;
        }
        ret += L"\n";
    }
    return ret;
}

// ボックス群を逆文字列化する。
void XgDeStringifyBoxes(const std::wstring& boxes)
{
    // ボックスをいったん破棄する。
    XgDeleteBoxes();

    // "\n"で文字列を分割する。
    std::vector<std::wstring> strs;
    mstr_split(strs, boxes, L"\n");

    for (auto& str : strs) {
        if (str.empty())
            continue;

        XG_BoxWindow::map_t map;

        while (str.size()) {
            mstr_trim(str, L" \t\r\n");

            const size_t index1 = str.find(L"{{");
            const size_t index2 = str.find(L"}}", index1);
            if (index1 == str.npos || index2 == str.npos)
                break;

            auto contents = str.substr(index1 + 2, index2 - index1 - 2);
            const size_t index3 = contents.find(L':');
            if (index3 == contents.npos)
                break;

            auto tag = contents.substr(0, index3);
            auto value = contents.substr(index3 + 1);
            xg_str_trim(tag);
            xg_str_trim(value);
            map[tag] = xg_str_unquote(value);

            str = str.substr(index2 + 2);
            xg_str_trim(str);
        }

        auto type_it = map.find(L"type");
        if (type_it == map.end())
            continue;

        auto type = type_it->second;
        if (type == L"pic") {
            auto ptr = std::make_shared<XG_PictureBoxWindow>();
            ptr->ReadMap(map);
            if (ptr->CreateDx(xg_canvasWnd)) {
                xg_boxes.emplace_back(ptr);
            }
        } else if (type == L"text") {
            auto ptr = std::make_shared<XG_TextBoxWindow>();
            ptr->ReadMap(map);
            if (ptr->CreateDx(xg_canvasWnd)) {
                xg_boxes.emplace_back(ptr);
            }
        }
    }

    // ボックスの位置を更新。
    PostMessageW(xg_hMainWnd, WM_COMMAND, ID_MOVEBOXES, 0);
}

// 使われている画像を取得する。
std::unordered_set<std::wstring> XgGetUsedImages(void)
{
    std::unordered_set<std::wstring> ret;
    for (auto& box : xg_boxes) {
        if (box->m_type == L"pic")
        {
            auto pic = dynamic_cast<XG_PictureBoxWindow*>(&*box);
            ret.insert(pic->GetText());
        }
    }
    ret.insert(xg_strBlackCellImage);
    return ret;
}

// ボックスJSONを読み込む。
BOOL XgDoLoadBoxJson(const json& boxes)
{
    try {
        for (auto& box : boxes) {
            if (box["type"] == "pic") {
                auto ptr = std::make_shared<XG_PictureBoxWindow>();
                ptr->ReadJson(box);
                if (ptr->CreateDx(xg_canvasWnd)) {
                    xg_boxes.push_back(ptr);
                    continue;
                }
            } else if (box["type"] == "text") {
                auto ptr = std::make_shared<XG_TextBoxWindow>();
                ptr->ReadJson(box);
                if (ptr->CreateDx(xg_canvasWnd)) {
                    xg_boxes.push_back(ptr);
                    continue;
                }
            }
        }
    } catch(...) {
        return FALSE;
    }

    return TRUE;
}

// ボックスJSONを保存。
BOOL XgDoSaveBoxJson(json& j)
{
    try
    {
        for (auto& box : xg_boxes) {
            box->WriteJson(j);
        }
    }
    catch(...)
    {
        return FALSE;
    }

    return TRUE;
}

// ボックスXDを読み込む。
BOOL XgLoadXdBox(const std::wstring& line)
{
    if (line.find(L"Box:") != 0)
        return FALSE;

    std::wstring str;
    str = line.substr(4);
    xg_str_trim(str);

    std::wstring type;
    if (str.find(L"{{") != str.npos && str.find(L"}}") != str.npos) {
        const size_t index0 = str.find(L"{{type:");
        const size_t index1 = str.find(L"}}", index0 + 7);
        if (index0 != str.npos && index1 != str.npos)
        {
            type = str.substr(index0 + 7, index1 - index0 - 7);
            xg_str_trim(type);
            type = xg_str_unquote(type);
        }
    } else {
        const size_t index0 = str.find(L":");
        type = str.substr(0, index0);
        xg_str_trim(type);
    }


    if (type == L"pic") {
        auto ptr = std::make_shared<XG_PictureBoxWindow>();
        ptr->ReadLine(line);
        if (ptr->CreateDx(xg_canvasWnd)) {
            xg_boxes.emplace_back(ptr);
            return TRUE;
        }
    } else if (type == L"text") {
        auto ptr = std::make_shared<XG_TextBoxWindow>();
        ptr->ReadLine(line);
        if (ptr->CreateDx(xg_canvasWnd)) {
            xg_boxes.emplace_back(ptr);
            return TRUE;
        }
    }

    return FALSE;
}

// XDファイルへボックスを書き込む。
BOOL XgWriteXdBoxes(FILE *fout)
{
    for (auto& box : xg_boxes) {
        box->WriteLine(fout);
    }
    return TRUE;
}

// ボックスを描画する。
void XgDrawBoxes(HDC hdc, const SIZE *psiz)
{
    const int nZoomRate = xg_nZoomRate;
    xg_nZoomRate = 100;
    RECT rc;
    for (auto& pbox : xg_boxes) {
        auto& box = *pbox;
        XgGetCellPosition(rc, box.m_i1, box.m_j1, box.m_i2, box.m_j2, FALSE);
        box.OnDraw(box, hdc, rc);
    }
    xg_nZoomRate = nZoomRate;
}

// 本当のクライアント領域を計算する。
void __fastcall XgGetRealClientRect(HWND hwnd, LPRECT prcClient) noexcept
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
    const int nCellSize = xg_nCellSize * xg_nZoomRate / 100;

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
        StringCchCatW(lf.lfFaceName, _countof(lf.lfFaceName), xg_szCellFont);
    else
        StringCchCatW(lf.lfFaceName, _countof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
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

    // ヒントウィンドウのハイライトを設定する。
    int nYoko = -1, nTate = -1;
    for (auto& info : xg_vTateInfo) {
        if (info.m_iRow == xg_caret_pos.m_i && info.m_jCol == xg_caret_pos.m_j) {
            nTate = info.m_number;
            break;
        }
    }
    for (auto& info : xg_vYokoInfo) {
        if (info.m_iRow == xg_caret_pos.m_i && info.m_jCol == xg_caret_pos.m_j) {
            nYoko = info.m_number;
            break;
        }
    }
    xg_hints_wnd.setHighlight(nYoko, nTate);
}

// キャレット位置をセットする。
void __fastcall XgSetCaretPos(int iRow, int jCol)
{
    xg_caret_pos.m_i = iRow;
    xg_caret_pos.m_j = jCol;
    XgUpdateCaretPos();
}

// スクロール情報を設定する。
void __fastcall XgUpdateScrollInfo(HWND hwnd, int x, int y) noexcept
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

    // クライアント領域を取得する。
    XgGetRealClientRect(hwnd, &rcClient);

    const int nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    // キャレットの矩形を設定する。
    rc = {
        xg_nMargin + xg_caret_pos.m_j * nCellSize,
        xg_nMargin + xg_caret_pos.m_i * nCellSize,
        xg_nMargin + (xg_caret_pos.m_j + 1) * nCellSize,
        xg_nMargin + (xg_caret_pos.m_i + 1) * nCellSize
    };

    // 横スクロール情報を修正する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    XgGetHScrollInfo(&si);
    if (rc.left < si.nPos) {
        XgSetHScrollPos(rc.left, TRUE);
    } else if (rc.right > static_cast<int>(si.nPos + si.nPage)) {
        XgSetHScrollPos(rc.right - si.nPage, TRUE);
    }

    // 縦スクロール情報を修正する。
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    XgGetVScrollInfo(&si);
    if (rc.top < si.nPos) {
        XgSetVScrollPos(rc.top, TRUE);
    } else if (rc.bottom > static_cast<int>(si.nPos + si.nPage)) {
        XgSetVScrollPos(rc.bottom - si.nPage, TRUE);
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

    ::InvalidateRect(xg_hCanvasWnd, NULL, TRUE);

    XgUpdateCaretPos();
}

// 現在の状態で好ましいと思われる単語の最大長を取得する。
// 旧来のスマート解決では最大長を指定する必要があった。
int __fastcall XgGetPreferredMaxLength(void) noexcept
{
    int ret = 7;

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
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_UNDO, xg_ubUndoBuffer.CanUndo());
    ::SendMessageW(xg_hToolBar, TB_ENABLEBUTTON, ID_REDO, xg_ubUndoBuffer.CanRedo());
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

    xg_nCellCharPercents = XG_DEF_CELL_CHAR_SIZE;
    xg_nSmallCharPercents = XG_DEF_SMALL_CHAR_SIZE;

    xg_strBlackCellImage.clear();

    XG_PatternDialog::xg_nPatWndX = CW_USEDEFAULT;
    XG_PatternDialog::xg_nPatWndY = CW_USEDEFAULT;
    XG_PatternDialog::xg_nPatWndCX = CW_USEDEFAULT;
    XG_PatternDialog::xg_nPatWndCY = CW_USEDEFAULT;
    XG_PatternDialog::xg_bShowAnswerOnPattern = TRUE;

    xg_nRules = XG_DEFAULT_RULES;
    xg_nViewMode = XG_VIEW_NORMAL;
    xg_nFileType = XG_FILETYPE_XD;
    xg_nLineWidthInPt = XG_LINE_WIDTH_DEFAULT;

    xg_nMarkingX = xg_nMarkingY = CW_USEDEFAULT;

    xg_imode = xg_im_ANY; // 自由入力。

    xg_nNumberToGenerate = 16;

    xg_UILangID = 0;

    xg_strDoubleFrameLetters = XgLoadStringDx1(IDS_DBLFRAME_LETTERS_1);

    xg_recently_used_files.clear();

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
            if (!app_key.QueryDword(L"MainWndMaximized", dwValue)) {
                xg_bMainWndMaximized = !!dwValue;
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
                switch (static_cast<XG_FILETYPE>(dwValue))
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
            if (!app_key.QueryDword(L"UILangID", dwValue)) {
                xg_UILangID = static_cast<LANGID>(dwValue);
            }

            if (!app_key.QuerySz(L"CellFont", sz, _countof(sz))) {
                StringCchCopy(xg_szCellFont, _countof(xg_szCellFont), sz);
            }
            if (!app_key.QuerySz(L"SmallFont", sz, _countof(sz))) {
                StringCchCopy(xg_szSmallFont, _countof(xg_szSmallFont), sz);
            }
            if (!app_key.QuerySz(L"UIFont", sz, _countof(sz))) {
                StringCchCopy(xg_szUIFont, _countof(xg_szUIFont), sz);
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
                xg_imode = static_cast<XG_InputMode>(dwValue);
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
            if (!app_key.QueryDword(L"MarkingX", dwValue)) {
                xg_nMarkingX = dwValue;
            }
            if (!app_key.QueryDword(L"MarkingY", dwValue)) {
                xg_nMarkingY = dwValue;
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
            if (!app_key.QueryDword(L"LineWidth", dwValue)) {
                float value = static_cast<float>(dwValue) * 0.01f;
                if (value < XG_MIN_LINEWIDTH)
                    value = XG_MIN_LINEWIDTH;
                if (value > XG_MAX_LINEWIDTH)
                    value = XG_MAX_LINEWIDTH;
                xg_nLineWidthInPt = value;
            }
            if (!app_key.QueryDword(L"OuterFrame", dwValue)) {
                float value = static_cast<float>(dwValue) * 0.01f;
                if (value < XG_MIN_OUTERFRAME)
                    value = XG_MIN_OUTERFRAME;
                if (value > XG_MAX_OUTERFRAME)
                    value = XG_MAX_OUTERFRAME;
                xg_nOuterFrameInPt = value;
            }

            if (!app_key.QuerySz(L"Recent", sz, _countof(sz))) {
                xg_dict_name = sz;
                if (!PathFileExists(xg_dict_name.c_str()))
                {
                    xg_dict_name.clear();
                }
            }

            if (!app_key.QuerySz(L"BlackCellImage", sz, _countof(sz))) {
                xg_strBlackCellImage = sz;
                XgGetFileManager()->load_block_image(sz);
            }

            if (!app_key.QuerySz(L"DoubleFrameLetters", sz, _countof(sz))) {
                xg_strDoubleFrameLetters = sz;
            }

            // 保存先のリストを取得する。
            if (!app_key.QueryDword(L"SaveToCount", dwValue)) {
                nDirCount = dwValue;
                for (i = 0; i < nDirCount; i++) {
                    StringCchPrintf(szFormat, _countof(szFormat), L"SaveTo %d", i + 1);
                    if (!app_key.QuerySz(szFormat, sz, _countof(sz))) {
                        xg_dirs_save_to.emplace_back(sz);
                    } else {
                        nDirCount = i;
                        break;
                    }
                }
            }

            // 最近使ったファイルのリストを取得する。
            if (!app_key.QueryDword(L"Recents", dwValue)) {
                for (i = 0; i < static_cast<int>(dwValue); i++) {
                    StringCchPrintf(szFormat, _countof(szFormat), L"Recent %d", i);
                    if (!app_key.QuerySz(szFormat, sz, _countof(sz))) {
                        if (PathFileExistsW(sz))
                            xg_recently_used_files.emplace_back(sz);
                    } else {
                        break;
                    }
                }
                if (xg_recently_used_files.size() > XG_MAX_RECENT)
                    xg_recently_used_files.resize(XG_MAX_RECENT);
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

    if (xg_nRows <= 1)
        xg_nRows = 2;
    if (xg_nCols <= 1)
        xg_nCols = 2;
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
            app_key.SetDword(L"UILangID", xg_UILangID);

            app_key.SetSz(L"CellFont", xg_szCellFont, _countof(xg_szCellFont));
            app_key.SetSz(L"SmallFont", xg_szSmallFont, _countof(xg_szSmallFont));
            app_key.SetSz(L"UIFont", xg_szUIFont, _countof(xg_szUIFont));

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
            app_key.SetDword(L"InputMode", static_cast<DWORD>(xg_imode));
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
            app_key.SetDword(L"MarkingX", xg_nMarkingX);
            app_key.SetDword(L"MarkingY", xg_nMarkingY);

            app_key.SetDword(L"ShowDoubleFrameLetters", xg_bShowDoubleFrameLetters);
            app_key.SetDword(L"ViewMode", xg_nViewMode);
            app_key.SetDword(L"LineWidth", static_cast<int>(xg_nLineWidthInPt * 100));
            app_key.SetDword(L"OuterFrame", static_cast<int>(xg_nOuterFrameInPt * 100));

            app_key.SetSz(L"Recent", xg_dict_name.c_str());

            if (xg_strBlackCellImage.find(L"$FILES\\") != 0)
                app_key.SetSz(L"BlackCellImage", xg_strBlackCellImage.c_str());

            app_key.SetSz(L"DoubleFrameLetters", xg_strDoubleFrameLetters.c_str());

            // 保存先のリストを設定する。
            nCount = static_cast<int>(xg_dirs_save_to.size());
            app_key.SetDword(L"SaveToCount", nCount);
            for (i = 0; i < nCount; i++)
            {
                StringCchPrintf(szFormat, _countof(szFormat), L"SaveTo %d", i + 1);
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
            app_key.SetDword(L"MainWndMaximized", !!xg_bMainWndMaximized);

            app_key.SetDword(L"TateInput", xg_bTateInput);

            app_key.SetDword(L"FileType", static_cast<DWORD>(xg_nFileType));

            // 最近使ったファイルのリストを設定する。
            nCount = static_cast<int>(xg_recently_used_files.size());
            app_key.SetDword(L"Recents", nCount);
            for (i = 0; i < nCount; i++) {
                StringCchPrintf(szFormat, _countof(szFormat), L"Recent %d", i);
                app_key.SetSz(szFormat, xg_recently_used_files[i].c_str());
            }
        }
    }

    return true;
}

// 設定を消去する。
bool __fastcall XgEraseSettings(void) noexcept
{
    // レジストリのアプリキーを削除する。
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
    const XG_EpvCode code = xg_xword.EveryPatternValid1(vNotFoundWords, pos, xg_bNoAddBlack);
    if (code == xg_epv_PATNOTMATCH) {
        if (check_words) {
            // パターンにマッチしないマスがあった。
            WCHAR sz[128];
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_NOCANDIDATE), pos.m_j + 1, pos.m_i + 1);
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
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_TOOLONGSPACE), pos.m_j + 1, pos.m_i + 1);
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
void XgDestroyCandsWnd(void) noexcept
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

    // ルールをチェックする。
    if (!XgRuleCheck(hwnd, FALSE))
        return false;

    // 候補ウィンドウを開く。
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
    StringCchCat(szPath, _countof(szPath), XgLoadStringDx1(IDS_HINTSTXT));
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
        StringCchPrintf(szCmdLine, _countof(szCmdLine), XgLoadStringDx1(IDS_NOTEPAD), szPath);
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
void XgNewCells(HWND hwnd, WCHAR ch, int nRows, int nCols)
{
    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);
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
    for (int iRow = 0; iRow < xg_nRows; ++iRow) {
        for (int jCol = 0; jCol < xg_nCols; ++jCol) {
            xg_xword.SetAt(iRow, jCol, ch);
        }
    }
    XgGetFileManager() = std::make_shared<XG_FileManager>();
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
    for (int iRow = 0; iRow < xg_nRows; ++iRow) {
        for (int jCol = 0; jCol < xg_nCols; ++jCol) {
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
    const auto cb = (str.size() + 1) * sizeof(WCHAR);
    if (HGLOBAL hGlobal = ::GlobalAlloc(GHND | GMEM_SHARE, cb)) {
        if (auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal))) {
            StringCchCopyW(psz, cb / sizeof(WCHAR), str.c_str());
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
void XgResizeCells(HWND hwnd, int nNewRows, int nNewCols)
{
    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);
    // マークの更新を通知する。
    XgMarkUpdate();
    // サイズを変更する。
    const int nOldRows = xg_nRows, nOldCols = xg_nCols;
    const int iMin = std::min((int)xg_nRows, (int)nNewRows);
    const int jMin = std::min((int)xg_nCols, (int)nNewCols);
    XG_Board copy;
    copy.ResetAndSetSize(nNewRows, nNewCols);
    for (int i = 0; i < iMin; ++i) {
        for (int j = 0; j < jMin; ++j) {
            xg_nRows = nOldRows;
            xg_nCols = nOldCols;
            const WCHAR ch = xg_xword.GetAt(i, j);
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
int CALLBACK XgBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM /*lpData*/) noexcept
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
        const int cxPaper = ::GetDeviceCaps(hdc, HORZRES);
        const int cyPaper = ::GetDeviceCaps(hdc, VERTRES);

        // ページ開始。
        if (::StartPage(hdc) > 0) {
            // 二重マス単語を描画する。
            if (bPrintAnswer && XgGetMarkWord(&xg_solution, strMarkWord)) {
                // 既定のフォント情報を取得する。
                hFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
                ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);

                // フォント名を取得する。
                StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
                if (xg_szCellFont[0])
                    StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), xg_szCellFont);

                // フォント情報を設定する。
                lf.lfHeight = cyPaper / 2 / 15;
                lf.lfWidth = 0;
                lf.lfWeight = FW_NORMAL;
                lf.lfQuality = ANTIALIASED_QUALITY;

                // 二重マス単語を描画する。
                hFont = ::CreateFontIndirectW(&lf);
                hFontOld = ::SelectObject(hdc, hFont);
                rc = { cxPaper / 8, cyPaper / 16, cxPaper * 7 / 8, cyPaper / 8 };
                str = XgLoadStringDx1(IDS_ANSWER);
                str += strMarkWord;
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);
                ::SelectObject(hdc, hFontOld);
                ::DeleteFont(hFont);
            }

            // サイズを取得する。
            SIZE siz;
            XgGetXWordExtent(&siz);

            // EMFオブジェクトを作成する。
            HDC hdcEMF = ::CreateEnhMetaFileW(hdc, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
            if (hdcEMF != nullptr) {
                // EMFオブジェクトにクロスワードを描画する。
                XgSetSizeOfEMF(hdcEMF, &siz);
                if (bPrintAnswer)
                    XgDrawXWord(xg_solution, hdcEMF, &siz, DRAW_MODE_EMF);
                else
                    XgDrawXWord(xg_xword, hdcEMF, &siz, DRAW_MODE_EMF);

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
                    rc = { x, y, x + cx, y + cy };

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
            hFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
            ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);

            // フォント名を取得する。
            StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
            if (xg_szCellFont[0])
                StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), xg_szCellFont);

            // フォント情報を設定する。
            lf.lfHeight = cyPaper / 2 / 45;
            lf.lfWidth = 0;
            lf.lfWeight = FW_NORMAL;
            lf.lfQuality = ANTIALIASED_QUALITY;
            hFont = ::CreateFontIndirectW(&lf);
            hFontOld = ::SelectObject(hdc, hFont);

            // ヒントを一行ずつ描画する。
            rc = { cxPaper / 8, cyPaper / 2, cxPaper * 7 / 8, cyPaper * 8 / 9 };
            for (ichOld = ich = 0; rc.bottom <= cyPaper * 8 / 9; ichOld = ich + 1) {
                ich = strHints.find(L"\n", ichOld);
                if (ich == std::wstring::npos)
                    break;
                str = strHints.substr(0, ich);  // 一行取り出す。
                rc = { cxPaper / 8, yLast, cxPaper * 7 / 8, cyPaper * 8 / 9 };
                ::DrawTextW(hdc, str.data(), static_cast<int>(str.size()), &rc,
                    DT_LEFT | DT_TOP | DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK);
            }
            // 最後の行を描画する。
            rc = { cxPaper / 8, yLast, cxPaper * 7 / 8, cyPaper * 8 / 9 };
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
                hFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
                ::GetObjectW(hFont, sizeof(LOGFONTW), &lf);
                StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
                lf.lfHeight = cyPaper / 2 / 45;
                lf.lfWidth = 0;
                lf.lfWeight = FW_NORMAL;
                lf.lfQuality = ANTIALIASED_QUALITY;

                // ヒントの残りを描画する。
                hFont = ::CreateFontIndirectW(&lf);
                hFontOld = ::SelectObject(hdc, hFont);
                str = strHints;
                rc = { cxPaper / 8, cyPaper / 9, cxPaper * 7 / 8, cyPaper * 8 / 9 };
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

// LOOKSファイルのインポート。
BOOL XgImportLooks(HWND hwnd, LPCWSTR pszFileName)
{
    if (!PathFileExistsW(pszFileName))
        return FALSE;

    XgGetFileManager()->set_looks(pszFileName);

    WCHAR szText[1024], szText2[MAX_PATH];

    // 色。
    GetPrivateProfileStringW(L"Looks", L"WhiteCellColor", XG_WHITE_COLOR_DEFAULT, szText, _countof(szText), pszFileName);
    xg_rgbWhiteCellColor = _wtoi(szText);

    GetPrivateProfileStringW(L"Looks", L"BlackCellColor", XG_BLACK_COLOR_DEFAULT, szText, _countof(szText), pszFileName);
    xg_rgbBlackCellColor = _wtoi(szText);

    GetPrivateProfileStringW(L"Looks", L"MarkedCellColor", XG_MARKED_COLOR_DEFAULT, szText, _countof(szText), pszFileName);
    xg_rgbMarkedCellColor = _wtoi(szText);

    // 線の幅（pt）。
    GetPrivateProfileStringW(L"Looks", L"LineWidthInPt", XG_LINE_WIDTH_DEFAULT2, szText, _countof(szText), pszFileName);
    xg_nLineWidthInPt = wcstof(szText, nullptr);
    if (xg_nLineWidthInPt < XG_MIN_LINEWIDTH)
        xg_nLineWidthInPt = XG_MIN_LINEWIDTH;
    if (xg_nLineWidthInPt > XG_MAX_LINEWIDTH)
        xg_nLineWidthInPt = XG_MAX_LINEWIDTH;

    // フォント。
    GetPrivateProfileStringW(L"Looks", L"CellFont", L"", szText, _countof(szText), pszFileName);
    StringCchCopyW(xg_szCellFont, _countof(xg_szCellFont), szText);

    GetPrivateProfileStringW(L"Looks", L"SmallFont", L"", szText, _countof(szText), pszFileName);
    StringCchCopyW(xg_szSmallFont, _countof(xg_szSmallFont), szText);

    GetPrivateProfileStringW(L"Looks", L"UIFont", L"", szText, _countof(szText), pszFileName);
    StringCchCopyW(xg_szUIFont, _countof(xg_szUIFont), szText);

    // スケルトンビューか？
    if (XgIsUserJapanese())
        GetPrivateProfileStringW(L"Looks", L"SkeletonView", L"0", szText, _countof(szText), pszFileName);
    else
        GetPrivateProfileStringW(L"Looks", L"SkeletonView", L"1", szText, _countof(szText), pszFileName);
    const BOOL bSkeltonView = _wtoi(szText);
    xg_nViewMode = bSkeltonView ? XG_VIEW_SKELETON : XG_VIEW_NORMAL;

    // 太枠をつけるか？
    GetPrivateProfileStringW(L"Looks", L"AddThickFrame", L"1", szText, _countof(szText), pszFileName);
    xg_bAddThickFrame = !!_wtoi(szText);

    // 二重マスに枠をつけるか？
    GetPrivateProfileStringW(L"Looks", L"DrawFrameForMarkedCell", L"1", szText, _countof(szText), pszFileName);
    xg_bDrawFrameForMarkedCell = !!_wtoi(szText);

    // 英小文字か？
    GetPrivateProfileStringW(L"Looks", L"Lowercase", L"0", szText, _countof(szText), pszFileName);
    xg_bLowercase = !!_wtoi(szText);

    // ひらがなか？
    GetPrivateProfileStringW(L"Looks", L"Hiragana", L"0", szText, _countof(szText), pszFileName);
    xg_bHiragana = !!_wtoi(szText);

    // 文字の大きさ。
    StringCchPrintfW(szText2, _countof(szText2), L"%d", XG_DEF_CELL_CHAR_SIZE);
    GetPrivateProfileStringW(L"Looks", L"CellCharPercents", szText2, szText, _countof(szText), pszFileName);
    xg_nCellCharPercents = _wtoi(szText);

    StringCchPrintfW(szText2, _countof(szText2), L"%d", XG_DEF_SMALL_CHAR_SIZE);
    GetPrivateProfileStringW(L"Looks", L"SmallCharPercents", szText2, szText, _countof(szText), pszFileName);
    xg_nSmallCharPercents = _wtoi(szText);

    // 黒マス画像。
    GetPrivateProfileStringW(L"Looks", L"BlackCellImage", L"", szText, _countof(szText), pszFileName);
    xg_strBlackCellImage = szText;

    ::DeleteObject(xg_hbmBlackCell);
    xg_hbmBlackCell = nullptr;
    ::DeleteEnhMetaFile(xg_hBlackCellEMF);
    xg_hBlackCellEMF = nullptr;
    XgGetFileManager()->load_block_image(xg_strBlackCellImage, xg_hbmBlackCell, xg_hBlackCellEMF);

    // 二重マス文字。
    GetPrivateProfileStringW(L"Looks", L"DoubleFrameLetters", XgLoadStringDx1(IDS_DBLFRAME_LETTERS_1), szText, _countof(szText), pszFileName);
    {
        std::vector<BYTE> data;
        XgHexToBin(data, szText);
        std::wstring str;
        str.resize(data.size() / sizeof(WCHAR));
        memcpy(&str[0], data.data(), data.size());
        xg_strDoubleFrameLetters = str;
    }

    return TRUE;
}

// LOOKSファイルのエクスポート。
BOOL XgExportLooks(HWND hwnd, LPCWSTR pszFileName)
{
    // 書く前にファイルを消す。
    DeleteFileW(pszFileName);
    // LOOKSファイル名をセットする。
    XgGetFileManager()->set_looks(pszFileName);

    // 文字の大きさの設定。
    WritePrivateProfileStringW(L"Looks", L"CellCharPercents", XgIntToStr(xg_nCellCharPercents), pszFileName);
    WritePrivateProfileStringW(L"Looks", L"SmallCharPercents", XgIntToStr(xg_nSmallCharPercents), pszFileName);

    // セルフォント。
    WritePrivateProfileStringW(L"Looks", L"CellFont", xg_szCellFont, pszFileName);

    // 小さい文字のフォント。
    WritePrivateProfileStringW(L"Looks", L"SmallFont", xg_szSmallFont, pszFileName);

    // UIフォント。
    WritePrivateProfileStringW(L"Looks", L"UIFont", xg_szUIFont, pszFileName);

    // もし黒マス画像が指定されていれば
    if (xg_strBlackCellImage.size() && xg_strBlackCellImage != XgLoadStringDx1(IDS_NONE))
    {
        std::wstring converted = xg_strBlackCellImage;
        XgGetFileManager()->convert(converted);
        WritePrivateProfileStringW(L"Looks", L"BlackCellImage", converted.c_str(), pszFileName);

        // さらに画像ファイルをエクスポートする。
        XgGetFileManager()->save_image(converted);
    }
    else
    {
        WritePrivateProfileStringW(L"Looks", L"BlackCellImage", L"", pszFileName);
    }

    // スケルトンビューか？
    const BOOL bSkeltonView = (xg_nViewMode == XG_VIEW_SKELETON);
    WritePrivateProfileStringW(L"Looks", L"SkeletonView", XgIntToStr(bSkeltonView), pszFileName);

    // 太枠をつけるか？
    WritePrivateProfileStringW(L"Looks", L"AddThickFrame", XgIntToStr(xg_bAddThickFrame), pszFileName);

    // 二重マスに枠をつけるか？
    WritePrivateProfileStringW(L"Looks", L"DrawFrameForMarkedCell", XgIntToStr(xg_bDrawFrameForMarkedCell), pszFileName);

    // 英小文字か？
    WritePrivateProfileStringW(L"Looks", L"Lowercase", XgIntToStr(xg_bLowercase), pszFileName);

    // ひらがなか？
    WritePrivateProfileStringW(L"Looks", L"Hiragana", XgIntToStr(xg_bHiragana), pszFileName);

    // 色を設定する。
    WritePrivateProfileStringW(L"Looks", L"WhiteCellColor", XgIntToStr(xg_rgbWhiteCellColor), pszFileName);
    WritePrivateProfileStringW(L"Looks", L"BlackCellColor", XgIntToStr(xg_rgbBlackCellColor), pszFileName);
    WritePrivateProfileStringW(L"Looks", L"MarkedCellColor", XgIntToStr(xg_rgbMarkedCellColor), pszFileName);

    // 線の幅（pt）を設定する。
    {
        WCHAR sz[64];
        StringCchPrintfW(sz, _countof(sz), L"%.1f", xg_nLineWidthInPt);
        WritePrivateProfileStringW(L"Looks", L"LineWidthInPt", sz, pszFileName);
    }

    // 二重マス文字。
    {
        std::wstring str = xg_strDoubleFrameLetters;
        str = XgBinToHex(str.c_str(), str.size() * sizeof(WCHAR));
        WritePrivateProfileStringW(L"Looks", L"DoubleFrameLetters", str.c_str(), pszFileName);
    }

    // フラッシュ！ すべてを完了させるための儀式。
    return WritePrivateProfileStringW(nullptr, nullptr, nullptr, pszFileName);
}

// クリップボードにテキストをコピーする。
BOOL XgSetClipboardUnicodeText(HWND hwnd, const std::wstring& str) noexcept
{
    // ヒープからメモリを確保する。
    const auto cb = (str.size() + 1) * sizeof(WCHAR);
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (hGlobal) {
        // メモリをロックする。
        auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal));
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

//////////////////////////////////////////////////////////////////////////////

static inline BOOL About_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    XgCenterDialog(hwnd);
    SetDlgItemTextW(hwnd, stc1, XgLoadStringDx1(IDS_VERSION));
    SetDlgItemTextW(hwnd, edt1, XgLoadStringDx1(IDS_COPYRIGHTINFO));

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    WCHAR szText[128];
    StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_HOWMANYCORES), sys_info.dwNumberOfProcessors);
    SetDlgItemTextW(hwnd, stc2, szText);

#ifdef _WIN64
    SetDlgItemTextW(hwnd, stc3, L"Win64");
#else
    SetDlgItemTextW(hwnd, stc3, L"Win32");
#endif

    return TRUE;
}

static inline void About_OnChangeLog(HWND hwnd) noexcept
{
    // Show HISTORY.txt
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(nullptr, szPath, _countof(szPath));
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"HISTORY.txt");
    ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
}

static inline void About_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept
{
    switch (id) {
    case IDOK:
    case IDCANCEL:
        EndDialog(hwnd, id);
        break;
    case IDYES:
        About_OnChangeLog(hwnd);
        break;
    default:
        break;
    }
}

static inline LRESULT About_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) noexcept
{
    switch (pnmhdr->code) {
    case NM_CLICK:
    case NM_RETURN:
        if (idFrom == ctl1) {
            // The user clicked URL
            ShellExecuteW(hwnd, nullptr, XgLoadStringDx1(IDS_HOMEPAGE), nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case NM_RCLICK:
        break;
    default:
        break;
    }
    return 0;
}

static inline void About_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    HWND hwndCtl1 = GetDlgItem(hwnd, ctl1);
    if (hwndContext != hwndCtl1)
        return;

    if (xPos == MAXWORD && yPos == MAXWORD) {
        RECT rc;
        GetWindowRect(hwndCtl1, &rc);
        xPos = (rc.left + rc.right) / 2;
        yPos = (rc.top + rc.bottom) / 2;
    }

    // The user right-clicked URL
    HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(3));
    HMENU hSubMenu = GetSubMenu(hMenu, 2);
    SetForegroundWindow(hwnd);
    const auto id = ::TrackPopupMenu(hSubMenu,
                                     TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                     xPos, yPos, 0, hwnd, nullptr);
    DestroyMenu(hMenu);
    PostMessage(hwnd, WM_NULL, 0, 0);
    switch (id)
    {
    case ID_COPYURL:
        XgSetClipboardUnicodeText(hwnd, XgLoadStringDx1(IDS_HOMEPAGE));
        break;
    default:
        break;
    }
}

static INT_PTR CALLBACK
AboutDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, About_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, About_OnCommand);
        HANDLE_MSG(hwnd, WM_NOTIFY, About_OnNotify);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, About_OnContextMenu);
    default:
        break;
    }
    return 0;
}

// バージョン情報を表示する。
void __fastcall XgOnAbout(HWND hwnd) noexcept
{
    DialogBoxW(xg_hInstance, MAKEINTRESOURCEW(IDD_ABOUT), hwnd, AboutDialogProc);
}

//////////////////////////////////////////////////////////////////////////////

// 保存する。
bool __fastcall XgDoSaveFiles(HWND hwnd, LPCWSTR pszFile)
{
    auto old_mgr = std::make_shared<XG_FileManager>(*XgGetFileManager());

    // パス情報を格納・変換する。
    XgGetFileManager()->set_file(pszFile);
    XgGetFileManager()->set_looks(L"");

    // 関連画像ファイルをパス名を変換して保存する。
    std::wstring files_dir;
    XgGetFileManager()->get_files_dir(files_dir);
    CreateDirectoryW(files_dir.c_str(), nullptr);
    for (auto& box : xg_boxes) {
        if (box->m_type == L"pic") {
            auto pic = dynamic_cast<XG_PictureBoxWindow*>(&*box);
            XgGetFileManager()->save_image2(pic->m_strText);
        }
    }
    XgGetFileManager()->save_image2(xg_strBlackCellImage);

    // 保存する。
    if (!XgDoSaveFile(hwnd, pszFile)) {
        // 保存に失敗。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
        XgGetFileManager() = old_mgr;
        return false;
    }

    // 関連ファイルをエクスポートする。
    auto looks_file = XgGetFileManager()->get_looks_file();
    XgExportLooks(hwnd, looks_file.c_str());

    // ファイルの種類を保存する。
    LPCWSTR pchDotExt = PathFindExtensionW(pszFile);
    if (lstrcmpiW(pchDotExt, L".xwj") == 0 ||
        lstrcmpiW(pchDotExt, L".json") == 0 ||
        lstrcmpiW(pchDotExt, L".jso") == 0)
    {
        xg_nFileType = XG_FILETYPE_XWJ;
    }
    else if (lstrcmpiW(pchDotExt, L".crp") == 0)
    {
        xg_nFileType = XG_FILETYPE_CRP;
    }
    else if (lstrcmpiW(pchDotExt, L".xd") == 0)
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

    // ファイル変更フラグ。
    XG_FILE_MODIFIED(FALSE);

    // 最近使ったファイルを更新する。
    XgUpdateRecentlyUsed(pszFile);

    // 元に戻す情報をクリアする。
    xg_ubUndoBuffer.clear();
    return true;
}

// 保存ダイアログ。
BOOL __fastcall XgOnSaveAs(HWND hwnd)
{
    if (xg_dicts.empty()) {
        // 辞書ファイルの名前を読み込む。
        XgLoadDictsAll();
    }

    // ユーザーにファイルの場所を問い合わせる準備。
    OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
    WCHAR sz[MAX_PATH] = L"";
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_SAVEFILTER));
    StringCchCopy(sz, _countof(sz), xg_strFileName.data());
    ofn.lpstrFile = sz;
    ofn.nMaxFile = static_cast<DWORD>(_countof(sz));
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
        XgDoSaveFiles(hwnd, sz);

        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        return TRUE;
    }

    return FALSE;
}

// 保存。
BOOL __fastcall XgOnSave(HWND hwnd)
{
    // ファイル名がセットされてなければ保存場所を聞く。
    if (xg_strFileName.empty())
        return XgOnSaveAs(hwnd);

    // 上書き保存。
    if (XgDoSaveFiles(hwnd, xg_strFileName.c_str()))
    {
        xg_ubUndoBuffer.clear();
        return TRUE;
    }
    return FALSE;
}

// 保存を確認し、必要なら保存する。
BOOL XgDoConfirmSave(HWND hwnd)
{
    if (!xg_bFileModified)
        return TRUE;

    // アプリ名。
    std::wstring strAppName = XgLoadStringDx2(IDS_APPNAME);
    // ファイルタイトル。
    std::wstring strFileName;
    if (xg_strFileName.empty())
        strFileName = XgLoadStringDx1(IDS_UNTITLED);
    else
        strFileName = PathFindFileNameW(xg_strFileName.c_str());

    // 保存するか確認。
    WCHAR szText[MAX_PATH];
    StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_QUERYSAVE),
                     strFileName.c_str());
    const auto id = XgCenterMessageBoxW(hwnd, szText, strAppName.c_str(),
                                        MB_ICONINFORMATION | MB_YESNOCANCEL);
    switch (id) {
    case IDYES:
        return XgOnSave(hwnd);
    case IDNO:
        XG_FILE_MODIFIED(FALSE);
        return TRUE;
    case IDCANCEL:
        return FALSE;
    default:
        return TRUE;
    }
}

// 新規作成ダイアログ。
BOOL __fastcall XgOnNew(HWND hwnd)
{
    if (!XgDoConfirmSave(hwnd))
        return FALSE;

    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

    // 新規作成ダイアログ。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    XG_NewDialog dialog;
    const auto nID = dialog.DoModal(hwnd);
    ::EnableWindow(xg_hwndInputPalette, TRUE);
    if (nID != IDOK)
        return FALSE;

    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    // 元に戻す情報を確定する。
    sa2->Get();
    xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);

    // ボックスをすべて削除する。
    XgDeleteBoxes();
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
    // 更新フラグ。
    XG_FILE_MODIFIED(FALSE);
    return TRUE;
}

// ファイルを読み込む。
bool __fastcall XgDoLoadFiles(HWND hwnd, LPCWSTR pszFile)
{
    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);
    // ボックスをすべて削除する。
    XgDeleteBoxes();

    // マネージャー入れ替え。
    auto old_mgr = XgGetFileManager();
    XgGetFileManager()->set_file(pszFile);
    XgGetFileManager()->set_looks(L"");

    // 開く。
    if (!XgDoLoadMainFile(hwnd, pszFile)) {
        // 失敗。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
        XgGetFileManager() = old_mgr;
        return false;
    }

    // 画像を破棄する。
    if (old_mgr)
        old_mgr->delete_handles();

    // 画像群を読み込む。
    XgLoadImageFiles();

    // LOOKSファイルも自動でインポートする。
    auto looks_file = XgGetFileManager()->get_looks_file();
    XgImportLooks(hwnd, looks_file.c_str());

    // キャレット位置を更新。
    XgSetCaretPos();
    // テーマを更新する。
    XgSetThemeString(xg_strTheme);
    XgUpdateTheme(hwnd);
    // イメージを更新する。
    XgMarkUpdate();
    XgUpdateImage(hwnd, 0, 0);
    // ヒントを表示する。
    XgShowHints(hwnd);
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
    // ルールを更新する。
    XgUpdateRules(hwnd);
    // 「元に戻す」情報をクリアする。
    xg_ubUndoBuffer.Empty();
    // ズームを実際のウィンドウに合わせる。
    XgFitZoom(hwnd);
    // イメージを更新する。
    XgUpdateImage(hwnd, 0, 0);

    // フォーカスを移動。
    SetFocus(hwnd);

    // ECWの場合は、保存できないのでファイルを変更したものと見なす。
    if (lstrcmpiW(PathFindExtensionW(pszFile), L".ecw") == 0) {
        XG_FILE_MODIFIED(TRUE);
    } else {
        XG_FILE_MODIFIED(FALSE);
    }

    // 最近使ったファイルを更新する。
    XgUpdateRecentlyUsed(pszFile);

    return true;
}

// ファイルを読み込む。
BOOL XgOnLoad(HWND hwnd, LPCWSTR pszFile, LPPOINT ppt)
{
    // 最初のファイルのパス名を取得する。
    WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
    StringCchCopyW(szFile, _countof(szFile), pszFile);

    // ショートカットだった場合は、ターゲットのパスを取得する。
    if (XgGetPathOfShortcutW(szFile, szTarget))
        StringCchCopy(szFile, _countof(szFile), szTarget);

    // LOOKSファイルだった場合は自動で適用する。
    LPWSTR pchDotExt = PathFindExtensionW(szFile);
    if (lstrcmpiW(pchDotExt, L".looks") == 0)
    {
        XgImportLooks(hwnd, szFile);
        XG_FILE_MODIFIED(TRUE);
        return TRUE;
    }

    // 画像ファイルだったら画像ボックスを作成する。
    const BOOL bImage = XgIsImageFile(szFile);
    // テキストファイルだったらテキストボックスを作成する。
    const BOOL bText = XgIsTextFile(szFile);
    if (bImage || bText)
    {
        if (ppt)
            ScreenToClient(xg_canvasWnd, ppt);

        int i1, j1;
        if (ppt) {
            XgSetCellPosition(ppt->x, ppt->y, i1, j1, FALSE);
        } else {
            POINT pt = { 0, 0 };
            XgSetCellPosition(pt.x, pt.y, i1, j1, FALSE);
        }
        if (i1 < 0)
            i1 = 0;
        if (j1 < 0)
            j1 = 0;
        int i2 = i1 + 2, j2 = j1 + 2;

        if (i2 >= xg_nRows) {
            i2 = xg_nRows;
            i1 = i2 - 2;
        }
        if (j2 >= xg_nCols) {
            j2 = xg_nCols;
            j1 = j2 - 2;
        }

        if (bText) {
            // テキストボックス。
            std::wstring strText;
            if (XgReadTextFileAll(szFile, strText)) {
                xg_str_trim_right(strText);
                auto ptr = std::make_shared<XG_TextBoxWindow>(i1, j1, i2, j2);
                ptr->SetText(strText);
                if (ptr->CreateDx(xg_canvasWnd)) {
                    auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                    sa1->Get();
                    {
                        ptr->Bound();
                        xg_boxes.emplace_back(ptr);
                    }
                    auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                    sa2->Get();
                    xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定。

                    // ファイルが変更された。
                    XG_FILE_MODIFIED(TRUE);
                    return TRUE;
                }
            }
        } else if (bImage) {
            // 画像ボックス。
            auto ptr = std::make_shared<XG_PictureBoxWindow>(i1, j1, i2, j2);
            ptr->SetText(szFile);
            if (ptr->CreateDx(xg_canvasWnd)) {
                auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                sa1->Get();
                {
                    ptr->Bound();
                    xg_boxes.emplace_back(ptr);
                }
                auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定。

                // ファイルが変更された。
                XG_FILE_MODIFIED(TRUE);
                return TRUE;
            }
        }

        return FALSE;
    }

    // 保存を確認し、必要なら保存する。
    if (!XgDoConfirmSave(xg_hMainWnd))
        return FALSE;

    if (XgDoLoadFiles(hwnd, pszFile)) {
        // 元に戻す情報をクリアする。
        xg_ubUndoBuffer.clear();
    }
    return FALSE;
}

// ファイルを開く。
BOOL __fastcall XgOnOpen(HWND hwnd)
{
    if (!XgDoConfirmSave(hwnd))
        return FALSE;

    // ユーザーにファイルの場所を問い合わせる。
    WCHAR sz[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_CROSSFILTER));
    ofn.lpstrFile = sz;
    ofn.nMaxFile = static_cast<DWORD>(_countof(sz));
    ofn.lpstrTitle = XgLoadStringDx1(IDS_OPENCROSSDATA);
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"xwd";
    if (::GetOpenFileNameW(&ofn)) {
        return XgOnLoad(hwnd, sz, nullptr);
    }
    return FALSE;
}

// 特定の場所にファイルを保存する。
bool __fastcall XgDoSaveToLocation(HWND hwnd)
{
    WCHAR szPath[MAX_PATH], szDir[MAX_PATH];
    WCHAR szName[32];

    // パスを生成する。
    StringCchCopy(szDir, _countof(szDir), xg_dirs_save_to[0].data());
    PathAddBackslashW(szDir);

    // ファイル名を生成する。
    UINT u;
    for (u = 1; u <= 0xFFFF; u++) {
        StringCchPrintf(szName, _countof(szName), L"%dx%d-%04u.xwj", xg_nRows, xg_nCols, u);
        StringCchCopy(szPath, _countof(szPath), szDir);
        StringCchCat(szPath, _countof(szPath), szName);
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
            bOK = XgDoSaveFiles(hwnd, szPath);
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
    int nZoomRate;
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
    INT_PTR nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    if (multiple) {
        // [問題の連続作成]ダイアログ。
        XG_SeqGenDialog dialog;
        nID = static_cast<int>(dialog.DoModal(hwnd));
    } else {
        // [問題の作成]ダイアログ。
        XG_GenDialog dialog;
        dialog.m_bShowAnswer = show_answer;
        nID = static_cast<int>(dialog.DoModal(hwnd));
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
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    // ズームを実際のウィンドウに合わせる。
    XgFitZoom(hwnd);
    // ステータスバーを更新する。
    XgUpdateStatusBar(hwnd);

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
    {
        XG_SeqPatGenDialog dialog;
        const auto nID = static_cast<int>(dialog.DoModal(hwnd));
        ::EnableWindow(xg_hwndInputPalette, TRUE);
        if (nID != IDOK) {
            return false;
        }
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
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

    // キャンセルダイアログを表示し、生成を開始する。
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    INT_PTR nID;
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANCELLED),
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else {
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_BLOCKSGENERATED),
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
    {
        XG_PatGenDialog dialog;
        const INT_PTR nID = dialog.DoModal(hwnd);
        ::EnableWindow(xg_hwndInputPalette, TRUE);
        if (nID != IDOK) {
            return false;
        }
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
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANCELLED),
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else {
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_BLOCKSGENERATED), 1,
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
    // 生成した数と生成する数。
    xg_nNumberGenerated = 0;
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

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

        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANCELLED),
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_SOLVED),
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANTSOLVE),
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
    // 生成した数と生成する数。
    xg_nNumberGenerated = 0;
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    // 候補ウィンドウを破棄する。
    XgDestroyCandsWnd();
    // ヒントウィンドウを破棄する。
    XgDestroyHintsWnd();
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANCELLED),
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_SOLVED),
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANTSOLVE),
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
    INT_PTR nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    {
        XG_SeqSolveDialog dialog;
        nID = static_cast<int>(dialog.DoModal(hwnd));
        ::EnableWindow(xg_hwndInputPalette, TRUE);
        if (nID != IDOK) {
            // イメージを更新する。
            XgSetCaretPos();
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
            return false;
        }
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
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), xg_nNumberGenerated,
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    } else {
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), xg_nNumberGenerated,
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
    INT_PTR nID;
    ::EnableWindow(xg_hwndInputPalette, FALSE);
    {
        XG_SeqSolveDialog dialog;
        nID = static_cast<int>(dialog.DoModal(hwnd));
        ::EnableWindow(xg_hwndInputPalette, TRUE);
        if (nID != IDOK) {
            // イメージを更新する。
            XgSetCaretPos();
            XgMarkUpdate();
            XgUpdateImage(hwnd, 0, 0);
            return false;
        }
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
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), xg_nNumberGenerated,
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    } else {
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), xg_nNumberGenerated,
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
static void XgOnLineSymmetryCheck(HWND hwnd) noexcept
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
static void XgOnPointSymmetryCheck(HWND hwnd)
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
    const DWORD cb = static_cast<DWORD>((str.size() + 1) * sizeof(WCHAR));
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (hGlobal) {
        // メモリをロックする。
        auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal));
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
                        XgSetSizeOfEMF(hdc, &siz);
                        XgDrawXWord(*pxw, hdc, &siz, DRAW_MODE_PRINT);

                        // EMFを設定。
                        HENHMETAFILE hEMF = ::CloseEnhMetaFile(hdc);
                        ::SetClipboardData(CF_ENHMETAFILE, hEMF);

                        // DIBを設定。
                        if (HDC hDC = CreateCompatibleDC(nullptr))
                        {
                            HBITMAP hbm = XgCreate24BppBitmap(hDC, siz.cx, siz.cy);
                            HGDIOBJ hbmOld = SelectObject(hDC, hbm);
                            RECT rc = { 0, 0, siz.cx, siz.cy };
                            FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));
                            XgDrawXWord(*pxw, hDC, &siz, DRAW_MODE_PRINT);
                            SelectObject(hDC, hbmOld);
                            ::DeleteDC(hDC);

                            std::vector<BYTE> data;
                            if (PackedDIB_CreateFromHandle(data, hbm))
                            {
                                HGLOBAL hGlobal2 = GlobalAlloc(GHND | GMEM_SHARE, data.size());
                                LPVOID pv = GlobalLock(hGlobal2);
                                if (pv && data.size())
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
    HENHMETAFILE hEMF = nullptr;
    HDC hdcRef = ::GetDC(hwnd);
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    if (hdc) {
        // EMFに描画する。
        XgSetSizeOfEMF(hdc, &siz);
        XgDrawXWord(*pxw, hdc, &siz, DRAW_MODE_PRINT);
        hEMF = ::CloseEnhMetaFile(hdc);
    }

    // BMPを作成する。
    HBITMAP hbm = nullptr;
    if (HDC hDC = CreateCompatibleDC(nullptr))
    {
        BITMAPINFO bi;
        ZeroMemory(&bi, sizeof(bi));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = siz.cx;
        bi.bmiHeader.biHeight = siz.cy;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        LPVOID pvBits;
        hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
        if (HGDIOBJ hbmOld = SelectObject(hDC, hbm))
        {
            RECT rc = { 0, 0, siz.cx, siz.cy };
            FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));
            PlayEnhMetaFile(hDC, hEMF, &rc);
            SelectObject(hDC, hbmOld);
        }
        DeleteDC(hDC);
    }
    HGLOBAL hGlobal = nullptr;
    if (hbm)
    {
        std::vector<BYTE> data;
        PackedDIB_CreateFromHandle(data, hbm);

        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, static_cast<DWORD>(data.size()));
        if (hGlobal)
        {
            if (LPVOID pv = GlobalLock(hGlobal))
            {
                memcpy(pv, &data[0], data.size());
                GlobalUnlock(hGlobal);
            }
        }
        DeleteObject(hbm);
        hbm = nullptr;
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
    const auto nCount = static_cast<int>(xg_vMarks.size());
    XgGetMarkWordExtent(nCount, &siz);

    // EMFを作成する。
    HENHMETAFILE hEMF = nullptr;
    HDC hdcRef = ::GetDC(hwnd);
    HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadStringDx1(IDS_APPNAME));
    if (hdc) {
        // EMFに描画する。
        XgSetSizeOfEMF(hdc, &siz);
        XgDrawMarkWord(hdc, &siz);
        hEMF = ::CloseEnhMetaFile(hdc);
    }
    ::ReleaseDC(hwnd, hdcRef);

    // すでに解があるかどうかによって切り替え。
    const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

    // 二重マス単語のテキストを取得。
    HGLOBAL hGlobal = nullptr;
    std::wstring strMarkWord;
    if (XgGetMarkWord(xw, strMarkWord)) {
        for (auto& wch : strMarkWord) {
            if (ZEN_LARGE_A <= wch && wch <= ZEN_LARGE_Z)
                wch = L'A' + (wch - ZEN_LARGE_A);
            else if (ZEN_SMALL_A <= wch && wch <= ZEN_SMALL_Z)
                wch = L'A' + (wch - ZEN_SMALL_A);
        }

        // CF_UNICODETEXTのデータを用意。
        const auto cbGlobal = (strMarkWord.size() + 1) * sizeof(WCHAR);
        hGlobal = GlobalAlloc(GHND | GMEM_SHARE, cbGlobal);
        if (auto psz = static_cast<LPWSTR>(GlobalLock(hGlobal))) {
            StringCchCopyW(psz, cbGlobal / sizeof(WCHAR), strMarkWord.c_str());
            GlobalUnlock(hGlobal);
        } else {
            GlobalFree(hGlobal);
            hGlobal = nullptr;
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
        auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal));
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
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

    xg_bSolved = false;
    xg_bShowAnswer = false;
    XgSetCaretPos();
    xg_vMarks.clear();
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();
    XgMarkUpdate();
    // ナンクロモードを解除。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();
    // 再描画。
    XgUpdateImage(hwnd, 0, 0);
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
    const auto CF_HTML = ::RegisterClipboardFormatW(L"HTML Format");

    // ヒープからメモリを確保する。
    DWORD cb = static_cast<DWORD>((str.size() + 1) * sizeof(WCHAR));
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
    if (hGlobal) {
        // メモリをロックする。
        auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal));
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
                    auto pb2 = static_cast<LPBYTE>(p2);
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
void __fastcall MainWnd_OnDestroy(HWND /*hwnd*/) noexcept
{
    // イメージリストを破棄する。
    ::ImageList_Destroy(xg_hImageList);
    xg_hImageList = nullptr;
    ::ImageList_Destroy(xg_hGrayedImageList);
    xg_hGrayedImageList = nullptr;

    // ウィンドウを破棄する。
    ::DestroyWindow(xg_hToolBar);
    xg_hToolBar = nullptr;
    ::DestroyWindow(xg_hSizeGrip);
    xg_hSizeGrip = nullptr;
    ::DestroyWindow(xg_cands_wnd);
    ::DestroyWindow(xg_hHintsWnd);
    xg_hHintsWnd = nullptr;
    ::DestroyWindow(xg_hwndInputPalette);
    xg_hwndInputPalette = nullptr;
    ::DestroyWindow(xg_hMarkingDlg);

    // アプリを終了する。
    ::PostQuitMessage(0);

    xg_hMainWnd = nullptr;
}

// 「辞書」メニューを取得する。
HMENU XgDoFindDictMenu(HMENU hMenu)
{
    WCHAR szText[128];
    LPCWSTR pszDict = XgLoadStringDx1(IDS_DICTIONARY);
    for (int i = 0; i < 16; ++i)
    {
        if (GetMenuStringW(hMenu, i, szText, _countof(szText), MF_BYPOSITION))
        {
            if (wcsstr(szText, pszDict) != nullptr)
            {
                return GetSubMenu(hMenu, i);
            }
        }
    }
    return nullptr;
}

// 「辞書」メニューを更新する。
void XgDoUpdateDictMenu(HMENU hDictMenu)
{
    // TODO: 「辞書」メニュー項目を更新したら、次の I_NONE_ITEM を修正すること。
#define I_NONE_ITEM 6 // メニュー項目「(なし)」の位置。
    int index = I_NONE_ITEM;

    // 辞書項目をすべて削除する。
    while (RemoveMenu(hDictMenu, index, MF_BYPOSITION))
    {
        ;
    }

    if (xg_dicts.empty()) // 辞書リストが空？
    {
        AppendMenuW(hDictMenu, MF_STRING | MF_GRAYED, -1, XgLoadStringDx1(IDS_NONE));
        return;
    }

    // 辞書項目を追加する。
    int count = 0, id = ID_DICTIONARY00;
    WCHAR szText[MAX_PATH];
    for (const auto& entry : xg_dicts)
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
        StringCchPrintfW(szText, _countof(szText), L"&%c ", L"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"[count]);
        StringCchCatW(szText, _countof(szText), text.c_str());
        AppendMenuW(hDictMenu, MF_STRING | MF_ENABLED, id, szText);
        ++index;
        ++count;
        ++id;
        if (count >= XG_MAX_DICTS)
            break;
    }

    // ラジオボタンを付ける。
    index = I_NONE_ITEM;
    for (const auto& entry : xg_dicts)
    {
        auto& file = entry.m_filename;
        if (lstrcmpiW(file.c_str(), xg_dict_name.c_str()) == 0)
        {
            const int nCount = GetMenuItemCount(hDictMenu);
            CheckMenuRadioItem(hDictMenu, I_NONE_ITEM, nCount - 1, index, MF_BYPOSITION);
            break;
        }
        ++index;
    }
}

// メニューを初期化する。
void __fastcall MainWnd_OnInitMenu(HWND /*hwnd*/, HMENU hMenu)
{
    if (HMENU hDictMenu = XgDoFindDictMenu(hMenu))
    {
        // 辞書メニューを更新。
        XgDoUpdateDictMenu(hDictMenu);
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

    // 黒マスパターンを反射する。
    if (xg_bSolved) {
        ::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, MF_BYCOMMAND | MF_GRAYED);
    } else {
        if (xg_nRules & (RULE_LINESYMMETRYH | RULE_LINESYMMETRYV | RULE_POINTSYMMETRY)) {
            ::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, MF_BYCOMMAND | MF_ENABLED);
        } else {
            ::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, MF_BYCOMMAND | MF_GRAYED);
        }
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
        ::CheckMenuItem(hMenu, ID_HIRAGANA, MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_HIRAGANA, MF_UNCHECKED);
    }

    // Lowercaseウィンドウのメニュー更新。
    if (xg_bLowercase) {
        ::CheckMenuItem(hMenu, ID_LOWERCASE, MF_CHECKED);
    } else {
        ::CheckMenuItem(hMenu, ID_LOWERCASE, MF_UNCHECKED);
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
        ::CheckMenuItem(hMenu, ID_VIEW_SKELETON_VIEW, MF_UNCHECKED);
        break;
    case XG_VIEW_SKELETON:
        // スケルトンビュー。
        ::CheckMenuItem(hMenu, ID_VIEW_SKELETON_VIEW, MF_CHECKED);
        break;
    }

    // ナンクロモード。
    if (!xg_bSolved) {
        ::EnableMenuItem(hMenu, ID_NUMCROMODE, MF_BYCOMMAND | MF_GRAYED);
    } else {
        ::EnableMenuItem(hMenu, ID_NUMCROMODE, MF_BYCOMMAND | MF_ENABLED);
        if (xg_bNumCroMode) {
            ::CheckMenuItem(hMenu, ID_NUMCROMODE, MF_CHECKED);
        } else {
            ::CheckMenuItem(hMenu, ID_NUMCROMODE, MF_UNCHECKED);
        }
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
        const int cItems = ::GetMenuItemCount(hMenu);
        ::DeleteMenu(hMenu, cItems - 1, MF_BYPOSITION);
    }

    // 最近使ったファイルを取得。
    HMENU hFileMenu = ::GetSubMenu(hMenu, 0);
    const int cFileItems = ::GetMenuItemCount(hFileMenu);
    HMENU hRecentMenu = ::GetSubMenu(hFileMenu, cFileItems - 7); // TODO: ファイルメニュー項目を追加したらここも変更。
    // 最近使ったファイルのメニュー項目をすべて削除。
    while (::DeleteMenu(hRecentMenu, 0, MF_BYPOSITION))
        ;
    // 最近使ったファイルのメニュー項目を新しく追加。
    int id = ID_RECENT_00, iItem = 0;
    for (auto& item : xg_recently_used_files) {
        std::wstring str;
        str += L'&';
        str += static_cast<WCHAR>(L'0' + iItem);
        str += L'\t';
        str += item;
        ::AppendMenuW(hRecentMenu, MF_STRING, id++, str.c_str());
        ++iItem;
    }
    // 最近使ったファイルが空の場合。
    if (xg_recently_used_files.empty()) {
        ::AppendMenuW(hRecentMenu, MF_STRING | MF_GRAYED, 0, XgLoadStringDx1(IDS_NONE));
    }
}

// ステータスバーを更新する。
void __fastcall XgUpdateStatusBar(HWND hwnd)
{
    // クライアント領域を取得する。
    RECT rc;
    GetClientRect(hwnd, &rc);

    // パーツのサイズを決定する。
    auto anWidth = make_array<int>(rc.right - 200, rc.right - 100, rc.right);

    // ステータスバーをパーツに分ける。
    SendMessageW(xg_hStatusBar, SB_SETPARTS, 3, reinterpret_cast<LPARAM>(anWidth.data()));

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
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 0, reinterpret_cast<LPARAM>(str.c_str()));

    // キャレット位置。
    WCHAR szText[64];
    StringCchPrintf(szText, _countof(szText), L"(%d, %d)", xg_caret_pos.m_j + 1, xg_caret_pos.m_i + 1);
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 1, reinterpret_cast<LPARAM>(szText));

    // 盤のサイズ。
    StringCchPrintf(szText, _countof(szText), L"%d x %d", xg_nCols, xg_nRows);
    SendMessageW(xg_hStatusBar, SB_SETTEXT, 2, reinterpret_cast<LPARAM>(szText));
}

// サイズが変更された。
void __fastcall MainWnd_OnSize(HWND hwnd, UINT state, int /*cx*/, int /*cy*/)
{
    int x, y;

    // ツールバーが作成されていなければ、初期化前なので、無視。
    if (xg_hToolBar == nullptr)
        return;

    // ステータスバーの高さを取得。
    int cyStatus = 0;
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
    const auto cx = rcClient.Width();
    auto cy = rcClient.Height();

    // ツールバーの高さを取得。
    int cyToolBar = 0;
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
    const int cyHScrollBar = ::GetSystemMetrics(SM_CYHSCROLL);
    const int cxVScrollBar = ::GetSystemMetrics(SM_CXVSCROLL);

    // 複数のウィンドウの位置とサイズをいっぺんに変更する。
    HDWP hDwp = ::BeginDeferWindowPos(2);
    if (hDwp) {
        if (::IsWindowVisible(xg_hSizeGrip)) {
            hDwp = ::DeferWindowPos(hDwp, xg_hSizeGrip, nullptr,
                x + cx - cxVScrollBar, y + cy - cyHScrollBar,
                cxVScrollBar, cyHScrollBar,
                SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        }
        hDwp = ::DeferWindowPos(hDwp, xg_canvasWnd, nullptr,
            x, y, cx, cy, SWP_SHOWWINDOW | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        ::EndDeferWindowPos(hDwp);
    }

    if (state == SIZE_MAXIMIZED) // 最大化されたか？
    {
        // 表示領域が小さいとき、ズームを全体に合わせる。
        SIZE siz;
        {
            ForDisplay for_display;
            XgGetXWordExtent(&siz);
        }
        MRect rcReal;
        XgGetRealClientRect(hwnd, &rcReal);
        if (siz.cx < rcReal.Width() && siz.cy < rcReal.Height())
            XgFitZoom(hwnd);
    }

    // 再描画する。
    ::InvalidateRect(xg_hToolBar, nullptr, TRUE);
    ::InvalidateRect(xg_hStatusBar, nullptr, TRUE);
    ::InvalidateRect(xg_hSizeGrip, nullptr, TRUE);

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
void __fastcall MainWnd_OnMove(HWND hwnd, int /*x*/, int /*y*/) noexcept
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
        StringCchCopy(szData, _countof(szData), xg_szUIFont);
        LPWSTR pch = wcsrchr(szData, L',');
        if (pch) {
            *pch++ = 0;
            std::wstring name(szData);
            std::wstring size(pch);
            xg_str_trim(name);
            xg_str_trim(size);

            StringCchCopy(s_lf.lfFaceName, _countof(s_lf.lfFaceName), name.data());

            HDC hdc = ::CreateCompatibleDC(nullptr);
            const int point_size = _wtoi(size.data());
            s_lf.lfHeight = -MulDiv(point_size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ::DeleteDC(hdc);
        } else {
            std::wstring name(szData);
            xg_str_trim(name);

            StringCchCopy(s_lf.lfFaceName, _countof(s_lf.lfFaceName), name.data());
        }
    }
    return &s_lf;
}

// 見た目の設定。
void MainWnd_OnSettings(HWND hwnd)
{
    XgDestroyCandsWnd();
    XG_SettingsDialog dialog;
    if (dialog.DoModal(hwnd) == IDOK) {
        XG_FILE_MODIFIED(TRUE);
    }
}

// テーマが変更された。
void XgUpdateTheme(HWND hwnd)
{
    std::unordered_set<std::wstring> priority, forbidden;
    XgParseTheme(priority, forbidden, xg_strDefaultTheme);
    xg_bThemeModified = (priority != xg_priority_tags || forbidden != xg_forbidden_tags);

    // メニュー項目の個数を取得。
    HMENU hMenu = ::GetMenu(hwnd);
    const int nCount = ::GetMenuItemCount(hMenu);
    assert(nCount > 0);
    // 辞書の文字列から「辞書」メニューのインデックスiMenuを取得。
    int iMenu;
    WCHAR szText[32];
    MENUITEMINFOW info = { sizeof(info) };
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    for (iMenu = 0; iMenu < nCount; ++iMenu) {
        szText[0] = 0;
        ::GetMenuStringW(hMenu, iMenu, szText, _countof(szText), MF_BYPOSITION);
        if (wcsstr(szText, XgLoadStringDx1(IDS_DICT)) != nullptr) {
            break;
        }
    }
    assert(iMenu != nCount);
    // 辞書の状態に対して文字列を指定。
    if (xg_bThemeModified || xg_dict_name.empty() ||
        xg_dict_name.find(L"SubDict") != xg_dict_name.npos)
    {
        StringCchCopyW(szText, _countof(szText), XgLoadStringDx1(IDS_MODIFIEDDICT));
    } else {
        StringCchCopyW(szText, _countof(szText), XgLoadStringDx1(IDS_DEFAULTDICT));
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
    const int nCount = ::GetMenuItemCount(hMenu);
    WCHAR szText[32];
    MENUITEMINFOW info = { sizeof(info) };
    info.fMask = MIIM_TYPE;
    info.fType = MFT_STRING;
    for (int i = nCount - 1; i >= 0; --i) {
        szText[0] = 0;
        ::GetMenuStringW(hMenu, i, szText, _countof(szText), MF_BYPOSITION);
        if (wcsstr(szText, XgLoadStringDx1(IDS_RULES)) != nullptr) {
            if (xg_nRules == XG_DEFAULT_RULES) {
                StringCchCopyW(szText, _countof(szText), XgLoadStringDx1(IDS_STANDARDRULES));
            } else {
                StringCchCopyW(szText, _countof(szText), XgLoadStringDx1(IDS_MODIFIEDRULES));
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
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

    // 消去するのか確認。
    if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_QUERYERASESETTINGS), XgLoadStringDx2(IDS_APPNAME),
                            MB_ICONWARNING | MB_YESNO) != IDYES)
    {
        return;
    }

    // 設定を消去する。
    const bool bSuccess = XgEraseSettings();

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
        WideCharToMultiByte(CP_UTF8, 0, url.c_str(), -1, &str[0], static_cast<int>(len), nullptr, nullptr);

    len = strlen(str.c_str());
    str.resize(len);

    std::wstring ret;
    WCHAR buf[4];
    static const WCHAR s_hex[] = L"0123456789ABCDEF";
    for (auto ch : str)
    {
        using namespace std;
        if (ch == ' ')
        {
            ret += L'+';
        }
        else if (('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || 
                 ('0' <= ch && ch <= '9'))
        {
            ret += static_cast<char>(ch);
        }
        else
        {
            switch (ch)
            {
            case L'.': case L'-': case L'_': case L'*':
                ret += static_cast<char>(ch);
                break;
            default:
                buf[0] = L'%';
                buf[1] = s_hex[(ch >> 4) & 0xF];
                buf[2] = s_hex[ch & 0xF];
                buf[3] = 0;
                ret += buf;
                break;
            }
        }
    }

    return ret;
}

// ウェブ検索。
void XgDoWebSearch(HWND hwnd, LPCWSTR str)
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

    ::ShellExecuteW(hwnd, nullptr, query.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
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
    for (int i = 0; i < xg_nRows; ++i) {
        for (int j = 0; j < xg_nCols; ++j) {
            const WCHAR ch = pxword->GetAt(i, j);
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
    if (pattern.find(ZEN_SPACE) != pattern.npos) {
        return;
    }

    // ウェブ検索。
    XgDoWebSearch(hwnd, pattern.c_str());
}

// 「黒マスパターン」を開く。
void __fastcall XgOpenPatterns(HWND hwnd)
{
    XG_PatternDialog dialog;
    dialog.DoModal(hwnd);
}

// 「黒マスルールの説明.txt」を開く。
void __fastcall XgOnOpenRulesTxt(HWND hwnd)
{
    WCHAR szPath[MAX_PATH], szDir[MAX_PATH];
    GetModuleFileNameW(nullptr, szPath, MAX_PATH);
    PathRemoveFileSpecW(szPath);
    StringCchCopyW(szDir, _countof(szDir), szPath);
    StringCchCopyW(szPath, _countof(szPath), szDir);
    PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
    if (!PathFileExistsW(szPath)) {
        StringCchCopyW(szPath, _countof(szPath), szDir);
        PathAppendW(szPath, L"..");
        PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
        if (!PathFileExistsW(szPath)) {
            StringCchCopyW(szPath, _countof(szPath), szDir);
            PathAppendW(szPath, L"..\\..");
            PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
            if (!PathFileExistsW(szPath)) {
                StringCchCopyW(szPath, _countof(szPath), szDir);
                PathAppendW(szPath, L"..\\..\\..");
                PathAppendW(szPath, XgLoadStringDx1(IDS_RULESTXT));
            }
        }
    }
    ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
}

// 黒マスルールをチェックする。
BOOL __fastcall XgRuleCheck(HWND hwnd, BOOL bMessageOnSuccess)
{
    const XG_Board& board = (xg_bShowAnswer ? xg_solution : xg_xword);
    // 連黒禁。
    if (xg_nRules & RULE_DONTDOUBLEBLACK) {
        if (board.DoubleBlack()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADJACENTBLOCK), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }
    // 四隅黒禁。
    if (xg_nRules & RULE_DONTCORNERBLACK) {
        if (board.CornerBlack()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CORNERBLOCK), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }
    // 三方黒禁。
    if (xg_nRules & RULE_DONTTRIDIRECTIONS) {
        if (board.TriBlackAround()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_TRIBLOCK), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }
    // 分断禁。
    if (xg_nRules & RULE_DONTDIVIDE) {
        if (board.DividedByBlack()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_DIVIDED), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }
    // 黒斜三連禁。
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (board.ThreeDiagonals()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_THREEDIAGONALS), nullptr, MB_ICONERROR);
            return FALSE;
        }
    } else {
        // 黒斜四連禁。
        if (xg_nRules & RULE_DONTFOURDIAGONALS) {
            if (board.FourDiagonals()) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_FOURDIAGONALS), nullptr, MB_ICONERROR);
                return FALSE;
            }
        }
    }
    // 黒マス点対称。
    if (xg_nRules & RULE_POINTSYMMETRY) {
        if (!board.IsPointSymmetry()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTPOINTSYMMETRY), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }
    // 黒マス線対称。
    if (xg_nRules & RULE_LINESYMMETRYV) {
        if (!board.IsLineSymmetryV()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRYV), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }
    if (xg_nRules & RULE_LINESYMMETRYH) {
        if (!board.IsLineSymmetryH()) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTLINESYMMETRYH), nullptr, MB_ICONERROR);
            return FALSE;
        }
    }

    // 合格。
    if (bMessageOnSuccess) {
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_RULESPASSED),
                            XgLoadStringDx2(IDS_PASSED), MB_ICONINFORMATION);
    }
    return TRUE;
}

// 「テーマ」ダイアログを表示する。
void __fastcall XgTheme(HWND hwnd)
{
    if (xg_tag_histgram.empty()) {
        // タグがありません。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_NOTAGS), nullptr, MB_ICONERROR);
        return;
    }

    XG_ThemeDialog dialog;
    const auto id = dialog.DoModal(hwnd);
    if (id == IDOK) {
        XgUpdateTheme(hwnd);
    }
}

// テーマをリセットする。
void __fastcall XgResetTheme(HWND hwnd, BOOL bQuery)
{
    if (bQuery) {
        const auto id = XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_RESETTHEME),
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANCELLED),
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
    } else if (xg_bSolved) {
        // 成功メッセージを表示する。
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_MADEPROBLEM),
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);

        // ヒントを更新して開く。
        XgUpdateHints();
        XgShowHints(hwnd);
    } else {
        // 失敗メッセージを表示する。
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANTMAKEPROBLEM),
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
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_OUTOFSTORAGE), xg_nNumberGenerated,
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
    } else {
        // あった。
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_PROBLEMSMADE), xg_nNumberGenerated,
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
static void XgSetZoomRate(HWND hwnd, int nZoomRate)
{
    if (nZoomRate < 9)
        nZoomRate = 9;
    xg_nZoomRate = nZoomRate;
    const int x = XgGetHScrollPos(), y = XgGetVScrollPos();
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

// UI言語の設定。
void XgSelectUILanguage(HWND hwnd)
{
    XG_UILanguageDialog dialog;
    dialog.m_ids.push_back(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT));
    dialog.m_ids.push_back(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    dialog.m_LangID = xg_UILangID;
    if (dialog.DoModal(hwnd) == IDOK)
    {
        xg_UILangID = dialog.m_LangID;
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
    // 二重マス単語の候補と配置を破棄する。
    ::DestroyWindow(xg_hMarkingDlg);

    // ダイアログを表示。
    int nID;
    {
        XG_WordListDialog dialog;
        nID = static_cast<int>(dialog.DoModal(hwnd));
        if (nID != IDOK)
            return;
    }

    // 計算時間を求めるために、開始時間を取得する。
    xg_dwlTick0 = xg_dwlTick1 = ::GetTickCount64();
    // 再計算までの時間を概算する。
    const auto size = XG_WordListDialog::s_wordset.size();
    xg_dwlWait = size * size / 3 + 100; // ミリ秒。

    // 単語リストから生成する。
    {
        XG_CancelFromWordsDialog dialog;
        nID = static_cast<int>(dialog.DoModal(hwnd));
    }
    if (nID == IDCANCEL) {
        // キャンセルされた。
        WCHAR sz[256];
        StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CANCELLED),
                       (xg_dwlTick2 - xg_dwlTick0) / 1000,
                       (xg_dwlTick2 - xg_dwlTick0) / 100 % 10);
        XgCenterMessageBoxW(hwnd, sz, XgLoadStringDx2(IDS_RESULTS), MB_ICONINFORMATION);
        return;
    }

    if (!s_generated) {
        // 生成できなかった。
        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTGENERATE), nullptr, MB_ICONERROR);
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
    // ナンクロモードをリセットする。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();
    // 単語リストを保存して後で使う。
    for (auto& word : XG_WordListDialog::s_words) {
        for (auto& wch : word) {
            if (ZEN_LARGE_A <= wch && wch <= ZEN_LARGE_Z)
                wch = L'a' + (wch - ZEN_LARGE_A);
            else if (ZEN_SMALL_A <= wch && wch <= ZEN_SMALL_Z)
                wch = L'a' + (wch - ZEN_SMALL_A);
        }
        CharLowerBuffW(&word[0], (DWORD)word.size());
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
    int i1 = xg_caret_pos.m_i, j1 = xg_caret_pos.m_j;
    int i2 = i1 + 2, j2 = j1 + 2;
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
        dialog1.SetTextColor(xg_rgbBlackCellColor, FALSE);
        dialog1.SetBgColor(xg_rgbWhiteCellColor, FALSE);
        if (dialog1.DoModal(hwnd) == IDOK) {
            auto ptr = std::make_shared<XG_TextBoxWindow>(i1, j1, i2, j2);
            ptr->SetText(dialog1.m_strText);
            ptr->SetTextColor(dialog1.GetTextColor());
            ptr->SetBgColor(dialog1.GetBgColor());
            ptr->m_strFontName = dialog1.m_strFontName;
            ptr->m_nFontSizeInPoints = dialog1.m_nFontSizeInPoints;
            if (ptr->CreateDx(xg_canvasWnd)) {
                auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                sa1->Get();
                {
                    xg_boxes.emplace_back(ptr);
                }
                auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定する。
                // ファイルが変更された。
                xg_bFileModified = TRUE;
                return TRUE;
            }
        }
        break;
    case ID_ADDPICTUREBOX:
        if (dialog2.DoModal(hwnd) == IDOK) {
            auto ptr = std::make_shared<XG_PictureBoxWindow>(i1, j1, i2, j2);
            ptr->SetText(dialog2.m_strFile);
            if (ptr->CreateDx(xg_canvasWnd)) {
                auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                sa1->Get();
                {
                    xg_boxes.emplace_back(ptr);
                }
                auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定する。
                // ファイルが変更された。
                xg_bFileModified = TRUE;
                return TRUE;
            }
        }
        break;
    default:
        break;
    }

    return FALSE;
}

// ナンクロの写像が正当か確認する。
BOOL __fastcall XgValidateNumCro(HWND hwnd)
{
    if (!xg_bSolved)
        return FALSE;

    for (int i = 0; i < xg_nRows; ++i) {
        for (int j = 0; j < xg_nCols; ++j) {
            const WCHAR ch = xg_solution.GetAt(i, j);
            if (ch == ZEN_SPACE || ch == ZEN_BLACK)
                continue;

            auto it = xg_mapNumCro1.find(ch);
            if (it == xg_mapNumCro1.end()) {
                return FALSE;
            }
        }
    }

    for (auto& pair : xg_mapNumCro1) {
        auto it = xg_mapNumCro2.find(pair.second);
        if (it == xg_mapNumCro2.end()) {
            return FALSE;
        }
    }

    return TRUE;
}

// ナンクロの写像を生成する。
void __fastcall XgMakeItNumCro(HWND hwnd)
{
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

    if (!xg_bSolved)
        return;

    int next_number = 1;
    for (int i = 0; i < xg_nRows; ++i) {
        for (int j = 0; j < xg_nCols; ++j) {
            WCHAR ch = xg_solution.GetAt(i, j);
            if (ch == ZEN_SPACE || ch == ZEN_BLACK)
                continue;

            auto it = xg_mapNumCro1.find(ch);
            if (it == xg_mapNumCro1.end()) {
                xg_mapNumCro2[next_number] = ch;
                xg_mapNumCro1[ch] = next_number;
                ++next_number;
            } else {
                int number = it->second;
                xg_mapNumCro2[number] = ch;
                xg_mapNumCro1[ch] = number;
            }
        }
    }

    if (!XgValidateNumCro(hwnd)) {
        xg_mapNumCro1.clear();
        xg_mapNumCro2.clear();
        xg_bNumCroMode = false;
    }

    XgUpdateImage(hwnd);
}

// ローカルファイルを見つける。
BOOL __fastcall XgFindLocalFile(LPWSTR pszPath, UINT cchPath, LPCWSTR pszFileName) noexcept
{
    GetModuleFileNameW(nullptr, pszPath, MAX_PATH);
    PathRemoveFileSpecW(pszPath);
    PathAppendW(pszPath, pszFileName);
    if (!PathFileExistsW(pszPath))
    {
        PathRemoveFileSpecW(pszPath);
        PathRemoveFileSpecW(pszPath);
        PathAppendW(pszPath, pszFileName);
        if (!PathFileExistsW(pszPath))
        {
            PathRemoveFileSpecW(pszPath);
            PathRemoveFileSpecW(pszPath);
            PathAppendW(pszPath, pszFileName);
        }
    }

    return PathFileExistsW(pszPath);
}

// マス位置からキャンバス座標を取得する。
void __fastcall XgCellToCanvas(RECT& rc, XG_Pos cell)
{
    XgCellToImage(rc, cell);

    INT x = XgGetHScrollPos();
    INT y = XgGetVScrollPos();
    ::OffsetRect(&rc, -x, -y);
}

// キャレット位置のマスを無効化する。
void __fastcall XgInvalidateCell(XG_Pos cell)
{
    RECT rcCell;
    XgCellToCanvas(rcCell, cell);

    ::InvalidateRect(xg_hCanvasWnd, &rcCell, TRUE);
}

// キャレットを描画する。
void __fastcall XgDrawCaret(HDC hdc)
{
    if (!xg_bShowCaret)
        return;

    RECT rc;
    XgCellToCanvas(rc, xg_caret_pos);

    // 赤いキャレットペンを作成する。
    LOGBRUSH lbRed;
    lbRed.lbStyle = BS_SOLID;
    lbRed.lbColor = RGB(255, 0, 0);
    HPEN hCaretPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
        1, &lbRed, 0, nullptr);
    HGDIOBJ hPenOld = ::SelectObject(hdc, hCaretPen);

    // セルの大きさ。
    const int nCellSize = xg_nCellSize * xg_nZoomRate / 100;

    // カギカッコみたいなもの、コーナーに四つ。
    const auto cxyMargin = nCellSize / 10; // 余白。
    const auto cxyLine = nCellSize / 3; // 線の位置。
    ::MoveToEx(hdc, rc.left + cxyMargin, rc.top + cxyMargin, nullptr);
    ::LineTo(hdc, rc.left + cxyMargin, rc.top + cxyLine);
    ::MoveToEx(hdc, rc.left + cxyMargin, rc.top + cxyMargin, nullptr);
    ::LineTo(hdc, rc.left + cxyLine, rc.top + cxyMargin);
    ::MoveToEx(hdc, rc.right - cxyMargin, rc.top + cxyMargin, nullptr);
    ::LineTo(hdc, rc.right - cxyMargin, rc.top + cxyLine);
    ::MoveToEx(hdc, rc.right - cxyMargin, rc.top + cxyMargin, nullptr);
    ::LineTo(hdc, rc.right - cxyLine, rc.top + cxyMargin);
    ::MoveToEx(hdc, rc.right - cxyMargin, rc.bottom - cxyMargin, nullptr);
    ::LineTo(hdc, rc.right - cxyMargin, rc.bottom - cxyLine);
    ::MoveToEx(hdc, rc.right - cxyMargin, rc.bottom - cxyMargin, nullptr);
    ::LineTo(hdc, rc.right - cxyLine, rc.bottom - cxyMargin);
    ::MoveToEx(hdc, rc.left + cxyMargin, rc.bottom - cxyMargin, nullptr);
    ::LineTo(hdc, rc.left + cxyMargin, rc.bottom - cxyLine);
    ::MoveToEx(hdc, rc.left + cxyMargin, rc.bottom - cxyMargin, nullptr);
    ::LineTo(hdc, rc.left + cxyLine, rc.bottom - cxyMargin);

    // 十字。
    const auto cxyCross = nCellSize / 10; // 十字の半径。
    ::MoveToEx(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cxyCross, nullptr);
    ::LineTo(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 + cxyCross);
    ::MoveToEx(hdc, (rc.left + rc.right) / 2 - cxyCross, (rc.top + rc.bottom) / 2, nullptr);
    ::LineTo(hdc, (rc.left + rc.right) / 2 + cxyCross, (rc.top + rc.bottom) / 2);

    ::SelectObject(hdc, hPenOld);
    ::DeleteObject(hCaretPen);
}

// コマンドを実行する。
void __fastcall MainWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT /*codeNotify*/)
{
    int x = -1, y = -1;
    BOOL bUpdateImage = FALSE;

#ifdef NO_RANDOM
    xg_random_seed = 0;
#endif

    switch (id) {
    case ID_LEFT:
        // キャレットを左へ移動。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_j > 0) {
            xg_caret_pos.m_j--;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番左のキャレットなら、左端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_RIGHT:
        // キャレットを右へ移動。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_j + 1 < xg_nCols) {
            xg_caret_pos.m_j++;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番右のキャレットなら、右端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_RIGHT, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_UP:
        // キャレットを上へ移動。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_i > 0) {
            xg_caret_pos.m_i--;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番上のキャレットなら、上端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_DOWN:
        // キャレットを下へ移動。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_i + 1 < xg_nRows) {
            xg_caret_pos.m_i++;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番下のキャレットなら、下端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_MOSTLEFT:
        // Ctrl+←。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_j > 0) {
            xg_caret_pos.m_j = 0;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番左のキャレットなら、左端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_MOSTRIGHT:
        // Ctrl+→。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_j + 1 < xg_nCols) {
            xg_caret_pos.m_j = xg_nCols - 1;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番右のキャレットなら、右端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_RIGHT, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_MOSTUPPER:
        // Ctrl+↑。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_i > 0) {
            xg_caret_pos.m_i = 0;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番上のキャレットなら、上端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_MOSTLOWER:
        // Ctrl+↓。
        XgInvalidateCell(xg_caret_pos);
        if (xg_caret_pos.m_i + 1 < xg_nRows) {
            xg_caret_pos.m_i = xg_nRows - 1;
            XgEnsureCaretVisible(hwnd);
        } else {
            // 一番下のキャレットなら、下端へ移動。
            SendMessageW(xg_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
        }
        XgInvalidateCell(xg_caret_pos);
        xg_prev_vk = 0;
        break;
    case ID_OPENCANDSWNDHORZ:
        XgOpenCandsWnd(hwnd, false);
        xg_prev_vk = 0;
        bUpdateImage = TRUE;
        break;
    case ID_OPENCANDSWNDVERT:
        XgOpenCandsWnd(hwnd, true);
        xg_prev_vk = 0;
        bUpdateImage = TRUE;
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
        bUpdateImage = TRUE;
        break;
    case ID_ERASESETTINGS:  // 設定の削除。
        MainWnd_OnEraseSettings(hwnd);
        bUpdateImage = TRUE;
        break;
    case ID_FLIPVH: // 縦と横を入れ替える。
        {
            const bool flag = !!::IsWindow(xg_hHintsWnd);
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 二重マス単語の候補と配置を破棄する。
            ::DestroyWindow(xg_hMarkingDlg);
            // 縦と横を入れ替える。
            MainWnd_OnFlipVH(hwnd);
            if (flag) {
                XgShowHints(hwnd);
            }
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
        }
        bUpdateImage = TRUE;
        break;

    case ID_NEW:    // 新規作成。
        XgOnNew(hwnd);
        bUpdateImage = TRUE;
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
            // 二重マス単語の候補と配置を破棄する。
            ::DestroyWindow(xg_hMarkingDlg);
            // 問題の作成。
            if (XgOnGenerate(hwnd, false, false)) {
                flag = true;
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
                // イメージを更新する。
                XgSetCaretPos();
                XgMarkUpdate();
                // メッセージボックスを表示する。
                XgShowResults(hwnd);
            }
            if (!flag) {
                sa1->Apply();
            }
            // イメージを更新する。
            XgSetCaretPos();
            XgMarkUpdate();
        }
        bUpdateImage = TRUE;
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
            // 二重マス単語の候補と配置を破棄する。
            ::DestroyWindow(xg_hMarkingDlg);
            // 問題の作成。
            if (XgOnGenerate(hwnd, true, false)) {
                flag = true;
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
                // イメージを更新する。
                XgSetCaretPos();
                XgMarkUpdate();
                // メッセージボックスを表示する。
                XgShowResults(hwnd);
            }
            if (!flag) {
                sa1->Apply();
            }
            // イメージを更新する。
            XgSetCaretPos();
            XgMarkUpdate();
        }
        bUpdateImage = TRUE;
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
            // 二重マス単語の候補と配置を破棄する。
            ::DestroyWindow(xg_hMarkingDlg);
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
                // メッセージボックスを表示する。
                XgShowResultsRepeatedly(hwnd);
            }
            // イメージを更新する。
            XgSetCaretPos();
            XgMarkUpdate();
        }
        bUpdateImage = TRUE;
        break;

    case ID_OPEN:   // ファイルを開く。
        XgOnOpen(hwnd);
        bUpdateImage = TRUE;
        break;
    case ID_SAVE: // ファイルを保存する。
        XgOnSave(hwnd);
        break;
    case ID_SAVEAS: // ファイルを保存する。
        XgOnSaveAs(hwnd);
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
        if (::GetFocus() != xg_hMainWnd) {
            ::SendMessageW(::GetFocus(), WM_UNDO, 0, 0);
        } else {
            if (xg_ubUndoBuffer.CanUndo()) {
                xg_ubUndoBuffer.Undo();
                if (::IsWindow(xg_hHintsWnd)) {
                    XG_HintsWnd::SetHintsData();
                }
            } else {
                ::MessageBeep(0xFFFFFFFF);
            }
        }
        if (xg_hMarkingDlg)
            xg_hMarkingDlg.RefreshCandidates(xg_hMarkingDlg);
        bUpdateImage = TRUE;
        break;
    case ID_REDO:   // やり直す。
        if (::GetFocus() != xg_hMainWnd) {
            ;
        } else {
            if (xg_ubUndoBuffer.CanRedo()) {
                xg_ubUndoBuffer.Redo();
                if (::IsWindow(xg_hHintsWnd)) {
                    XG_HintsWnd::SetHintsData();
                }
            } else {
                ::MessageBeep(0xFFFFFFFF);
            }
        }
        if (xg_hMarkingDlg)
            xg_hMarkingDlg.RefreshCandidates(xg_hMarkingDlg);
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVESYMMETRY), nullptr, MB_ICONERROR);
                break;
            }
            // ルール「黒マス線対称」では黒マス追加ありの解を求めることはできません。
            if (xg_nRules & RULE_LINESYMMETRYV) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), nullptr, MB_ICONERROR);
                break;
            }
            if (xg_nRules & RULE_LINESYMMETRYH) {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), nullptr, MB_ICONERROR);
                break;
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
        }
        bUpdateImage = TRUE;
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
                if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_SHALLIMIRROR), nullptr,
                                        MB_ICONINFORMATION | MB_YESNO) == IDYES)
                {
                    xg_xword.m_vCells = copy.m_vCells;
                }
            }

            // イメージを更新する。
            XgUpdateImage(hwnd, x, y);

            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            {
                // 解を求める（黒マス追加なし）。
                XgOnSolve_NoAddBlack(hwnd);
            }
            sa2->Get();

            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
        }
        bUpdateImage = TRUE;
        break;
    case ID_SOLVEREPEATEDLY:    // 連続で解を求める
        // ルール「黒マス点対称」では黒マス追加ありの解を求めることはできません。
        if (xg_nRules & RULE_POINTSYMMETRY) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVESYMMETRY), nullptr, MB_ICONERROR);
            break;
        }
        // ルール「黒マス線対称」では黒マス追加ありの解を求めることはできません。
        if (xg_nRules & RULE_LINESYMMETRYV) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), nullptr, MB_ICONERROR);
            break;
        }
        if (xg_nRules & RULE_LINESYMMETRYH) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSOLVELINESYMMETRY), nullptr, MB_ICONERROR);
            break;
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
                if (XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_SHALLIMIRROR), nullptr,
                                        MB_ICONINFORMATION | MB_YESNO) == IDYES)
                {
                    xg_xword.m_vCells = copy.m_vCells;
                }
            }

            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 二重マス単語の候補と配置を破棄する。
            ::DestroyWindow(xg_hMarkingDlg);
            // 連続で解を求める。
            if (XgOnSolveRepeatedly(hwnd)) {
                sa1->Apply();
            }
        }
        // イメージを更新する。
        XgSetCaretPos();
        XgMarkUpdate();
        bUpdateImage = TRUE;
        break;
    case ID_SOLVEREPEATEDLYNOADDBLACK:  // 連続で解を求める(黒マス追加なし)
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ヒントウィンドウを破棄する。
            XgDestroyHintsWnd();
            // 二重マス単語の候補と配置を破棄する。
            ::DestroyWindow(xg_hMarkingDlg);
            // 連続で解を求める。
            if (XgOnSolveRepeatedlyNoAddBlack(hwnd)) {
                sa1->Apply();
            }
        }
        // イメージを更新する。
        XgSetCaretPos();
        XgMarkUpdate();
        bUpdateImage = TRUE;
        break;
    case ID_SHOWSOLUTION:   // 解を表示する。
        xg_bShowAnswer = true;
        XgMarkUpdate();
        bUpdateImage = TRUE;
        break;
    case ID_NOSOLUTION: // 解を表示しない。
        xg_bShowAnswer = false;
        XgMarkUpdate();
        bUpdateImage = TRUE;
        break;
    case ID_SHOWHIDESOLUTION:   // 解の表示を切り替える。
        xg_bShowAnswer = !xg_bShowAnswer;
        XgMarkUpdate();
        bUpdateImage = TRUE;
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
            auto ahwnd = make_array<HWND>(
                xg_hMainWnd,
                xg_hHintsWnd,
                xg_cands_wnd,
                xg_hwndInputPalette,
                xg_hMarkingDlg
            );

            size_t i = 0, k, m;
            const auto count = ahwnd.size();
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
            auto ahwnd = make_array<HWND>(
                xg_hMarkingDlg,
                xg_hwndInputPalette,
                xg_cands_wnd,
                xg_hHintsWnd,
                xg_hMainWnd
            );

            size_t i = 0, k, m;
            const auto count = ahwnd.size();
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
            ::SendMessageW(::GetFocus(), WM_CUT, 0, 0);
        }
        bUpdateImage = TRUE;
        break;
    case ID_COPY:   // コピー。
        if (::GetForegroundWindow() == xg_hMainWnd) {
            XgCopyBoard(hwnd);
        } else {
            ::SendMessageW(::GetFocus(), WM_COPY, 0, 0);
        }
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
            ::SendMessageW(::GetFocus(), WM_PASTE, 0, 0);
        }
        bUpdateImage = TRUE;
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
            xg_hHintsWnd = nullptr;
            xg_bShowClues = FALSE;
        } else {
            XgShowHints(hwnd);
            xg_bShowClues = TRUE;
            ::SetForegroundWindow(xg_hHintsWnd);
        }
        break;
    case ID_MARKSNEXT:  // 次の二重マス単語
        if (xg_bSolved) {
            if (::IsWindowVisible(xg_hMarkingDlg)) {
                ::DestroyWindow(xg_hMarkingDlg);
            } else {
                xg_hMarkingDlg.CreateDialogDx(hwnd);
            }
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
                ::DestroyWindow(xg_hMarkingDlg);
            }
            mu2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
        }
        bUpdateImage = TRUE;
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
        ::ShellExecuteW(hwnd, nullptr, XgLoadStringDx1(IDS_HOMEPAGE), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case ID_OPENBBS:        // 掲示板を開く。
        {
            static LPCWSTR s_pszBBS = L"http://katahiromz.bbs.fc2.com/";
            ::ShellExecuteW(hwnd, nullptr, s_pszBBS, nullptr, nullptr, SW_SHOWNORMAL);
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
        XgUpdateScrollInfo(hwnd, XgGetHScrollPos(), XgGetVScrollPos());
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
        bUpdateImage = TRUE;
        break;
    case ID_BLOCK:
        SendMessageW(hwnd, WM_CHAR, L'#', 0);
        bUpdateImage = TRUE;
        break;
    case ID_SPACE:
        SendMessageW(hwnd, WM_CHAR, L'_', 0);
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
        bUpdateImage = TRUE;
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
        ShellExecuteW(hwnd, nullptr, XgLoadStringDx1(IDS_MATRIXSEARCH), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case ID_BLOCKNOFEED:
        {
            const bool bOldFeed = xg_bCharFeed;
            xg_bCharFeed = false;
            SendMessageW(hwnd, WM_CHAR, L'#', 0);
            xg_bCharFeed = bOldFeed;
        }
        bUpdateImage = TRUE;
        break;
    case ID_SPACENOFEED:
        {
            const bool bOldFeed = xg_bCharFeed;
            xg_bCharFeed = false;
            SendMessageW(hwnd, WM_CHAR, L'_', 0);
            xg_bCharFeed = bOldFeed;
        }
        bUpdateImage = TRUE;
        break;
    case ID_SHOWHIDENUMBERING:
        xg_bShowNumbering = !xg_bShowNumbering;
        bUpdateImage = TRUE;
        break;
    case ID_SHOWHIDECARET:
        xg_bShowCaret = !xg_bShowCaret;
        bUpdateImage = TRUE;
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
        bUpdateImage = TRUE;
        break;
    case ID_ZOOMOUT:
        if (xg_nZoomRate > 200) {
            xg_nZoomRate -= 20;
        } else if (xg_nZoomRate > 100) {
            xg_nZoomRate -= 10;
        } else if (xg_nZoomRate > 50) {
            xg_nZoomRate -= 5;
        } else if (xg_nZoomRate > 9) {
            xg_nZoomRate -= 1;
        }
        x = XgGetHScrollPos();
        y = XgGetVScrollPos();
        XgUpdateScrollInfo(hwnd, x, y);
        bUpdateImage = TRUE;
        break;
    case ID_ZOOM100:
        XgSetZoomRate(hwnd, 100);
        bUpdateImage = TRUE;
        break;
    case ID_ZOOM30:
        XgSetZoomRate(hwnd, 30);
        bUpdateImage = TRUE;
        break;
    case ID_ZOOM50:
        XgSetZoomRate(hwnd, 50);
        bUpdateImage = TRUE;
        break;
    case ID_ZOOM65:
        XgSetZoomRate(hwnd, 65);
        bUpdateImage = TRUE;
        break;
    case ID_ZOOM80:
        XgSetZoomRate(hwnd, 80);
        bUpdateImage = TRUE;
        break;
    case ID_ZOOM90:
        XgSetZoomRate(hwnd, 90);
        bUpdateImage = TRUE;
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
    case ID_LOWERCASE:
        xg_bLowercase = !xg_bLowercase;
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        bUpdateImage = TRUE;
        break;
    case ID_HIRAGANA:
        xg_bHiragana = !xg_bHiragana;
        if (xg_hwndInputPalette) {
            XgCreateInputPalette(hwnd);
        }
        bUpdateImage = TRUE;
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
        if (xg_hMarkingDlg)
            xg_hMarkingDlg.RefreshCandidates(xg_hMarkingDlg);
        break;
    case ID_RESETRULES:
        xg_nRules = XG_DEFAULT_RULES;
        XgUpdateRules(hwnd);
        break;
    case ID_OPENRULESTXT:
        XgOnOpenRulesTxt(hwnd);
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
            xg_nRules &= ~RULE_DONTFOURDIAGONALS;
        } else if ((xg_nRules & RULE_DONTTHREEDIAGONALS) && !(xg_nRules & RULE_DONTFOURDIAGONALS)) {
            xg_nRules &= ~RULE_DONTTHREEDIAGONALS;
            xg_nRules |= RULE_DONTFOURDIAGONALS;
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
        XgRuleCheck(hwnd, TRUE);
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
        }
        bUpdateImage = TRUE;
        break;
    case ID_FILLBYBLOCKS:
        XgNewCells(hwnd, ZEN_BLACK, xg_nRows, xg_nCols);
        bUpdateImage = TRUE;
        break;
    case ID_FILLBYWHITES:
        XgNewCells(hwnd, ZEN_SPACE, xg_nRows, xg_nCols);
        bUpdateImage = TRUE;
        break;
    case ID_ERASESOLUTIONANDUNLOCKEDIT:
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                std::wstring str;
                XG_Board *pxw = (xg_bSolved && xg_bShowAnswer) ? &xg_solution : &xg_xword;
                pxw->GetString(str);
                XgPasteBoard(hwnd, str);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
        }
        bUpdateImage = TRUE;
        break;
    case ID_RELOADDICTS:
        XgLoadDictsAll();
        break;
    case ID_VIEW_SKELETON_VIEW:
        {
            auto sa1 = std::make_shared<XG_UndoData_ViewMode>();
            auto sa2 = std::make_shared<XG_UndoData_ViewMode>();
            sa1->Get();

            if (xg_nViewMode == XG_VIEW_NORMAL) {
                // 黒マス画像をクリアする。
                xg_strBlackCellImage.clear();
                if (xg_hbmBlackCell) {
                    ::DeleteObject(xg_hbmBlackCell);
                    xg_hbmBlackCell = nullptr;
                }
                if (xg_hBlackCellEMF) {
                    DeleteEnhMetaFile(xg_hBlackCellEMF);
                    xg_hBlackCellEMF = nullptr;
                }
                // スケルトンビューを設定。
                xg_nViewMode = XG_VIEW_SKELETON;
            } else {
                // 通常ビューを設定。
                xg_nViewMode = XG_VIEW_NORMAL;
            }
            sa2->Get();

            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_VIEWMODE, sa1, sa2);
        }
        bUpdateImage = TRUE;
        break;
    case ID_COPY_WORD_LIST:
        // 単語リストのコピー。
        XgCopyWordList(hwnd);
        break;
    case ID_VIEW_DOUBLEFRAME_LETTERS:
        // 二重マス文字を表示するか？
        xg_bShowDoubleFrameLetters = !xg_bShowDoubleFrameLetters;
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
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
        }
        bUpdateImage = TRUE;
        break;
    case ID_GENERATEFROMWORDLIST:
        XgGenerateFromWordList(hwnd);
        bUpdateImage = TRUE;
        break;
    case ID_FITZOOM:
        XgFitZoom(hwnd);
        bUpdateImage = TRUE;
        break;
    case ID_RULEPRESET:
        XgOnRulePreset(hwnd);
        break;
    case ID_MOVEBOXES:
        for (auto& box : xg_boxes) {
            box->Bound();
        }
        break;
    case ID_ADDTEXTBOX:
    case ID_ADDPICTUREBOX:
        XgAddBox(hwnd, id);
        bUpdateImage = TRUE;
        break;
    case ID_DELETEBOX:
        {
            HWND hwndTarget = XG_BoxWindow::s_hwndSelected;
            for (auto it = xg_boxes.begin(); it != xg_boxes.end(); ++it) {
                if ((*it)->m_hWnd != hwndTarget)
                    continue;

                DestroyWindow(hwndTarget);

                auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                sa1->Get();
                {
                    xg_boxes.erase(it);
                }
                auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                sa2->Get();
                xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定する。

                // ファイルが変更された。
                XG_FILE_MODIFIED(TRUE);
                break;
            }
        }
        bUpdateImage = TRUE;
        break;
    case ID_BOXPROP:
        {
            HWND hwndTarget = XG_BoxWindow::s_hwndSelected;
            for (auto it = xg_boxes.begin(); it != xg_boxes.end(); ++it) {
                if ((*it)->m_hWnd != hwndTarget)
                    continue;

                auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                sa1->Get();
                if ((*it)->Prop(hwndTarget)) {
                    auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                    sa2->Get();
                    xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定する。

                    XG_FILE_MODIFIED(TRUE);
                }
                break;
            }
        }
        bUpdateImage = TRUE;
        break;
    case ID_UILANGID:
        XgSelectUILanguage(hwnd);
        break;
    case ID_REFLECTBLOCKS:
        if (!xg_bSolved) {
            xg_xword.Mirror();
        }
        bUpdateImage = TRUE;
        break;
    case ID_NUMCROMODE:
        if (xg_bSolved) {
            auto sa1 = std::make_shared<XG_UndoData_NumCro>();
            auto sa2 = std::make_shared<XG_UndoData_NumCro>();
            sa1->Get();
            xg_bNumCroMode = !xg_bNumCroMode;
            if (xg_bNumCroMode) {
                XgMakeItNumCro(hwnd);
            } else {
                xg_mapNumCro1.clear();
                xg_mapNumCro2.clear();
            }
            sa2->Get();
            xg_ubUndoBuffer.Commit(UC_NUMCRO, sa1, sa2);
        }
        bUpdateImage = TRUE;
        break;
    case ID_OPENPATTXT:
        {
            // PAT.txtを開く。
            WCHAR szPath[MAX_PATH];
            if (XgFindLocalFile(szPath, _countof(szPath), L"PAT.txt"))
                ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case ID_OPENTHEMETXT:
        {
            // THEME.txtを開く。
            WCHAR szPath[MAX_PATH];
            if (XgFindLocalFile(szPath, _countof(szPath), L"THEME.txt"))
                ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    case ID_CLEARUNDOBUFFER:
        // Undoバッファをクリアする。
        xg_ubUndoBuffer.clear();
        // 音を鳴らす。
        Beep(0x1000, 50);
        Beep(0x800, 450);
        bUpdateImage = TRUE;
        break;
    case ID_DOWNLOADDICT:
        ShellExecuteW(hwnd, nullptr, L"https://katahiromz.web.fc2.com/xword/dict", nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case ID_OPENPOLICYTXT:
        {
            // POLICY.txtを開く。
            WCHAR szPath[MAX_PATH];
            if (XgFindLocalFile(szPath, _countof(szPath), L"POLICY.txt"))
                ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;

    case ID_RECENT_00:
    case ID_RECENT_01:
    case ID_RECENT_02:
    case ID_RECENT_03:
    case ID_RECENT_04:
    case ID_RECENT_05:
    case ID_RECENT_06:
    case ID_RECENT_07:
    case ID_RECENT_08:
    case ID_RECENT_09:
        {
            // 最近使ったファイルを開く。
            const size_t i = id - ID_RECENT_00;
            if (i < xg_recently_used_files.size())
                XgOnLoad(hwnd, xg_recently_used_files[i].c_str());
        }
        break;

    default:
        if (!XgOnCommandExtra(hwnd, id)) {
            ::MessageBeep(0xFFFFFFFF);
        }
        break;
    }

    // 必要なら盤面のイメージを更新する。
    if (bUpdateImage) {
        if (x == -1 && y == -1)
            XgUpdateImage(hwnd);
        else
            XgUpdateImage(hwnd, x, y);
    }
    // ステータスバーを更新。
    XgUpdateStatusBar(hwnd);
    // ツールバーを更新。
    XgUpdateToolBarUI(hwnd);
}

// 無効状態のビットマップを作成する。
HBITMAP XgCreateGrayedBitmap(HBITMAP hbm, COLORREF crMask = CLR_INVALID) noexcept
{
    HDC hdc = ::GetDC(nullptr);
    if (::GetDeviceCaps(hdc, BITSPIXEL) < 24) {
        HPALETTE hPal = static_cast<HPALETTE>(::GetCurrentObject(hdc, OBJ_PAL));
        const UINT index = ::GetNearestPaletteIndex(hPal, crMask);
        if (index != CLR_INVALID)
            crMask = PALETTEINDEX(index);
    }
    ::ReleaseDC(nullptr, hdc);

    BITMAP bm;
    ::GetObject(hbm, sizeof(bm), &bm);

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = bm.bmWidth;
    bi.bmiHeader.biHeight = bm.bmHeight;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;

    RECT rc;
    rc.left = 0;
    rc.top = 0;
    rc.right = bm.bmWidth;
    rc.bottom = bm.bmHeight;

    HDC hdc1 = ::CreateCompatibleDC(nullptr);
    HDC hdc2 = ::CreateCompatibleDC(nullptr);

    LPVOID pvBits;
    HBITMAP hbmNew = ::CreateDIBSection(
        nullptr, &bi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
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
                    by = static_cast<BYTE>(
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
                        by = static_cast<BYTE>(
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
    constexpr auto style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPCHILDREN;
    if (!xg_canvasWnd.CreateWindowDx(hwnd, nullptr, style, WS_EX_ACCEPTFILES))
    {
        return false;
    }

    // サイズグリップを作る。
    xg_hSizeGrip = ::CreateWindowW(
        L"SCROLLBAR", nullptr,
        WS_CHILD | WS_VISIBLE | SBS_SIZEGRIP,
        0, 0, 0, 0,
        hwnd, nullptr, xg_hInstance, nullptr);
    if (xg_hSizeGrip == nullptr)
        return false;

    // イメージリストの準備をする。
    xg_hImageList = ::ImageList_Create(32, 32, ILC_COLOR24, 0, 0);
    if (xg_hImageList == nullptr)
        return FALSE;
    xg_hGrayedImageList = ::ImageList_Create(32, 32, ILC_COLOR24, 0, 0);
    if (xg_hGrayedImageList == nullptr)
        return FALSE;

    ::ImageList_SetBkColor(xg_hImageList, CLR_NONE);
    ::ImageList_SetBkColor(xg_hGrayedImageList, CLR_NONE);

    HBITMAP hbm = static_cast<HBITMAP>(::LoadImageW(
        xg_hInstance, MAKEINTRESOURCE(1),
        IMAGE_BITMAP,
        0, 0,
        LR_COLOR));
    ::ImageList_Add(xg_hImageList, hbm, nullptr);
    HBITMAP hbmGrayed = XgCreateGrayedBitmap(hbm);
    ::ImageList_Add(xg_hGrayedImageList, hbmGrayed, nullptr);
    ::DeleteObject(hbm);
    ::DeleteObject(hbmGrayed);

    // ツールバーのボタン情報。
    static const TBBUTTON atbb[] = {
        {0, ID_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {1, ID_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {2, ID_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {14, ID_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {15, ID_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {6, ID_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {7, ID_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
        {3, ID_GENERATE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {4, ID_GENERATEANSWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
        {5, ID_GENERATEREPEATEDLY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
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
    if (xg_hStatusBar == nullptr)
        return FALSE;

    XgUpdateStatusBar(hwnd);

    // ツールバーを作成する。
    constexpr int c_IDW_TOOLBAR = 1;
    xg_hToolBar = ::CreateWindowW(
        TOOLBARCLASSNAMEW, nullptr, 
        WS_CHILD | CCS_TOP | TBSTYLE_TOOLTIPS,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<UINT_PTR>(c_IDW_TOOLBAR)),
        xg_hInstance,
        nullptr);
    if (xg_hToolBar == nullptr)
        return FALSE;

    // ツールバーを初期化する。
    ::SendMessageW(xg_hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    ::SendMessageW(xg_hToolBar, TB_SETBITMAPSIZE, 0, MAKELPARAM(32, 32));
    ::SendMessageW(xg_hToolBar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(xg_hImageList));
    ::SendMessageW(xg_hToolBar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)xg_hGrayedImageList);
    ::SendMessageW(xg_hToolBar, TB_ADDBUTTONS, std::size(atbb), reinterpret_cast<LPARAM>(&atbb));
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
    if (s_dwNumberOfProcessors <= 2 && !s_bOldNotice) {
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
    // コントロールのレイアウトを更新する。
    ::SendMessageW(hwnd, WM_SIZE, 0, 0);

    int argc;
    LPWSTR *wargv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
    if (argc >= 2) {
        // 変換コマンドラインか？
        if (argc == 4 && lstrcmpiW(wargv[1], L"--convert") == 0) {
            // 読み込んで名前を付けて保存する。
            auto input = wargv[2];
            auto output = wargv[3];
            if (XgDoLoadFiles(hwnd, input) && XgDoSaveFiles(hwnd, output)) {
                PostQuitMessage(0);
            } else {
                PostQuitMessage(-1);
            }
            xg_bNoGUI = TRUE;
            return true;
        }
        else
        {
            // コマンドライン引数があれば、それを開く。
            XgOnLoad(hwnd, wargv[1]);
        }
    }
    GlobalFree(wargv);

    // ルールを更新する。
    XgUpdateRules(hwnd);
    // ツールバーのUIを更新する。
    XgUpdateToolBarUI(hwnd);
    // 辞書メニューの表示を更新。
    XgUpdateTheme(hwnd);

    return true;
}

// ポップアップメニューを読み込む。
HMENU XgLoadPopupMenu(HWND hwnd, int nPos) noexcept
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
void MainWnd_OnNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh) noexcept
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
        assert(IDS_TT_UNDO == ID_TT_BASE + ID_UNDO);
        assert(IDS_TT_REDO == ID_TT_BASE + ID_REDO);
    }
}

// ウィンドウのサイズを制限する。
void MainWnd_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept
{
    lpMinMaxInfo->ptMinTrackSize.x = 300;
    lpMinMaxInfo->ptMinTrackSize.y = 150;
}

//////////////////////////////////////////////////////////////////////////////

// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd, int x, int y)
{
    ForDisplay for_display;

    // 描画サイズを取得する。
    SIZE siz;
    XgGetXWordExtent(&siz);

    // 内部画像が残っているか？
    if (xg_hbmImage) {
        // 元の画像のサイズを取得する。
        BITMAP bm;
        if (::GetObject(xg_hbmImage, sizeof(bm), &bm)) {
            // 画像が小さいか大きすぎるなら破棄する。
            if (bm.bmWidth < siz.cx || bm.bmHeight < siz.cy ||
                bm.bmWidth > 3 * siz.cx || bm.bmHeight > 3 * siz.cy)
            {
                ::DeleteObject(xg_hbmImage);
                xg_hbmImage = nullptr;
            }
        }
    }

    if (xg_hbmImage == nullptr) {
        // 必要ならイメージを作成する。
        if (xg_bSolved && xg_bShowAnswer)
            xg_hbmImage = XgCreateXWordImage(xg_solution, &siz, true);
        else
            xg_hbmImage = XgCreateXWordImage(xg_xword, &siz, true);
    } else {
        // 必要ならイメージを描画する。
        HDC hdc = ::CreateCompatibleDC(NULL);
        HGDIOBJ hbmOld = ::SelectObject(hdc, xg_hbmImage);
        if (xg_bSolved && xg_bShowAnswer)
            XgDrawXWord(xg_solution, hdc, &siz, DRAW_MODE_SCREEN);
        else
            XgDrawXWord(xg_xword, hdc, &siz, DRAW_MODE_SCREEN);
        ::SelectObject(hdc, hbmOld);
    }

    // スクロール情報を更新する。
    XgUpdateScrollInfo(hwnd, x, y);

    // 再描画する。
    MRect rcClient;
    XgGetRealClientRect(hwnd, &rcClient);
    ::InvalidateRect(xg_canvasWnd, &rcClient, TRUE);

    // ツールバーを更新する。
    XgUpdateToolBarUI(hwnd);
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
    XgUpdateImage(hwnd);
}

// ウィンドウを閉じようとした。
void MainWnd_OnClose(HWND hwnd)
{
    if (XgDoConfirmSave(hwnd))
    {
        DestroyWindow(hwnd);
    }
}

// 最大化状態を保存するために使用。
void MainWnd_OnWindowPosChanged(HWND hwnd, const LPWINDOWPOS lpwpos) noexcept
{
    if (xg_hMainWnd)
    {
        // 最大化状態を保存する。
        xg_bMainWndMaximized = ::IsZoomed(hwnd);
    }

    // デフォルトの処理。
    FORWARD_WM_WINDOWPOSCHANGED(hwnd, lpwpos, DefWindowProcW);
}

// ウィンドウプロシージャ。
extern "C"
LRESULT CALLBACK
XgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    HANDLE_MSG(hWnd, WM_CREATE, MainWnd_OnCreate);
    HANDLE_MSG(hWnd, WM_CLOSE, MainWnd_OnClose);
    HANDLE_MSG(hWnd, WM_DESTROY, MainWnd_OnDestroy);
    HANDLE_MSG(hWnd, WM_MOVE, MainWnd_OnMove);
    HANDLE_MSG(hWnd, WM_SIZE, MainWnd_OnSize);
    HANDLE_MSG(hWnd, WM_WINDOWPOSCHANGED, MainWnd_OnWindowPosChanged);
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
    constexpr auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
    constexpr auto exstyle = WS_EX_TOOLWINDOW;
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
void XgDestroyHintsWnd(void) noexcept
{
    // ヒントウィンドウが存在するか？
    if (xg_hHintsWnd && ::IsWindow(xg_hHintsWnd)) {
        // 更新を無視・破棄する。
        HWND hwnd = xg_hHintsWnd;
        xg_hHintsWnd = nullptr;
        ::DestroyWindow(hwnd);
    }
}

// ヒントの内容をヒントウィンドウで開く。
bool XgOpenHintsByWindow(HWND hwnd)
{
    // もしヒントウィンドウが存在すれば破棄する。
    if (xg_hHintsWnd) {
        HWND hwndSave = xg_hHintsWnd;
        xg_hHintsWnd = nullptr;
        ::DestroyWindow(hwndSave);
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
HHOOK xg_hCtrlAHook = nullptr;

// hook proc for Ctrl+A
LRESULT CALLBACK XgCtrlAMessageProc(int nCode, WPARAM wParam, LPARAM lParam) noexcept
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

    // xg_str_escape
    assert(xg_str_escape(L"\a") == L"\\a");
    assert(xg_str_escape(L"\b") == L"\\b");
    assert(xg_str_escape(L"\t") == L"\\t");
    assert(xg_str_escape(L"\n") == L"\\n");
    assert(xg_str_escape(L"\r") == L"\\r");
    assert(xg_str_escape(L"\f") == L"\\f");
    assert(xg_str_escape(L"\v") == L"\\v");
    assert(xg_str_escape(L"\\") == L"\\\\");
    assert(xg_str_escape(L"\aabc") == L"\\aabc");
    assert(xg_str_escape(L"\babc") == L"\\babc");
    assert(xg_str_escape(L"\tabc") == L"\\tabc");
    assert(xg_str_escape(L"\nabc") == L"\\nabc");
    assert(xg_str_escape(L"\rabc") == L"\\rabc");
    assert(xg_str_escape(L"\fabc") == L"\\fabc");
    assert(xg_str_escape(L"\vabc") == L"\\vabc");
    assert(xg_str_escape(L"\\abc") == L"\\\\abc");

    // xg_str_unescape
    assert(xg_str_unescape(L"\\a") == L"\a");
    assert(xg_str_unescape(L"\\b") == L"\b");
    assert(xg_str_unescape(L"\\t") == L"\t");
    assert(xg_str_unescape(L"\\n") == L"\n");
    assert(xg_str_unescape(L"\\r") == L"\r");
    assert(xg_str_unescape(L"\\f") == L"\f");
    assert(xg_str_unescape(L"\\v") == L"\v");
    assert(xg_str_unescape(L"\\\\") == L"\\");
    assert(xg_str_unescape(L"\\aabc") == L"\aabc");
    assert(xg_str_unescape(L"\\babc") == L"\babc");
    assert(xg_str_unescape(L"\\tabc") == L"\tabc");
    assert(xg_str_unescape(L"\\nabc") == L"\nabc");
    assert(xg_str_unescape(L"\\rabc") == L"\rabc");
    assert(xg_str_unescape(L"\\fabc") == L"\fabc");
    assert(xg_str_unescape(L"\\vabc") == L"\vabc");
    assert(xg_str_unescape(L"\\\\abc") == L"\\abc");
    assert(xg_str_unescape(L"\\x11T") == L"\021T");
    assert(xg_str_unescape(L"\\021T") == L"\021T");

    // パターンの単体テスト。
    BOOL XgPatternsUnitTest(void);
    XgPatternsUnitTest();
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
    // クリティカルセクションを初期化する。
    ::InitializeCriticalSection(&xg_cs);

    // クリティカルセクションを自動的に破棄する。
    struct AutoDeleteCriticalSection
    {
        AutoDeleteCriticalSection() = default;
        ~AutoDeleteCriticalSection()
        {
            // クリティカルセクションを破棄する。
            ::DeleteCriticalSection(&xg_cs);
        }
    };
    AutoDeleteCriticalSection xg_auto_cs;

    // テストをする。
    XgDoTests();

    // アプリのインスタンスを保存する。
    xg_hInstance = hInstance;

    // 設定を読み込む。
    XgLoadSettings();

    // UI言語の設定。
    if (xg_UILangID)
        SetThreadUILanguage(xg_UILangID);
    else
        xg_UILangID = GetThreadUILanguage();

    // 辞書ファイルの名前を読み込む。
    XgLoadDictsAll();

    // 乱数モジュールを初期化する。
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());

    // プロセッサの数を取得する。
    SYSTEM_INFO si;
    ::GetSystemInfo(&si);
    s_dwNumberOfProcessors = si.dwNumberOfProcessors;

#ifdef SINGLE_THREAD_MODE
    xg_dwThreadCount = 1;
#else
    // プロセッサの数に合わせてスレッドの数を決める。
    if (s_dwNumberOfProcessors <= 3)
        xg_dwThreadCount = 2;
    else
        xg_dwThreadCount = s_dwNumberOfProcessors - 1;
#endif

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
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_3DFACE + 1));
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

    // 前回最大化されたか？
    const BOOL bZoomed = xg_bMainWndMaximized;

    // メインウィンドウを作成する。
    const DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
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
        ::ShowWindowAsync(xg_hMainWnd, (bZoomed ? SW_SHOWMAXIMIZED : nCmdShow));
        ::UpdateWindow(xg_hMainWnd);
    }

    // Ctrl+Aの機能を有効にする。
    xg_hCtrlAHook = ::SetWindowsHookEx(WH_MSGFILTER,
        XgCtrlAMessageProc, nullptr, ::GetCurrentThreadId());

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

        if (xg_hMarkingDlg) {
            if (::IsDialogMessageW(xg_hMarkingDlg, &msg))
                continue;
        }

        if (!::TranslateAcceleratorW(xg_hMainWnd, s_hAccel, &msg)) {
            ::TranslateMessage(&msg);
            ::DispatchMessageW(&msg);
        }
    }

    // Ctrl+Aの機能を解除する。
    ::UnhookWindowsHookEx(xg_hCtrlAHook);
    xg_hCtrlAHook = nullptr;

    // 設定を保存。
    XgSaveSettings();

    return static_cast<int>(msg.wParam);
}

//////////////////////////////////////////////////////////////////////////////
