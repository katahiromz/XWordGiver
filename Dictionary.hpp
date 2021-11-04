//////////////////////////////////////////////////////////////////////////////
// Dictionary.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#ifndef __XG_DICTIONARY_HPP__
#define __XG_DICTIONARY_HPP__

//////////////////////////////////////////////////////////////////////////////
// 単語データ。

struct XG_WordData
{
    // 単語。
    std::wstring     m_word;
    // ヒント。
    std::wstring     m_hint;

    // コンストラクタ。
    XG_WordData() { }

    // コンストラクタ。
    XG_WordData(const std::wstring& word_) : m_word(word_)
    {
    }

    // コンストラクタ。
    XG_WordData(const std::wstring& word_, const std::wstring& hint_) :
        m_word(word_), m_hint(hint_)
    {
    }

    // コンストラクタ。
    XG_WordData(std::wstring&& word_, std::wstring&& hint_) :
        m_word(std::move(word_)), m_hint(std::move(hint_))
    {
    }

    // コピーコンストラクタ。
    XG_WordData(const XG_WordData& wd) :
        m_word(wd.m_word), m_hint(wd.m_hint)
    {
    }

    // コピーコンストラクタ。
    XG_WordData(XG_WordData&& wd) :
        m_word(std::move(wd.m_word)), m_hint(std::move(wd.m_hint))
    {
    }

    // 代入。
    void __fastcall operator=(const XG_WordData& wd) {
        m_word = wd.m_word;
        m_hint = wd.m_hint;
    }

    // 代入。
    void __fastcall operator=(XG_WordData&& wd) {
        m_word = std::move(wd.m_word);
        m_hint = std::move(wd.m_hint);
    }

    void clear() {
        m_word.clear();
        m_hint.clear();
    }
};

namespace std
{
    template <>
    inline void swap(XG_WordData& data1, XG_WordData& data2) {
        std::swap(data1.m_word, data2.m_word);
        std::swap(data1.m_hint, data2.m_hint);
    }
}

// 辞書データ。優先タグか否かで分ける。
extern std::vector<XG_WordData> xg_dict_1, xg_dict_2;
// タグ付けデータ。
extern std::unordered_map<std::wstring, std::unordered_set<std::wstring> > xg_word_to_tags_map;
// タグのヒストグラム。
extern std::unordered_map<std::wstring, size_t> xg_tag_histgram;
// 単語の長さのヒストグラム。
extern std::unordered_map<size_t, size_t> xg_word_length_histgram;
// 優先タグ。
extern std::unordered_set<std::wstring> xg_priority_tags;
// 除外タグ。
extern std::unordered_set<std::wstring> xg_forbidden_tags;
// テーマ文字列。
extern std::wstring xg_strTheme;
// 既定のテーマ文字列。
extern std::wstring xg_strDefaultTheme;
// テーマが変更されたか？
extern bool xg_bThemeModified;
// 配置できる最大単語長。
extern INT xg_nDictMaxWordLen;
// 配置できる最小単語長。
extern INT xg_nDictMinWordLen;

// 辞書ファイルを読み込む。
bool __fastcall XgLoadDictFile(LPCWSTR pszFile);
// テーマをリセットする。
void __fastcall XgResetTheme(HWND hwnd);
// テーマを設定する。
void XgSetThemeString(const std::wstring& strTheme);
// テーマ文字列をパースする。
void XgParseTheme(std::unordered_set<std::wstring>& priority,
                  std::unordered_set<std::wstring>& forbidden,
                  const std::wstring& strTheme);

// ミニ辞書を作成する。
std::vector<XG_WordData> XgCreateMiniDict(void);

//////////////////////////////////////////////////////////////////////////////
// XG_WordData構造体を比較するファンクタ。

class xg_word_less
{
public:
    bool __fastcall operator()(const XG_WordData& wd1, const XG_WordData& wd2)
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
    bool operator()(const XG_WordData wd1, const XG_WordData wd2)
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
    xg_word_toolong(int n)
    {
        m_n = n;
    }

    bool __fastcall operator()(const XG_WordData& wd) const
    {
        return static_cast<int>(wd.m_word.size()) > m_n;
    }

protected:
    int m_n;    // 最大長。
};

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef __XG_DICTIONARY_HPP__
