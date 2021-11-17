﻿//////////////////////////////////////////////////////////////////////////////
// Utils.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#pragma once
#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx1(int id);

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx2(int id);

// フィルター文字列を作る。
LPWSTR __fastcall XgMakeFilterString(LPWSTR psz);

// ショートカットのターゲットのパスを取得する。
bool __fastcall XgGetPathOfShortcutW(LPCWSTR pszLnkFile, LPWSTR pszPath);

// 文字列の前後の空白を取り除く。
void __fastcall xg_str_trim(std::wstring& str);

// 文字列を置換する。
void __fastcall xg_str_replace_all(std::wstring &s, const std::wstring& from, const std::wstring& to);

// 文字列からマルチセットへ変換する。
void __fastcall xg_str_to_multiset(std::unordered_multiset<WCHAR>& mset, const std::wstring& str);

// ベクターからマルチセットへ変換する。
void __fastcall xg_vec_to_multiset(std::unordered_multiset<WCHAR>& mset, const std::vector<WCHAR>& str);

// 部分マルチセットかどうか？
bool __fastcall xg_submultiseteq(const std::unordered_multiset<WCHAR>& ms1,
                                 const std::unordered_multiset<WCHAR>& ms2);

// UTF-8 -> Unicode.
std::wstring __fastcall XgUtf8ToUnicode(const std::string& ansi);

// ダイアログを中央によせる関数。
void __fastcall XgCenterDialog(HWND hwnd);

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType);

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxIndirectW(LPMSGBOXPARAMS lpMsgBoxParams);

// ReadMeを開く。
void __fastcall XgOpenReadMe(HWND hwnd);

// Licenseを開く。
void __fastcall XgOpenLicense(HWND hwnd);

// ファイルが書き込み可能か？
bool __fastcall XgCanWriteFile(const WCHAR *pszFile);

// Unicode -> UTF8
std::string XgUnicodeToUtf8(const std::wstring& wide);

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi);

// Unicode -> ANSI
std::string XgUnicodeToAnsi(const std::wstring& wide);

// JSON文字列を作る。
std::wstring XgJsonEncodeString(const std::wstring& str);

// 16進で表す。
char XgToHex(char code);

// 24BPPビットマップを作成。
HBITMAP XgCreate24BppBitmap(HDC hDC, LONG width, LONG height);

BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm);

//////////////////////////////////////////////////////////////////////////////
// パスを作る。

BOOL XgMakePathW(LPCWSTR pszPath);

//////////////////////////////////////////////////////////////////////////////

// エンディアン変換。
void XgSwab(LPBYTE pbFile, DWORD cbFile);

//////////////////////////////////////////////////////////////////////////////

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(const std::string& html_utf8, const std::string& style_utf8/* = ""*/);
// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(const std::wstring& html_wide, const std::wstring& style_wide/* = L""*/);

//////////////////////////////////////////////////////////////////////////////

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

template <typename T_STR_CONTAINER>
inline typename T_STR_CONTAINER::value_type
mstr_join(const T_STR_CONTAINER& container,
          const typename T_STR_CONTAINER::value_type& sep)
{
    typename T_STR_CONTAINER::value_type result;
    typename T_STR_CONTAINER::const_iterator it, end;
    it = container.begin();
    end = container.end();
    if (it != end)
    {
        result = *it;
        for (++it; it != end; ++it)
        {
            result += sep;
            result += *it;
        }
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////