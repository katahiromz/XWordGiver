//////////////////////////////////////////////////////////////////////////////
// Marks.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////
// global variables

// マーク。
std::vector<XG_Pos>      xg_vMarks;

// 二重マス単語候補。
std::vector<std::wstring>     xg_vMarkedCands;

// 二重マス単語。
std::wstring xg_strMarked;

// 選択中の二重マス単語の候補のインデックス。
int xg_iMarkedCand = -1;

//////////////////////////////////////////////////////////////////////////////

// マーク文字列を取得する。
void __fastcall XgGetStringOfMarks(std::wstring& str)
{
    WCHAR sz[64];
    str.clear();
    str += XgLoadStringDx1(IDS_HLINE);
    str += xg_pszNewLine;
    for (const auto& mark : xg_vMarks) {
        if (xg_bSolved)
            StringCchPrintf(sz, _countof(sz), L"(%d, %d)%c\r\n",
                mark.m_i + 1, mark.m_j + 1,
                xg_solution.GetAt(mark.m_i, mark.m_j));
        else
            StringCchPrintf(sz, _countof(sz), L"(%d, %d)%c\r\n",
                mark.m_i + 1, mark.m_j + 1,
                xg_xword.GetAt(mark.m_i, mark.m_j));
        str += sz;
    }
}

// マーク文字列を取得する2。
void __fastcall XgGetStringOfMarks2(std::wstring& str)
{
    WCHAR sz[64];
    str.clear();
    int i = 0;
    auto xg = (xg_bSolved ? &xg_solution : &xg_xword);
    for (const auto& mark : xg_vMarks) {
        const WCHAR szLetter[2] = { xg->GetAt(mark.m_i, mark.m_j), 0 };
        auto letter = XgNormalizeStringEx(szLetter);
        StringCchPrintf(sz, _countof(sz), L"MARK%d. (%u, %u): %s\n",
                       i + 1, mark.m_j + 1, mark.m_i + 1, letter.c_str());
        str += sz;
        ++i;
    }
}

// マークされているか（二重マス）？
int __fastcall XgGetMarked(const std::vector<XG_Pos>& vMarks, const XG_Pos& pos) noexcept
{
    const int size = static_cast<int>(vMarks.size());
    for (int i = 0; i < size; i++) {
        if (vMarks[i] == pos)
            return i;
    }
    return -1;
}

// マークされているか（二重マス）？
int __fastcall XgGetMarked(const std::vector<XG_Pos>& vMarks, int i, int j) noexcept
{
    return XgGetMarked(vMarks, XG_Pos(i, j));
}

// マークされているか（二重マス）？
int __fastcall XgGetMarked(int i, int j) noexcept
{
    return XgGetMarked(xg_vMarks, XG_Pos(i, j));
}

// 二重マスが更新された。
void __fastcall XgMarkUpdate(void)
{
    WCHAR sz[64];
    std::wstring str;

    // すでに解があるかどうかによって切り替え。
    const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);

    // ファイル名があるか？
    if (xg_strFileName.size() || PathFileExistsW(xg_strFileName.c_str())) {
        // マークされているか？ 答えを表示するか？
        LPWSTR pchFileTitle = PathFindFileNameW(xg_strFileName.c_str());
        if (XgGetMarkWord(xw, str) && xg_bShowAnswer) {
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_APPTITLE2), str.data(), pchFileTitle);
            ::SetWindowTextW(xg_hMainWnd, sz);
        } else {
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_APPTITLE), pchFileTitle);
            ::SetWindowTextW(xg_hMainWnd, sz);
        }
    } else {
        // マークされているか？ 答えを表示するか？
        if (XgGetMarkWord(xw, str) && xg_bShowAnswer) {
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_APPINFO2), str.data());
            ::SetWindowTextW(xg_hMainWnd, sz);
        } else {
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_APPINFO));
            ::SetWindowTextW(xg_hMainWnd, sz);
        }
    }
}

// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(const XG_Pos& pos)
{
    for (const auto& markpos : xg_vMarks) {
        if (markpos == pos)
            return;
    }
    xg_vMarks.emplace_back(pos);

    // マークの更新を通知する。
    XgMarkUpdate();
}

// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(int i, int j)
{
    XgSetMark(XG_Pos(i, j));
}

// 指定のマスのマーク（二重マス）を解除する。
void __fastcall XgDeleteMark(int i, int j)
{
    // (i, j)がマークされていなければ無視。
    const int nMarked = XgGetMarked(i, j);
    if (nMarked == -1)
        return;

    // 二重マスを消す。
    xg_vMarks.erase(xg_vMarks.begin() + nMarked);

    // マークの更新を通知する。
    XgMarkUpdate();
}

// マーク文字列を設定する。
void __fastcall XgSetStringOfMarks(LPCWSTR psz)
{
    // 初期化する。
    xg_vMarkedCands.clear();
    xg_vMarks.clear();

    // 区切り線があるか？
    LPCWSTR pch = XgLoadStringDx1(IDS_HLINE);
    psz = wcsstr(psz, pch);
    if (psz == nullptr) {
        // 区切り線がなければ二重マスの情報はない。
        return;
    }
    psz += wcslen(pch);

    // マーク文字列を解析する。
    int count = 0;
    while (*psz != L'\0' && *psz != ZEN_ULEFT) {
        // 1000文字以上はありえない。
        if (count++ > 1000)
            break;

        // 読み飛ばし。
        while (*psz == L' ' || *psz == L'\r' || *psz == L'\n' || *psz == L'(')
            psz++;
        if (*psz == L'\0' || *psz == ZEN_ULEFT)
            break;

        // 数字がなければ終了。
        if (!(L'0' <= *psz && *psz <= L'9'))
            break;

        // 行のインデックスを読み込む。
        const int i = _wtoi(psz) - 1;
        while (L'0' <= *psz && *psz <= L'9')
            psz++;
        if (*psz == L'\0' || *psz == ZEN_ULEFT)
            break;

        // 読み飛ばし。
        while (*psz == L' ' || *psz == L',')
            psz++;

        // 数字がなければ終了。
        if (!(L'0' <= *psz && *psz <= L'9'))
            break;

        // 列のインデックスを読み込む。
        const int j = _wtoi(psz) - 1;

        // 読み飛ばし。
        while ((L'0' <= *psz && *psz <= L'9') || *psz == L')')
            psz++;

        while (XgIsCharKanaW(*psz) || XgIsCharHankakuAlphaW(*psz) ||
               XgIsCharZenkakuAlphaW(*psz) || XgIsCharKanjiW(*psz) ||
               XgIsCharHangulW(*psz) || *psz == ZEN_PROLONG)
        {
            psz++;
        }

        // 終わりであれば終了。
        if (*psz == L'\0' || *psz == ZEN_ULEFT)
            break;

        // マークを設定する。
        XgSetMark(i, j);
    }
}

// 二重マス単語を取得する。
bool __fastcall XgGetMarkWord(const XG_Board *xw, std::wstring& str)
{
    // 初期化する。
    str.clear();

    // 二重マス単語があるか？
    if (xg_vMarks.size()) {
        bool bExists = true;

        // 二重マス単語を構築する。
        for (const auto& mark : xg_vMarks) {
            const WCHAR ch = xw->GetAt(mark.m_i, mark.m_j);
            // 空白マスか黒マスであれば、二重マス単語はない。
            if (ch == ZEN_SPACE || ch == ZEN_BLACK)
                bExists = false;
            str += ch;
        }
        return bExists;     // 存在するか？
    }
    return false;   // 失敗。
}

// 二重マス単語候補を取得する。
bool __fastcall XgGetMarkedCandidates(void)
{
    // 初期化する。
    xg_vMarkedCands.clear();

    // 解がなければ失敗。
    if (!xg_bSolved)
        return false;

    // マスの文字をマルチセットに変換。
    std::unordered_multiset<WCHAR> msCells;
    xg_vec_to_multiset(msCells, xg_solution.m_vCells);

    // xg_dict_1に登録されている単語について繰り返す。
    for (const auto& data : xg_dict_1) {
        // 単語を取り出す。2文字以下は無視。
        const std::wstring& word = data.m_word;
        if (word.size() <= 2)
            continue;

        // 単語をマルチセットへ変換。
        std::unordered_multiset<WCHAR> ms;
        xg_str_to_multiset(ms, word);

        // 部分マルチセットになっているか？
        if (xg_submultiseteq(ms, msCells)) {
            // 配置を調べる。
            std::vector<XG_Pos> vPos;
            for (auto wch : word) {
                for (int i = 0; i < xg_nRows; ++i) {
                    for (int j = 0; j < xg_nCols; ++j) {
                        if (wch == xg_solution.GetAt(i, j) &&
                            XgGetMarked(vPos, i, j) == -1)
                        {
                            vPos.emplace_back(i, j);
                            goto break2;
                        }
                    }
                }
break2:;
            }

            // 隣り合う配置があれば失敗。
            const int size = static_cast<int>(vPos.size());
            for (int i = 0; i < size - 1; i++) {
                for (int j = i + 1; j < size; j++) {
                    if (vPos[i].m_i == vPos[j].m_i) {
                        if (vPos[i].m_j + 1 == vPos[j].m_j ||
                            vPos[i].m_j == vPos[j].m_j + 1)
                        {
                            goto failed;
                        }
                    }
                    if (vPos[i].m_j == vPos[j].m_j)
                    {
                        if (vPos[i].m_i + 1 == vPos[j].m_i ||
                            vPos[i].m_i == vPos[j].m_i + 1)
                        {
                            goto failed;
                        }
                    }
                }
            }
            xg_vMarkedCands.emplace_back(word);
failed:;
        }
    }

    // xg_dict_2に登録されている単語について繰り返す。
    for (const auto& data : xg_dict_2) {
        // 単語を取り出す。2文字以下は無視。
        const std::wstring& word = data.m_word;
        if (word.size() <= 2)
            continue;

        // 単語をマルチセットへ変換。
        std::unordered_multiset<WCHAR> ms;
        xg_str_to_multiset(ms, word);

        // 部分マルチセットになっているか？
        if (xg_submultiseteq(ms, msCells)) {
            // 配置を調べる。
            std::vector<XG_Pos> vPos;
            for (auto wch : word) {
                for (int i = 0; i < xg_nRows; ++i) {
                    for (int j = 0; j < xg_nCols; ++j) {
                        if (wch == xg_solution.GetAt(i, j) &&
                            XgGetMarked(vPos, i, j) == -1)
                        {
                            vPos.emplace_back(i, j);
                            goto break2_2;
                        }
                    }
                }
break2_2:;
            }

            // 隣り合う配置があれば失敗。
            const int size = static_cast<int>(vPos.size());
            for (int i = 0; i < size - 1; i++) {
                for (int j = i + 1; j < size; j++) {
                    if (vPos[i].m_i == vPos[j].m_i) {
                        if (vPos[i].m_j + 1 == vPos[j].m_j ||
                            vPos[i].m_j == vPos[j].m_j + 1)
                        {
                            goto failed_2;
                        }
                    }
                    if (vPos[i].m_j == vPos[j].m_j)
                    {
                        if (vPos[i].m_i + 1 == vPos[j].m_i ||
                            vPos[i].m_i == vPos[j].m_i + 1)
                        {
                            goto failed_2;
                        }
                    }
                }
            }
            xg_vMarkedCands.emplace_back(word);
failed_2:;
        }
    }

    // ソート・一意化する。
    std::sort(xg_vMarkedCands.begin(), xg_vMarkedCands.end(),
        [](const std::wstring& x, const std::wstring& y) noexcept {
            if (x.size() > y.size())
                return true;
            if (x.size() < y.size())
                return false;
            return (x < y);
        }
    );
    xg_vMarkedCands.erase(
        std::unique(xg_vMarkedCands.begin(), xg_vMarkedCands.end()),
        xg_vMarkedCands.end());

    xg_iMarkedCand = -1;
    return !xg_vMarkedCands.empty();
}

// 二重マス単語を設定する。
BOOL __fastcall XgSetMarkedWord(const std::wstring& str, WCHAR *pchNotFound)
{
    auto marks = xg_vMarks;
    auto marked = xg_strMarked;
    // 初期化する。
    xg_vMarks.clear();

    // 二重マス単語と文字マスの情報に従って二重マスを設定する。
    std::wstring word;
    for (const auto ch : str) {
        const int m = rand() % xg_nRows;
        const int n = rand() % xg_nCols;
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                const int i0 = (i + m) % xg_nRows;
                const int j0 = (j + n) % xg_nCols;
                if (ch == xg_solution.GetAt(i0, j0) &&
                    XgGetMarked(i0, j0) == -1)
                {
                    word.push_back(ch);
                    XgSetMark(i0, j0);
                    goto break2;
                }
            }
        }
        if (pchNotFound) {
            *pchNotFound = ch;
            break;
        }
break2:;
    }

    if (str != word) {
        xg_vMarks = std::move(marks);
        xg_strMarked = std::move(marked);
        return FALSE;
    }

    xg_strMarked = std::move(word);
    return TRUE;
}

// 二重マス単語を空にする。
void __fastcall XgSetMarkedWord(void)
{
    // 空文字列を設定する。
    XgSetMarkedWord(L"");
}

//////////////////////////////////////////////////////////////////////////////
