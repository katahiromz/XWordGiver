//////////////////////////////////////////////////////////////////////////////
// Marks.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////
// global variables

// マーク。
std::vector<XG_Pos>      xg_vMarks;

// 二重マス単語候補。
std::vector<std::wstring>     xg_vMarkedCands;

//////////////////////////////////////////////////////////////////////////////
// static variables

// 選択している二重マス単語候補のインデックス。
static int  s_iMarkedCands = -1;

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
            StringCbPrintf(sz, sizeof(sz), L"(%u, %u)%c\r\n",
                mark.m_i + 1, mark.m_j + 1,
                xg_solution.GetAt(mark.m_i, mark.m_j));
        else
            StringCbPrintf(sz, sizeof(sz), L"(%u, %u)%c\r\n",
                mark.m_i + 1, mark.m_j + 1,
                xg_xword.GetAt(mark.m_i, mark.m_j));
        str += sz;
    }
}

// マークされているか（二重マス）？
int __fastcall XgGetMarked(const std::vector<XG_Pos>& vMarks, const XG_Pos& pos)
{
    const int size = static_cast<int>(vMarks.size());
    for (int i = 0; i < size; i++) {
        if (vMarks[i] == pos)
            return i;
    }
    return -1;
}

// マークされているか（二重マス）？
int __fastcall XgGetMarked(const std::vector<XG_Pos>& vMarks, int i, int j)
{
    return XgGetMarked(vMarks, XG_Pos(i, j));
}

// マークされているか（二重マス）？
int __fastcall XgGetMarked(int i, int j)
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
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_APPTITLE2), str.data(), pchFileTitle);
            ::SetWindowTextW(xg_hMainWnd, sz);
        } else {
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_APPTITLE), pchFileTitle);
            ::SetWindowTextW(xg_hMainWnd, sz);
        }
    } else {
        // マークされているか？ 答えを表示するか？
        if (XgGetMarkWord(xw, str) && xg_bShowAnswer) {
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(1177), str.data());
            ::SetWindowTextW(xg_hMainWnd, sz);
        } else {
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(1176));
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
    int nMarked = XgGetMarked(i, j);
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
        int i = _wtoi(psz) - 1;
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
        int j = _wtoi(psz) - 1;

        // 読み飛ばし。
        while ((L'0' <= *psz && *psz <= L'9') || *psz == L')')
            psz++;
        while (XgIsCharKanaW(*psz) || XgIsCharHankakuAlphaW(*psz) ||
               XgIsCharZenkakuAlphaW(*psz) || XgIsCharKanjiW(*psz) ||
               XgIsCharHangulW(*psz))
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
    s_iMarkedCands = -1;
    xg_vMarkedCands.clear();

    // 解がなければ失敗。
    if (!xg_bSolved)
        return false;

    // マスの文字をマルチセットに変換。
    std::unordered_multiset<WCHAR> msCells;
    xg_vec_to_multiset(msCells, xg_solution.m_vCells);

    // すべての登録されている単語について繰り返す。
    for (const auto& data : xg_dict_data) {
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
            for (size_t k = 0; k < word.size(); k++) {
                for (int i = 0; i < xg_nRows; ++i) {
                    for (int j = 0; j < xg_nCols; ++j) {
                        if (word[k] == xg_solution.GetAt(i, j) &&
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

    sort(xg_vMarkedCands.begin(), xg_vMarkedCands.end(), xg_wstring_size_greater());
    return !xg_vMarkedCands.empty();
}

// 二重マス単語を設定する。
void __fastcall XgSetMarkedWord(const std::wstring& str)
{
    // 初期化する。
    xg_vMarks.clear();

    // 二重マス単語と文字マスの情報に従って二重マスを設定する。
    for (const auto ch : str) {
        int m = rand() % xg_nRows;
        int n = rand() % xg_nCols;
        for (int i = 0; i < xg_nRows; i++) {
            for (int j = 0; j < xg_nCols; j++) {
                int i0 = (i + m) % xg_nRows;
                int j0 = (j + n) % xg_nCols;
                if (ch == xg_solution.GetAt(i0, j0) &&
                    XgGetMarked(i0, j0) == -1)
                {
                    XgSetMark(i0, j0);
                    goto break2;
                }
            }
        }
break2:;
    }

    // イメージを更新する。
    XgUpdateImage(xg_hMainWnd, 0, 0);
}

// 二重マス単語を空にする。
void __fastcall XgSetMarkedWord(void)
{
    // 空文字列を設定する。
    std::wstring str;
    XgSetMarkedWord(str);
}

// 次の二重マス単語を取得する。
void __fastcall XgGetNextMarkedWord(void)
{
    if (xg_vMarkedCands.empty()) {
        // 二重マス単語候補がない場合、候補を取得する。
        if (!XgGetMarkedCandidates()) {
            // 候補が無かった。
            XgCenterMessageBoxW(xg_hMainWnd, XgLoadStringDx1(IDS_NOMARKCANDIDATES), NULL,
                                MB_ICONERROR);
            return;     // 失敗。
        }

        // 二重マス単語の候補を取得した。最初の候補を設定する。
        s_iMarkedCands = 0;
    }
    else if (s_iMarkedCands + 1 < static_cast<int>(xg_vMarkedCands.size()))
    {
        // 次の候補を設定する。
        s_iMarkedCands++;
    }

    // 二重マス単語を設定する。
    XgSetMarkedWord(xg_vMarkedCands[s_iMarkedCands]);
}

// 前の二重マス単語を取得する。
void __fastcall XgGetPrevMarkedWord(void)
{
    if (xg_vMarkedCands.empty()) {
        // 二重マス単語候補がない場合、候補を取得する。
        if (!XgGetMarkedCandidates()) {
            // 候補が無かった。
            XgCenterMessageBoxW(xg_hMainWnd, XgLoadStringDx1(IDS_NOMARKCANDIDATES), NULL,
                                MB_ICONERROR);
            return;
        }

        // 二重マス単語の候補を取得した。最初の候補を設定する。
        s_iMarkedCands = 0;
    } else if (s_iMarkedCands > 0) {
        // 一つ前の候補を設定する。
        s_iMarkedCands--;
    }

    // 二重マス単語を設定する。
    XgSetMarkedWord(xg_vMarkedCands[s_iMarkedCands]);
}

//////////////////////////////////////////////////////////////////////////////
