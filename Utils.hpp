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
static inline void __fastcall xg_str_trim(XGStringW& str)
{
    const size_t i = str.find_first_not_of(XG_WHITE_SPACES);
    const size_t j = str.find_last_not_of(XG_WHITE_SPACES);
    if (i != XGStringW::npos)
        str = str.substr(i, j - i + 1);
    else
        str.clear();
}

// 文字列の右側の空白を取り除く。
static inline void __fastcall xg_str_trim_right(XGStringW& str)
{
    const size_t j = str.find_last_not_of(XG_WHITE_SPACES);
    if (j != XGStringW::npos)
        str = str.substr(0, j + 1);
    else
        str.clear();
}

// 文字列をエスケープする。
XGStringW xg_str_escape(const XGStringW& str);
// 文字列をアンエスケープする。
XGStringW xg_str_unescape(const XGStringW& str);

// 文字列を引用する。
static inline XGStringW xg_str_quote(const XGStringW& str)
{
    XGStringW ret = L"\"";
    ret += xg_str_escape(str);
    ret += L"\"";
    return ret;
}
// 文字列を逆引用する。
static inline XGStringW xg_str_unquote(const XGStringW& str)
{
    XGStringW ret;
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
bool __fastcall xg_str_replace_all(XGStringW &s, const XGStringW& from, const XGStringW& to);

// 文字列からマルチセットへ変換する。
void __fastcall xg_str_to_multiset(std::unordered_multiset<WCHAR>& mset, const XGStringW& str);

// ベクターからマルチセットへ変換する。
void __fastcall xg_vec_to_multiset(std::unordered_multiset<WCHAR>& mset, const std::vector<WCHAR>& str);

// 部分マルチセットかどうか？
bool __fastcall xg_submultiseteq(const std::unordered_multiset<WCHAR>& ms1,
                                 const std::unordered_multiset<WCHAR>& ms2);

// UTF-8 -> Unicode.
XGStringW __fastcall XgUtf8ToUnicode(const std::string& ansi);

// ダイアログを中央によせる関数。
void __fastcall XgCenterDialog(HWND hwnd) noexcept;

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType) noexcept;

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxIndirectW(LPMSGBOXPARAMS lpMsgBoxParams) noexcept;

// ファイルが書き込み可能か？
bool __fastcall XgCanWriteFile(const WCHAR *pszFile);

// Unicode -> UTF8
std::string XgUnicodeToUtf8(const XGStringW& wide);

// ANSI -> Unicode
XGStringW XgAnsiToUnicode(const std::string& ansi);

// ANSI -> Unicode
XGStringW XgAnsiToUnicode(const std::string& ansi, INT charset);

// Unicode -> ANSI
std::string XgUnicodeToAnsi(const XGStringW& wide);

// JSON文字列を作る。
XGStringW XgJsonEncodeString(const XGStringW& str);

// 16進で表す。
char XgToHex(char code) noexcept;

// 24BPPビットマップを作成。
HBITMAP XgCreate24BppBitmap(HDC hDC, LONG width, LONG height) noexcept;

// パック形式のDIBを作成する。
BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm);

//////////////////////////////////////////////////////////////////////////////

// パスを作る。
BOOL XgMakePathW(LPCWSTR pszPath);
// ローカルファイルを見つける。
BOOL XgFindLocalFile(LPWSTR pszPath, UINT cchPath, LPCWSTR pszFileName);
// ローカルファイルを開く。
inline void XgOpenLocalFile(HWND hwnd, LPCWSTR pszFileName)
{
    WCHAR szPath[MAX_PATH];
    if (XgFindLocalFile(szPath, _countof(szPath), pszFileName))
        ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
}

//////////////////////////////////////////////////////////////////////////////

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(const std::string& html_utf8, const std::string& style_utf8/* = ""*/);
// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(const XGStringW& html_wide, const XGStringW& style_wide/* = L""*/);

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

template <typename T_STRING>
inline void mstr_trim(T_STRING& str, const typename T_STRING::value_type *spaces)
{
    const auto i = str.find_first_not_of(spaces);
    const auto j = str.find_last_not_of(spaces);
    if ((i == str.npos) || (j == str.npos))
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
XGStringW XgBinToHex(const void *ptr, size_t size);
// 16進をバイナリにする。
void XgHexToBin(std::vector<BYTE>& data, const XGStringW& str);
// 画像を読み込む。
BOOL XgLoadImage(const XGStringW& filename, HBITMAP& hbm, HENHMETAFILE& hEMF);
// 画像ファイルか？
BOOL XgIsImageFile(LPCWSTR pszFileName) noexcept;
// テキストファイルか？
BOOL XgIsTextFile(LPCWSTR pszFileName) noexcept;
// ファイルを読み込む。
BOOL XgReadFileAll(LPCWSTR file, std::string& strBinary);
BOOL XgReadTextFileAll(LPCWSTR file, XGStringW& strText, bool ecw = false);
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
    XGStringW m_filename;
    XGStringW m_looks;
    XGStringW m_block_dir;
    std::unordered_map<XGStringW, std::string> m_path2contents;
    std::unordered_map<XGStringW, HBITMAP> m_path2hbm;
    std::unordered_map<XGStringW, HENHMETAFILE> m_path2hemf;

    XG_FileManager() noexcept
    {
    }
    ~XG_FileManager() noexcept
    {
    }

    void set_file(LPCWSTR filename);
    XGStringW get_looks_file();
    void set_looks(LPCWSTR looks);
    void delete_handles() noexcept;
    bool load_image(LPCWSTR filename);
    bool save_image(const XGStringW& path);
    bool save_image2(XGStringW& path);
    bool save_images();
    bool save_images(const std::unordered_set<XGStringW>& files);
    void convert();
    void clear();
    XGStringW get_file_title(const XGStringW& str) const;
    XGStringW get_full_path(const XGStringW& str) const;
    XGStringW get_block_dir();
    XGStringW get_block_dir_worker() const;
    bool get_files_dir(XGStringW& dir) const;
    XGStringW get_canonical(const XGStringW& path);
    XGStringW get_real_path(const XGStringW& path);
    bool get_list(std::vector<XGStringW>& paths);
    bool load_block_image(const XGStringW& path);
    bool load_block_image(const XGStringW& path, HBITMAP& hbm, HENHMETAFILE& hEMF);

    void convert(XGStringW& path)
    {
        if (path.empty())
            return;
        XGStringW canonical = L"$FILES\\";
        canonical += get_file_title(path);
        path = std::move(canonical);
    }

    template <typename T_TYPE>
    void convert(std::unordered_map<XGStringW, T_TYPE>& map)
    {
        std::unordered_map<XGStringW, T_TYPE> new_map;
        for (auto& pair : map)
        {
            if (pair.first.empty())
                continue;
            XGStringW path = L"$FILES\\";
            path += get_file_title(pair.first);
            new_map[path] = pair.second;
        }
        map = std::move(new_map);
    }
};

std::shared_ptr<XG_FileManager>& XgGetFileManager(void);

//////////////////////////////////////////////////////////////////////////////
