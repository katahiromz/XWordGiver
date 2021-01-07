//////////////////////////////////////////////////////////////////////////////
// Dictionary.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"
#include "Auto.hpp"

// 辞書データ。
std::vector<XG_WordData>     xg_dict_data;

//////////////////////////////////////////////////////////////////////////////
// 辞書データのファイル処理。

// Unicodeを一行読み込む。
void XgReadUnicodeLine(LPWSTR pchLine)
{
    XG_WordData entry;
    WCHAR szWord[64];

    // コメント行を無視する。
    if (*pchLine == L'#') {
        return;
    }

    // 空行を無視する。
    if (*pchLine == L'\0') {
        return;
    }

    // ヒントをさがす。
    LPWSTR pchHint = wcschr(pchLine, L'\t');
    if (pchHint) {
        *pchHint = 0;
        pchHint++;
    } else {
        pchHint = nullptr;
    }

    // 第３フィールド以降を無視する。
    if (pchHint) {
        if (LPWSTR pch = wcschr(pchHint, L'\t'))
            *pch = 0;
    }

    // 単語文字列を全角・カタカナ・大文字にする。
    LCMapStringW(JPN_LOCALE,
        LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
        pchLine, static_cast<int>(wcslen(pchLine) + 1), szWord, 64);

    // 文字列の前後の空白を取り除く。
    entry.m_word = szWord;
    xg_str_trim(entry.m_word);

    // 小さな字を大きな字にする。
    for (size_t i = 0; i < ARRAYSIZE(xg_large); i++)
        xg_str_replace_all(entry.m_word,
            std::wstring(xg_small[i]), std::wstring(xg_large[i]));

    // 単語とヒントを登録する。一字以下の単語は登録しない。
    if (entry.m_word.size() > 1) {
        if (pchHint) {
            entry.m_hint = pchHint;
            xg_str_trim(entry.m_hint);
        }
        else
            entry.m_hint.clear();

        xg_dict_data.emplace_back(std::move(entry));
    }
}

// Unicodeのファイルの中身を読み込む。
bool XgReadUnicodeFile(LPWSTR pszData, DWORD /*cchData*/)
{
    // 最初の一行を取り出す。
    LPWSTR pchLine = wcstok(pszData, xg_pszNewLine);
    if (pchLine == nullptr)
        return false;

    // 一行ずつ処理する。
    do {
        XgReadUnicodeLine(pchLine);
        pchLine = wcstok(nullptr, xg_pszNewLine);
    } while (pchLine);
    return true;
}

// ANSI (Shift_JIS) のファイルの中身を読み込む。
bool __fastcall XgReadAnsiFile(LPCSTR pszData, DWORD /*cchData*/)
{
    // Unicodeに変換できないときは失敗。
    int cchWide = MultiByteToWideChar(SJIS_CODEPAGE, 0, pszData, -1, nullptr, 0);
    if (cchWide == 0)
        return false;

    // Unicodeに変換して処理する。
    std::wstring strWide(cchWide - 1, 0);
    MultiByteToWideChar(SJIS_CODEPAGE, 0, pszData, -1, &strWide[0], cchWide);
    return XgReadUnicodeFile(&strWide[0], cchWide - 1);
}

// UTF-8のファイルの中身を読み込む。
bool __fastcall XgReadUtf8File(LPCSTR pszData, DWORD /*cchData*/)
{
    // Unicodeに変換できないときは失敗。
    int cchWide = MultiByteToWideChar(CP_UTF8, 0, pszData, -1, nullptr, 0);
    if (cchWide == 0)
        return false;

    // Unicodeに変換して処理する。
    std::wstring strWide(cchWide - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, pszData, -1, &strWide[0], cchWide);
    return XgReadUnicodeFile(&strWide[0], cchWide - 1);
}

// 辞書ファイルを読み込む。
bool __fastcall XgLoadDictFile(LPCWSTR pszFile)
{
    DWORD cbRead, i;

    // 初期化する。
    xg_dict_data.clear();

    // ファイルを開く。
    AutoCloseHandle hFile(CreateFileW(pszFile, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                      OPEN_EXISTING, 0, nullptr));
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    // ファイルサイズを取得する。
    DWORD cbFile = ::GetFileSize(hFile, nullptr);
    if (cbFile == 0xFFFFFFFF)
        return false;

    try {
        // メモリを確保してファイルから読み込む。
        std::vector<BYTE> pbFile(cbFile + 4, 0);
        i = cbFile;
        if (!ReadFile(hFile, &pbFile[0], cbFile, &cbRead, nullptr))
            return false;

        // BOMチェック。
        if (pbFile[0] == 0xFF && pbFile[1] == 0xFE) {
            // Unicode
            std::wstring str = reinterpret_cast<LPWSTR>(&pbFile[2]);
            if (!XgReadUnicodeFile(&str[0], static_cast<DWORD>(str.size())))
                return false;
            i = 0;
        } else if (pbFile[0] == 0xFE && pbFile[1] == 0xFF) {
            // Unicode BE
            XgSwab(&pbFile[0], cbFile);
            std::wstring str = reinterpret_cast<LPWSTR>(&pbFile[2]);
            if (!XgReadUnicodeFile(&str[0], static_cast<DWORD>(str.size())))
                return false;
            i = 0;
        } else if (pbFile[0] == 0xEF && pbFile[1] == 0xBB && pbFile[2] == 0xBF) {
            // UTF-8
            std::wstring str = XgUtf8ToUnicode(reinterpret_cast<LPCSTR>(&pbFile[3]));
            if (!XgReadUnicodeFile(&str[0], static_cast<DWORD>(str.size())))
                return false;
            i = 0;
        } else {
            for (i = 0; i < cbFile; i++) {
                // ナル文字があればUnicodeと判断する。
                if (pbFile[i] == 0) {
                    if (i & 1) {
                        // Unicode
                        if (!XgReadUnicodeFile(reinterpret_cast<LPWSTR>(&pbFile[0]), cbFile / 2))
                            return false;
                    } else {
                        // Unicode BE
                        XgSwab(&pbFile[0], cbFile);
                        if (!XgReadUnicodeFile(reinterpret_cast<LPWSTR>(&pbFile[0]), cbFile / 2))
                            return false;
                    }
                    i = 0;
                    break;
                }
            }
        }

        if (i == cbFile) {
            if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    reinterpret_cast<LPCSTR>(pbFile[0]),
                                    static_cast<int>(cbFile), nullptr, 0))
            {
                // UTF-8
                if (!XgReadUtf8File(reinterpret_cast<LPSTR>(&pbFile[0]), cbFile))
                    return false;
            } else {
                // ANSI
                if (!XgReadAnsiFile(reinterpret_cast<LPSTR>(&pbFile[0]), cbFile))
                    return false;
            }
        }

        // 二分探索のために、並び替えておく。
        std::sort(xg_dict_data.begin(), xg_dict_data.end(), xg_word_less());
        return true; // 成功。
    } catch (...) {
        // 例外が発生した。
    }

    return false; // 失敗。
}

// 辞書データをソートし、一意的にする。
void __fastcall XgSortAndUniqueDictData(void)
{
    sort(xg_dict_data.begin(), xg_dict_data.end(), xg_word_less());
    auto last = unique(xg_dict_data.begin(), xg_dict_data.end(),
                       XG_WordData_Equal());
    std::vector<XG_WordData> dict_data;
    for (auto it = xg_dict_data.begin(); it != last; ++it) {
        dict_data.emplace_back(*it);
    }
    xg_dict_data = std::move(dict_data);
}

// ミニ辞書を作成する。
std::vector<XG_WordData> XgCreateMiniDict(void)
{
    std::vector<XG_WordData> ret;
    for (const auto& hint : xg_vecTateHints)
    {
        ret.emplace_back(hint.m_strWord, hint.m_strHint);
    }
    for (const auto& hint : xg_vecYokoHints)
    {
        ret.emplace_back(hint.m_strWord, hint.m_strHint);
    }
    std::sort(ret.begin(), ret.end(),
        [](const XG_WordData& a, const XG_WordData& b) {
            return a.m_word < b.m_word;
        }
    );
    return ret;
}
