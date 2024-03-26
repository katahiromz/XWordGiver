//////////////////////////////////////////////////////////////////////////////
// XWordGiver.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#include "XWordGiver.hpp"
#include "XG_Settings.hpp"
#include <clocale>

//////////////////////////////////////////////////////////////////////////////
// global variables

// 直前に開いたクロスワードデータファイルのパスファイル名。
XGStringW xg_strFileName;

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

// 答え合わせしているか？
bool xg_bCheckingAnswer = false;

// スマート解決か？
bool xg_bSmartResolution = true;

// PAT.txtから自動的にパターンを選択するか？
bool xg_bChoosePAT = true;

// スレッドの数。
DWORD xg_dwThreadCount;

// スレッド情報。
std::vector<XG_ThreadInfo>    xg_aThreadInfo;

// スレッドのハンドル。
std::vector<HANDLE>           xg_ahThreads;

// ヒント文字列。
XGStringW             xg_strHints;

// ヘッダー文字列。
XGStringW             xg_strHeader;
// 備考文字列。
XGStringW             xg_strNotes;

// 排他制御のためのクリティカルセクション（ロック）。
CRITICAL_SECTION    xg_csLock;

// キャレットの位置。
XG_Pos              xg_caret_pos = {0, 0};

// クロスワードの問題。
XG_BoardEx xg_xword;
// クロスワードの解。
XG_Board xg_solution;

// クロスワードのサイズ。
volatile int& xg_nRows = xg_xword.m_nRows;
volatile int& xg_nCols = xg_xword.m_nCols;

// タテとヨコのかぎ。
std::vector<XG_PlaceInfo> xg_vVertInfo, xg_vHorzInfo;

// ビットマップのハンドル。
HBITMAP     xg_hbmImage = nullptr;

// ヒントデータ。
std::vector<XG_Hint> xg_vecVertHints, xg_vecHorzHints;

// 二重マスに枠を描くか？
bool xg_bDrawFrameForMarkedCell = true;

// 文字送り？
bool xg_bCharFeed = true;

// タテ入力？
bool xg_bVertInput = false;

//////////////////////////////////////////////////////////////////////////////
// static variables

// 縦と横を反転しているか？
static bool s_bSwapped = false;

//////////////////////////////////////////////////////////////////////////////
// パターン。

// パターンのデータを扱いやすいよう、加工する。
void XgGetPatternData(XG_PATDATA& pat)
{
    XGStringW text = pat.text;
    auto& data = pat.data;
    const int cx = pat.num_columns;
    const int cy = pat.num_rows;
    xg_str_replace_all(text, L"\r\n", L"\n");
    xg_str_replace_all(text, L"\u2501", L"");
    xg_str_replace_all(text, L"\u250F\u2513\n", L"");
    xg_str_replace_all(text, L"\u2517\u251B\n", L"");
    data.resize(cx * cy);
    int x = 0, y = 0;
    for (auto ch : text)
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
        default:
            break;
        }
        if (y == cy)
            break;
    }
}

// パターンのデータを扱いやすいよう、加工する。
void XgSetPatternData(XG_PATDATA& pat)
{
    XGStringW text;
    const auto& data = pat.data;
    const int cx = pat.num_columns;
    const int cy = pat.num_rows;

    text += 0x250F; // ┏
    for (int x = 0; x < cx; ++x) {
        text += 0x2501; // ━
    }
    text += 0x2513; // ┓
    text += L"\r\n";

    for (int y = 0; y < cy; ++y) {
        text += 0x2503; // ┃
        for (int x = 0; x < cx; ++x) {
            text += data[x + cx * y];
        }
        text += 0x2503; // ┃
        text += L"\r\n";
    }

    text += 0x2517; // ┗
    for (int x = 0; x < cx; ++x) {
        text += 0x2501; // ━
    }
    text += 0x251B; // ┛
    text += L"\r\n";

    pat.text = std::move(text);
}

#define XG_GET_DATA(x, y) data[(y) * pat.num_columns + (x)]

// パターンを転置する。
XG_PATDATA XgTransposePattern(const XG_PATDATA& pat)
{
    XG_PATDATA ret;
    ret.num_columns = pat.num_rows;
    ret.num_rows = pat.num_columns;

    ret.data = pat.data;
    auto& data = pat.data;
    for (int y = 0; y < pat.num_rows; ++y) {
        for (int x = 0; x < pat.num_columns; ++x) {
            ret.data[x * ret.num_columns + y] = XG_GET_DATA(x, y);
        }
    }

    XgSetPatternData(ret);

    return ret;
}

// パターンを水平に反転する。
XG_PATDATA XgFlipPatternH(const XG_PATDATA& pat)
{
    XG_PATDATA ret = pat;
    ret.data = pat.data;
    auto& data = pat.data;
    for (int y = 0; y < pat.num_rows; ++y) {
        for (int x = 0; x < pat.num_columns; ++x) {
            ret.data[y * ret.num_columns + x] = XG_GET_DATA((ret.num_columns - x - 1), y);
        }
    }

    XgSetPatternData(ret);

    return ret;
}

// パターンを垂直に反転する。
XG_PATDATA XgFlipPatternV(const XG_PATDATA& pat)
{
    XG_PATDATA ret = pat;
    ret.data = pat.data;
    auto& data = pat.data;
    for (int y = 0; y < pat.num_rows; ++y) {
        for (int x = 0; x < pat.num_columns; ++x) {
            ret.data[y * ret.num_columns + x] = XG_GET_DATA(x, pat.num_rows - y - 1);
        }
    }

    XgSetPatternData(ret);

    return ret;
}

// パターンが黒マスで分断されているか？
BOOL XgIsPatternDividedByBlocks(const XG_PATDATA& pat)
{
    int nCount = pat.num_rows * pat.num_columns;
    auto& data = pat.data;

    // 各マスに対応するフラグ群。
    std::vector<BYTE> pb(nCount, 0);

    // 位置のキュー。
    // 黒マスではないマスを探し、positionsに追加する。
    std::queue<XG_Pos> positions;
    if (XG_GET_DATA(0, 0) != ZEN_BLACK) {
        positions.emplace(0, 0);
    } else {
        for (int i = 0; i < pat.num_rows; ++i) {
            for (int j = 0; j < pat.num_columns; ++j) {
                if (XG_GET_DATA(j, i) != ZEN_BLACK) {
                    positions.emplace(i, j);
                    i = pat.num_rows;
                    j = pat.num_columns;
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
        if (!pb[pos.m_i * pat.num_columns + pos.m_j]) {
            // フラグを立てる。
            pb[pos.m_i * pat.num_columns + pos.m_j] = 1;
            // 上。
            if (pos.m_i > 0 && XG_GET_DATA(pos.m_j, pos.m_i - 1) != ZEN_BLACK)
                positions.emplace(pos.m_i - 1, pos.m_j);
            // 下。
            if (pos.m_i < pat.num_rows - 1 && XG_GET_DATA(pos.m_j, pos.m_i + 1) != ZEN_BLACK)
                positions.emplace(pos.m_i + 1, pos.m_j);
            // 左。
            if (pos.m_j > 0 && XG_GET_DATA(pos.m_j - 1, pos.m_i) != ZEN_BLACK)
                positions.emplace(pos.m_i, pos.m_j - 1);
            // 右。
            if (pos.m_j < pat.num_columns - 1 && XG_GET_DATA(pos.m_j + 1, pos.m_i) != ZEN_BLACK)
                positions.emplace(pos.m_i, pos.m_j + 1);
        }
    }

    // すべてのマスについて。
    while (nCount-- > 0) {
        // フラグが立っていないのに、黒マスではないマスがあったら、失敗。
        if (pb[nCount] == 0 &&
            XG_GET_DATA(nCount % pat.num_columns, nCount / pat.num_columns) != ZEN_BLACK)
        {
            return TRUE;    // 分断されている。
        }
    }

    return FALSE; // 分断されていない。
}

// パターンが黒マスルールに適合するか？
BOOL __fastcall XgPatternRuleIsOK(const XG_PATDATA& pat)
{
    const auto& data = pat.data;

    // データサイズを確認。
    assert(int(data.size()) == pat.num_rows * pat.num_columns);
    if (static_cast<int>(data.size()) != pat.num_rows * pat.num_columns)
        return FALSE;

    if (xg_nRules & RULE_DONTDOUBLEBLACK) {
        for (int y = 0; y < pat.num_rows; ++y) {
            for (int x = 0; x < pat.num_columns - 1; ++x) {
                if (XG_GET_DATA(x, y) == ZEN_BLACK && XG_GET_DATA(x + 1, y) == ZEN_BLACK) {
                    return FALSE;
                }
            }
        }
        for (int x = 0; x < pat.num_columns; ++x) {
            for (int y = 0; y < pat.num_rows - 1; ++y) {
                if (XG_GET_DATA(x, y) == ZEN_BLACK && XG_GET_DATA(x, y + 1) == ZEN_BLACK) {
                    return FALSE;
                }
            }
        }
    }
    if (xg_nRules & RULE_DONTCORNERBLACK) {
        if (XG_GET_DATA(0, 0) == ZEN_BLACK)
            return FALSE;
        if (XG_GET_DATA(pat.num_columns - 1, 0) == ZEN_BLACK)
            return FALSE;
        if (XG_GET_DATA(pat.num_columns - 1, pat.num_rows - 1) == ZEN_BLACK)
            return FALSE;
        if (XG_GET_DATA(0, pat.num_rows - 1) == ZEN_BLACK)
            return FALSE;
    }
    if (xg_nRules & RULE_DONTDIVIDE) {
        if (XgIsPatternDividedByBlocks(pat))
            return FALSE;
    }
    if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
        for (int y = 0; y < pat.num_rows - 2; ++y) {
            for (int x = 0; x < pat.num_columns - 2; ++x) {
                if (XG_GET_DATA(x, y) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x + 1, y + 1) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x + 2, y + 2) != ZEN_BLACK)
                    continue;
                return FALSE;
            }
        }
        for (int y = 0; y < pat.num_rows - 2; ++y) {
            for (int x = 2; x < pat.num_columns; ++x) {
                if (XG_GET_DATA(x, y) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x - 1, y + 1) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x - 2, y + 2) != ZEN_BLACK)
                    continue;
                return FALSE;
            }
        }
    }
    else if (xg_nRules & RULE_DONTFOURDIAGONALS) {
        for (int y = 0; y < pat.num_rows - 3; ++y) {
            for (int x = 0; x < pat.num_columns - 3; ++x) {
                if (XG_GET_DATA(x, y) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x + 1, y + 1) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x + 2, y + 2) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x + 3, y + 3) != ZEN_BLACK)
                    continue;
                return FALSE;
            }
        }
        for (int y = 0; y < pat.num_rows - 3; ++y) {
            for (int x = 3; x < pat.num_columns; ++x) {
                if (XG_GET_DATA(x, y) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x - 1, y + 1) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x - 2, y + 2) != ZEN_BLACK)
                    continue;
                if (XG_GET_DATA(x - 3, y + 3) != ZEN_BLACK)
                    continue;
                return FALSE;
            }
        }
    }
    if (xg_nRules & RULE_DONTTRIDIRECTIONS) {
        for (int y = 0; y < pat.num_rows; ++y) {
            for (int x = 0; x < pat.num_columns; ++x) {
                int nCount = 0;
                if (x > 0 && XG_GET_DATA(x - 1, y) == ZEN_BLACK)
                    ++nCount;
                if (y > 0 && XG_GET_DATA(x, y - 1) == ZEN_BLACK)
                    ++nCount;
                if (x + 1 < pat.num_columns && XG_GET_DATA(x + 1, y) == ZEN_BLACK)
                    ++nCount;
                if (y + 1 < pat.num_rows && XG_GET_DATA(x, y + 1) == ZEN_BLACK)
                    ++nCount;
                if (nCount >= 3) {
                    return FALSE;
                }
            }
        }
    }
    if (xg_nRules & RULE_POINTSYMMETRY) {
        for (int y = 0; y < pat.num_rows; ++y) {
            for (int x = 0; x < pat.num_columns; ++x) {
                if ((XG_GET_DATA(x, y) == ZEN_BLACK) !=
                    (XG_GET_DATA(pat.num_columns - (x + 1), pat.num_rows - (y + 1)) == ZEN_BLACK))
                {
                    return FALSE;
                }
            }
        }
    }
    if (xg_nRules & RULE_LINESYMMETRYV) {
        for (int y = 0; y < pat.num_rows; ++y) {
            for (int x = 0; x < pat.num_columns; ++x) {
                if ((XG_GET_DATA(x, y) == ZEN_BLACK) !=
                    (XG_GET_DATA(x, pat.num_rows - (y + 1)) == ZEN_BLACK))
                {
                    return FALSE;
                }
            }
        }
    }
    if (xg_nRules & RULE_LINESYMMETRYH) {
        for (int y = 0; y < pat.num_rows; ++y) {
            for (int x = 0; x < pat.num_columns; ++x) {
                if ((XG_GET_DATA(x, y) == ZEN_BLACK) !=
                    (XG_GET_DATA(pat.num_columns - (x + 1), y) == ZEN_BLACK))
                {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

#undef XG_GET_DATA

// パターンデータを読み込む。
BOOL XgLoadPatterns(LPCWSTR pszFileName, patterns_t& patterns, XGStringW *pComments)
{
    // パターンデータをクリアする。
    patterns.clear();

    // read all as UTF-8
    std::string utf8;
    CHAR buf[256];
    FILE *fp = _wfopen(pszFileName, L"rb");
    if (!fp)
        return FALSE;

    while (fgets(buf, _countof(buf), fp))
    {
        utf8 += buf;
    }
    fclose(fp);

    // BOMを消す。
    if (utf8.size() >= 3 && memcmp(&utf8[0], "\xEF\xBB\xBF", 3) == 0) {
        utf8 = utf8.substr(3);
    }

    // split as UTF-16 lines
    auto utf16 = XgUtf8ToUnicode(utf8);
    std::vector<XGStringW> lines;
    mstr_split(lines, utf16, L"\n");

    utf8.clear();

    XG_PATDATA pat;
    XGStringW text, comments;

    // parse each line
    for (auto& line : lines) {
        xg_str_trim(line);
        if (line.empty())
            continue;

        switch (line[0]) {
        case L';':
            comments += line;
            comments += L"\r\n";
            break;
        case 0x250F: // ┏
            text.clear();
            text += line;
            text += L"\r\n";
            pat.num_columns = pat.num_rows = 0;
            break;
        case 0x2517: // ┗
            text += line;
            text += L"\r\n";
            pat.text = text;
            // パターンのデータを扱いやすいよう、加工する。
            XgGetPatternData(pat);
            // 追加する。
            patterns.push_back(pat);
            break;
        case 0x2503: // ┃
            pat.num_columns = static_cast<int>(line.size() - 2);
            text += line;
            text += L"\r\n";
            pat.num_rows += 1;
            break;
        default:
            break;
        }
    }

    if (pComments)
        *pComments = std::move(comments);

    return TRUE;
}

// パターンデータを書き込む。
BOOL XgSavePatterns(LPCWSTR pszFileName, const patterns_t& patterns,
                    const XGStringW *pComments)
{
    XGStringW text;

    if (pComments) {
        text += *pComments;
        text += L"\r\n";
    }

    for (auto& pat : patterns) {
        text += to_XGStringW(pat.num_columns);
        text += L" x ";
        text += to_XGStringW(pat.num_rows);
        text += L" = ";
        text += to_XGStringW(pat.num_columns * pat.num_rows);
        text += L"\r\n";
        text += pat.text;
        text += L"\r\n";
    }

    FILE *fp = _wfopen(pszFileName, L"wb");
    if (!fp)
        return FALSE;

    auto utf8 = XgUnicodeToUtf8(text);
    fputs("\xEF\xBB\xBF", fp); // BOMを追加する。
    fputs(utf8.c_str(), fp);
    fclose(fp);
    return TRUE;
}

// ソートして一意化する。
VOID XgSortAndUniquePatterns(patterns_t& patterns, BOOL bExpand)
{
    patterns_t temp_pats; // 一時的なパターン
    if (bExpand) {
        // 反転・転置したパターンも追加する。
        for (const auto& pat : patterns) {
            if (XgIsPatternDividedByBlocks(pat))
                continue;
            temp_pats.push_back(pat);
            auto transposed = XgTransposePattern(pat);
            auto flip_h = XgFlipPatternH(pat);
            auto flip_v = XgFlipPatternV(pat);
            auto flip_hv = XgFlipPatternH(flip_v);
            temp_pats.push_back(transposed);
            temp_pats.push_back(flip_h);
            temp_pats.push_back(flip_v);
            temp_pats.push_back(flip_hv);
        }
    } else {
        // 反転・転置を考慮して一意化する。
        for (const auto& pat : patterns) {
            if (XgIsPatternDividedByBlocks(pat))
                continue;
            auto transposed = XgTransposePattern(pat);
            auto flip_h = XgFlipPatternH(pat);
            auto flip_v = XgFlipPatternV(pat);
            auto flip_hv = XgFlipPatternH(flip_v);
            temp_pats.erase(
                std::remove_if(temp_pats.begin(), temp_pats.end(), [&](const XG_PATDATA& pat2) noexcept {
                    return transposed.text == pat2.text ||
                           flip_h.text == pat2.text ||
                           flip_v.text == pat2.text ||
                           flip_hv.text == pat2.text;
                }),
                temp_pats.end()
            );
            temp_pats.push_back(pat);
        }
    }
    patterns = std::move(temp_pats);

    // ソートする。
    std::sort(patterns.begin(), patterns.end(), [](const XG_PATDATA& a, const XG_PATDATA& b) {
        if (a.num_columns < b.num_columns)
            return true;
        if (a.num_columns > b.num_columns)
            return false;
        if (a.num_rows < b.num_rows)
            return true;
        if (a.num_rows > b.num_rows)
            return false;
        return (a.text < b.text);
    });

    // 一意化する。
    auto last = std::unique(patterns.begin(), patterns.end(), [](const XG_PATDATA& a, const XG_PATDATA& b) noexcept {
        return a.text == b.text;
    });
    patterns.erase(last, patterns.end());
}

// パターンを現在の盤面から取得。
XG_PATDATA XgGetCurrentPat(void)
{
    // コピーする盤を選ぶ。
    XG_Board *pxw = (xg_bSolved && xg_bShowAnswer) ? &xg_solution : &xg_xword;

    // クロスワードの文字列を取得する。
    XGStringW text;
    pxw->GetString(text);

    for (auto& ch : text){
        switch (ch)
        {
        case ZEN_SPACE:
        case ZEN_BLACK:
        case '\n':
        case '\r':
        case 0x250F: // ┏
        case 0x2501: // ━
        case 0x2513: // ┓
        case 0x2503: // ┃
        case 0x2517: // ┗
        case 0x251B: // ┛
            break;
        default:
            ch = ZEN_SPACE;
            break;
        }
    }

    // パターンのデータを扱いやすいよう、加工する。
    XG_PATDATA pat = { xg_nCols, xg_nRows, text };
    XgGetPatternData(pat);

    return pat;
}

// パターン編集。
BOOL XgPatEdit(HWND hwnd, BOOL bAdd)
{
    // パターンを読み込む。
    patterns_t patterns;
    WCHAR szPath[MAX_PATH];
    XgFindLocalFile(szPath, _countof(szPath), L"PAT.txt");
    XGStringW comments;
    if (!XgLoadPatterns(szPath, patterns, &comments))
        return FALSE;

    // ソートして一意化する。
    XgSortAndUniquePatterns(patterns, FALSE);

    // 現在の盤面のパターンを取得。
    XG_PATDATA cur_pat = XgGetCurrentPat();

    // 分断禁。
    if (XgIsPatternDividedByBlocks(cur_pat))
        return FALSE;

    // 反転・転置した盤面も考慮。
    auto transposed = XgTransposePattern(cur_pat);
    auto flip_h = XgFlipPatternH(cur_pat);
    auto flip_v = XgFlipPatternV(cur_pat);
    auto flip_hv = XgFlipPatternH(flip_v);

    if (bAdd) { // 追加。
        patterns.push_back(cur_pat);
        patterns.push_back(transposed);
        patterns.push_back(flip_h);
        patterns.push_back(flip_v);
        patterns.push_back(flip_hv);
    } else { // 削除。
        patterns_t temp_pats; // 一時的なパターン
        for (const auto& pat : patterns) {
            if (pat.text == cur_pat.text ||
                pat.text == transposed.text ||
                pat.text == flip_h.text ||
                pat.text == flip_v.text ||
                pat.text == flip_hv.text)
            {
                continue;
            }
            temp_pats.push_back(pat);
        }
        patterns = std::move(temp_pats);
    }

    // ソートして一意化する。
    XgSortAndUniquePatterns(patterns, FALSE);

    // パターンを書き込む。
    return XgSavePatterns(szPath, patterns, &comments);
}

//////////////////////////////////////////////////////////////////////////////

// 候補があるか？
template <bool t_alternative>
bool __fastcall XgAnyCandidateAddBlack(const XGStringW& pattern) noexcept
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
        const XGStringW& word = data.m_word;
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
bool __fastcall XgAnyCandidateNoAddBlack(const XGStringW& pattern) noexcept
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
        const XGStringW& word = data.m_word;
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
void XG_BoardEx::ReCount() noexcept {
    int nCount = 0;
    for (int i = 0; i < m_nRows; ++i) {
        for (int j = 0; j < m_nCols; ++j) {
            nCount += (GetAt(i, j) != ZEN_SPACE);
        }
    }
    m_vCells[m_nRows * m_nCols] = static_cast<WCHAR>(nCount);
}

// 行を挿入する。
void XG_BoardEx::InsertRow(int iRow) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows + 1, m_nCols);
    for (int i = 0; i < m_nRows; ++i) {
        for (int j = 0; j < m_nCols; ++j) {
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
void XG_BoardEx::InsertColumn(int jCol) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows, m_nCols + 1);
    for (int i = 0; i < m_nRows; ++i) {
        for (int j = 0; j < m_nCols; ++j) {
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
void XG_BoardEx::DeleteRow(int iRow) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows - 1, m_nCols);
    for (int i = 0; i < m_nRows - 1; ++i) {
        for (int j = 0; j < m_nCols; ++j) {
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
void XG_BoardEx::DeleteColumn(int jCol) {
    XG_Board copy;
    copy.ResetAndSetSize(m_nRows, m_nCols - 1);
    for (int i = 0; i < m_nRows; ++i) {
        for (int j = 0; j < m_nCols - 1; ++j) {
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
bool __fastcall XgAnyCandidateWholeSpace(int patlen) noexcept
{
    // すべての単語について調べる。
    for (const auto& data : xg_dict_1) {
        const XGStringW& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());
        if (wordlen == patlen)
            return true;    // あった。
    }
    for (const auto& data : xg_dict_2) {
        const XGStringW& word = data.m_word;
        const int wordlen = static_cast<int>(word.size());
        if (wordlen == patlen)
            return true;    // あった。
    }
    return false;   // なかった。
}

// 四隅に黒マスがあるかどうか。
bool __fastcall XG_Board::CornerBlack() const noexcept
{
    return (GetAt(0, 0) == ZEN_BLACK ||
            GetAt(xg_nRows - 1, 0) == ZEN_BLACK ||
            GetAt(xg_nRows - 1, xg_nCols - 1) == ZEN_BLACK ||
            GetAt(0, xg_nCols - 1) == ZEN_BLACK);
}

// 黒マスが隣り合っているか？
bool __fastcall XG_Board::DoubleBlack() const noexcept
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
        for (int i0 = 0; i0 < n2; i0++) {
            if (GetAt(i0, j) == ZEN_BLACK && GetAt(i0 + 1, j) == ZEN_BLACK)
                return true;    // 隣り合っていた。
        }
    }
    return false;   // 隣り合っていなかった。
}

// 三方向が黒マスで囲まれたマスがあるかどうか？
bool __fastcall XG_Board::TriBlackAround() const noexcept
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
    std::vector<BYTE> pb(nCount, 0);

    // 位置のキュー。
    // 黒マスではないマスを探し、positionsに追加する。
    std::queue<XG_Pos> positions;
    if (GetAt(0, 0) != ZEN_BLACK) {
        positions.emplace(0, 0);
    } else {
        for (int i = 0; i < nRows; ++i) {
            for (int j = 0; j < nCols; ++j) {
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
    std::vector<XGStringW>& vNotFoundWords,
    XG_Pos& pos, bool bNonBlackCheckSpace) const
{
    const int nRows = xg_nRows, nCols = xg_nCols;

    // 使った単語の集合。
    std::unordered_set<XGStringW> used_words;
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    const WCHAR ch = GetAt(i, k);
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
                    const WCHAR ch = GetAt(i, k);
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    const WCHAR ch = GetAt(k, j);
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
                    const WCHAR ch = GetAt(k, j);
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
    std::vector<XGStringW>& vNotFoundWords,
    XG_Pos& pos, bool bNonBlackCheckSpace) const
{
    const int nRows = xg_nRows;
    const int nCols = xg_nCols;
    XG_Pos pos2 = pos;
    int& i = pos2.m_i;
    int& j = pos2.m_j;

    // 使った単語の集合。
    std::unordered_set<XGStringW> used_words;
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    const WCHAR ch = GetAt(i, k);
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
                    const WCHAR ch = GetAt(i, k);
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    const WCHAR ch = GetAt(k, j);
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
                    const WCHAR ch = GetAt(k, j);
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
    std::vector<XGStringW> vNotFoundWords;
    vNotFoundWords.reserve(xg_nRows * xg_nCols / 4);
    const XG_EpvCode code = EveryPatternValid2(vNotFoundWords, pos, false);
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
    std::vector<XGStringW> vNotFoundWords;
    vNotFoundWords.reserve(xg_nRows * xg_nCols / 4);
    const XG_EpvCode code = EveryPatternValid2(vNotFoundWords, pos, false);
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
    std::unordered_set<XGStringW> used_words;
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
    xg_vVertInfo.clear();
    xg_vHorzInfo.clear();

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
                XGStringW word;
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
                    xg_vVertInfo.emplace_back(lo, j, std::move(word));
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
                XGStringW word;
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
                    xg_vHorzInfo.emplace_back(i, lo, std::move(word));
                } else {
                    return false;   // 登録されていない単語のため、失敗。
                }
space_found_2:;
            }
        }
    }

    // カギの格納情報に順序をつける。
    std::vector<XG_PlaceInfo *> data;
    const int size1 = static_cast<int>(xg_vVertInfo.size());
    const int size2 = static_cast<int>(xg_vHorzInfo.size());
    data.reserve(size1 + size2);
    for (int k = 0; k < size1; k++) {
        data.emplace_back(&xg_vVertInfo[k]);
    }
    for (int k = 0; k < size2; k++) {
        data.emplace_back(&xg_vHorzInfo[k]);
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
    sort(xg_vVertInfo.begin(), xg_vVertInfo.end(), xg_placeinfo_compare_number());
    sort(xg_vHorzInfo.begin(), xg_vHorzInfo.end(), xg_placeinfo_compare_number());

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
    xg_vVertInfo.clear();
    xg_vHorzInfo.clear();

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
                XGStringW word;
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
                    xg_vVertInfo.emplace_back(lo, j, std::move(word));
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
                XGStringW word;
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
                    xg_vHorzInfo.emplace_back(i, lo, std::move(word));
                }
            }
        }
    }

    // カギの格納情報に順序をつける。
    std::vector<XG_PlaceInfo *> data;
    {
        const int size = static_cast<int>(xg_vVertInfo.size());
        for (int k = 0; k < size; k++) {
            data.emplace_back(&xg_vVertInfo[k]);
        }
    }
    {
        const int size = static_cast<int>(xg_vHorzInfo.size());
        for (int k = 0; k < size; k++) {
            data.emplace_back(&xg_vHorzInfo[k]);
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
    sort(xg_vVertInfo.begin(), xg_vVertInfo.end(), xg_placeinfo_compare_number());
    sort(xg_vHorzInfo.begin(), xg_vHorzInfo.end(), xg_placeinfo_compare_number());
}

// 候補を取得する。
template <bool t_alternative> bool __fastcall
XgGetCandidatesAddBlack(
    std::vector<XGStringW>& cands, const XGStringW& pattern, int& nSkip,
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
    XGStringW result(pattern);

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
        const XGStringW& word = data.m_word;
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
XgGetCandidatesNoAddBlack(std::vector<XGStringW>& cands, const XGStringW& pattern)
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
        const XGStringW& word = data.m_word;
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
    const auto nRows = xg_nRows, nCols = xg_nCols;

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
bool __fastcall XgParseHints(std::vector<XG_Hint>& hints, const XGStringW& str)
{
    // ヒントをクリアする。
    hints.clear();

    int count = 0;
    for (size_t i = 0;;) {
        if (count++ > 1000) {
            return false;
        }

        const auto i0 = str.find(XgLoadStringDx1(IDS_KEYLEFT), i);
        if (i0 == XGStringW::npos) {
            break;
        }

        const auto i1 = str.find_first_of(L"0123456789", i0);
        if (i1 == XGStringW::npos) {
            return false;
        }

        int number = _wtoi(str.data() + i1);
        if (number <= 0) {
            return false;
        }

        const auto i2 = str.find(XgLoadStringDx1(IDS_KEYRIGHT), i0);
        if (i2 == XGStringW::npos) {
            return false;
        }

        XGStringW word;
        size_t i3, i4;
        i3 = str.find(L"\x226A", i2); // U+226A ≪
        if (i3 != XGStringW::npos) {
            i3 += wcslen(L"\x226A"); // U+226A ≪

            i4 = str.find(L"\x226B", i3); // U+226B ≫
            if (i4 == XGStringW::npos) {
                return false;
            }
            word = str.substr(i3, i4 - i3);
            i4 += wcslen(L"\x226B"); // U+226B ≫
        } else {
            i4 = i2 + wcslen(XgLoadStringDx1(IDS_KEYRIGHT));
        }

        const auto i5 = str.find(XgLoadStringDx1(IDS_KEYLEFT), i4);
        if (i5 == XGStringW::npos) {
            XGStringW hint = str.substr(i4);
            xg_str_replace_all(hint, L"\r", L"");
            xg_str_replace_all(hint, L"\n", L"");
            xg_str_replace_all(hint, L"\t", L"");
            xg_str_trim(hint);
            hints.emplace_back(number, std::move(word), std::move(hint));
            break;
        } else {
            XGStringW hint = str.substr(i4, i5 - i4);
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
bool __fastcall XgParseHintsStr(const XGStringW& strHints)
{
    // ヒントをクリアする。
    xg_vecVertHints.clear();
    xg_vecHorzHints.clear();

    // ヒント文字列の前後の空白を取り除く。
    XGStringW str(strHints);
    xg_str_trim(str);

    // strCaption1とstrCaption2により、tateとyokoに分ける。
    XGStringW strCaption1 = XgLoadStringDx1(IDS_DOWN);
    XGStringW strCaption2 = XgLoadStringDx1(IDS_ACROSS);
    size_t i1 = str.find(strCaption1);
    if (i1 == XGStringW::npos)
        return false;
    i1 += strCaption1.size();
    size_t i2 = str.find(strCaption2);
    if (i2 == XGStringW::npos)
        return false;
    XGStringW tate = str.substr(i1, i2 - i1);
    i2 += strCaption2.size();
    XGStringW yoko = str.substr(i2);

    // 備考欄を取り除く。
    const auto i3 = yoko.find(XgLoadStringDx1(IDS_HEADERSEP2));
    if (i3 != XGStringW::npos) {
        yoko = yoko.substr(0, i3);
    }

    // 前後の空白を取り除く。
    xg_str_trim(tate);
    xg_str_trim(yoko);

    // それぞれについて解析する。
    return XgParseHints(xg_vecVertHints, tate) &&
           XgParseHints(xg_vecHorzHints, yoko);
}

// ヒントを取得する。
void __fastcall
XgGetHintsStr(const XG_Board& board, XGStringW& str, int hint_type, bool bShowAnswer)
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
        str += L"\r\n";

        for (const auto& info : xg_vVertInfo) {
            // 番号を格納する。
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_DOWNNUMBER), info.m_number);
            str += sz;

            // 答えを見せるかどうか？
            if (bShowAnswer) {
                str += (WCHAR)0x226A; // U+226A ≪
                str += info.m_word;
                str += (WCHAR)0x226B; // U+226B ≫
            }

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecVertHints) {
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
            str += L"\r\n";   // 改行。
        }
        str += L"\r\n";
    }
    if (hint_type == 1 || hint_type == 2) {
        // ヨコのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_ACROSS);
        str += L"\r\n";
        for (const auto& info : xg_vHorzInfo) {
            // 番号を格納する。
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_ACROSSNUMBER), info.m_number);
            str += sz;

            // 答えを見せるかどうか？
            if (bShowAnswer) {
                str += (WCHAR)0x226A; // U+226A ≪
                str += info.m_word;
                str += (WCHAR)0x226B; // U+226B ≫
            }

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecHorzHints) {
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
            str += L"\r\n";   // 改行。
        }
        str += L"\r\n";
    }
    if (hint_type == 3 || hint_type == 5) {
        // タテのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_PARABOLD);     // <p><b>
        str += XgLoadStringDx1(IDS_DOWNLABEL);
        str += XgLoadStringDx1(IDS_ENDPARABOLD);    // </b></p>
        str += L"\r\n";
        str += XgLoadStringDx1(IDS_OL);    // <ol>
        str += L"\r\n";

        for (const auto& info : xg_vVertInfo) {
            // <li>
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_LI), info.m_number);
            str += sz;

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecVertHints) {
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
            str += L"\r\n";           // 改行。
        }
        str += XgLoadStringDx1(IDS_ENDOL);    // </ol>
        str += L"\r\n";           // 改行。
    }
    if (hint_type == 4 || hint_type == 5) {
        // ヨコのカギの文字列を構成する。
        str += XgLoadStringDx1(IDS_PARABOLD);     // <p><b>
        str += XgLoadStringDx1(IDS_ACROSSLABEL);
        str += XgLoadStringDx1(IDS_ENDPARABOLD);    // </b></p>
        str += L"\r\n";
        str += XgLoadStringDx1(IDS_OL);    // <ol>
        str += L"\r\n";

        for (const auto& info : xg_vHorzInfo) {
            // <li>
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_LI), info.m_number);
            str += sz;

            // ヒント文章を追加する。
            bool added = false;
            for (auto& hint : xg_vecHorzHints) {
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
            str += L"\r\n";           // 改行。
        }
        str += XgLoadStringDx1(IDS_ENDOL);    // </ol>
        str += L"\r\n";           // 改行。
    }
}

// 文字列のルールを解析する。
int __fastcall XgParseRules(const XGStringW& str)
{
    int nRules = 0;
    std::vector<XGStringW> rules;
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
XGStringW __fastcall XgGetRulesString(int rules)
{
    // メモ：英語対応のため、空白区切りから" / "区切りに変更しました。
    XGStringW ret;

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
bool __fastcall XgSetJsonString(HWND hwnd, const XGStringW& str)
{
    std::string utf8 = XgUnicodeToUtf8(str);

    bool success = true;
    try {
        json j0 = json::parse(utf8);
        int row_count = j0["row_count"];
        int column_count = j0["column_count"];
        auto cell_data = j0["cell_data"];
        const bool is_solved = j0["is_solved"];
        const bool has_mark = j0["has_mark"];
        const bool has_hints = j0["has_hints"];
        int rules = XG_DEFAULT_RULES;
        if (j0["policy"].is_number_integer()) {
            rules = static_cast<int>(j0["policy"]);
        } else if (j0["rules"].is_string()) {
            auto str0 = XgUtf8ToUnicode(j0["rules"].get<std::string>());
            rules = XgParseRules(str0);
        }
        XGStringW dictionary;
        if (j0["dictionary"].is_string()) {
            dictionary = XgUtf8ToUnicode(j0["dictionary"].get<std::string>());
        }
        if (j0["view_mode"].is_number_integer()) {
            switch (int(j0["view_mode"])) {
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
            throw 1;
        }

        if (row_count != static_cast<int>(cell_data.size())) {
            throw 2;
        }

        XG_Board xw;
        int nRowsSave = xg_nRows, nColsSave = xg_nCols;
        xg_nRows = row_count;
        xg_nCols = column_count;
        xw.ResetAndSetSize(row_count, column_count);

        // ボックス。
        XgDeleteBoxes();
        if (j0["boxes"].is_array()) {
            XgDoLoadBoxJson(j0["boxes"]);
        }

        if (!cell_data.is_array()) {
            success = false;
        }

        for (int i = 0; i < row_count; ++i) {
            if (!cell_data[i].is_string()) {
                success = false;
                break;
            }

            std::string str1 = cell_data[i];
            XGStringW row = XgUtf8ToUnicode(str1).c_str();
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
            auto& marks = j0["marks"];
            for (auto& mark : marks) {
                int i = static_cast<int>(mark[0]) - 1;
                int j = static_cast<int>(mark[1]) - 1;
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
            auto hints = j0["hints"];
            auto v = hints["v"];
            auto h = hints["h"];
            for (size_t i = 0; i < v.size(); ++i) {
                auto data = v[i];
                int number = static_cast<int>(data[0]);
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
                int number = static_cast<int>(data[0]);
                if (number <= 0) {
                    success = false;
                    break;
                }
                auto word = XgUtf8ToUnicode(data[1]);
                auto hint = XgUtf8ToUnicode(data[2]);
                yoko.emplace_back(number, word, hint);
            }
        }

        auto header = XgUtf8ToUnicode(j0["header"]);
        auto notes = XgUtf8ToUnicode(j0["notes"]);
        if (j0["theme"].is_string()) {
            xg_strTheme = XgUtf8ToUnicode(j0["theme"]);
        } else {
            xg_strTheme = xg_strDefaultTheme;
        }

        if (success) {
            // カギをクリアする。
            xg_vVertInfo.clear();
            xg_vHorzInfo.clear();

            if (is_solved) {
                xg_bSolved = true;
                xg_bCheckingAnswer = false;
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
                xg_bCheckingAnswer = false;
                xg_xword = xw;
            }
            xg_vMarks = mark_positions;
            xg_vecVertHints = tate;
            xg_vecHorzHints = yoko;
            xg_strHeader = header;
            xg_str_trim(xg_strHeader);
            xg_strNotes = notes;
            xg_str_trim(xg_strNotes);

            LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
            if (xg_strNotes.empty()) {
                ;
            } else if (xg_strNotes.find(psz) == 0) {
                xg_strNotes = xg_strNotes.substr(XGStringW(psz).size());
                xg_str_trim(xg_strNotes);
            }

            // ヒント追加フラグをクリアする。
            xg_bHintsAdded = false;

            // ルールを設定する。
            xg_nRules = rules | RULE_DONTDIVIDE;

            // ナンクロモード。
            xg_bNumCroMode = false;
            xg_mapNumCro1.clear();
            xg_mapNumCro2.clear();
            do {
                if (is_solved &&
                    j0["is_numcro"].is_boolean() && j0["is_numcro"] &&
                    j0["num_cro"].is_object())
                {
                    for (auto& pair : j0["num_cro"].items()) {
                        int number = strtoul(pair.key().c_str(), nullptr, 10);
                        auto value = XgUtf8ToUnicode(pair.value());
                        if (value.size() != 1)
                            break;

                        WCHAR ch = value[0];
                        xg_mapNumCro1[ch] = number;
                        xg_mapNumCro2[number] = ch;
                    }

                    if (XgValidateNumCro(xg_hMainWnd)) {
                        xg_bNumCroMode = true;
                    } else {
                        xg_mapNumCro1.clear();
                        xg_mapNumCro2.clear();
                        break;
                    }
                }
            } while (0);

            // 辞書名の辞書を読み込む。
            if (dictionary.size()) {
                for (auto& entry : xg_dicts) {
                    const auto& file = entry.m_filename;
                    if (file.find(dictionary) != XGStringW::npos) {
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

    }
    catch (json::exception&)
    {
        success = false;
    }

    return success;
}

// 文字列を設定する。
bool __fastcall XgSetStdString(HWND hwnd, const XGStringW& str)
{
    // クロスワードデータ。
    XG_Board xword;

    // 文字列を読み込む。
    if (!xword.SetString(str))
        return false;

    // 空きマスが存在しないか？
    const bool bFulfilled = xword.IsFulfilled();
    if (bFulfilled) {
        // 空きマスがなかった。

        // ヒントを設定する。
        const auto i0 = str.find(ZEN_LRIGHT, 0);
        if (i0 != XGStringW::npos) {
            // フッターを取得する。
            XGStringW s = str.substr(i0 + 1);

            // フッターの備考欄を取得して、取り除く。
            XGStringW strFooterSep = XgLoadStringDx1(IDS_HEADERSEP2);
            const auto i3 = s.find(strFooterSep);
            if (i3 != XGStringW::npos) {
                xg_strNotes = s.substr(i3 + strFooterSep.size());
                s = s.substr(0, i3);
            }

            LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
            if (xg_strNotes.find(psz) == 0) {
                xg_strNotes = xg_strNotes.substr(XGStringW(psz).size());
                xg_str_trim(xg_strNotes);
            }

            // ヒントの前後の空白を取り除く。
            xg_str_trim(s);

            // ヒント文字列を設定する。
            if (!s.empty()) {
                xg_strHints = s;
                xg_strHints += L"\r\n";
            }
        } else {
            // ヒントがない。
            xg_strHints.clear();
        }

        // ヒント文字列を解析する。
        if (xg_strHints.empty() || !XgParseHintsStr(xg_strHints)) {
            // 失敗した。
            xg_strHints.clear();
            xg_vecVertHints.clear();
            xg_vecHorzHints.clear();
        } else {
            // カギに単語が書かれていなかった場合の処理。
            for (auto& hint : xg_vecVertHints) {
                if (hint.m_strWord.size())
                    continue;

                for (const auto& info : xg_vVertInfo) {
                    if (info.m_number == hint.m_number) {
                        XGStringW word;
                        for (int k = info.m_iRow; k < xg_nRows; ++k) {
                            const WCHAR ch = xword.GetAt(k, info.m_jCol);
                            if (ch == ZEN_BLACK)
                                break;
                            word += ch;
                        }
                        hint.m_strWord = std::move(word);
                        break;
                    }
                }
            }
            for (auto& hint : xg_vecHorzHints) {
                if (hint.m_strWord.size())
                    continue;

                for (const auto& info : xg_vHorzInfo) {
                    if (info.m_number == hint.m_number) {
                        XGStringW word;
                        for (int k = info.m_jCol; k < xg_nCols; ++k) {
                            const auto ch = xword.GetAt(info.m_iRow, k);
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
            xg_bCheckingAnswer = false;
            xg_bShowAnswer = false;
        } else {
            // 空きマスがなく、ヒントがあった。これは解である。
            xg_solution = xword;
            xg_bSolved = true;
            xg_bCheckingAnswer = false;
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
        xg_bCheckingAnswer = false;
        xg_bShowAnswer = false;
    }

    // ヒント追加フラグをクリアする。
    xg_bHintsAdded = false;

    // マーク文字列を読み込む。
    XgSetStringOfMarks(str.data());

    return true;
}

bool __fastcall XgSetXDString(HWND hwnd, const XGStringW& str)
{
    XGStringW header, notes;
    std::vector<XGStringW> lines, rows, clues, marks;
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
                // 二重マス単語。
                if (line.find(L"MARK") == 0 && L'0' <= line[4] && line[4] <= L'9') {
                    marks.push_back(line);
                } else if (line.find(L"NUMCRO-") == 0 && L'0' <= line[7] && line[7] <= L'9') {
                    // ナンクロモード。
                    const auto ich = line.find(L": ");
                    int number = wcstoul(&line.c_str()[7], nullptr, 10);
                    WCHAR ch = line.c_str()[ich + 2];
                    xg_mapNumCro1[ch] = number;
                    xg_mapNumCro2[number] = ch;
                } else if (line.find(L"Box: ") == 0) {
                    // ボックス。
                    XgLoadXdBox(line);
                } else if (line.find(L"ViewMode:") == 0) {
                    // ビューモード。
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
                } else if (line.find(L"Policy:") == 0) {
                    xg_nRules = (_wtoi(&line[7]) | RULE_DONTDIVIDE);
                } else {
                    notes += line;
                    notes += L"\r\n";
                }
                break;
            default:
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
            else if (ch == L'#' || ch == ZEN_SHARP1 || ch == ZEN_SHARP2 || ch == L'.' || ch == ZEN_DOT)
                ch = ZEN_BLACK;
        }
        line = XgNormalizeString(line);
    }

    bool bOK = false;
    XG_Board xword;
    std::vector<XG_Hint> tate, yoko;
    {
        XGStringW str0;
        const auto nWidth = static_cast<int>(rows[0].size());

        str0 += ZEN_ULEFT;
        for (int i = 0; i < nWidth; ++i) {
            str0 += ZEN_HLINE;
        }
        str0 += ZEN_URIGHT;
        str0 += L"\r\n";

        for (auto& item : rows) {
            str0 += ZEN_VLINE;
            for (int i = 0; i < nWidth; ++i) {
                str0 += item[i];
            }
            str0 += ZEN_VLINE;
            str0 += L"\r\n";
        }

        str0 += ZEN_LLEFT;
        for (int i = 0; i < nWidth; ++i) {
            str0 += ZEN_HLINE;
        }
        str0 += ZEN_LRIGHT;
        str0 += L"\r\n";

        // 文字列を読み込む。
        bOK = xword.SetString(str0);
    }

    if (!bOK)
        return false;

    xg_str_trim(header);
    xg_strHeader = header;
    xg_str_trim(notes);
    xg_strNotes = notes;
    xg_nCols = static_cast<int>(rows[0].size());
    xg_nRows = static_cast<int>(rows.size());
    xg_vecVertHints.clear();
    xg_vecHorzHints.clear();
    xg_bSolved = false;
    xg_bCheckingAnswer = false;
    xg_nViewMode = view_mode;

    if (xword.IsFulfilled() && clues.size()) {
        // 番号付けを行う。
        xword.DoNumberingNoCheck();

        for (auto& str0 : clues) {
            xg_str_trim(str0);
            if (str0.empty() || (str0[0] != L'A' && str0[0] != L'D'))
                break;
            const auto iDot = str0.find(L'.');
            const auto iTilda = str0.rfind(L'~');
            if (iDot != str0.npos && iTilda != str0.npos) {
                auto word = str0.substr(iTilda + 1);
                xg_str_trim(word);
                auto hint = str0.substr(iDot + 1, iTilda - (iDot + 1));
                xg_str_trim(hint);
                word = XgNormalizeString(word);
                for (XG_PlaceInfo& item : xg_vVertInfo) {
                    if (item.m_word == word) {
                        tate.emplace_back(item.m_number, word, hint);
                        break;
                    }
                }
                for (XG_PlaceInfo& item : xg_vHorzInfo) {
                    if (item.m_word == word) {
                        yoko.emplace_back(item.m_number, word, hint);
                        break;
                    }
                }
            }
        }

        // 不足分を追加。
        for (XG_PlaceInfo& item : xg_vHorzInfo) {
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
        for (XG_PlaceInfo& item : xg_vVertInfo) {
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
                xg_bCheckingAnswer = false;
                xg_bShowAnswer = false;
                XgClearNonBlocks();
                xg_vecVertHints = tate;
                xg_vecHorzHints = yoko;
            }

            for (auto& mark : marks) {
                if (mark.substr(0, 4) == L"MARK" && L'0' <= mark[4] && mark[4] <= L'9') {
                    const auto i0 = mark.find(L'(');
                    const auto i1 = mark.find(L", ");
                    const int x = _wtoi(&mark[i0 + 1]) - 1;
                    const int y = _wtoi(&mark[i1 + 2]) - 1;
                    XgSetMark(XG_Pos(y, x));
                }
            }

            // ナンクロモード。
            if (XgValidateNumCro(xg_hMainWnd)) {
                xg_bNumCroMode = true;
            } else {
                xg_bNumCroMode = false;
                xg_mapNumCro1.clear();
                xg_mapNumCro2.clear();
            }

            return true;
        }
    }

    // 成功。
    xg_xword = xword;
    xg_solution.ResetAndSetSize(xg_nRows, xg_nCols);
    xg_bSolved = false;
    xg_bCheckingAnswer = false;
    xg_bShowAnswer = false;
    xg_vecVertHints.clear();
    xg_vecHorzHints.clear();
    return true;
}

struct ECW_ENTRY
{
    XGStringW word;
    size_t iColumn;
    size_t iRow;
    XGStringW clue;
    bool across;
};

bool __fastcall XgParseEcwEntry(std::vector<ECW_ENTRY>& entries, const XGStringW& line, bool across)
{
    std::vector<XGStringW> fields;
    mstr_split(fields, line, L":");
    if (fields.size() < 3)
        return false;
    xg_str_trim(fields[0]);
    xg_str_trim(fields[1]);
    xg_str_trim(fields[2]);
    if (fields[0].empty())
        return false;
    if (fields[1].empty())
        return false;

    std::vector<XGStringW> column_and_row;
    mstr_split(column_and_row, fields[1], L",");
    if (column_and_row.size() != 2)
        return false;

    xg_str_trim(column_and_row[0]);
    xg_str_trim(column_and_row[1]);
    int iColumn = _wtoi(column_and_row[0].c_str());
    int iRow = _wtoi(column_and_row[1].c_str());
    if (iColumn <= 0 || iRow <= 0)
        return false;

    ECW_ENTRY entry = { XgNormalizeString(fields[0]), static_cast<size_t>(iColumn), static_cast<size_t>(iRow), fields[2], across };
    entries.push_back(entry);

    return true;
}

// EclipseCrosswordのECWファイルの文字列を読み込む。
bool __fastcall XgSetEcwString(HWND hwnd, const XGStringW& str)
{
    std::vector<XGStringW> lines;
    mstr_split(lines, str, L"\n");

    XGStringW title, copyright, author;
    size_t width = 0, height = 0;
    std::vector<ECW_ENTRY> entries;
    bool across = false, down = false;
    for (auto& line : lines) {
        xg_str_trim(line);

        // Ignore empty lines
        if (line.empty())
            continue;

        // Ignore comment lines
        if (line[0] == L';')
            continue;

        size_t index;

        index = line.find(L"Title:");
        if (index == 0) {
            title = &line.c_str()[6];
            xg_str_trim(title);
            continue;
        }

        index = line.find(L"Author:");
        if (index == 0) {
            author = &line.c_str()[7];
            xg_str_trim(author);
            continue;
        }

        index = line.find(L"Width:");
        if (index == 0) {
            width = _wtoi(&line.c_str()[6]);
            if (width <= 1 || width > XG_MAX_SIZE)
                return false;
            continue;
        }

        index = line.find(L"Height:");
        if (index == 0) {
            height = _wtoi(&line.c_str()[7]);
            if (height <= 1 || height > XG_MAX_SIZE)
                return false;
            continue;
        }

        index = line.find(L"CodePage:");
        if (index == 0) {
            continue;
        }

        index = line.find(L"Copyright:");
        if (index == 0) {
            copyright = &line.c_str()[10];
            xg_str_trim(copyright);
            continue;
        }

        if (line[0] == L'*') {
            line = line.substr(1);
            xg_str_trim(line);
            if (lstrcmpiW(line.c_str(), L"Across words") == 0) {
                across = true;
                down = false;
                continue;
            }
            if (lstrcmpiW(line.c_str(), L"Down words") == 0) {
                across = false;
                down = true;
                continue;
            }
        }

        if (across) {
            XgParseEcwEntry(entries, line, true);
            continue;
        }
        if (down) {
            XgParseEcwEntry(entries, line, false);
            continue;
        }
    }

    if (width <= 1 || height <= 1)
        return false;

    std::vector<XGStringW> cells;
    cells.resize(height, XGStringW(width, ZEN_BLACK));

    for (auto& entry : entries) {
        for (size_t i = 0; i < entry.word.size(); ++i) {
            if (entry.across) {
                if (!(1 <= entry.iColumn + i && entry.iColumn + i <= width)) {
                    assert(0);
                    return false;
                }
                if (!(1 <= entry.iRow && entry.iRow <= height)) {
                    assert(0);
                    return false;
                }
                cells[entry.iRow - 1][i + entry.iColumn - 1] = entry.word[i];
            } else {
                if (!(1 <= entry.iColumn && entry.iColumn <= width)) {
                    assert(0);
                    return false;
                }
                if (!(1 <= entry.iRow + i && entry.iRow + i <= height)) {
                    assert(0);
                    return false;
                }
                cells[i + entry.iRow - 1][entry.iColumn - 1] = entry.word[i];
            }
        }
    }

    XGStringW body = ZEN_ULEFT + XGStringW(width, ZEN_HLINE) + ZEN_URIGHT + L"\r\n";
    for (size_t i = 0; i < height; ++i) {
        body += ZEN_VLINE;
        body += XgNormalizeString(cells[i]);
        body += ZEN_VLINE;
        body += L"\r\n";
    }
    body += ZEN_LLEFT + XGStringW(width, ZEN_HLINE) + ZEN_LRIGHT + L"\r\n";

    XG_Board xword;
    if (!xword.SetString(body))
        return false;

    XGStringW header;
    header += L"Title: " + title + L"\r\n";
    header += L"Author: " + author + L"\r\n";
    header += L"Copyright: " + copyright;

    xg_strHeader = header;
    xg_strNotes.clear();
    xg_nCols = width;
    xg_nRows = height;
    xg_vecVertHints.clear();
    xg_vecHorzHints.clear();
    xg_bSolved = false;
    xg_bCheckingAnswer = false;
    xg_nViewMode = XG_VIEW_NORMAL;

    // 番号付けを行う。
    xword.DoNumberingNoCheck();

    std::vector<XG_Hint> tate, yoko;
    for (auto& entry : entries) {
        entry.word = XgNormalizeString(entry.word);
        if (entry.across) {
            for (XG_PlaceInfo& item : xg_vHorzInfo) {
                if (item.m_word == entry.word) {
                    yoko.emplace_back(item.m_number, entry.word, entry.clue);
                    break;
                }
            }
        } else {
            for (XG_PlaceInfo& item : xg_vVertInfo) {
                if (item.m_word == entry.word) {
                    tate.emplace_back(item.m_number, entry.word, entry.clue);
                    break;
                }
            }
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
            xg_bCheckingAnswer = false;
            xg_bShowAnswer = false;
            XgClearNonBlocks();
            xg_vecVertHints = tate;
            xg_vecHorzHints = yoko;
        }

        xg_vMarks.clear();
        xg_bNumCroMode = false;

        return true;
    }

    // 失敗。
    return false;
}

// 文字列を設定する。
bool __fastcall XgSetString(HWND hwnd, const XGStringW& str, XG_FILETYPE type)
{
    // ボックスを削除する。
    XgDeleteBoxes();

    // ナンクロモード。
    xg_bNumCroMode = false;
    xg_mapNumCro1.clear();
    xg_mapNumCro2.clear();

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
    case XG_FILETYPE_ECW:
        return XgSetEcwString(hwnd, str);
    default:
        return (XgSetXDString(hwnd, str) || XgSetJsonString(hwnd, str) ||
                XgSetStdString(hwnd, str));
    }
}

// スレッド情報を取得する。
XG_ThreadInfo *__fastcall XgGetThreadInfo(void) noexcept
{
#ifdef SINGLE_THREAD_MODE
    return &xg_aThreadInfo[0];
#else
    const DWORD threadid = ::GetCurrentThreadId();
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        if (xg_aThreadInfo[i].m_threadid == threadid)
            return &xg_aThreadInfo[i];
    }
#endif
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
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = j, hi = j;
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(i, k);
                }

                const bool left_black_check = (lo != 0);
                const bool right_black_check = (hi + 1 != nCols);

                // パターンにマッチする候補を取得する。
                int nSkip;
                std::vector<XGStringW> cands;
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
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i + 1, j);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = i, hi = i;
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(k, j);
                }

                const bool left_black_check = (lo != 0);
                const bool right_black_check = (hi + 1 != nRows);

                // パターンにマッチする候補を取得する。
                int nSkip;
                std::vector<XGStringW> cands;
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
    ::EnterCriticalSection(&xg_csLock);
    if (!xg_bCancelled && !xg_bSolved && xw.IsSolution()) { // 解だった。
        xg_bSolved = true;
        xg_bCheckingAnswer = false;
        xg_solution = xw;
        xg_solution.DoNumbering();

        // 解に合わせて、問題に黒マスを置く。
        const int nCount = nRows * nCols;
        for (int i = 0; i < nCount; i++) {
            if (xw.GetAt(i) == ZEN_BLACK)
                xg_xword.SetAt(i, ZEN_BLACK);
        }
    }
    ::LeaveCriticalSection(&xg_csLock);
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
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i, j + 1);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = j, hi = j;
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(i, k);
                }

                // パターンにマッチする候補を取得する（黒マス追加なし）。
                std::vector<XGStringW> cands;
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
            const WCHAR ch1 = xw.GetAt(i, j), ch2 = xw.GetAt(i + 1, j);
            if ((ch1 == ZEN_SPACE && ch2 != ZEN_BLACK && ch2 != ZEN_SPACE) ||
                (ch1 != ZEN_SPACE && ch1 != ZEN_BLACK && ch2 == ZEN_SPACE))
            {
                // 文字が置ける区間[lo, hi]を求める。
                int lo = i, hi = i;
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
                XGStringW pattern;
                for (int k = lo; k <= hi; k++) {
                    pattern += xw.GetAt(k, j);
                }

                // パターンにマッチする候補を取得する。
                std::vector<XGStringW> cands;
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
    ::EnterCriticalSection(&xg_csLock);
    if (!xg_bCancelled && !xg_bSolved && xw.IsSolution()) { // 解だった。
        xg_bSolved = true;
        xg_bCheckingAnswer = false;
        xg_solution = xw;
        xg_solution.DoNumbering();

        // 解に合わせて、問題に黒マスを置く。
        const int nCount = nRows * nCols;
        for (int i = 0; i < nCount; i++) {
            if (xw.GetAt(i) == ZEN_BLACK)
                xg_xword.SetAt(i, ZEN_BLACK);
        }
    }
    ::LeaveCriticalSection(&xg_csLock);
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
static void __fastcall XgSolveXWord_AddBlack(const XG_Board& xw)
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
                    const XGStringW& word = word_data.m_word;
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
                    const XGStringW& word = word_data.m_word;
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
                    const XGStringW& word = word_data.m_word;
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
                    const XGStringW& word = word_data.m_word;
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
static void __fastcall XgSolveXWord_NoAddBlack(const XG_Board& xw)
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
    int max_patlen = 0, max_i = -1, max_lo = -1;
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

                const int patlen = hi - lo + 1;
                if (patlen > max_patlen) {
                    max_patlen = patlen;
                    max_lo = lo;
                    max_i = i;
                }
            }
        }
    }

    // パターンの長さを求める。
    const int patlen = max_patlen, lo = max_lo, i = max_i;
    for (const auto& word_data : words) {
        // すでに解かれているなら、終了。
        // キャンセルされているなら、終了。
        // 再計算すべきなら、終了する。
        if (xg_bSolved || xg_bCancelled)
            return;

        // 単語とパターンの長さが等しいか？
        const XGStringW& word = word_data.m_word;
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
}

#ifdef NO_RANDOM
    int xg_random_seed = 0;
#endif

// マルチスレッド用の関数。
static unsigned __stdcall XgSolveProc_AddBlack(void *param)
{
    // スレッド情報を取得する。
    auto info = static_cast<XG_ThreadInfo *>(param);

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
static unsigned __stdcall XgSolveProc_NoAddBlack(void *param)
{
    // スレッド情報を取得する。
    auto info = static_cast<XG_ThreadInfo *>(param);

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
static unsigned __stdcall XgSolveProcSmart(void *param)
{
    // スレッド情報を取得する。
    auto info = static_cast<XG_ThreadInfo *>(param);

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
    XgGenerateBlacksSmart(nullptr);

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
void __fastcall XgStartSolve_NoAddBlack(void) noexcept
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
void __fastcall XgStartSolve_Smart(void) noexcept
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

    for (int i = 0; i < xg_nRows; ++i) {
        for (int j = 0; j < xg_nCols; ++j) {
            const WCHAR oldch = xg_xword.GetAt(i, j);
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

// ポイント単位をピクセル単位に変換する（X座標）。
float XPixelsFromPoints(HDC hDC, float points) noexcept
{
    return points * static_cast<float>(::GetDeviceCaps(hDC, LOGPIXELSX)) / 72;
}

// ポイント単位をピクセル単位に変換する（Y座標）。
float YPixelsFromPoints(HDC hDC, float points) noexcept
{
    return points * static_cast<float>(::GetDeviceCaps(hDC, LOGPIXELSY)) / 72;
}

// 文字を置き換える。
WCHAR XgReplaceChar(WCHAR ch)
{
    WCHAR new_ch, ch0 = ch;

    // 文字を変換する。
    if (xg_bHiragana) {
        LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch0, 1, &new_ch, 1);
        ch0 = new_ch;
    } else {
        LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch0, 1, &new_ch, 1);
        ch0 = new_ch;
    }

    if (XgIsCharKanaW(ch0) || ch == ZEN_PROLONG || xg_imode == xg_im_KANA || xg_imode == xg_im_KANJI) {
        if (xg_bLowercase) {
            LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE, &ch0, 1, &new_ch, 1);
            ch0 = new_ch;
        } else {
            LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_UPPERCASE, &ch0, 1, &new_ch, 1);
            ch0 = new_ch;
        }
    } else {
        if (xg_bLowercase) {
            LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch0, 1, &new_ch, 1);
            ch0 = new_ch;
        } else {
            LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch0, 1, &new_ch, 1);
            ch0 = new_ch;
        }
    }

    return ch0;
}

// 文字マスを描画する。
void XgDrawLetterCell(HDC hdc, WCHAR ch, RECT& rc, HFONT hFont)
{
    // 文字の背景は透明。塗りつぶさない。
    ::SetBkMode(hdc, TRANSPARENT);

    // 文字を書く。DrawTextだとなぜか失敗する。
    ::SetTextColor(hdc, xg_rgbBlackCellColor);
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFont);
    {
        SIZE siz;
        ::GetTextExtentPoint32W(hdc, &ch, 1, &siz);
        MRect rect = rc;
        MPoint pt = rect.CenterPoint();
        ::TextOutW(hdc, pt.x - siz.cx / 2, pt.y - siz.cy / 2, &ch, 1);
    }
    ::SelectObject(hdc, hFontOld);
}

// 通常の文字のフォントを作成する。
HFONT XgCreateNormalFont(INT nCellSize)
{
    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), XgLoadStringDx1(IDS_MONOFONT));
    if (xg_szCellFont[0])
        StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), xg_szCellFont);
    lf.lfHeight = -nCellSize * xg_nCellCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    return ::CreateFontIndirectW(&lf);
}

// 小さい文字のフォントを作成する。
HFONT XgCreateSmallFont(INT nCellSize)
{
    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    if (xg_szSmallFont[0])
        StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), xg_szSmallFont);
    lf.lfHeight = -nCellSize * xg_nSmallCharPercents / 100;
    lf.lfWidth = 0;
    lf.lfWeight = FW_NORMAL;
    lf.lfQuality = ANTIALIASED_QUALITY;
    lf.lfCharSet = SHIFTJIS_CHARSET;
    return ::CreateFontIndirectW(&lf);
}

std::unordered_set<XG_Pos> XgGetSlot(int number, BOOL vertical)
{
    std::unordered_set<XG_Pos> ret;
    if (!xg_bSolved)
        return ret;

    int i = -1, j = -1;
    if (vertical) {
        for (auto& info : xg_vVertInfo) {
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
        for (auto& info : xg_vHorzInfo) {
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
constexpr COLORREF c_rgbHighlight = RGB(255, 255, 140);
constexpr COLORREF c_rgbHighlightAndDblFrame = RGB(255, 155, 100);

// 答え合わせで一致しないセルの色。
constexpr COLORREF c_rgbUnmatchedCell = RGB(255, 191, 191);

// マスの数字を描画する。
void __fastcall
XgDrawCellNumber(HDC hdc, const RECT& rcCell, int i, int j, int number,
                 const std::unordered_set<XG_Pos>& slot)
{
    WCHAR sz[8];
    StringCchPrintf(sz, _countof(sz), L"%d", number);

    RECT rcText;
    SIZE siz;
    GetTextExtentPoint32(hdc, sz, lstrlen(sz), &siz);
    rcText = rcCell;
    rcText.right = rcCell.left + siz.cx;
    rcText.bottom = rcCell.top + siz.cy;

    ::SetBkMode(hdc, TRANSPARENT);

    // 文字の背景を塗りつぶす。
    const int nMarked = XgGetMarked(i, j);
    COLORREF rgbBack;
    if (xg_bCheckingAnswer && xg_bSolved &&
        xg_xword.GetAt(i, j) != xg_solution.GetAt(i, j))
    {
        // 答え合わせで一致しないセル。
        rgbBack = c_rgbUnmatchedCell;
    } else if (xg_bSolved && xg_bNumCroMode) {
        // 文字の背景を塗りつぶす。
        if (slot.count(XG_Pos(i, j)) > 0) {
            rgbBack = c_rgbHighlight;
        } else {
            rgbBack = xg_rgbWhiteCellColor;
        }
    } else {
        if (slot.count(XG_Pos(i, j)) > 0) {
            rgbBack = c_rgbHighlight;
        } else if (nMarked != -1 && !xg_bNumCroMode) {
            rgbBack = xg_rgbMarkedCellColor;
        } else {
            rgbBack = xg_rgbWhiteCellColor;
        }
    }
    // EMFにおいてFillRectが余分な枠線を描画する不具合の回避のため、NULL_PENを選択する。
    HGDIOBJ hPen2Old = ::SelectObject(hdc, ::GetStockObject(NULL_PEN));
    {
        HBRUSH hbr = ::CreateSolidBrush(rgbBack);
        ::FillRect(hdc, &rcText, hbr);
        ::DeleteObject(hbr);
    }
    ::SelectObject(hdc, hPen2Old);

    if (xg_bShowNumbering) {
        // 数字を描く。
        ::SetTextColor(hdc, xg_rgbBlackCellColor);
        ::TextOutW(hdc, rcText.left, rcText.top, sz, lstrlenW(sz));
    }
}

// 二重マスを描画する。
void XgDrawDoubleFrameCell(HDC hdc, int nMarked, const RECT& rc, int nCellSize, HPEN hThinPen,
                           int i, int j)
{
    // 二重マスの内側の枠を描く。
    if (xg_bDrawFrameForMarkedCell) {
        RECT rcFrame = rc;
        ::InflateRect(&rcFrame, -nCellSize / 9, -nCellSize / 9);
        ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));
        HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
        ::Rectangle(hdc, rcFrame.left, rcFrame.top, rcFrame.right + 1, rcFrame.bottom + 1);
        ::SelectObject(hdc, hPenOld);
    }

    // 二重マスの文字を描画するか？
    if (xg_bShowDoubleFrameLetters) {
        WCHAR sz[2];
        if (nMarked < static_cast<int>(xg_strDoubleFrameLetters.size()))
            StringCchPrintf(sz, _countof(sz), L"%c", xg_strDoubleFrameLetters[nMarked]);
        else
            StringCchPrintf(sz, _countof(sz), L"%c", ZEN_BLACK);

        // 二重マスの右下端の文字の背景を塗りつぶす。
        SIZE siz;
        GetTextExtentPoint32(hdc, sz, lstrlen(sz), &siz);
        RECT rcText = rc;
        rcText.left = rc.right - (siz.cx + siz.cy) / 2;
        rcText.top = rc.bottom - siz.cy;

        // EMFで枠線が余分に描画されてしまう不具合の回避のため、FillRectの前にNULL_PENを選択する。
        HGDIOBJ hPen2Old = ::SelectObject(hdc, ::GetStockObject(NULL_PEN));
        {
            HBRUSH hbr;
            if (i != -1 && j != -1 &&
                xg_bCheckingAnswer && xg_bSolved &&
                xg_xword.GetAt(i, j) != xg_solution.GetAt(i, j))
            {
                // 答え合わせで一致しないセル。
                hbr = ::CreateSolidBrush(c_rgbUnmatchedCell);
            } else {
                // その他の場合。
                hbr = ::CreateSolidBrush(xg_rgbMarkedCellColor);
            }

            ::FillRect(hdc, &rcText, hbr);
            ::DeleteObject(hbr);
        }
        ::SelectObject(hdc, hPen2Old);

        // 二重マスの右下端の文字を描く。
        ::SetBkMode(hdc, TRANSPARENT);
        ::SetTextColor(hdc, xg_rgbBlackCellColor);
        ::TextOutW(hdc, rcText.left, rcText.top - 1, sz, lstrlenW(sz));
    }
}

// マス位置からイメージ座標を取得する。
void __fastcall XgCellToImage(RECT& rc, XG_Pos cell)
{
    // セルの大きさ。
    const int nCellSize = (xg_nCellSize * xg_nZoomRate / 100);

    rc = {
        xg_nMargin + cell.m_j * nCellSize,
        xg_nMargin + cell.m_i * nCellSize,
        xg_nMargin + (cell.m_j + 1) * nCellSize,
        xg_nMargin + (cell.m_i + 1) * nCellSize
    };
}

// マス１個だけ描画する。画面表示のみを想定。
void __fastcall XgDrawOneCell(HDC hdc, INT iRow, INT jCol)
{
    // セルの幅。
    const int nCellSize = (xg_nCellSize * xg_nZoomRate / 100);

    // 線の太さ。
    int c_nThin = static_cast<int>(YPixelsFromPoints(hdc, xg_nLineWidthInPt));
    int c_nWide = static_cast<int>(YPixelsFromPoints(hdc, xg_nOuterFrameInPt));
    if (c_nThin < 2)
        c_nThin = 2;
    if (c_nWide < 2)
        c_nWide = 2;
    if (c_nWide > xg_nMargin)
        c_nWide = xg_nMargin;

    // 黒いブラシ。
    LOGBRUSH lbBlack;
    lbBlack.lbStyle = BS_SOLID;
    lbBlack.lbColor = xg_rgbBlackCellColor;

    // 細いペンを作成し、選択する。
    HPEN hThinPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nThin, &lbBlack, 0, nullptr);

    // 文字マスのフォントを作成する。
    HFONT hFontNormal = XgCreateNormalFont(nCellSize);

    // 小さい文字のフォントを作成する。
    HFONT hFontSmall = XgCreateSmallFont(nCellSize);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);
    HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
    HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);
    HBRUSH hbrUnmatched = ::CreateSolidBrush(c_rgbUnmatchedCell);

    auto slot = XgGetSlot(xg_highlight.m_number, xg_highlight.m_vertical);

    // 黒マスビットマップの情報を取得する。
    BITMAP bm;
    ::GetObject(xg_hbmBlackCell, sizeof(bm), &bm);

    // 黒マスビットマップを選択する。
    HDC hdcMem = ::CreateCompatibleDC(nullptr);
    SelectObject(hdcMem, xg_hbmBlackCell);
    SetStretchBltMode(hdcMem, STRETCH_HALFTONE);

    // セルからイメージ座標へ。
    RECT rc;
    XgCellToImage(rc, { iRow, jCol });

    // 文字を取得する。
    WCHAR ch;
    if (xg_bSolved && xg_bShowAnswer)
        ch = xg_solution.GetAt(iRow, jCol);
    else
        ch = xg_xword.GetAt(iRow, jCol);

    // 二重マスか？
    const auto nMarked = XgGetMarked(iRow, jCol);

    // セルの背景を描画する。
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
    } else if (xg_bCheckingAnswer && xg_bSolved &&
               xg_xword.GetAt(iRow, jCol) != xg_solution.GetAt(iRow, jCol))
    {
        // 答え合わせで一致しないセル。
        ::FillRect(hdc, &rc, hbrUnmatched);
    } else if (slot.count(XG_Pos(iRow, jCol)) > 0 && nMarked != -1 && !xg_bNumCroMode) {
        // ハイライトかつ二重マスかつ非ナンクロ。
        ::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
    } else if (slot.count(XG_Pos(iRow, jCol)) > 0) {
        // ハイライト。
        ::FillRect(hdc, &rc, hbrHighlight);
    } else if (nMarked != -1 && !xg_bNumCroMode) {
        // 二重マスかつ非ナンクロ。
        ::FillRect(hdc, &rc, hbrMarked);
    } else {
        // その他のマス。
        ::FillRect(hdc, &rc, hbrWhite);
    }

    // デバイスコンテキストを破棄する。
    ::DeleteDC(hdcMem);

    // 小さい文字のフォントを選択する。
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall);

    // 二重マスを描画する。
    if (!xg_bNumCroMode && nMarked != -1) {
        XgDrawDoubleFrameCell(hdc, nMarked, rc, nCellSize, hThinPen, iRow, jCol);
    }

    // 番号を描画する。
    if (!xg_bNumCroMode) {
        for (auto& tate : xg_vVertInfo) {
            if (tate.m_iRow == iRow && tate.m_jCol == jCol) {
                RECT rc0 = rc;
                ::OffsetRect(&rc0, c_nThin, c_nThin * 2 / 3);
                XgDrawCellNumber(hdc, rc0, iRow, jCol, tate.m_number, slot);
            }
        }
        for (auto& yoko : xg_vHorzInfo) {
            if (yoko.m_iRow == iRow && yoko.m_jCol == jCol) {
                RECT rc0 = rc;
                ::OffsetRect(&rc0, c_nThin, c_nThin * 2 / 3);
                XgDrawCellNumber(hdc, rc0, iRow, jCol, yoko.m_number, slot);
            }
        }
    } else {
        const WCHAR ch0 = xg_solution.GetAt(iRow, jCol);
        ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);
        XgDrawCellNumber(hdc, rc, iRow, jCol, xg_mapNumCro1[ch0], slot);
    }

    // フォントの選択を解除する。
    ::SelectObject(hdc, hFontOld);

    // セルの文字を描画する。
    XgDrawLetterCell(hdc, XgReplaceChar(ch), rc, hFontNormal);

    // Rectangle関数ではうまくいかないので直接線を描画する。
    HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
    ::MoveToEx(hdc, rc.left, rc.top, nullptr);
    ::LineTo(hdc, rc.right, rc.top);
    ::LineTo(hdc, rc.right, rc.bottom);
    ::LineTo(hdc, rc.left, rc.bottom);
    ::LineTo(hdc, rc.left, rc.top);
    ::SelectObject(hdc, hPenOld);

    // 破棄する。
    ::DeleteObject(hThinPen);
    ::DeleteObject(hFontNormal);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
    ::DeleteObject(hbrHighlight);
    ::DeleteObject(hbrHighlightAndDblFrame);
    ::DeleteObject(hbrUnmatched);
}

// 二重マス単語を描画する。
void __fastcall XgDrawMarkWord(HDC hdc, const SIZE *psiz)
{
    const auto nCount = static_cast<int>(xg_vMarks.size());
    if (nCount == 0) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    auto slot = XgGetSlot(xg_highlight.m_number, xg_highlight.m_vertical);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);

    // 線の太さ。
    int c_nThin = static_cast<int>(YPixelsFromPoints(hdc, xg_nLineWidthInPt));
    int c_nWide = static_cast<int>(YPixelsFromPoints(hdc, xg_nOuterFrameInPt));
    if (c_nThin < 2)
        c_nThin = 2;
    if (c_nWide < 2)
        c_nWide = 2;
    if (c_nWide > xg_nNarrowMargin)
        c_nWide = xg_nNarrowMargin;

    // セルの幅。
    const int nCellSize = xg_nCellSize;

    // 黒いブラシ。
    LOGBRUSH lbBlack;
    lbBlack.lbStyle = BS_SOLID;
    lbBlack.lbColor = xg_rgbBlackCellColor;

    // 細いペンを作成し、選択する。
    HPEN hThinPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nThin, &lbBlack, 0, nullptr);

    // 文字マスのフォントを作成する。
    HFONT hFontNormal = XgCreateNormalFont(nCellSize);

    // 小さい文字のフォントを作成する。
    HFONT hFontSmall = XgCreateSmallFont(nCellSize);

    // 全体を白で塗りつぶす。
    RECT rc0 = { 0, 0, psiz->cx, psiz->cy };
    ::FillRect(hdc, &rc0, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

    // 周りに太い線を描く。
    if (xg_bAddThickFrame) {
        ::InflateRect(&rc0, -c_nWide, -c_nWide);
        ::FillRect(hdc, &rc0, hbrBlack);
        ::InflateRect(&rc0, +c_nWide, +c_nWide);
    }

    const auto& xw = (xg_bSolved && xg_bShowAnswer) ? xg_solution : xg_xword;

    // 二重マスを描画する。
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall);
    for (int i = 0; i < nCount; ++i) {
        RECT rc = {
            xg_nNarrowMargin + i * nCellSize,
            xg_nNarrowMargin,
            xg_nNarrowMargin + (i + 1) * nCellSize,
            xg_nNarrowMargin + nCellSize
        };

        // ナンクロなら白。さもなければ二重マスの色。
        if (xg_bNumCroMode) {
            ::FillRect(hdc, &rc, hbrWhite);
        } else {
            ::FillRect(hdc, &rc, hbrMarked);
        }

        const XG_Pos& pos = xg_vMarks[i];

        // ナンクロなら数字、さもなければ二重マスの文字を描画する。
        if (xg_bSolved && xg_bNumCroMode) {
            const WCHAR ch = xg_solution.GetAt(pos.m_i, pos.m_j);
            ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);
            XgDrawCellNumber(hdc, rc, pos.m_i, pos.m_j, xg_mapNumCro1[ch], slot);
        } else {
            XgDrawDoubleFrameCell(hdc, i, rc, nCellSize, hThinPen, -1, -1);
        }
    }
    ::SelectObject(hdc, hFontOld);

    // 塗りつぶさない。
    ::SelectObject(hdc, ::GetStockObject(NULL_BRUSH));

    // マスの文字とセルの枠を描画する。
    HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
    for (int i = 0; i < nCount; ++i) {
        RECT rc = {
            xg_nNarrowMargin + i * nCellSize,
            xg_nNarrowMargin,
            xg_nNarrowMargin + (i + 1) * nCellSize,
            xg_nNarrowMargin + nCellSize
        };

        const XG_Pos& pos = xg_vMarks[i];
        const WCHAR ch = XgReplaceChar(xw.GetAt(pos.m_i, pos.m_j));

        // マスの文字を描画する。
        XgDrawLetterCell(hdc, ch, rc, hFontNormal);

        // Rectangle関数ではうまくいかないので直接線を描画する。
        ::MoveToEx(hdc, rc.left, rc.top, nullptr);
        ::LineTo(hdc, rc.right, rc.top);
        ::LineTo(hdc, rc.right, rc.bottom);
        ::LineTo(hdc, rc.left, rc.bottom);
        ::LineTo(hdc, rc.left, rc.top);
    }
    ::SelectObject(hdc, hPenOld);

    // 破棄する。
    ::DeleteObject(hThinPen);
    ::DeleteObject(hFontNormal);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
}

// クロスワードを描画する（通常ビュー）。
void __fastcall XgDrawXWord_NormalView(const XG_Board& xw, HDC hdc, const SIZE *psiz, DRAW_MODE mode)
{
    // セルの大きさ。
    const int nCellSize = (xg_nForDisplay > 0) ? (xg_nCellSize * xg_nZoomRate / 100) : xg_nCellSize;

    // 全体を白で塗りつぶす。
    RECT rc0 = { 0, 0, psiz->cx, psiz->cy };
    ::SelectObject(hdc, ::GetStockObject(NULL_PEN));
    ::FillRect(hdc, &rc0, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));

    // 文字マスのフォントを作成する。
    HFONT hFontNormal = XgCreateNormalFont(nCellSize);

    // 小さい文字のフォントを作成する。
    HFONT hFontSmall = XgCreateSmallFont(nCellSize);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);
    HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
    HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);
    HBRUSH hbrUnmatched = ::CreateSolidBrush(c_rgbUnmatchedCell);

    auto slot = XgGetSlot(xg_highlight.m_number, xg_highlight.m_vertical);
    if (xg_nForDisplay <= 0)
        slot.clear();

    // 線の太さ。
    int c_nThin = 1, c_nWide = 4;
    switch (mode)
    {
    case DRAW_MODE_SCREEN:
    case DRAW_MODE_PRINT:
        c_nThin = static_cast<int>(YPixelsFromPoints(hdc, xg_nLineWidthInPt));
        c_nWide = static_cast<int>(YPixelsFromPoints(hdc, xg_nOuterFrameInPt));
        break;
    case DRAW_MODE_EMF:
        // FIXME: "Microsoft Print to PDF" で印刷すると線の幅がおかしくなる。
        // よくわからないので、1pt == 1pxで近似する。
        c_nThin = static_cast<int>(xg_nLineWidthInPt);
        c_nWide = static_cast<int>(xg_nOuterFrameInPt);
        break;
    }
    if (c_nThin < 2)
        c_nThin = 2;
    if (c_nWide < 2)
        c_nWide = 2;
    if (c_nWide > xg_nMargin)
        c_nWide = xg_nMargin;

    LOGBRUSH lbBlack;
    lbBlack.lbStyle = BS_SOLID;
    lbBlack.lbColor = xg_rgbBlackCellColor;

    // 黒の細いペンを作成する。
    HPEN hThinPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nThin, &lbBlack, 0, nullptr);

    BITMAP bm;
    GetObject(xg_hbmBlackCell, sizeof(bm), &bm);

    // 黒マスビットマップを選択する。
    HDC hdcMem = ::CreateCompatibleDC(nullptr);
    ::SelectObject(hdcMem, xg_hbmBlackCell);
    ::SetStretchBltMode(hdcMem, STRETCH_HALFTONE);

    // セルの背景を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            RECT rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize, 
                xg_nMargin + (i + 1) * nCellSize
            };

            // 二重マスか？
            const int nMarked = XgGetMarked(i, j);

            // 塗りつぶす。
            const WCHAR ch = xw.GetAt(i, j);
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
            } else if (xg_bCheckingAnswer && xg_bSolved &&
                       xg_xword.GetAt(i, j) != xg_solution.GetAt(i, j))
            {
                // 答え合わせで一致しないセル。
                ::FillRect(hdc, &rc, hbrUnmatched);
            } else if (slot.count(XG_Pos(i, j)) > 0 && nMarked != -1 && !xg_bNumCroMode) {
                // ハイライトかつ二重マスかつ非ナンクロ。
                ::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
            } else if (slot.count(XG_Pos(i, j)) > 0) {
                // ハイライト。
                ::FillRect(hdc, &rc, hbrHighlight);
            } else if (nMarked != -1 && !xg_bNumCroMode) {
                // 二重マスかつ非ナンクロ。
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
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall);

    // 二重マスを描画する。
    if (!xg_bNumCroMode) {
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                // セルの座標をセットする。
                RECT rc = {
                    xg_nMargin + j * nCellSize,
                    xg_nMargin + i * nCellSize,
                    xg_nMargin + (j + 1) * nCellSize,
                    xg_nMargin + (i + 1) * nCellSize
                };

                // 二重マスか？
                const auto nMarked = XgGetMarked(i, j);
                if (nMarked == -1)
                    continue;

                XgDrawDoubleFrameCell(hdc, nMarked, rc, nCellSize, hThinPen, i, j);
            }
        }
    }

    // タテのカギの先頭マス。
    if (!xg_bNumCroMode) {
        const int size = static_cast<int>(xg_vVertInfo.size());
        for (int k = 0; k < size; k++) {
            const auto i = xg_vVertInfo[k].m_iRow, j = xg_vVertInfo[k].m_jCol;

            RECT rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };
            ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

            XgDrawCellNumber(hdc, rc, i, j, xg_vVertInfo[k].m_number, slot);
        }
    }

    // ヨコのカギの先頭マス。
    if (!xg_bNumCroMode) {
        const int size = static_cast<int>(xg_vHorzInfo.size());
        for (int k = 0; k < size; k++) {
            const auto i = xg_vHorzInfo[k].m_iRow, j = xg_vHorzInfo[k].m_jCol;

            RECT rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };
            ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

            XgDrawCellNumber(hdc, rc, i, j, xg_vHorzInfo[k].m_number, slot);
        }
    }

    // ナンクロモードの小さい数字。
    if (xg_bSolved && xg_bNumCroMode) {
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                const auto ch = xg_solution.GetAt(i, j);
                if (ch == ZEN_BLACK || ch == ZEN_SPACE)
                    continue;

                RECT rc = {
                    xg_nMargin + j * nCellSize,
                    xg_nMargin + i * nCellSize,
                    xg_nMargin + (j + 1) * nCellSize,
                    xg_nMargin + (i + 1) * nCellSize
                };
                ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

                XgDrawCellNumber(hdc, rc, i, j, xg_mapNumCro1[ch], slot);
            }
        }
    }

    // フォントの選択を解除する。
    ::SelectObject(hdc, hFontOld);

    // セルの文字を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            // セルの座標をセットする。
            RECT rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };

            const auto ch = XgReplaceChar(xw.GetAt(i, j));
            if (ch == ZEN_BLACK)
                continue;

            XgDrawLetterCell(hdc, ch, rc, hFontNormal);
        }
    }

    // 線を引く。
    HGDIOBJ hPenOld = ::SelectObject(hdc, hThinPen);
    for (int i = 0; i <= xg_nRows; i++) {
        ::MoveToEx(hdc, xg_nMargin, static_cast<int>(xg_nMargin + i * nCellSize), nullptr);
        ::LineTo(hdc, psiz->cx - xg_nMargin, static_cast<int>(xg_nMargin + i * nCellSize));
    }
    for (int j = 0; j <= xg_nCols; j++) {
        ::MoveToEx(hdc, static_cast<int>(xg_nMargin + j * nCellSize), xg_nMargin, nullptr);
        ::LineTo(hdc, static_cast<int>(xg_nMargin + j * nCellSize), psiz->cy - xg_nMargin);
    }
    ::SelectObject(hdc, hPenOld);

    // 周りに太い線を描く。線を引こうと思ったが、コーナーがうまく描画できないので、
    // 塗りつぶしで対処する。
    hPenOld = ::SelectObject(hdc, GetStockBrush(NULL_PEN));
    if (xg_bAddThickFrame) {
        RECT rc = { xg_nMargin, xg_nMargin, psiz->cx - xg_nMargin, psiz->cy - xg_nMargin };
        ::InflateRect(&rc, c_nWide, c_nWide);
        RECT rc2;

        rc2 = rc;
        rc2.right = rc.left + c_nWide;
        ::FillRect(hdc, &rc2, hbrBlack);

        rc2 = rc;
        rc2.left = rc.right - c_nWide;
        ::FillRect(hdc, &rc2, hbrBlack);

        rc2 = rc;
        rc2.bottom = rc.top + c_nWide;
        ::FillRect(hdc, &rc2, hbrBlack);

        rc2 = rc;
        rc2.top = rc.bottom - c_nWide;
        ::FillRect(hdc, &rc2, hbrBlack);
    }
    ::SelectObject(hdc, hPenOld);

    // 破棄する。
    ::DeleteObject(hFontNormal);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hThinPen);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
    ::DeleteObject(hbrHighlight);
    ::DeleteObject(hbrHighlightAndDblFrame);
    ::DeleteObject(hbrUnmatched);
}

// クロスワードを描画する（スケルトンビュー）。
void __fastcall XgDrawXWord_SkeletonView(const XG_Board& xw, HDC hdc, const SIZE *psiz, DRAW_MODE mode)
{
    const int nCellSize = (xg_nForDisplay > 0) ? (xg_nCellSize * xg_nZoomRate / 100) : xg_nCellSize;

    // スクリーンの場合は、全体を白で塗りつぶす。
    // それ以外は、四隅に白い点を描く（EMFで余白が省略されないように）。
    RECT rc = { 0, 0, psiz->cx, psiz->cy };
    if (mode == DRAW_MODE_SCREEN) {
        ::FillRect(hdc, &rc, static_cast<HBRUSH>(::GetStockObject(WHITE_BRUSH)));
    } else {
        constexpr COLORREF rgbWhite = RGB(255, 255, 255);
        ::SetPixelV(hdc, rc.left, rc.top, rgbWhite);
        ::SetPixelV(hdc, rc.right - 1, rc.top, rgbWhite);
        ::SetPixelV(hdc, rc.right - 1, rc.bottom - 1, rgbWhite);
        ::SetPixelV(hdc, rc.left, rc.bottom - 1, rgbWhite);
    }

    // 文字マスのフォントを作成する。
    HFONT hFontNormal = XgCreateNormalFont(nCellSize);

    // 小さい文字のフォントを作成する。
    HFONT hFontSmall = XgCreateSmallFont(nCellSize);

    // ブラシを作成する。
    HBRUSH hbrBlack = ::CreateSolidBrush(xg_rgbBlackCellColor);
    HBRUSH hbrWhite = ::CreateSolidBrush(xg_rgbWhiteCellColor);
    HBRUSH hbrMarked = ::CreateSolidBrush(xg_rgbMarkedCellColor);
    HBRUSH hbrHighlight = ::CreateSolidBrush(c_rgbHighlight);
    HBRUSH hbrHighlightAndDblFrame = ::CreateSolidBrush(c_rgbHighlightAndDblFrame);
    HBRUSH hbrUnmatched = ::CreateSolidBrush(c_rgbUnmatchedCell);

    auto slot = XgGetSlot(xg_highlight.m_number, xg_highlight.m_vertical);
    if (xg_nForDisplay <= 0)
        slot.clear();

    // 線の太さ。
    int c_nThin = 1, c_nWide = 4;
    switch (mode)
    {
    case DRAW_MODE_SCREEN:
    case DRAW_MODE_PRINT:
        c_nThin = static_cast<int>(YPixelsFromPoints(hdc, xg_nLineWidthInPt));
        c_nWide = static_cast<int>(YPixelsFromPoints(hdc, xg_nOuterFrameInPt));
        break;
    case DRAW_MODE_EMF:
        // FIXME: "Microsoft Print to PDF" で印刷すると線の幅がおかしくなる。
        // よくわからないので、1pt == 1pxで近似する。
        c_nThin = static_cast<int>(xg_nLineWidthInPt);
        c_nWide = static_cast<int>(xg_nOuterFrameInPt);
        break;
    }
    if (c_nThin < 2)
        c_nThin = 2;
    if (c_nWide < 2)
        c_nWide = 2;

    // 黒いブラシ。
    LOGBRUSH lbBlack;
    lbBlack.lbStyle = BS_SOLID;
    lbBlack.lbColor = xg_rgbBlackCellColor;

    // 黒の細いペンを作成する。
    HPEN hThinPen = ::ExtCreatePen(
        PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_SQUARE | PS_JOIN_BEVEL,
        c_nThin, &lbBlack, 0, nullptr);

    // 外枠と背景を描画する。
    if (xg_bAddThickFrame) {
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                const WCHAR ch = xw.GetAt(i, j);
                if (ch == ZEN_BLACK)
                    continue;

                // セルの座標をセットする。
                rc = {
                    xg_nMargin + j * nCellSize,
                    xg_nMargin + i * nCellSize,
                    xg_nMargin + (j + 1) * nCellSize,
                    xg_nMargin + (i + 1) * nCellSize
                };

                RECT rcExtended = rc;
                ::InflateRect(&rcExtended, c_nWide, c_nWide);

                // 背景を塗りつぶす。
                ::FillRect(hdc, &rcExtended, hbrBlack);
            }
        }
    }

    // 文字の背景を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            const WCHAR ch = xw.GetAt(i, j);
            if (ch == ZEN_BLACK)
                continue;

            // セルの座標をセットする。
            rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };

            // 二重マスか？
            const int nMarked = XgGetMarked(i, j);

            // 塗りつぶす。
            if (xg_bCheckingAnswer && xg_bSolved &&
                xg_xword.GetAt(i, j) != xg_solution.GetAt(i, j))
            {
                // 答え合わせで一致しないセル。
                ::FillRect(hdc, &rc, hbrUnmatched);
            } else if (slot.count(XG_Pos(i, j)) > 0 && nMarked != -1 && !xg_bNumCroMode) {
                // ハイライトかつ二重マスかつ非ナンクロ。
                ::FillRect(hdc, &rc, hbrHighlightAndDblFrame);
            } else if (slot.count(XG_Pos(i, j)) > 0) {
                // ハイライト。
                ::FillRect(hdc, &rc, hbrHighlight);
            } else if (nMarked != -1 && !xg_bNumCroMode) {
                // 二重マスかつ非ナンクロ。
                ::FillRect(hdc, &rc, hbrMarked);
            } else {
                // その他のマス。
                ::FillRect(hdc, &rc, hbrWhite);
            }
        }
    }

    // 小さい文字のフォントを選択する。
    HGDIOBJ hFontOld = ::SelectObject(hdc, hFontSmall);

    // 二重マスを描画する。
    if (!xg_bNumCroMode) {
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                // 二重マスか？
                const int nMarked = XgGetMarked(i, j);
                if (nMarked == -1)
                    continue;

                // セルの座標をセットする。
                rc = {
                    xg_nMargin + j * nCellSize,
                    xg_nMargin + i * nCellSize,
                    xg_nMargin + (j + 1) * nCellSize - 1,
                    xg_nMargin + (i + 1) * nCellSize
                };

                XgDrawDoubleFrameCell(hdc, nMarked, rc, nCellSize, hThinPen, i, j);
            }
        }
    }

    // タテのカギの先頭マス。
    if (!xg_bNumCroMode) {
        const int size = static_cast<int>(xg_vVertInfo.size());
        for (int k = 0; k < size; k++) {
            const auto i = xg_vVertInfo[k].m_iRow, j = xg_vVertInfo[k].m_jCol;

            rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };
            ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

            XgDrawCellNumber(hdc, rc, i, j, xg_vVertInfo[k].m_number, slot);
        }
    }

    // ヨコのカギの先頭マス。
    if (!xg_bNumCroMode) {
        const int size = static_cast<int>(xg_vHorzInfo.size());
        for (int k = 0; k < size; k++) {
            const auto i = xg_vHorzInfo[k].m_iRow, j = xg_vHorzInfo[k].m_jCol;

            rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };
            ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

            XgDrawCellNumber(hdc, rc, i, j, xg_vHorzInfo[k].m_number, slot);
        }
    }

    // ナンクロモードの小さい数字。
    if (xg_bSolved && xg_bNumCroMode) {
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                const WCHAR ch = xg_solution.GetAt(i, j);
                if (ch == ZEN_BLACK || ch == ZEN_SPACE)
                    continue;

                rc = {
                    xg_nMargin + j * nCellSize,
                    xg_nMargin + i * nCellSize,
                    xg_nMargin + (j + 1) * nCellSize,
                    xg_nMargin + (i + 1) * nCellSize
                };
                ::OffsetRect(&rc, c_nThin, c_nThin * 2 / 3);

                XgDrawCellNumber(hdc, rc, i, j, xg_mapNumCro1[ch], slot);
            }
        }
    }

    // フォントの選択を解除する。
    ::SelectObject(hdc, hFontOld);

    // セルの文字を描画する。
    for (int i = 0; i < xg_nRows; i++) {
        for (int j = 0; j < xg_nCols; j++) {
            const auto ch = XgReplaceChar(xw.GetAt(i, j));
            if (ch == ZEN_BLACK)
                continue;

            // セルの座標をセットする。
            rc = {
                xg_nMargin + j * nCellSize,
                xg_nMargin + i * nCellSize,
                xg_nMargin + (j + 1) * nCellSize,
                xg_nMargin + (i + 1) * nCellSize
            };

            // マスの文字を描画する。
            XgDrawLetterCell(hdc, ch, rc, hFontNormal);

            // セルの枠を描画する。
            // Rectangle関数ではうまくいかないので直接線を描画する。
            HGDIOBJ hPen2Old = ::SelectObject(hdc, hThinPen);
            ::MoveToEx(hdc, rc.left, rc.top, nullptr);
            ::LineTo(hdc, rc.right, rc.top);
            ::LineTo(hdc, rc.right, rc.bottom);
            ::LineTo(hdc, rc.left, rc.bottom);
            ::LineTo(hdc, rc.left, rc.top);
            ::SelectObject(hdc, hPen2Old);
        }
    }

    // 破棄する。
    ::DeleteObject(hFontNormal);
    ::DeleteObject(hFontSmall);
    ::DeleteObject(hThinPen);
    ::DeleteObject(hbrBlack);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrMarked);
    ::DeleteObject(hbrHighlight);
    ::DeleteObject(hbrHighlightAndDblFrame);
    ::DeleteObject(hbrUnmatched);
}

// クロスワードを描画する。
void __fastcall XgDrawXWord(const XG_Board& xw, HDC hdc, const SIZE *psiz, DRAW_MODE mode)
{
    switch (xg_nViewMode)
    {
    case XG_VIEW_NORMAL:
    default:
        XgDrawXWord_NormalView(xw, hdc, psiz, mode);
        break;
    case XG_VIEW_SKELETON:
        XgDrawXWord_SkeletonView(xw, hdc, psiz, mode);
        break;
    }
    // ボックスを描画する。
    if (mode != DRAW_MODE_SCREEN)
        XgDrawBoxes(hdc, psiz);
}

// クロスワードのイメージを作成する。
HBITMAP __fastcall XgCreateXWordImage(const XG_Board& xw, const SIZE *psiz, bool bScreen)
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
    HBITMAP hbm = ::CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
    if (hbm == nullptr) {
        DeleteDC(hdc);
        return nullptr;
    }

    // 描画する。
    HGDIOBJ hbmOld = ::SelectObject(hdc, hbm);
    XgDrawXWord(xw, hdc, psiz, bScreen ? DRAW_MODE_SCREEN : DRAW_MODE_PRINT);
    ::SelectObject(hdc, hbmOld);

    // 互換DCを破棄する。
    ::DeleteDC(hdc);
    return hbm;
}

// CRPファイルを開く。
bool __fastcall XgDoLoadCrpFile(HWND hwnd, LPCWSTR pszFile)
{
    int i;
    std::vector<XGStringW> rows;
    WCHAR szName[32], szText[128];
    XG_Board xword;
    std::vector<XG_Hint> tate, yoko;
    bool bOK = false;

    const int nWidth = GetPrivateProfileIntW(L"Cross", L"Width", -1, pszFile);
    const int nHeight = GetPrivateProfileIntW(L"Cross", L"Height", -1, pszFile);

    static const WCHAR sz1[] = { ZEN_UNDERLINE, 0 };
    static const WCHAR sz2[] = { ZEN_SPACE, 0 };

    if (nWidth > 0 && nHeight > 0) {
        for (i = 0; i < nHeight; ++i) {
            StringCchPrintf(szName, _countof(szName), L"Line%d", i + 1);
            GetPrivateProfileStringW(L"Cross", szName, L"", szText, _countof(szText), pszFile);

            XGStringW str = szText;
            xg_str_trim(str);
            xg_str_replace_all(str, L",", L"");
            xg_str_replace_all(str, sz1, sz2);
            str = XgNormalizeString(str);

            if (static_cast<int>(str.size()) != nWidth)
                break;

            rows.push_back(str);
        }
        if (i == nHeight) {
            XGStringW str;

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
        xg_vecVertHints.clear();
        xg_vecHorzHints.clear();
        xg_bSolved = false;
        xg_bCheckingAnswer = false;

        if (xword.IsFulfilled()) {
            // 番号付けを行う。
            xword.DoNumberingNoCheck();

            const int nClueCount = GetPrivateProfileIntW(L"Clue", L"Count", 0, pszFile);
            if (nClueCount) {
                for (i = 0; i < nClueCount; ++i) {
                    StringCchPrintf(szName, _countof(szName), L"Clue%d", i + 1);
                    GetPrivateProfileStringW(L"Clue", szName, L"", szText, _countof(szText), pszFile);

                    XGStringW str = szText;
                    xg_str_trim(str);
                    if (str.empty())
                        break;
                    const auto icolon = str.find(L':');
                    if (icolon != str.npos) {
                        XGStringW word = str.substr(0, icolon);
                        XGStringW hint = str.substr(icolon + 1);
                        word = XgNormalizeString(word);
                        for (XG_PlaceInfo& item : xg_vVertInfo) {
                            if (item.m_word == word) {
                                tate.emplace_back(item.m_number, word, hint);
                                break;
                            }
                        }
                        for (XG_PlaceInfo& item : xg_vHorzInfo) {
                            if (item.m_word == word) {
                                yoko.emplace_back(item.m_number, word, hint);
                                break;
                            }
                        }
                    }
                }
            } else {
                for (i = 0; i < 256; ++i) {
                    StringCchPrintf(szName, _countof(szName), L"Down%d", i + 1);
                    GetPrivateProfileStringW(L"Clue", szName, L"", szText, _countof(szText), pszFile);

                    XGStringW str = szText;
                    xg_str_trim(str);
                    if (str.empty())
                        break;
                    if (str == L"{N/A}")
                        continue;
                    for (XG_PlaceInfo& item : xg_vVertInfo) {
                        if (item.m_number == i + 1) {
                            tate.emplace_back(item.m_number, item.m_word, str);
                            break;
                        }
                    }
                }

                for (i = 0; i < 256; ++i) {
                    StringCchPrintf(szName, _countof(szName), L"Across%d", i + 1);
                    GetPrivateProfileStringW(L"Clue", szName, L"", szText, _countof(szText), pszFile);

                    XGStringW str = szText;
                    xg_str_trim(str);
                    if (str.empty())
                        break;
                    if (str == L"{N/A}")
                        continue;
                    for (XG_PlaceInfo& item : xg_vHorzInfo) {
                        if (item.m_number == i + 1) {
                            yoko.emplace_back(item.m_number, item.m_word, str);
                            break;
                        }
                    }
                }
            }
        }

        // 不足分を追加。
        for (XG_PlaceInfo& item : xg_vHorzInfo) {
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
        for (XG_PlaceInfo& item : xg_vVertInfo) {
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
            xg_bCheckingAnswer = false;
            xg_bShowAnswer = false;
            XgClearNonBlocks();
            xg_vecVertHints = std::move(tate);
            xg_vecHorzHints = std::move(yoko);
        }

        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, nullptr);
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
        XGStringW strText;
        if (!XgReadTextFileAll(pszFile, strText, type == XG_FILETYPE_ECW))
            return false;

        if (XgSetString(hwnd, strText, type)) {
            // ファイルパスをセットする。
            WCHAR szFileName[MAX_PATH];
            ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, nullptr);
            if (type == XG_FILETYPE_ECW) {
                // ECWファイルの場合は保存できないのでファイル名をクリアする。
                szFileName[0] = 0;
            }
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
    if (fout == nullptr)
        return false;

    const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

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
            "Width=%d\n"
            "Height=%d\n", xg_nCols, xg_nRows);

        // マス。
        for (int i = 0; i < xg_nRows; ++i) {
            XGStringW row;
            for (int j = 0; j < xg_nCols; ++j) {
                if (row.size())
                    row += L",";
                const WCHAR ch = xw->GetAt(i, j);
                if (ch == ZEN_SPACE)
                    row += 0xFF3F; // '＿'
                else
                    row += ch;
            }
            fprintf(fout, "Line%d=%s\n", i + 1, XgUnicodeToAnsi(row).c_str());
        }

        // 二重マス。
        std::vector<std::vector<int> > marks;
        marks.resize(xg_nRows);
        for (auto& mark : marks) {
            mark.resize(xg_nCols);
        }

        XGStringW answer;
        if (xg_vMarks.size()) {
            for (size_t i = 0; i < xg_vMarks.size(); ++i) {
                answer += xw->GetAt(xg_vMarks[i].m_i, xg_vMarks[i].m_j);
                marks[xg_vMarks[i].m_i][xg_vMarks[i].m_j] = static_cast<int>(i) + 1;
            }
        }

        for (int i = 0; i < xg_nRows; ++i) {
            std::string row;
            for (int j = 0; j < xg_nCols; ++j) {
                if (row.size())
                    row += ",";
                row += std::to_string(marks[i][j]);
            }
            fprintf(fout, "MarkUpLine%d=%s\n", i + 1, row.c_str());
        }
        fprintf(fout,
            "\n"
            "[Property]\n"
            "Symmetry=None\n"
            "\n"
            "[Clue]\n");

        // ヒント。
        if (xg_vecVertHints.size() && xg_vecHorzHints.size()) {
            fprintf(fout, "Count=%d\n", static_cast<int>(xg_vecVertHints.size() + xg_vecHorzHints.size()));
            int iHint = 1;
            // タテのカギ。
            for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
                auto& tate_hint = xg_vecVertHints[i];
                fprintf(fout, "Clue%d=%s:%s\n", iHint++,
                    XgUnicodeToAnsi(tate_hint.m_strWord).c_str(),
                    XgUnicodeToAnsi(tate_hint.m_strHint).c_str());
            }
            // ヨコのカギ。
            for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                auto& yoko_hint = xg_vecHorzHints[i];
                fprintf(fout, "Clue%d=%s:%s\n", iHint++,
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
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, nullptr);
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
    XGStringW str, strTable, strMarks;

    // ファイルを作成する。
    hFile = ::CreateFileW(pszFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    try
    {
        json j0;
        // 作成者情報。
        j0["creator_info"] = XgUnicodeToUtf8(XgLoadStringDx1(IDS_APPINFO));
        // 行の数。
        j0["row_count"] = xg_nRows;
        // 列の数。
        j0["column_count"] = xg_nCols;
        // ルール。
        j0["rules"] = XgUnicodeToUtf8(XgGetRulesString(xg_nRules));
        // ルール（整数値）。
        j0["policy"] = xg_nRules;
        // 辞書名。
        j0["dictionary"] = XgUnicodeToUtf8(PathFindFileNameW(xg_dict_name.c_str()));
        // ビューモード。
        j0["view_mode"] = (int)xg_nViewMode;

        // 盤の切り替え。
        const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);
        j0["is_solved"] = !!xg_bSolved;

        // ナンクロモード。
        if (xg_bNumCroMode) {
            j0["is_numcro"] = true;
            for (auto& pair : xg_mapNumCro1) {
                auto ch = pair.first;
                auto number = pair.second;
                WCHAR szNum[64];
                StringCchPrintfW(szNum, _countof(szNum), L"%04d", number);
                const WCHAR szChar[2] = { ch, 0 };
                j0["num_cro"][XgUnicodeToUtf8(szNum)] = XgUnicodeToUtf8(szChar);
            }
        }

        // マス。
        for (int i = 0; i < xg_nRows; ++i) {
            XGStringW row;
            for (int j = 0; j < xg_nCols; ++j) {
                row += xw->GetAt(i, j);
            }
            j0["cell_data"].push_back(XgUnicodeToUtf8(row));
        }

        // ボックス。
        XgDoSaveBoxJson(j0);

        // 二重マス。
        if (xg_vMarks.size()) {
            j0["has_mark"] = true;

            XGStringW mark_word;
            for (auto& mark : xg_vMarks) {
                const WCHAR ch = xw->GetAt(mark.m_i, mark.m_j);
                mark_word += ch;
            }

            j0["mark_word"] = XgUnicodeToUtf8(mark_word);

            str += L"\t\"marks\": [\r\n";
            for (size_t i = 0; i < xg_vMarks.size(); ++i) {
                json mark;
                mark.push_back(xg_vMarks[i].m_i + 1);
                mark.push_back(xg_vMarks[i].m_j + 1);
                const WCHAR sz[2] = { mark_word[i] , 0 };
                mark.push_back(XgUnicodeToUtf8(sz));
                j0["marks"].push_back(mark);
            }
        } else {
            j0["has_mark"] = false;
        }

        // ヒント。
        if (xg_vecVertHints.size() && xg_vecHorzHints.size()) {
            j0["has_hints"] = true;

            json hints;

            // タテのカギ。
            json v;
            for (auto& tate_hint : xg_vecVertHints) {
                json hint;
                hint.push_back(tate_hint.m_number);
                hint.push_back(XgUnicodeToUtf8(tate_hint.m_strWord));
                hint.push_back(XgUnicodeToUtf8(tate_hint.m_strHint));
                v.push_back(hint);
            }
            hints["v"] = v;

            // ヨコのカギ。
            json h;
            for (auto& yoko_hint : xg_vecHorzHints) {
                json hint;
                hint.push_back(yoko_hint.m_number);
                hint.push_back(XgUnicodeToUtf8(yoko_hint.m_strWord));
                hint.push_back(XgUnicodeToUtf8(yoko_hint.m_strHint));
                h.push_back(hint);
            }
            hints["h"] = h;

            j0["hints"] = hints;
        } else {
            j0["has_hints"] = false;
        }

        // ヘッダー。
        xg_str_trim(xg_strHeader);
        j0["header"] = XgUnicodeToUtf8(xg_strHeader);

        // 備考欄。
        xg_str_trim(xg_strNotes);
        LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
        if (xg_strNotes.find(psz) == 0) {
            xg_strNotes = xg_strNotes.substr(XGStringW(psz).size());
        }
        j0["notes"] = XgUnicodeToUtf8(xg_strNotes);

        // テーマ。
        j0["theme"] = XgUnicodeToUtf8(xg_strTheme);

        // UTF-8へ変換する。
        std::string utf8 = j0.dump(1, '\t');
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
        auto size = static_cast<DWORD>(utf8.size() * sizeof(CHAR));
        if (::WriteFile(hFile, utf8.data(), size, &size, nullptr)) {
            ::CloseHandle(hFile);

            // ファイルパスをセットする。
            WCHAR szFileName[MAX_PATH];
            ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, nullptr);
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
    XGStringW str, strTable, strMarks, hints;

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
        str += L"\r\n";       // 改行。
        str += XgLoadStringDx1(IDS_HEADERSEP1); // ヘッダー分離線。
        str += XgLoadStringDx1(IDS_APPINFO); // アプリ情報。
        str += L"\r\n";       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
        str += L"\r\n";       // 改行。
        str += hints;               // ヒント。
    } else {
        // ヒントなし。
        board.GetString(strTable);
        str += xg_strHeader;        // ヘッダー文字列。
        str += L"\r\n";       // 改行。
        str += XgLoadStringDx1(IDS_HEADERSEP1); // ヘッダー分離線。
        str += XgLoadStringDx1(IDS_APPINFO); // アプリ情報。
        str += L"\r\n";       // 改行。
        str += strMarks;            // マーク。
        str += strTable;            // 本体。
    }
    str += XgLoadStringDx1(IDS_HEADERSEP2);     // フッター分離線。

    // 備考欄。
    LPCWSTR psz = XgLoadStringDx1(IDS_BELOWISNOTES);
    str += psz;
    str += L"\r\n";
    if (xg_strNotes.find(psz) == 0) {
        xg_strNotes = xg_strNotes.substr(XGStringW(psz).size());
    }
    xg_str_trim(xg_strNotes);
    str += xg_strNotes;
    str += L"\r\n";

    // ファイルに書き込んで、ファイルを閉じる。
    DWORD size = 2;
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
XGStringW XgNormalizeStringEx(const XGStringW& str, BOOL bUppercase, BOOL bKatakana) {
    XGStringW ret;
    for (auto ch : str) {
        WCHAR newch = 0;
        // 小さな字を大きな字にする。
        auto it = xg_small2large.find(ch);
        if (it != xg_small2large.end())
            ch = it->second;
        if (XgIsCharKanaW(ch) || XgIsCharKanjiW(ch) || ch == ZEN_PROLONG || XgIsCharHangulW(ch) ||
            ch == ZEN_UNDERLINE)
        {
            // カナか漢字かハングルなら全角文字。
            if (bKatakana) {
                LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch, 1, &newch, 1);
            } else {
                LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch, 1, &newch, 1);
            }
        } else {
            // それ以外は半角文字。
            if (bUppercase) {
                LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch, 1, &newch, 1);
            } else {
                LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch, 1, &newch, 1);
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
    if (fout == nullptr)
        return false;

    const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

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
                const WCHAR ch = xw->GetAt(i, j);
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
            XGStringW row;
            for (int j = 0; j < xg_nCols; ++j) {
                const WCHAR ch = xw->GetAt(i, j);
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
            if (bAsian)
                row = XgNormalizeString(row);
            else
                row = XgNormalizeStringEx(row);
            fprintf(fout, "%s\n", XgUnicodeToUtf8(row).c_str());
        }
        fprintf(fout, "\n\n");

        // ヒント。
        if (xg_bSolved && xg_vecVertHints.size() && xg_vecHorzHints.size()) {
            char line[512];
            std::string strACROSS, strDOWN;
            // タテのカギ。
            for (auto& tate_hint : xg_vecVertHints) {
                auto word = XgNormalizeStringEx(tate_hint.m_strWord);
                StringCchPrintfA(line, _countof(line), "D%d. %s ~ %s\n",
                                 tate_hint.m_number,
                                 XgUnicodeToUtf8(tate_hint.m_strHint).c_str(),
                                 XgUnicodeToUtf8(word).c_str());
                strDOWN += line;
            }
            // ヨコのカギ。
            for (auto& yoko_hint : xg_vecHorzHints) {
                auto word = XgNormalizeStringEx(yoko_hint.m_strWord);
                StringCchPrintfA(line, _countof(line), "A%d. %s ~ %s\n",
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
            XGStringW strMarks;
            XgGetStringOfMarks2(strMarks);
            fprintf(fout, "%s\n\n", XgUnicodeToUtf8(strMarks).c_str());
        }

        // ビューモード。
        fprintf(fout, "ViewMode: %d\n\n", xg_nViewMode);

        // ナンクロモード。
        if (xg_bNumCroMode) {
            std::map<int, WCHAR> mapping;
            for (auto& pair : xg_mapNumCro2) {
                mapping[pair.first] = pair.second;
            }
            for (auto& pair : mapping) {
                const WCHAR sz[2] = { pair.second, 0 };
                fprintf(fout, "NUMCRO-%04d: %s\n", pair.first, XgUnicodeToUtf8(sz).c_str());
            }
            fprintf(fout, "\n");
        }

        // 備考欄。
        if (xg_strNotes.size()) {
            auto str = xg_strNotes;
            xg_str_replace_all(str, L"\r\n", L"\n");
            xg_str_trim(str);
            if (str.size()) {
                fprintf(fout, "%s\n", XgUnicodeToUtf8(str).c_str());
            }
        }

        // ルール。
        fprintf(fout, "\nPolicy: %d\n", xg_nRules);

        // ボックス。
        XgWriteXdBoxes(fout);

        fclose(fout);

        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, nullptr);
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
    bool ret = false;
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
        break;
    }

    if (ret) {
        // ファイルパスをセットする。
        WCHAR szFileName[MAX_PATH];
        ::GetFullPathNameW(pszFile, MAX_PATH, szFileName, nullptr);
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
    } else if (lstrcmpiW(pchDotExt, L".ecw") == 0) {
        return XgDoLoadFileType(hwnd, pszFile, XG_FILETYPE_ECW);
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

// 問題を画像ファイルとして保存する。
void __fastcall XgSaveProbAsImage(HWND hwnd)
{
    // 「問題を画像ファイルとして保存」ダイアログを表示。
    WCHAR szFileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_IMGFILTER));
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = _countof(szFileName);
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
                if (!SaveBitmapToFile(szFileName, hbm)) {
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
                XgSetSizeOfEMF(hdc, &siz);
                XgDrawXWord(xg_xword, hdc, &siz, DRAW_MODE_EMF);
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
    if (!xg_bSolved) {
        ::MessageBeep(0xFFFFFFFF);
        return;
    }

    // 「解答を画像ファイルとして保存」ダイアログを表示。
    WCHAR szFileName[MAX_PATH] = L"";
    OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
    ofn.lpstrFilter = XgMakeFilterString(XgLoadStringDx2(IDS_IMGFILTER));
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = _countof(szFileName);
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
                if (!SaveBitmapToFile(szFileName, hbm)) {
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
                XgSetSizeOfEMF(hdc, &siz);
                XgDrawXWord(xg_solution, hdc, &siz, DRAW_MODE_EMF);
                ::CloseEnhMetaFile(hdc);
            } else {
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTSAVE2), nullptr, MB_ICONERROR);
            }
            ::ReleaseDC(hwnd, hdcRef);
        }
    }
}

// EMFの寸法をセットする。
void XgSetSizeOfEMF(HDC hdcEMF, const SIZE *psiz) noexcept
{
    //SetMapMode(hdcEMF, MM_ISOTROPIC);
    //SetWindowExtEx(hdcEMF, psiz->cx, psiz->cy, nullptr);
    //SetViewportExtEx(hdcEMF, psiz->cx, psiz->cy, nullptr);
}

// クロスワードの文字列を取得する。
void __fastcall XG_Board::GetString(XGStringW& str) const
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
bool __fastcall XG_Board::SetString(const XGStringW& strToBeSet)
{
    std::vector<WCHAR> v;
    XGStringW str(strToBeSet);

    // ヘッダーを取得する。
    xg_strHeader.clear();
    XGStringW strHeaderSep = XgLoadStringDx1(IDS_HEADERSEP1);
    const auto i0 = str.find(strHeaderSep);
    if (i0 != XGStringW::npos) {
        xg_strHeader = str.substr(0, i0);
        str = str.substr(i0 + strHeaderSep.size());
    }
    xg_str_trim(xg_strHeader);

    // 初期化する。
    const int size = static_cast<int>(str.size());
    int nRows = 0, nCols = 0;

    // 左上の角があるか？
    int i1 = 0;
    for (; i1 < size; i1++) {
        if (str[i1] == ZEN_ULEFT)
            break;
    }
    if (i1 == size) {
        // 見つからなかった。失敗。
        return false;
    }

    // 文字列を読み込む。
    bool bFoundLastCorner = false;
    for (; i1 < size; i1++) {
        if (str[i1] == ZEN_VLINE) {
            // 左の境界線が見つかった。
            i1++;    // 次の文字へ。

            // 横線以外の文字を格納する。
            while (i1 < size && str[i1] != ZEN_VLINE) {
                v.emplace_back(str[i1]);
                i1++;    // 次の文字へ。
            }

            // 文字列の長さを超えたら、中断する。
            if (i1 >= size)
                break;

            // 右の境界線が見つかった。列数が未格納なら、格納する。
            if (nCols == 0)
                nCols = static_cast<int>(v.size());

            nRows++;    // 次の行へ。
            i1++;    // 次の文字へ。
        } else if (str[i1] == ZEN_LRIGHT) {
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
    xg_bCheckingAnswer = false;
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
    xg_vVertInfo.clear();
    xg_vHorzInfo.clear();

    // 成功。
    return true;
}

//////////////////////////////////////////////////////////////////////////////

// 黒マスが線対称か？
bool XG_Board::IsLineSymmetry() const noexcept
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
bool XG_Board::IsPointSymmetry() const noexcept
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
bool XG_Board::IsLineSymmetryV() const noexcept
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
bool XG_Board::IsLineSymmetryH() const noexcept
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
void XG_Board::Mirror() noexcept
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
bool XG_Board::ThreeDiagonals() const noexcept
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
bool XG_Board::FourDiagonals() const noexcept
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
int xg_nMaxWordLen = 4;

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
    const int iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);
    // 終了条件。
    if (iRow == nRows && jCol == 0) {
        ::EnterCriticalSection(&xg_csLock);
        if (!xg_bCancelled && !xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_csLock);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの左側を見る。
    int jLeft;
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
    int iTop;
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
    const int iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);

    // 終了条件。
    if (iRow == nRows && jCol == 0) {
        ::EnterCriticalSection(&xg_csLock);
        if (!xg_bCancelled && !xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_csLock);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの左側を見る。
    int jLeft;
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
    int iTop;
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

    const int nRows = xg_nRows, nCols = xg_nCols;
    const int nHalfRows = nRows / 2;
    const int iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);

    // 終了条件。
    if (iRow == 0 && jCol >= nCols) {
        ::EnterCriticalSection(&xg_csLock);
        if (!xg_bCancelled && !xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_csLock);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの上側を見る。
    int iTop;
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
    int jLeft;
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
        XGStringW strPattern = xword.GetPatternV(XG_Pos(iRow, jCol));
        if (static_cast<int>(strPattern.size()) > xg_nMaxWordLen) {
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

    const int nRows = xg_nRows, nCols = xg_nCols;
    const int nHalfCols = nCols / 2;
    const int iRow = LOWORD(iRowjCol), jCol = HIWORD(iRowjCol);

    // 終了条件。
    if (iRow >= nRows && jCol == 0) {
        ::EnterCriticalSection(&xg_csLock);
        if (!xg_bCancelled && !xg_bBlacksGenerated) {
            xg_bBlacksGenerated = true;
            xg_xword = xword;
        }
        ::LeaveCriticalSection(&xg_csLock);
        return xg_bBlacksGenerated || xg_bCancelled;
    }

    // 終了条件。
    if (xg_bBlacksGenerated || xg_bCancelled)
        return true;

    // マスの左側を見る。
    int jLeft;
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
    int iTop;
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
        XGStringW strPattern = xword.GetPatternH(XG_Pos(iRow, jCol));
        if (static_cast<int>(strPattern.size()) > xg_nMaxWordLen) {
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
#ifndef NO_RANDOM
    srand(static_cast<DWORD>(::GetTickCount64()) ^ ::GetCurrentThreadId());
#else
    srand(xg_random_seed++);
#endif

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
#ifndef NO_RANDOM
    srand(static_cast<DWORD>(::GetTickCount64()) ^ ::GetCurrentThreadId());
#else
    srand(xg_random_seed++);
#endif

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
static unsigned __stdcall XgGenerateBlacksPointSym(void *param)
{
#ifndef NO_RANDOM
    srand(static_cast<DWORD>(::GetTickCount64()) ^ ::GetCurrentThreadId());
#else
    srand(xg_random_seed++);
#endif
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
static unsigned __stdcall XgGenerateBlacksLineSymV(void *param)
{
#ifndef NO_RANDOM
    srand(static_cast<DWORD>(::GetTickCount64()) ^ ::GetCurrentThreadId());
#else
    srand(xg_random_seed++);
#endif
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
static unsigned __stdcall XgGenerateBlacksLineSymH(void *param)
{
#ifndef NO_RANDOM
    srand(static_cast<DWORD>(::GetTickCount64()) ^ ::GetCurrentThreadId());
#else
    srand(xg_random_seed++);
#endif
    xg_solution.clear();
    XG_Board xword;
    do {
        if (xg_bBlacksGenerated || xg_bCancelled)
            break;
        xword.clear();
    } while (!XgGenerateBlacksLineSymHRecurse(xword, 0));
    return 1;
}

void __fastcall XgStartGenerateBlacks(void) noexcept
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
            auto& hThread = xg_ahThreads[i];
            auto& info = xg_aThreadInfo[i];
            hThread = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksPointSym, &info, 0, &info.m_threadid));
            assert(hThread != nullptr);
        }
#endif
    } else if (xg_nRules & RULE_LINESYMMETRYV) {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacksLineSymV(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            auto& hThread = xg_ahThreads[i];
            auto& info = xg_aThreadInfo[i];
            hThread = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksLineSymV, &info, 0, &info.m_threadid));
            assert(hThread != nullptr);
        }
#endif
    } else if (xg_nRules & RULE_LINESYMMETRYH) {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacksLineSymH(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            auto& hThread = xg_ahThreads[i];
            auto& info = xg_aThreadInfo[i];
            hThread = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacksLineSymH, &info, 0, &info.m_threadid));
            assert(hThread != nullptr);
        }
#endif
    } else {
#ifdef SINGLE_THREAD_MODE
        XgGenerateBlacks(&xg_aThreadInfo[0]);
#else
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            auto& hThread = xg_ahThreads[i];
            auto& info = xg_aThreadInfo[i];
            hThread = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, XgGenerateBlacks, &info, 0, &info.m_threadid));
            assert(hThread != nullptr);
        }
#endif
    }
}

// クロスワードで使う文字に変換する。
XGStringW __fastcall XgNormalizeString(const XGStringW& text) {
    WCHAR szText[512];
    LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
        text.c_str(), -1, szText, _countof(szText));
    XGStringW ret(szText);
    for (auto& ch : ret) {
        // 小さな字を大きな字にする。
        auto it = xg_small2large.find(ch);
        if (it != xg_small2large.end())
            ch = it->second;
    }
    return ret;
}

// タテ向きにパターンを読み取る。
XGStringW __fastcall XG_Board::GetPatternV(const XG_Pos& pos) const
{
    XGStringW pattern;
    if (GetAt(pos.m_i, pos.m_j) == ZEN_BLACK)
        return pattern;

    int lo = pos.m_i, hi = pos.m_i;
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
XGStringW __fastcall XG_Board::GetPatternH(const XG_Pos& pos) const
{
    XGStringW pattern;
    if (GetAt(pos.m_i, pos.m_j) == ZEN_BLACK)
        return pattern;

    int lo = pos.m_j, hi = pos.m_j;
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
void __fastcall XgCloseThreads(void) noexcept
{
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        ::CloseHandle(xg_ahThreads[i]);
        xg_ahThreads[i] = nullptr;
    }
}

// スレッドを待つ。
void __fastcall XgWaitForThreads(void) noexcept
{
    ::WaitForMultipleObjects(xg_dwThreadCount, xg_ahThreads.data(), true, 1000);
}

// スレッドが終了したか？
bool __fastcall XgIsAnyThreadTerminated(void) noexcept
{
    DWORD dwExitCode;
    for (DWORD i = 0; i < xg_dwThreadCount; i++) {
        ::GetExitCodeThread(xg_ahThreads[i], &dwExitCode);
        if (dwExitCode != STILL_ACTIVE)
            return true;
    }
    return false;
}

// パターンの統計情報を表示。
void XgShowPatInfo(HWND hwndInfo)
{
    XGStringW text; // この関数では、この変数にテキストを追加していく。

    if (xg_bSolved) {
        std::map<DWORD, DWORD> len2num;
        std::vector<XG_WordData> dict = XgCreateMiniDict();
        for (auto& data : dict) {
            len2num[(DWORD)data.m_word.size()] += 1;
        }
        std::vector<std::pair<DWORD, DWORD>> pairs;
        for (auto& pair : len2num) {
            pairs.push_back(pair);
        }
        std::sort(pairs.begin(), pairs.end(), [](auto& a, auto& b){
            return a.second > b.second;
        });
        // 使用単語数。
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_WORDCOUNT);
        text += L" ]] : ";
        text += to_XGStringW(dict.size());
        text += L"\r\n";
        // 単語の長さの頻度分布。
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_WORDLENDIST);
        text += L" ]]";
        text += L"\r\n";
        for (auto& pair : pairs) {
            text += to_XGStringW(pair.first);
            text += L" : ";
            text += to_XGStringW(pair.second);
            text += L"\r\n";
        }
    }

    // ヒントの長さ。
    if (xg_bSolved) {
        std::vector<XG_WordData> dict = XgCreateMiniDict();
        size_t count = dict.size();
        size_t sum = 0, min = 999999, max = 0;
        for (auto& data : dict) {
            auto len = data.m_hint.size();
            if (min > len)
                min = len;
            if (max < len)
                max = len;
            sum += data.m_hint.size();
        }
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_HINTLENAVG); // 平均値。
        text += L" ]]";
        text += L": ";
        text += to_XGStringW(sum / (double)count);
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_HINTLENMIN); // 最小値。
        text += L" ]]";
        text += L": ";
        text += to_XGStringW(min);
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_HINTLENMAX); // 最大値。
        text += L" ]]";
        text += L": ";
        text += to_XGStringW(max);
        text += L"\r\n";
    }

    std::vector<XG_WordData> dict;
    dict.insert(dict.end(), xg_dict_1.begin(), xg_dict_1.end());
    dict.insert(dict.end(), xg_dict_2.begin(), xg_dict_2.end());
    std::sort(dict.begin(), dict.end(), xg_word_less());
    dict.erase(std::unique(dict.begin(), dict.end(), [](auto& a, auto& b) {
        return a.m_word == b.m_word;
    }), dict.end());

    {
        size_t count = dict.size();
        size_t sum = 0, min = 999999, max = 0;
        for (auto& data : dict) {
            auto len = data.m_word.size();
            if (min > len)
                min = len;
            if (max < len)
                max = len;
            sum += data.m_word.size();
        }
        // 辞書中の単語の個数。
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTWORDCOUNT);
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(count);
        // 辞書中の単語の長さ。
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTWORDLENAVG); // 平均値。
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(sum / (double)count);
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTWORDLENMIN); // 最小値。
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(min);
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTWORDLENMAX); // 最大値。
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(max);
        text += L"\r\n";
    }

    // 辞書中のヒントの長さ。
    {
        size_t count = dict.size();
        size_t sum = 0, min = 999999, max = 0;
        for (auto& data : dict) {
            auto len = data.m_hint.size();
            if (min > len)
                min = len;
            if (max < len)
                max = len;
            sum += data.m_hint.size();
        }
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTHINTLENAVG); // 平均値。
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(sum / (double)count);
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTHINTLENMIN); // 最小値。
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(min);
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_DICTHINTLENMAX); // 最大値。
        text += L" ]]";
        text += L" : ";
        text += to_XGStringW(max);
        text += L"\r\n";
    }

    // 使用文字の頻度分布。
    {
        std::map<WCHAR, DWORD> ch2num;
        {
            const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);
            for (int i = 0; i < xg_nRows; i++) {
                for (int j = 0; j < xg_nCols; j++) {
                    auto ch = xw->GetAt(i, j);
                    ch2num[ch] += 1;
                }
            }
        }
        std::vector<std::pair<WCHAR, DWORD>> pairs;
        for (auto& pair : ch2num) {
            pairs.push_back(pair);
        }
        std::sort(pairs.begin(), pairs.end(), [](auto& a, auto& b){
            return a.second > b.second;
        });
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_CHARUSAGEDIST);
        text += L" ]]";
        text += L"\r\n";
        for (auto& pair : pairs) {
            text += (WCHAR)0x3010; //【
            text += pair.first;
            text += (WCHAR)0x3011; // 】
            text += L" : ";
            text += to_XGStringW(pair.second);
            text += L"\r\n";
        }
    }

    {
        // パターンを読み込む。
        patterns_t patterns;
        WCHAR szPath[MAX_PATH];
        XgFindLocalFile(szPath, _countof(szPath), L"PAT.txt");
        if (!XgLoadPatterns(szPath, patterns)) {
            ::SetWindowTextW(hwndInfo, NULL);
            return;
        }

        // ソートして一意化する。
        XgSortAndUniquePatterns(patterns, TRUE);

        std::map<DWORD, DWORD> size2num;
        for (const auto& pat : patterns) {
            size2num[MAKELONG(pat.num_rows, pat.num_columns)] += 1;
        }
        // パターンの個数。
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_PATCOUNT);
        text += L" ]] : ";
        text += to_XGStringW(patterns.size());
        text += L"\r\n";
        // パターンの頻度分布。
        text += L"\r\n";
        text += L"[[ ";
        text += XgLoadStringDx1(IDS_PATSIZEDIST);
        text += L" ]]";
        text += L"\r\n";
        for (auto& pair : size2num) {
            size_t num_columns = HIWORD(pair.first);
            size_t num_rows = LOWORD(pair.first);
            text += to_XGStringW(num_columns);
            text += L" x ";
            text += to_XGStringW(num_rows);
            text += L" : ";
            text += to_XGStringW(pair.second);
            text += L"\r\n";
        }
    }

    ::SetWindowTextW(hwndInfo, text.c_str());
}
