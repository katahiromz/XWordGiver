//////////////////////////////////////////////////////////////////////////////
// XWordGiver.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#ifndef XWORDGIVER
#define XWORDGIVER

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

//////////////////////////////////////////////////////////////////////////////
// クロスワード。

// クロスワードのサイズの制限。
#define XG_MIN_SIZE 2
#define XG_MAX_SIZE 256

// 単語の長さの制限。
#define XG_MIN_WORD_LEN 2
#define XG_MAX_WORD_LEN 100

// 日本語ロケール。
#define JPN_LOCALE \
    MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT)

// ロシア語ロケール。
#define RUS_LOCALE \
    MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_DEFAULT), SORT_DEFAULT)

// Shift_JIS codepage
#define SJIS_CODEPAGE 932

// Russian codepage
#define RUS_CODEPAGE 1251

// 既定の文字の大きさ(%)。
#define DEF_CELL_CHAR_SIZE 66
#define DEF_SMALL_CHAR_SIZE 30

// 表示用に描画するか？（XgGetXWordExtentとXgDrawXWordとXgCreateXWordImageで使う）。
extern INT xg_nForDisplay;
struct ForDisplay {
    ForDisplay() {
        ++xg_nForDisplay;
    }
    ~ForDisplay() {
        --xg_nForDisplay;
    }
};

// ズーム比率(%)。
extern INT xg_nZoomRate;
// セルの大きさ。
#define xg_nCellSize        48
// 上下左右のマージン。
#define xg_nMargin          16
// 上下左右のマージン。
#define xg_nNarrowMargin    8
// 候補の最大数。
#define xg_nMaxCandidates   500

// クロスワードで使う文字に変換する。
std::wstring __fastcall XgNormalizeString(const std::wstring& text);
// 文字列を標準化する。
std::wstring XgNormalizeStringEx(const std::wstring& str, BOOL bUppercase = TRUE, BOOL bKatakana = TRUE);

// XG_Board::EveryPatternValid1, XG_Board::EveryPatternValid2の戻り値。
enum XG_EpvCode
{
    xg_epv_SUCCESS,        // 成功。
    xg_epv_PATNOTMATCH,    // マッチしなかったパターンがある。
    xg_epv_DOUBLEWORD,     // 単語が重複している。
    xg_epv_LENGTHMISMATCH, // 空白の長さが一致しない。
    xg_epv_NOTFOUNDWORD    // 単語にマッチしなかったマスがある。
};

// セルの色。
extern COLORREF xg_rgbWhiteCellColor;
extern COLORREF xg_rgbBlackCellColor;
extern COLORREF xg_rgbMarkedCellColor;

// 二重マスに枠を描くか？
extern bool xg_bDrawFrameForMarkedCell;

// 文字送り？
extern bool xg_bCharFeed;

// スマート解決のとき、配置できる最大単語長。
extern INT xg_nMaxWordLen;

// ナンクロモードか？
extern bool xg_bNumCroMode;
// ナンクロモードの場合、写像を保存する。
extern std::unordered_map<WCHAR, INT> xg_mapNumCro1;
extern std::unordered_map<INT, WCHAR> xg_mapNumCro2;
// ナンクロの写像を生成する。
void __fastcall XgMakeItNumCro(HWND hwnd);
// ナンクロの写像が正当か確認する。
BOOL __fastcall XgValidateNumCro(HWND hwnd);

// 全角文字。
#define ZEN_SPACE       WCHAR(0x3000)  // L'　'
#define ZEN_BLACK       WCHAR(0x25A0)  // L'■'
#define ZEN_LARGE_A     WCHAR(0xFF21)  // L'Ａ'
#define ZEN_LARGE_Z     WCHAR(0xFF3A)  // L'Ｚ'
#define ZEN_SMALL_A     WCHAR(0xFF41)  // L'ａ'
#define ZEN_SMALL_Z     WCHAR(0xFF5A)  // L'ｚ'
#define ZEN_A           WCHAR(0x30A2)  // L'ア'
#define ZEN_I           WCHAR(0x30A4)  // L'イ'
#define ZEN_U           WCHAR(0x30A6)  // L'ウ'
#define ZEN_E           WCHAR(0x30A8)  // L'エ'
#define ZEN_O           WCHAR(0x30AA)  // L'オ'
#define ZEN_KA          WCHAR(0x30AB)  // L'カ'
#define ZEN_KI          WCHAR(0x30AD)  // L'キ'
#define ZEN_KU          WCHAR(0x30AF)  // L'ク'
#define ZEN_KE          WCHAR(0x30B1)  // L'ケ'
#define ZEN_KO          WCHAR(0x30B3)  // L'コ'
#define ZEN_SA          WCHAR(0x30B5)  // L'サ'
#define ZEN_SI          WCHAR(0x30B7)  // L'シ'
#define ZEN_SU          WCHAR(0x30B9)  // L'ス'
#define ZEN_SE          WCHAR(0x30BB)  // L'セ'
#define ZEN_SO          WCHAR(0x30BD)  // L'ソ'
#define ZEN_TA          WCHAR(0x30BF)  // L'タ'
#define ZEN_CHI         WCHAR(0x30C1)  // L'チ'
#define ZEN_TSU         WCHAR(0x30C4)  // L'ツ'
#define ZEN_TE          WCHAR(0x30C6)  // L'テ'
#define ZEN_TO          WCHAR(0x30C8)  // L'ト'
#define ZEN_NA          WCHAR(0x30CA)  // L'ナ'
#define ZEN_NI          WCHAR(0x30CB)  // L'ニ'
#define ZEN_NU          WCHAR(0x30CC)  // L'ヌ'
#define ZEN_NE          WCHAR(0x30CD)  // L'ネ'
#define ZEN_NO          WCHAR(0x30CE)  // L'ノ'
#define ZEN_NN          WCHAR(0x30F3)  // L'ン'
#define ZEN_HA          WCHAR(0x30CF)  // L'ハ'
#define ZEN_HI          WCHAR(0x30D2)  // L'ヒ'
#define ZEN_FU          WCHAR(0x30D5)  // L'フ'
#define ZEN_HE          WCHAR(0x30D8)  // L'ヘ'
#define ZEN_HO          WCHAR(0x30DB)  // L'ホ'
#define ZEN_MA          WCHAR(0x30DE)  // L'マ'
#define ZEN_MI          WCHAR(0x30DF)  // L'ミ'
#define ZEN_MU          WCHAR(0x30E0)  // L'ム'
#define ZEN_ME          WCHAR(0x30E1)  // L'メ'
#define ZEN_MO          WCHAR(0x30E2)  // L'モ'
#define ZEN_YA          WCHAR(0x30E4)  // L'ヤ'
#define ZEN_YU          WCHAR(0x30E6)  // L'ユ'
#define ZEN_YO          WCHAR(0x30E8)  // L'ヨ'
#define ZEN_RA          WCHAR(0x30E9)  // L'ラ'
#define ZEN_RI          WCHAR(0x30EA)  // L'リ'
#define ZEN_RU          WCHAR(0x30EB)  // L'ル'
#define ZEN_RE          WCHAR(0x30EC)  // L'レ'
#define ZEN_RO          WCHAR(0x30ED)  // L'ロ'
#define ZEN_WA          WCHAR(0x30EF)  // L'ワ'
#define ZEN_WI          WCHAR(0x30F0)  // L'ヰ'
#define ZEN_WE          WCHAR(0x30F1)  // L'ヱ'
#define ZEN_WO          WCHAR(0x30F2)  // L'ヲ'
#define ZEN_GA          WCHAR(0x30AC)  // L'ガ'
#define ZEN_GI          WCHAR(0x30AE)  // L'ギ'
#define ZEN_GU          WCHAR(0x30B0)  // L'グ'
#define ZEN_GE          WCHAR(0x30B2)  // L'ゲ'
#define ZEN_GO          WCHAR(0x30B4)  // L'ゴ'
#define ZEN_ZA          WCHAR(0x30B6)  // L'ザ'
#define ZEN_JI          WCHAR(0x30B8)  // L'ジ'
#define ZEN_ZU          WCHAR(0x30BA)  // L'ズ'
#define ZEN_ZE          WCHAR(0x30BC)  // L'ゼ'
#define ZEN_ZO          WCHAR(0x30BE)  // L'ゾ'
#define ZEN_DA          WCHAR(0x30C0)  // L'ダ'
#define ZEN_DI          WCHAR(0x30C2)  // L'ヂ'
#define ZEN_DU          WCHAR(0x30C5)  // L'ヅ'
#define ZEN_DE          WCHAR(0x30C7)  // L'デ'
#define ZEN_DO          WCHAR(0x30C9)  // L'ド'
#define ZEN_BA          WCHAR(0x30D0)  // L'バ'
#define ZEN_BI          WCHAR(0x30D3)  // L'ビ'
#define ZEN_BU          WCHAR(0x30D6)  // L'ブ'
#define ZEN_BE          WCHAR(0x30D9)  // L'ベ'
#define ZEN_BO          WCHAR(0x30DC)  // L'ボ'
#define ZEN_PA          WCHAR(0x30D1)  // L'パ'
#define ZEN_PI          WCHAR(0x30D4)  // L'ピ'
#define ZEN_PU          WCHAR(0x30D7)  // L'プ'
#define ZEN_PE          WCHAR(0x30DA)  // L'ペ'
#define ZEN_PO          WCHAR(0x30DD)  // L'ポ'
#define ZEN_PROLONG     WCHAR(0x30FC)  // L'ー'
#define ZEN_ULEFT       WCHAR(0x250F)  // L'┏'
#define ZEN_URIGHT      WCHAR(0x2513)  // L'┓'
#define ZEN_LLEFT       WCHAR(0x2517)  // L'┗'
#define ZEN_LRIGHT      WCHAR(0x251B)  // L'┛'
#define ZEN_VLINE       WCHAR(0x2503)  // L'┃'
#define ZEN_HLINE       WCHAR(0x2501)  // L'━'
#define ZEN_UP          WCHAR(0x2191)  // L'↑'
#define ZEN_DOWN        WCHAR(0x2193)  // L'↓'
#define ZEN_LEFT        WCHAR(0x2190)  // L'←'
#define ZEN_RIGHT       WCHAR(0x2192)  // L'→'
#define ZEN_ASTERISK    WCHAR(0xFF0A)  // L'＊'
#define ZEN_BULLET      WCHAR(0x25CF)  // L'●'
#define ZEN_UNDERLINE   WCHAR(0xFF3F)  // L'＿'

//////////////////////////////////////////////////////////////////////////////
// ルール群。

enum RULES
{
    RULE_DONTDOUBLEBLACK = (1 << 0),    // 連黒禁。
    RULE_DONTCORNERBLACK = (1 << 1),    // 四隅黒禁。
    RULE_DONTTRIDIRECTIONS = (1 << 2),  // 三方黒禁。
    RULE_DONTDIVIDE = (1 << 3),         // 分断禁。
    RULE_DONTFOURDIAGONALS = (1 << 4),  // 黒斜四連禁。
    RULE_POINTSYMMETRY = (1 << 5),      // 黒マス点対称。
    RULE_DONTTHREEDIAGONALS = (1 << 6), // 黒斜三連禁。
    RULE_LINESYMMETRYV = (1 << 7),      // 黒マス線対称（タテ）。
    RULE_LINESYMMETRYH = (1 << 8),      // 黒マス線対称（ヨコ）。
};

// デフォルトのルール。
#define DEFAULT_RULES (RULE_DONTDIVIDE | RULE_POINTSYMMETRY)

// ルール群。
extern INT xg_nRules;

// 正当なルール。
#define VALID_RULES ( \
    RULE_DONTDOUBLEBLACK | \
    RULE_DONTCORNERBLACK | \
    RULE_DONTTRIDIRECTIONS | \
    RULE_DONTDIVIDE | \
    RULE_DONTFOURDIAGONALS | \
    RULE_POINTSYMMETRY | \
    RULE_DONTTHREEDIAGONALS | \
    RULE_LINESYMMETRYV | \
    RULE_LINESYMMETRYH)


//////////////////////////////////////////////////////////////////////////////
// マスの位置。

struct XG_Pos
{
    int m_i;    // 行のインデックス。
    int m_j;    // 列のインデックス。

    // コンストラクタ。
    XG_Pos() { }

    // コンストラクタ。
    xg_constexpr XG_Pos(int i, int j) : m_i(i), m_j(j) { }

    // コンストラクタ。
    xg_constexpr XG_Pos(const XG_Pos& pos) : m_i(pos.m_i), m_j(pos.m_j) { }

    // 比較。
    xg_constexpr bool __fastcall operator==(const XG_Pos& pos) const {
        return m_i == pos.m_i && m_j == pos.m_j;
    }

    // 比較。
    xg_constexpr bool __fastcall operator!=(const XG_Pos& pos) const {
        return m_i != pos.m_i || m_j != pos.m_j;
    }

    void clear() {
        m_i = 0; m_j = 0;
    }
};

namespace std
{
    template <>
    inline void swap(XG_Pos& pos1, XG_Pos& pos2) {
        std::swap(pos1.m_i, pos2.m_i);
        std::swap(pos1.m_j, pos2.m_j);
    }

    template <>
    struct hash<XG_Pos> {
        size_t operator()(const XG_Pos& pos) const {
            return MAKELONG(pos.m_i, pos.m_j);
        }
    };
} // namespace std

// キャレットの位置。
extern XG_Pos xg_caret_pos;

//////////////////////////////////////////////////////////////////////////////
// ヒントデータ。

struct XG_Hint
{
    int             m_number;
    std::wstring    m_strWord;
    std::wstring    m_strHint;

    // コンストラクタ。
    XG_Hint() { }

    // コンストラクタ。
    inline
    XG_Hint(int number, const std::wstring& word, const std::wstring& hint) :
        m_number(number), m_strWord(word), m_strHint(hint)
    {
    }

    // コンストラクタ。
    inline
    XG_Hint(int number, std::wstring&& word, std::wstring&& hint) :
        m_number(number),
        m_strWord(std::move(word)),
        m_strHint(std::move(hint))
    {
    }

    // コピーコンストラクタ。
    inline
    XG_Hint(const XG_Hint& info) :
        m_number(info.m_number),
        m_strWord(info.m_strWord),
        m_strHint(info.m_strHint)
    {
    }

    // コピーコンストラクタ。
    inline
    XG_Hint(XG_Hint&& info) :
        m_number(info.m_number),
        m_strWord(std::move(info.m_strWord)),
        m_strHint(std::move(info.m_strHint))
    {
    }

    inline void operator=(const XG_Hint& info)
    {
        m_number = info.m_number;
        m_strWord = info.m_strWord;
        m_strHint = info.m_strHint;
    }

    inline void operator=(XG_Hint&& info)
    {
        m_number = info.m_number;
        m_strWord = std::move(info.m_strWord);
        m_strHint = std::move(info.m_strHint);
    }
};

namespace std
{
    inline void swap(XG_Hint& hint1, XG_Hint& hint2)
    {
        std::swap(hint1.m_number, hint2.m_number);
        std::swap(hint1.m_strWord, hint2.m_strWord);
        std::swap(hint1.m_strHint, hint2.m_strHint);
    }
}

extern std::vector<XG_Hint> xg_vecTateHints, xg_vecYokoHints;

// ヒントを更新する。
void __fastcall XgUpdateHints(void);

//////////////////////////////////////////////////////////////////////////////
// クロスワード データ。

// クロスワードのサイズ。
extern volatile INT& xg_nRows;
extern volatile INT& xg_nCols;

class XG_Board
{
public:
    // コンストラクタ。
    XG_Board() noexcept;
    XG_Board(const XG_Board& xw) noexcept;
    XG_Board(XG_Board&& xw) noexcept;

    // 代入。
    void __fastcall operator=(const XG_Board& xw) noexcept;
    void __fastcall operator=(XG_Board&& xw) noexcept;

    // マスの内容を取得する。
    WCHAR __fastcall GetAt(int i) const noexcept;
    WCHAR __fastcall GetAt(int iRow, int jCol) const noexcept {
        assert(0 <= iRow && iRow < xg_nRows);
        assert(0 <= jCol && jCol < xg_nCols);
        return GetAt(iRow * xg_nCols + jCol);
    }
    WCHAR __fastcall GetAt(const XG_Pos& pos) const noexcept {
        return GetAt(pos.m_i, pos.m_j);
    }
    // マスの内容を設定する。
    void __fastcall SetAt(int i, WCHAR ch) noexcept;
    void __fastcall SetAt(int iRow, int jCol, WCHAR ch) noexcept {
        assert(0 <= iRow && iRow < xg_nRows);
        assert(0 <= jCol && jCol < xg_nCols);
        SetAt(iRow * xg_nCols + jCol, ch);
    }
    void __fastcall SetAt(const XG_Pos& pos, WCHAR ch) noexcept {
        return SetAt(pos.m_i, pos.m_j, ch);
    }
    void __fastcall SetAt2(int i, int j, int nRows, int nCols, WCHAR ch) noexcept {
        m_vCells[i * nCols + j] = ch;
    }
    // 空ではないマスの個数を返す。
    WCHAR& __fastcall Count() noexcept;
    WCHAR __fastcall Count() const noexcept;
    VOID ReCount();
    // クリアする。
    void __fastcall clear() noexcept;
    // リセットしてサイズを設定する。
    void __fastcall ResetAndSetSize(int nRows, int nCols) noexcept;
    // 解か？
    bool __fastcall IsSolution() const noexcept;
    // 正当かどうか？
    bool __fastcall IsValid() const noexcept;
    // 正当かどうか？（簡略版、黒マス追加なし）
    bool __fastcall IsNoAddBlackOK() const noexcept;
    // 番号をつける。
    bool __fastcall DoNumbering() noexcept;
    // 番号をつける（チェックなし）。
    void __fastcall DoNumberingNoCheck() noexcept;

    // クロスワードの文字列を取得する。
    void __fastcall GetString(std::wstring& str) const;
    bool __fastcall SetString(const std::wstring& strToBeSet);

    // クロスワードが空かどうか。
    bool __fastcall IsEmpty() const noexcept;
    // クロスワードがすべて埋め尽くされているかどうか。
    bool __fastcall IsFulfilled() const noexcept;
    // 四隅に黒マスがあるかどうか。
    bool __fastcall CornerBlack() const noexcept;
    // 黒マスが隣り合っているか？
    bool __fastcall DoubleBlack() const noexcept;
    // 三方向が黒マスで囲まれたマスがあるかどうか？
    bool __fastcall TriBlackAround() const noexcept;
    // 黒マスで分断されているかどうか？
    bool __fastcall DividedByBlack() const noexcept;
    // すべてのパターンが正当かどうか調べる。
    XG_EpvCode __fastcall EveryPatternValid1(
        std::vector<std::wstring>& vNotFoundWords,
        XG_Pos& pos, bool bNonBlackCheckSpace) const noexcept;
    // すべてのパターンが正当かどうか調べる。
    XG_EpvCode __fastcall EveryPatternValid2(
        std::vector<std::wstring>& vNotFoundWords,
        XG_Pos& pos, bool bNonBlackCheckSpace) const noexcept;

    // 黒マスを置けるか？
    bool __fastcall CanPutBlack(int iRow, int jCol) const noexcept;

    // マスの三方向が黒マスで囲まれているか？
    int __fastcall BlacksAround(int iRow, int jCol) const noexcept;

    // 黒マスが点対称か？
    bool IsPointSymmetry() const noexcept;
    // 黒マスが線対称か？
    bool IsLineSymmetry() const noexcept;
    // 黒マスが線対称（タテ）か？
    bool IsLineSymmetryV() const noexcept;
    // 黒マスが線対称（ヨコ）か？
    bool IsLineSymmetryH() const noexcept;
    // 必要ならルールに従って対称にする。
    void Mirror();

    // 黒斜三連か？
    bool ThreeDiagonals() const noexcept;
    // 黒斜四連か？
    bool FourDiagonals() const noexcept;

    // 縦と横を入れ替える。
    void SwapXandY() noexcept;

    // タテ向きにパターンを読み取る。
    std::wstring __fastcall GetPatternV(const XG_Pos& pos) const noexcept;
    // ヨコ向きにパターンを読み取る。
    std::wstring __fastcall GetPatternH(const XG_Pos& pos) const noexcept;

public:
    // マス情報。
    std::vector<WCHAR> m_vCells;
};

// 盤にサイズ情報を追加。
class XG_BoardEx : public XG_Board
{
public:
    volatile INT m_nRows;
    volatile INT m_nCols;

    XG_BoardEx() : XG_Board(), m_nRows(7), m_nCols(7) {
    }
    XG_BoardEx(const XG_Board& src) {
        m_vCells = src.m_vCells;
    }
    XG_BoardEx(const XG_BoardEx& src) {
        m_vCells = src.m_vCells;
        m_nRows = src.m_nRows;
        m_nCols = src.m_nCols;
    }

    XG_BoardEx& operator=(const XG_Board& src) {
        m_vCells = src.m_vCells;
        return *this;
    }
    XG_BoardEx& operator=(const XG_BoardEx& src) {
        m_vCells = src.m_vCells;
        m_nRows = src.m_nRows;
        m_nCols = src.m_nCols;
        return *this;
    }

    // カウントを更新する。
    void ReCount();

    // 行を挿入する。
    void InsertRow(INT iRow);
    // 列を挿入する。
    void InsertColumn(INT jCol);
    // 行を削除する。
    void DeleteRow(INT iRow);
    // 列を削除する。
    void DeleteColumn(INT jCol);
    // マスの内容を取得する。
    WCHAR __fastcall GetAt(int ij) const noexcept;
    // マスの内容を取得する。
    WCHAR __fastcall GetAt(int iRow, int jCol) const noexcept {
        assert(0 <= iRow && iRow < m_nRows);
        assert(0 <= jCol && jCol < m_nCols);
        return GetAt(iRow * m_nCols + jCol);
    }
    // マスの内容を取得する。
    WCHAR __fastcall GetAt(const XG_Pos& pos) const noexcept {
        return GetAt(pos.m_i, pos.m_j);
    }
    // マスの内容を設定する。
    void __fastcall SetAt(int ij, WCHAR ch) noexcept;
    // マスの内容を設定する。
    void __fastcall SetAt(int iRow, int jCol, WCHAR ch) noexcept {
        assert(0 <= iRow && iRow < m_nRows);
        assert(0 <= jCol && jCol < m_nCols);
        SetAt(iRow * m_nCols + jCol, ch);
    }
    // マスの内容を設定する。
    void __fastcall SetAt(const XG_Pos& pos, WCHAR ch) noexcept {
        SetAt(pos.m_i, pos.m_j, ch);
    }
};

bool XgDoSaveStandard(HWND hwnd, LPCWSTR pszFile, const XG_Board& board);
// ファイル（XD形式）を保存する。
bool __fastcall XgDoSaveXdFile(LPCWSTR pszFile);

namespace std
{
    inline void swap(XG_Board& xw1, XG_Board& xw2) {
        std::swap(xw1.m_vCells, xw2.m_vCells);
    }
}

inline bool operator==(const XG_Board& xw1, const XG_Board& xw2) {
    for (int i = 0; i < xg_nRows; ++i) {
        for (int j = 0; j < xg_nCols; ++j) {
            if (xw1.GetAt(i, j) != xw2.GetAt(i, j))
                return false;
        }
    }
    return true;
}

inline bool operator!=(const XG_Board& xw1, const XG_Board& xw2) {
    return !(xw1 == xw2);
}

// クロスワードの描画サイズを計算する。
inline void __fastcall XgGetXWordExtent(LPSIZE psiz)
{
    INT nCellSize;
    if (xg_nForDisplay > 0)
        nCellSize = xg_nCellSize * xg_nZoomRate / 100;
    else
        nCellSize = xg_nCellSize;
    psiz->cx = static_cast<int>(xg_nMargin * 2 + xg_nCols * nCellSize);
    psiz->cy = static_cast<int>(xg_nMargin * 2 + xg_nRows * nCellSize);
}

// クロスワードの描画サイズを計算する。
inline void __fastcall XgGetMarkWordExtent(int count, LPSIZE psiz)
{
    INT nCellSize;
    if (xg_nForDisplay > 0)
        nCellSize = xg_nCellSize * xg_nZoomRate / 100;
    else
        nCellSize = xg_nCellSize;
    psiz->cx = static_cast<int>(xg_nNarrowMargin * 2 + count * nCellSize);
    psiz->cy = static_cast<int>(xg_nNarrowMargin * 2 + 1 * nCellSize);
}

// クロスワードのイメージを作成する。
HBITMAP __fastcall XgCreateXWordImage(XG_Board& xw, LPSIZE psiz, bool bCaret);

// 二重マス単語を描画する。
void __fastcall XgDrawMarkWord(HDC hdc, LPSIZE psiz);

// クロスワードを描画する。
void __fastcall XgDrawXWord(XG_Board& xw, HDC hdc, LPSIZE psiz, bool bCaret);

// 解を求めるのを開始。
void __fastcall XgStartSolve_AddBlack(void) noexcept;

// 解を求めるのを開始（黒マス追加なし）。
void __fastcall XgStartSolve_NoAddBlack(void) noexcept;

// 解を求めるのを開始（スマート解決）。
void __fastcall XgStartSolve_Smart(void) noexcept;

// 解を求めようとした後の後処理。
void __fastcall XgEndSolve(void) noexcept;

// 黒マスパターンを生成する。
void __fastcall XgStartGenerateBlacks(void) noexcept;

// ステータスバーを更新する。
void __fastcall XgUpdateStatusBar(HWND hwnd);

// 黒マスルールをチェックする。
void __fastcall XgRuleCheck(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////

// クロスワードの問題。
extern XG_BoardEx       xg_xword;

// クロスワードの解。
extern XG_Board         xg_solution;

// クロスワードの解があるかどうか？
extern bool             xg_bSolved;

// ヘッダー文字列。
extern std::wstring     xg_strHeader;
// 備考文字列。
extern std::wstring     xg_strNotes;

// 黒マス画像。
extern HBITMAP xg_hbmBlackCell;
extern HENHMETAFILE xg_hBlackCellEMF;
extern std::wstring xg_strBlackCellImage;

// ビューモード。
typedef enum XG_VIEW_MODE
{
    XG_VIEW_NORMAL, // 通常ビュー。
    XG_VIEW_SKELETON // スケルトンビュー。
} XG_VIEW_MODE;
extern XG_VIEW_MODE xg_nViewMode;

//////////////////////////////////////////////////////////////////////////////
// inline functions

// コンストラクタ。
inline XG_Board::XG_Board() noexcept
{
}

// コピーコンストラクタ。
inline XG_Board::XG_Board(const XG_Board& xw) noexcept : m_vCells(xw.m_vCells)
{
}

// 代入。
inline void __fastcall XG_Board::operator=(const XG_Board& xw) noexcept
{
    m_vCells = xw.m_vCells;
}

// コンストラクタ。
inline XG_Board::XG_Board(XG_Board&& xw) noexcept : m_vCells(xw.m_vCells)
{
}

// 代入。
inline void __fastcall XG_Board::operator=(XG_Board&& xw) noexcept
{
    m_vCells = std::move(xw.m_vCells);
}

// マスの内容を取得する。
inline WCHAR __fastcall XG_Board::GetAt(int i) const noexcept
{
    assert(0 <= i && i < xg_nRows * xg_nCols);
    return m_vCells[i];
}

// マスの内容を取得する。
inline WCHAR __fastcall XG_BoardEx::GetAt(int ij) const noexcept {
    return m_vCells[ij];
}

// 空マスじゃないマスの個数を返す。
inline WCHAR& __fastcall XG_Board::Count() noexcept
{
    return m_vCells[xg_nRows * xg_nCols];
}

// 空マスじゃないマスの個数を返す。
inline WCHAR __fastcall XG_Board::Count() const noexcept
{
    return m_vCells[xg_nRows * xg_nCols];
}

// クロスワードが空かどうか。
inline bool __fastcall XG_Board::IsEmpty() const noexcept
{
#if 1
    return Count() == 0;
#else
    for (int i = 0; i < xg_nRows; i++)
    {
        for (int j = 0; j < xg_nCols; j++)
        {
            if (GetAt(i, j) != ZEN_SPACE)
                return false;
        }
    }
    return true;
#endif
}

// クロスワードがすべて埋め尽くされているかどうか。
inline bool __fastcall XG_Board::IsFulfilled() const noexcept
{
#if 1
    return Count() == xg_nRows * xg_nCols;
#else
    for (int i = 0; i < xg_nRows; i++)
    {
        for (int j = 0; j < xg_nCols; j++)
        {
            if (GetAt(i, j) == ZEN_SPACE)
                return false;
        }
    }
    return true;
#endif
}

// リセットしてサイズを設定する。
inline void __fastcall XG_Board::ResetAndSetSize(int nRows, int nCols) noexcept
{
    m_vCells.assign(nRows * nCols + 1, ZEN_SPACE);
    m_vCells[nRows * nCols] = 0;
}

// クリアする。
inline void __fastcall XG_Board::clear() noexcept
{
    ResetAndSetSize(xg_nRows, xg_nCols);
}

// マスの内容を設定する。
inline void __fastcall XG_Board::SetAt(int i, WCHAR ch) noexcept
{
    assert(0 <= i && i < xg_nRows * xg_nCols);
    WCHAR& ch2 = m_vCells[i];
    if (ch2 != ZEN_SPACE)
    {
        if (ch == ZEN_SPACE)
        {
            assert(Count() != 0);
            Count()--;
        }
    }
    else
    {
        if (ch != ZEN_SPACE)
            Count()++;
    }
    ch2 = ch;
}

// マスの内容を設定する。
inline void __fastcall XG_BoardEx::SetAt(int ij, WCHAR ch) noexcept
{
    WCHAR& ch2 = m_vCells[ij];
    if (ch2 != ZEN_SPACE)
    {
        if (ch == ZEN_SPACE)
        {
            assert(Count() != 0);
            Count()--;
        }
    }
    else
    {
        if (ch != ZEN_SPACE)
            Count()++;
    }
    ch2 = ch;
}

// マスの三方向が黒マスで囲まれているか？
inline int __fastcall XG_Board::BlacksAround(int iRow, int jCol) const noexcept
{
    assert(0 <= iRow && iRow < xg_nRows);
    assert(0 <= jCol && jCol < xg_nCols);

    int count = 0;
    if (0 <= iRow - 1) {
        if (GetAt(iRow - 1, jCol) == ZEN_BLACK)
            ++count;
    }
    if (iRow + 1 < xg_nRows) {
        if (GetAt(iRow + 1, jCol) == ZEN_BLACK)
            ++count;
    }
    if (0 <= jCol - 1) {
        if (GetAt(iRow, jCol - 1) == ZEN_BLACK)
            ++count;
    }
    if (jCol + 1 < xg_nCols) {
        if (GetAt(iRow, jCol + 1) == ZEN_BLACK)
            ++count;
    }
    return count;
}

// 黒マスを置けるか？
inline bool __fastcall XG_Board::CanPutBlack(int iRow, int jCol) const noexcept
{
    assert(0 <= iRow && iRow < xg_nRows);
    assert(0 <= jCol && jCol < xg_nCols);

    // 四隅かどうか？
    if (iRow == 0 && jCol == 0)
        return false;
    if (iRow == xg_nRows - 1 && jCol == 0)
        return false;
    if (iRow == xg_nRows - 1 && jCol == xg_nCols - 1)
        return false;
    if (iRow == 0 && jCol == xg_nCols - 1)
        return false;

    // 黒マスが隣り合ってしまうか？
    if (0 <= iRow - 1 && GetAt(iRow - 1, jCol) == ZEN_BLACK)
        return false;
    if (iRow + 1 < xg_nRows && GetAt(iRow + 1, jCol) == ZEN_BLACK)
        return false;
    if (0 <= jCol - 1 && GetAt(iRow, jCol - 1) == ZEN_BLACK)
        return false;
    if (jCol + 1 < xg_nCols && GetAt(iRow, jCol + 1) == ZEN_BLACK)
        return false;

    // 三方向が黒マスで囲まれたマスができるかどうか？
    BOOL bBlack = (GetAt(iRow, jCol) == ZEN_BLACK);
    if (0 <= iRow - 1) {
        if (BlacksAround(iRow - 1, jCol) >= 2 + bBlack)
            return false;
    }
    if (iRow + 1 < xg_nRows) {
        if (BlacksAround(iRow + 1, jCol) >= 2 + bBlack)
            return false;
    }
    if (0 <= jCol - 1) {
        if (BlacksAround(iRow, jCol - 1) >= 2 + bBlack)
            return false;
    }
    if (jCol + 1 < xg_nCols) {
        if (BlacksAround(iRow, jCol + 1) >= 2 + bBlack)
            return false;
    }

    return true;
}

// ユーザは日本人か？
inline BOOL XgIsUserJapanese(VOID)
{
    static BOOL s_bInit = FALSE, s_bIsJapanese = FALSE; // 高速化のためキャッシュを使う。
    if (!s_bInit) {
        // IDS_MAIN_LANGUAGEの値が"Japanese"だったら日本語と見なす。
        WCHAR szText[64];
        LoadStringW(NULL, IDS_MAIN_LANGUAGE, szText, _countof(szText));
        s_bIsJapanese = (lstrcmpiW(szText, L"Japanese") == 0);
        s_bInit = TRUE;
    }
    return s_bIsJapanese;
    // return PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_JAPANESE; // これはだめ。
}

// ユーザは東アジア人（中国、日本、韓国）か？
inline BOOL XgIsUserCJK(VOID)
{
    LCID lcid = GetUserDefaultLCID();
    WORD langid = LANGIDFROMLCID(lcid);
    switch (PRIMARYLANGID(langid)) {
    case LANG_CHINESE:
    case LANG_JAPANESE:
    case LANG_KOREAN:
        return TRUE;
    }
    return FALSE;
}

// 番号を表示するか？
extern BOOL xg_bShowNumbering;
// キャレットを表示するか？
extern BOOL xg_bShowCaret;
// 二重マス文字。
extern std::wstring xg_strDoubleFrameLetters;
// 二重マス文字を表示するか？
extern BOOL xg_bShowDoubleFrameLetters;

// std::random_shuffleの代わり。
#include <random>
template <typename t_elem>
inline void xg_random_shuffle(const t_elem& begin, const t_elem& end) {
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(begin, end, g);
}

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
    xg_im_DIGITS,   // 数字入力
    xg_im_GREEK,    // ギリシャ文字。
    xg_im_ANY,      // 自由入力。
};
extern XG_InputMode xg_imode;

void __fastcall XgSetInputModeFromDict(HWND hwnd);

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

// 全角数字か？
inline bool XgIsCharZenkakuNumericW(WCHAR ch)
{
    return 0xFF10 <= ch && ch <= 0xFF19;
}
// 半角数字か？
inline bool XgIsCharHankakuNumericW(WCHAR ch)
{
    return L'0' <= ch && ch <= L'9';
}

// ギリシャ文字か？
inline bool XgIsCharGreekW(WCHAR ch)
{
    return 0x0370 <= ch && ch <= 0x03FF;
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

#include "Utils.hpp"

//////////////////////////////////////////////////////////////////////////////

// 改行。
extern const LPCWSTR xg_pszNewLine;

// インスタンスのハンドル。
extern HINSTANCE xg_hInstance;

// メインウィンドウのハンドル。
extern HWND xg_hMainWnd;

// ツールバーのハンドル。
extern HWND xg_hToolBar;

// ステータスバーのハンドル。
extern HWND xg_hStatusBar;

// ヒントウィンドウのハンドル。
extern HWND xg_hHintsWnd;

// 答えを表示するか？
extern bool xg_bShowAnswer;

// 黒マス追加なしか？
extern bool xg_bNoAddBlack;

// 空のクロスワードの解を解く場合か？
extern bool xg_bSolvingEmpty;

// 黒マスパターンが生成されたか？
extern bool xg_bBlacksGenerated;

// スマート解決か？
extern bool xg_bSmartResolution;

// 太枠をつけるか？
extern bool xg_bAddThickFrame;

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
extern HBITMAP xg_hbmImage;

// ヒント文字列。
extern std::wstring xg_strHints;

// スレッドのハンドル。
extern std::vector<HANDLE> xg_ahThreads;

// マスのフォント。
extern WCHAR xg_szCellFont[];

// 番号のフォント。
extern WCHAR xg_szSmallFont[];

// 直前に押したキーを覚えておく。
extern WCHAR xg_prev_vk;

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

//////////////////////////////////////////////////////////////////////////////

// 再計算するか？
extern bool xg_bAutoRetry;

// 排他制御のためのクリティカルセクション。
extern CRITICAL_SECTION xg_cs;

// スレッドの数。
extern DWORD xg_dwThreadCount;

// 計算がキャンセルされたか？
extern bool xg_bCancelled;

// スレッドの数。
extern DWORD xg_dwThreadCount;

// 連続生成の場合、問題を生成した数。
extern INT xg_nNumberGenerated;

// プログレスバーの更新頻度。
#define xg_dwTimerInterval 300

// スレッドを閉じる。
void __fastcall XgCloseThreads(void) noexcept;
// スレッドを待つ。
void __fastcall XgWaitForThreads(void) noexcept;
// スレッドが終了したか？
bool __fastcall XgIsAnyThreadTerminated(void) noexcept;

// 再計算までの時間を概算する。
inline DWORD XgGetRetryInterval(void)
{
    return 8 * (xg_nRows + xg_nCols) * (xg_nRows + xg_nCols) + 1000;
}

//////////////////////////////////////////////////////////////////////////////

// ファイルの種類。
typedef enum XG_FILETYPE {
    XG_FILETYPE_XWD = 0, // XWordGiver 形式。
    XG_FILETYPE_XWJ, // XWordGiver JSON 形式。
    XG_FILETYPE_CRP, // Crossword Builder 形式。
    XG_FILETYPE_XD, // XD形式。
} XG_FILETYPE;

// メインファイルと関連ファイルを読み込む。
bool __fastcall XgDoLoadFiles(HWND hwnd, LPCWSTR pszFile);
// メインファイルを読み込む。
bool __fastcall XgDoLoadMainFile(HWND hwnd, LPCWSTR pszFile);
// メインファイルを読み込む。
bool __fastcall XgDoLoadFileType(HWND hwnd, LPCWSTR pszFile, XG_FILETYPE type);
// CRPファイルを開く。
bool __fastcall XgDoLoadCrpFile(HWND hwnd, LPCWSTR pszFile);

// ファイルを保存する。
bool __fastcall XgDoSaveFile(HWND hwnd, LPCWSTR pszFile);
// ファイルを保存する。
bool __fastcall XgDoSaveFileType(HWND hwnd, LPCWSTR pszFile, XG_FILETYPE type);

// ヒント文字列を解析する。
bool __fastcall XgParseHintsStr(const std::wstring& strHints);

// 問題を画像ファイルとして保存する。
void __fastcall XgSaveProbAsImage(HWND hwnd);

// 解答を画像ファイルとして保存する。
void __fastcall XgSaveAnsAsImage(HWND hwnd);

template <bool t_alternative>
bool __fastcall XgGetCandidatesAddBlack(
    std::vector<std::wstring>& cands, const std::wstring& pattern, int& nSkip,
    bool left_black_check, bool right_black_check) noexcept;

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacks(void *param) noexcept;
// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksSmart(void *param) noexcept;

// ヒント文字列を取得する。
// hint_type 0: タテ。
// hint_type 1: ヨコ。
// hint_type 2: タテとヨコ。
// hint_type 3: HTMLのタテ。
// hint_type 4: HTMLのヨコ。
// hint_type 5: HTMLのタテとヨコ。
void __fastcall XgGetHintsStr(const XG_Board& board, std::wstring& str, int hint_type,
                              bool bShowAnswer = true);

// 文字マスをクリア。
void __fastcall XgClearNonBlocks(void);

// ハイライト情報。
struct XG_HighLight
{
    INT m_number;
    BOOL m_vertical;
};
extern XG_HighLight xg_highlight;

// キャレット位置をセットする。
void __fastcall XgSetCaretPos(INT iRow = 0, INT jCol = 0);
// キャレット位置を更新する。
void __fastcall XgUpdateCaretPos(void);
// ボックスをすべて削除する。
void XgDeleteBoxes(void);
// ボックスJSONを読み込む。
BOOL XgDoLoadBoxJson(const json& boxes);
// ボックスJSONを保存。
BOOL XgDoSaveBoxJson(json& j);
// ボックスを描画する。
void XgDrawBoxes(XG_Board& xw, HDC hdc, LPSIZE psiz);
// ボックスJSONを読み込む。
BOOL XgLoadXdBox(const std::wstring& line);
// XDファイルからボックスを読み込む。
BOOL XgWriteXdBoxes(FILE *fout);
// ファイル変更フラグ。
void XgSetModified(BOOL bModified, LPCSTR file, INT line);
#define XG_FILE_MODIFIED(bModified) XgSetModified((bModified), __FILE__, __LINE__)
// 保存を確認し、必要なら保存する。
BOOL XgDoConfirmSave(HWND hwnd);
// ファイルを読み込む。
BOOL XgOnLoad(HWND hwnd, LPCWSTR pszFile, LPPOINT ppt = NULL);

//////////////////////////////////////////////////////////////////////////////

#include "Dictionary.hpp"
#include "Marks.hpp"
#include "SaveBitmapToFile.h"

#endif  // ndef XWORDGIVER
