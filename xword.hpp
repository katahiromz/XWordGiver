//////////////////////////////////////////////////////////////////////////////
// xword.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#ifndef __XWORD_HPP__
#define __XWORD_HPP__

//////////////////////////////////////////////////////////////////////////////
// クロスワード。

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
#define DEF_SMALL_CHAR_SIZE 21

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

// タテ入力？
extern bool xg_bTateInput;

// 全角文字。
#define ZEN_SPACE       WCHAR(0x3000)  // L'　'
#define ZEN_BLACK       WCHAR(0x25A0)  // L'■'
#define ZEN_LARGE_A     WCHAR(0xFF21)  // L'Ａ'
#define ZEN_SMALL_A     WCHAR(0xFF41)  // L'ａ'
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
    RULE_POINTSYMMETRY = (1 << 5)       // 黒マス点対称。
};

// デフォルトのルール。
#define DEFAULT_RULES (RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | \
                       RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE)

// ルール群。
extern INT xg_nRules;

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
}

struct XG_PosHash
{
    size_t operator()(const XG_Pos& pos) const {
        return static_cast<size_t>((pos.m_i << 16) | pos.m_j);
    }
};

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

    inline
    void operator=(const XG_Hint& info)
    {
        m_number = info.m_number;
        m_strWord = info.m_strWord;
        m_strHint = info.m_strHint;
    }

    inline
    void operator=(XG_Hint&& info)
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

//////////////////////////////////////////////////////////////////////////////
// クロスワード データ。

// クロスワードのサイズ。
extern int xg_nRows, xg_nCols;

class XG_Board
{
public:
    // コンストラクタ。
    XG_Board();
    XG_Board(const XG_Board& xw);
    XG_Board(XG_Board&& xw);

    // 代入。
    void __fastcall operator=(const XG_Board& xw);
    void __fastcall operator=(XG_Board&& xw);

    // マスの内容を取得する。
    WCHAR __fastcall GetAt(int i) const;
    WCHAR __fastcall GetAt(int iRow, int jCol) const;
    WCHAR __fastcall GetAt(const XG_Pos& pos) const;
    // マスの内容を設定する。
    void __fastcall SetAt(int i, WCHAR ch);
    void __fastcall SetAt(int iRow, int jCol, WCHAR ch);
    void __fastcall SetAt(const XG_Pos& pos, WCHAR ch);
    // 空ではないマスの個数を返す。
    WCHAR& __fastcall Count();
    WCHAR __fastcall Count() const;
    // クリアする。
    void __fastcall clear();
    // リセットしてサイズを設定する。
    void __fastcall ResetAndSetSize(int nRows, int nCols);
    // 解か？
    bool __fastcall IsSolution() const;
    // 正当などうか？
    bool __fastcall IsValid() const;
    // 正当などうか？（簡略版）
    bool __fastcall IsOK() const;
    // 正当などうか？（簡略版、黒マス追加なし）
    bool __fastcall IsNoAddBlackOK() const;
    // 番号をつける。
    bool __fastcall DoNumbering();
    // 番号をつける（チェックなし）。
    void __fastcall DoNumberingNoCheck();

    // クロスワードの文字列を取得する。
    void __fastcall GetString(std::wstring& str) const;
    bool __fastcall SetString(const std::wstring& strToBeSet);

    // ヒント文字列を取得する。
    // hint_type 0: タテ。
    // hint_type 1: ヨコ。
    // hint_type 2: タテとヨコ。
    // hint_type 3: HTMLのタテ。
    // hint_type 4: HTMLのヨコ。
    // hint_type 5: HTMLのタテとヨコ。
    void __fastcall GetHintsStr(
        std::wstring& str, int hint_type, bool bShowAnswer = true) const;

    // クロスワードが空かどうか。
    bool __fastcall IsEmpty() const;
    // クロスワードがすべて埋め尽くされているかどうか。
    bool __fastcall IsFulfilled() const;
    // 四隅に黒マスがあるかどうか。
    bool __fastcall CornerBlack() const;
    // 黒マスが隣り合っているか？
    bool __fastcall DoubleBlack() const;
    // 三方向が黒マスで囲まれたマスがあるかどうか？
    bool __fastcall TriBlackAround() const;
    // 黒マスで分断されているかどうか？
    bool __fastcall DividedByBlack() const;
    // すべてのパターンが正当かどうか調べる。
    XG_EpvCode __fastcall EveryPatternValid1(
        std::vector<std::wstring>& vNotFoundWords,
        XG_Pos& pos, bool bNonBlackCheckSpace) const;
    // すべてのパターンが正当かどうか調べる。
    XG_EpvCode __fastcall EveryPatternValid2(
        std::vector<std::wstring>& vNotFoundWords,
        XG_Pos& pos, bool bNonBlackCheckSpace) const;

    // 黒マスを置けるか？
    bool __fastcall CanPutBlack(int iRow, int jCol) const;

    // マスの三方向が黒マスで囲まれているか？
    int __fastcall BlacksAround(int iRow, int jCol) const;

    // 黒マスが線対称か？
    bool IsLineSymmetry() const;

    // 黒マスが点対称か？
    bool IsPointSymmetry() const;

    // 黒斜四連か？
    bool FourDiagonals() const;

    // 縦と横を入れ替える。
    void SwapXandY();

    // タテ向きにパターンを読み取る。
    std::wstring __fastcall GetPatternV(const XG_Pos& pos) const;
    // ヨコ向きにパターンを読み取る。
    std::wstring __fastcall GetPatternH(const XG_Pos& pos) const;

public:
    // マス情報。
    std::vector<WCHAR> m_vCells;
};

bool __fastcall XgDoSaveStandard(HWND hwnd, LPCWSTR pszFile, const XG_Board& board);

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
void __fastcall XgStartSolve_AddBlack(void);

// 解を求めるのを開始（黒マス追加なし）。
void __fastcall XgStartSolve_NoAddBlack(void);

// 解を求めるのを開始（スマート解決）。
void __fastcall XgStartSolveSmart(void);

// 解を求めようとした後の後処理。
void __fastcall XgEndSolve(void);

// 黒マスパターンを生成する。
void __fastcall XgStartGenerateBlacks(bool sym);

// ステータスバーを更新する。
void __fastcall XgUpdateStatusBar(HWND hwnd);

// 黒マスルールをチェックする。
void __fastcall XgRuleCheck(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////

// クロスワードの問題。
extern XG_Board         xg_xword;

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

//////////////////////////////////////////////////////////////////////////////
// inline functions

// コンストラクタ。
inline XG_Board::XG_Board()
{
}

// コピーコンストラクタ。
inline XG_Board::XG_Board(const XG_Board& xw) : m_vCells(xw.m_vCells)
{
}

// 代入。
inline void __fastcall XG_Board::operator=(const XG_Board& xw)
{
    m_vCells = xw.m_vCells;
}

// コンストラクタ。
inline XG_Board::XG_Board(XG_Board&& xw) : m_vCells(xw.m_vCells)
{
}

// 代入。
inline void __fastcall XG_Board::operator=(XG_Board&& xw)
{
    m_vCells = std::move(xw.m_vCells);
}

// マスの内容を取得する。
inline WCHAR __fastcall XG_Board::GetAt(int i) const
{
    assert(0 <= i && i < xg_nRows * xg_nCols);
    return m_vCells[i];
}

// マスの内容を取得する。
inline WCHAR __fastcall XG_Board::GetAt(int iRow, int jCol) const
{
    assert(0 <= iRow && iRow < xg_nRows);
    assert(0 <= jCol && jCol < xg_nCols);
    return m_vCells[iRow * xg_nCols + jCol];
}

// マスの内容を取得する。
inline WCHAR __fastcall XG_Board::GetAt(const XG_Pos& pos) const
{
    return GetAt(pos.m_i, pos.m_j);
}

// 空マスじゃないマスの個数を返す。
inline WCHAR& __fastcall XG_Board::Count()
{
    return m_vCells[xg_nRows * xg_nCols];
}

// 空マスじゃないマスの個数を返す。
inline WCHAR __fastcall XG_Board::Count() const
{
    return m_vCells[xg_nRows * xg_nCols];
}

// クロスワードが空かどうか。
inline bool __fastcall XG_Board::IsEmpty() const
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
inline bool __fastcall XG_Board::IsFulfilled() const
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
inline void __fastcall XG_Board::ResetAndSetSize(int nRows, int nCols)
{
    m_vCells.assign(nRows * nCols + 1, ZEN_SPACE);
    m_vCells[nRows * nCols] = 0;
}

// クリアする。
inline void __fastcall XG_Board::clear()
{
    ResetAndSetSize(xg_nRows, xg_nCols);
}

// マスの内容を設定する。
inline void __fastcall XG_Board::SetAt(int i, WCHAR ch)
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
inline void __fastcall XG_Board::SetAt(int iRow, int jCol, WCHAR ch)
{
    assert(0 <= iRow && iRow < xg_nRows);
    assert(0 <= jCol && jCol < xg_nCols);
    WCHAR& ch2 = m_vCells[iRow * xg_nCols + jCol];
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
inline void __fastcall XG_Board::SetAt(const XG_Pos& pos, WCHAR ch)
{
    return SetAt(pos.m_i, pos.m_j, ch);
}

// マスの三方向が黒マスで囲まれているか？
inline int __fastcall XG_Board::BlacksAround(int iRow, int jCol) const
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
inline bool __fastcall XG_Board::CanPutBlack(int iRow, int jCol) const
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

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef __XWORD_HPP__
