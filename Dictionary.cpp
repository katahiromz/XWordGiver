//////////////////////////////////////////////////////////////////////////////
// Dictionary.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"
#include "Auto.hpp"

// 辞書データ。
std::vector<XG_WordData>     xg_dict_1, xg_dict_2;

// タグ付けデータ。
std::unordered_map<std::wstring, std::unordered_set<std::wstring> > xg_word_to_tags_map;

// タグのヒストグラム。
std::unordered_map<std::wstring, size_t> xg_tag_histgram;

// 優先タグ。
std::unordered_set<std::wstring> xg_priority_tags;

// 除外タグ。
std::unordered_set<std::wstring> xg_forbidden_tags;

// テーマ文字列。
std::wstring xg_strTheme;

// 既定のテーマ文字列。
std::wstring xg_strDefaultTheme;

// テーマが変更されたか？
bool xg_bThemeModified = false;

//////////////////////////////////////////////////////////////////////////////
// 辞書データのファイル処理。

template <typename T_STR_CONTAINER>
inline void
mstr_split(T_STR_CONTAINER& container,
           const typename T_STR_CONTAINER::value_type& str,
           const typename T_STR_CONTAINER::value_type& chars)
{
    container.clear();
    size_t i = 0, k = str.find_first_of(chars);
    while (k != T_STR_CONTAINER::value_type::npos)
    {
        container.push_back(str.substr(i, k - i));
        i = k + 1;
        k = str.find_first_of(chars, i);
    }
    container.push_back(str.substr(i));
}

template <typename T_STR_CONTAINER>
inline void
mstr_split_insert(T_STR_CONTAINER& container,
                  const typename T_STR_CONTAINER::value_type& str,
                  const typename T_STR_CONTAINER::value_type& chars)
{
    container.clear();
    size_t i = 0, k = str.find_first_of(chars);
    while (k != T_STR_CONTAINER::value_type::npos)
    {
        container.insert(str.substr(i, k - i));
        i = k + 1;
        k = str.find_first_of(chars, i);
    }
    container.insert(str.substr(i));
}

template <typename T_CHAR>
inline void mstr_trim(std::basic_string<T_CHAR>& str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == string_type::npos) || (j == string_type::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

// テーマ文字列をパースする。
void XgParseTheme(std::unordered_set<std::wstring>& priority,
                  std::unordered_set<std::wstring>& forbidden,
                  const std::wstring& strTheme)
{
    priority.clear();
    forbidden.clear();

    std::vector<std::wstring> strs;
    mstr_split(strs, strTheme, L",");
    for (const auto& str : strs) {
        std::wstring item = str;
        if (item.empty())
            continue;
        bool minus = false;
        if (item[0] == L'-') {
            minus = true;
            item = item.substr(1);
        }
        if (item[0] == L'+') {
            item = item.substr(1);
        }
        if (minus) {
            forbidden.emplace(item);
        } else {
            priority.emplace(item);
        }
    }
}

// テーマを設定する。
void XgSetThemeString(const std::wstring& strTheme)
{
    XgParseTheme(xg_priority_tags, xg_forbidden_tags, strTheme);
    xg_strTheme = strTheme;

    std::unordered_set<std::wstring> priority, forbidden;
    XgParseTheme(priority, forbidden, xg_strDefaultTheme);
    xg_bThemeModified = (priority != xg_priority_tags || forbidden != xg_forbidden_tags);
}

// テーマをリセットする。
void __fastcall XgResetTheme(HWND hwnd)
{
    XgSetThemeString(xg_strDefaultTheme);
    xg_bThemeModified = false;
}

// Unicodeを一行読み込む。
void XgReadUnicodeLine(LPWSTR pchLine)
{
    XG_WordData entry;
    WCHAR szWord[64];

    // コメント行を無視する。
    if (*pchLine == L'#') {
        // ただし、コメント内に"DEFAULT:"がある場合は例外として既定のテーマを読み取る。
        pchLine = wcsstr(pchLine, L"DEFAULT:");
        if (pchLine) {
            pchLine += wcslen(L"DEFAULT:");
            xg_strDefaultTheme = pchLine;
            mstr_trim(xg_strDefaultTheme, L" \t\r\n");
        }
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

    // 第３フィールドはタグ群。
    LPWSTR pchTags = NULL;
    if (pchHint) {
        pchTags = wcschr(pchHint, L'\t');
        if (pchTags) {
            *pchTags = 0;
            ++pchTags;
        }
    }

    // 単語文字列を全角・カタカナ・大文字にする。
    LCMapStringW(JPN_LOCALE,
        LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
        pchLine, static_cast<int>(wcslen(pchLine) + 1), szWord, 64);

    // 文字列の前後の空白を取り除く。
    entry.m_word = szWord;
    xg_str_trim(entry.m_word);
    auto word = entry.m_word;

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

        xg_dict_1.emplace_back(std::move(entry));
    }

    // タグがあれば、単語のタグ付けを行う。
    if (pchTags) {
        std::wstring strTags = pchTags;
        xg_str_replace_all(strTags, L",", L" ");
        xg_str_replace_all(strTags, L"  ", L" ");

        std::unordered_set<std::wstring> tags;
        mstr_split_insert(tags, strTags, L" ");

        xg_word_to_tags_map[word] = tags;

        for (auto& tag : tags) {
            if (tag.empty())
                continue;
            xg_tag_histgram[tag] += 1;
        }
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
    xg_dict_1.clear();
    xg_dict_2.clear();
    xg_word_to_tags_map.clear();
    xg_tag_histgram.clear();
    xg_strDefaultTheme.clear();

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

        // テーマが変更されたか？
        if (!xg_bThemeModified) {
            // テーマを設定する。
            XgSetThemeString(xg_strDefaultTheme);
        } else {
            // テーマを設定する。
            XgSetThemeString(xg_strTheme);
        }

        // xg_forbidden_tags のタグが付いた単語は除外する（erase-remove idiom）。
        auto it = std::remove_if(xg_dict_1.begin(), xg_dict_1.end(), [](const XG_WordData& data) {
            auto found = xg_word_to_tags_map.find(data.m_word);
            if (found != xg_word_to_tags_map.end()) {
                auto& tags2 = found->second;
                for (auto& tag2 : tags2) {
                    for (auto& tag1 : xg_forbidden_tags) {
                        if (tag1 == tag2) {
                            return true;
                        }
                    }
                }
            }
            return false;
        });
        xg_dict_1.erase(it, xg_dict_1.end());

        // xg_priority_tagsのタグのついてない単語をxg_dict_2に振り分ける。
        std::vector<XG_WordData> tmp;
        for (auto& data : xg_dict_1) {
            auto found = xg_word_to_tags_map.find(data.m_word);
            if (found == xg_word_to_tags_map.end()) {
                xg_dict_2.push_back(data);
            } else {
                auto& tags2 = found->second;
                for (auto& tag2 : tags2) {
                    bool found = false;
                    for (auto& tag1 : xg_priority_tags) {
                        if (tag1 == tag2) {
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        tmp.push_back(data);
                    } else {
                        xg_dict_2.push_back(data);
                    }
                }
            }
        }
        xg_dict_1 = std::move(tmp);

        // xg_dict_1が空のときは交換する。
        if (xg_dict_1.empty()) {
            std::swap(xg_dict_1, xg_dict_2);
        }

        // 二分探索のために、並び替えておく。
        std::sort(xg_dict_1.begin(), xg_dict_1.end(), xg_word_less());
        std::sort(xg_dict_2.begin(), xg_dict_2.end(), xg_word_less());
        return true; // 成功。
    } catch (...) {
        // 例外が発生した。
    }

    return false; // 失敗。
}

// 辞書データをソートし、一意的にする。
void __fastcall XgSortAndUniqueDictData(std::vector<XG_WordData>& data)
{
    std::sort(data.begin(), data.end(), xg_word_less());
    auto it = std::unique(data.begin(), data.end(), XG_WordData_Equal());
    data.erase(it, data.end());
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
