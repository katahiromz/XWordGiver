//////////////////////////////////////////////////////////////////////////////
// xword.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2019 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////
// global variables

// 直前に開いたクロスワードデータファイルのパスファイル名。
std::wstring xg_strFileName;

// 再計算しなおしているか？
bool xg_bRetrying = false;

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
DWORD               xg_dwThreadCount;

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

// クロスワードのサイズ。
int                 xg_nRows = 0;
int                 xg_nCols = 0;

// タテとヨコのかぎ。
std::vector<XG_PlaceInfo> xg_vTateInfo, xg_vYokoInfo;

// 拗音変換用データ。
const std::array<LPCWSTR,11>    xg_small = 
{
    {
        L"\x30A1", L"\x30A3", L"\x30A5", L"\x30A7", L"\x30A9", L"\x30C3",
        L"\x30E3", L"\x30E5", L"\x30E7", L"\x30F5", L"\x30F6"
    }
};
const std::array<LPCWSTR,11>    xg_large = 
{
    {
        L"\x30A2", L"\x30A4", L"\x30A6", L"\x30A8", L"\x30AA", L"\x30C4",
        L"\x30E4", L"\x30E6", L"\x30E8", L"\x30AB", L"\x30B1",
    }
};

// クロスワードの問題。
XG_Board    xg_xword;

// クロスワードの解。
XG_Board    xg_solution;

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
bool xg_bCharFeed = false;

// タテ入力？
bool xg_bTateInput = false;

//////////////////////////////////////////////////////////////////////////////
// static variables

// 縦と横を反転しているか？
static bool s_bSwapped = false;

// カギの答えを囲む枠。
static const LPCWSTR s_szBeginWord = L"\x226A", s_szEndWord = L"\x226B";

//////////////////////////////////////////////////////////////////////////////

// 候補があるか？
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
    for (const auto& data : xg_dict_data) {
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
    for (const auto& data : xg_dict_data) {
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

// 候補があるか？（黒マス追加なし、すべて空白）
bool __fastcall XgAnyCandidateWholeSpace(int patlen)
{
    // すべての単語について調べる。
    for (const auto& data : xg_dict_data) {
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
bool __fastcall XG_Board::TriBlackArround() const
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
    const int nRows = xg_nRows, nCols = xg_nCols;
    int nCount = nRows * nCols;

    // 各マスに対応するフラグ群。
    LPBYTE pb = new BYTE[nCount];

    // フラグをすべてゼロにする。
    memset(pb, 0, nCount);

    // 位置のキュー。
    // 一番左上のマス(黒マスではないと仮定する)を追加。
    std::queue<XG_Pos> positions;
    positions.emplace(0, 0);

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
            delete[] pb;
            return true;    // 分断されている。
        }
    }

    // フラグ群を解放。
    delete[] pb;

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
                        if (!XgAnyCandidateNoAddBlack(pattern)) {
                            // マッチしなかった位置。
                            pos = XG_Pos(i, lo);
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack(pattern)) {
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
                    if (!XgWordDataExists(XG_WordData(pattern))) {
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
                    if (!XgAnyCandidateWholeSpace(patlen))
                    {
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
                        if (!XgAnyCandidateNoAddBlack(pattern)) {
                            // マッチしなかった位置。
                            pos = XG_Pos(lo, j);
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack(pattern)) {
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
                    if (!XgWordDataExists(XG_WordData(pattern))) {
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
                        if (!XgAnyCandidateNoAddBlack(pattern)) {
                            // マッチしなかった位置。
                            j = lo;
                            pos = pos2;
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack(pattern)) {
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
                    if (!XgWordDataExists(XG_WordData(pattern))) {
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
                        if (!XgAnyCandidateNoAddBlack(pattern)) {
                            // マッチしなかった位置。
                            i = lo;
                            pos = pos2;
                            return xg_epv_PATNOTMATCH;  // マッチしなかった。
                        }
                    } else {
                        if (!XgAnyCandidateAddBlack(pattern)) {
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
                    if (!XgWordDataExists(XG_WordData(pattern))) {
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

// 正当などうか？
inline bool __fastcall XG_Board::IsValid() const
{
    // 四隅には黒マスは置けません。
    // 連黒禁。
    // 三方向が黒マスで囲まれたマスを作ってはいけません。
    if (CornerBlack() || DoubleBlack() || TriBlackArround())
        return false;

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

// 正当などうか？（簡略版）
inline bool __fastcall XG_Board::IsOK() const
{
    if (0)
    {
        // 四隅には黒マスは置けません。
        // 連黒禁。
        // 三方向が黒マスで囲まれたマスを作ってはいけません。
        if (CornerBlack() || DoubleBlack() || TriBlackArround())
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
                if (XgWordDataExists(wd)) {
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
                if (XgWordDataExists(wd)) {
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
bool __fastcall XgGetCandidatesAddBlack(
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
        if (pattern[i] != ZEN_SPACE)
            bSpaceFound = true;
    }
    assert(bSpaceFound);
#endif

    // 候補をクリアする。
    cands.clear();
    // スピードのために予約する。
    cands.reserve(xg_dict_data.size() / 32);

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
    for (const auto& data : xg_dict_data) {
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
bool __fastcall
XgGetCandidatesNoAddBlack(
    std::vector<std::wstring>& cands, const std::wstring& pattern)
{
    // 単語の長さ。
    const int patlen = static_cast<int>(pattern.size());
    assert(patlen >= 2);

    // 候補をクリアする。
    cands.clear();
    // スピードのため、予約する。
    cands.reserve(xg_dict_data.size() / 32);

    // すべての登録された単語について。
    for (const auto& data : xg_dict_data) {
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
    assert(IsValid());

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

#if 0   // すでにIsValidメソッドで確認済み。
    // 四隅には黒マスは置けません。
    // 連黒禁。
    // 三方向が黒マスで囲まれたマスを作ってはいけません。
    if (CornerBlack() || DoubleBlack() || TriBlackArround())
        return false;

    // 空のクロスワードを解いているときは、分断禁をチェックする必要はない。
    if (!xg_bSolvingEmpty) {
        // 分断禁。
        if (DividedByBlack())
            return false;
    }
#endif  // 0

    return true;
}

// ヒント文字列を解析する。
bool __fastcall XgParseHints(
    std::vector<XG_Hint>& hints, const std::wstring& str)
{
    // ヒントをクリアする。
    hints.clear();

    int count = 0;
    for (size_t i = 0;;) {
        if (count++ > 1000) {
            return false;
        }

        size_t i0 = str.find(XgLoadStringDx1(98), i);
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

        size_t i2 = str.find(XgLoadStringDx1(94), i0);
        if (i2 == std::wstring::npos) {
            return false;
        }

        std::wstring word;
        size_t i3, i4;
        i3 = str.find(s_szBeginWord, i2);
        if (i3 != std::wstring::npos) {
            i3 += wcslen(s_szBeginWord);

            i4 = str.find(s_szEndWord, i3);
            if (i4 == std::wstring::npos) {
                return false;
            }
            word = str.substr(i3, i4 - i3);
            i4 += wcslen(s_szEndWord);
        } else {
            i4 = i2 + wcslen(XgLoadStringDx1(94));
        }

        size_t i5 = str.find(XgLoadStringDx1(98), i4);
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
bool __fastcall XgParseHintsStr(HWND hwnd, const std::wstring& strHints)
{
    // ヒントをクリアする。
    xg_vecTateHints.clear();
    xg_vecYokoHints.clear();

    // ヒント文字列の前後の空白を取り除く。
    std::wstring str(strHints);
    xg_str_trim(str);

    // strCaption1とstrCaption2により、tateとyokoに分ける。
    std::wstring strCaption1 = XgLoadStringDx1(22);
    std::wstring strCaption2 = XgLoadStringDx1(23);
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
    size_t i3 = yoko.find(XgLoadStringDx1(82));
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

// JSON文字列を設定する。
bool __fastcall XgSetJsonString(HWND hwnd, const std::wstring& str)
{
    std::string utf8 = XgUnicodeToUtf8(str);

    using namespace picojson;
    value root;

    std::string err;
    parse(root, utf8.begin(), utf8.end(), &err);
    if (err.size() || !root.is<object>()) {
        return false;
    }

    object root_obj = root.get<object>();
    int row_count = static_cast<int>(root_obj["row_count"].get<int64_t>());
    int column_count = static_cast<int>(root_obj["column_count"].get<int64_t>());
    picojson::array cell_data = root_obj["cell_data"].get<picojson::array>();
    bool is_solved = root_obj["is_solved"].get<bool>();
    bool has_mark = root_obj["has_mark"].get<bool>();
    bool has_hints = root_obj["has_hints"].get<bool>();

    if (row_count <= 0 || column_count <= 0) {
        return false;
    }

    if (row_count != static_cast<int>(cell_data.size())) {
        return false;
    }

    XG_Board xw;
    int nRowsSave = xg_nRows, nColsSave = xg_nCols;
    xg_nRows = row_count;
    xg_nCols = column_count;
    xw.ResetAndSetSize(row_count, column_count);

    bool success = true;

    for (int i = 0; i < row_count; ++i) {
        std::wstring row = XgUtf8ToUnicode(cell_data[i].get<std::string>());
        if (static_cast<int>(row.size()) != column_count) {
            success = false;
            break;
        }
        for (int j = 0; j < column_count; ++j) {
            xw.SetAt(i, j, row[j]);
        }
    }

    std::vector<XG_Pos> mark_positions;
    if (has_mark) {
        picojson::array marks = root_obj["marks"].get<picojson::array>();
        for (size_t k = 0; k < marks.size(); ++k) {
            picojson::array mark = marks[k].get<picojson::array>();
            int i = static_cast<int>(mark[0].get<int64_t>()) - 1;
            int j = static_cast<int>(mark[1].get<int64_t>()) - 1;
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
        object hints = root_obj["hints"].get<object>();
        picojson::array v = hints["v"].get<picojson::array>();
        picojson::array h = hints["h"].get<picojson::array>();
        for (size_t i = 0; i < v.size(); ++i) {
            picojson::array data = v[i].get<picojson::array>();
            int number = static_cast<int>(data[0].get<int64_t>());
            if (number <= 0) {
                success = false;
                break;
            }
            std::wstring word = XgUtf8ToUnicode(data[1].get<std::string>());
            std::wstring hint = XgUtf8ToUnicode(data[2].get<std::string>());
            tate.emplace_back(number, word, hint);
        }
        for (size_t i = 0; i < h.size(); ++i) {
            picojson::array data = h[i].get<picojson::array>();
            int number = static_cast<int>(data[0].get<int64_t>());
            if (number <= 0) {
                success = false;
                break;
            }
            std::wstring word = XgUtf8ToUnicode(data[1].get<std::string>());
            std::wstring hint = XgUtf8ToUnicode(data[2].get<std::string>());
            yoko.emplace_back(number, word, hint);
        }
    }

    std::string header = root_obj["header"].get<std::string>();
    std::string notes = root_obj["notes"].get<std::string>();

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
            xg_solution.GetHintsStr(xg_strHints, 2, true);
        } else {
            xg_bSolved = false;
            xg_xword = xw;
        }
        xg_vMarks = mark_positions;
        xg_vecTateHints = tate;
        xg_vecYokoHints = yoko;
        xg_strHeader = XgUtf8ToUnicode(header);
        xg_str_trim(xg_strHeader);
        xg_strNotes = XgUtf8ToUnicode(notes);
        xg_str_trim(xg_strNotes);

        LPCWSTR psz = XgLoadStringDx1(83);
        if (xg_strNotes.empty()) {
            ;
        } else if (xg_strNotes.find(psz) == 0) {
            xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
            xg_str_trim(xg_strNotes);
        }

        // ヒント追加フラグをクリアする。
        xg_bHintsAdded = false;

        if (is_solved) {
            // ヒントを表示する。
            XgShowHints(hwnd);
        }
    } else {
        xg_nRows = nRowsSave;
        xg_nCols = nColsSave;
    }

    return success;
}

// 文字列を設定する。
bool __fastcall
XgSetString(HWND hwnd, const std::wstring& str, bool json)
{
    if (json) {
        // JSON形式。
        return XgSetJsonString(hwnd, str);
    }

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
            std::wstring strFooterSep = XgLoadStringDx1(82);
            size_t i3 = s.find(strFooterSep);
            if (i3 != std::wstring::npos) {
                xg_strNotes = s.substr(i3 + strFooterSep.size());
                s = s.substr(0, i3);
            }

            LPCWSTR psz = XgLoadStringDx1(83);
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
        if (xg_strHints.empty() || !XgParseHintsStr(hwnd, xg_strHints)) {
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
                        hint.m_strWord = word;
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
                        hint.m_strWord = word;
                        break;
                    }
                }
            }

            // ヒントを表示する。
            XgShowHints(hwnd);
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

            xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);
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

// ヒントを取得する。
void __fastcall XG_Board::GetHintsStr(
    std::wstring& str, int hint_type, bool bShowAnswer) const
{
    // 文字列バッファ。
    std::array<WCHAR,64> sz;

    // 初期化。
    str.clear();

    // まだ解かれていない場合は、何も返さない。
    if (!xg_bSolved)
        return;

    // ヒントに変更があれば、更新する。
    if (XgAreHintsModified()) {
        XgUpdateHintsData();
        XgUpdateDictData();
    }

    assert(hint_type == 0 || hint_type == 1 || hint_type == 2);

    if (hint_type == 0 || hint_type == 2) {
        // タテのカギの文字列を構成する。
        str += XgLoadStringDx1(22);
        str += xg_pszNewLine;

        for (const auto& info : xg_vTateInfo) {
            // 番号を格納する。
            ::wsprintfW(sz.data(), XgLoadStringDx1(24), info.m_number);
            str += sz.data();

            // 答えを見せるかどうか？
            if (bShowAnswer) {
                str += s_szBeginWord;
                str += info.m_word;
                str += s_szEndWord;
            }

            // ヒント文章を追加する。
            for (const auto& data : xg_dict_data) {
                if (_wcsicmp(data.m_word.data(),
                             info.m_word.data()) == 0)
                {
                    str += data.m_hint;
                    break;
                }
            }
            str += xg_pszNewLine;   // 改行。
        }
        str += xg_pszNewLine;
    }
    if (hint_type == 1 || hint_type == 2) {
        // ヨコのカギの文字列を構成する。
        str += XgLoadStringDx1(23);
        str += xg_pszNewLine;
        for (const auto& info : xg_vYokoInfo) {
            // 番号を格納する。
            ::wsprintfW(sz.data(), XgLoadStringDx1(25), info.m_number);
            str += sz.data();

            // 答えを見せるかどうか？
            if (bShowAnswer) {
                str += s_szBeginWord;
                str += info.m_word;
                str += s_szEndWord;
            }

            // ヒント文章を追加する。
            for (const auto& data : xg_dict_data) {
                if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                    str += data.m_hint;
                    break;
                }
            }
            str += xg_pszNewLine;   // 改行。
        }
        str += xg_pszNewLine;
    }
    if (hint_type == 3 || hint_type == 5) {
        // タテのカギの文字列を構成する。
        str += XgLoadStringDx1(99);     // <p><b>
        str += XgLoadStringDx1(101);
        str += XgLoadStringDx1(100);    // </b></p>
        str += xg_pszNewLine;
        str += XgLoadStringDx1(103);    // <ol>
        str += xg_pszNewLine;

        for (const auto& info : xg_vTateInfo) {
            // <li>
            ::wsprintfW(sz.data(), XgLoadStringDx1(105), info.m_number);
            str += sz.data();

            // ヒント文章を追加する。
            for (const auto& data : xg_dict_data) {
                if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                    str += data.m_hint;
                    break;
                }
            }
            str += XgLoadStringDx1(106);    // </li>
            str += xg_pszNewLine;           // 改行。
        }
        str += XgLoadStringDx1(104);    // </ol>
        str += xg_pszNewLine;           // 改行。
    }
    if (hint_type == 4 || hint_type == 5) {
        // ヨコのカギの文字列を構成する。
        str += XgLoadStringDx1(99);     // <p><b>
        str += XgLoadStringDx1(102);
        str += XgLoadStringDx1(100);    // </b></p>
        str += xg_pszNewLine;
        str += XgLoadStringDx1(103);    // <ol>
        str += xg_pszNewLine;

        for (const auto& info : xg_vYokoInfo) {
            // <li>
            ::wsprintfW(sz.data(), XgLoadStringDx1(105), info.m_number);
            str += sz.data();

            // ヒント文章を追加する。
            for (const auto& data : xg_dict_data) {
                if (_wcsicmp(data.m_word.data(), info.m_word.data()) == 0) {
                    str += data.m_hint;
                    break;
                }
            }
            str += XgLoadStringDx1(106);    // </li>
            str += xg_pszNewLine;           // 改行。
        }
        str += XgLoadStringDx1(104);    // </ol>
        str += xg_pszNewLine;           // 改行。
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
void __fastcall XgSolveXWordAddBlackRecurse(const XG_Board& xw)
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
    if (xg_bCancelled || xg_bRetrying)
        return;

    // 無効であれば、終了。
    if (!xw.IsOK())
        return;

    const int nRows = xg_nRows, nCols = xg_nCols;

    // 各行について、横向きにスキャンする。
    const int nColsMinusOne = nCols - 1;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nColsMinusOne; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                if (XgGetCandidatesAddBlack(cands, pattern, nSkip,
                                            left_black_check, right_black_check))
                {
                    // 候補の一部をかき混ぜる。
                    auto it = cands.begin();
                    std::advance(it, nSkip);
                    std::random_shuffle(it, cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled || xg_bRetrying)
                            return;

                        bool bCanPutBlack = true;
                        for (int k = lo; k <= hi; k++) {
                            if (cand[k - lo] == ZEN_BLACK && !xw.CanPutBlackEasy(i, k)) {
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
                            XgSolveXWordAddBlackRecurse(copy);
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
            if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                if (XgGetCandidatesAddBlack(cands, pattern, nSkip,
                                            left_black_check, right_black_check))
                {
                    // 候補の一部をかき混ぜる。
                    auto it = cands.begin();
                    std::advance(it, nSkip);
                    std::random_shuffle(it, cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled || xg_bRetrying)
                            return;

                        bool bCanPutBlack = true;
                        for (int k = lo; k <= hi; k++) {
                            if (cand[k - lo] == ZEN_BLACK && !xw.CanPutBlackEasy(k, j)) {
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
                            XgSolveXWordAddBlackRecurse(copy);
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
void __fastcall XgSolveXWordNoAddBlackRecurse(const XG_Board& xw)
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
    if (xg_bCancelled || xg_bRetrying)
        return;

    // 無効であれば、終了。
    if (!xw.IsOK())
        return;

    const int nRows = xg_nRows, nCols = xg_nCols;

    // 各行について、横向きにスキャンする。
    const int nColsMinusOne = nCols - 1;
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nColsMinusOne; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                if (XgGetCandidatesNoAddBlack(cands, pattern)) {
                    // 候補をかき混ぜる。
                    std::random_shuffle(cands.begin(), cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                        XgSolveXWordNoAddBlackRecurse(copy);

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
            if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                if (XgGetCandidatesNoAddBlack(cands, pattern)) {
                    // 候補をかき混ぜる。
                    std::random_shuffle(cands.begin(), cands.end());

                    for (const auto& cand : cands) {
                        // すでに解かれているなら、終了。
                        // キャンセルされているなら、終了。
                        // 再計算すべきなら、終了する。
                        if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                        XgSolveXWordNoAddBlackRecurse(copy);

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
void __fastcall XgSolveXWordAddBlack(const XG_Board& xw)
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
        XgSolveXWordAddBlackRecurse(xw);
        return;
    }

    // 無効であれば、終了。
    if (!xw.IsOK())
        return;

    // ランダムな順序の単語ベクターを作成する。
    std::vector<XG_WordData> words(xg_dict_data);
    std::random_shuffle(words.begin(), words.end());

#if 0
    // 長すぎる単語を削除する。
    {
        int n1 = xg_nRows;
        int n2 = xg_nCols;
        remove_if(words.begin(), words.end(),
                  xg_word_toolong(n1 >= n2 ? n1 : n2));
    }
#endif

    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                    if (xg_bSolved || xg_bCancelled || xg_bRetrying)
                        return;

                    // 単語の長さがパターンの長さ以下か？
                    const std::wstring& word = word_data.m_word;
                    const int wordlen = static_cast<int>(word.size());
                    if (wordlen > patlen)
                        continue;

                    // 必要な黒マスは置けるか？
                    if ((lo == 0 || xw.CanPutBlackEasy(i, lo - 1)) &&
                        (hi + 1 >= nCols || xw.CanPutBlackEasy(i, hi + 1)))
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

                        //if (copy.TriBlackArround()) {
                        //    continue;
                        //}

                        // 再帰する。
                        XgSolveXWordAddBlackRecurse(copy);
                    }
                }
                return;
            }
        }
    }
}

// 解く（黒マス追加なし）。
void __fastcall XgSolveXWordNoAddBlack(const XG_Board& xw)
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
        XgSolveXWordNoAddBlackRecurse(xw);
        return;
    }

    // 無効であれば、終了。
    if (!xw.IsOK())
        return;

    // ランダムな順序の単語ベクターを作成する。
    std::vector<XG_WordData> words(xg_dict_data);
    std::random_shuffle(words.begin(), words.end());

    // 文字マスがなかった場合。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols - 1; j++) {
            // すでに解かれているなら、終了。
            // キャンセルされているなら、終了。
            // 再計算すべきなら、終了する。
            if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                    if (xg_bSolved || xg_bCancelled || xg_bRetrying)
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
                    XgSolveXWordNoAddBlackRecurse(copy);
                }
                return;
            }
        }
    }
}

#ifdef NO_RANDOM
    int xg_random_seed = 0;
#endif

// マルチスレッド用の関数。
unsigned __stdcall XgSolveProcAddBlack(void *param)
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
    XgSolveXWordAddBlack(xg_xword);
    return 0;
}

// マルチスレッド用の関数（黒マス追加なし）。
unsigned __stdcall XgSolveProcNoAddBlack(void *param)
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
    XgSolveXWordNoAddBlack(xg_xword);
    return 0;
}

// マルチスレッド用の関数（スマート解決）。
unsigned __stdcall XgSolveProcSmart(void *param)
{
    // スレッド情報を取得する。
    XG_ThreadInfo *info = reinterpret_cast<XG_ThreadInfo *>(param);

    // 黒マスを生成する。
    XgGenerateBlacksSmart(NULL);

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
    XgSolveXWordNoAddBlack(xg_xword);
    return 0;
}

// 解を求めるのを開始。
void __fastcall XgStartSolve(void)
{
    // フラグを初期化する。
    xg_bSolved = xg_bCancelled = xg_bRetrying = false;

    if (xg_bSolvingEmpty)
        xg_xword.ResetAndSetSize(xg_nRows, xg_nCols);

    // 横より縦の方が長い場合、計算時間を減らすために、
    // 縦と横を入れ替え、後でもう一度縦と横を入れ替える。
    if (xg_nRows > xg_nCols) {
        s_bSwapped = true;
        xg_xword.SwapXandY();
        std::swap(xg_nRows, xg_nCols);
    }

    // スレッドを開始する。
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        xg_aThreadInfo[i].m_count = static_cast<DWORD>(xg_xword.Count());
        xg_ahThreads[i] = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, XgSolveProcAddBlack, &xg_aThreadInfo[i], 0,
                &xg_aThreadInfo[i].m_threadid));
        assert(xg_ahThreads[i] != nullptr);
    }
}

// 解を求めるのを開始（黒マス追加なし）。
void __fastcall XgStartSolveNoAddBlack(void)
{
    // フラグを初期化する。
    xg_bSolved = xg_bCancelled = xg_bRetrying = false;

    // スレッドを開始する。
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        xg_aThreadInfo[i].m_count = static_cast<DWORD>(xg_xword.Count());
        xg_ahThreads[i] = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, XgSolveProcNoAddBlack, &xg_aThreadInfo[i], 0,
                &xg_aThreadInfo[i].m_threadid));
        assert(xg_ahThreads[i] != nullptr);
    }
}

// 解を求めるのを開始（スマート解決）。
void __fastcall XgStartSolveSmart(void)
{
    // フラグを初期化する。
    xg_bSolved = xg_bCancelled = xg_bRetrying = false;

    // まだブロック生成していない。
    xg_bBlacksGenerated = FALSE;

    // スレッドを開始する。
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        xg_aThreadInfo[i].m_count = static_cast<DWORD>(xg_xword.Count());
        xg_ahThreads[i] = reinterpret_cast<HANDLE>(
            _beginthreadex(nullptr, 0, XgSolveProcSmart, &xg_aThreadInfo[i], 0,
                &xg_aThreadInfo[i].m_threadid));
        assert(xg_ahThreads[i] != nullptr);
    }
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
            if (!XgParseHintsStr(xg_hMainWnd, xg_strHints)) {
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

    // 文字マスのフォントを作成する。
    LOGFONTW lf;
    ::GetObjectW(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    if (xg_imode == xg_im_HANGUL) {
        // ハングル文字。
        ::lstrcpynW(lf.lfFaceName, XgLoadStringDx1(67), LF_FACESIZE);
    } else {
        // その他。
        ::lstrcpynW(lf.lfFaceName, XgLoadStringDx1(35), LF_FACESIZE);
    }
    if (xg_szCellFont[0])
        ::lstrcpyW(lf.lfFaceName, xg_szCellFont.data());
    lf.lfHeight = xg_nCellSize * 2 / 3;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    HFONT hFont = ::CreateFontIndirectW(&lf);

    // 小さい文字のフォントを作成する。
    ::GetObjectW(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    if (xg_szSmallFont[0])
        ::lstrcpyW(lf.lfFaceName, xg_szSmallFont.data());
    lf.lfHeight = xg_nCellSize / 4;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    HFONT hFontSmall = ::CreateFontIndirectW(&lf);

    // 全体を白で塗りつぶす。
    RECT rc;
    ::SetRect(&rc, 0, 0, psiz->cx, psiz->cy);
    ::FillRect(hdc, &rc, reinterpret_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

    // 二重マスを描画する。
    std::array<WCHAR,32> sz;
    SIZE siz;
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall);
    HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
    for (int i = 0; i < nCount; i++) {
        ::SetRect(&rc,
            static_cast<int>(xg_nNarrowMargin + i * xg_nCellSize), 
            static_cast<int>(xg_nNarrowMargin + 0 * xg_nCellSize),
            static_cast<int>(xg_nNarrowMargin + (i + 1) * xg_nCellSize), 
            static_cast<int>(xg_nNarrowMargin + 1 * xg_nCellSize));
        ::FillRect(hdc, &rc, hbrMarked);
        ::InflateRect(&rc, -4, -4);
        if (xg_bDrawFrameForMarkedCell) {
            ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
            ::Rectangle(hdc, rc.left, rc.top, rc.right + 1, rc.bottom + 1);
        }
        ::InflateRect(&rc, 4, 4);

        // 二重マスの文字を描く。
        ::SetTextColor(hdc, xg_rgbBlackCellColor);
        ::SetBkMode(hdc, OPAQUE);
        ::SetBkColor(hdc, xg_rgbMarkedCellColor);
        ::wsprintfW(sz.data(), L"%c", ZEN_LARGE_A + i);
        ::GetTextExtentPoint32W(hdc, sz.data(), static_cast<int>(wcslen(sz.data())), &siz);
        ::SetRect(&rc,
            static_cast<int>(xg_nNarrowMargin + i * xg_nCellSize - 1 - siz.cx), 
            static_cast<int>(xg_nNarrowMargin + 0 * xg_nCellSize - 1 - siz.cy),
            static_cast<int>(xg_nNarrowMargin + (i + 1) * xg_nCellSize - 1), 
            static_cast<int>(xg_nNarrowMargin + 1 * xg_nCellSize - 1));
        ::DrawTextW(hdc, sz.data(), -1, &rc, DT_RIGHT | DT_SINGLELINE | DT_BOTTOM);
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

        // マスの文字を描画する。
        ::SetTextColor(hdc, xg_rgbBlackCellColor);
        ::SetBkMode(hdc, TRANSPARENT);
        ::DrawTextW(hdc, &ch, 1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
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

    // 周りに太い線を描く。
    if (xg_bAddThickFrame) {
        hPenOld = ::SelectObject(hdc, hWidePen);
        c_nWide /= 2;
        ::MoveToEx(hdc, xg_nNarrowMargin - c_nWide, xg_nNarrowMargin - c_nWide, nullptr);
        ::LineTo(hdc, psiz->cx - xg_nNarrowMargin + c_nWide, xg_nNarrowMargin - c_nWide);
        ::LineTo(hdc, psiz->cx - xg_nNarrowMargin + c_nWide, psiz->cy - xg_nNarrowMargin + c_nWide);
        ::LineTo(hdc, xg_nNarrowMargin - c_nWide, psiz->cy - xg_nNarrowMargin + c_nWide);
        ::LineTo(hdc, xg_nNarrowMargin - c_nWide, xg_nNarrowMargin - c_nWide);
        ::SelectObject(hdc, hPenOld);
    }

    // 破棄する。
    ::DeleteObject(hWidePen);
    ::DeleteObject(hThinPen);
    ::DeleteObject(hFont);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
}

// クロスワードを描画する。
void __fastcall XgDrawXWord(XG_Board& xw, HDC hdc, LPSIZE psiz, bool bCaret)
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

    // 文字マスのフォントを作成する。
    LOGFONTW lf;
    ::GetObjectW(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    if (xg_imode == xg_im_HANGUL) {
        // ハングル文字。
        ::lstrcpynW(lf.lfFaceName, XgLoadStringDx1(67), LF_FACESIZE);
    } else {
        // その他。
        ::lstrcpynW(lf.lfFaceName, XgLoadStringDx1(35), LF_FACESIZE);
    }
    if (xg_szCellFont[0])
        ::lstrcpyW(lf.lfFaceName, xg_szCellFont.data());
    lf.lfHeight = nCellSize * 2 / 3;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    HFONT hFont = ::CreateFontIndirectW(&lf);

    // 小さい文字のフォントを作成する。
    ::GetObjectW(::GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONTW), &lf);
    if (xg_szSmallFont[0])
        ::lstrcpyW(lf.lfFaceName, xg_szSmallFont.data());
    lf.lfHeight = nCellSize / 4;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    HFONT hFontSmall = ::CreateFontIndirectW(&lf);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);

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

    std::array<WCHAR,32> sz;
    SIZE siz;
    HGDIOBJ hFontOld, hPenOld;
    
    // セルを描画する。
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
                ::FillRect(hdc, &rc, hbrBlack);
            } else if (nMarked != -1) {
                // 二重マス。
                ::FillRect(hdc, &rc, hbrMarked);
            } else {
                // その他のマス。
                ::FillRect(hdc, &rc, hbrWhite);
            }

            // 文字の背景は透明。塗りつぶさない。
            ::SetBkMode(hdc, TRANSPARENT);

            // 文字を書く。
            hFontOld = ::SelectObject(hdc, hFont);
            ::SetTextColor(hdc, xg_rgbBlackCellColor);
            ::DrawTextW(hdc, &ch, 1, &rc, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
            ::SelectObject(hdc, hFontOld);
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
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
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

            // 二重マスの右下端の文字の背景を塗りつぶす。
            ::SetBkMode(hdc, OPAQUE);
            ::SetBkColor(hdc, xg_rgbMarkedCellColor);

            // 二重マスの右下端の文字を描く。
            ::wsprintfW(sz.data(), L"%c", ZEN_LARGE_A + nMarked);
            ::GetTextExtentPoint32W(hdc, sz.data(), static_cast<int>(wcslen(sz.data())), &siz);
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize - 1 - siz.cx), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize - 1 - siz.cy),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize - 1), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize - 1));
            ::DrawTextW(hdc, sz.data(), -1, &rc, DT_RIGHT | DT_SINGLELINE | DT_BOTTOM);
        }
    }

    // タテのカギの先頭マス。
    {
        const int size = static_cast<int>(xg_vTateInfo.size());
        for (int k = 0; k < size; k++) {
            const int i = xg_vTateInfo[k].m_iRow;
            const int j = xg_vTateInfo[k].m_jCol;
            ::wsprintfW(sz.data(), L"%u", xg_vTateInfo[k].m_number);

            // 文字の背景を塗りつぶす。
            ::SetBkMode(hdc, OPAQUE);
            int nMarked = XgGetMarked(i, j);
            if (nMarked != -1) {
                ::SetBkColor(hdc, xg_rgbMarkedCellColor);
            } else {
                ::SetBkColor(hdc, xg_rgbWhiteCellColor);
            }

            // 数字を描く。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));
            ::OffsetRect(&rc, 2, 2);
            ::DrawTextW(hdc, sz.data(), -1, &rc, DT_LEFT | DT_SINGLELINE | DT_TOP);
        }
    }
    // ヨコのカギの先頭マス。
    {
        const int size = static_cast<int>(xg_vYokoInfo.size());
        for (int k = 0; k < size; k++) {
            const int i = xg_vYokoInfo[k].m_iRow;
            const int j = xg_vYokoInfo[k].m_jCol;
            ::wsprintfW(sz.data(), L"%u", xg_vYokoInfo[k].m_number);

            // 文字の背景を塗りつぶす。
            int nMarked = XgGetMarked(i, j);
            if (nMarked != -1) {
                ::SetBkColor(hdc, xg_rgbMarkedCellColor);
            } else {
                ::SetBkColor(hdc, xg_rgbWhiteCellColor);
            }

            // 数字を描く。
            ::SetRect(&rc,
                static_cast<int>(xg_nMargin + j * nCellSize), 
                static_cast<int>(xg_nMargin + i * nCellSize),
                static_cast<int>(xg_nMargin + (j + 1) * nCellSize), 
                static_cast<int>(xg_nMargin + (i + 1) * nCellSize));
            ::OffsetRect(&rc, 2, 2);
            ::DrawTextW(hdc, sz.data(), -1, &rc, DT_LEFT | DT_SINGLELINE | DT_TOP);
        }
    }

    // フォントの選択を解除する。
    ::SelectObject(hdc, hFontOld);

    // キャレットを描画する。
    if (bCaret) {
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
}

// クロスワードのイメージを作成する。
HBITMAP __fastcall XgCreateXWordImage(XG_Board& xw, LPSIZE psiz, bool bCaret)
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
    XgDrawXWord(xw, hdc, psiz, bCaret);
    ::SelectObject(hdc, hbmOld);

    // 互換DCを破棄する。
    ::DeleteDC(hdc);
    return hbm;
}

// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd, int x, int y)
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

    // 再描画する。
    ::InvalidateRect(hwnd, &rcClient, TRUE);
}

// ファイルを開く。
bool __fastcall XgDoLoadBuilderFile(HWND hwnd, LPCWSTR pszFile)
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
            wsprintfW(szName, L"Line%u", i + 1);
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
        XgDestroyCandsWnd();
        XgDestroyHintsWnd();
        xg_bSolved = false;

        if (xword.IsFulfilled()) {
            xword.DoNumberingNoCheck();

            for (i = 0; i < 256; ++i) {
                wsprintfW(szName, L"Down%u", i + 1);
                GetPrivateProfileStringW(L"Clue", szName, L"", szText, ARRAYSIZE(szText), pszFile);

                std::wstring str = szText;
                xg_str_trim(str);
                if (str.empty())
                    break;
                if (str == L"{N/A}")
                    continue;
                str = XgNormalizeString(str);

                for (XG_PlaceInfo& item : xg_vTateInfo) {
                    if (item.m_number == i + 1) {
                        tate.emplace_back(item.m_number, item.m_word, str);
                        break;
                    }
                }
            }

            for (i = 0; i < 256; ++i) {
                wsprintfW(szName, L"Across%u", i + 1);
                GetPrivateProfileStringW(L"Clue", szName, L"", szText, ARRAYSIZE(szText), pszFile);

                std::wstring str = szText;
                xg_str_trim(str);
                if (str.empty())
                    break;
                if (str == L"{N/A}")
                    continue;
                str = XgNormalizeString(str);

                for (XG_PlaceInfo& item : xg_vYokoInfo) {
                    if (item.m_number == i + 1) {
                        yoko.emplace_back(item.m_number, item.m_word, str);
                        break;
                    }
                }
            }
        }

        // 成功。
        xg_xword = xword;
        xg_solution = xword;
        if (tate.size() && yoko.size()) {
            xg_bSolved = true;
            xg_bShowAnswer = false;
            XgClearNonBlocks(hwnd);
            xg_vecTateHints = tate;
            xg_vecYokoHints = yoko;
            XgShowHints(hwnd);
        }

        XgUpdateImage(hwnd, 0, 0);
        XgMarkUpdate();

        // ファイルパスをセットする。
        std::array<WCHAR,MAX_PATH> szFileName;
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName.data(), NULL);
        xg_strFileName = szFileName.data();
    }

    return bOK;
}

// ファイルを開く。
bool __fastcall XgDoLoad(HWND hwnd, LPCWSTR pszFile, bool json)
{
    HANDLE hFile;
    DWORD i, cbFile, cbRead;
    LPBYTE pbFile = nullptr;
    bool bOK = false;

    // 二重マス単語を空にする。
    XgSetMarkedWord();

    // ファイルを開く。
    hFile = ::CreateFileW(pszFile, GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    // ファイルサイズを取得。
    cbFile = ::GetFileSize(hFile, nullptr);
    if (cbFile == 0xFFFFFFFF)
        return false;

    try {
        // メモリを確保してファイルから読み込む。
        pbFile = new BYTE[cbFile + 3];
        i = cbFile;
        if (::ReadFile(hFile, pbFile, cbFile, &cbRead, nullptr)) {
            // BOMチェック。
            if (pbFile[0] == 0xFF && pbFile[1] == 0xFE) {
                // Unicode
                pbFile[cbFile] = 0;
                pbFile[cbFile + 1] = 0;
                pbFile[cbFile + 2] = 0;
                std::wstring str = reinterpret_cast<LPWSTR>(&pbFile[2]);
                bOK = XgSetString(hwnd, str, json);
                i = 0;
            } else if (pbFile[0] == 0xFE && pbFile[1] == 0xFF) {
                // Unicode BigEndian
                pbFile[cbFile] = 0;
                pbFile[cbFile + 1] = 0;
                pbFile[cbFile + 2] = 0;
                XgSwab(pbFile, cbFile);
                std::wstring str = reinterpret_cast<LPWSTR>(&pbFile[2]);
                bOK = XgSetString(hwnd, str, json);
                i = 0;
            } else if (pbFile[0] == 0xEF && pbFile[1] == 0xBB &&
                       pbFile[2] == 0xBF)
            {
                // UTF-8
                pbFile[cbFile] = 0;
                std::wstring str = XgUtf8ToUnicode(reinterpret_cast<LPCSTR>(&pbFile[3]));
                bOK = XgSetString(hwnd, str, json);
                i = 0;
            } else {
                for (i = 0; i < cbFile; i++) {
                    // ナル文字があればUnicodeと判断する。
                    if (pbFile[i] == 0) {
                        pbFile[cbFile] = 0;
                        pbFile[cbFile + 1] = 0;
                        pbFile[cbFile + 2] = 0;
                        // エンディアンの判定。
                        if (i & 1) {
                            // Unicode
                            std::wstring str = reinterpret_cast<LPWSTR>(pbFile);
                            bOK = XgSetString(hwnd, str, json);
                        } else {
                            // Unicode BE
                            XgSwab(pbFile, cbFile);
                            std::wstring str = reinterpret_cast<LPWSTR>(pbFile);
                            bOK = XgSetString(hwnd, str, json);
                        }
                        break;
                    }
                }
            }
            if (i == cbFile) {
                pbFile[cbFile] = 0;
                if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        reinterpret_cast<LPCSTR>(pbFile),
                                        static_cast<int>(cbFile), nullptr, 0))
                {
                    // UTF-8
                    std::wstring str = XgUtf8ToUnicode(reinterpret_cast<LPCSTR>(pbFile));
                    bOK = XgSetString(hwnd, str, json);
                } else {
                    // ANSI
                    std::wstring str = XgAnsiToUnicode(reinterpret_cast<LPCSTR>(pbFile));
                    bOK = XgSetString(hwnd, str, json);
                }
            }

            if (bOK) {
                // 成功。
                delete[] pbFile;
                ::CloseHandle(hFile);
                XgUpdateImage(hwnd, 0, 0);
                XgMarkUpdate();

                // ファイルパスをセットする。
                std::array<WCHAR,MAX_PATH> szFileName;
                ::GetFullPathNameW(pszFile, MAX_PATH, szFileName.data(), NULL);
                xg_strFileName = szFileName.data();
                return true;
            }
        }
    } catch (...) {
        // 例外が発生した。
    }

    // 失敗。
    delete[] pbFile;
    ::CloseHandle(hFile);
    return false;
}

// メールの件名を作成する。
bool __fastcall XgGetMailTitle(HWND hwnd, std::wstring& strTitle)
{
    strTitle = XgLoadStringDx1(85);
    return true;
}

// メールの本文を作成する。
bool __fastcall XgGetMailBody(HWND hwnd, std::wstring& strBody)
{
    std::wstring str, strTable, strMarks, hints;

    // マークを取得する。
    if (!xg_vMarks.empty())
        XgGetStringOfMarks(strMarks);

    // ファイルに書き込む文字列を求める。
    if (xg_bSolved) {
        // ヒントあり。
        xg_solution.GetString(strTable);
        xg_solution.GetHintsStr(hints, 2, true);
        // メールの本文。
        xg_str_trim(xg_strHeader);
        if (xg_strHeader.empty())
            str += XgLoadStringDx1(84);
        else
            str += xg_strHeader;
        str += xg_pszNewLine;       // 改行。
        str += XgLoadStringDx1(81); // ヘッダー分離線。
        str += XgLoadStringDx1(15); // アプリ情報。
        str += xg_pszNewLine;       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
        str += hints;               // ヒント。
    } else {
        // ヒントなし。
        xg_xword.GetString(strTable);
        // メールの本文。
        xg_str_trim(xg_strHeader);
        if (xg_strHeader.empty())
            str += XgLoadStringDx1(84);
        else
            str += xg_strHeader;
        str += xg_pszNewLine;       // 改行。
        str += XgLoadStringDx1(81); // ヘッダー分離線。
        str += XgLoadStringDx1(15); // アプリ情報。
        str += xg_pszNewLine;       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
    }
    str += XgLoadStringDx1(82);     // フッター分離線。

    // 備考欄。
    LPCWSTR psz = XgLoadStringDx1(83);
    str += psz;
    str += xg_pszNewLine;
    if (xg_strNotes.empty()) {
        ;
    } else {
        if (xg_strNotes.find(psz) == 0) {
            xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
        }
        xg_str_trim(xg_strNotes);
        str += xg_strNotes;
    }

    strBody = str;
    return true;
}

// .xwjファイル（JSON形式）を保存する。
bool __fastcall XgDoSaveJson(HWND /*hwnd*/, LPCWSTR pszFile)
{
    HANDLE hFile;
    std::wstring str, strTable, strMarks, hints;
    DWORD size;
    std::array<WCHAR,1024> buf;

    // ファイルを作成する。
    hFile = ::CreateFileW(pszFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    // ファイルに書き込む文字列を求める。
    str += L"{\r\n";
    str += L"\t\"creator_info\": \"";
    str += XgJsonEncodeString(XgLoadStringDx1(1176));
    str += L"\",\r\n";

    // クロスワードのサイズ。
    ::wsprintfW(buf.data(), L"\t\"row_count\": %d,\r\n", xg_nRows);
    str += buf.data();
    ::wsprintfW(buf.data(), L"\t\"column_count\": %d,\r\n", xg_nCols);
    str += buf.data();

    // 盤の切り替え。
    XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);
    if (xg_bSolved) {
        str += L"\t\"is_solved\": true,\r\n";
    } else {
        str += L"\t\"is_solved\": false,\r\n";
    }

    // マス。
    str += L"\t\"cell_data\": [\r\n";
    for (int i = 0; i < xg_nRows; ++i) {
        std::wstring row;
        for (int j = 0; j < xg_nCols; ++j) {
            WCHAR ch = xw->GetAt(i, j);
            row += ch;
        }
        str += L"\t\t\"";
        str += row;
        if (i == xg_nRows - 1)
            str += L"\"\r\n";
        else
            str += L"\",\r\n";
    }
    str += L"\t],\r\n";

    // 二重マス。
    if (xg_vMarks.size()) {
        str += L"\t\"has_mark\": true,\r\n";
        std::wstring mark_word;
        for (size_t i = 0; i < xg_vMarks.size(); ++i) {
            WCHAR ch = xw->GetAt(xg_vMarks[i].m_i, xg_vMarks[i].m_j);
            mark_word += ch;
        }

        str += L"\t\"mark_word\": \"";
        str += XgJsonEncodeString(mark_word);
        str += L"\",\r\n";

        str += L"\t\"marks\": [\r\n";
        for (size_t i = 0; i < xg_vMarks.size(); ++i) {
            if (i == xg_vMarks.size() - 1)
                ::wsprintfW(buf.data(), L"\t\t[%d, %d, \"%c\"]\r\n",
                    xg_vMarks[i].m_i + 1, xg_vMarks[i].m_j + 1, mark_word[i]);
            else
                ::wsprintfW(buf.data(), L"\t\t[%d, %d, \"%c\"],\r\n",
                    xg_vMarks[i].m_i + 1, xg_vMarks[i].m_j + 1, mark_word[i]);
            str += buf.data();
        }
        str += L"\t],\r\n";
    } else {
        str += L"\t\"has_mark\": false,\r\n";
    }

    // ヒント。
    if (xg_vecTateHints.size() && xg_vecYokoHints.size()) {
        str += L"\t\"has_hints\": true,\r\n";
        str += L"\t\"hints\": {\r\n";
        // タテのカギ。
        str += L"\t\t\"v\": [\r\n";
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            const XG_Hint& hint = xg_vecTateHints[i];
            ::wsprintfW(buf.data(), L"\t\t\t[%d, \"", hint.m_number);
            str += buf.data();
            str += XgJsonEncodeString(hint.m_strWord);
            str += L"\", \"";
            str += XgJsonEncodeString(hint.m_strHint);
            if (i == xg_vecTateHints.size() - 1) {
                str += L"\"]\r\n";
            } else {
                str += L"\"],\r\n";
            }
        }
        str += L"\t\t],\r\n";
        // ヨコのカギ。
        str += L"\t\t\"h\": [\r\n";
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            const XG_Hint& hint = xg_vecYokoHints[i];
            ::wsprintfW(buf.data(), L"\t\t\t[%d, \"", hint.m_number);
            str += buf.data();
            str += XgJsonEncodeString(hint.m_strWord);
            str += L"\", \"";
            str += XgJsonEncodeString(hint.m_strHint);
            if (i == xg_vecYokoHints.size() - 1) {
                str += L"\"]\r\n";
            } else {
                str += L"\"],\r\n";
            }
        }
        str += L"\t\t]\r\n";
        str += L"\t},\r\n";
    } else {
        str += L"\t\"has_hints\": false,\r\n";
    }

    // ヘッダー。
    xg_str_trim(xg_strHeader);
    str += L"\t\"header\": \"";
    str += XgJsonEncodeString(xg_strHeader);
    str += L"\",\r\n";

    // 備考欄。
    xg_str_trim(xg_strNotes);
    LPCWSTR psz = XgLoadStringDx1(83);
    if (xg_strNotes.find(psz) == 0) {
        xg_strNotes = xg_strNotes.substr(std::wstring(psz).size());
    }
    str += L"\t\"notes\": \"";
    str += XgJsonEncodeString(xg_strNotes);
    str += L"\"\r\n";

    str += L"}\r\n";

    // UTF-8へ変換する。
    std::string utf8 = XgUnicodeToUtf8(str);

    // ファイルに書き込んで、ファイルを閉じる。
    size = static_cast<DWORD>(utf8.size()) * sizeof(CHAR);
    if (::WriteFile(hFile, utf8.data(), size, &size, nullptr)) {
        ::CloseHandle(hFile);

        // ファイルパスをセットする。
        std::array<WCHAR, MAX_PATH> szFileName;
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName.data(), NULL);
        xg_strFileName = szFileName.data();
        XgMarkUpdate();
        return true;
    }
    ::CloseHandle(hFile);

    // 正しく書き込めなかった。不正なファイルを消す。
    ::DeleteFileW(pszFile);
    return false;
}

// .xwd/.xwjファイルを保存する。
bool __fastcall XgDoSaveStandard(HWND hwnd, LPCWSTR pszFile, const XG_Board& board)
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
        board.GetHintsStr(hints, 2, true);
        str += xg_strHeader;        // ヘッダー文字列。
        str += xg_pszNewLine;       // 改行。
        str += XgLoadStringDx1(81); // ヘッダー分離線。
        str += XgLoadStringDx1(1176); // アプリ情報。
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
        str += XgLoadStringDx1(81); // ヘッダー分離線。
        str += XgLoadStringDx1(1176); // アプリ情報。
        str += xg_pszNewLine;       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
    }
    str += XgLoadStringDx1(82);     // フッター分離線。

    // 備考欄。
    LPCWSTR psz = XgLoadStringDx1(83);
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

bool __fastcall XgDoSave(HWND hwnd, LPCWSTR pszFile, bool json)
{
    bool ret;
    if (json) {
        // JSON形式で保存。
        ret = XgDoSaveJson(hwnd, pszFile);
    } else if (xg_bSolved) {
        // ヒントあり。
        ret = XgDoSaveStandard(hwnd, pszFile, xg_solution);
    } else {
        ret = XgDoSaveStandard(hwnd, pszFile, xg_xword);
    }
    if (ret) {
        // ファイルパスをセットする。
        std::array<WCHAR,MAX_PATH> szFileName;
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName.data(), NULL);
        xg_strFileName = szFileName.data();
        XgMarkUpdate();
    }
    return ret;
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
    std::array<WCHAR,MAX_PATH> szFileName = { { 0 } };

    // 「問題を画像ファイルとして保存」ダイアログを表示。
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(49));
    ofn.lpstrFile = szFileName.data();
    ofn.nMaxFile = static_cast<DWORD>(szFileName.size());
    ofn.lpstrTitle = XgLoadStringDx1(30);
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
                if (!XgSaveBitmapToFile(szFileName.data(), hbm))
                {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
                }
                ::DeleteObject(hbm);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
            }
        } else {
            // EMFを保存する。
            HDC hdcRef = ::GetDC(hwnd);
            HDC hdc = ::CreateEnhMetaFileW(hdcRef, szFileName.data(),
                                         nullptr, XgLoadStringDx1(2));
            if (hdc) {
                XgDrawXWord(xg_xword, hdc, &siz, false);
                ::CloseEnhMetaFile(hdc);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
            }
            ::ReleaseDC(hwnd, hdcRef);
        }
    }
}

// 解答を画像ファイルとして保存する。
void __fastcall XgSaveAnsAsImage(HWND hwnd)
{
    OPENFILENAMEW ofn;
    std::array<WCHAR,MAX_PATH> szFileName = { {L'\0'} };

    if (!xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // 「解答を画像ファイルとして保存」ダイアログを表示。
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(49));
    ofn.lpstrFile = szFileName.data();
    ofn.nMaxFile = static_cast<DWORD>(szFileName.size());
    ofn.lpstrTitle = XgLoadStringDx1(29);
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
                if (!XgSaveBitmapToFile(szFileName.data(), hbm)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
                }
                ::DeleteObject(hbm);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
            }
        } else {
            // EMFを保存する。
            HDC hdcRef = ::GetDC(hwnd);
            HDC hdc = ::CreateEnhMetaFileW(hdcRef, szFileName.data(), nullptr,
                                         XgLoadStringDx1(2));
            if (hdc) {
                XgDrawXWord(xg_solution, hdc, &siz, false);
                ::CloseEnhMetaFile(hdc);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(7), nullptr, MB_ICONERROR);
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
    std::wstring strHeaderSep = XgLoadStringDx1(81);
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

    // マスの文字の種類に応じて入力モードを切り替える。
    for (int i = 0; i < nRows; i++) {
        for (int j = 0; j < nCols; j++) {
            ch = GetAt(i, j);
            if (XgIsCharHankakuAlphaW(ch) || XgIsCharZenkakuAlphaW(ch)) {
                xg_imode = xg_im_ABC;
                goto break2;
            }
            if (XgIsCharKanaW(ch)) {
                xg_imode = xg_im_KANA;
                goto break2;
            }
            if (XgIsCharKanjiW(ch)) {
                xg_imode = xg_im_KANJI;
                goto break2;
            }
            if (XgIsCharHangulW(ch)) {
                xg_imode = xg_im_HANGUL;
                goto break2;
            }
            if (XgIsCharZenkakuCyrillicW(ch)) {
                xg_imode = xg_im_RUSSIA;
                goto break2;
            }
        }
    }
break2:;

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

//////////////////////////////////////////////////////////////////////////////
// 黒マスパターンを生成する。

bool xg_bBlacksGenerated = false;

template <int t_mode>
bool __fastcall XgGenerateBlacksRecurse(const XG_Board& xword)
{
    if (xword.CornerBlack() || xword.DoubleBlack() || xword.TriBlackArround())
        return false;

    if (xword.DividedByBlack())
        return false;

    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    for (int i = 0; i < nRows; ++i) {
        for (int j = 0; j < nCols; ++j) {
            if (xg_bBlacksGenerated || xg_bCancelled)
                return true;

            if (xword.GetAt(i, j) == ZEN_SPACE) {
                // 文字が置けるヨコ向きの区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (xword.GetAt(i, lo - 1) != ZEN_SPACE)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (xword.GetAt(i, j + 1) != ZEN_SPACE)
                        break;
                    j++;
                }
                const int hi = j;
                j++;

                if (t_mode) {
                    if (lo + 4 <= hi) {
                        char a[] = {0, 1, 2, 3};
                        std::random_shuffle(std::begin(a), std::end(a));

                        for (int k = 0; k < 4; ++k) {
                            XG_Board copy(xword);
                            copy.SetAt(i, lo + a[k], ZEN_BLACK);
                            if (XgGenerateBlacksRecurse<t_mode>(copy))
                                return true;
                        }
                        return false;
                    }
                } else {
                    if (lo + 5 <= hi) {
                        char a[] = {0, 1, 2, 3, 4};
                        std::random_shuffle(std::begin(a), std::end(a));

                        for (int k = 0; k < 5; ++k) {
                            XG_Board copy(xword);
                            copy.SetAt(i, lo + a[k], ZEN_BLACK);
                            if (XgGenerateBlacksRecurse<t_mode>(copy))
                                return true;
                        }
                        return false;
                    }
                }
            }
        }
    }

    for (int j = 0; j < nCols; ++j) {
        for (int i = 0; i < nRows; ++i) {
            if (xg_bBlacksGenerated || xg_bCancelled)
                return true;

            if (xword.GetAt(i, j) == ZEN_SPACE) {
                // 文字が置けるヨコ向きの区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (xword.GetAt(lo - 1, j) != ZEN_SPACE)
                        break;
                    lo--;
                }
                while (i + 1 < nCols) {
                    if (xword.GetAt(i + 1, j) != ZEN_SPACE)
                        break;
                    i++;
                }
                const int hi = i;
                i++;

                if (t_mode == 0) {
                    if (lo + 4 <= hi) {
                        char a[] = {0, 1, 2, 3};
                        std::random_shuffle(std::begin(a), std::end(a));

                        for (int k = 0; k < 4; ++k) {
                            XG_Board copy(xword);
                            copy.SetAt(lo + a[k], j, ZEN_BLACK);
                            if (XgGenerateBlacksRecurse<t_mode>(copy))
                                return true;
                        }
                        return false;
                    }
                } else if (t_mode == 1) {
                    if (lo + 5 <= hi) {
                        char a[] = {0, 1, 2, 3, 4};
                        std::random_shuffle(std::begin(a), std::end(a));

                        for (int k = 0; k < 5; ++k) {
                            XG_Board copy(xword);
                            copy.SetAt(lo + a[k], j, ZEN_BLACK);
                            if (XgGenerateBlacksRecurse<t_mode>(copy))
                                return true;
                        }
                        return false;
                    }
                } else if (t_mode == 2) {
                    if (lo + 6 <= hi) {
                        char a[] = {0, 1, 2, 3, 4, 5};
                        std::random_shuffle(std::begin(a), std::end(a));

                        for (int k = 0; k < 6; ++k) {
                            XG_Board copy(xword);
                            copy.SetAt(lo + a[k], j, ZEN_BLACK);
                            if (XgGenerateBlacksRecurse<t_mode>(copy))
                                return true;
                        }
                        return false;
                    }
                }
            }
        }
    }

    EnterCriticalSection(&xg_cs);
    if (!xg_bBlacksGenerated) {
        xg_bBlacksGenerated = true;
        xg_xword = xword;
    }
    ::LeaveCriticalSection(&xg_cs);
    return xg_bBlacksGenerated || xg_bCancelled;
}

bool __fastcall XgGenerateBlacksSym2Recurse(const XG_Board& xword)
{
    if (xword.CornerBlack() || xword.DoubleBlack() || xword.TriBlackArround())
        return false;

    if (xword.DividedByBlack())
        return false;

    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    for (int i = 0; i < nRows; ++i) {
        for (int j = 0; j < nCols; ++j) {
            if (xg_bBlacksGenerated || xg_bCancelled)
                return true;

            if (xword.GetAt(i, j) == ZEN_SPACE) {
                // 文字が置けるヨコ向きの区間[lo, hi]を求める。
                int lo = j;
                while (lo > 0) {
                    if (xword.GetAt(i, lo - 1) != ZEN_SPACE)
                        break;
                    lo--;
                }
                while (j + 1 < nCols) {
                    if (xword.GetAt(i, j + 1) != ZEN_SPACE)
                        break;
                    j++;
                }
                const int hi = j;
                j++;

                if (lo + 4 <= hi) {
                    char a[4] = {0, 1, 2, 3};
                    std::random_shuffle(&a[0], &a[4]);

                    for (int k = 0; k < 4; ++k) {
                        XG_Board copy(xword);
                        copy.SetAt(i, lo + a[k], ZEN_BLACK);
                        copy.SetAt(nRows - (i + 1), nCols - (lo + a[k] + 1), ZEN_BLACK);
                        if (XgGenerateBlacksSym2Recurse(copy))
                            return true;
                    }
                    return false;
                }
            }
        }
    }

    for (int j = 0; j < nCols; ++j) {
        for (int i = 0; i < nRows; ++i) {
            if (xg_bBlacksGenerated || xg_bCancelled)
                return true;

            if (xword.GetAt(i, j) == ZEN_SPACE) {
                // 文字が置けるヨコ向きの区間[lo, hi]を求める。
                int lo = i;
                while (lo > 0) {
                    if (xword.GetAt(lo - 1, j) != ZEN_SPACE)
                        break;
                    lo--;
                }
                while (i + 1 < nCols) {
                    if (xword.GetAt(i + 1, j) != ZEN_SPACE)
                        break;
                    i++;
                }
                const int hi = i;
                i++;

                if (lo + 4 <= hi) {
                    char a[4] = {0, 1, 2, 3};
                    std::random_shuffle(&a[0], &a[4]);

                    for (int k = 0; k < 4; ++k) {
                        XG_Board copy(xword);
                        copy.SetAt(lo + a[k], j, ZEN_BLACK);
                        copy.SetAt(nRows - (lo + a[k] + 1), nCols - (j + 1), ZEN_BLACK);
                        if (XgGenerateBlacksSym2Recurse(copy))
                            return true;
                    }
                    return false;
                }
            }
        }
    }

    EnterCriticalSection(&xg_cs);
    if (!xg_bBlacksGenerated && xword.IsPointSymmetry()) {
        xg_bBlacksGenerated = true;
        xg_xword = xword;
    }
    ::LeaveCriticalSection(&xg_cs);
    return xg_bBlacksGenerated || xg_bCancelled;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacks(void *param)
{
    srand(::GetTickCount() ^ ::GetCurrentThreadId());
    xg_solution.clear();
    XG_Board xword;

    if (xg_imode == xg_im_KANJI || xg_nRows < 5 || xg_nCols < 5) {
        do {
            if (xg_bBlacksGenerated || xg_bCancelled)
                break;
            xword.clear();
        } while (!XgGenerateBlacksRecurse<1>(xword));
    } else if (xg_imode == xg_im_RUSSIA) {
        do {
            if (xg_bBlacksGenerated || xg_bCancelled)
                break;
            xword.clear();
        } while (!XgGenerateBlacksRecurse<2>(xword));
    } else {
        do {
            if (xg_bBlacksGenerated || xg_bCancelled)
                break;
            xword.clear();
        } while (!XgGenerateBlacksRecurse<0>(xword));
    }
    return 1;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksSmart(void *param)
{
    if (xg_bBlacksGenerated)
        return 1;

    XG_Board xword;
    srand(::GetTickCount() ^ ::GetCurrentThreadId());
    if (xg_imode == xg_im_KANJI || xg_nRows < 5 || xg_nCols < 5) {
        do {
            if (xg_bCancelled)
                break;
            if (xg_bBlacksGenerated) {
                break;
            }
            xword.clear();
        } while (!XgGenerateBlacksRecurse<1>(xword));
    } else if (xg_imode == xg_im_RUSSIA) {
        do {
            if (xg_bCancelled)
                break;
            if (xg_bBlacksGenerated) {
                break;
            }
            xword.clear();
        } while (!XgGenerateBlacksRecurse<2>(xword));
    } else {
        do {
            if (xg_bCancelled)
                break;
            if (xg_bBlacksGenerated) {
                break;
            }
            xword.clear();
        } while (!XgGenerateBlacksRecurse<0>(xword));
    }
    return 1;
}

// マルチスレッド用の関数。
unsigned __stdcall XgGenerateBlacksSym2(void *param)
{
    srand(::GetTickCount() ^ ::GetCurrentThreadId());
    xg_solution.ResetAndSetSize(xg_nRows, xg_nCols);
    XG_Board xword;
    do {
        if (xg_bBlacksGenerated || xg_bCancelled)
            break;
        xword.ResetAndSetSize(xg_nRows, xg_nCols);
    } while (!XgGenerateBlacksSym2Recurse(xword));
    return 1;
}

void __fastcall XgStartGenerateBlacks(bool sym)
{
    xg_bBlacksGenerated = false;
    xg_bCancelled = false;

    // スレッドを開始する。
    if (sym) {
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            xg_ahThreads[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksSym2, &xg_aThreadInfo[i], 0,
                    &xg_aThreadInfo[i].m_threadid));
            assert(xg_ahThreads[i] != nullptr);
        }
    } else {
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            xg_ahThreads[i] = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacks, &xg_aThreadInfo[i], 0,
                    &xg_aThreadInfo[i].m_threadid));
            assert(xg_ahThreads[i] != nullptr);
        }
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
        for (size_t i = 0; i < xg_small.size(); i++) {
            if (ch == xg_small[i][0]) {
                ch = xg_large[i][0];
                break;
            }
        }
    }
    return ret;
}
