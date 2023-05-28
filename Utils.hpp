//////////////////////////////////////////////////////////////////////////////
// Utils.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#pragma once
#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////

// デバッグ出力。
#ifdef NDEBUG
    #define DebugPrintfW(file, lineno, pszFormat, ...)
    #define DOUTW(fmt, ...)
#else
    void __cdecl DebugPrintfW(const char *file, int lineno, LPCWSTR pszFormat, ...);
    #define DOUTW(fmt, ...) DebugPrintfW(__FILE__, __LINE__, (fmt), ##__VA_ARGS__)
#endif

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx1(int id) noexcept;

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx2(int id) noexcept;

// フィルター文字列を作る。
LPWSTR __fastcall XgMakeFilterString(LPWSTR psz) noexcept;

// ショートカットのターゲットのパスを取得する。
bool __fastcall XgGetPathOfShortcutW(LPCWSTR pszLnkFile, LPWSTR pszPath);

// 空白文字群。
#define XG_WHITE_SPACES  L" \t\r\n\x3000"

// 文字列の前後の空白を取り除く。
static inline void __fastcall xg_str_trim(std::wstring& str)
{
    const size_t i = str.find_first_not_of(XG_WHITE_SPACES);
    const size_t j = str.find_last_not_of(XG_WHITE_SPACES);
    if (i != std::wstring::npos)
        str = str.substr(i, j - i + 1);
    else
        str.clear();
}

// 文字列の右側の空白を取り除く。
static inline void __fastcall xg_str_trim_right(std::wstring& str)
{
    const size_t j = str.find_last_not_of(XG_WHITE_SPACES);
    if (j != std::wstring::npos)
        str = str.substr(0, j + 1);
    else
        str.clear();
}

// 文字列をエスケープする。
std::wstring xg_str_escape(const std::wstring& str);
// 文字列をアンエスケープする。
std::wstring xg_str_unescape(const std::wstring& str);

// 文字列を引用する。
static inline std::wstring xg_str_quote(const std::wstring& str)
{
    std::wstring ret = L"\"";
    ret += xg_str_escape(str);
    ret += L"\"";
    return ret;
}
// 文字列を逆引用する。
static inline std::wstring xg_str_unquote(const std::wstring& str)
{
    std::wstring ret;
    ret = str;
    xg_str_trim(ret);
    if (ret.empty())
        return L"";
    if (ret[0] == L'"')
        ret = ret.substr(1);
    if (ret.size() && ret[ret.size() - 1] == L'"')
        ret = ret.substr(0, ret.size() - 1);
    return xg_str_unescape(ret);
}

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
void __fastcall XgCenterDialog(HWND hwnd) noexcept;

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType) noexcept;

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxIndirectW(LPMSGBOXPARAMS lpMsgBoxParams) noexcept;

// ReadMeを開く。
void __fastcall XgOpenReadMe(HWND hwnd) noexcept;

// Licenseを開く。
void __fastcall XgOpenLicense(HWND hwnd) noexcept;

// ファイルが書き込み可能か？
bool __fastcall XgCanWriteFile(const WCHAR *pszFile);

// Unicode -> UTF8
std::string XgUnicodeToUtf8(const std::wstring& wide);

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi);

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi, INT charset);

// Unicode -> ANSI
std::string XgUnicodeToAnsi(const std::wstring& wide);

// JSON文字列を作る。
std::wstring XgJsonEncodeString(const std::wstring& str);

// 16進で表す。
char XgToHex(char code) noexcept;

// 24BPPビットマップを作成。
HBITMAP XgCreate24BppBitmap(HDC hDC, LONG width, LONG height) noexcept;

BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm);

//////////////////////////////////////////////////////////////////////////////
// パスを作る。

BOOL XgMakePathW(LPCWSTR pszPath);

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
    const auto i = str.find_first_not_of(spaces);
    const auto j = str.find_last_not_of(spaces);
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

// 整数を文字列にする。
LPCWSTR XgIntToStr(int nValue);
// バイナリを16進にする。
std::wstring XgBinToHex(const void *ptr, size_t size);
// 16進をバイナリにする。
void XgHexToBin(std::vector<BYTE>& data, const std::wstring& str);
// 画像を読み込む。
BOOL XgLoadImage(const std::wstring& filename, HBITMAP& hbm, HENHMETAFILE& hEMF);
// 画像ファイルか？
BOOL XgIsImageFile(LPCWSTR pszFileName) noexcept;
// テキストファイルか？
BOOL XgIsTextFile(LPCWSTR pszFileName) noexcept;
// ファイルを読み込む。
BOOL XgReadFileAll(LPCWSTR file, std::string& strBinary);
BOOL XgReadTextFileAll(LPCWSTR file, std::wstring& strText, bool ecw = false);
// ファイルを読み込む。
BOOL XgWriteFileAll(LPCWSTR file, const std::string& strBinary) noexcept;
// エンディアン変換。
void XgSwab(LPBYTE pbFile, size_t cbFile) noexcept;

// コンボボックスからテキストを取得。
BOOL ComboBox_RealGetText(HWND hwndCombo, LPWSTR pszText, int cchText) noexcept;
// コンボボックスにテキストを設定。
BOOL ComboBox_RealSetText(HWND hwndCombo, LPCWSTR pszText) noexcept;

struct XG_FileManager
{
    std::wstring m_filename;
    std::wstring m_looks;
    std::wstring m_block_dir;
    std::unordered_map<std::wstring, std::string> m_path2contents;
    std::unordered_map<std::wstring, HBITMAP> m_path2hbm;
    std::unordered_map<std::wstring, HENHMETAFILE> m_path2hemf;

    XG_FileManager() noexcept
    {
    }
    ~XG_FileManager() noexcept
    {
    }

    void set_file(LPCWSTR filename);
    std::wstring get_looks_file();
    void set_looks(LPCWSTR looks);
    void delete_handles() noexcept;
    bool load_image(LPCWSTR filename);
    bool save_image(const std::wstring& path);
    bool save_image2(std::wstring& path);
    bool save_images();
    bool save_images(const std::unordered_set<std::wstring>& files);
    void convert();
    void clear();
    std::wstring get_file_title(const std::wstring& str) const;
    std::wstring get_full_path(const std::wstring& str) const;
    std::wstring get_block_dir();
    std::wstring get_block_dir_worker() const;
    bool get_files_dir(std::wstring& dir) const;
    std::wstring get_canonical(const std::wstring& path);
    std::wstring get_real_path(const std::wstring& path);
    bool get_list(std::vector<std::wstring>& paths);
    bool load_block_image(const std::wstring& path);
    bool load_block_image(const std::wstring& path, HBITMAP& hbm, HENHMETAFILE& hEMF);

    void convert(std::wstring& path)
    {
        if (path.empty())
            return;
        std::wstring canonical = L"$FILES\\";
        canonical += get_file_title(path);
        path = std::move(canonical);
    }

    template <typename T_TYPE>
    void convert(std::unordered_map<std::wstring, T_TYPE>& map)
    {
        std::unordered_map<std::wstring, T_TYPE> new_map;
        for (auto& pair : map)
        {
            if (pair.first.empty())
                continue;
            std::wstring path = L"$FILES\\";
            path += get_file_title(pair.first);
            new_map[path] = pair.second;
        }
        map = std::move(new_map);
    }
};

std::shared_ptr<XG_FileManager>& XgGetFileManager(void);

//////////////////////////////////////////////////////////////////////////////
