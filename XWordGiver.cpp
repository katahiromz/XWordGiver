//////////////////////////////////////////////////////////////////////////////
// XWordGiver.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#include "XWordGiver.hpp"
#include "Auto.hpp"

//////////////////////////////////////////////////////////////////////////////
// global variables

// 直前に開いたクロスワードデータファイルのパスファイル名。
std::wstring xg_strFileName;

// 空のクロスワードの解を解く場合か？
bool xg_bSolvingEmpty = false;

// 計算がキャンセルされたか？
bool xg_bCancelled = false;

// クロスワードの解があるかどうか？
bool xg_bSolved = false;

// 答えを表示するか？
bool xg_bShowAnswer = false;

// 黒マス追加なしか？
bool xg_bNoAddBlack = false;

// スマート解決か？
bool xg_bSmartResolution = true;

// スレッドの数。
DWORD xg_dwThreadCount;

// スレッド情報。
std::vector<XG_ThreadInfo>    xg_aThreadInfo;

// スレッドのハンドル。
std::vector<HANDLE>           xg_ahThreads;

// ヒント文字列。
std::wstring             xg_strHints;

// ヘッダー文字列。
std::wstring             xg_strHeader;
// 備考文字列。
std::wstring             xg_strNotes;

// 排他制御のためのクリティカルセクション。
CRITICAL_SECTION    xg_cs;

// 改行。
const LPCWSTR       xg_pszNewLine = L"\r\n";

// キャレットの位置。
XG_Pos              xg_caret_pos = {0, 0};

// クロスワードの問題。
XG_BoardEx xg_xword;
// クロスワードの解。
XG_Board xg_solution;

// クロスワードのサイズ。
volatile INT& xg_nRows = xg_xword.m_nRows;
volatile INT& xg_nCols = xg_xword.m_nCols;

// タテとヨコのかぎ。
std::vector<XG_PlaceInfo> xg_vTateInfo, xg_vYokoInfo;

// 拗音変換用データ。
const LPCWSTR xg_small[11] = 
{
    L"\x30A1", L"\x30A3", L"\x30A5", L"\x30A7", L"\x30A9", L"\x30C3",
    L"\x30E3", L"\x30E5", L"\x30E7", L"\x30F5", L"\x30F6"
};
const LPCWSTR xg_large[11] = 
{
    L"\x30A2", L"\x30A4", L"\x30A6", L"\x30A8", L"\x30AA", L"\x30C4",
    L"\x30E4", L"\x30E6", L"\x30E8", L"\x30AB", L"\x30B1",
};

// ビットマップのハンドル。
HBITMAP     xg_hbmImage = nullptr;

// ヒントデータ。
std::vector<XG_Hint> xg_vecTateHints, xg_vecYokoHints;

// セルの色。
COLORREF xg_rgbWhiteCellColor = RGB(255, 255, 255);
COLORREF xg_rgbBlackCellColor = RGB(0x33, 0x33, 0x33);
COLORREF xg_rgbMarkedCellColor = RGB(255, 255, 255);

// 二重マスに枠を描くか？
bool xg_bDrawFrameForMarkedCell = true;

// 文字送り？
bool xg_bCharFeed = true;

// タテ入力？
bool xg_bTateInput = false;

// 文字の大きさ（％）。
INT xg_nCellCharPercents = DEF_SMALL_CHAR_SIZE;

// 小さい文字の大きさ（％）。
INT xg_nSmallCharPercents = DEF_SMALL_CHAR_SIZE;

// 黒マス画像。
HBITMAP xg_hbmBlackCell = NULL;
HENHMETAFILE xg_hBlackCellEMF = NULL;
std::wstring xg_strBlackCellImage;

// ビューモード。
XG_VIEW_MODE xg_nViewMode = XG_VIEW_NORMAL;

//////////////////////////////////////////////////////////////////////////////
// static variables

// 縦と横を反転しているか？
static bool s_bSwapped = false;

//////////////////////////////////////////////////////////////////////////////

// 候補があるか？
template <bool t_alternative>
bool __fastcall XgAnyCandidateAddBlack(const std::wstring& pattern)
{
    // パターンの長さ。
    const int patlen = static_cast<int>(pattern.size());
    assert(patlen >= 2);

#ifndef NDEBUG
    // パターンに黒マスがないことを仮定する。
    // パターンに空白があることを仮定する。
    bool bSpaceFound = false;
    for (int i = 0; i < patlen; i++) {
        assert(pattern[i] != ZEN_BLACK);
        if (pattern[i] != ZEN_SPACE)
            bSpaceFound = true;
    }
    assert(bSpaceFound);
#endif

    // パターンが３字以上か？
    if (patlen > 2) {
        // 孤立した文字マスが左端か？
        if (pattern[0] != ZEN_SPACE && pattern[1] == ZEN_SPACE) {
            return true;    // 孤立した文字マスがあった。
        } else {
            bool bCharNotFound = true;  // 文字マスがなかったか？

            // 孤立した文字マスが中にあるかを調べる。
            // ついでに文字マスが途中にあるか調べる。
            for (int j = 1; j < patlen - 1; j++) {
                if (pattern[j] != ZEN_SPACE) {
                    if (pattern[j - 1] == ZEN_SPACE && pattern[j + 1] == ZEN_SPACE) {
                        return true;    // 孤立した文字マスがあった。
                    }
                    bCharNotFound = false;
                    break;
                }
            }

            if (bCharNotFound) {
                // 孤立した文字マスが右端か？
                if (pattern[patlen - 1] != ZEN_SPACE && pattern[patlen - 2] == ZEN_SPACE) {
                    return true;    // 孤立した文字マスがあった。
                }
            }
        }
    }

    // すべての単語について調べる。
    for (const auto& data : (t_alternative ? xg_dict_2 : xg_dict_1)) {
        const std::wstring& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());

        // パターンより単語の方が長い場合、スキップする。
        if (wordlen > patlen)
            continue;

        // 単語の置ける区間について調べる。
        const int patlen_minus_wordlen = patlen - wordlen;
        for (int j = 0; j <= patlen_minus_wordlen; j++) {
            // もし単語の位置の前後に文字があったらスキップする。
            if (j > 0 && pattern[j - 1] != ZEN_SPACE)
                continue;
            if (j < patlen_minus_wordlen && pattern[j + wordlen] != ZEN_SPACE)
                continue;

            // 区間[j, j + wordlen - 1]に文字マスがあるか？
            // 単語がにマッチするか？
            bool bCharFound = false;
            for (int k = 0; k < wordlen; k++) {
                if (pattern[j + k] != ZEN_SPACE) {
                    bCharFound = true;
                    if (pattern[j + k] != word[k]) {
                        // マッチしなかった。
                        goto break_continue;
                    }
                }
            }
            // マッチした。
            if (bCharFound)
                return bCharFound;  // あった。
break_continue:;
        }
    }

    return false;   // なかった。
}

// 候補があるか？（黒マス追加なし）
template <bool t_alternative>
bool __fastcall XgAnyCandidateNoAddBlack(const std::wstring& pattern)
{
    // パターンの長さ。
    const int patlen = static_cast<int>(pattern.size());
    assert(patlen >= 2);

#ifndef NDEBUG
    // パターンに黒マスがないことを仮定する。
    // パターンに空白があることを仮定する。
    bool bSpaceFound = false;
    for (int i = 0; i < patlen; i++) {
        assert(pattern[i] != ZEN_BLACK);
        if (pattern[i] != ZEN_SPACE)
            bSpaceFound = true;
    }
    assert(bSpaceFound);
#endif

    // すべての単語について調べる。
    for (const auto& data : (t_alternative ? xg_dict_2 : xg_dict_1)) {
        // 単語の長さ。
        const std::wstring& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());

        // パターンと単語の長さが異なるとき、スキップする。
        if (wordlen != patlen)
            continue;

        // 単語がマッチするか？
        // 区間[0, wordlen - 1]に文字マスがあるか？
        bool bCharFound = false;
        for (int k = 0; k < wordlen; k++) {
            if (pattern[k] != ZEN_SPACE) {
                bCharFound = true;
                if (pattern[k] != word[k]) {
                    // マッチしなかった。
                    goto break_continue;
                }
            }
        }
        // マッチした。
        if (bCharFound)
            return bCharFound;      // あった。
break_continue:;
    }

    return false;   // なかった。
}

// カウントを更新する。
void XG_BoardEx::ReCount() {
    INT nCount = 0;
    for (INT i = 0; i < m_nRows; ++i) {
        for (INT j = 0; j < m_nCols; ++j) {
            nCount += (GetAt(i, j) != ZEN_SPACE);
        }
    }
    m_vCells[m_nRows * m_nCols] = static_cast<WCHAR>(nCount);
}

// 行を挿入する。
void XG_BoardEx::InsertRow(INT iRow) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows + 1, m_nCols);
    for (INT i = 0; i < m_nRows; ++i) {
        for (INT j = 0; j < m_nCols; ++j) {
            if (i < iRow)
                copy.SetAt2(i, j, m_nRows + 1, m_nCols, GetAt(i, j));
            else
                copy.SetAt2(i + 1, j, m_nRows + 1, m_nCols, GetAt(i, j));
        }
    }
    m_vCells = copy.m_vCells;
    ++m_nRows;
    ReCount();
}

// 列を挿入する。
void XG_BoardEx::InsertColumn(INT jCol) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows, m_nCols + 1);
    for (INT i = 0; i < m_nRows; ++i) {
        for (INT j = 0; j < m_nCols; ++j) {
            if (j < jCol)
                copy.SetAt2(i, j, m_nRows, m_nCols + 1, GetAt(i, j));
            else
                copy.SetAt2(i, j + 1, m_nRows, m_nCols + 1, GetAt(i, j));
        }
    }
    m_vCells = copy.m_vCells;
    ++m_nCols;
    ReCount();
}

// 行を削除する。
void XG_BoardEx::DeleteRow(INT iRow) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows - 1, m_nCols);
    for (INT i = 0; i < m_nRows - 1; ++i) {
        for (INT j = 0; j < m_nCols; ++j) {
            if (i < iRow)
                copy.SetAt2(i, j, m_nRows - 1, m_nCols, GetAt(i, j));
            else
                copy.SetAt2(i, j, m_nRows - 1, m_nCols, GetAt(i + 1, j));
        }
    }
    m_vCells = copy.m_vCells;
    --m_nRows;
    ReCount();
}

// 列を削除する。
void XG_BoardEx::DeleteColumn(INT jCol) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows, m_nCols - 1);
    for (INT i = 0; i < m_nRows; ++i) {
        for (INT j = 0; j < m_nCols - 1; ++j) {
            if (j < jCol)
                copy.SetAt2(i, j, m_nRows, m_nCols - 1, GetAt(i, j));
            else
                copy.SetAt2(i, j, m_nRows, m_nCols - 1, GetAt(i, j + 1));
        }
    }
    m_vCells = copy.m_vCells;
    --m_nCols;
    ReCount();
}

// 候補があるか？（黒マス追加なし、すべて空白）
bool __fastcall XgAnyCandidateWholeSpace(int patlen)
{
    // すべての単語について調べる。
    for (const auto& data : xg_dict_1) {
        const std::wstring& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());
        if (wordlen == patlen)
            return true;    // あった。
    }
    for (const auto& data : xg_dict_2) {
        const std::wstring& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());
        if (wordlen == patlen)
            return true;    // あった。
    }
    return false;   // なかった。
}

// 四隅に黒マスがあるかどうか。
bool __fastcall XG_Board::CornerBlack() const
{
    return (GetAt(0, 0) == ZEN_BLACK ||
            GetAt(xg_nRows - 1, 0) == ZEN_BLACK ||
            GetAt(xg_nRows - 1, xg_nCols - 1) == ZEN_BLACK ||
            GetAt(0, xg_nCols - 1) == ZEN_BLACK);
}

// 黒マスが隣り合っているか？
bool __fastcall XG_Board::DoubleBlack() const
{
    const int n1 = xg_nCols - 1;
    const int n2 = xg_nRows - 1;
    int i = xg_nRows;
    for (--i; i >= 0; --i) {
        for (int j = 0; j < n1; j++) {
            if (GetAt(i, j) == ZEN_BLACK && GetAt(i, j + 1) == ZEN_BLACK)
                return true;    // 隣り合っていた。
        }
    }
    int j = xg_nCols;
    for (--j; j >= 0; --j) {
        for (int i = 0; i < n2; i++) {
            if (GetAt(i, j) == ZEN_BLACK && GetAt(i + 1, j) == ZEN_BLACK)
                return true;    // 隣り合っていた。
        }
    }
    return false;   // 隣り合っていなかった。
}

// 三方向が黒マスで囲まれたマスがあるかどうか？
bool __fastcall XG_Board::TriBlackAround() const
{
    for (int i = xg_nRows - 2; i >= 1; --i) {
        for (int j = xg_nCols - 2; j >= 1; --j) {
            if ((GetAt(i - 1, j) == ZEN_BLACK) + (GetAt(i + 1, j) == ZEN_BLACK) + 
                (GetAt(i, j - 1) == ZEN_BLACK) + (GetAt(i, j + 1) == ZEN_BLACK) >= 3)
            {
                return true;    // あった。
            }
        }
    }
    return false;   // なかった。
}

// 黒マスで分断されているかどうか？
bool __fastcall XG_Board::DividedByBlack() const
{
    const INT nRows = xg_nRows, nCols = xg_nCols;
    INT nCount = nRows * nCols;

    // 各マスに対応するフラグ群。
    std::vector<BYTE> pb(nCount, 0);

    // 位置のキュー。
    // 黒マスではないマスを探し、positionsに追加する。
    std::queue<XG_Pos> positions;
    if (GetAt(0, 0) != ZEN_BLACK) {
        positions.emplace(0, 0);
    } else {
        for (INT i = 0; i < nRows; ++i) {
            for (INT j = 0; j < nCols; ++j) {
                if (GetAt(i, j) != ZEN_BLACK) {
                    positions.emplace(i, j);
                    i = nRows;
                    j = nCols;
                    break;
                }
            }
        }
    }

    // 連続領域の塗りつぶし。
    while (!positions.empty()) {
        // 位置をキューの一番上から取り出す。
        XG_Pos pos = positions.front();
        positions.pop();
        // フラグが立っていないか？
        if (!pb[pos.m_i * nCols + pos.m_j]) {
            // フラグを立てる。
            pb[pos.m_i * nCols + pos.m_j] = 1;
            // 上。
            if (pos.m_i > 0 && GetAt(pos.m_i - 1, pos.m_j) != ZEN_BLACK)
                positions.emplace(pos.m_i - 1, pos.m_j);
            // 下。
            if (pos.m_i < nRows - 1 && GetAt(pos.m_i + 1, pos.m_j) != ZEN_BLACK)
                positions.emplace(pos.m_i + 1, pos.m_j);
            // 左。
            if (pos.m_j > 0 && GetAt(pos.m_i, pos.m_j - 1) != ZEN_BLACK)
                positions.emplace(pos.m_i, pos.m_j - 1);
            // 右。
            if (pos.m_j < nCols - 1 && GetAt(pos.m_i, pos.m_j + 1) != ZEN_BLACK)
                positions.emplace(pos.m_i, pos.m_j + 1);
        }
    }

    // すべてのマスについて。
    while (nCount-- > 0) {
        // フラグが立っていないのに、黒マスではないマスがあったら、失敗。
        if (pb[nCount] == 0 && GetAt(nCount) != ZEN_BLACK) {
            // フラグ群を解放。
            return true;    // 分断されている。
        }
    }

    return false;   // 分断されていない。
}

// すべてのパターンが正当かどうか調べる。
XG_EpvCode __fastcall XG_Board::EveryPatternValid1(
    std::vector<std::wstring>& vNotFoundWords,
    XG_Pos& pos, bool bNonBlackCheckSpace) const
{
    const int nRows = xg_nRows, nCols = xg_nCols;

    // 使った単語の集合。
    std::unordered_set<std::wstring> used_words;
    // スピードのために、予約。
    used_words.reserve(nRows * nCols / 4);

    // 単語ベクターが空であることを仮定する。
    assert(vNotFoundWords.empty());
    //vNotFoundWords.clear();

    // 各行について、横向きにスキャンする。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // 黒マスではない、文字が含まれている連続した２マスか？
            const WCHAR ch1 = GetAt(i, j), ch2 = GetAt(i, j + 1);
            if (ch1 != ZEN_BLACK && ch2 != ZEN_BLACK &&
                (ch1 != ZEN_SPACE || ch2 != ZEN_SPACE))
            {
                // 文字が置けるヨコ向きの区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (GetAt(i, j + 1) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = j;
                j++;

                // パターンを生成する。
                bool bSpaceFound = false;
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(i, k);
                    if (ch == ZEN_SPACE) {
                        bSpaceFound = true;
                        for (; k <= hi; k++)
                            pattern += GetAt(i, k);
                        break;
                    }
                    pattern += ch;
                }

                // スペースがあったか？
                if (bSpaceFound) {
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (xg_bNoAddBlack) {
                        if (!XgAnyCandidateNoAddBlack<false>(pattern) &&
                            !XgAnyCandidateNoAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            pos = XG_Pos(i, lo);
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack<false>(pattern) &&
                            !XgAnyCandidateAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            pos = XG_Pos(i, lo);
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    }
                } else {
                    // すでに使用した単語があるか？
                    if (used_words.count(pattern))
                        return xg_epv_DOUBLEWORD;   // 単語の重複のため、失敗。

                    // 二分探索で単語があるかどうか調べる。
                    if (!XgWordDataExists(xg_dict_1, XG_WordData(pattern)) &&
                        !XgWordDataExists(xg_dict_2, XG_WordData(pattern)))
                    {
                        // 登録されていない単語なので、登録する。
                        vNotFoundWords.emplace_back(pattern);
                    }

                    // 一度使った単語として登録。
                    used_words.emplace(pattern);
                }
            } else if (bNonBlackCheckSpace && ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // ヨコ向きの区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (GetAt(i, j + 1) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = j;
                j++;

                // パターンを生成する。
                // 非空白があるか？
                bool bNonSpaceFound = false;
                int patlen = 0;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(i, k);
                    if (ch != ZEN_SPACE) {
                        bNonSpaceFound = true;
                        break;
                    }
                    patlen++;
                }

                if (!bNonSpaceFound) {
                    // 非空白がない。
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (!XgAnyCandidateWholeSpace(patlen)) {
                        // マッチしなかった位置。
                        pos = XG_Pos(i, lo);
                        return xg_epv_LENGTHMISMATCH;   // マッチしなかった。
                    }
                }
            }
        }
    }

    // 各列について、縦向きにスキャンする。
    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRows - 1; i++) {
            // 黒マスではない、文字が含まれている連続した２マスか？
            const WCHAR ch1 = GetAt(i, j), ch2 = GetAt(i + 1, j);
            if (ch1 != ZEN_BLACK && ch2 != ZEN_BLACK &&
                (ch1 != ZEN_SPACE || ch2 != ZEN_SPACE))
            {
                // 文字が置けるタテ向きの区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (GetAt(lo - 1, j) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (i + 1 < nRows) {
                    if (GetAt(i + 1, j) == ZEN_BLACK)
                        break;
                    i++;
                }
                const int hi = i;
                i++;

                // パターンを生成する。
                bool bSpaceFound = false;
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(k, j);
                    if (ch == ZEN_SPACE) {
                        bSpaceFound = true;
                        for (; k <= hi; k++)
                            pattern += GetAt(k, j);
                        break;
                    }
                    pattern += ch;
                }

                // スペースがあったか？
                if (bSpaceFound) {
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (xg_bNoAddBlack) {
                        if (!XgAnyCandidateNoAddBlack<false>(pattern) &&
                            !XgAnyCandidateNoAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            pos = XG_Pos(lo, j);
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack<false>(pattern) &&
                            !XgAnyCandidateAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            pos = XG_Pos(lo, j);
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    }
                } else {
                    // すでに使用した単語があるか？
                    if (used_words.count(pattern)) {
                        return xg_epv_DOUBLEWORD;   // 単語の重複のため、失敗。
                    }

                    // 二分探索で単語があるかどうか調べる。
                    if (!XgWordDataExists(xg_dict_1, XG_WordData(pattern)) &&
                        !XgWordDataExists(xg_dict_2, XG_WordData(pattern)))
                    {
                        // 登録されていない単語なので、登録する。
                        vNotFoundWords.emplace_back(pattern);
                    }

                    // 一度使った単語として登録。
                    used_words.emplace(pattern);
                }
            } else if (bNonBlackCheckSpace && ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // タテ向きの区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (GetAt(lo - 1, j) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (i + 1 < nRows) {
                    if (GetAt(i + 1, j) == ZEN_BLACK)
                        break;
                    i++;
                }
                const int hi = i;
                i++;

                // パターンを生成する。
                // 非空白があるか？
                bool bNonSpaceFound = false;
                int patlen = 0;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(k, j);
                    if (ch != ZEN_SPACE) {
                        bNonSpaceFound = true;
                        break;
                    }
                    patlen++;
                }

                if (!bNonSpaceFound) {
                    // 非空白がない。
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (!XgAnyCandidateWholeSpace(patlen)) {
                        // マッチしなかった位置。
                        pos = XG_Pos(lo, j);
                        return xg_epv_LENGTHMISMATCH;   // 長さが一致しなかった。
                    }
                }
            }
        }
    }

    return xg_epv_SUCCESS;      // 成功。
}

// すべてのパターンが正当かどうか調べる。
XG_EpvCode __fastcall XG_Board::EveryPatternValid2(
    std::vector<std::wstring>& vNotFoundWords,
    XG_Pos& pos, bool bNonBlackCheckSpace) const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    XG_Pos pos2 = pos;
    int& i = pos2.m_i;
    int& j = pos2.m_j;

    // 使った単語の集合。
    std::unordered_set<std::wstring> used_words;
    // スピードのため、予約。
    used_words.reserve(nRows * nCols / 4);

    // 単語ベクターが空であることを仮定する。
    assert(vNotFoundWords.empty());
    //vNotFoundWords.clear();

    // 各行について、横向きにスキャンする。
    const int nColsMinusOne = nCols - 1;
    for (i = 0; i < nRows; i++) {
        for (j = 0; j < nColsMinusOne; j++) {
            // 黒マスではない、文字が含まれている連続した２マスか？
            const WCHAR ch1 = GetAt(i, j), ch2 = GetAt(i, j + 1);
            if (ch1 != ZEN_BLACK && ch2 != ZEN_BLACK &&
                (ch1 != ZEN_SPACE || ch2 != ZEN_SPACE))
            {
                // 文字が置けるヨコ向きの区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (GetAt(i, --lo) == ZEN_BLACK) {
                        ++lo;
                        break;
                    }
                }
                while (j + 1 < nCols) {
                    if (GetAt(i, ++j) == ZEN_BLACK) {
                        --j;
                        break;
                    }
                }
                const int hi = j;
                j++;

                // パターンを生成する。
                bool bSpaceFound = false;
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(i, k);
                    if (ch == ZEN_SPACE) {
                        bSpaceFound = true;
                        for (; k <= hi; k++)
                            pattern += GetAt(i, k);
                        break;
                    }
                    pattern += ch;
                }

                // スペースがあったか？
                if (bSpaceFound) {
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (xg_bNoAddBlack) {
                        if (!XgAnyCandidateNoAddBlack<false>(pattern) &&
                            !XgAnyCandidateNoAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            j = lo;
                            pos = pos2;
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack<false>(pattern) &&
                            !XgAnyCandidateAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            j = lo;
                            pos = pos2;
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    }
                } else {
                    // すでに使用した単語があるか？
                    if (used_words.count(pattern)) {
                        pos = pos2;
                        return xg_epv_DOUBLEWORD;   // 単語の重複のため、失敗。
                    }

                    // 二分探索で単語があるかどうか調べる。
                    if (!XgWordDataExists(xg_dict_1, XG_WordData(pattern)) &&
                        !XgWordDataExists(xg_dict_2, XG_WordData(pattern)))
                    {
                        // 登録されていない単語なので、登録する。
                        vNotFoundWords.emplace_back(std::move(pattern));
                        pos = pos2;
                        return xg_epv_NOTFOUNDWORD; // 登録されていない単語があった。
                    }

                    // 一度使った単語として登録。
                    used_words.emplace(std::move(pattern));
                }
            } else if (bNonBlackCheckSpace && ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // ヨコ向きの区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (GetAt(i, --lo) == ZEN_BLACK) {
                        ++lo;
                        break;
                    }
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (GetAt(i, ++j) == ZEN_BLACK) {
                        --j;
                        break;
                    }
                }
                const int hi = j;
                j++;

                // パターンを生成する。
                // 非空白があるか？
                bool bNonSpaceFound = false;
                int patlen = 0;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(i, k);
                    if (ch != ZEN_SPACE) {
                        bNonSpaceFound = true;
                        break;
                    }
                    patlen++;
                }

                if (!bNonSpaceFound) {
                    // 非空白がない。
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (!XgAnyCandidateWholeSpace(patlen)) {
                        // マッチしなかった位置。
                        j = lo;
                        pos = pos2;
                        return xg_epv_LENGTHMISMATCH;   // 長さがマッチしなかった。
                    }
                }
            }
        }
    }

    // 各列について、縦向きにスキャンする。
    const int nRowsMinusOne = nRows - 1;
    for (j = 0; j < nCols; j++) {
        for (i = 0; i < nRowsMinusOne; i++) {
            // 黒マスではない、文字が含まれている連続した２マスか？
            const WCHAR ch1 = GetAt(i, j), ch2 = GetAt(i + 1, j);
            if (ch1 != ZEN_BLACK && ch2 != ZEN_BLACK &&
                (ch1 != ZEN_SPACE || ch2 != ZEN_SPACE))
            {
                // 文字が置けるタテ向きの区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (GetAt(--lo, j) == ZEN_BLACK) {
                        ++lo;
                        break;
                    }
                }
                while (i + 1 < nRows) {
                    if (GetAt(++i, j) == ZEN_BLACK) {
                        --i;
                        break;
                    }
                }
                const int hi = i;
                i++;

                // パターンを生成する。
                bool bSpaceFound = false;
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(k, j);
                    if (ch == ZEN_SPACE) {
                        bSpaceFound = true;
                        for (; k <= hi; k++)
                            pattern += GetAt(k, j);
                        break;
                    }
                    pattern += ch;
                }

                // スペースがあったか？
                if (bSpaceFound) {
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (xg_bNoAddBlack) {
                        if (!XgAnyCandidateNoAddBlack<false>(pattern) &&
                            !XgAnyCandidateNoAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            i = lo;
                            pos = pos2;
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack<false>(pattern) &&
                            !XgAnyCandidateAddBlack<true>(pattern))
                        {
                            // マッチしなかった位置。
                            i = lo;
                            pos = pos2;
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    }
                } else {
                    // すでに使用した単語があるか？
                    if (used_words.count(pattern)) {
                        pos = pos2;
                        return xg_epv_DOUBLEWORD;
                    }

                    // 二分探索で単語があるかどうか調べる。
                    if (!XgWordDataExists(xg_dict_1, XG_WordData(pattern)) &&
                        !XgWordDataExists(xg_dict_2, XG_WordData(pattern)))
                    {
                        // 登録されていない単語なので、登録する。
                        vNotFoundWords.emplace_back(std::move(pattern));
                        pos = pos2;
                        return xg_epv_NOTFOUNDWORD; // 登録されていない単語があった。s
                    }

                    // 一度使った単語として登録。
                    used_words.emplace(std::move(pattern));
                }
            } else if (bNonBlackCheckSpace && ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // タテ向きの区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (GetAt(--lo, j) == ZEN_BLACK) {
                        ++lo;
                        break;
                    }
                }
                while (i + 1 < nRows) {
                    if (GetAt(++i, j) == ZEN_BLACK) {
                        --i;
                        break;
                    }
                }
                const int hi = i;
                i++;

                // パターンを生成する。
                // 非空白があるか？
                bool bNonSpaceFound = false;
                int patlen = 0;
                for (int k = lo; k <= hi; k++) {
                    WCHAR ch = GetAt(k, j);
                    if (ch != ZEN_SPACE) {
                        bNonSpaceFound = true;
                        break;
                    }
                    patlen++;
                }

                if (!bNonSpaceFound) {
                    // 非空白がない。
                    // パターンにマッチする候補が存在しなければ失敗する。
                    if (!XgAnyCandidateWholeSpace(patlen)) {
                        // マッチしなかった位置。
                        i = lo;
                        pos = pos2;
                        return xg_epv_LENGTHMISMATCH;   // 長さがマッチしなかった。
                    }
                }
            }
        }
    }

    return xg_epv_SUCCESS;      // 成功。
}

// 正当かどうか？
inline bool __fastcall XG_Board::IsValid() const
{
    if ((xg_nRules & RULE_DONTCORNERBLACK) && CornerBlack())
        return false;
    if ((xg_nRules & RULE_DONTDOUBLEBLACK) && DoubleBlack())
        return false;
    if ((xg_nRules & RULE_DONTTRIDIRECTIONS) && TriBlackAround())
        return false;
    if ((xg_nRules & RULE_DONTDIVIDE) && DividedByBlack())
        return false;
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (ThreeDiagonals())
            return false;
    } else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
        if (FourDiagonals())
            return false;
    }

    // クロスワードに含まれる単語のチェック。
    XG_Pos pos;
    std::vector<std::wstring> vNotFoundWords;
    vNotFoundWords.reserve(xg_nRows * xg_nCols / 4);
    XG_EpvCode code = EveryPatternValid2(vNotFoundWords, pos, false);
    if (code != xg_epv_SUCCESS || !vNotFoundWords.empty())
        return false;

    // 空のクロスワードを解いているときは、分断禁をチェックする必要はない。
    // 分断禁。
    if (!xg_bSolvingEmpty && DividedByBlack())
        return false;

    return true;    // 成功。
}

// 正当かどうか？（簡略版、黒マス追加なし）
bool __fastcall XG_Board::IsNoAddBlackOK() const
{
    // クロスワードに含まれる単語のチェック。
    XG_Pos pos;
    std::vector<std::wstring> vNotFoundWords;
    vNotFoundWords.reserve(xg_nRows * xg_nCols / 4);
    XG_EpvCode code = EveryPatternValid2(vNotFoundWords, pos, false);
    if (code != xg_epv_SUCCESS || !vNotFoundWords.empty())
        return false;

    // 空のクロスワードを解いているときは、分断禁をチェックする必要はない。

    return true;    // 成功。
}

// 番号をつける。
bool __fastcall XG_Board::DoNumbering()
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;

    // 使った単語の集合。
    std::unordered_set<std::wstring> used_words;
    // スピードのため、予約。
    used_words.reserve(nRows * nCols / 4);

    // 単語データ。
    XG_WordData wd;

#ifndef NDEBUG
    // 空マスがないことを仮定する。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            assert(GetAt(i, j) != ZEN_SPACE);
        }
    }
#endif

    // カギをクリアする。
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();

    // 各列について、縦向きにスキャンする。
    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRows - 1; i++) {
            // 文字マスの連続が見つかったか？
            if (GetAt(i, j) != ZEN_BLACK && GetAt(i + 1, j) != ZEN_BLACK) {
                int lo, hi;

                // 単語が置ける区間を求める。
                lo = hi = i;
                while (hi + 1 < nRows) {
                    if (GetAt(hi + 1, j) == ZEN_BLACK)
                        break;
                    hi++;
                }
                assert(hi == nRows - 1 || GetAt(hi + 1, j) == ZEN_BLACK);

                // 次の位置を設定する。
                i = hi + 1;

                // 空白があるかを調べると同時に、その区間にある単語を取得する。
                std::wstring word;
                for (int k = lo; k <= hi; k++) {
                    if (GetAt(k, j) == ZEN_SPACE)
                        goto space_found_1;

                    word += GetAt(k, j);
                }

                // 空白が見つからなかった。
                // すでに使用した単語があるか？
                if (used_words.count(word)) {
                    return false;   // すでに使用した単語が使われたので、失敗。
                }

                // 二分探索で単語があるかどうか調べる。
                wd.m_word = word;
                if (XgWordDataExists(xg_dict_1, wd) || XgWordDataExists(xg_dict_2, wd)) {
                    // 一度使った単語として登録。
                    used_words.emplace(word);

                    // 縦のカギに登録。
                    xg_vTateInfo.emplace_back(lo, j, std::move(word));
                } else {
                    return false;   // 登録されていない単語があったので失敗。
                }
space_found_1:;
            }
        }
    }

    // 各行について横向きにスキャンする。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // 文字マスの連続が見つかったか？
            if (GetAt(i, j) != ZEN_BLACK && GetAt(i, j + 1) != ZEN_BLACK) {
                int lo, hi;

                // 単語が置ける区間を求める。
                lo = hi = j;
                while (hi + 1 < nCols) {
                    if (GetAt(i, hi + 1) == ZEN_BLACK)
                        break;
                    hi++;
                }
                assert(hi == nCols - 1 || GetAt(i, hi + 1) == ZEN_BLACK);

                // 次の位置を設定する。
                j = hi + 1;

                // 空白があるかを調べると同時に、その区間にある単語を取得する。
                std::wstring word;
                for (int k = lo; k <= hi; k++) {
                    if (GetAt(i, k) == ZEN_SPACE)
                        goto space_found_2;

                    word += GetAt(i, k);
                }

                // 空白が見つからなかった。
                // すでに使用した単語があるか？
                if (used_words.count(word)) {
                    return false;   // 使用した単語が使われたため、失敗。
                }

                // 二分探索で単語があるかどうか調べる。
                wd.m_word = word;
                if (XgWordDataExists(xg_dict_1, wd) || XgWordDataExists(xg_dict_2, wd)) {
                    // 一度使った単語として登録。
                    used_words.emplace(word);

                    // 横のカギとして登録。
                    xg_vYokoInfo.emplace_back(i, lo, std::move(word));
                } else {
                    return false;   // 登録されていない単語のため、失敗。
                }
space_found_2:;
            }
        }
    }

    // カギの格納情報に順序をつける。
    std::vector<XG_PlaceInfo *> data;
    const int size1 = static_cast<int>(xg_vTateInfo.size());
    const int size2 = static_cast<int>(xg_vYokoInfo.size());
    data.reserve(size1 + size2);
    for (int k = 0; k < size1; k++) {
        data.emplace_back(&xg_vTateInfo[k]);
    }
    for (int k = 0; k < size2; k++) {
        data.emplace_back(&xg_vYokoInfo[k]);
    }
    sort(data.begin(), data.end(), xg_placeinfo_compare_position());

    // 順序付けられたカギの格納情報に番号を設定する。
    int number = 1;
    {
        const int size = static_cast<int>(data.size());
        for (int k = 0; k < size; k++) {
            // 番号を設定する。
            data[k]->m_number = number;
            if (k + 1 < size) {
                // 次の格納情報が同じ位置ならば
                if (data[k]->m_iRow == data[k + 1]->m_iRow &&
                    data[k]->m_jCol == data[k + 1]->m_jCol)
                {
                    // 無視する。
                    ;
                } else {
                    // 違う位置なら、番号を増やす。
                    number++;
                }
            }
        }
    }

    // カギの格納情報を番号順に並べ替える。
    sort(xg_vTateInfo.begin(), xg_vTateInfo.end(), xg_placeinfo_compare_number());
    sort(xg_vYokoInfo.begin(), xg_vYokoInfo.end(), xg_placeinfo_compare_number());

    return true;
}

// 番号をつける（チェックなし）。
void __fastcall XG_Board::DoNumberingNoCheck()
{
    // 単語データ。
    XG_WordData wd;

    const int nRows = xg_nRows;
    const int nCols = xg_nCols;

#ifndef NDEBUG
    // 空マスがないと仮定する。
    for (int i = 0; i < nRows * nCols; i++) {
        assert(GetAt(i) != ZEN_SPACE);
    }
#endif

    // カギをクリアする。
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();

    // 各列について、縦向きにスキャンする。
    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRows - 1; i++) {
            // 文字マスの連続が見つかったか？
            if (GetAt(i, j) != ZEN_BLACK && GetAt(i + 1, j) != ZEN_BLACK) {
                int lo, hi;

                // 単語が置ける区間を求める。
                lo = hi = i;
                while (hi + 1 < nRows) {
                    if (GetAt(hi + 1, j) == ZEN_BLACK)
                        break;
                    hi++;
                }
                assert(hi == nRows - 1 || GetAt(hi + 1, j) == ZEN_BLACK);

                // 次の位置を設定する。
                i = hi + 1;

                // 空白があるかを調べると同時に、その区間にある単語を取得する。
                bool bFound = false;
                std::wstring word;
                for (int k = lo; k <= hi; k++) {
                    if (GetAt(k, j) == ZEN_SPACE) {
                        bFound = true;
                        break;
                    }
                    word += GetAt(k, j);
                }

                // 空白が見つからなかったか？
                if (!bFound) {
                    // 単語を登録する。
                    xg_vTateInfo.emplace_back(lo, j, std::move(word));
                }
            }
        }
    }

    // 各行について、横向きにスキャンする。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // 文字マスの連続が見つかったか？
            if (GetAt(i, j) != ZEN_BLACK && GetAt(i, j + 1) != ZEN_BLACK) {
                int lo, hi;

                // 単語が置ける区間を求める。
                lo = hi = j;
                while (hi + 1 < nCols) {
                    if (GetAt(i, hi + 1) == ZEN_BLACK)
                        break;
                    hi++;
                }
                assert(hi == nCols - 1 || GetAt(i, hi + 1) == ZEN_BLACK);

                // 次の位置を設定する。
                j = hi + 1;

                // 空白があるかを調べると同時に、その区間にある単語を取得する。
                std::wstring word;
                bool bFound = false;
                for (int k = lo; k <= hi; k++) {
                    if (GetAt(i, k) == ZEN_SPACE) {
                        bFound = true;
                        break;
                    }
                    word += GetAt(i, k);
                }

                // 空白が見つからなかったか？
                if (!bFound) {
                    // 横のカギとして登録。
                    xg_vYokoInfo.emplace_back(i, lo, std::move(word));
                }
            }
        }
    }

    // カギの格納情報に順序をつける。
    std::vector<XG_PlaceInfo *> data;
    {
        const int size = static_cast<int>(xg_vTateInfo.size());
        for (int k = 0; k < size; k++) {
            data.emplace_back(&xg_vTateInfo[k]);
        }
    }
    {
        const int size = static_cast<int>(xg_vYokoInfo.size());
        for (int k = 0; k < size; k++) {
            data.emplace_back(&xg_vYokoInfo[k]);
        }
    }
    sort(data.begin(), data.end(), xg_placeinfo_compare_position());

    // 順序付けられたカギの格納情報に番号を設定する。
    int number = 1;
    {
        const int size = static_cast<int>(data.size());
        for (int k = 0; k < size; k++) {
            // 番号を設定する。
            data[k]->m_number = number;
            if (k + 1 < size) {
                // 次の格納情報が同じ位置ならば
                if (data[k]->m_iRow == data[k + 1]->m_iRow &&
                    data[k]->m_jCol == data[k + 1]->m_jCol)
                {
                    // 無視する。
                    ;
                } else {
                    // 違う位置なら、番号を増やす。
                    number++;
                }
            }
        }
    }

    // カギの格納情報を番号順に並べ替える。
    sort(xg_vTateInfo.begin(), xg_vTateInfo.end(), xg_placeinfo_compare_number());
    sort(xg_vYokoInfo.begin(), xg_vYokoInfo.end(), xg_placeinfo_compare_number());
}

// 候補を取得する。
template <bool t_alternative> bool __fastcall
XgGetCandidatesAddBlack(
    std::vector<std::wstring>& cands, const std::wstring& pattern, int& nSkip,
    bool left_black_check, bool right_black_check)
{
    // パターンの長さ。
    const int patlen = static_cast<int>(pattern.size());
    assert(patlen >= 2);

#ifndef NDEBUG
    // パターンに黒マスがないことを仮定する。
    // パターンに空白があることを仮定する。
    bool bSpaceFound = false;
    for (int i = 0; i < patlen; i++) {
        assert(pattern[i] != ZEN_BLACK);
        if (pattern[i] == ZEN_SPACE)
            bSpaceFound = true;
    }
    assert(bSpaceFound);
#endif

    // 候補をクリアする。
    cands.clear();
    // スピードのために予約する。
    if (t_alternative)
        cands.reserve(xg_dict_2.size() / 32);
    else
        cands.reserve(xg_dict_1.size() / 32);

    // 初期化する。
    nSkip = 0;
    std::wstring result(pattern);

    // パターンが3字以上か？
    if (patlen > 2) {
        // 孤立した文字マスが左端か？
        if (result[0] != ZEN_SPACE && result[1] == ZEN_SPACE) {
            // その右にブロックを置いたものを候補にする。
            result[1] = ZEN_BLACK;
            cands.emplace_back(result);
            result = pattern;
            // これは先に処理されるべきなので、ランダム化から除外する。
            nSkip++;
        } else {
            bool bCharNotFound = true;  // 文字マスがなかったか？

            // 孤立した文字マスが中にあるか？
            for (int j = 1; j < patlen - 1; j++) {
                if (result[j] != ZEN_SPACE) {
                    if (result[j - 1] == ZEN_SPACE && result[j + 1] == ZEN_SPACE) {
                        // 孤立した文字マスがあった。
                        // その両端にブロックを置く。
                        result[j - 1] = result[j + 1] = ZEN_BLACK;
                        cands.emplace_back(result);
                        result = pattern;
                        // これは先に処理されるべきなので、ランダム化から除外する。
                        nSkip++;
                    }
                    bCharNotFound = false;
                    break;
                }
            }

            if (bCharNotFound) {
                // 孤立した文字マスが右端か？
                if (result[patlen - 1] != ZEN_SPACE && result[patlen - 2] == ZEN_SPACE) {
                    // その左にブロックを置く。
                    result[patlen - 2] = ZEN_BLACK;
                    cands.emplace_back(result);
                    result = pattern;
                    // これは先に処理されるべきなので、ランダム化から除外する。
                    nSkip++;
                }
            }
        }
    }

    // すべての登録されている単語について。
    for (const auto& data : (t_alternative ? xg_dict_2 : xg_dict_1)) {
        // パターンより単語の方が長い場合、スキップする。
        const std::wstring& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());
        if (wordlen > patlen)
            continue;

        // 単語の置ける区間について調べる。
        const int patlen_minus_wordlen = patlen - wordlen;
        for (int j = 0; j <= patlen_minus_wordlen; j++) {
            // 区間[j, j + wordlen - 1]の前後に文字があったらスキップする。
            if (j > 0 && pattern[j - 1] != ZEN_SPACE)
                continue;
            if (j < patlen_minus_wordlen && pattern[j + wordlen] != ZEN_SPACE)
                continue;

            // 区間[j, j + wordlen - 1]に文字マスがあるか？
            bool bCharFound = false;
            const int j_plus_wordlen = j + wordlen;
            for (int m = j; m < j_plus_wordlen; m++) {
                assert(pattern[m] != ZEN_BLACK);
                if (pattern[m] != ZEN_SPACE) {
                    bCharFound = true;
                    break;
                }
            }
            if (!bCharFound)
                continue;

            // パターンが単語にマッチするか？
            bool bMatched = true;
            for (int m = j, n = 0; n < wordlen; m++, n++) {
                if (pattern[m] != ZEN_SPACE && pattern[m] != word[n]) {
                    bMatched = false;
                    break;
                }
            }
            if (!bMatched)
                continue;

            // マッチした。
            result = pattern;

            // 区間[j, j + wordlen - 1]の前後に■をおく。
            if (j > 0)
                result[j - 1] = ZEN_BLACK;
            if (j < patlen_minus_wordlen)
                result[j + wordlen] = ZEN_BLACK;

            // 区間[j, j + wordlen - 1]に単語を適用する。
            for (int k = 0, m = j; k < wordlen; k++, m++)
                result[m] = word[k];

            // 黒マスの連続を除外する。
            if (left_black_check && result[0] == ZEN_BLACK)
                continue;
            if (right_black_check && result[patlen - 1] == ZEN_BLACK)
                continue;

            // 追加する。
            cands.emplace_back(result);
        }
    }

    // 候補が空でなければ成功。
    return !cands.empty();
}

// 候補を取得する（黒マス追加なし）。
template <bool t_alternative> bool __fastcall
XgGetCandidatesNoAddBlack(std::vector<std::wstring>& cands, const std::wstring& pattern)
{
    // 単語の長さ。
    const int patlen = static_cast<int>(pattern.size());
    assert(patlen >= 2);

    // 候補をクリアする。
    cands.clear();
    // スピードのため、予約する。
    if (t_alternative)
        cands.reserve(xg_dict_2.size() / 32);
    else
        cands.reserve(xg_dict_1.size() / 32);

    // すべての登録された単語について。
    for (const auto& data : (t_alternative ? xg_dict_2 : xg_dict_1)) {
        // パターンと単語の長さが等しくなければ、スキップする。
        const std::wstring& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());
        if (wordlen != patlen)
            continue;

        // 区間[0, wordlen - 1]に文字マスがあるか？
        bool bCharFound = false;
        for (int k = 0; k < wordlen; k++) {
            assert(pattern[k] != ZEN_BLACK);
            if (pattern[k] != ZEN_SPACE) {
                bCharFound = true;
                break;
            }
        }
        if (!bCharFound)
            continue;

        // パターンが単語にマッチするか？
        bool bMatched = true;
        for (int k = 0; k < wordlen; k++) {
            if (pattern[k] != ZEN_SPACE && pattern[k] != word[k]) {
                bMatched = false;
                break;
            }
        }
        if (!bMatched)
            continue;

        // 追加する。
        cands.emplace_back(word);
    }

    // 候補が空でなければ成功。
    return !cands.empty();
}

// 解か？
bool __fastcall XG_Board::IsSolution() const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;

    // 空きマスがあれば解ではない。
#if 1
    if (Count() != nRows * nCols)
        return false;
#else
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if (GetAt(i, j) == ZEN_SPACE)
                return false;
        }
    }
#endif

    return IsValid();
}

// ヒント文字列を解析する。
bool __fastcall XgParseHints(std::vector<XG_Hint>& hints, const std::wstring& str)
{
    // ヒントをクリアする。
    hints.clear();

    int count = 0;
    for (size_t i = 0;;) {
        if (count++ > 1000) {
            return false;
        }

        size_t i0 = str.find(XgLoadStringDx1(IDS_KEYLEFT), i);
        if (i0 == std::wstring::npos) {
            break;
        }

        size_t i1 = str.find_first_of(L"0123456789", i0);
        if (i1 == std::wstring::npos) {
            return false;
        }

        int number = _wtoi(str.data() + i1);
        if (number <= 0) {
            return false;
        }

        size_t i2 = str.find(XgLoadStringDx1(IDS_KEYRIGHT), i0);
        if (i2 == std::wstring::npos) {
            return false;
        }

        std::wstring word;
        size_t i3, i4;
        i3 = str.find(L"\x226A", i2);
        if (i3 != std::wstring::npos) {
            i3 += wcslen(L"\x226A");

            i4 = str.find(L"\x226B", i3);
            if (i4 == std::wstring::npos) {
                return false;
            }
            word = str.substr(i3, i4 - i3);
            i4 += wcslen(L"\x226B");
        } else {
            i4 = i2 + wcslen(XgLoadStringDx1(IDS_KEYRIGHT));
        }

        size_t i5 = str.find(XgLoadStringDx1(IDS_KEYLEFT), i4);
        if (i5 == std::wstring::npos) {
            std::wstring hint = str.substr(i4);
            xg_str_replace_all(hint, L"\r", L"");
            xg_str_replace_all(hint, L"\n", L"");
            xg_str_replace_all(hint, L"\t", L"");
            xg_str_trim(hint);
            hints.emplace_back(number, std::move(word), std::move(hint));
            break;
        } else {
            std::wstring hint = str.substr(i4, i5 - i4);
            xg_str_replace_all(hint, L"\r", L"");
            xg_str_replace_all(hint, L"\n", L"");
            xg_str_replace_all(hint, L"\t", L"");
            xg_str_trim(hint);
            hints.emplace_back(number, std::move(word), std::move(hint));
            i = i5;
        }
    }

    return true;
}

// ヒント文字列を解析する。
bool __fastcall XgParseHintsStr(const std::wstring& strHints)
{
    // ヒントをクリアする。
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();

    // ヒント文字列の前後の空白を取り除く。
    std::wstring str(strHints);
    xg_str_trim(str);

    // strCaption1とstrCaption2により、tateとyokoに分ける。
    std::wstring strCaption1 = XgLoadStringDx1(IDS_DOWN);
    std::wstring strCaption2 = XgLoadStringDx1(IDS_ACROSS);
    size_t i1 = str.find(strCaption1);
    if (i1 == std::wstring::npos)
        return false;
    i1 += strCaption1.size();
    size_t i2 = str.find(strCaption2);
    if (i2 == std::wstring::npos)
        return false;
    std::wstring tate = str.substr(i1, i2 - i1);
    i2 += strCaption2.size();
    std::wstring yoko = str.substr(i2);

    // 備考欄を取り除く。
    size_t i3 = yoko.find(XgLoadStringDx1(IDS_HEADERSEP2));
    if (i3 != std::wstring::npos) {
        yoko = yoko.substr(0, i3);
    }

    // 前後の空白を取り除く。
    xg_str_trim(tate);
    xg_str_trim(yoko);

    // それぞれについて解析する。
    return XgParseHints(xg_vecTateHints, tate) &&
           XgParseHints(xg_vecYokoHints, yoko);
}

// ヒントを取得する。
void __fastcall
XgGetHintsStr(const XG_Board& board, std::wstring& str, int hint_type, bool bShowAnswer)
{
    // 文字列バッファ。
    WCHAR sz[64];

    // 初期化。
    str.clear();

    // まだ解かれていない場合は、何も返さない。
    if (!xg_bSolved)
        return;

    assert(0 <= hint_type && hint_type < 6);

    if (hint_type == 0 || hint_type == 2) {
        // タテのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_DOWN);
        str += xg_pszNewLine;

        for (const auto& info : xg_vTateInfo) {
            // 番号を格納する。
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_DOWNNUMBER), info.m_number);
            str += sz;

            // 答えを見せるかどうか？
            if (bShowAnswer) {
                str += L"\x226A";
                str += info.m_word;
                str += L"\x226B";
            }

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecTateHints) {
                if (hint.m_strWord == info.m_word) {
                    str += hint.m_strHint;
                    added = true;
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_1) {
                    if (_wcsicmp(data.m_word.data(),
                        info.m_word.data()) == 0)
                    {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_2) {
                    if (_wcsicmp(data.m_word.data(),
                        info.m_word.data()) == 0)
                    {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            str += xg_pszNewLine;   // 改行。
        }
        str += xg_pszNewLine;
    }
    if (hint_type == 1 || hint_type == 2) {
        // ヨコのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_ACROSS);
        str += xg_pszNewLine;
        for (const auto& info : xg_vYokoInfo) {
            // 番号を格納する。
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_ACROSSNUMBER), info.m_number);
            str += sz;

            // 答えを見せるかどうか？
            if (bShowAnswer) {
                str += L"\x226A";
                str += info.m_word;
                str += L"\x226B";
            }

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecYokoHints) {
                if (hint.m_strWord == info.m_word) {
                    str += hint.m_strHint;
                    added = true;
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_1) {
                    if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_2) {
                    if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            str += xg_pszNewLine;   // 改行。
        }
        str += xg_pszNewLine;
    }
    if (hint_type == 3 || hint_type == 5) {
        // タテのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_PARABOLD);     // <p><b>
        str += XgLoadStringDx1(IDS_DOWNLABEL);
        str += XgLoadStringDx1(IDS_ENDPARABOLD);    // </b></p>
        str += xg_pszNewLine;
        str += XgLoadStringDx1(IDS_OL);    // <ol>
        str += xg_pszNewLine;

        for (const auto& info : xg_vTateInfo) {
            // <li>
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_LI), info.m_number);
            str += sz;

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecTateHints) {
                if (hint.m_strWord == info.m_word) {
                    str += hint.m_strHint;
                    added = true;
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_1) {
                    if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_2) {
                    if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            str += XgLoadStringDx1(IDS_ENDLI);    // </li>
            str += xg_pszNewLine;           // 改行。
        }
        str += XgLoadStringDx1(IDS_ENDOL);    // </ol>
        str += xg_pszNewLine;           // 改行。
    }
    if (hint_type == 4 || hint_type == 5) {
        // ヨコのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_PARABOLD);     // <p><b>
        str += XgLoadStringDx1(IDS_ACROSSLABEL);
        str += XgLoadStringDx1(IDS_ENDPARABOLD);    // </b></p>
        str += xg_pszNewLine;
        str += XgLoadStringDx1(IDS_OL);    // <ol>
        str += xg_pszNewLine;

        for (const auto& info : xg_vYokoInfo) {
            // <li>
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_LI), info.m_number);
            str += sz;

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecYokoHints) {
                if (hint.m_strWord == info.m_word) {
                    str += hint.m_strHint;
                    added = true;
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_1) {
                    if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            if (!added) {
                for (const auto& data : xg_dict_2) {
                    if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                        str += data.m_hint;
                        added = true;
                        break;
                    }
                }
            }
            str += XgLoadStringDx1(IDS_ENDLI);    // </li>
            str += xg_pszNewLine;           // 改行。
        }
        str += XgLoadStringDx1(IDS_ENDOL);    // </ol>
        str += xg_pszNewLine;           // 改行。
    }
}

// 文字列のルールを解析する。
INT __fastcall XgParseRules(const std::wstring& str)
{
    INT nRules = 0;
    std::vector<std::wstring> rules;
    if (str.find(L" / ") != str.npos) { // もし" / "が含まれていたら
        mstr_split(rules, str, L"/"); // "/"で分割する。
    } else { // 含まれていなければ
        mstr_split(rules, str, L" \t"); // 空白で分割する。
    }
    for (auto& rule : rules) {
        xg_str_trim(rule); // 前後の空白を取り除く。
        if (rule.empty())
            continue;
        if (rule == XgLoadStringDx1(IDS_RULE_DONTDOUBLEBLACK)) {
            nRules |= RULE_DONTDOUBLEBLACK;
        } else if (rule == XgLoadStringDx1(IDS_RULE_DONTCORNERBLACK)) {
            nRules |= RULE_DONTCORNERBLACK;
        } else if (rule == XgLoadStringDx1(IDS_RULE_DONTTRIDIRECTIONS)) {
            nRules |= RULE_DONTTRIDIRECTIONS;
        } else if (rule == XgLoadStringDx1(IDS_RULE_DONTDIVIDE)) {
            nRules |= RULE_DONTDIVIDE;
        } else if (rule == XgLoadStringDx1(IDS_RULE_DONTTHREEDIAGONALS)) {
            nRules |= RULE_DONTTHREEDIAGONALS;
        } else if (rule == XgLoadStringDx1(IDS_RULE_DONTFOURDIAGONALS)) {
            nRules |= RULE_DONTFOURDIAGONALS;
        } else if (rule == XgLoadStringDx1(IDS_RULE_POINTSYMMETRY)) {
            nRules |= RULE_POINTSYMMETRY;
        } else if (rule == XgLoadStringDx1(IDS_RULE_LINESYMMETRYV)) {
            nRules |= RULE_LINESYMMETRYV;
        } else if (rule == XgLoadStringDx1(IDS_RULE_LINESYMMETRYH)) {
            nRules |= RULE_LINESYMMETRYH;
        }
    }
    return nRules;
}

// ルールを文字列にする。
std::wstring __fastcall XgGetRulesString(INT rules)
{
    // メモ：英語対応のため、空白区切りから" / "区切りに変更しました。
    std::wstring ret;

    if (rules & RULE_DONTDOUBLEBLACK) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_DONTDOUBLEBLACK);
    }
    if (rules & RULE_DONTCORNERBLACK) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_DONTCORNERBLACK);
    }
    if (rules & RULE_DONTTRIDIRECTIONS) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_DONTTRIDIRECTIONS);
    }
    if (rules & RULE_DONTDIVIDE) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_DONTDIVIDE);
    }
    if (rules & RULE_DONTTHREEDIAGONALS) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_DONTTHREEDIAGONALS);
    } else if (rules & RULE_DONTFOURDIAGONALS) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_DONTFOURDIAGONALS);
    }
    if (rules & RULE_POINTSYMMETRY) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_POINTSYMMETRY);
    }
    if (rules & RULE_LINESYMMETRYV) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_LINESYMMETRYV);
    }
    if (rules & RULE_LINESYMMETRYH) {
        if (ret.size())  {
            ret += L" / ";
        }
        ret += XgLoadStringDx1(IDS_RULE_LINESYMMETRYH);
    }
    ret += L" / ";
    return ret;
}

// JSON文字列を設定する。
bool __fastcall XgSetJsonString(HWND hwnd, const std::wstring& str)
{
    std::string utf8 = XgUnicodeToUtf8(str);

    try {
        json j = json::parse(utf8);
        int row_count = j["row_count"];
        int column_count = j["column_count"];
        auto cell_data = j["cell_data"];
        bool is_solved = j["is_solved"];
        bool has_mark = j["has_mark"];
        bool has_hints = j["has_hints"];
        INT rules = DEFAULT_RULES;
        if (j["rules"].is_string()) {
            auto str = XgUtf8ToUnicode(j["rules"].get<std::string>());
            rules = XgParseRules(str);
        }
        std::wstring dictionary;
        if (j["dictionary"].is_string()) {
            dictionary = XgUtf8ToUnicode(j["dictionary"].get<std::string>());
        }
        if (j["view_mode"].is_number_integer()) {
            switch (int(j["view_mode"])) {
            case XG_VIEW_NORMAL:
                xg_nViewMode = XG_VIEW_NORMAL;
                break;
            case XG_VIEW_SKELETON:
                xg_nViewMode = XG_VIEW_SKELETON;
                break;
            default:
                xg_nViewMode = XG_VIEW_NORMAL;
                break;
            }
        }

        if (row_count <= 0 || column_count <= 0) {
            return false;
        }

        if (row_count != int(cell_data.size())) {
            return false;
        }

        XG_Board xw;
        int nRowsSave = xg_nRows, nColsSave = xg_nCols;
        xg_nRows = row_count;
        xg_nCols = column_count;
        xw.ResetAndSetSize(row_count, column_count);

        // ボックス。
        XgDeleteBoxes();
        if (j["boxes"].is_array()) {
            XgDoLoadBoxJson(j["boxes"]);
        }

        bool success = true;

        for (int i = 0; i < row_count; ++i) {
            std::wstring row = XgUtf8ToUnicode(cell_data[i]);
            if (int(row.size()) != column_count) {
                success = false;
                break;
            }
            for (int j = 0; j < column_count; ++j) {
                xw.SetAt(i, j, row[j]);
            }
        }

        std::vector<XG_Pos> mark_positions;
        if (has_mark) {
            auto marks = j["marks"];
            for (size_t k = 0; k < marks.size(); ++k) {
                auto mark = marks[k];
                int i = int(mark[0]) - 1;
                int j = int(mark[1]) - 1;
                if (i < 0 || row_count < i) {
                    success = false;
                    break;
                }
                if (j < 0 || column_count < j) {
                    success = false;
                    break;
                }
                mark_positions.emplace_back(i, j);
            }
        }

        std::vector<XG_Hint> tate, yoko;
        if (has_hints) {
            auto hints = j["hints"];
            auto v = hints["v"];
            auto h = hints["h"];
            for (size_t i = 0; i < v.size(); ++i) {
                auto data = v[i];
                int number = int(data[0]);
                if (number <= 0) {
                    success = false;
                    break;
                }
                auto word = XgUtf8ToUnicode(data[1]);
                auto hint = XgUtf8ToUnicode(data[2]);
                tate.emplace_back(number, word, hint);
            }
            for (size_t i = 0; i < h.size(); ++i) {
                auto data = h[i];
                int number = int(data[0]);
                if (number <= 0) {
                    success = false;
                    break;
                }
                auto word = XgUtf8ToUnicode(data[1]);
                auto hint = XgUtf8ToUnicode(data[2]);
                yoko.emplace_back(number, word, hint);
            }
        }

        auto header = XgUtf8ToUnicode(j["header"]);
        auto notes = XgUtf8ToUnicode(j["notes"]);
        if (j["theme"].is_string()) {
            xg_strTheme = XgUtf8ToUnicode(j["theme"]);
        } else {
            xg_strTheme = xg_strDefaultTheme;
        }

        if (success) {
            // カギをクリアする。
            xg_vTateInfo.clear();
            xg_vYokoInfo.clear();

            if (is_solved) {
                xg_bSolved = true;
                xg_solution = xw;
                xg_xword.ResetAndSetSize(row_count, column_count);
                for (int i = 0; i < xg_nRows; i++) {
                    for (int j = 0; j < xg_nCols; j++) {
                        // 解に合わせて、問題に黒マスを置く。
                        if (xg_solution.GetAt(i, j) == ZEN_BLACK)
                            xg_xword.SetAt(i, j, ZEN_BLACK);
                    }
                }
                if (has_hints) {
                    xg_solution.DoNumberingNoCheck();
                }
                XgGetHintsStr(xg_solution, xg_strHints, 2, true);
            } else {
                xg_bSolved = false;
                xg_xword = xw;
            }
            xg_vMarks = mark_positions;
            xg_vecTateHints = tate;
            xg_vecYokoHints = yoko;
            xg_strHeader = header;
            xg_str_trim(xg_strHeader);
            xg_strNotes = notes;
            xg_str_trim(xg_strNotes);

            LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
            if (xg_strNotes.empty()) {
                ;
            } else if (xg_strNotes.find(psz) == 0) {
                xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
                xg_str_trim(xg_strNotes);
            }

            // ヒント追加フラグをクリアする。
            xg_bHintsAdded = false;

            // ルールを設定する。
            xg_nRules = rules | RULE_DONTDIVIDE;

            // 辞書名の辞書を読み込む。
            if (dictionary.size()) {
                for (auto& entry : xg_dict_files) {
                    auto& file = entry.m_filename;
                    if (file.find(dictionary) != std::wstring::npos) {
                        if (XgLoadDictFile(file.c_str())) {
                            XgSetDict(file.c_str());
                            XgSetInputModeFromDict(xg_hMainWnd);
                            break;
                        }
                    }
                }
            }
        } else {
            xg_nRows = nRowsSave;
            xg_nCols = nColsSave;
        }

        return success;
    }
    catch (json::exception&)
    {
    }

    return false;
}

// 文字列を設定する。
bool __fastcall XgSetStdString(HWND hwnd, const std::wstring& str)
{
    // クロスワードデータ。
    XG_Board xword;

    // 文字列を読み込む。
    if (!xword.SetString(str))
        return false;

    // 空きマスが存在しないか？
    bool bFulfilled = xword.IsFulfilled();
    if (bFulfilled) {
        // 空きマスがなかった。

        // ヒントを設定する。
        size_t i = str.find(ZEN_LRIGHT, 0);
        if (i != std::wstring::npos) {
            // フッターを取得する。
            std::wstring s = str.substr(i + 1);

            // フッターの備考欄を取得して、取り除く。
            std::wstring strFooterSep = XgLoadStringDx1(IDS_HEADERSEP2);
            size_t i3 = s.find(strFooterSep);
            if (i3 != std::wstring::npos) {
                xg_strNotes = s.substr(i3 + strFooterSep.size());
                s = s.substr(0, i3);
            }

            LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
            if (xg_strNotes.find(psz) == 0) {
                xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
                xg_str_trim(xg_strNotes);
            }

            // ヒントの前後の空白を取り除く。
            xg_str_trim(s);

            // ヒント文字列を設定する。
            if (!s.empty()) {
                xg_strHints = s;
                xg_strHints += xg_pszNewLine;
            }
        } else {
            // ヒントがない。
            xg_strHints.clear();
        }

        // ヒント文字列を解析する。
        if (xg_strHints.empty() || !XgParseHintsStr(xg_strHints)) {
            // 失敗した。
            xg_strHints.clear();
            xg_vecTateHints.clear();
            xg_vecYokoHints.clear();
        } else {
            // カギに単語が書かれていなかった場合の処理。
            for (auto& hint : xg_vecTateHints) {
                if (hint.m_strWord.size())
                    continue;

                for (const auto& info : xg_vTateInfo) {
                    if (info.m_number == hint.m_number) {
                        std::wstring word;
                        for (int k = info.m_iRow; k < xg_nRows; ++k) {
                            WCHAR ch = xword.GetAt(k, info.m_jCol);
                            if (ch == ZEN_BLACK)
                                break;
                            word += ch;
                        }
                        hint.m_strWord = std::move(word);
                        break;
                    }
                }
            }
            for (auto& hint : xg_vecYokoHints) {
                if (hint.m_strWord.size())
                    continue;

                for (const auto& info : xg_vYokoInfo) {
                    if (info.m_number == hint.m_number) {
                        std::wstring word;
                        for (int k = info.m_jCol; k < xg_nCols; ++k) {
                            WCHAR ch = xword.GetAt(info.m_iRow, k);
                            if (ch == ZEN_BLACK)
                                break;
                            word += ch;
                        }
                        hint.m_strWord = std::move(word);
                        break;
                    }
                }
            }
        }

        // ヒントがあるか？
        if (xg_strHints.empty()) {
            // ヒントがなかった。解ではない。
            xg_xword = xword;
            xg_bSolved = false;
            xg_bShowAnswer = false;
        } else {
            // 空きマスがなく、ヒントがあった。これは解である。
            xg_solution = xword;
            xg_bSolved = true;
            xg_bShowAnswer = false;

            xg_xword.clear();
            for (int i = 0; i < xg_nRows; i++) {
                for (int j = 0; j < xg_nCols; j++) {
                    // 解に合わせて、問題に黒マスを置く。
                    if (xword.GetAt(i, j) == ZEN_BLACK)
                        xg_xword.SetAt(i, j, ZEN_BLACK);
                }
            }

            // 番号付けを行う。
            xg_solution.DoNumberingNoCheck();
        }
    } else {
        // 空きマスがあった。解ではない。
        xg_xword = xword;
        xg_bSolved = false;
        xg_bShowAnswer = false;
    }

    // ヒント追加フラグをクリアする。
    xg_bHintsAdded = false;

    // マーク文字列を読み込む。
    XgSetStringOfMarks(str.data());

    return true;
}

bool __fastcall XgSetXDString(HWND hwnd, const std::wstring& str)
{
    std::wstring header, notes;
    std::vector<std::wstring> lines, rows, clues, marks;
    XG_VIEW_MODE view_mode = xg_nViewMode;

    int iSection = 0;
    int cEmpty = 0;
    mstr_split(lines, str, L"\n");
    for (auto& line : lines) {
        xg_str_trim(line);

        if (line.empty() && iSection < 3) {
            if (cEmpty == 1) {
                ++iSection;
            }
            ++cEmpty;
        } else {
            cEmpty = 0;
            switch (iSection) {
            case 0:
                header += line;
                header += L"\r\n";
                break;
            case 1:
                rows.push_back(line);
                break;
            case 2:
                clues.push_back(line);
                break;
            case 3:
                if (line.find(L"MARK") == 0 && L'0' <= line[4] && line[4] <= L'9') {
                    marks.push_back(line);
                } else if (line.find(L"Box: ") == 0) {
                    XgLoadXdBox(line);
                } else if (line.find(L"ViewMode:") == 0) {
                    view_mode = static_cast<XG_VIEW_MODE>(_wtoi(&line[9]));
                    switch (view_mode) {
                    case XG_VIEW_NORMAL:
                    case XG_VIEW_SKELETON:
                        break;
                    default:
                        if (XgIsUserJapanese())
                            view_mode = XG_VIEW_NORMAL;
                        else
                            view_mode = XG_VIEW_SKELETON;
                    }
                } else {
                    notes += line;
                    notes += L"\r\n";
                }
                break;
            }
        }
    }

    if (header.empty())
        return false;
    if (rows.size() <= 1 || rows[0].size() <= 1)
        return false;
    for (auto& line : rows) {
        if (line.size() != rows[0].size())
            return false;
    }

    for (auto& line : rows) {
        for (auto& ch : line) {
            if (ch == L' ' || ch == L'_' || ch == ZEN_UNDERLINE)
                ch = ZEN_SPACE;
            else if (ch == L'#' || ch == L'.' || ch == ZEN_BLACK)
                ch = ZEN_BLACK;
        }
        line = XgNormalizeString(line);
    }

    bool bOK = false;
    XG_Board xword;
    std::vector<XG_Hint> tate, yoko;
    {
        std::wstring str;
        int i, nWidth = INT(rows[0].size());

        str += ZEN_ULEFT;
        for (i = 0; i < nWidth; ++i) {
            str += ZEN_HLINE;
        }
        str += ZEN_URIGHT;
        str += L"\r\n";

        for (auto& item : rows) {
            str += ZEN_VLINE;
            for (i = 0; i < nWidth; ++i) {
                str += item[i];
            }
            str += ZEN_VLINE;
            str += L"\r\n";
        }

        str += ZEN_LLEFT;
        for (i = 0; i < nWidth; ++i) {
            str += ZEN_HLINE;
        }
        str += ZEN_LRIGHT;
        str += L"\r\n";

        // 文字列を読み込む。
        bOK = xword.SetString(str);
    }

    if (!bOK)
        return false;

    xg_str_trim(header);
    xg_strHeader = header;
    xg_str_trim(notes);
    xg_strNotes = notes;
    xg_nCols = INT(rows[0].size());
    xg_nRows = INT(rows.size());
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    xg_bSolved = false;
    xg_nViewMode = view_mode;

    if (xword.IsFulfilled() && clues.size()) {
        // 番号付けを行う。
        xword.DoNumberingNoCheck();

        for (auto& str : clues) {
            xg_str_trim(str);
            if (str.empty() || (str[0] != L'A' && str[0] != L'D'))
                break;
            size_t iDot = str.find(L'.');
            size_t iTilda = str.rfind(L'~');
            if (iDot != str.npos && iTilda != str.npos) {
                auto word = str.substr(iTilda + 1);
                xg_str_trim(word);
                auto hint = str.substr(iDot + 1, iTilda - (iDot + 1));
                xg_str_trim(hint);
                word = XgNormalizeString(word);
                for (XG_PlaceInfo& item : xg_vTateInfo) {
                    if (item.m_word == word) {
                        tate.emplace_back(item.m_number, word, hint);
                        break;
                    }
                }
                for (XG_PlaceInfo& item : xg_vYokoInfo) {
                    if (item.m_word == word) {
                        yoko.emplace_back(item.m_number, word, hint);
                        break;
                    }
                }
            }
        }

        // 不足分を追加。
        for (XG_PlaceInfo& item : xg_vYokoInfo) {
            bool found = false;
            for (auto& info : yoko) {
                if (item.m_number == info.m_number) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                yoko.emplace_back(item.m_number, item.m_word, L"");
            }
        }
        for (XG_PlaceInfo& item : xg_vTateInfo) {
            bool found = false;
            for (auto& info : tate) {
                if (item.m_number == info.m_number) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                tate.emplace_back(item.m_number, item.m_word, L"");
            }
        }

        // ソート。
        std::sort(tate.begin(), tate.end(),
            [](const XG_Hint& a, const XG_Hint& b) {
                return a.m_number < b.m_number;
            }
        );
        std::sort(yoko.begin(), yoko.end(),
            [](const XG_Hint& a, const XG_Hint& b) {
                return a.m_number < b.m_number;
            }
        );

        if (tate.size() && yoko.size()) {
            // 成功。
            xg_xword = xword;
            xg_solution = xword;
            if (tate.size() && yoko.size()) {
                xg_bSolved = true;
                xg_bShowAnswer = false;
                XgClearNonBlocks();
                xg_vecTateHints = tate;
                xg_vecYokoHints = yoko;
            }

            for (auto& mark : marks) {
                if (mark.substr(0, 4) == L"MARK" && L'0' <= mark[4] && mark[4] <= L'9') {
                    size_t i0 = mark.find(L'(');
                    size_t i1 = mark.find(L", ");
                    int x = _wtoi(&mark[i0 + 1]) - 1;
                    int y = _wtoi(&mark[i1 + 2]) - 1;
                    XgSetMark(XG_Pos(y, x));
                }
            }
            return true;
        }
    }

    // 成功。
    xg_xword = xword;
    xg_solution.ResetAndSetSize(xg_nRows, xg_nCols);
    xg_bSolved = false;
    xg_bShowAnswer = false;
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();
    return true;
}

// 文字列を設定する。
bool __fastcall XgSetString(HWND hwnd, const std::wstring& str, XG_FILETYPE type)
{
    // ボックスを削除する。
    XgDeleteBoxes();

    switch (type) {
    case XG_FILETYPE_XWD:
        return XgSetStdString(hwnd, str);
    case XG_FILETYPE_XWJ:
        return XgSetJsonString(hwnd, str);
    case XG_FILETYPE_CRP:
        assert(0);
        return false;
    case XG_FILETYPE_XD:
        return XgSetXDString(hwnd, str);
    default:
        return (XgSetXDString(hwnd, str) || XgSetJsonString(hwnd, str) ||
                XgSetStdString(hwnd, str));
    }
}

// スレッド情報を取得する。
XG_ThreadInfo *__fastcall XgGetThreadInfo(void)
{
    const DWORD threadid = ::GetCurrentThreadId();
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        if (xg_aThreadInfo[i].m_threadid == threadid)
            return &xg_aThreadInfo[i];
    }
    return nullptr;
}

// 再帰する。
void __fastcall XgSolveXWord_AddBlackRecurse(const XG_Board& xw)
{
    // すでに解かれているなら、終了。
    if (xg_bSolved)
        return;

    // スレッド情報を取得する。
    XG_ThreadInfo *info = XgGetThreadInfo();
    if (info == nullptr)
        return;

    // 空ではないマスの個数をセットする。
    info->m_count = xw.Count();

    // キャンセルされているなら、終了。
    // 再計算すべきなら、終了する。
    if (xg_bCancelled)
        return;

    // 無効であれば、終了。
    if (!xw.IsValid())
        return;

    const int nRows = xg_nRows, nCols = xg_nCols;

    // 各行について、横向きにスキャンする。
    const int nColsMinusOne = nCols - 1;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nColsMinusOne; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白と文字が隣り合っているか？
            WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                int lo, hi;

                // 文字が置ける区間[lo, hi]を求める。
                lo = hi = j;
                while (lo > 0) {
                    if (xw.GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (hi + 1 < nCols) {
                    if (xw.GetAt(i, hi + 1) == ZEN_BLACK)
                        break;
                    hi++;
                }

                // パターンを生成する。
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(i, k);
                }

                const bool left_black_check = (lo != 0);
                const bool right_black_check = (hi + 1 != nCols);

                // パターンにマッチする候補を取得する。
                int nSkip;
                std::vector<std::wstring> cands;
                if (XgGetCandidatesAddBlack<false>(cands, pattern, nSkip,
                                                   left_black_check, right_black_check))
                {
                    // 候補の一部をかき混ぜる。
                    auto it = cands.begin();
                    std::advance(it, nSkip);
                    xg_random_shuffle(it, cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        bool bCanPutBlack = true;
                        for (int k = lo; k <= hi; k++) {
                            if (cand[k - lo] == ZEN_BLACK && !xw.CanPutBlack(i, k)) {
                                bCanPutBlack = false;
                                break;
                            }
                        }
                        if (bCanPutBlack) {
                            // 候補を適用して再帰する。
                            XG_Board copy(xw);

                            assert(cand.size() == pattern.size());

                            for (int k = lo; k <= hi; k++) {
                                assert(copy.GetAt(i, k) == ZEN_SPACE ||
                                       copy.GetAt(i, k) == cand[k - lo]);

                                if (cand[k - lo] == ZEN_BLACK && !copy.CanPutBlack(i, k)) {
                                    bCanPutBlack = false;
                                    break;
                                }

                                copy.SetAt(i, k, cand[k - lo]);
                            }

                            if (!bCanPutBlack) {
                                continue;
                            }

                            // 再帰する。
                            XgSolveXWord_AddBlackRecurse(copy);
                        }

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                if (XgGetCandidatesAddBlack<true>(cands, pattern, nSkip,
                                                  left_black_check, right_black_check))
                {
                    // 候補の一部をかき混ぜる。
                    auto it = cands.begin();
                    std::advance(it, nSkip);
                    xg_random_shuffle(it, cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        bool bCanPutBlack = true;
                        for (int k = lo; k <= hi; k++) {
                            if (cand[k - lo] == ZEN_BLACK && !xw.CanPutBlack(i, k)) {
                                bCanPutBlack = false;
                                break;
                            }
                        }
                        if (bCanPutBlack) {
                            // 候補を適用して再帰する。
                            XG_Board copy(xw);

                            assert(cand.size() == pattern.size());

                            for (int k = lo; k <= hi; k++) {
                                assert(copy.GetAt(i, k) == ZEN_SPACE ||
                                       copy.GetAt(i, k) == cand[k - lo]);

                                if (cand[k - lo] == ZEN_BLACK && !copy.CanPutBlack(i, k)) {
                                    bCanPutBlack = false;
                                    break;
                                }

                                copy.SetAt(i, k, cand[k - lo]);
                            }

                            if (!bCanPutBlack) {
                                continue;
                            }

                            // 再帰する。
                            XgSolveXWord_AddBlackRecurse(copy);
                        }

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                return;
            }
        }
    }

    // 各列について、縦向きにスキャンする。
    const int nRowsMinusOne = nRows - 1;
    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRowsMinusOne; i++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白と文字が隣り合っているか？
            WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i + 1, j);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                int lo, hi;

                // 文字が置ける区間[lo, hi]を求める。
                lo = hi = i;
                while (lo > 0) {
                    if (xw.GetAt(lo - 1, j) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (hi + 1 < nRows) {
                    if (xw.GetAt(hi + 1, j) == ZEN_BLACK)
                        break;
                    hi++;
                }

                // パターンを生成する。
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(k, j);
                }

                const bool left_black_check = (lo != 0);
                const bool right_black_check = (hi + 1 != nRows);

                // パターンにマッチする候補を取得する。
                int nSkip;
                std::vector<std::wstring> cands;
                if (XgGetCandidatesAddBlack<false>(cands, pattern, nSkip,
                                                   left_black_check, right_black_check))
                {
                    // 候補の一部をかき混ぜる。
                    auto it = cands.begin();
                    std::advance(it, nSkip);
                    xg_random_shuffle(it, cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        bool bCanPutBlack = true;
                        for (int k = lo; k <= hi; k++) {
                            if (cand[k - lo] == ZEN_BLACK && !xw.CanPutBlack(k, j)) {
                                bCanPutBlack = false;
                                break;
                            }
                        }
                        if (bCanPutBlack) {
                            // 候補を適用して再帰する。
                            XG_Board copy(xw);

                            assert(cand.size() == pattern.size());

                            for (int k = lo; k <= hi; k++) {
                                assert(copy.GetAt(k, j) == ZEN_SPACE ||
                                       copy.GetAt(k, j) == cand[k - lo]);

                                if (cand[k - lo] == ZEN_BLACK && !copy.CanPutBlack(k, j)) {
                                    bCanPutBlack = false;
                                    break;
                                }

                                copy.SetAt(k, j, cand[k - lo]);
                            }

                            if (!bCanPutBlack) {
                                continue;
                            }

                            // 再帰する。
                            XgSolveXWord_AddBlackRecurse(copy);
                        }

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                if (XgGetCandidatesAddBlack<true>(cands, pattern, nSkip,
                                                  left_black_check, right_black_check))
                {
                    // 候補の一部をかき混ぜる。
                    auto it = cands.begin();
                    std::advance(it, nSkip);
                    xg_random_shuffle(it, cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        bool bCanPutBlack = true;
                        for (int k = lo; k <= hi; k++) {
                            if (cand[k - lo] == ZEN_BLACK && !xw.CanPutBlack(k, j)) {
                                bCanPutBlack = false;
                                break;
                            }
                        }
                        if (bCanPutBlack) {
                            // 候補を適用して再帰する。
                            XG_Board copy(xw);

                            assert(cand.size() == pattern.size());

                            for (int k = lo; k <= hi; k++) {
                                assert(copy.GetAt(k, j) == ZEN_SPACE ||
                                       copy.GetAt(k, j) == cand[k - lo]);

                                if (cand[k - lo] == ZEN_BLACK && !copy.CanPutBlack(k, j)) {
                                    bCanPutBlack = false;
                                    break;
                                }

                                copy.SetAt(k, j, cand[k - lo]);
                            }

                            if (!bCanPutBlack) {
                                continue;
                            }

                            // 再帰する。
                            XgSolveXWord_AddBlackRecurse(copy);
                        }

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                return;
            }
        }
    }

    // 解かどうか？
    EnterCriticalSection(&xg_cs);
    bool ok = xw.IsSolution();
    ::LeaveCriticalSection(&xg_cs);
    if (ok) {
        // 解だった。
        ::EnterCriticalSection(&xg_cs);
        xg_bSolved = true;
        xg_solution = xw;
        xg_solution.DoNumbering();

        // 解に合わせて、問題に黒マスを置く。
        const int nCount = nRows * nCols;
        for (int i = 0; i < nCount; i++) {
            if (xw.GetAt(i) == ZEN_BLACK)
                xg_xword.SetAt(i, ZEN_BLACK);
        }
        ::LeaveCriticalSection(&xg_cs);
    }
}

// 再帰する（黒マス追加なし）。
void __fastcall XgSolveXWord_NoAddBlackRecurse(const XG_Board& xw)
{
    // すでに解かれているなら、終了。
    if (xg_bSolved)
        return;

    // スレッド情報を取得する。
    XG_ThreadInfo *info = XgGetThreadInfo();
    if (info == nullptr)
        return;

    // 空ではないマスの個数をセットする。
    info->m_count = xw.Count();

    // キャンセルされているなら、終了。
    // 再計算すべきなら、終了する。
    if (xg_bCancelled)
        return;

    // 無効であれば、終了。
    if (!xw.IsNoAddBlackOK())
        return;

    const int nRows = xg_nRows, nCols = xg_nCols;

    // 各行について、横向きにスキャンする。
    const int nColsMinusOne = nCols - 1;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nColsMinusOne; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白と文字が隣り合っているか？
            WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                int lo, hi;

                // 文字が置ける区間[lo, hi]を求める。
                lo = hi = j;
                while (lo > 0) {
                    if (xw.GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (hi + 1 < nCols) {
                    if (xw.GetAt(i, hi + 1) == ZEN_BLACK)
                        break;
                    hi++;
                }

                // パターンを生成する。
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(i, k);
                }

                // パターンにマッチする候補を取得する（黒マス追加なし）。
                std::vector<std::wstring> cands;
                if (XgGetCandidatesNoAddBlack<false>(cands, pattern)) {
                    // 候補をかき混ぜる。
                    xg_random_shuffle(cands.begin(), cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        // 候補を適用して再帰する。
                        XG_Board copy(xw);
                        assert(cand.size() == pattern.size());
                        for (int k = lo; k <= hi; k++) {
                            assert(copy.GetAt(i, k) == ZEN_SPACE ||
                                   copy.GetAt(i, k) == cand[k - lo]);
                            copy.SetAt(i, k, cand[k - lo]);
                        }

                        // 再帰する。
                        XgSolveXWord_NoAddBlackRecurse(copy);

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                if (XgGetCandidatesNoAddBlack<true>(cands, pattern)) {
                    // 候補をかき混ぜる。
                    xg_random_shuffle(cands.begin(), cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        // 候補を適用して再帰する。
                        XG_Board copy(xw);
                        assert(cand.size() == pattern.size());
                        for (int k = lo; k <= hi; k++) {
                            assert(copy.GetAt(i, k) == ZEN_SPACE ||
                                   copy.GetAt(i, k) == cand[k - lo]);
                            copy.SetAt(i, k, cand[k - lo]);
                        }

                        // 再帰する。
                        XgSolveXWord_NoAddBlackRecurse(copy);

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                return;
            }
        }
    }

    // 各列について、縦向きにスキャンする。
    const int nRowsMinusOne = nRows - 1;
    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRowsMinusOne; i++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白と文字が隣り合っているか？
            WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i + 1, j);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                int lo, hi;

                // 文字が置ける区間[lo, hi]を求める。
                lo = hi = i;
                while (lo > 0) {
                    if (xw.GetAt(lo - 1, j) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (hi + 1 < nRows) {
                    if (xw.GetAt(hi + 1, j) == ZEN_BLACK)
                        break;
                    hi++;
                }

                // パターンを生成する。
                std::wstring pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(k, j);
                }

                // パターンにマッチする候補を取得する。
                std::vector<std::wstring> cands;
                if (XgGetCandidatesNoAddBlack<false>(cands, pattern)) {
                    // 候補をかき混ぜる。
                    xg_random_shuffle(cands.begin(), cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        // 候補を適用して再帰する。
                        XG_Board copy(xw);
                        assert(cand.size() == pattern.size());
                        for (int k = lo; k <= hi; k++) {
                            assert(copy.GetAt(k, j) == ZEN_SPACE ||
                                   copy.GetAt(k, j) == cand[k - lo]);
                            copy.SetAt(k, j, cand[k - lo]);
                        }

                        // 再帰する。
                        XgSolveXWord_NoAddBlackRecurse(copy);

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                if (XgGetCandidatesNoAddBlack<false>(cands, pattern)) {
                    // 候補をかき混ぜる。
                    xg_random_shuffle(cands.begin(), cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled)
                            return;

                        // 候補を適用して再帰する。
                        XG_Board copy(xw);
                        assert(cand.size() == pattern.size());
                        for (int k = lo; k <= hi; k++) {
                            assert(copy.GetAt(k, j) == ZEN_SPACE ||
                                   copy.GetAt(k, j) == cand[k - lo]);
                            copy.SetAt(k, j, cand[k - lo]);
                        }

                        // 再帰する。
                        XgSolveXWord_NoAddBlackRecurse(copy);

                        // 空ではないマスの個数をセットする。
                        info->m_count = xw.Count();
                    }
                }
                return;
            }
        }
    }

    // 解かどうか？
    ::EnterCriticalSection(&xg_cs);
    bool ok = xw.IsSolution();
    ::LeaveCriticalSection(&xg_cs);
    if (ok) {
        // 解だった。
        xg_bSolved = true;
        ::EnterCriticalSection(&xg_cs);
        xg_solution = xw;
        xg_solution.DoNumbering();

        // 解に合わせて、問題に黒マスを置く。
        const int nCount = nRows * nCols;
        for (int i = 0; i < nCount; i++) {
            if (xw.GetAt(i) == ZEN_BLACK)
                xg_xword.SetAt(i, ZEN_BLACK);
        }
        ::LeaveCriticalSection(&xg_cs);
    }
}

// 縦と横を入れ替える。
void XG_Board::SwapXandY()
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;

    std::vector<WCHAR> vCells;
    vCells.assign(nRows * nCols + 1, ZEN_SPACE);

    for (int i = 0; i < nRows; ++i) {
        for (int j = 0; j < nCols; ++j) {
            vCells[j * nRows + i] = m_vCells[i * nCols + j];
        }
    }
    vCells[nRows * nCols] = Count();
    m_vCells = vCells;
}

// 解く。
void __fastcall XgSolveXWord_AddBlack(const XG_Board& xw)
{
    const int nRows = xg_nRows, nCols = xg_nCols;

    // 文字マスがあるか？
    bool bCharFound = false;
    for (int i = 0; i < nRows * nCols; i++) {
        const WCHAR ch1 = xw.GetAt(i);
        if (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK)
        {
            bCharFound = true;
        }
    }
    if (bCharFound) {
        // 文字マスがあった場合は、そのまま解く。
        XgSolveXWord_AddBlackRecurse(xw);
        return;
    }

    // 無効であれば、終了。
    if (!xw.IsValid())
        return;

    // ランダムな順序の単語ベクターを作成する。
    std::vector<XG_WordData> words(xg_dict_1);
    xg_random_shuffle(words.begin(), words.end());

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白の連続があるか？
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if (ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (xw.GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (xw.GetAt(i, j + 1) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = j;

                // パターンの長さを求める。
                const int patlen = hi - lo + 1;
                for (const auto& word_data : words) {
                    // すでに解かれているなら、終了。
                    // キャンセルされているなら、終了。
                    // 再計算すべきなら、終了する。
                    if (xg_bSolved || xg_bCancelled)
                        return;

                    // 単語の長さがパターンの長さ以下か？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen > patlen)
                        continue;

                    // 必要な黒マスは置けるか？
                    if ((lo == 0 || xw.CanPutBlack(i, lo - 1)) &&
                        (hi + 1 >= nCols || xw.CanPutBlack(i, hi + 1)))
                    {
                        // 単語とその両端の外側の黒マスをセットして再帰する。
                        XG_Board copy(xw);
                        for (int k = 0; k < wordlen; k++) {
                            copy.SetAt(i, lo + k, word[k]);
                        }

                        if (lo > 0 && copy.GetAt(i, lo - 1) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(i, lo - 1))
                                continue;

                            copy.SetAt(i, lo - 1, ZEN_BLACK);
                        }
                        if (hi + 1 < nCols && copy.GetAt(i, hi + 1) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(i, hi + 1))
                                continue;

                            copy.SetAt(i, hi + 1, ZEN_BLACK);
                        }

                        //if (copy.TriBlackAround()) {
                        //    continue;
                        //}

                        // 再帰する。
                        XgSolveXWord_AddBlackRecurse(copy);
                    }
                }
                goto retry_1;
            }
        }
    }

    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRows - 1; i++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白の連続があるか？
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i + 1, j);
            if (ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (xw.GetAt(lo - 1, j) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (i + 1 < nRows) {
                    if (xw.GetAt(i + 1, j) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = i;

                // パターンの長さを求める。
                const int patlen = hi - lo + 1;
                for (const auto& word_data : words) {
                    // すでに解かれているなら、終了。
                    // キャンセルされているなら、終了。
                    // 再計算すべきなら、終了する。
                    if (xg_bSolved || xg_bCancelled)
                        return;

                    // 単語の長さがパターンの長さ以下か？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen > patlen)
                        continue;

                    // 必要な黒マスは置けるか？
                    if ((lo == 0 || xw.CanPutBlack(lo - 1, j)) &&
                        (hi + 1 >= nRows || xw.CanPutBlack(hi + 1, j)))
                    {
                        // 単語とその両端の外側の黒マスをセットして再帰する。
                        XG_Board copy(xw);
                        for (int k = 0; k < wordlen; k++) {
                            copy.SetAt(lo + k, j, word[k]);
                        }

                        if (lo > 0 && copy.GetAt(lo - 1, j) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(lo - 1, j))
                                continue;

                            copy.SetAt(lo - 1, j, ZEN_BLACK);
                        }
                        if (hi + 1 < nRows && copy.GetAt(hi + 1, j) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(hi + 1, j))
                                continue;

                            copy.SetAt(hi + 1, j, ZEN_BLACK);
                        }

                        //if (copy.TriBlackAround()) {
                        //    continue;
                        //}

                        // 再帰する。
                        XgSolveXWord_AddBlackRecurse(copy);
                    }
                }
                goto retry_1;
            }
        }
    }

    // ランダムな順序の単語ベクターを作成する。
retry_1:;
    words = xg_dict_2;
    xg_random_shuffle(words.begin(), words.end());

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白の連続があるか？
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if (ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (xw.GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (xw.GetAt(i, j + 1) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = j;

                // パターンの長さを求める。
                const int patlen = hi - lo + 1;
                for (const auto& word_data : words) {
                    // すでに解かれているなら、終了。
                    // キャンセルされているなら、終了。
                    // 再計算すべきなら、終了する。
                    if (xg_bSolved || xg_bCancelled)
                        return;

                    // 単語の長さがパターンの長さ以下か？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen > patlen)
                        continue;

                    // 必要な黒マスは置けるか？
                    if ((lo == 0 || xw.CanPutBlack(i, lo - 1)) &&
                        (hi + 1 >= nCols || xw.CanPutBlack(i, hi + 1)))
                    {
                        // 単語とその両端の外側の黒マスをセットして再帰する。
                        XG_Board copy(xw);
                        for (int k = 0; k < wordlen; k++) {
                            copy.SetAt(i, lo + k, word[k]);
                        }

                        if (lo > 0 && copy.GetAt(i, lo - 1) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(i, lo - 1))
                                continue;

                            copy.SetAt(i, lo - 1, ZEN_BLACK);
                        }
                        if (hi + 1 < nCols && copy.GetAt(i, hi + 1) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(i, hi + 1))
                                continue;

                            copy.SetAt(i, hi + 1, ZEN_BLACK);
                        }

                        //if (copy.TriBlackAround()) {
                        //    continue;
                        //}

                        // 再帰する。
                        XgSolveXWord_AddBlackRecurse(copy);
                    }
                }
                goto retry_2;
            }
        }
    }

    for (int j = 0; j < nCols; j++) {
        for (int i = 0; i < nRows - 1; i++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白の連続があるか？
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i + 1, j);
            if (ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (xw.GetAt(lo - 1, j) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (i + 1 < nRows) {
                    if (xw.GetAt(i + 1, j) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = i;

                // パターンの長さを求める。
                const int patlen = hi - lo + 1;
                for (const auto& word_data : words) {
                    // すでに解かれているなら、終了。
                    // キャンセルされているなら、終了。
                    // 再計算すべきなら、終了する。
                    if (xg_bSolved || xg_bCancelled)
                        return;

                    // 単語の長さがパターンの長さ以下か？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen > patlen)
                        continue;

                    // 必要な黒マスは置けるか？
                    if ((lo == 0 || xw.CanPutBlack(lo - 1, j)) &&
                        (hi + 1 >= nRows || xw.CanPutBlack(hi + 1, j)))
                    {
                        // 単語とその両端の外側の黒マスをセットして再帰する。
                        XG_Board copy(xw);
                        for (int k = 0; k < wordlen; k++) {
                            copy.SetAt(lo + k, j, word[k]);
                        }

                        if (lo > 0 && copy.GetAt(lo - 1, j) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(lo - 1, j))
                                continue;

                            copy.SetAt(lo - 1, j, ZEN_BLACK);
                        }
                        if (hi + 1 < nRows && copy.GetAt(hi + 1, j) != ZEN_BLACK) {
                            if (!copy.CanPutBlack(hi + 1, j))
                                continue;

                            copy.SetAt(hi + 1, j, ZEN_BLACK);
                        }

                        //if (copy.TriBlackAround()) {
                        //    continue;
                        //}

                        // 再帰する。
                        XgSolveXWord_AddBlackRecurse(copy);
                    }
                }
                goto retry_2;
            }
        }
    }
retry_2:;
}

// 解く（黒マス追加なし）。
void __fastcall XgSolveXWord_NoAddBlack(const XG_Board& xw)
{
    const int nRows = xg_nRows, nCols = xg_nCols;

    // 文字マスがあるか？
    bool bCharFound = false;
    for (int i = 0; i < nRows * nCols; i++) {
        const WCHAR ch1 = xw.GetAt(i);
        if (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK) {
            bCharFound = true;
        }
    }
    if (bCharFound) {
        // 文字マスがあった場合は、そのまま解く。
        XgSolveXWord_NoAddBlackRecurse(xw);
        return;
    }

    // 無効であれば、終了。
    if (!xw.IsNoAddBlackOK())
        return;

    // ランダムな順序の単語ベクターを作成する。
    std::vector<XG_WordData> words(xg_dict_1);
    xg_random_shuffle(words.begin(), words.end());

    // 文字マスがなかった場合。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白の連続があるか？
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if (ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (xw.GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (xw.GetAt(i, j + 1) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = j;

                // パターンの長さを求める。
                const int patlen = hi - lo + 1;
                for (const auto& word_data : words) {
                    // すでに解かれているなら、終了。
                    // キャンセルされているなら、終了。
                    // 再計算すべきなら、終了する。
                    if (xg_bSolved || xg_bCancelled)
                        return;

                    // 単語とパターンの長さが等しいか？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen != patlen)
                        continue;

                    // 単語をセットする。
                    XG_Board copy(xw);
                    for (int k = 0; k < wordlen; k++) {
                        copy.SetAt(i, lo + k, word[k]);
                    }

                    // 再帰する。
                    XgSolveXWord_NoAddBlackRecurse(copy);
                }
                goto retry_1;
            }
        }
    }

    // ランダムな順序の単語ベクターを作成する。
retry_1:;
    words = xg_dict_2;
    xg_random_shuffle(words.begin(), words.end());

    // 文字マスがなかった場合。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled)
                return;

            // 空白の連続があるか？
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if (ch1 == ZEN_SPACE && ch2 == ZEN_SPACE) {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (xw.GetAt(i, lo - 1) == ZEN_BLACK)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (xw.GetAt(i, j + 1) == ZEN_BLACK)
                        break;
                    j++;
                }
                const int hi = j;

                // パターンの長さを求める。
                const int patlen = hi - lo + 1;
                for (const auto& word_data : words) {
                    // すでに解かれているなら、終了。
                    // キャンセルされているなら、終了。
                    // 再計算すべきなら、終了する。
                    if (xg_bSolved || xg_bCancelled)
                        return;

                    // 単語とパターンの長さが等しいか？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen != patlen)
                        continue;

                    // 単語をセットする。
                    XG_Board copy(xw);
                    for (int k = 0; k < wordlen; k++) {
                        copy.SetAt(i, lo + k, word[k]);
                    }

                    // 再帰する。
                    XgSolveXWord_NoAddBlackRecurse(copy);
                }
                goto retry_2;
            }
        }
    }

retry_2:;
}

#ifdef NO_RANDOM
    int xg_random_seed = 0;
#endif

// マルチスレッド用の関数。
unsigned __stdcall XgSolveProc_AddBlack(void *param)
{
    // スレッド情報を取得する。
    XG_ThreadInfo *info = reinterpret_cast<XG_ThreadInfo *>(param);

    // 空ではないマスの個数をセットする。
    info->m_count = xg_xword.Count();

    // スレッドの優先度を上げる。
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    // 乱数生成ルーチンを初期化する。
    #ifndef NO_RANDOM
        srand(::GetTickCount() ^ info->m_threadid);
    #else
        srand(xg_random_seed++);
    #endif

    // 解く。
    XgSolveXWord_AddBlack(xg_xword);
    return 0;
}

// マルチスレッド用の関数（黒マス追加なし）。
unsigned __stdcall XgSolveProc_NoAddBlack(void *param)
{
    // スレッド情報を取得する。
    XG_ThreadInfo *info = reinterpret_cast<XG_ThreadInfo *>(param);

    // 空ではないマスの個数をセットする。
    info->m_count = xg_xword.Count();

    // スレッドの優先度を上げる。
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    // 乱数生成ルーチンを初期化する。
    #ifndef NO_RANDOM
        srand(::GetTickCount() ^ info->m_threadid);
    #else
        srand(xg_random_seed++);
    #endif

    // 解く（黒マス追加なし）。
    XgSolveXWord_NoAddBlack(xg_xword);
    return 0;
}

// マルチスレッド用の関数（スマート解決）。
unsigned __stdcall XgSolveProcSmart(void *param)
{
    // スレッド情報を取得する。
    XG_ThreadInfo *info = reinterpret_cast<XG_ThreadInfo *>(param);

    // 空ではないマスの個数をセットする。
    info->m_count = xg_xword.Count();

    // スレッドの優先度を上げる。
    ::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    // 乱数生成ルーチンを初期化する。
    #ifndef NO_RANDOM
        srand(::GetTickCount() ^ info->m_threadid);
    #else
        srand(xg_random_seed++);
    #endif

    // 黒マスを生成する。
    XgGenerateBlacksSmart(NULL);

    // 空ではないマスの個数をセットする。
    info->m_count = xg_xword.Count();

    // 解く（黒マス追加なし）。
    XgSolveXWord_NoAddBlack(xg_xword);
    return 0;
}

//#define SINGLE_THREAD_MODE

// 解を求めるのを開始。
void __fastcall XgStartSolve_AddBlack(void)
{
    // フラグを初期化する。
    xg_bSolved = xg_bCancelled = false;

    if (xg_bSolvingEmpty)
        xg_xword.clear();

    // 横より縦の方が長い場合、計算時間を減らすために、
    // 縦と横を入れ替え、後でもう一度縦と横を入れ替える。
    if (xg_nRows > xg_nCols) {
        s_bSwapped = true;
        xg_xword.SwapXandY();
        std::swap(xg_nRows, xg_nCols);
    }

    // 最大長を制限する。
    if (xg_nMaxWordLen > xg_nDictMaxWordLen) {
        xg_nMaxWordLen = xg_nDictMaxWordLen;
    }

#ifdef SINGLE_THREAD_MODE
    XgSolveProc_AddBlack(&xg_aThreadInfo[0]);
#else
    // スレッドを開始する。
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        xg_aThreadInfo[i].m_count = static_cast<DWORD>(xg_xword.Count());
        xg_ahThreads[i] = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, XgSolveProc_AddBlack, &xg_aThreadInfo[i], 0,
                &xg_aThreadInfo[i].m_threadid));
        assert(xg_ahThreads[i] != nullptr);
    }
#endif
}

// 解を求めるのを開始（黒マス追加なし）。
void __fastcall XgStartSolve_NoAddBlack(void)
{
    // フラグを初期化する。
    xg_bSolved = xg_bCancelled = false;

#ifdef SINGLE_THREAD_MODE
    XgSolveProc_NoAddBlack(&xg_aThreadInfo[0]);
#else
    // スレッドを開始する。
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        xg_aThreadInfo[i].m_count = static_cast<DWORD>(xg_xword.Count());
        xg_ahThreads[i] = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, XgSolveProc_NoAddBlack, &xg_aThreadInfo[i], 0,
                &xg_aThreadInfo[i].m_threadid));
        assert(xg_ahThreads[i] != nullptr);
    }
#endif
}

// 解を求めるのを開始（スマート解決）。
void __fastcall XgStartSolve_Smart(void)
{
    // フラグを初期化する。
    xg_bSolved = xg_bCancelled = false;

    // まだブロック生成していない。
    xg_bBlacksGenerated = FALSE;

    // 最大長を制限する。
    if (xg_nMaxWordLen > xg_nDictMaxWordLen) {
        xg_nMaxWordLen = xg_nDictMaxWordLen;
    }

#ifdef SINGLE_THREAD_MODE
    XgSolveProcSmart(&xg_aThreadInfo[0]);
#else
    // スレッドを開始する。
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        xg_aThreadInfo[i].m_count = static_cast<DWORD>(xg_xword.Count());
        xg_ahThreads[i] = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, XgSolveProcSmart, &xg_aThreadInfo[i], 0,
                &xg_aThreadInfo[i].m_threadid));
        assert(xg_ahThreads[i] != nullptr);
    }
#endif
}

// 文字をクリア。
void __fastcall XgClearNonBlocks(void)
{
    XgSetCaretPos();

    if (xg_bSolved) {
        xg_bShowAnswer = false;
    }

    for (INT i = 0; i < xg_nRows; ++i) {
        for (INT j = 0; j < xg_nCols; ++j) {
            WCHAR oldch = xg_xword.GetAt(i, j);
            if (oldch != ZEN_BLACK && oldch != ZEN_SPACE) {
                xg_xword.SetAt(i, j, ZEN_SPACE);
            }
        }
    }

    xg_prev_vk = 0;
}

// 解を求めようとした後の後処理。
void __fastcall XgEndSolve(void)
{
    if (s_bSwapped) {
        xg_xword.SwapXandY();
        if (xg_bSolved) {
            xg_solution.SwapXandY();
        }
        std::swap(xg_nRows, xg_nCols);
        if (xg_bSolved) {
            xg_solution.DoNumbering();
            if (!XgParseHintsStr(xg_strHints)) {
                xg_strHints.clear();
            }
        }
        s_bSwapped = false;
    }
}

// 二重マス単語を描画する。
void __fastcall XgDrawMarkWord(HDC hdc, LPSIZE psiz)
{
    int nCount = static_cast<int>(xg_vMarks.size());
    if (nCount == 0) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);

    // 細いペンを作成し、選択する。
    HPEN hThinPen = ::CreatePen(PS_SOLID, 1, xg_rgbBlackCellColor);

    // 太い線と黒いブラシを作成する。
    LOGBRUSH lbBlack;
    ::GetObject(hbrBlack, sizeof(lbBlack), &lbBlack);
    int c_nWide = 4;
    HPEN hWidePen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nWide, &lbBlack, 0, NULL);

    LOGFONTW lf;

    // 文字マスのフォントを作成する。
    ZeroMemory(&lf, sizeof(lf));
    StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
    if (xg_szCellFont[0])
        StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);
    lf.lfHeight = -xg_nCellSize * xg_nCellCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    HFONT hFont = ::CreateFontIndirectW(&lf);

    // 小さい文字のフォントを作成する。
    ZeroMemory(&lf, sizeof(lf));
    if (xg_szSmallFont[0])
        StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szSmallFont);
    lf.lfHeight = -xg_nCellSize * xg_nSmallCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    HFONT hFontSmall = ::CreateFontIndirectW(&lf);

    // 全体を白で塗りつぶす。
    RECT rc;
    ::SetRect(&rc, 0, 0, psiz->cx, psiz->cy);
    ::FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

    // 周りに太い線を描く。
    if (xg_bAddThickFrame) {
        InflateRect(&rc, -xg_nNarrowMargin, -xg_nNarrowMargin);
        InflateRect(&rc, +c_nWide, +c_nWide);
        FillRect(hdc, &rc, hbrBlack);
        InflateRect(&rc, -c_nWide, -c_nWide);
        InflateRect(&rc, +xg_nNarrowMargin, +xg_nNarrowMargin);
    }

    // 二重マスを描画する。
    WCHAR sz[32];
    SIZE siz;
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall);
    HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
    for (int i = 0; i < nCount; i++) {
        ::SetRect(&rc,
            static_cast<int>(xg_nNarrowMargin + i * xg_nCellSize), 
            static_cast<int>(xg_nNarrowMargin + 0 * xg_nCellSize),
            static_cast<int>(xg_nNarrowMargin + (i + 1) * xg_nCellSize - 1), 
            static_cast<int>(xg_nNarrowMargin + 1 * xg_nCellSize));
        ::FillRect(hdc, &rc, hbrMarked);
        ::InflateRect(&rc, -4, -4);
        if (xg_bDrawFrameForMarkedCell) {
            ::MoveToEx(hdc, rc.left, rc.top, NULL);
            ::LineTo(hdc, rc.right + 1, rc.top);
            ::LineTo(hdc, rc.right + 1, rc.bottom + 1);
            ::LineTo(hdc, rc.left, rc.bottom + 1);
            ::LineTo(hdc, rc.left, rc.top);
        }
        ::InflateRect(&rc, 4, 4);

        if (i < (INT)xg_strDoubleFrameLetters.size())
            StringCbPrintf(sz, sizeof(sz), L"%c", xg_strDoubleFrameLetters[i]);
        else
            StringCbPrintf(sz, sizeof(sz), L"%c", ZEN_BLACK);
        ::GetTextExtentPoint32W(hdc, sz, int(wcslen(sz)), &siz);

        RECT rcText = rc;
        rcText.left = rc.right - std::max(siz.cx, siz.cy);
        rcText.top = rc.bottom - std::max(siz.cx, siz.cy);
        ::FillRect(hdc, &rcText, hbrMarked);

        INT x = (rcText.left + rcText.right - siz.cx) / 2;
        INT y = rcText.top;

        // 二重マスの文字を描く。
        ::SetTextColor(hdc, xg_rgbBlackCellColor);
        ::SetBkMode(hdc, TRANSPARENT);
        ::TextOutW(hdc, x, y, sz, lstrlenW(sz));
    }
    ::SelectObject(hdc, hFontOld);
    ::SelectObject(hdc, hPenOld);

    // マスの文字を描画する。
    hFontOld = ::SelectObject(hdc, hFont);
    for (int i = 0; i < nCount; i++) {
        ::SetRect(&rc,
            static_cast<int>(xg_nNarrowMargin + i * xg_nCellSize), 
            static_cast<int>(xg_nNarrowMargin + 0 * xg_nCellSize),
            static_cast<int>(xg_nNarrowMargin + (i + 1) * xg_nCellSize), 
            static_cast<int>(xg_nNarrowMargin + 1 * xg_nCellSize));

        WCHAR ch;
        XG_Pos& pos = xg_vMarks[i];
        if (xg_bSolved && xg_bShowAnswer) {
            ch = xg_solution.GetAt(pos.m_i, pos.m_j);
        } else {
            ch = xg_xword.GetAt(pos.m_i, pos.m_j);
        }

        // 文字を変換する。
        if (xg_bHiragana) {
            WCHAR new_ch;
            LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch, 1, &new_ch, 1);
            ch = new_ch;
        } else {
            WCHAR new_ch;
            LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch, 1, &new_ch, 1);
            ch = new_ch;
        }

        if (XgIsCharKanaW(ch) || ch == ZEN_PROLONG ||
            xg_imode == xg_im_KANA || xg_imode == xg_im_KANJI)
        {
            WCHAR new_ch;
            if (xg_bLowercase) {
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE, &ch, 1, &new_ch, 1);
                ch = new_ch;
            } else {
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_UPPERCASE, &ch, 1, &new_ch, 1);
                ch = new_ch;
            }
        } else {
            WCHAR new_ch;
            if (xg_bLowercase) {
                LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch, 1, &new_ch, 1);
                ch = new_ch;
            } else {
                LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch, 1, &new_ch, 1);
                ch = new_ch;
            }
        }

        StringCbPrintf(sz, sizeof(sz), L"%c", ch);

        ::GetTextExtentPoint32W(hdc, sz, int(wcslen(sz)), &siz);
        INT x = (rc.left + rc.right) / 2 - siz.cx / 2;
        INT y = (rc.top + rc.bottom) / 2 - siz.cy / 2;

        // マスの文字を描画する。
        ::SetTextColor(hdc, xg_rgbBlackCellColor);
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetBkColor(hdc, xg_rgbMarkedCellColor);
        ::TextOutW(hdc, x, y, sz, lstrlenW(sz));
    }
    ::SelectObject(hdc, hFontOld);

    // 線を引く。塗りつぶさない。
    ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
    hPenOld = ::SelectObject(hdc, hThinPen);
    for (int i = 0; i < nCount; i++) {
        ::Rectangle(hdc,
                static_cast<int>(xg_nNarrowMargin + i * xg_nCellSize),
                static_cast<int>(xg_nNarrowMargin + 0 * xg_nCellSize),
                static_cast<int>(xg_nNarrowMargin + (i + 1) * xg_nCellSize + 1),
                static_cast<int>(xg_nNarrowMargin + 1 * xg_nCellSize) + 1);
    }
    ::SelectObject(hdc, hPenOld);

    // 破棄する。
    ::DeleteObject(hWidePen);
    ::DeleteObject(hThinPen);
    ::DeleteObject(hFont);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
}

std::unordered_set<XG_Pos> XgGetSlot(INT number, BOOL vertical)
{
    std::unordered_set<XG_Pos> ret;
    if (!xg_bSolved)
        return ret;

    INT i = -1, j = -1;
    if (vertical) {
        for (auto& info : xg_vTateInfo) {
            if (info.m_number == number) {
                i = info.m_iRow;
                j = info.m_jCol;
                break;
            }
        }
        if (i != -1) {
            ret.emplace(i, j);
            for (++i; i < xg_nRows; ++i) {
                if (xg_xword.GetAt(i, j) == ZEN_BLACK) {
                    break;
                }
                ret.emplace(i, j);
            }
        }
    } else {
        for (auto& info : xg_vYokoInfo) {
            if (info.m_number == number) {
                i = info.m_iRow;
                j = info.m_jCol;
                break;
            }
        }
        if (i != -1) {
            ret.emplace(i, j);
            for (++j; j < xg_nCols; ++j) {
                if (xg_xword.GetAt(i, j) == ZEN_BLACK) {
                    break;
                }
                ret.emplace(i, j);
            }
        }
    }

    return ret;
}

// ハイライト色。
const COLORREF c_rgbHighlight = RGB(255, 255, 100);
const COLORREF c_rgbHighlightAndDblFrame = RGB(255, 155, 100);

// クロスワードを描画する（通常ビュー）。
void __fastcall XgDrawXWord_NormalView(XG_Board& xw, HDC hdc, LPSIZE psiz, bool bScreen)
{
    INT nCellSize;
    if (xg_nForDisplay > 0) {
        nCellSize = xg_nCellSize * xg_nZoomRate / 100;
    } else {
        nCellSize = xg_nCellSize;
    }

    // 全体を白で塗りつぶす。
    RECT rc;
    ::SetRect(&rc, 0, 0, psiz->cx, psiz->cy);
    ::FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

    LOGFONTW lf;

    // 文字マスのフォントを作成する。
    ZeroMemory(&lf, sizeof(lf));
    // その他。
    StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
    if (xg_szCellFont[0])
        StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);
    lf.lfHeight = -nCellSize * xg_nCellCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    HFONT hFont = ::CreateFontIndirectW(&lf);

    // 小さい文字のフォントを作成する。
    ZeroMemory(&lf, sizeof(lf));
    if (xg_szSmallFont[0])
        StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szSmallFont);
    lf.lfHeight = -nCellSize * xg_nSmallCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    HFONT hFontSmall = ::CreateFontIndirectW(&lf);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);
    HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
    HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);

    auto slot = XgGetSlot(xg_highlight.m_number, xg_highlight.m_vertical);
    if (xg_nForDisplay <= 0)
        slot.clear();

    // 黒の細いペンを作成する。
    HPEN hThinPen = ::CreatePen(PS_SOLID, 1, xg_rgbBlackCellColor);

    // 赤いキャレットペンを作成する。
    LOGBRUSH lbRed;
    lbRed.lbStyle = BS_SOLID;
    lbRed.lbColor = RGB(255, 0, 0);
    HPEN hCaretPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
        1, &lbRed, 0, NULL);

    // 黒い太いペンを作成する。
    LOGBRUSH lbBlack;
    ::GetObject(hbrBlack, sizeof(lbBlack), &lbBlack);
    int c_nWide = 4;
    HPEN hWidePen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nWide, &lbBlack, 0, NULL);

    WCHAR sz[32];
    SIZE siz;
    HGDIOBJ hFontOld, hPenOld;

    BITMAP bm;
    GetObject(xg_hbmBlackCell, sizeof(bm), &bm);

    // 黒マスビットマップを選択する。
    HDC hdcMem = ::CreateCompatibleDC(NULL);
    SelectObject(hdcMem, xg_hbmBlackCell);
    SetStretchBltMode(hdcMem, STRETCH_HALFTONE);

    // セルの背景を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            // 二重マスか？
            int nMarked = XgGetMarked(i, j);

            // 塗りつぶす。
            WCHAR ch = xw.GetAt(i, j);
            if (ch == ZEN_BLACK) {
                // 黒マス。
                if (xg_hbmBlackCell) {
                    StretchBlt(hdc,
                               rc.left, rc.top,
                               rc.right - rc.left, rc.bottom - rc.top,
                               hdcMem, 0, 0, bm.bmWidth, bm.bmHeight,
                               SRCCOPY);
                } else if (xg_hBlackCellEMF) {
                    ::PlayEnhMetaFile(hdc, xg_hBlackCellEMF, &rc);
                } else {
                    ::FillRect(hdc, &rc, hbrBlack);
                }
            } else if (slot.count(XG_Pos(i, j)) > 0 && nMarked != -1) {
                // ハイライトかつ二重マス。
                ::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
            } else if (slot.count(XG_Pos(i, j)) > 0) {
                // ハイライト。
                ::FillRect(hdc, &rc, hbrHighlight);
            } else if (nMarked != -1) {
                // 二重マス。
                ::FillRect(hdc, &rc, hbrMarked);
            } else {
                // その他のマス。
                ::FillRect(hdc, &rc, hbrWhite);
            }
        }
    }

    // デバイスコンテキストを破棄する。
    ::DeleteDC(hdcMem);

    // 小さい文字のフォントを選択する。
    hFontOld = ::SelectObject(hdc, hFontSmall);

    // 二重マスを描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize),
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize) - 1,
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            // 二重マスか？
            int nMarked = XgGetMarked(i, j);
            if (nMarked == -1) {
                continue;
            }

            // 二重マスの内側の枠を描く。
            if (xg_bDrawFrameForMarkedCell) {
                ::InflateRect(&rc, -4, -4);
                ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
                hPenOld = ::SelectObject(hdc, hThinPen);
                ::Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
                ::SelectObject(hdc, hPenOld);
                ::InflateRect(&rc, 4, 4);
            }

            if (xg_bShowDoubleFrameLetters) {
                if (nMarked < (INT)xg_strDoubleFrameLetters.size())
                    StringCbPrintf(sz, sizeof(sz), L"%c", xg_strDoubleFrameLetters[nMarked]);
                else
                    StringCbPrintf(sz, sizeof(sz), L"%c", ZEN_BLACK);

                // 二重マスの右下端の文字の背景を塗りつぶす。
                RECT rcText;
                GetTextExtentPoint32(hdc, sz, lstrlen(sz), &siz);
                rcText = rc;
                rcText.left = rc.right - std::max(siz.cx, siz.cy);
                rcText.top = rc.bottom - std::max(siz.cx, siz.cy);

                HBRUSH hbr = CreateSolidBrush(xg_rgbMarkedCellColor);
                FillRect(hdc, &rcText, hbr);
                DeleteObject(hbr);

                // 二重マスの右下端の文字を描く。
                ::SetBkMode(hdc, TRANSPARENT);
                ::SetTextColor(hdc, xg_rgbBlackCellColor);
                ::DrawTextW(hdc, sz, -1, &rcText, DT_CENTER | DT_SINGLELINE | DT_BOTTOM);
            }
        }
    }

    // タテのカギの先頭マス。
    {
        const int size = static_cast<int>(xg_vTateInfo.size());
        for (int k = 0; k < size; k++) {
            const int i = xg_vTateInfo[k].m_iRow;
            const int j = xg_vTateInfo[k].m_jCol;
            StringCbPrintf(sz, sizeof(sz), L"%u", xg_vTateInfo[k].m_number);

            // 文字の背景を塗りつぶす。
            ::SetBkMode(hdc, OPAQUE);
            int nMarked = XgGetMarked(i, j);
            if (slot.count(XG_Pos(i, j)) > 0) {
                ::SetBkColor(hdc, c_rgbHighlight);
            } else if (nMarked != -1) {
                ::SetBkColor(hdc, xg_rgbMarkedCellColor);
            } else {
                ::SetBkColor(hdc, xg_rgbWhiteCellColor);
            }

            if (xg_bShowNumbering) {
                // 数字を描く。
                ::SetRect(&rc,
                    static_cast<int>(xg_nMargin + j * nCellSize), 
                    static_cast<int>(xg_nMargin + i * nCellSize),
                    static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                    static_cast<int>(xg_nMargin + (i + 1) * nCellSize));
                ::OffsetRect(&rc, 2, 1);
                ::SetTextColor(hdc, xg_rgbBlackCellColor);
                ::DrawTextW(hdc, sz, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_TOP);
            }
        }
    }
    // ヨコのカギの先頭マス。
    {
        const int size = static_cast<int>(xg_vYokoInfo.size());
        for (int k = 0; k < size; k++) {
            const int i = xg_vYokoInfo[k].m_iRow;
            const int j = xg_vYokoInfo[k].m_jCol;
            StringCbPrintf(sz, sizeof(sz), L"%u", xg_vYokoInfo[k].m_number);

            // 文字の背景を塗りつぶす。
            int nMarked = XgGetMarked(i, j);
            if (slot.count(XG_Pos(i, j)) > 0) {
                ::SetBkColor(hdc, c_rgbHighlight);
            } else if (nMarked != -1) {
                ::SetBkColor(hdc, xg_rgbMarkedCellColor);
            } else {
                ::SetBkColor(hdc, xg_rgbWhiteCellColor);
            }

            if (xg_bShowNumbering) {
                // 数字を描く。
                ::SetRect(&rc,
                    static_cast<int>(xg_nMargin + j * nCellSize), 
                    static_cast<int>(xg_nMargin + i * nCellSize),
                    static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                    static_cast<int>(xg_nMargin + (i + 1) * nCellSize));
                ::OffsetRect(&rc, 2, 1);
                ::SetTextColor(hdc, xg_rgbBlackCellColor);
                ::DrawTextW(hdc, sz, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_TOP);
            }
        }
    }

    // フォントの選択を解除する。
    ::SelectObject(hdc, hFontOld);

    // 文字の背景は透明。塗りつぶさない。
    ::SetBkMode(hdc, TRANSPARENT);

    // セルの文字を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            WCHAR ch = xw.GetAt(i, j);
            if (ch == ZEN_BLACK)
                continue;

            // 文字を変換する。
            if (xg_bHiragana) {
                WCHAR new_ch;
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch, 1, &new_ch, 1);
                ch = new_ch;
            } else {
                WCHAR new_ch;
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch, 1, &new_ch, 1);
                ch = new_ch;
            }

            if (XgIsCharKanaW(ch) || ch == ZEN_PROLONG ||
                xg_imode == xg_im_KANA || xg_imode == xg_im_KANJI)
            {
                WCHAR new_ch;
                if (xg_bLowercase) {
                    LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                } else {
                    LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_UPPERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                }
            } else {
                WCHAR new_ch;
                if (xg_bLowercase) {
                    LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                } else {
                    LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                }
            }

            // 文字を書く。
            hFontOld = ::SelectObject(hdc, hFont);
            ::SetTextColor(hdc, xg_rgbBlackCellColor);
            ::DrawTextW(hdc, &ch, 1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            ::SelectObject(hdc, hFontOld);
        }
    }

    // キャレットを描画する。
    if (bScreen && xg_bShowCaret) {
        const int i = xg_caret_pos.m_i;
        const int j = xg_caret_pos.m_j;
        ::SetRect(&rc,
            static_cast<int>(xg_nMargin + j * nCellSize), 
            static_cast<int>(xg_nMargin + i * nCellSize),
            static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
            static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

        const int cxyMargin = nCellSize / 10;
        const int cxyLine = nCellSize / 3;
        const int cxyCross = nCellSize / 10;

        hPenOld = ::SelectObject(hdc, hCaretPen);
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

        ::MoveToEx(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cxyCross, nullptr);
        ::LineTo(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 + cxyCross);
        ::MoveToEx(hdc, (rc.left + rc.right) / 2 - cxyCross, (rc.top + rc.bottom) / 2, nullptr);
        ::LineTo(hdc, (rc.left + rc.right) / 2 + cxyCross, (rc.top + rc.bottom) / 2);
        ::SelectObject(hdc, hPenOld);
    }

    // 線を引く。
    hPenOld = ::SelectObject(hdc, hThinPen);
    for (int i = 0; i <= xg_nRows; i++) {
        ::MoveToEx(hdc, xg_nMargin, static_cast<int>(xg_nMargin + i * nCellSize), nullptr);
        ::LineTo(hdc, psiz->cx - xg_nMargin, static_cast<int>(xg_nMargin + i * nCellSize));
    }
    for (int j = 0; j <= xg_nCols; j++) {
        ::MoveToEx(hdc, static_cast<int>(xg_nMargin + j * nCellSize), xg_nMargin, nullptr);
        ::LineTo(hdc, static_cast<int>(xg_nMargin + j * nCellSize), psiz->cy - xg_nMargin);
    }
    ::SelectObject(hdc, hPenOld);

    // 周りに太い線を描く。
    hPenOld = ::SelectObject(hdc, hWidePen);
    if (xg_bAddThickFrame) {
        c_nWide /= 2;
        ::MoveToEx(hdc, xg_nMargin - c_nWide, xg_nMargin - c_nWide, nullptr);
        ::LineTo(hdc, psiz->cx - xg_nMargin + c_nWide, xg_nMargin - c_nWide);
        ::LineTo(hdc, psiz->cx - xg_nMargin + c_nWide, psiz->cy - xg_nMargin + c_nWide);
        ::LineTo(hdc, xg_nMargin - c_nWide, psiz->cy - xg_nMargin + c_nWide);
        ::LineTo(hdc, xg_nMargin - c_nWide, xg_nMargin - c_nWide);
    }
    ::SelectObject(hdc, hPenOld);

    // 破棄する。
    ::DeleteObject(hFont);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hThinPen);
    ::DeleteObject(hWidePen);
    ::DeleteObject(hCaretPen);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
    ::DeleteObject(hbrHighlight);
    ::DeleteObject(hbrHighlightAndDblFrame);
}

// クロスワードを描画する（スケルトンビュー）。
void __fastcall XgDrawXWord_SkeletonView(XG_Board& xw, HDC hdc, LPSIZE psiz, bool bScreen)
{
    INT nCellSize;
    if (xg_nForDisplay > 0) {
        nCellSize = xg_nCellSize * xg_nZoomRate / 100;
    } else {
        nCellSize = xg_nCellSize;
    }

    // スクリーンの場合は、全体を白で塗りつぶす。
    RECT rc;
    ::SetRect(&rc, 0, 0, psiz->cx, psiz->cy);
    if (bScreen)
        ::FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

    LOGFONTW lf;

    // 文字マスのフォントを作成する。
    ZeroMemory(&lf, sizeof(lf));
    // その他。
    StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
    if (xg_szCellFont[0])
        StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szCellFont);
    lf.lfHeight = -nCellSize * xg_nCellCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    HFONT hFont = ::CreateFontIndirectW(&lf);

    // 小さい文字のフォントを作成する。
    ZeroMemory(&lf, sizeof(lf));
    if (xg_szSmallFont[0])
        StringCbCopy(lf.lfFaceName, sizeof(lf.lfFaceName), xg_szSmallFont);
    lf.lfHeight = -nCellSize * xg_nSmallCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    HFONT hFontSmall = ::CreateFontIndirectW(&lf);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);
    HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
    HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);

    auto slot = XgGetSlot(xg_highlight.m_number, xg_highlight.m_vertical);
    if (xg_nForDisplay <= 0)
        slot.clear();

    // 黒の細いペンを作成する。
    HPEN hThinPen = ::CreatePen(PS_SOLID, 1, xg_rgbBlackCellColor);

    // 赤いキャレットペンを作成する。
    LOGBRUSH lbRed;
    lbRed.lbStyle = BS_SOLID;
    lbRed.lbColor = RGB(255, 0, 0);
    HPEN hCaretPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
        1, &lbRed, 0, NULL);

    // 黒い太いペンを作成する。
    LOGBRUSH lbBlack;
    ::GetObject(hbrBlack, sizeof(lbBlack), &lbBlack);
    int c_nWide = 4;
    HPEN hWidePen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nWide, &lbBlack, 0, NULL);

    WCHAR sz[32];
    SIZE siz;
    HGDIOBJ hFontOld, hPenOld;

    // セルの背景を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            RECT rcExtended = rc;
            InflateRect(&rcExtended, c_nWide, c_nWide);

            WCHAR ch = xw.GetAt(i, j);
            if (ch != ZEN_BLACK) {
                // 背景を塗りつぶす。
                ::FillRect(hdc, &rcExtended, hbrBlack);
            }
        }
    }

    // 文字の背景を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            // 二重マスか？
            int nMarked = XgGetMarked(i, j);

            WCHAR ch = xw.GetAt(i, j);
            if (ch == ZEN_BLACK)
                continue;

            // 塗りつぶす。
            if (slot.count(XG_Pos(i, j)) > 0 && nMarked != -1) {
                // ハイライトかつ二重マス。
                ::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
            } else if (slot.count(XG_Pos(i, j)) > 0) {
                // ハイライト。
                ::FillRect(hdc, &rc, hbrHighlight);
            } else if (nMarked != -1) {
                // 二重マス。
                ::FillRect(hdc, &rc, hbrMarked);
            } else {
                // その他のマス。
                ::FillRect(hdc, &rc, hbrWhite);
            }

            if (ch != ZEN_BLACK) {
                // 線を引く。
                hPenOld = ::SelectObject(hdc, hThinPen);
                {
                    ::MoveToEx(hdc, static_cast<int>(xg_nMargin + j * nCellSize), static_cast<int>(xg_nMargin + i * nCellSize), nullptr);
                    ++i;
                    ::LineTo(hdc, static_cast<int>(xg_nMargin + j * nCellSize), static_cast<int>(xg_nMargin + i * nCellSize));
                    --i;

                    ::MoveToEx(hdc, static_cast<int>(xg_nMargin + j * nCellSize), static_cast<int>(xg_nMargin + i * nCellSize), nullptr);
                    ++j;
                    ::LineTo(hdc, static_cast<int>(xg_nMargin + j * nCellSize), static_cast<int>(xg_nMargin + i * nCellSize));
                    --j;
                }
                ::SelectObject(hdc, hPenOld);
            }
        }
    }

    // 小さい文字のフォントを選択する。
    hFontOld = ::SelectObject(hdc, hFontSmall);

    // 二重マスを描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize),
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize) - 1,
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            // 二重マスか？
            int nMarked = XgGetMarked(i, j);
            if (nMarked == -1) {
                continue;
            }

            // 二重マスの内側の枠を描く。
            if (xg_bDrawFrameForMarkedCell) {
                ::InflateRect(&rc, -4, -4);
                ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
                hPenOld = ::SelectObject(hdc, hThinPen);
                ::Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
                ::SelectObject(hdc, hPenOld);
                ::InflateRect(&rc, 4, 4);
            }

            if (xg_bShowDoubleFrameLetters) {
                if (nMarked < (INT)xg_strDoubleFrameLetters.size())
                    StringCbPrintf(sz, sizeof(sz), L"%c", xg_strDoubleFrameLetters[nMarked]);
                else
                    StringCbPrintf(sz, sizeof(sz), L"%c", ZEN_BLACK);

                // 二重マスの右下端の文字の背景を塗りつぶす。
                RECT rcText;
                GetTextExtentPoint32(hdc, sz, lstrlen(sz), &siz);
                rcText = rc;
                rcText.left = rc.right - std::max(siz.cx, siz.cy);
                rcText.top = rc.bottom - std::max(siz.cx, siz.cy);

                HBRUSH hbr = CreateSolidBrush(xg_rgbMarkedCellColor);
                FillRect(hdc, &rcText, hbr);
                DeleteObject(hbr);

                // 二重マスの右下端の文字を描く。
                ::SetBkMode(hdc, TRANSPARENT);
                ::SetTextColor(hdc, xg_rgbBlackCellColor);
                ::DrawTextW(hdc, sz, -1, &rcText, DT_CENTER | DT_SINGLELINE | DT_BOTTOM);
            }
        }
    }

    // タテのカギの先頭マス。
    {
        const int size = static_cast<int>(xg_vTateInfo.size());
        for (int k = 0; k < size; k++) {
            const int i = xg_vTateInfo[k].m_iRow;
            const int j = xg_vTateInfo[k].m_jCol;
            StringCbPrintf(sz, sizeof(sz), L"%u", xg_vTateInfo[k].m_number);

            // 文字の背景を塗りつぶす。
            ::SetBkMode(hdc, OPAQUE);
            int nMarked = XgGetMarked(i, j);
            if (slot.count(XG_Pos(i, j)) > 0) {
                ::SetBkColor(hdc, c_rgbHighlight);
            } else if (nMarked != -1) {
                ::SetBkColor(hdc, xg_rgbMarkedCellColor);
            } else {
                ::SetBkColor(hdc, xg_rgbWhiteCellColor);
            }

            if (xg_bShowNumbering) {
                // 数字を描く。
                ::SetRect(&rc,
                    static_cast<int>(xg_nMargin + j * nCellSize), 
                    static_cast<int>(xg_nMargin + i * nCellSize),
                    static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                    static_cast<int>(xg_nMargin + (i + 1) * nCellSize));
                ::OffsetRect(&rc, 2, 1);
                ::SetTextColor(hdc, xg_rgbBlackCellColor);
                ::DrawTextW(hdc, sz, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_TOP);
            }
        }
    }
    // ヨコのカギの先頭マス。
    {
        const int size = static_cast<int>(xg_vYokoInfo.size());
        for (int k = 0; k < size; k++) {
            const int i = xg_vYokoInfo[k].m_iRow;
            const int j = xg_vYokoInfo[k].m_jCol;
            StringCbPrintf(sz, sizeof(sz), L"%u", xg_vYokoInfo[k].m_number);

            // 文字の背景を塗りつぶす。
            int nMarked = XgGetMarked(i, j);
            if (slot.count(XG_Pos(i, j)) > 0) {
                ::SetBkColor(hdc, c_rgbHighlight);
            } else if (nMarked != -1) {
                ::SetBkColor(hdc, xg_rgbMarkedCellColor);
            } else {
                ::SetBkColor(hdc, xg_rgbWhiteCellColor);
            }

            if (xg_bShowNumbering) {
                // 数字を描く。
                ::SetRect(&rc,
                    static_cast<int>(xg_nMargin + j * nCellSize), 
                    static_cast<int>(xg_nMargin + i * nCellSize),
                    static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                    static_cast<int>(xg_nMargin + (i + 1) * nCellSize));
                ::OffsetRect(&rc, 2, 1);
                ::SetTextColor(hdc, xg_rgbBlackCellColor);
                ::DrawTextW(hdc, sz, -1, &rc, DT_LEFT | DT_SINGLELINE | DT_TOP);
            }
        }
    }

    // フォントの選択を解除する。
    ::SelectObject(hdc, hFontOld);

    // 文字の背景は透明。塗りつぶさない。
    ::SetBkMode(hdc, TRANSPARENT);

    // セルの文字を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

            WCHAR ch = xw.GetAt(i, j);
            if (ch == ZEN_BLACK)
                continue;

            // 文字を変換する。
            if (xg_bHiragana) {
                WCHAR new_ch;
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch, 1, &new_ch, 1);
                ch = new_ch;
            } else {
                WCHAR new_ch;
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch, 1, &new_ch, 1);
                ch = new_ch;
            }

            if (XgIsCharKanaW(ch) || ch == ZEN_PROLONG ||
                xg_imode == xg_im_KANA || xg_imode == xg_im_KANJI)
            {
                if (xg_bLowercase) {
                    WCHAR new_ch;
                    LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                } else {
                    WCHAR new_ch;
                    LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_UPPERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                }
            } else {
                if (xg_bLowercase) {
                    WCHAR new_ch;
                    LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                } else {
                    WCHAR new_ch;
                    LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch, 1, &new_ch, 1);
                    ch = new_ch;
                }
            }

            // 文字を書く。
            hFontOld = ::SelectObject(hdc, hFont);
            ::SetTextColor(hdc, xg_rgbBlackCellColor);
            ::DrawTextW(hdc, &ch, 1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            ::SelectObject(hdc, hFontOld);
        }
    }

    // キャレットを描画する。
    if (bScreen && xg_bShowCaret) {
        const int i = xg_caret_pos.m_i;
        const int j = xg_caret_pos.m_j;
        ::SetRect(&rc,
            static_cast<int>(xg_nMargin + j * nCellSize), 
            static_cast<int>(xg_nMargin + i * nCellSize),
            static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
            static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

        const int cxyMargin = nCellSize / 10;
        const int cxyLine = nCellSize / 3;
        const int cxyCross = nCellSize / 10;

        hPenOld = ::SelectObject(hdc, hCaretPen);
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

        ::MoveToEx(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 - cxyCross, nullptr);
        ::LineTo(hdc, (rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2 + cxyCross);
        ::MoveToEx(hdc, (rc.left + rc.right) / 2 - cxyCross, (rc.top + rc.bottom) / 2, nullptr);
        ::LineTo(hdc, (rc.left + rc.right) / 2 + cxyCross, (rc.top + rc.bottom) / 2);
        ::SelectObject(hdc, hPenOld);
    }

    // 破棄する。
    ::DeleteObject(hFont);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hThinPen);
    ::DeleteObject(hWidePen);
    ::DeleteObject(hCaretPen);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
    ::DeleteObject(hbrHighlight);
    ::DeleteObject(hbrHighlightAndDblFrame);
}

// クロスワードを描画する。
void __fastcall XgDrawXWord(XG_Board& xw, HDC hdc, LPSIZE psiz, bool bScreen)
{
    switch (xg_nViewMode)
    {
    case XG_VIEW_NORMAL:
    default:
        XgDrawXWord_NormalView(xw, hdc, psiz, bScreen);
        break;
    case XG_VIEW_SKELETON:
        XgDrawXWord_SkeletonView(xw, hdc, psiz, bScreen);
        break;
    }
    // ボックスを描画する。
    if (!bScreen)
        XgDrawBoxes(xw, hdc, psiz);
}

// クロスワードのイメージを作成する。
HBITMAP __fastcall XgCreateXWordImage(XG_Board& xw, LPSIZE psiz, bool bScreen)
{
    // 互換DCを作成する。
    HDC hdc = ::CreateCompatibleDC(nullptr);
    if (hdc == nullptr)
        return nullptr;

    // DIBを作成する。
    BITMAPINFO bi;
    LPVOID pvBits;
    ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = psiz->cx;
    bi.bmiHeader.biHeight = psiz->cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;
    HBITMAP hbm;
    hbm = ::CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
    if (hbm == nullptr) {
        DeleteDC(hdc);
        return nullptr;
    }

    // 描画する。
    HGDIOBJ hbmOld = ::SelectObject(hdc, hbm);
    XgDrawXWord(xw, hdc, psiz, bScreen);
    ::SelectObject(hdc, hbmOld);

    // 互換DCを破棄する。
    ::DeleteDC(hdc);
    return hbm;
}

// CRPファイルを開く。
bool __fastcall XgDoLoadCrpFile(HWND hwnd, LPCWSTR pszFile)
{
    INT i, nWidth, nHeight;
    std::vector<std::wstring> rows;
    WCHAR szName[32], szText[128];
    XG_Board xword;
    std::vector<XG_Hint> tate, yoko;
    bool bOK = false;

    nWidth = GetPrivateProfileIntW(L"Cross", L"Width", -1, pszFile);
    nHeight = GetPrivateProfileIntW(L"Cross", L"Height", -1, pszFile);

    static const WCHAR sz1[] = { ZEN_UNDERLINE, 0 };
    static const WCHAR sz2[] = { ZEN_SPACE, 0 };

    if (nWidth > 0 && nHeight > 0) {
        for (i = 0; i < nHeight; ++i) {
            StringCbPrintf(szName, sizeof(szName), L"Line%u", i + 1);
            GetPrivateProfileStringW(L"Cross", szName, L"", szText, ARRAYSIZE(szText), pszFile);

            std::wstring str = szText;
            xg_str_trim(str);
            xg_str_replace_all(str, L",", L"");
            xg_str_replace_all(str, sz1, sz2);
            str = XgNormalizeString(str);

            if (INT(str.size()) != nWidth)
                break;

            rows.push_back(str);
        }
        if (i == nHeight) {
            std::wstring str;

            str += ZEN_ULEFT;
            for (i = 0; i < nWidth; ++i) {
                str += ZEN_HLINE;
            }
            str += ZEN_URIGHT;
            str += L"\r\n";

            for (auto& item : rows) {
                str += ZEN_VLINE;
                for (i = 0; i < nWidth; ++i) {
                    str += item[i];
                }
                str += ZEN_VLINE;
                str += L"\r\n";
            }

            str += ZEN_LLEFT;
            for (i = 0; i < nWidth; ++i) {
                str += ZEN_HLINE;
            }
            str += ZEN_LRIGHT;
            str += L"\r\n";

            // 文字列を読み込む。
            bOK = xword.SetString(str);
        }
    }

    if (bOK) {
        xg_nCols = nWidth;
        xg_nRows = nHeight;
        xg_vecTateHints.clear();
        xg_vecYokoHints.clear();
        xg_bSolved = false;

        if (xword.IsFulfilled()) {
            // 番号付けを行う。
            xword.DoNumberingNoCheck();

            INT nClueCount = GetPrivateProfileIntW(L"Clue", L"Count", 0, pszFile);
            if (nClueCount) {
                for (i = 0; i < nClueCount; ++i) {
                    StringCbPrintf(szName, sizeof(szName), L"Clue%u", i + 1);
                    GetPrivateProfileStringW(L"Clue", szName, L"", szText, ARRAYSIZE(szText), pszFile);

                    std::wstring str = szText;
                    xg_str_trim(str);
                    if (str.empty())
                        break;
                    size_t icolon = str.find(L':');
                    if (icolon != str.npos) {
                        std::wstring word = str.substr(0, icolon);
                        std::wstring hint = str.substr(icolon + 1);
                        word = XgNormalizeString(word);
                        for (XG_PlaceInfo& item : xg_vTateInfo) {
                            if (item.m_word == word) {
                                tate.emplace_back(item.m_number, word, hint);
                                break;
                            }
                        }
                        for (XG_PlaceInfo& item : xg_vYokoInfo) {
                            if (item.m_word == word) {
                                yoko.emplace_back(item.m_number, word, hint);
                                break;
                            }
                        }
                    }
                }
            } else {
                for (i = 0; i < 256; ++i) {
                    StringCbPrintf(szName, sizeof(szName), L"Down%u", i + 1);
                    GetPrivateProfileStringW(L"Clue", szName, L"", szText, ARRAYSIZE(szText), pszFile);

                    std::wstring str = szText;
                    xg_str_trim(str);
                    if (str.empty())
                        break;
                    if (str == L"{N/A}")
                        continue;
                    for (XG_PlaceInfo& item : xg_vTateInfo) {
                        if (item.m_number == i + 1) {
                            tate.emplace_back(item.m_number, item.m_word, str);
                            break;
                        }
                    }
                }

                for (i = 0; i < 256; ++i) {
                    StringCbPrintf(szName, sizeof(szName), L"Across%u", i + 1);
                    GetPrivateProfileStringW(L"Clue", szName, L"", szText, ARRAYSIZE(szText), pszFile);

                    std::wstring str = szText;
                    xg_str_trim(str);
                    if (str.empty())
                        break;
                    if (str == L"{N/A}")
                        continue;
                    for (XG_PlaceInfo& item : xg_vYokoInfo) {
                        if (item.m_number == i + 1) {
                            yoko.emplace_back(item.m_number, item.m_word, str);
                            break;
                        }
                    }
                }
            }
        }

        // 不足分を追加。
        for (XG_PlaceInfo& item : xg_vYokoInfo) {
            bool found = false;
            for (auto& info : yoko) {
                if (item.m_number == info.m_number) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                yoko.emplace_back(item.m_number, item.m_word, L"");
            }
        }
        for (XG_PlaceInfo& item : xg_vTateInfo) {
            bool found = false;
            for (auto& info : tate) {
                if (item.m_number == info.m_number) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                tate.emplace_back(item.m_number, item.m_word, L"");
            }
        }

        // ソート。
        std::sort(tate.begin(), tate.end(),
            [](const XG_Hint& a, const XG_Hint& b) {
                return a.m_number < b.m_number;
            }
        );
        std::sort(yoko.begin(), yoko.end(),
            [](const XG_Hint& a, const XG_Hint& b) {
                return a.m_number < b.m_number;
            }
        );

        // 成功。
        xg_xword = xword;
        xg_solution = xword;
        if (tate.size() && yoko.size()) {
            xg_bSolved = true;
            xg_bShowAnswer = false;
            XgClearNonBlocks();
            xg_vecTateHints = tate;
            xg_vecYokoHints = yoko;
        }

        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, NULL);
        xg_strFileName = szFileName;
    }

    return bOK;
}

// ファイルを開く。
bool __fastcall XgDoLoadFileType(HWND hwnd, LPCWSTR pszFile, XG_FILETYPE type)
{
    if (type == XG_FILETYPE_CRP)
        return XgDoLoadCrpFile(hwnd, pszFile);

    // 二重マス単語を空にする。
    XgSetMarkedWord();

    try {
        std::wstring strText;
        if (!XgReadTextFileAll(pszFile, strText))
            return false;

        if (XgSetString(hwnd, strText, type)) {
            // ファイルパスをセットする。
            WCHAR szFileName[MAX_PATH];
            ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, NULL);
            xg_strFileName = szFileName;
            return true;
        }
    } catch (...) {
        // 例外が発生した。
    }

    // 失敗。
    return false;
}

// ファイル（CRP形式）を保存する。
bool __fastcall XgDoSaveCrpFile(LPCWSTR pszFile)
{
    // ファイルを作成する。
    FILE *fout = _wfopen(pszFile, L"w");
    if (fout == NULL)
        return false;

    XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

    try
    {
        fprintf(fout,
            "[Version]\n"
            "Ver=0.3.0\n"
            "\n"
            "[Puzzle]\n"
            "Puzzle=0\n"
            "\n"
            "[Cross]\n"
            "Width=%u\n"
            "Height=%u\n", xg_nCols, xg_nRows);

        // マス。
        for (int i = 0; i < xg_nRows; ++i) {
            std::wstring row;
            for (int j = 0; j < xg_nCols; ++j) {
                if (row.size())
                    row += L",";
                WCHAR ch = xw->GetAt(i, j);
                if (ch == ZEN_SPACE)
                    row += 0xFF3F; // '＿'
                else
                    row += ch;
            }
            fprintf(fout, "Line%u=%s\n", i + 1, XgUnicodeToAnsi(row).c_str());
        }

        // 二重マス。
        std::vector<std::vector<INT> > marks;
        marks.resize(xg_nRows);
        for (auto& mark : marks) {
            mark.resize(xg_nCols);
        }

        std::wstring answer;
        if (xg_vMarks.size()) {
            for (size_t i = 0; i < xg_vMarks.size(); ++i) {
                answer += xw->GetAt(xg_vMarks[i].m_i, xg_vMarks[i].m_j);
                marks[xg_vMarks[i].m_i][xg_vMarks[i].m_j] = INT(i) + 1;
            }
        }

        for (int i = 0; i < xg_nRows; ++i) {
            std::string row;
            for (int j = 0; j < xg_nCols; ++j) {
                if (row.size())
                    row += ",";
                row += std::to_string(marks[i][j]);
            }
            fprintf(fout, "MarkUpLine%u=%s\n", i + 1, row.c_str());
        }
        fprintf(fout,
            "\n"
            "[Property]\n"
            "Symmetry=None\n"
            "\n"
            "[Clue]\n");

        // ヒント。
        if (xg_vecTateHints.size() && xg_vecYokoHints.size()) {
            fprintf(fout, "Count=%u\n", int(xg_vecTateHints.size() + xg_vecYokoHints.size()));
            int iHint = 1;
            // タテのカギ。
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                auto& tate_hint = xg_vecTateHints[i];
                fprintf(fout, "Clue%u=%s:%s\n", iHint++,
                    XgUnicodeToAnsi(tate_hint.m_strWord).c_str(),
                    XgUnicodeToAnsi(tate_hint.m_strHint).c_str());
            }
            // ヨコのカギ。
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                auto& yoko_hint = xg_vecYokoHints[i];
                fprintf(fout, "Clue%u=%s:%s\n", iHint++,
                    XgUnicodeToAnsi(yoko_hint.m_strWord).c_str(),
                    XgUnicodeToAnsi(yoko_hint.m_strHint).c_str());
            }
        }

        fprintf(fout,
            "\n"
            "[Numbering]\n"
            "Hint=%s\n"
            "Answer=%s\n"
            "Theme=%s\n"
            "CantUseChar=%s\n",
            "",
            XgUnicodeToAnsi(answer).c_str(),
            XgUnicodeToAnsi(xg_strTheme).c_str(),
            "");

        fclose(fout);

        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, NULL);
        xg_strFileName = szFileName;
        XgMarkUpdate();
        return true;
    }
    catch(...)
    {
        ;
    }

    // 正しく書き込めなかった。不正なファイルを消す。
    ::DeleteFileW(pszFile);
    return false;
}

// .xwjファイル（JSON形式）を保存する。
bool __fastcall XgDoSaveJson(LPCWSTR pszFile)
{
    HANDLE hFile;
    std::wstring str, strTable, strMarks, hints;
    DWORD size;

    // ファイルを作成する。
    hFile = ::CreateFileW(pszFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    try
    {
        json j;
        // 作成者情報。
        j["creator_info"] = XgUnicodeToUtf8(XgLoadStringDx1(IDS_APPINFO));
        // 行の数。
        j["row_count"] = xg_nRows;
        // 列の数。
        j["column_count"] = xg_nCols;
        // ルール。
        j["rules"] = XgUnicodeToUtf8(XgGetRulesString(xg_nRules));
        // 辞書名。
        j["dictionary"] = XgUnicodeToUtf8(PathFindFileNameW(xg_dict_name.c_str()));
        // ビューモード。
        j["view_mode"] = (int)xg_nViewMode;

        // 盤の切り替え。
        XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);
        j["is_solved"] = !!xg_bSolved;

        // マス。
        for (int i = 0; i < xg_nRows; ++i) {
            std::wstring row;
            for (int j = 0; j < xg_nCols; ++j) {
                WCHAR ch = xw->GetAt(i, j);
                row += ch;
            }
            j["cell_data"].push_back(XgUnicodeToUtf8(row));
        }

        // ボックス。
        XgDoSaveBoxJson(j);

        // 二重マス。
        if (xg_vMarks.size()) {
            j["has_mark"] = true;

            std::wstring mark_word;
            for (size_t i = 0; i < xg_vMarks.size(); ++i) {
                WCHAR ch = xw->GetAt(xg_vMarks[i].m_i, xg_vMarks[i].m_j);
                mark_word += ch;
            }

            j["mark_word"] = XgUnicodeToUtf8(mark_word);

            str += L"\t\"marks\": [\r\n";
            for (size_t i = 0; i < xg_vMarks.size(); ++i) {
                json mark;
                mark.push_back(xg_vMarks[i].m_i + 1);
                mark.push_back(xg_vMarks[i].m_j + 1);
                WCHAR sz[2] = { mark_word[i] , 0 };
                mark.push_back(XgUnicodeToUtf8(sz));
                j["marks"].push_back(mark);
            }
        } else {
            j["has_mark"] = false;
        }

        // ヒント。
        if (xg_vecTateHints.size() && xg_vecYokoHints.size()) {
            j["has_hints"] = true;

            json hints;

            // タテのカギ。
            json v;
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                json hint;
                auto& tate_hint = xg_vecTateHints[i];
                hint.push_back(tate_hint.m_number);
                hint.push_back(XgUnicodeToUtf8(tate_hint.m_strWord));
                hint.push_back(XgUnicodeToUtf8(tate_hint.m_strHint));
                v.push_back(hint);
            }
            hints["v"] = v;

            // ヨコのカギ。
            json h;
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                json hint;
                auto& yoko_hint = xg_vecYokoHints[i];
                hint.push_back(yoko_hint.m_number);
                hint.push_back(XgUnicodeToUtf8(yoko_hint.m_strWord));
                hint.push_back(XgUnicodeToUtf8(yoko_hint.m_strHint));
                h.push_back(hint);
            }
            hints["h"] = h;

            j["hints"] = hints;
        } else {
            j["has_hints"] = false;
        }

        // ヘッダー。
        xg_str_trim(xg_strHeader);
        j["header"] = XgUnicodeToUtf8(xg_strHeader);

        // 備考欄。
        xg_str_trim(xg_strNotes);
        LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
        if (xg_strNotes.find(psz) == 0) {
            xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
        }
        j["notes"] = XgUnicodeToUtf8(xg_strNotes);

        // テーマ。
        j["theme"] = XgUnicodeToUtf8(xg_strTheme);

        // UTF-8へ変換する。
        std::string utf8 = j.dump(1, '\t');
        utf8 += '\n';

        std::string replaced;
        for (auto ch : utf8)
        {
            if (ch == '\n')
                replaced += '\r';
            replaced += ch;
        }
        utf8 = std::move(replaced);

        // ファイルに書き込んで、ファイルを閉じる。
        size = static_cast<DWORD>(utf8.size()) * sizeof(CHAR);
        if (::WriteFile(hFile, utf8.data(), size, &size, nullptr)) {
            ::CloseHandle(hFile);

            // ファイルパスをセットする。
            WCHAR szFileName[MAX_PATH];
            ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, NULL);
            xg_strFileName = szFileName;
            XgMarkUpdate();
            return true;
        }
        ::CloseHandle(hFile);
    }
    catch(...)
    {
        ;
    }

    // 正しく書き込めなかった。不正なファイルを消す。
    ::DeleteFileW(pszFile);
    return false;
}

// .xwd/.xwjファイルを保存する。
bool XgDoSaveStandard(HWND hwnd, LPCWSTR pszFile, const XG_Board& board)
{
    HANDLE hFile;
    std::wstring str, strTable, strMarks, hints;
    DWORD size;

    // ファイルを作成する。
    hFile = ::CreateFileW(pszFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    // マークを取得する。
    if (!xg_vMarks.empty())
        XgGetStringOfMarks(strMarks);

    // ファイルに書き込む文字列を求める。
    xg_str_trim(xg_strHeader);

    if (xg_bSolved) {
        // ヒントあり。
        board.GetString(strTable);
        XgGetHintsStr(board, hints, 2, true);
        str += xg_strHeader;        // ヘッダー文字列。
        str += xg_pszNewLine;       // 改行。
        str += XgLoadStringDx1(IDS_HEADERSEP1); // ヘッダー分離線。
        str += XgLoadStringDx1(IDS_APPINFO); // アプリ情報。
        str += xg_pszNewLine;       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
        str += xg_pszNewLine;       // 改行。
        str += hints;               // ヒント。
    } else {
        // ヒントなし。
        board.GetString(strTable);
        str += xg_strHeader;        // ヘッダー文字列。
        str += xg_pszNewLine;       // 改行。
        str += XgLoadStringDx1(IDS_HEADERSEP1); // ヘッダー分離線。
        str += XgLoadStringDx1(IDS_APPINFO); // アプリ情報。
        str += xg_pszNewLine;       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
    }
    str += XgLoadStringDx1(IDS_HEADERSEP2);     // フッター分離線。

    // 備考欄。
    LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
    str += psz;
    str += xg_pszNewLine;
    if (xg_strNotes.find(psz) == 0) {
        xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
    }
    xg_str_trim(xg_strNotes);
    str += xg_strNotes;
    str += xg_pszNewLine;

    // ファイルに書き込んで、ファイルを閉じる。
    size = 2;
    if (::WriteFile(hFile, "\xFF\xFE", size, &size, nullptr)) {
        size = static_cast<DWORD>(str.size()) * sizeof(WCHAR);
        if (::WriteFile(hFile, str.data(), size, &size, nullptr)) {
            ::CloseHandle(hFile);
            return true;
        }
    }
    ::CloseHandle(hFile);

    // 正しく書き込めなかった。不正なファイルを消す。
    ::DeleteFileW(pszFile);
    return false;
}

// 文字列を標準化する。
std::wstring XgNormalizeStringEx(const std::wstring& str, BOOL bUppercase, BOOL bKatakana) {
    std::wstring ret;
    for (auto ch : str) {
        WCHAR newch = 0;
        // 小さな字を大きな字にする。
        for (size_t ich = 0; ich < _countof(xg_small); ++ich) {
            if (ch == xg_small[ich][0]) {
                ch = xg_large[ich][0];
                break;
            }
        }
        if (XgIsCharKanaW(ch) || XgIsCharKanjiW(ch) || ch == ZEN_PROLONG || XgIsCharHangulW(ch) ||
            ch == ZEN_UNDERLINE)
        {
            // カナか漢字かハングルなら全角文字。
            if (bKatakana) {
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch, 1, &newch, 1);
            } else {
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch, 1, &newch, 1);
            }
        } else {
            // それ以外は半角文字。
            if (bUppercase) {
                LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch, 1, &newch, 1);
            } else {
                LCMapStringW(JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch, 1, &newch, 1);
            }
        }
        ret += newch;
    }
    return ret;
}

// ファイル（XD形式）を保存する。
bool __fastcall XgDoSaveXdFile(LPCWSTR pszFile)
{
    // ファイルを作成する。
    FILE *fout = _wfopen(pszFile, L"wb");
    if (fout == NULL)
        return false;

    XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

    try
    {
        // ヘッダ。
        auto strHeader = xg_strHeader;
        xg_str_trim(strHeader);
        if (strHeader.empty()) {
            WCHAR szFileTitle[MAX_PATH];
            StringCchCopyW(szFileTitle, _countof(szFileTitle), PathFindFileNameW(pszFile));
            PathRemoveExtensionW(szFileTitle);
            fprintf(fout, "Title: %s\n", XgUnicodeToUtf8(szFileTitle).c_str());
            fprintf(fout, "Author: \n");
            fprintf(fout, "Editor: \n");
            fprintf(fout, "Copyright: \n");
            SYSTEMTIME st;
            ::GetLocalTime(&st);
            fprintf(fout, "Date: %04u-%02u-%02u\n", st.wYear, st.wMonth, st.wDay);
        } else {
            xg_str_replace_all(strHeader, L"\r\n", L"\n");
            xg_str_trim(strHeader);
            fprintf(fout, "%s\n", XgUnicodeToUtf8(strHeader).c_str());
        }
        fprintf(fout, "\n\n");

        BOOL bAsian = FALSE;
        for (int i = 0; i < xg_nRows; ++i) {
            for (int j = 0; j < xg_nCols; ++j) {
                WCHAR ch = xw->GetAt(i, j);
                if (XgIsCharKanaW(ch) || XgIsCharKanjiW(ch) ||
                    ch == ZEN_PROLONG || XgIsCharHangulW(ch))
                {
                    bAsian = TRUE;
                    j = xg_nCols;
                    i = xg_nRows;
                }
            }
        }

        // マス。
        for (int i = 0; i < xg_nRows; ++i) {
            std::wstring row;
            for (int j = 0; j < xg_nCols; ++j) {
                WCHAR ch = xw->GetAt(i, j);
                if (bAsian) {
                    if (ch == ZEN_SPACE)
                        row += ZEN_UNDERLINE;
                    else if (ch == ZEN_BLACK)
                        row += ZEN_BLACK;
                    else
                        row += ch;
                } else {
                    if (ch == ZEN_SPACE)
                        row += L'_';
                    else if (ch == ZEN_BLACK)
                        row += L'#';
                    else
                        row += ch;
                }
            }
            row = XgNormalizeStringEx(row);
            fprintf(fout, "%s\n", XgUnicodeToUtf8(row).c_str());
        }
        fprintf(fout, "\n\n");

        // ヒント。
        if (xg_vecTateHints.size() && xg_vecYokoHints.size()) {
            char line[512];
            std::string strACROSS, strDOWN;
            // タテのカギ。
            for (auto& tate_hint : xg_vecTateHints) {
                auto word = XgNormalizeStringEx(tate_hint.m_strWord);
                StringCchPrintfA(line, _countof(line), "D%u. %s ~ %s\n",
                                 tate_hint.m_number,
                                 XgUnicodeToUtf8(tate_hint.m_strHint).c_str(),
                                 XgUnicodeToUtf8(word).c_str());
                strDOWN += line;
            }
            // ヨコのカギ。
            for (auto& yoko_hint : xg_vecYokoHints) {
                auto word = XgNormalizeStringEx(yoko_hint.m_strWord);
                StringCchPrintfA(line, _countof(line), "A%u. %s ~ %s\n",
                                 yoko_hint.m_number,
                                 XgUnicodeToUtf8(yoko_hint.m_strHint).c_str(),
                                 XgUnicodeToUtf8(word).c_str());
                strACROSS += line;
            }

            fprintf(fout, "%s\n%s\n\n", strACROSS.c_str(), strDOWN.c_str());
        } else {
            // ヒントなし。
            fprintf(fout, "(No clues)\n\n\n");
        }

        // マーク。
        if (xg_vMarks.size()) {
            std::wstring strMarks;
            XgGetStringOfMarks2(strMarks);
            fprintf(fout, "%s\n\n", XgUnicodeToUtf8(strMarks).c_str());
        }

        // ビューモード。
        fprintf(fout, "ViewMode: %d\n\n", xg_nViewMode);

        // 備考欄。
        if (xg_strNotes.size()) {
            auto str = xg_strNotes;
            xg_str_replace_all(str, L"\r\n", L"\n");
            xg_str_trim(str);
            if (str.size()) {
                fprintf(fout, "%s\n", XgUnicodeToUtf8(str).c_str());
            }
        }

        // ボックス。
        XgWriteXdBoxes(fout);

        fclose(fout);

        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, NULL);
        xg_strFileName = szFileName;
        XgMarkUpdate();
        return true;
    }
    catch(...)
    {
        ;
    }

    // 正しく書き込めなかった。不正なファイルを消す。
    ::DeleteFileW(pszFile);
    return false;
}

// ファイルを保存する。
bool __fastcall XgDoSaveFileType(HWND hwnd, LPCWSTR pszFile, XG_FILETYPE type)
{
    bool ret;
    switch (type)
    {
    case XG_FILETYPE_XWD:
        if (xg_bSolved) {
            // ヒントあり。
            ret = XgDoSaveStandard(hwnd, pszFile, xg_solution);
        } else {
            ret = XgDoSaveStandard(hwnd, pszFile, xg_xword);
        }
        break;
    case XG_FILETYPE_XWJ:
        ret = XgDoSaveJson(pszFile);
        break;
    case XG_FILETYPE_CRP:
        ret = XgDoSaveCrpFile(pszFile);
        break;
    case XG_FILETYPE_XD:
        ret = XgDoSaveXdFile(pszFile);
        break;
    default:
        assert(0);
        ret = false;
    }

    if (ret) {
        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, NULL);
        xg_strFileName = szFileName;
        XgMarkUpdate();
    }
    return ret;
}

// ファイルを読み込む。
bool __fastcall XgDoLoadMainFile(HWND hwnd, LPCWSTR pszFile)
{
    LPCWSTR pchDotExt = PathFindExtensionW(pszFile);
    if (lstrcmpiW(pchDotExt, L".xwj") == 0 ||
        lstrcmpiW(pchDotExt, L".json") == 0 ||
        lstrcmpiW(pchDotExt, L".jso") == 0)
    {
        return XgDoLoadFileType(hwnd, pszFile, XG_FILETYPE_XWJ);
    } else if (lstrcmpiW(pchDotExt, L".crp") == 0) {
        return XgDoLoadFileType(hwnd, pszFile, XG_FILETYPE_CRP);
    } else if (lstrcmpiW(pchDotExt, L".xd") == 0) {
        return XgDoLoadFileType(hwnd, pszFile, XG_FILETYPE_XD);
    } else if (lstrcmpiW(pchDotExt, L".xwd") == 0) {
        return XgDoLoadFileType(hwnd, pszFile, XG_FILETYPE_XWD);
    } else {
        return XgDoLoadFileType(hwnd, pszFile, XG_FILETYPE_XWJ);
    }
}

bool __fastcall XgDoSaveFile(HWND hwnd, LPCWSTR pszFile)
{
    LPCWSTR pchDotExt = PathFindExtensionW(pszFile);
    if (lstrcmpiW(pchDotExt, L".xwj") == 0 ||
        lstrcmpiW(pchDotExt, L".json") == 0 ||
        lstrcmpiW(pchDotExt, L".jso") == 0)
    {
        return XgDoSaveFileType(hwnd, pszFile, XG_FILETYPE_XWJ);
    } else if (lstrcmpiW(pchDotExt, L".crp") == 0) {
        return XgDoSaveFileType(hwnd, pszFile, XG_FILETYPE_CRP);
    } else if (lstrcmpiW(pchDotExt, L".xd") == 0) {
        return XgDoSaveFileType(hwnd, pszFile, XG_FILETYPE_XD);
    } else if (lstrcmpiW(pchDotExt, L".xwd") == 0) {
        return XgDoSaveFileType(hwnd, pszFile, XG_FILETYPE_XWD);
    } else {
        WCHAR szPath[MAX_PATH];
        StringCchCopyW(szPath, _countof(szPath), pszFile);
        PathAddExtensionW(szPath, L".xwj");
        return XgDoSaveFileType(hwnd, szPath, XG_FILETYPE_XWJ);
    }
}

// BITMAPINFOEX構造体。
typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR * LPBITMAPINFOEX;

// ビットマップをファイルに保存する。
bool __fastcall XgSaveBitmapToFile(LPCWSTR pszFileName, HBITMAP hbm)
{
    bool f;
    BITMAPFILEHEADER bf;
    BITMAPINFOEX bi;
    BITMAPINFOHEADER *pbmih;
    DWORD cb;
    DWORD cColors, cbColors;
    HDC hDC;
    HANDLE hFile;
    LPVOID pBits;
    BITMAP bm;
    DWORD dwError = 0;

    // ビットマップの情報を取得する。
    if (!::GetObject(hbm, sizeof(BITMAP), &bm))
        return false;

    // BITMAPINFO構造体を設定する。
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

    // BITMAPFILEHEADER構造体を設定する。
    bf.bfType = 0x4d42;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    cb = sizeof(BITMAPFILEHEADER) + pbmih->biSize + cbColors;
    bf.bfOffBits = cb;
    bf.bfSize = cb + pbmih->biSizeImage;

    // ビット格納用のメモリを確保する。
    pBits = ::HeapAlloc(::GetProcessHeap(), 0, pbmih->biSizeImage);
    if (pBits == nullptr)
        return false;

    // DCを取得する。
    f = false;
    hDC = ::GetDC(nullptr);
    if (hDC != nullptr) {
        // ビットを取得する。
        if (::GetDIBits(hDC, hbm, 0, bm.bmHeight, pBits,
                      reinterpret_cast<BITMAPINFO*>(&bi),
                      DIB_RGB_COLORS))
        {
            // ファイルを作成する。
            hFile = ::CreateFileW(pszFileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL |
                                FILE_FLAG_WRITE_THROUGH, nullptr);
            if (hFile != INVALID_HANDLE_VALUE) {
                // ファイルに書き込む。
                f = ::WriteFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &cb, nullptr) &&
                    ::WriteFile(hFile, &bi, sizeof(BITMAPINFOHEADER), &cb, nullptr) &&
                    ::WriteFile(hFile, bi.bmiColors, cbColors, &cb, nullptr) &&
                    ::WriteFile(hFile, pBits, pbmih->biSizeImage, &cb, nullptr);
                if (!f)
                    dwError = ::GetLastError();
                // ファイルを閉じる。
                ::CloseHandle(hFile);

                if (!f)
                    ::DeleteFileW(pszFileName);
            } else {
                dwError = ::GetLastError();
            }
        } else {
            dwError = ::GetLastError();
        }

        // DCを解放する。
        ::ReleaseDC(nullptr, hDC);
    } else {
        dwError = ::GetLastError();
    }

    // 確保したメモリを解放する。
    ::HeapFree(::GetProcessHeap(), 0, pBits);
    // エラーコードを設定する。
    ::SetLastError(dwError);
    return f;
}

#ifndef CDSIZEOF_STRUCT
    #define CDSIZEOF_STRUCT(structname,member) \
        (((INT_PTR)((LPBYTE)(&((structname*)0)->member) - ((LPBYTE)((structname*)0)))) + sizeof(((structname*)0)->member))
#endif
#ifndef OPENFILENAME_SIZE_VERSION_400W
    #define OPENFILENAME_SIZE_VERSION_400W CDSIZEOF_STRUCT(OPENFILENAMEW,lpTemplateName)
#endif

// 問題を画像ファイルとして保存する。
void __fastcall XgSaveProbAsImage(HWND hwnd)
{
    OPENFILENAMEW ofn;
    WCHAR szFileName[MAX_PATH] = L"";

    // 「問題を画像ファイルとして保存」ダイアログを表示。
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_IMGFILTER));
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = ARRAYSIZE(szFileName);
    ofn.lpstrTitle = XgLoadStringDx1(IDS_SAVEPROBASIMG);
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"bmp";
    if (::GetSaveFileNameW(&ofn)) {
        // 描画サイズを取得する。
        SIZE siz;
        XgGetXWordExtent(&siz);

        if (ofn.nFilterIndex <= 1) {
            // ビットマップを保存する。
            HBITMAP hbm = XgCreateXWordImage(xg_xword, &siz, false);
            if (hbm != nullptr) {
                if (!XgSaveBitmapToFile(szFileName, hbm))
                {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
                }
                ::DeleteObject(hbm);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            }
        } else {
            // EMFを保存する。
            HDC hdcRef = ::GetDC(hwnd);
            HDC hdc = ::CreateEnhMetaFileW(hdcRef, szFileName, nullptr, XgLoadStringDx1(IDS_APPNAME));
            if (hdc) {
                XgDrawXWord(xg_xword, hdc, &siz, false);
                ::CloseEnhMetaFile(hdc);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            }
            ::ReleaseDC(hwnd, hdcRef);
        }
    }
}

// 解答を画像ファイルとして保存する。
void __fastcall XgSaveAnsAsImage(HWND hwnd)
{
    OPENFILENAMEW ofn;
    WCHAR szFileName[MAX_PATH] = L"";

    if (!xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // 「解答を画像ファイルとして保存」ダイアログを表示。
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_IMGFILTER));
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = ARRAYSIZE(szFileName);
    ofn.lpstrTitle = XgLoadStringDx1(IDS_SAVEANSASIMG);
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"bmp";
    if (::GetSaveFileNameW(&ofn)) {
        // 描画サイズを取得する。
        SIZE siz;
        XgGetXWordExtent(&siz);

        if (ofn.nFilterIndex <= 1) {
            // ビットマップを保存する。
            HBITMAP hbm = XgCreateXWordImage(xg_solution, &siz, false);
            if (hbm != nullptr) {
                if (!XgSaveBitmapToFile(szFileName, hbm)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
                }
                ::DeleteObject(hbm);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            }
        } else {
            // EMFを保存する。
            HDC hdcRef = ::GetDC(hwnd);
            HDC hdc = ::CreateEnhMetaFileW(hdcRef, szFileName, nullptr, XgLoadStringDx1(IDS_APPNAME));
            if (hdc) {
                XgDrawXWord(xg_solution, hdc, &siz, false);
                ::CloseEnhMetaFile(hdc);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            }
            ::ReleaseDC(hwnd, hdcRef);
        }
    }
}

// クロスワードの文字列を取得する。
void __fastcall XG_Board::GetString(std::wstring& str) const
{
    str.clear();

    str += ZEN_ULEFT;
    for (int j = 0; j < xg_nCols; j++)
        str += ZEN_HLINE;
    str += ZEN_URIGHT;
    str += L"\r\n";

    for (int i = 0; i < xg_nRows; i++) {
        str += ZEN_VLINE;
        for (int j = 0; j < xg_nCols; j++)
        {
            str += GetAt(i, j);
        }
        str += ZEN_VLINE;
        str += L"\r\n";
    }

    str += ZEN_LLEFT;
    for (int j = 0; j < xg_nCols; j++)
        str += ZEN_HLINE;
    str += ZEN_LRIGHT;
    str += L"\r\n";
}

// クロスワードに文字列を設定する。
bool __fastcall XG_Board::SetString(const std::wstring& strToBeSet)
{
    int i, nRows, nCols;
    std::vector<WCHAR> v;
    std::wstring str(strToBeSet);

    // ヘッダーを取得する。
    xg_strHeader.clear();
    std::wstring strHeaderSep = XgLoadStringDx1(IDS_HEADERSEP1);
    size_t i0 = str.find(strHeaderSep);
    if (i0 != std::wstring::npos) {
        xg_strHeader = str.substr(0, i0);
        str = str.substr(i0 + strHeaderSep.size());
    }
    xg_str_trim(xg_strHeader);

    // 初期化する。
    const int size = static_cast<int>(str.size());
    nRows = nCols = 0;

    // 左上の角があるか？
    for (i = 0; i < size; i++) {
        if (str[i] == ZEN_ULEFT)
            break;
    }
    if (i == size) {
        // 見つからなかった。失敗。
        return false;
    }

    // 文字列を読み込む。
    bool bFoundLastCorner = false;
    for (; i < size; i++) {
        if (str[i] == ZEN_VLINE) {
            // 左の境界線が見つかった。
            i++;    // 次の文字へ。

            // 横線以外の文字を格納する。
            while (i < size && str[i] != ZEN_VLINE) {
                v.emplace_back(str[i]);
                i++;    // 次の文字へ。
            }

            // 文字列の長さを超えたら、中断する。
            if (i >= size)
                break;

            // 右の境界線が見つかった。列数が未格納なら、格納する。
            if (nCols == 0)
                nCols = static_cast<int>(v.size());

            nRows++;    // 次の行へ。
            i++;    // 次の文字へ。
        } else if (str[i] == ZEN_LRIGHT) {
            // 右下の角が見つかった。
            bFoundLastCorner = true;
            break;
        }
        // その他の文字は読み捨て。
    }

    // きちんと読み込めたか確認する。
    if (nRows == 0 || nCols == 0 || !bFoundLastCorner) {
        // 失敗。
        return false;
    }

    // クロスワードを初期化する。
    xg_bSolved = false;
    xg_bShowAnswer = false;
    ResetAndSetSize(nRows, nCols);
    xg_nRows = nRows;
    xg_nCols = nCols;

    // 空白じゃないマスの個数を数える。
    WCHAR ch = 0;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if (v[i * xg_nCols + j] != ZEN_SPACE)
                ch++;
        }
    }
    v.emplace_back(ch);

    // マス情報を格納する。
    m_vCells = v;

    if (xg_imode != xg_im_ANY) { // 自由入力でなければ
        // マスの文字の種類に応じて入力モードを切り替える。
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                ch = GetAt(i, j);
                if (XgIsCharHankakuAlphaW(ch) || XgIsCharZenkakuAlphaW(ch)) {
                    xg_imode = xg_im_ABC;
                    goto break2;
                }
                if (XgIsCharKanaW(ch) || ch == ZEN_PROLONG) {
                    xg_imode = xg_im_KANA;
                    goto break2;
                }
                if (XgIsCharKanjiW(ch)) {
                    xg_imode = xg_im_KANJI;
                    goto break2;
                }
                if (XgIsCharZenkakuCyrillicW(ch)) {
                    xg_imode = xg_im_RUSSIA;
                    goto break2;
                }
                if (XgIsCharGreekW(ch)) {
                    xg_imode = xg_im_GREEK;
                    goto break2;
                }
                if (XgIsCharZenkakuNumericW(ch) || XgIsCharHankakuNumericW(ch)) {
                    xg_imode = xg_im_DIGITS;
                    goto break2;
                }
            }
        }
break2:;
    }

    // カギをクリアする。
    xg_vTateInfo.clear();
    xg_vYokoInfo.clear();

    // 成功。
    return true;
}

//////////////////////////////////////////////////////////////////////////////

// 黒マスが線対称か？
bool XG_Board::IsLineSymmetry() const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if ((GetAt(i, j) == ZEN_BLACK) !=
                (GetAt(nRows - i - 1, j) == ZEN_BLACK))
            {
                goto skip01;
            }
        }
    }
    return true;
skip01:;

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if ((GetAt(i, j) == ZEN_BLACK) !=
                (GetAt(i, xg_nCols - j - 1) == ZEN_BLACK))
            {
                goto skip02;
            }
        }
    }
    return true;
skip02:;

    if (nRows != nCols)
        return false;

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if ((GetAt(i, j) == ZEN_BLACK) !=
                (GetAt(j, i) == ZEN_BLACK))
            {
                goto skip03;
            }
        }
    }
    return true;
skip03:;

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if ((GetAt(i, j) == ZEN_BLACK) !=
                (GetAt(nRows - j - 1, nCols - i - 1) == ZEN_BLACK))
            {
                goto skip04;
            }
        }
    }
    return true;
skip04:;

    return false;
}

// 黒マスが点対称か？
bool XG_Board::IsPointSymmetry() const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if ((GetAt(i, j) == ZEN_BLACK) !=
                (GetAt(nRows - (i + 1), nCols - (j + 1)) == ZEN_BLACK))
            {
                return false;
            }
        }
    }
    return true;
}

// 黒マスが線対称（タテ）か？
bool XG_Board::IsLineSymmetryV() const
{
    const int nRows = xg_nRows;
    const int nHalfRows = nRows / 2;
    const int nCols = xg_nCols;
    for (int i = 0; i < nHalfRows; i++) {
        for (int j = 0; j < nCols; j++) {
            if ((GetAt(i, j) == ZEN_BLACK) != (GetAt(nRows - (i + 1), j) == ZEN_BLACK))
                return false;
        }
    }
    return true;
}

// 黒マスが線対称（ヨコ）か？
bool XG_Board::IsLineSymmetryH() const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    const int nHalfCols = nCols / 2;
    for (int j = 0; j < nHalfCols; j++) {
        for (int i = 0; i < nRows; i++) {
            if ((GetAt(i, j) == ZEN_BLACK) != (GetAt(i, nCols - (j + 1)) == ZEN_BLACK))
                return false;
        }
    }
    return true;
}

// 必要ならルールに従って対称にする。
void XG_Board::Mirror()
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    if (xg_nRules & RULE_POINTSYMMETRY) {
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                if (GetAt(i, j) == ZEN_BLACK)
                    SetAt(nRows - (i + 1), nCols - (j + 1), ZEN_BLACK);
            }
        }
    } else if (xg_nRules & RULE_LINESYMMETRYV) {
        for (int i = 0; i < nRows; i++) {
            for (int j = 0; j < nCols; j++) {
                if (GetAt(i, j) == ZEN_BLACK)
                    SetAt(nRows - (i + 1), j, ZEN_BLACK);
            }
        }
    } else if (xg_nRules & RULE_LINESYMMETRYH) {
        for (int j = 0; j < nCols; j++) {
            for (int i = 0; i < nRows; i++) {
                if (GetAt(i, j) == ZEN_BLACK)
                    SetAt(i, nCols - (j + 1), ZEN_BLACK);
            }
        }
    }
}

// 黒斜三連か？
bool XG_Board::ThreeDiagonals() const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    for (int i = 0; i < nRows - 2; i++) {
        for (int j = 0; j < nCols - 2; j++) {
            if (GetAt(i, j) != ZEN_BLACK)
                continue;
            if (GetAt(i + 1, j + 1) != ZEN_BLACK)
                continue;
            if (GetAt(i + 2, j + 2) != ZEN_BLACK)
                continue;
            return true;
        }
    }
    for (int i = 0; i < nRows - 2; i++) {
        for (int j = 2; j < nCols; j++) {
            if (GetAt(i, j) != ZEN_BLACK)
                continue;
            if (GetAt(i + 1, j - 1) != ZEN_BLACK)
                continue;
            if (GetAt(i + 2, j - 2) != ZEN_BLACK)
                continue;
            return true;
        }
    }
    return false;
}

// 黒斜四連か？
bool XG_Board::FourDiagonals() const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    for (int i = 0; i < nRows - 3; i++) {
        for (int j = 0; j < nCols - 3; j++) {
            if (GetAt(i, j) != ZEN_BLACK)
                continue;
            if (GetAt(i + 1, j + 1) != ZEN_BLACK)
                continue;
            if (GetAt(i + 2, j + 2) != ZEN_BLACK)
                continue;
            if (GetAt(i + 3, j + 3) != ZEN_BLACK)
                continue;
            return true;
        }
    }
    for (int i = 0; i < nRows - 3; i++) {
        for (int j = 3; j < nCols; j++) {
            if (GetAt(i, j) != ZEN_BLACK)
                continue;
            if (GetAt(i + 1, j - 1) != ZEN_BLACK)
                continue;
            if (GetAt(i + 2, j - 2) != ZEN_BLACK)
                continue;
            if (GetAt(i + 3, j - 3) != ZEN_BLACK)
                continue;
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////////
// 黒マスパターンを生成する。

// 黒マスパターンが生成されたか？
bool xg_bBlacksGenerated = false;
// 配置できる最大単語長。
INT xg_nMaxWordLen = 4;

// 黒マスパターンを生成する。
bool __fastcall XgGenerateBlacksRecurse(const XG_Board& xword, LONG iRowjCol)
{
    // ルールの適合性をチェックする。
    if ((xg_nRules & RULE_DONTCORNERBLACK) && xword.CornerBlack())
        return false;
    if ((xg_nRules & RULE_DONTDOUBLEBLACK) && xword.DoubleBlack())
        return false;
    if ((xg_nRules & RULE_DONTTRIDIRECTIONS) && xword.TriBlackAround())
        return false;
    if ((xg_nRules & RULE_DONTDIVIDE) && xword.DividedByBlack())
        return false;
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (xword.ThreeDiagonals())
            return false;
    } else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
        if (xword.FourDiagonals())
            return false;
    }

    const int nRows = xg_nRows, nCols = xg_nCols;
    INT iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);
    // 終了条件。
    if (iRow == nRows && jCol == 0) {
        EnterCriticalSection(&xg_cs);
        if (!xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_cs);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの左側を見る。
    INT jLeft;
    for (jLeft = jCol; jLeft > 0; --jLeft) {
        if (xword.GetAt(iRow, jLeft - 1) == ZEN_BLACK) {
            break;
        }
    }
    if (jCol - jLeft + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
        return false;
    }
    // マスの上側を見る。
    INT iTop;
    for (iTop = iRow; iTop > 0; --iTop) {
        if (xword.GetAt(iTop - 1, jCol) == ZEN_BLACK) {
            break;
        }
    }
    if (iRow - iTop + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
        return false;
    }

    if (rand() < RAND_MAX / 2) { // 1/2の確率で。。。
        // 黒マスをセットして再帰。
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksRecurse(xword, MAKELONG(iRow, jCol + 1)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksRecurse(xword, MAKELONG(iRow + 1, 0)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
    } else {
        // 黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
            if (XgGenerateBlacksRecurse(xword, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
            if (XgGenerateBlacksRecurse(xword, MAKELONG(iRow + 1, 0)))
                return true;
        }
    }
    return false;
}

// 黒マスパターンを生成する（点対称）。
bool __fastcall XgGenerateBlacksPointSymRecurse(const XG_Board& xword, LONG iRowjCol)
{
    // ルールの適合性をチェックする。
    if ((xg_nRules & RULE_DONTCORNERBLACK) && xword.CornerBlack())
        return false;
    if ((xg_nRules & RULE_DONTDOUBLEBLACK) && xword.DoubleBlack())
        return false;
    if ((xg_nRules & RULE_DONTTRIDIRECTIONS) && xword.TriBlackAround())
        return false;
    if ((xg_nRules & RULE_DONTDIVIDE) && xword.DividedByBlack())
        return false;
    if (xg_nRules & RULE_DONTTHREEDIAGONALS)
    {
        if (xword.ThreeDiagonals())
            return false;
    }
    else if (xg_nRules & RULE_DONTFOURDIAGONALS)
    {
        if (xword.FourDiagonals())
            return false;
    }

    const int nRows = xg_nRows, nCols = xg_nCols;
    INT iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);

    // 終了条件。
    if (iRow == nRows && jCol == 0) {
        EnterCriticalSection(&xg_cs);
        if (!xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_cs);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの左側を見る。
    INT jLeft;
    for (jLeft = jCol; jLeft > 0; --jLeft) {
        if (xword.GetAt(iRow, jLeft - 1) == ZEN_BLACK) {
            break;
        }
    }
    if (jCol - jLeft + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(nRows - (iRow + 1), nCols - (jCol + 1), ZEN_BLACK);
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
        return false;
    }
    // マスの上側を見る。
    INT iTop;
    for (iTop = iRow; iTop > 0; --iTop) {
        if (xword.GetAt(iTop - 1, jCol) == ZEN_BLACK) {
            break;
        }
    }
    if (iRow - iTop + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(nRows - (iRow + 1), nCols - (jCol + 1), ZEN_BLACK);
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
        return false;
    }

    if (rand() < RAND_MAX / 2) { // 1/2の確率で。。。
        // 黒マスをセットして再帰。
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksPointSymRecurse(xword, MAKELONG(iRow, jCol + 1)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(nRows - (iRow + 1), nCols - (jCol + 1), ZEN_BLACK);
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksPointSymRecurse(xword, MAKELONG(iRow + 1, 0)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(nRows - (iRow + 1), nCols - (jCol + 1), ZEN_BLACK);
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
    } else {
        // 黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(nRows - (iRow + 1), nCols - (jCol + 1), ZEN_BLACK);
        if (jCol + 1 < nCols) {
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
            if (XgGenerateBlacksPointSymRecurse(xword, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksPointSymRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
            if (XgGenerateBlacksPointSymRecurse(xword, MAKELONG(iRow + 1, 0)))
                return true;
        }
    }
    return false;
}

// 黒マスパターンを生成する（タテ線対称）。
bool __fastcall XgGenerateBlacksLineSymVRecurse(const XG_Board& xword, LONG iRowjCol)
{
    // ルールの適合性をチェックする。
    if ((xg_nRules & RULE_DONTCORNERBLACK) && xword.CornerBlack())
        return false;
    if ((xg_nRules & RULE_DONTDOUBLEBLACK) && xword.DoubleBlack())
        return false;
    if ((xg_nRules & RULE_DONTTRIDIRECTIONS) && xword.TriBlackAround())
        return false;
    if ((xg_nRules & RULE_DONTDIVIDE) && xword.DividedByBlack())
        return false;
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (xword.ThreeDiagonals())
            return false;
    } else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
        if (xword.FourDiagonals())
            return false;
    }

    const INT nRows = xg_nRows, nCols = xg_nCols;
    const INT nHalfRows = nRows / 2;
    INT iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);

    // 終了条件。
    if (iRow == 0 && jCol >= nCols) {
        EnterCriticalSection(&xg_cs);
        if (!xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_cs);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの上側を見る。
    INT iTop;
    for (iTop = iRow; iTop > 0; --iTop) {
        if (xword.GetAt(iTop - 1, jCol) == ZEN_BLACK) {
            break;
        }
    }
    if (iRow - iTop + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(nRows - (iRow + 1), jCol, ZEN_BLACK);
        if (iRow + 1 <= nHalfRows) {
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(iRow + 1, jCol)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(0, jCol + 1)))
                return true;
        }
        return false;
    }

    // マスの左側を見る。
    INT jLeft;
    for (jLeft = jCol; jLeft > 0; --jLeft) {
        if (xword.GetAt(iRow, jLeft - 1) == ZEN_BLACK) {
            break;
        }
    }
    if (jCol - jLeft + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(nRows - (iRow + 1), jCol, ZEN_BLACK);
        if (iRow + 1 <= nHalfRows) {
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(iRow + 1, jCol)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(0, jCol + 1)))
                return true;
        }
        return false;
    }

    // 盤の真ん中か？
    if (iRow >= nHalfRows) {
        std::wstring strPattern = xword.GetPatternV(XG_Pos(iRow, jCol));
        if ((INT)strPattern.size() > xg_nMaxWordLen) {
            // 空白が最大長よりも長い。黒マスをセットして再帰。
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(nRows - (iRow + 1), jCol, ZEN_BLACK);
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(0, jCol + 1)))
                return true;
            return false;
        }
    }

    if (rand() < RAND_MAX / 2) { // 1/2の確率で。。。
        // 黒マスをセットして再帰。
        if (iRow + 1 <= nHalfRows) {
            if (XgGenerateBlacksLineSymVRecurse(xword, MAKELONG(iRow + 1, jCol)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(nRows - (iRow + 1), jCol, ZEN_BLACK);
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(iRow + 1, jCol)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymVRecurse(xword, MAKELONG(0, jCol + 1)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(nRows - (iRow + 1), jCol, ZEN_BLACK);
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(0, jCol + 1)))
                return true;
        }
    } else {
        // 黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(nRows - (iRow + 1), jCol, ZEN_BLACK);
        if (iRow + 1 <= nHalfRows) {
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(iRow + 1, jCol)))
                return true;
            if (XgGenerateBlacksLineSymVRecurse(xword, MAKELONG(iRow + 1, jCol)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymVRecurse(copy, MAKELONG(0, jCol + 1)))
                return true;
            if (XgGenerateBlacksLineSymVRecurse(xword, MAKELONG(0, jCol + 1)))
                return true;
        }
    }
    return false;
}

// 黒マスパターンを生成する（ヨコ線対称）。
bool __fastcall XgGenerateBlacksLineSymHRecurse(const XG_Board& xword, LONG iRowjCol)
{
    // ルールの適合性をチェックする。
    if ((xg_nRules & RULE_DONTCORNERBLACK) && xword.CornerBlack())
        return false;
    if ((xg_nRules & RULE_DONTDOUBLEBLACK) && xword.DoubleBlack())
        return false;
    if ((xg_nRules & RULE_DONTTRIDIRECTIONS) && xword.TriBlackAround())
        return false;
    if ((xg_nRules & RULE_DONTDIVIDE) && xword.DividedByBlack())
        return false;
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        if (xword.ThreeDiagonals())
            return false;
    } else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
        if (xword.FourDiagonals())
            return false;
    }

    const INT nRows = xg_nRows, nCols = xg_nCols;
    const INT nHalfCols = nCols / 2;
    INT iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);

    // 終了条件。
    if (iRow >= nRows && jCol == 0) {
        EnterCriticalSection(&xg_cs);
        if (!xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_cs);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの左側を見る。
    INT jLeft;
    for (jLeft = jCol; jLeft > 0; --jLeft) {
        if (xword.GetAt(iRow, jLeft - 1) == ZEN_BLACK) {
            break;
        }
    }
    if (jCol - jLeft + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(iRow, nCols - (jCol + 1), ZEN_BLACK);
        if (jCol + 1 <= nHalfCols) {
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
        return false;
    }

    // マスの上側を見る。
    INT iTop;
    for (iTop = iRow; iTop > 0; --iTop) {
        if (xword.GetAt(iTop - 1, jCol) == ZEN_BLACK) {
            break;
        }
    }
    if (iRow - iTop + 1 > xg_nMaxWordLen) {
        // 空白が最大長よりも長い。黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(iRow, nCols - (jCol + 1), ZEN_BLACK);
        if (jCol + 1 <= nHalfCols) {
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
        return false;
    }

    // 盤の真ん中か？
    if (jCol >= nHalfCols) {
        std::wstring strPattern = xword.GetPatternH(XG_Pos(iRow, jCol));
        if ((INT)strPattern.size() > xg_nMaxWordLen) {
            // 空白が最大長よりも長い。黒マスをセットして再帰。
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(iRow, nCols - (jCol + 1), ZEN_BLACK);
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
            return false;
        }
    }

    if (rand() < RAND_MAX / 2) { // 1/2の確率で。。。
        // 黒マスをセットして再帰。
        if (jCol + 1 <= nHalfCols) {
            if (XgGenerateBlacksLineSymHRecurse(xword, MAKELONG(iRow, jCol + 1)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(iRow, nCols - (jCol + 1), ZEN_BLACK);
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymHRecurse(xword, MAKELONG(iRow + 1, 0)))
                return true;
            XG_Board copy(xword);
            copy.SetAt(iRow, jCol, ZEN_BLACK);
            copy.SetAt(iRow, nCols - (jCol + 1), ZEN_BLACK);
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
        }
    } else {
        // 黒マスをセットして再帰。
        XG_Board copy(xword);
        copy.SetAt(iRow, jCol, ZEN_BLACK);
        copy.SetAt(iRow, nCols - (jCol + 1), ZEN_BLACK);
        if (jCol + 1 <= nHalfCols) {
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow, jCol + 1)))
                return true;
            if (XgGenerateBlacksLineSymHRecurse(xword, MAKELONG(iRow, jCol + 1)))
                return true;
        } else {
            if (XgGenerateBlacksLineSymHRecurse(copy, MAKELONG(iRow + 1, 0)))
                return true;
            if (XgGenerateBlacksLineSymHRecurse(xword, MAKELONG(iRow + 1, 0)))
                return true;
        }
    }
    return false;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacks(void *param)
{
    XG_Board xword;
    xg_solution.clear();

    // 乱数をかく乱する。
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());

    // 再帰求解関数に突入する。
    do {
        if (xg_bBlacksGenerated || xg_bCancelled)
            break;
        xword.clear();
    } while (!XgGenerateBlacksRecurse(xword, 0));
    return 1;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksSmart(void *param)
{
    if (xg_bBlacksGenerated)
        return 1;

    XG_Board xword;

    // 乱数をかく乱する。
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());

    if (xg_nRules & RULE_POINTSYMMETRY) {
        // 再帰求解関数に突入する。
        do {
            if (xg_bCancelled || xg_bBlacksGenerated)
                break;
            xword.clear();
        } while (!XgGenerateBlacksPointSymRecurse(xword, 0));
    } else if (xg_nRules & RULE_LINESYMMETRYV) {
        // 再帰求解関数に突入する。
        do {
            if (xg_bCancelled || xg_bBlacksGenerated)
                break;
            xword.clear();
        } while (!XgGenerateBlacksLineSymVRecurse(xword, 0));
    } else if (xg_nRules & RULE_LINESYMMETRYH) {
        // 再帰求解関数に突入する。
        do {
            if (xg_bCancelled || xg_bBlacksGenerated)
                break;
            xword.clear();
        } while (!XgGenerateBlacksLineSymHRecurse(xword, 0));
    } else {
        // 再帰求解関数に突入する。
        do {
            if (xg_bCancelled || xg_bBlacksGenerated)
                break;
            xword.clear();
        } while (!XgGenerateBlacksRecurse(xword, 0));
    }
    return 1;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksPointSym(void *param)
{
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());
    xg_solution.clear();
    XG_Board xword;
    do {
        if (xg_bBlacksGenerated || xg_bCancelled)
            break;
        xword.clear();
    } while (!XgGenerateBlacksPointSymRecurse(xword, 0));
    return 1;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksLineSymV(void *param)
{
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());
    xg_solution.clear();
    XG_Board xword;
    do {
        if (xg_bBlacksGenerated || xg_bCancelled)
            break;
        xword.clear();
    } while (!XgGenerateBlacksLineSymVRecurse(xword, 0));
    return 1;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksLineSymH(void *param)
{
    srand(DWORD(::GetTickCount64()) ^ ::GetCurrentThreadId());
    xg_solution.clear();
    XG_Board xword;
    do {
        if (xg_bBlacksGenerated || xg_bCancelled)
            break;
        xword.clear();
    } while (!XgGenerateBlacksLineSymHRecurse(xword, 0));
    return 1;
}

void __fastcall XgStartGenerateBlacks(void)
{
    xg_bBlacksGenerated = false;
    xg_bCancelled = false;

    // 最大長を制限する。
    if (xg_nMaxWordLen > xg_nDictMaxWordLen) {
        xg_nMaxWordLen = xg_nDictMaxWordLen;
    }

    // スレッドを開始する。
    if (xg_nRules & RULE_POINTSYMMETRY) {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacksPointSym(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            xg_ahThreads[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksPointSym, &xg_aThreadInfo[i], 0,
                    &xg_aThreadInfo[i].m_threadid));
            assert(xg_ahThreads[i] != nullptr);
        }
#endif
    } else if (xg_nRules & RULE_LINESYMMETRYV) {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacksLineSymV(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            xg_ahThreads[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksLineSymV, &xg_aThreadInfo[i], 0,
                    &xg_aThreadInfo[i].m_threadid));
            assert(xg_ahThreads[i] != nullptr);
        }
#endif
    } else if (xg_nRules & RULE_LINESYMMETRYH) {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacksLineSymH(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            xg_ahThreads[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksLineSymH, &xg_aThreadInfo[i], 0,
                    &xg_aThreadInfo[i].m_threadid));
            assert(xg_ahThreads[i] != nullptr);
        }
#endif
    } else {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacks(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            xg_ahThreads[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacks, &xg_aThreadInfo[i], 0,
                    &xg_aThreadInfo[i].m_threadid));
            assert(xg_ahThreads[i] != nullptr);
        }
#endif
    }
}

// クロスワードで使う文字に変換する。
std::wstring __fastcall XgNormalizeString(const std::wstring& text) {
    WCHAR szText[512];
    LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
        text.c_str(), -1, szText, 512);
    std::wstring ret = szText;
    for (auto& ch : ret) {
        // 小さな字を大きな字にする。
        for (size_t i = 0; i < ARRAYSIZE(xg_small); i++) {
            if (ch == xg_small[i][0]) {
                ch = xg_large[i][0];
                break;
            }
        }
    }
    return ret;
}

// タテ向きにパターンを読み取る。
std::wstring __fastcall XG_Board::GetPatternV(const XG_Pos& pos) const
{
    int lo, hi;
    std::wstring pattern;
    if (GetAt(pos.m_i, pos.m_j) == ZEN_BLACK)
        return pattern;

    lo = hi = pos.m_i;
    while (lo > 0) {
        if (GetAt(lo - 1, pos.m_j) != ZEN_BLACK)
            --lo;
        else
            break;
    }
    while (hi + 1 < xg_nRows) {
        if (GetAt(hi + 1, pos.m_j) != ZEN_BLACK)
            ++hi;
        else
            break;
    }

    for (int i = lo; i <= hi; ++i) {
        pattern += GetAt(i, pos.m_j);
    }

    return pattern;
}

// ヨコ向きにパターンを読み取る。
std::wstring __fastcall XG_Board::GetPatternH(const XG_Pos& pos) const
{
    int lo, hi;
    std::wstring pattern;
    if (GetAt(pos.m_i, pos.m_j) == ZEN_BLACK)
        return pattern;

    lo = hi = pos.m_j;
    while (lo > 0) {
        if (GetAt(pos.m_i, lo - 1) != ZEN_BLACK)
            --lo;
        else
            break;
    }
    while (hi + 1 < xg_nCols) {
        if (GetAt(pos.m_i, hi + 1) != ZEN_BLACK)
            ++hi;
        else
            break;
    }

    for (int j = lo; j <= hi; ++j) {
        pattern += GetAt(pos.m_i, j);
    }

    return pattern;
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
void __fastcall XgWaitForThreads(void)
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
