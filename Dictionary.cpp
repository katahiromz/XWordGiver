﻿//////////////////////////////////////////////////////////////////////////////
// Dictionary.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#include "XWordGiver.hpp"

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

// 単語の長さのヒストグラム。
std::unordered_map<size_t, size_t> xg_word_length_histgram;

// 配置できる最大単語長。
int xg_nDictMaxWordLen = 7;
// 配置できる最小単語長。
int xg_nDictMinWordLen = 2;

//////////////////////////////////////////////////////////////////////////////
// 辞書データのファイル処理。

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
    LPWSTR pchTags = nullptr;
    if (pchHint) {
        pchTags = wcschr(pchHint, L'\t');
        if (pchTags) {
            *pchTags = 0;
            ++pchTags;
        }
    }

    // 単語文字列を全角・カタカナ・大文字にする。
    LCMapStringW(XG_JPN_LOCALE,
                 LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                 pchLine, static_cast<int>(wcslen(pchLine) + 1), szWord, 64);

    // 文字列の前後の空白を取り除く。
    entry.m_word = szWord;
    xg_str_trim(entry.m_word);
    auto word = entry.m_word;

    // 小さな字を大きな字にする。
    for (auto& ch : entry.m_word) {
        auto it = xg_small2large.find(ch);
        if (it != xg_small2large.end())
            ch = it->second;
    }

    // ハイフン、アポストロフィ、ピリオド、カンマを取り除く。
    std::wstring tmp;
    for (auto ch : entry.m_word) {
        if (ch == L'-' || ch == 0xFF0D)
            continue;
        if (ch == L'\'' || ch == 0xFF07)
            continue;
        if (ch == L'.' || ch == 0xFF0E)
            continue;
        if (ch == L',' || ch == 0xFF0C)
            continue;
        tmp += ch;
    }
    entry.m_word = std::move(tmp);

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
        xg_str_replace_all(strTags, L",", L"\t");
        xg_str_replace_all(strTags, L"  ", L" ");

        std::unordered_set<std::wstring> tags;
        mstr_split_insert(tags, strTags, L"\t");

        for (auto tag : tags) {
            xg_str_trim(tag);
            if (tag.empty())
                continue;
            xg_word_to_tags_map[word].insert(tag);
            xg_tag_histgram[tag] += 1;
        }
    }
}

// Unicodeのファイルの中身を読み込む。
bool XgReadUnicodeFile(LPWSTR pszData, size_t cchData)
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

// 辞書ファイルを読み込む。
bool __fastcall XgLoadDictFile(LPCWSTR pszFile)
{
    // 初期化する。
    xg_dict_1.clear();
    xg_dict_2.clear();
    xg_word_to_tags_map.clear();
    xg_tag_histgram.clear();
    xg_strDefaultTheme.clear();
    xg_word_length_histgram.clear();
    xg_nDictMaxWordLen = 255;
    xg_nDictMinWordLen = 2;

    try {
        // ファイルを開く。
        std::wstring strText;
        if (!XgReadTextFileAll(pszFile, strText))
            return false;

        if (!XgReadUnicodeFile(&strText[0], strText.size()))
            return false;

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
            auto it2 = xg_word_to_tags_map.find(data.m_word);
            if (it2 == xg_word_to_tags_map.end()) {
                xg_dict_2.push_back(data);
            } else {
                auto& tags2 = it2->second;
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

        // 単語の長さのヒストグラムを構築する。
        for (auto& worddata : xg_dict_1) {
            const auto len = worddata.m_word.size();
            xg_word_length_histgram[len]++;
        }
        for (auto& worddata : xg_dict_2) {
            const auto len = worddata.m_word.size();
            xg_word_length_histgram[len]++;
        }

        // 最長、最短を更新。
        if (xg_word_length_histgram.size()) {
            xg_nDictMaxWordLen = 2;
            xg_nDictMinWordLen = 255;
            for (auto& pair : xg_word_length_histgram) {
                if (xg_nDictMaxWordLen < static_cast<int>(pair.first))
                    xg_nDictMaxWordLen = static_cast<int>(pair.first);
                if (xg_nDictMinWordLen > static_cast<int>(pair.first))
                    xg_nDictMinWordLen = static_cast<int>(pair.first);
            }
        }

        return true; // 成功。
    } catch (...) {
        // 例外が発生した。
        assert(0);
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
        [](const XG_WordData& a, const XG_WordData& b) noexcept {
            return a.m_word < b.m_word;
        }
    );
    return ret;
}

std::wstring XgLoadTitleFromDict(LPCWSTR pszPath)
{
    std::wstring ret = pszPath;

    if (FILE *fp = _wfopen(pszPath, L"rb"))
    {
        char buf[256];
        WCHAR szText[256];
        const BOOL bJapanese = XgIsUserJapanese();
        int nLineCount = 0;
        auto name_jpn = L"# NAME-JPN:";
        auto name_eng = L"# NAME-ENG:";
        while (fgets(buf, _countof(buf), fp))
        {
            if (nLineCount++ >= 8)
                break;

            MultiByteToWideChar(CP_UTF8, 0, buf, -1, szText, _countof(szText));

            const auto len1 = lstrlenW(name_jpn);
            if (memcmp(szText, name_jpn, len1 * sizeof(WCHAR)) == 0)
            {
                if (bJapanese)
                {
                    std::wstring str = &szText[len1];
                    xg_str_trim(str);
                    ret = str;
                    break;
                }
            }

            const auto len2 = lstrlenW(name_eng);
            if (memcmp(szText, name_eng, len2 * sizeof(WCHAR)) == 0)
            {
                std::wstring str = &szText[len2];
                xg_str_trim(str);
                ret = str;
                if (!bJapanese)
                    break;
            }
        }
        fclose(fp);
    }

    return ret;
}

// フォルダから辞書群を読み込む。
BOOL XgLoadDictsFromDir(LPCWSTR pszDir)
{
    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW find;
    HANDLE hFind;

    // ファイル *.dic を列挙する。
    StringCbCopy(szPath, sizeof(szPath), pszDir);
    PathAppend(szPath, L"*.dic");
    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            StringCbCopy(szPath, sizeof(szPath), pszDir);
            PathAppend(szPath, find.cFileName);
            auto path = szPath;
            auto title = XgLoadTitleFromDict(szPath);
            xg_dicts.emplace_back(path, title);
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    // ファイル *.tsv を列挙する。
    StringCbCopy(szPath, sizeof(szPath), pszDir);
    PathAppend(szPath, L"*.tsv");
    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            StringCbCopy(szPath, sizeof(szPath), pszDir);
            PathAppend(szPath, find.cFileName);
            auto path = szPath;
            auto title = XgLoadTitleFromDict(szPath);
            xg_dicts.emplace_back(path, title);
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    // ファイル *.dic も列挙する。
    StringCbCopy(szPath, sizeof(szPath), pszDir);
    PathAppend(szPath, L"*.dic");
    hFind = FindFirstFileW(szPath, &find);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            StringCbCopy(szPath, sizeof(szPath), pszDir);
            PathAppend(szPath, find.cFileName);
            auto path = szPath;
            auto title = XgLoadTitleFromDict(szPath);
            xg_dicts.emplace_back(path, title);
        } while (FindNextFile(hFind, &find));
        FindClose(hFind);
    }

    return !xg_dicts.empty();
}

// 辞書ファイルをすべて読み込む。
BOOL XgLoadDictsAll(void)
{
    xg_dicts.clear();

    // 実行ファイルのパスを取得。
    WCHAR sz[MAX_PATH];
    ::GetModuleFileNameW(nullptr, sz, sizeof(sz));

    // 実行ファイルの近くにある.dic/.tsvファイルを列挙する。
    PathRemoveFileSpec(sz);
    PathAppend(sz, L"DICT");
    if (!XgLoadDictsFromDir(sz))
    {
        PathRemoveFileSpec(sz);
        PathRemoveFileSpec(sz);
        PathAppend(sz, L"DICT");
        if (!XgLoadDictsFromDir(sz))
        {
            PathRemoveFileSpec(sz);
            PathRemoveFileSpec(sz);
            PathAppend(sz, L"DICT");
            XgLoadDictsFromDir(sz);
        }
    }

    // 読み込んだ中から見つかるか？
    bool bFound = false;
    for (auto& entry : xg_dicts)
    {
        const auto& file = entry.m_filename;
        if (lstrcmpiW(file.c_str(), xg_dict_name.c_str()) == 0)
        {
            bFound = true;
            break;
        }
    }

    if (xg_dict_name.empty() || !bFound)
    {
        LPCWSTR pszNormal = XgLoadStringDx1(IDS_NORMAL_DICT);
        LPCWSTR pszBasicDict = XgLoadStringDx2(IDS_BASICDICTDATA);
        for (auto& entry : xg_dicts)
        {
            const auto& file = entry.m_filename;
            if (file.find(pszBasicDict) != std::wstring::npos &&
                file.find(pszNormal) != std::wstring::npos &&
                PathFileExistsW(file.c_str()))
            {
                xg_dict_name = file;
                break;
            }
        }
        if (xg_dict_name.empty() && xg_dicts.size())
        {
            xg_dict_name = xg_dicts[0].m_filename;
        }
    }

    // ファイルが実際に存在するかチェックし、存在しない項目は消す。
    for (size_t i = 0; i < xg_dicts.size(); ++i) {
        auto& entry = xg_dicts[i];
        const auto& file = entry.m_filename;
        if (!PathFileExistsW(file.c_str())) {
            xg_dicts.erase(xg_dicts.begin() + i);
            --i;
        }
    }

    return !xg_dicts.empty();
}

// 辞書名をセットする。
void XgSetDict(const std::wstring& strFile)
{
    // 辞書名を格納。
    xg_dict_name = strFile;

    // 辞書として追加、ソート、一意にする。
    if (xg_dicts.size() < XG_MAX_DICTS)
    {
        auto title = XgLoadTitleFromDict(strFile.c_str());
        xg_dicts.emplace_back(strFile, title);
        std::sort(xg_dicts.begin(), xg_dicts.end(),
            [](const XG_DICT& e1, const XG_DICT& e2) noexcept {
                return e1.m_filename < e2.m_filename;
            }
        );
        auto last = std::unique(xg_dicts.begin(), xg_dicts.end(),
            [](const XG_DICT& e1, const XG_DICT& e2) noexcept {
                return e1.m_filename == e2.m_filename;
            }
        );
        xg_dicts.erase(last, xg_dicts.end());
    }
}

// 辞書を切り替える。
void XgSelectDict(HWND hwnd, size_t iDict)
{
    // 範囲外は無視。
    if (iDict >= xg_dicts.size())
        return;

    // 辞書を読み込み、セットする。
    auto& entry = xg_dicts[iDict];
    const auto& file = entry.m_filename;
    if (XgLoadDictFile(file.c_str()))
    {
        XgSetDict(file.c_str());
        XgSetInputModeFromDict(hwnd);
    }

    // 二重マス単語の候補をクリアする。
    xg_vMarkedCands.clear();
    xg_iMarkedCand = -1;
}
