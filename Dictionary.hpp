﻿//////////////////////////////////////////////////////////////////////////////
// Dictionary.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#pragma once

// 辞書の最大数。
#define XG_MAX_DICTS 64

//////////////////////////////////////////////////////////////////////////////
// 単語データ。

struct XG_WordData
{
    // 単語。
    QStringW     m_word;
    // ヒント。
    QStringW     m_hint;

    // コンストラクタ。
    XG_WordData() noexcept { }

    // コンストラクタ。
    XG_WordData(const QStringW& word_) : m_word(word_)
    {
    }

    // コンストラクタ。
    XG_WordData(const QStringW& word_, const QStringW& hint_) :
        m_word(word_), m_hint(hint_)
    {
    }

    // コンストラクタ。
    XG_WordData(QStringW&& word_, QStringW&& hint_) noexcept :
        m_word(std::move(word_)), m_hint(std::move(hint_))
    {
    }

    // コピーコンストラクタ。
    XG_WordData(const XG_WordData& wd) = default;

    // コピーコンストラクタ。
    XG_WordData(XG_WordData&& wd) noexcept :
        m_word(std::move(wd.m_word)), m_hint(std::move(wd.m_hint))
    {
    }

    // 代入。
    void __fastcall operator=(const XG_WordData& wd) {
        m_word = wd.m_word;
        m_hint = wd.m_hint;
    }

    // 代入。
    void __fastcall operator=(XG_WordData&& wd) noexcept {
        m_word = std::move(wd.m_word);
        m_hint = std::move(wd.m_hint);
    }

    void clear() noexcept {
        m_word.clear();
        m_hint.clear();
    }
};

namespace std
{
    template <>
    inline void swap(XG_WordData& data1, XG_WordData& data2) noexcept {
        std::swap(data1.m_word, data2.m_word);
        std::swap(data1.m_hint, data2.m_hint);
    }
}

// 辞書データ。優先タグか否かで分ける。
extern std::vector<XG_WordData> xg_dict_1, xg_dict_2;
// タグ付けデータ。
extern std::unordered_map<QStringW, std::unordered_set<QStringW> > xg_word_to_tags_map;
// タグのヒストグラム。
extern std::unordered_map<QStringW, size_t> xg_tag_histgram;
// 単語の長さのヒストグラム。
extern std::unordered_map<size_t, size_t> xg_word_length_histgram;
// 優先タグ。
extern std::unordered_set<QStringW> xg_priority_tags;
// 除外タグ。
extern std::unordered_set<QStringW> xg_forbidden_tags;
// テーマ文字列。
extern QStringW xg_strTheme;
// 既定のテーマ文字列。
extern QStringW xg_strDefaultTheme;
// テーマが変更されたか？
extern bool xg_bThemeModified;
// 配置できる最大単語長。
extern int xg_nDictMaxWordLen;
// 配置できる最小単語長。
extern int xg_nDictMinWordLen;

// 辞書ファイルを読み込む。
bool __fastcall XgLoadDictFile(LPCWSTR pszFile);
// テーマをリセットする。
void __fastcall XgResetTheme(HWND hwnd);
// テーマを設定する。
void XgSetThemeString(const QStringW& strTheme);
// テーマ文字列をパースする。
void XgParseTheme(std::unordered_set<QStringW>& priority,
                  std::unordered_set<QStringW>& forbidden,
                  const QStringW& strTheme);

// ミニ辞書を作成する。
std::vector<XG_WordData> XgCreateMiniDict(void);

//////////////////////////////////////////////////////////////////////////////
// XG_WordData構造体を比較するファンクタ。

class xg_word_less
{
public:
    bool __fastcall operator()(const XG_WordData& wd1, const XG_WordData& wd2) noexcept
    {
        return wd1.m_word < wd2.m_word;
    }
};

// 単語データが存在するか？
inline bool XgWordDataExists(const std::vector<XG_WordData>& data, const XG_WordData& wd)
{
    return binary_search(data.begin(), data.end(), wd, xg_word_less());
}

struct XG_WordData_Equal
{
    bool operator()(const XG_WordData wd1, const XG_WordData wd2) noexcept
    {
        return wd1.m_word == wd2.m_word;
    }
};

// 辞書データをソートし、一意的にする。
void __fastcall XgSortAndUniqueDictData(std::vector<XG_WordData>& data);

//////////////////////////////////////////////////////////////////////////////
// 単語が長すぎるかどうかを確認するファンクタ。

class xg_word_toolong
{
public:
    xg_word_toolong(int n) noexcept
    {
        m_n = n;
    }

    bool __fastcall operator()(const XG_WordData& wd) const noexcept
    {
        return static_cast<int>(wd.m_word.size()) > m_n;
    }

protected:
    int m_n;    // 最大長。
};

//////////////////////////////////////////////////////////////////////////////
// 2文字未満の単語を削除する。

template <typename t_string>
inline bool XgTrimDict(std::unordered_set<t_string>& words)
{
    for (auto& word : words) {
        if (word.size() <= 1) {
            words.erase(word);
        }
    }
    return !words.empty();
}

template <typename t_string>
inline bool XgTrimDict(std::vector<t_string>& words)
{
    auto it = std::remove_if(words.begin(), words.end(), [](const t_string& word) noexcept {
        return (word.size() <= 1);
    });
    words.erase(it, words.end());
    return !words.empty();
}

//////////////////////////////////////////////////////////////////////////////

// 辞書のエントリ。
struct XG_DICT
{
    QStringW m_filename; // ファイル名。
    QStringW m_friendly_name; // 親切な名前。
    XG_DICT() noexcept
    {
    }
    XG_DICT(const QStringW& filename, const QStringW& friendly_name)
        : m_filename(filename)
        , m_friendly_name(friendly_name)
    {
    }
};

// 現在の辞書名。
extern QStringW xg_dict_name;
// すべての辞書ファイル。
typedef std::deque<XG_DICT> dicts_t;
extern dicts_t xg_dicts;

// 辞書名をセットする。
void XgSetDict(const QStringW& strFile);
// フォルダから辞書群を読み込む。
BOOL XgLoadDictsFromDir(LPCWSTR pszDir);
// 辞書ファイルをすべて読み込む。
BOOL XgLoadDictsAll(void);
// 辞書を切り替える。
void XgSelectDict(HWND hwnd, size_t iDict);
// 辞書からタイトルを取得。
QStringW XgLoadTitleFromDict(LPCWSTR pszPath);
