//////////////////////////////////////////////////////////////////////////////
// XWordGiver.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#ifndef __XWORDGIVER_XG_H__
#define __XWORDGIVER_XG_H__

#ifndef __cplusplus
    #error You lose.    // not C++ compiler
#endif

#if __cplusplus < 199711L
    #error Modern C++ required. You lose.
#endif

#if __cplusplus < 201103L
    #define xg_constexpr   /*empty*/
#else
    #define xg_constexpr   constexpr
#endif

#if !defined(NDEBUG) && !defined(_DEBUG)
    #define _DEBUG
#endif

#define _CRT_SECURE_NO_WARNINGS // kill security warnings
#define _CRT_NON_CONFORMING_WCSTOK  // use legacy wcstok 

////////////////////////////////////////////////////////////////////////////

#include "TargetVer.h"  // for WINVER, _WIN32_WINNT, _WIN32_IE

#include <windows.h>    // for Windows API
#include <windowsx.h>   // for HANDLE_MSG
#include <commctrl.h>   // common controls
#include <shlobj.h>     // for CoCreateInstance, IShellLink, IPersistFile
#include <imm.h>        // for ImmSetOpenStatus
#include <shlwapi.h>    // Shell Light-weight API

#include <cstdlib>      // for std::memcpy, std::memset
#include <cassert>      // for assert
#include <vector>       // for std::vector
#include <set>          // for std::set, std::multiset
#include <map>          // for std::map

#include <algorithm>        // for std::replace
#include <unordered_set>    // for std::unordered_set, std::unordered_multiset
#include <unordered_map>    // for std::unordered_map
#include <utility>          // for std::move

#include <queue>        // for std::queue
#include <deque>        // for std::deque
#include <algorithm>    // for std::sort, std::random_shuffle
#include <string>       // for std::wstring

#include <process.h>    // for _beginthreadex

#include <strsafe.h>    // for String... functions

#include "resource.h"   // resource-related macros

////////////////////////////////////////////////////////////////////////////

#include "MPointSizeRect.hpp"
#include "MRegKey.hpp"
#include "MScrollView.hpp"

#include "Dictionary.hpp"
#include "xword.hpp"
#include "Marks.hpp"
#include "SaveBitmapToFile.h"

////////////////////////////////////////////////////////////////////////////

#undef min
#undef max

#include "nlohmann/json.hpp"
using json = nlohmann::json;

//////////////////////////////////////////////////////////////////////////////
// std::wstringを比較するファンクタ。

class xg_wstrinxg_less
{
public:
    bool __fastcall operator()(
        const std::wstring& s1, const std::wstring& s2) const
    {
        return s1 < s2;
    }
};

// 長さでstd::wstringを比較するファンクタ。
class xg_wstring_size_greater
{
public:
    bool __fastcall operator()(
        const std::wstring& s1, const std::wstring& s2) const
    {
        return s1.size() > s2.size();
    }
};

//////////////////////////////////////////////////////////////////////////////
// 入力モード。

enum XG_InputMode
{
    xg_im_KANA,     // カナ入力。
    xg_im_ABC,      // 英字入力。
    xg_im_KANJI,    // 漢字入力。
    xg_im_RUSSIA,   // ロシア文字。
};
extern XG_InputMode xg_imode;

//////////////////////////////////////////////////////////////////////////////
// 文字の判定。

// 半角大文字英字か？
inline bool XgIsCharHankakuUpperW(WCHAR ch)
{
    return (L'A' <= ch && ch <= L'Z');
}

// 半角小文字英字か？
inline bool XgIsCharHankakuLowerW(WCHAR ch)
{
    return (L'a' <= ch && ch <= L'z');
}

// 半角英字か？
inline bool XgIsCharHankakuAlphaW(WCHAR ch)
{
    return XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch);
}

// 全角大文字英字か？
inline bool XgIsCharZenkakuUpperW(WCHAR ch)
{
    return (L'\xFF21' <= ch && ch <= L'\xFF3A');
}

// 全角小文字英字か？
inline bool XgIsCharZenkakuLowerW(WCHAR ch)
{
    return (L'\xFF41' <= ch && ch <= L'\xFF5A');
}

// 全角英字か？
inline bool XgIsCharZenkakuAlphaW(WCHAR ch)
{
    return XgIsCharZenkakuUpperW(ch) || XgIsCharZenkakuLowerW(ch);
}

// ひらがなか？
inline bool XgIsCharHiraganaW(WCHAR ch)
{
    return ((L'\x3041' <= ch && ch <= L'\x3093') || ch == L'\x30FC' || ch == L'\x3094');
}

// カタカナか？
inline bool XgIsCharKatakanaW(WCHAR ch)
{
    return ((L'\x30A1' <= ch && ch <= L'\x30F3') || ch == L'\x30FC' || ch == L'\x30F4');
}

// かなか？
inline bool XgIsCharKanaW(WCHAR ch)
{
    return XgIsCharHiraganaW(ch) || XgIsCharKatakanaW(ch);
}

// 漢字か？
inline bool XgIsCharKanjiW(WCHAR ch)
{
    return ((0x3400 <= ch && ch <= 0x9FFF) ||
            (0xF900 <= ch && ch <= 0xFAFF) || ch == L'\x3007');
}

// ハングルか？
inline bool XgIsCharHangulW(WCHAR ch)
{
    return ((0x1100 <= ch && ch <= 0x11FF) ||
            (0xAC00 <= ch && ch <= 0xD7A3) ||
            (0x3130 <= ch && ch <= 0x318F));
}

// キリル文字か？
inline bool XgIsCharZenkakuCyrillicW(WCHAR ch)
{
    return 0x0400 <= ch && ch <= 0x04FF;
}

//////////////////////////////////////////////////////////////////////////////

// スレッド情報。
struct XG_ThreadInfo
{
    // スレッドID。
    unsigned    m_threadid;
    // 空ではないマスの数。
    int         m_count;
};

// スレッド情報。
extern std::vector<XG_ThreadInfo>       xg_aThreadInfo;

//////////////////////////////////////////////////////////////////////////////
// 格納情報。

// 単語の格納情報。
struct XG_PlaceInfo
{
    int             m_iRow;         // 行のインデックス。
    int             m_jCol;         // 列のインデックス。
    std::wstring    m_word;         // 単語。
    int             m_number;       // 番号。

    // コンストラクタ。
    XG_PlaceInfo()
    {
    }

    // コンストラクタ。
    XG_PlaceInfo(int iRow_, int jCol_, const std::wstring& word_) :
        m_iRow(iRow_), m_jCol(jCol_), m_word(word_)
    {
    }

    // コンストラクタ。
    XG_PlaceInfo(int iRow_, int jCol_, std::wstring&& word_) :
        m_iRow(iRow_), m_jCol(jCol_), m_word(std::move(word_))
    {
    }

    // コンストラクタ。
    XG_PlaceInfo(int iRow_, int jCol_, const std::wstring& word_, int number_) :
        m_iRow(iRow_), m_jCol(jCol_), m_word(word_), m_number(number_)
    {
    }

    // コンストラクタ。
    XG_PlaceInfo(int iRow_, int jCol_, std::wstring&& word_, int number_) :
        m_iRow(iRow_), m_jCol(jCol_), m_word(std::move(word_)), m_number(number_)
    {
    }

    // コピーコンストラクタ。
    XG_PlaceInfo(const XG_PlaceInfo& pi)
    {
        m_iRow = pi.m_iRow;
        m_jCol = pi.m_jCol;
        m_word = pi.m_word;
        m_number = pi.m_number;
    }

    // コピーコンストラクタ。
    XG_PlaceInfo(XG_PlaceInfo&& pi)
    {
        m_iRow = pi.m_iRow;
        m_jCol = pi.m_jCol;
        m_word = std::move(pi.m_word);
        m_number = pi.m_number;
    }

    // 代入。
    void __fastcall operator=(const XG_PlaceInfo& pi)
    {
        m_iRow = pi.m_iRow;
        m_jCol = pi.m_jCol;
        m_word = pi.m_word;
        m_number = pi.m_number;
    }

    // 代入。
    void __fastcall operator=(XG_PlaceInfo&& pi)
    {
        m_iRow = pi.m_iRow;
        m_jCol = pi.m_jCol;
        m_word = std::move(pi.m_word);
        m_number = pi.m_number;
    }
};

namespace std
{
    inline void swap(XG_PlaceInfo& info1, XG_PlaceInfo& info2)
    {
        std::swap(info1.m_iRow, info2.m_iRow);
        std::swap(info1.m_jCol, info2.m_jCol);
        std::swap(info1.m_word, info2.m_word);
        std::swap(info1.m_number, info2.m_number);
    }
}

// タテとヨコのかぎ。
extern std::vector<XG_PlaceInfo> xg_vTateInfo, xg_vYokoInfo;

//////////////////////////////////////////////////////////////////////////////

// XG_PlaceInfo構造体を番号で比較するファンクタ。
class xg_placeinfo_compare_number
{
public:
    xg_constexpr bool __fastcall operator()
        (const XG_PlaceInfo& pi1, const XG_PlaceInfo& pi2) const
    {
        return pi1.m_number < pi2.m_number;
    }
};

// XG_PlaceInfo構造体を位置で比較するファンクタ。
class xg_placeinfo_compare_position
{
public:
    bool __fastcall operator()
        (const XG_PlaceInfo *ppi1, const XG_PlaceInfo *ppi2) const
    {
        if (ppi1->m_iRow < ppi2->m_iRow)
            return true;
        if (ppi1->m_iRow > ppi2->m_iRow)
            return false;
        if (ppi1->m_jCol < ppi2->m_jCol)
            return true;
        return false;
    }
};

//////////////////////////////////////////////////////////////////////////////
// Utils.cpp

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx1(int id);
// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx2(int id);

// ダイアログを中央によせる関数。
void __fastcall XgCenterDialog(HWND hwnd);
// メッセージボックスフック用の関数。
extern "C" LRESULT CALLBACK
XgMsgBoxCbtProc(int nCode, WPARAM wParam, LPARAM /*lParam*/);
// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType);
// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxIndirectW(LPMSGBOXPARAMS lpMsgBoxParams);

// 文字列を置換する。
void __fastcall xg_str_replace_all(
    std::wstring &s, const std::wstring& from, const std::wstring& to);
// 文字列の前後の空白を取り除く。
void __fastcall xg_str_trim(std::wstring& str);
// ショートカットのターゲットのパスを取得する。
bool __fastcall XgGetPathOfShortcutW(LPCWSTR pszLnkFile, LPWSTR pszPath);
// フィルター文字列を作る。
LPWSTR __fastcall XgMakeFilterString(LPWSTR psz);

// 文字列からマルチセットへ変換する。
void __fastcall xg_str_to_multiset(
    std::unordered_multiset<WCHAR>& mset, const std::wstring& str);
// ベクターからマルチセットへ変換する。
void __fastcall xg_vec_to_multiset(
    std::unordered_multiset<WCHAR>& mset, const std::vector<WCHAR>& str);
// 部分マルチセットかどうか？
bool __fastcall xg_submultiseteq(const std::unordered_multiset<WCHAR>& ms1,
                                 const std::unordered_multiset<WCHAR>& ms2);
// ReadMeを開く。
void __fastcall XgOpenReadMe(HWND hwnd);
// Licenseを開く。
void __fastcall XgOpenLicense(HWND hwnd);
// パターンを開く。
void __fastcall XgOpenPatterns(HWND hwnd);

// ファイルが書き込み可能か？
bool __fastcall XgCanWriteFile(const WCHAR *pszFile);

void __fastcall XgSetInputModeFromDict(HWND hwnd);

// Unicode -> UTF8
std::string XgUnicodeToUtf8(const std::wstring& wide);

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi);

// Unicode -> ANSI
std::string XgUnicodeToAnsi(const std::wstring& wide);

// UTF-8 -> Unicode.
std::wstring __fastcall XgUtf8ToUnicode(const std::string& ansi);

// JSON文字列を作る。
std::wstring XgJsonEncodeString(const std::wstring& str);

// パスを作る。
BOOL XgMakePathW(LPCWSTR pszPath);

// エンディアン変換。
void XgSwab(LPBYTE pbFile, DWORD cbFile);

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(
    const std::string& html_utf8, const std::string& style_utf8 = "");

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(
    const std::wstring& html_wide, const std::wstring& style_wide = L"");

//////////////////////////////////////////////////////////////////////////////

// 改行。
extern const LPCWSTR xg_pszNewLine;

// インスタンスのハンドル。
extern HINSTANCE   xg_hInstance;

// メインウィンドウのハンドル。
extern HWND        xg_hMainWnd;

// ツールバーのハンドル。
extern HWND        xg_hToolBar;

// ステータスバーのハンドル。
extern HWND        xg_hStatusBar;

// ヒントウィンドウのハンドル。
extern HWND        xg_hHintsWnd;

// 答えを表示するか？
extern bool xg_bShowAnswer;

// 入力パレットを表示するか？
extern bool xg_bShowInputPalette;

// 入力パレットの位置。
extern INT xg_nInputPaletteWndX;
extern INT xg_nInputPaletteWndY;

// 黒マス追加なしか？
extern bool xg_bNoAddBlack;

// 排他制御のためのクリティカルセクション。
extern CRITICAL_SECTION xg_cs;

// スレッドの数。
extern DWORD xg_dwThreadCount;

// 計算がキャンセルされたか？
extern bool xg_bCancelled;

// 空のクロスワードの解を解く場合か？
extern bool xg_bSolvingEmpty;

// 黒マスパターンが生成されたか？
extern bool xg_bBlacksGenerated;

// スマート解決か？
extern bool xg_bSmartResolution;

// 太枠をつけるか？
extern bool xg_bAddThickFrame;

// 「入力パレット」縦置き？
extern bool xg_bTateOki;

// 直前に開いたクロスワードデータファイルのパスファイル名。
extern std::wstring xg_strFileName;

// ヒント追加フラグ。
extern bool xg_bHintsAdded;

// JSONファイルとして保存するか？
extern bool xg_bSaveAsJsonFile;

// 拗音変換用データ。
extern const LPCWSTR xg_small[11];
extern const LPCWSTR xg_large[11];

// ビットマップのハンドル。
extern HBITMAP          xg_hbmImage;

// ヒント文字列。
extern std::wstring     xg_strHints;

// 再計算しなおしているか？
extern bool             xg_bRetrying;

// スレッドのハンドル。
extern std::vector<HANDLE> xg_ahThreads;

// マスのフォント。
extern WCHAR xg_szCellFont[];

// 番号のフォント。
extern WCHAR xg_szSmallFont[];

// 直前に押したキーを覚えておく。
extern WCHAR xg_prev_vk;

// 入力パレット。
extern HWND xg_hwndInputPalette;

// ひらがな表示か？
extern BOOL xg_bHiragana;
// Lowercase表示か？
extern BOOL xg_bLowercase;

// 文字の大きさ（％）。
extern INT xg_nCellCharPercents;

// 小さい文字の大きさ（％）。
extern INT xg_nSmallCharPercents;

// 「黒マスパターン」ダイアログの位置とサイズ。
extern INT xg_nPatWndX;
extern INT xg_nPatWndY;
extern INT xg_nPatWndCX;
extern INT xg_nPatWndCY;

// 現在の辞書名。
extern std::wstring xg_dict_name;
// すべての辞書ファイル。
extern std::deque<std::wstring>  xg_dict_files;

// 辞書名をセットする。
void XgSetDict(const std::wstring& strFile);

//////////////////////////////////////////////////////////////////////////////

// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd, int x, int y);

// ファイルを開く。
bool __fastcall XgDoLoadFile(HWND hwnd, LPCWSTR pszFile, bool json);
// ファイルを開く。
bool __fastcall XgDoLoadCrpFile(HWND hwnd, LPCWSTR pszFile);

// ファイルを保存する。
bool __fastcall XgDoSave(HWND /*hwnd*/, LPCWSTR pszFile);

// ファイル（JSON形式）を保存する。
bool __fastcall XgDoSaveJson(HWND /*hwnd*/, LPCWSTR pszFile);
// ファイル（CRP形式）を保存する。
bool __fastcall XgDoSaveCrpFile(HWND /*hwnd*/, LPCWSTR pszFile);

// ヒントを表示する。
void __fastcall XgShowHints(HWND hwnd);

// ヒントの内容をヒントウィンドウで開く。
bool __fastcall XgOpenHintsByWindow(HWND /*hwnd*/);

// ヒントの内容をメモ帳で開く。
bool __fastcall XgOpenHintsByNotepad(HWND /*hwnd*/, bool bShowAnswer);

// ヒントが変更されたか？
bool __fastcall XgAreHintsModified(void);

// ヒントデータを更新する。
bool __fastcall XgUpdateHintsData(void);

// ヒントデータを設定する。
void __fastcall XgSetHintsData(void);

// ヒント文字列を解析する。
bool __fastcall XgParseHintsStr(HWND hwnd, const std::wstring& strHints);

// 問題を画像ファイルとして保存する。
void __fastcall XgSaveProbAsImage(HWND hwnd);

// 解答を画像ファイルとして保存する。
void __fastcall XgSaveAnsAsImage(HWND hwnd);

// ヒントウィンドウを作成する。
BOOL XgCreateHintsWnd(HWND hwnd);
// ヒントウィンドウを破棄する。
void XgDestroyHintsWnd(void);

template <bool t_alternative>
bool __fastcall XgGetCandidatesAddBlack(
    std::vector<std::wstring>& cands, const std::wstring& pattern, int& nSkip,
    bool left_black_check, bool right_black_check);

// UIフォントの論理オブジェクトを取得する。
LOGFONTW *XgGetUIFont(void);

// 候補の内容を候補ウィンドウで開く。
bool __fastcall XgOpenCandsWnd(HWND hwnd, bool vertical);

// 候補ウィンドウを破棄する。
void XgDestroyCandsWnd(void);

// 入力パレットを作成する。
BOOL XgCreateInputPalette(HWND hwndOwner);

// 入力パレットを破棄する。
BOOL XgDestroyInputPalette(void);

// 入力モードを切り替える。
void __fastcall XgSetInputMode(HWND hwnd, XG_InputMode mode);

// 文字が入力された。
void __fastcall MainWnd_OnChar(HWND hwnd, TCHAR ch, int cRepeat);

// BackSpaceを実行する。
void __fastcall XgCharBack(HWND hwnd);

// 入力方向を切り替える。
void __fastcall XgInputDirection(HWND hwnd, INT nDirection);
// 文字送りを切り替える。
void __fastcall XgSetCharFeed(HWND hwnd, INT nMode);
// 改行する。
void __fastcall XgReturn(HWND hwnd);
// 文字をクリア。
void __fastcall XgClearNonBlocks(HWND hwnd);

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacks(void *param);
// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksSmart(void *param);
// 二重マス切り替え。
void __fastcall XgToggleMark(HWND hwnd);

// クロスワードで使う文字に変換する。
std::wstring __fastcall XgNormalizeString(const std::wstring& text);

// ポップアップメニューを読み込む。
HMENU XgLoadPopupMenu(HWND hwnd, INT nPos);

//////////////////////////////////////////////////////////////////////////////
// スクロール。

// 水平スクロールの位置を取得する。
int __fastcall XgGetHScrollPos(void);
// 垂直スクロールの位置を取得する。
int __fastcall XgGetVScrollPos(void);
// 水平スクロールの情報を取得する。
BOOL __fastcall XgGetHScrollInfo(LPSCROLLINFO psi);
// 垂直スクロールの情報を取得する。
BOOL __fastcall XgGetVScrollInfo(LPSCROLLINFO psi);
// 水平スクロールの位置を設定する。
int __fastcall XgSetHScrollPos(int nPos, BOOL bRedraw);
// 垂直スクロールの位置を設定する。
int __fastcall XgSetVScrollPos(int nPos, BOOL bRedraw);
// スクロール情報を設定する。
void __fastcall XgUpdateScrollInfo(HWND hwnd, int x, int y);
// キャレットが見えるように、必要ならばスクロールする。
void __fastcall XgEnsureCaretVisible(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////
// 候補。

extern int  xg_iCandPos;
extern int  xg_jCandPos;
extern bool xg_bCandVertical;

//////////////////////////////////////////////////////////////////////////////

#include "UndoBuffer.hpp"

#endif  // ndef __XWORDGIVER_XG_H__
