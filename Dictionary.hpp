//////////////////////////////////////////////////////////////////////////////
// Dictionary.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2019 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

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
    inline void swap(XG_WordData& data1, XG_WordData& data2)
    {
        std::swap(data1.m_word, data2.m_word);
        std::swap(data1.m_hint, data2.m_hint);
    }
}

// 辞書データ。
extern std::vector<XG_WordData> xg_dict_data;

// 辞書ファイルを読み込む。
bool __fastcall XgLoadDictFile(LPCWSTR pszFile);

// 辞書から単語を探し出す。
XG_WordData *XgFindWordFromDict(const std::wstring& word);

// 辞書に更新があるかどうか？
bool __fastcall XgIsDictUpdated(void);

// 辞書に更新があるかどうか？
void __fastcall XgDictSetModified(bool modified);

// 辞書データを更新する。
bool __fastcall XgUpdateDictData(void);

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
inline bool XgWordDataExists(const XG_WordData& wd)
{
    return binary_search(xg_dict_data.begin(), xg_dict_data.end(),
                         wd, xg_word_less());
}

// 辞書データをソートする。
inline void XgSortDictData(void)
{
    sort(xg_dict_data.begin(), xg_dict_data.end(), xg_word_less());
}

struct XG_WordData_Equal
{
    bool operator()(const XG_WordData wd1, const XG_WordData wd2)
    {
        return wd1.m_word == wd2.m_word;
    }
};

// 辞書データをソートし、一意的にする。
void __fastcall XgSortAndUniqueDictData(void);

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
