//////////////////////////////////////////////////////////////////////////////
// Utils.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#include "XWordGiver.hpp"
#define min std::min
#define max std::max
#include <gdiplus.h>

std::shared_ptr<XG_FileManager> xg_pFileManager;

std::shared_ptr<XG_FileManager>& XgGetFileManager(void)
{
    if (!xg_pFileManager)
        xg_pFileManager = std::make_shared<XG_FileManager>();
    return xg_pFileManager;
}

//////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
    // デバッグ出力。
    void __cdecl DebugPrintfW(const char *file, int lineno, LPCWSTR pszFormat, ...)
    {
        va_list va;
        int cch;
        static WCHAR s_szText[1024];
        va_start(va, pszFormat);
        ::EnterCriticalSection(&xg_cs);
        if (file) {
            StringCchPrintfW(s_szText, _countof(s_szText), L"%hs (%d): ", file, lineno);
            cch = lstrlenW(s_szText);
            StringCchVPrintfW(&s_szText[cch], _countof(s_szText) - cch, pszFormat, va);
        } else {
            StringCchVPrintfW(s_szText, _countof(s_szText), pszFormat, va);
        }
        OutputDebugStringW(s_szText);
        ::LeaveCriticalSection(&xg_cs);
        va_end(va);
    }
#endif

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx1(int id) noexcept
{
    static WCHAR sz[512];
    LoadStringW(xg_hInstance, id, sz, _countof(sz));
    return sz;
}

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx2(int id) noexcept
{
    static WCHAR sz[512];
    LoadStringW(xg_hInstance, id, sz, _countof(sz));
    return sz;
}

// フィルター文字列を作る。
LPWSTR __fastcall XgMakeFilterString(LPWSTR psz) noexcept
{
    // 文字列中の '|' を '\0' に変換する。
    LPWSTR pch = psz;
    while (*pch != L'\0') {
        if (*pch == L'|')
            *pch = L'\0';
        pch++;
    }
    return psz;
}

// ショートカットのターゲットのパスを取得する。
bool __fastcall XgGetPathOfShortcutW(LPCWSTR pszLnkFile, LPWSTR pszPath)
{
    IShellLinkW*     pShellLink;
    IPersistFile*    pPersistFile;
    WIN32_FIND_DATAW find;
    bool             bRes = false;

    pszPath[0] = L'\0';
    HRESULT hRes = CoInitialize(nullptr);
    if (SUCCEEDED(hRes)) {
        if (SUCCEEDED(hRes = CoCreateInstance(CLSID_ShellLink, nullptr, 
            CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&pShellLink)))
        {
            if (SUCCEEDED(hRes = pShellLink->QueryInterface(IID_IPersistFile, 
                (void **)&pPersistFile)))
            {
                hRes = pPersistFile->Load(pszLnkFile, STGM_READ);
                if (SUCCEEDED(hRes)) {
                    if (SUCCEEDED(hRes = pShellLink->GetPath(pszPath, MAX_PATH, &find, 0)))
                    {
                        bRes = (::GetFileAttributesW(pszPath) != 0xFFFFFFFF);
                    }
                }
                pPersistFile->Release();
            }
            pShellLink->Release();
        }
        CoUninitialize();
    }
    return bRes;
}

// 文字列を置換する。
void __fastcall xg_str_replace_all(std::wstring &s, const std::wstring& from, const std::wstring& to)
{
    std::wstring t;
    size_t i = 0;
    while ((i = s.find(from, i)) != std::wstring::npos) {
        t = s.substr(0, i);
        t += to;
        t += s.substr(i + from.size());
        s = t;
        i += to.size();
    }
}

// 文字列からマルチセットへ変換する。
void __fastcall xg_str_to_multiset(
    std::unordered_multiset<WCHAR>& mset, const std::wstring& str)
{
    // マルチセットが空であることを仮定する。
    assert(mset.empty());
    //mset.clear();

    // 事前に予約して、スピードを得る。
    mset.reserve(str.size());

    // 各文字について。
    for (auto ch : str) {
        // 黒マスや空白マスを無視する。
        if (ch == ZEN_BLACK || ch == ZEN_SPACE)
            continue;

        // 文字をマルチセットに追加する。
        mset.emplace(ch);
    }
}

// ベクターからマルチセットへ変換する。
void __fastcall xg_vec_to_multiset(
    std::unordered_multiset<WCHAR>& mset, const std::vector<WCHAR>& str)
{
    // マルチセットが空であることを仮定する。
    assert(mset.empty());
    //mset.clear();

    // 事前に予約して、スピードを得る。
    mset.reserve(str.size());

    // 各文字について。
    for (auto ch : str) {
        // 黒マスや空白マスを無視する。
        if (ch == ZEN_BLACK || ch == ZEN_SPACE)
            continue;

        // 文字をマルチセットに追加する。
        mset.emplace(ch);
    }
}

// 部分マルチセットかどうか？
bool __fastcall xg_submultiseteq(const std::unordered_multiset<WCHAR>& ms1,
                                 const std::unordered_multiset<WCHAR>& ms2)
{
    for (const auto& elem : ms1) {
        if (ms1.count(elem) > ms2.count(elem))
            return false;
    }
    return true;
}

// UTF-8 -> Unicode.
std::wstring __fastcall XgUtf8ToUnicode(const std::string& ansi)
{
    // 変換先の文字数を取得する。
    const int cch = MultiByteToWideChar(CP_UTF8, 0, ansi.data(), -1, nullptr, 0);
    if (cch == 0)
        return L"";

    // 変換先のバッファを確保する。
    std::wstring uni(cch - 1, 0);

    // 変換して格納する。
    MultiByteToWideChar(CP_UTF8, 0, ansi.data(), -1, &uni[0], cch);
    return uni;
}

// ダイアログを中央によせる関数。
void __fastcall XgCenterDialog(HWND hwnd) noexcept
{
    // 子ウィンドウか？
    const bool bChild = !!(::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD);

    // オーナーウィンドウ（親ウィンドウ）を取得する。
    HWND hwndOwner;
    if (bChild)
        hwndOwner = ::GetParent(hwnd);
    else
        hwndOwner = ::GetWindow(hwnd, GW_OWNER);

    // オーナーウィンドウ（親ウィンドウ）の座標を取得する。
    // オーナーウィンドウ（親ウィンドウ）がないときはワークエリアを使う。
    RECT rc, rcOwner;
    if (hwndOwner != nullptr)
        ::GetWindowRect(hwndOwner, &rcOwner);
    else
        ::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcOwner, 0);

    // ウィンドウの座標をスクリーン座標で取得する。
    ::GetWindowRect(hwnd, &rc);

    // スクリーン座標で中央寄せの位置を計算する。
    POINT pt;
    pt.x = rcOwner.left +
        ((rcOwner.right - rcOwner.left) - (rc.right - rc.left)) / 2;
    pt.y = rcOwner.top +
        ((rcOwner.bottom - rcOwner.top) - (rc.bottom - rc.top)) / 2;

    // 子ウィンドウなら、スクリーン座標をクライアント座標に変換する。
    if (bChild && hwndOwner != nullptr)
        ::ScreenToClient(hwndOwner, &pt);

    // ウィンドウの位置を設定する。
    ::SetWindowPos(hwnd, nullptr, pt.x, pt.y, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // ワークエリアからはみでていたら修正する。
    ::SendMessageW(hwnd, DM_REPOSITION, 0, 0);
}

// メッセージボックスフック用。
static HHOOK s_hMsgBoxHook = nullptr;

// メッセージボックスフック用の関数。
extern "C" LRESULT CALLBACK
XgMsgBoxCbtProc(int nCode, WPARAM wParam, LPARAM /*lParam*/)
{
    if (nCode == HCBT_ACTIVATE) {
        // ウィンドウがアクティブ化されようとしている。
        // ウィンドウハンドルを取得。
        HWND hwnd = reinterpret_cast<HWND>(wParam);

        // ウィンドウクラスの確認。
        WCHAR szClassName[MAX_PATH];
        ::GetClassNameW(hwnd, szClassName, _countof(szClassName));
        if (::lstrcmpiW(szClassName, L"#32770") == 0) {
            // ダイアログだった。おそらくメッセージボックス。
            // 中央寄せする。
            XgCenterDialog(hwnd);

            // フックを解除する。
            if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
                s_hMsgBoxHook = nullptr;
        }
    }
    // allow the operation
    return 0;
}

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType) noexcept
{
    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    // フックのためにウィンドウのインスタンスを取得する。
    HINSTANCE hInst =
        reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

    // フックを開始。
    const auto dwThreadID = ::GetCurrentThreadId();
    s_hMsgBoxHook = ::SetWindowsHookEx(WH_CBT, XgMsgBoxCbtProc, hInst, dwThreadID);

    // メッセージボックスを表示する。
    const int nID = MessageBoxW(hwnd, pszText, pszCaption, uType);

    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    return nID;     // メッセージボックスの戻り値。
}

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxIndirectW(LPMSGBOXPARAMS lpMsgBoxParams) noexcept
{
    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    // フックのためにウィンドウのインスタンスを取得する。
    HINSTANCE hInst =
        reinterpret_cast<HINSTANCE>(
            ::GetWindowLongPtr(lpMsgBoxParams->hwndOwner, GWLP_HINSTANCE));

    // フックを開始。
    const auto dwThreadID = ::GetCurrentThreadId();
    s_hMsgBoxHook = ::SetWindowsHookEx(WH_CBT, XgMsgBoxCbtProc, hInst, dwThreadID);

    // メッセージボックスを表示する。
    const int nID = MessageBoxIndirectW(lpMsgBoxParams);

    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    return nID;     // メッセージボックスの戻り値。
}

// ReadMeを開く。
void __fastcall XgOpenReadMe(HWND hwnd) noexcept
{
    // 実行ファイルのパスを取得。
    WCHAR szPath[MAX_PATH];
    ::GetModuleFileNameW(nullptr, szPath, _countof(szPath));

    // ReadMeへのパスを作成。
    PathRemoveFileSpec(szPath);
    PathAppend(szPath, XgLoadStringDx1(IDS_README));

    // ReadMeを開く。
    ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
}

// Licenseを開く。
void __fastcall XgOpenLicense(HWND hwnd) noexcept
{
    // 実行ファイルのパスを取得。
    WCHAR szPath[MAX_PATH];
    ::GetModuleFileNameW(nullptr, szPath, _countof(szPath));

    // Licenseへのパスを作成。
    PathRemoveFileSpec(szPath);
    PathAppend(szPath, XgLoadStringDx1(IDS_LICENSE));

    // Licenseを開く。
    ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
}

// ファイルが書き込み可能か？
bool __fastcall XgCanWriteFile(const WCHAR *pszFile)
{
    // 書き込みをすべきでない特殊フォルダのID。
    static const auto s_anFolders = make_array<int>(
        CSIDL_PROGRAM_FILES,
        CSIDL_PROGRAM_FILES_COMMON,
        CSIDL_PROGRAM_FILES_COMMONX86,
        CSIDL_PROGRAM_FILESX86,
        CSIDL_SYSTEM,
        CSIDL_SYSTEMX86,
        CSIDL_WINDOWS
    );

    // 与えられたパスファイル名。
    std::wstring str(pszFile);

    for (auto folder : s_anFolders) {
        // 特殊フォルダの位置の取得。
        LPITEMIDLIST pidl = nullptr;
        if (SUCCEEDED(::SHGetSpecialFolderLocation(xg_hMainWnd, folder, &pidl)))
        {
            // 特殊フォルダのパスを得る。
            WCHAR szPath[MAX_PATH];
            ::SHGetPathFromIDListW(pidl, szPath);
            ::CoTaskMemFree(pidl);

            // パスが一致するか？
            if (str.find(szPath) == 0)
                return false;   // ここには保存すべきでない。
        }
    }

    // 書き込みできるか？
    return _waccess(pszFile, 02) == 0;
}

// Unicode -> UTF8
std::string XgUnicodeToUtf8(const std::wstring& wide)
{
    const int len = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0)
        return "";

    std::string utf8(len - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, &utf8[0], len, nullptr, nullptr);
    return utf8;
}

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi)
{
    const int len = ::MultiByteToWideChar(XG_SJIS_CODEPAGE, 0, ansi.data(), -1, nullptr, 0);
    if (len == 0)
        return L"";

    std::wstring uni(len - 1, 0);
    ::MultiByteToWideChar(XG_SJIS_CODEPAGE, 0, ansi.data(), -1, &uni[0], len);
    return uni;
}

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi, INT codepage)
{
    const int len = ::MultiByteToWideChar(codepage, 0, ansi.data(), -1, nullptr, 0);
    if (len == 0)
        return L"";

    std::wstring uni(len - 1, 0);
    ::MultiByteToWideChar(codepage, 0, ansi.data(), -1, &uni[0], len);
    return uni;
}

// Unicode -> ANSI
std::string XgUnicodeToAnsi(const std::wstring& wide)
{
    const int len = ::WideCharToMultiByte(XG_SJIS_CODEPAGE, 0, wide.data(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0)
        return "";

    std::string ansi(len - 1, 0);
    ::WideCharToMultiByte(XG_SJIS_CODEPAGE, 0, wide.data(), -1, &ansi[0], len, nullptr, nullptr);
    return ansi;
}

// JSON文字列を作る。
std::wstring XgJsonEncodeString(const std::wstring& str)
{
    std::wstring encoded;
    wchar_t buf[16];
    size_t i;
    const auto siz = str.size();

    encoded.clear();
    for (i = 0; i < siz; ++i) {
        switch (str[i]) {
        case L'\"': encoded += L"\\\""; break;
        case L'\\': encoded += L"\\\\"; break;
        case L'/':  encoded += L"\\/"; break;
        case L'\b': encoded += L"\\b"; break;
        case L'\f': encoded += L"\\f"; break;
        case L'\n': encoded += L"\\n"; break;
        case L'\r': encoded += L"\\r"; break;
        case L'\t': encoded += L"\\t"; break;
        default:
            if (0 < str[i] && str[i] <= L'\x1F') {
                StringCchPrintf(buf, _countof(buf), L"\\u%04X", str[i]);
                encoded += buf;
            } else {
                encoded += str[i];
            }
        }
    }
    return encoded;
}

// 16進で表す。
char XgToHex(char code) noexcept
{
    static const char s_hex[] = "0123456789abcdef";
    assert(0 <= code && code < 16);
    return s_hex[code & 15];
}

//////////////////////////////////////////////////////////////////////////////
// パスを作る。

BOOL XgMakePathW(LPCWSTR pszPath)
{
    WCHAR szPath[MAX_PATH];
    StringCchCopy(szPath, _countof(szPath), pszPath);

    const DWORD attrs = ::GetFileAttributesW(szPath);
    if (attrs != 0xFFFFFFFF) {
        return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == nullptr) {
        return TRUE;
    }
    *pch = 0;

    if (XgMakePathW(szPath)) {
        return CreateDirectoryW(pszPath, nullptr);
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

// エンディアン変換。
void XgSwab(LPBYTE pbFile, size_t cbFile) noexcept
{
    LPWORD pw = reinterpret_cast<LPWORD>(pbFile);
    size_t cw = (cbFile >> 1);
    while (cw--) {
        const WORD w = *pw;
        const BYTE lo = LOBYTE(w);
        const BYTE hi = HIBYTE(w);
        *pw = MAKEWORD(hi, lo);
        ++pw;
    }
}

//////////////////////////////////////////////////////////////////////////////

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(const std::string& html_utf8,
                               const std::string& style_utf8/* = ""*/)
{
    using namespace std;
    std::string str(
        "Version:0.9\r\n"
        "StartHTML:00000000\r\n"
        "EndHTML:00000000\r\n"
        "StartFragment:00000000\r\n"
        "EndFragment:00000000\r\n"
        "<html><head>\r\n"
        "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\r\n"
        "<style type=\"text/css\"><!--\r\n");
    str += style_utf8;
    str += 
        "--></style></head>"
        "<body>\r\n"
        "<!-- StartFragment -->\r\n";
    str += html_utf8;
    str += "\r\n<!-- EndFragment -->\r\n";
    str += "</body></html>\r\n";

    const size_t iHtmlStart = str.find("<html>");
    const size_t iHtmlEnd = str.size();
    const size_t iFragmentStart = str.find("<!-- StartFragment -->");
    const size_t iFragmentEnd = str.find("<!-- EndFragment -->");

    char buf[9];
    size_t i;
    i = str.find("StartHTML:");
    i += 10;
    StringCchPrintfA(buf, _countof(buf), "%08u", static_cast<UINT>(iHtmlStart));
    str.replace(i, 8, buf);

    i = str.find("EndHTML:");
    i += 8;
    StringCchPrintfA(buf, _countof(buf), "%08u", static_cast<UINT>(iHtmlEnd));
    str.replace(i, 8, buf);

    i = str.find("StartFragment:");
    i += 14;
    StringCchPrintfA(buf, _countof(buf), "%08u", static_cast<UINT>(iFragmentStart));
    str.replace(i, 8, buf);

    i = str.find("EndFragment:");
    i += 12;
    StringCchPrintfA(buf, _countof(buf), "%08u", static_cast<UINT>(iFragmentEnd));
    str.replace(i, 8, buf);

    return str;
}

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(const std::wstring& html_wide,
                               const std::wstring& style_wide/* = L""*/)
{
    return XgMakeClipHtmlData(
        XgUnicodeToUtf8(html_wide), XgUnicodeToUtf8(style_wide));
}

//////////////////////////////////////////////////////////////////////////////

// 24BPPビットマップを作成。
HBITMAP XgCreate24BppBitmap(HDC hDC, LONG width, LONG height) noexcept
{
    BITMAPINFO bmi;
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    return CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
}

// BITMAPINFOEX構造体。
typedef struct tagBITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} BITMAPINFOEX, FAR * LPBITMAPINFOEX;

BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm)
{
    vecData.clear();

    BITMAP bm;
    if (!GetObject(hbm, sizeof(bm), &bm))
        return FALSE;

    BITMAPINFOEX bi;
    BITMAPINFOHEADER *pbmih;
    DWORD cColors, cbColors;

    pbmih = &bi.bmiHeader;
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
    pbmih->biSize             = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth            = bm.bmWidth;
    pbmih->biHeight           = bm.bmHeight;
    pbmih->biPlanes           = 1;
    pbmih->biBitCount         = bm.bmBitsPixel;
    pbmih->biSizeImage        = bm.bmWidthBytes * bm.bmHeight;

    if (bm.bmBitsPixel < 16)
        cColors = 1 << bm.bmBitsPixel;
    else
        cColors = 0;
    cbColors = cColors * sizeof(RGBQUAD);

    std::vector<BYTE> Bits(pbmih->biSizeImage);
    HDC hDC = CreateCompatibleDC(nullptr);
    if (hDC == nullptr)
        return FALSE;

    auto pbi = reinterpret_cast<LPBITMAPINFO>(&bi);
    if (!GetDIBits(hDC, hbm, 0, bm.bmHeight, &Bits[0], pbi, DIB_RGB_COLORS))
    {
        DeleteDC(hDC);
        return FALSE;
    }

    DeleteDC(hDC);

    std::string stream;
    stream.append(reinterpret_cast<const char *>(pbmih), sizeof(*pbmih));
    stream.append(reinterpret_cast<const char *>(bi.bmiColors), cbColors);
    stream.append(reinterpret_cast<const char *>(&Bits[0]), Bits.size());
    vecData.assign(stream.begin(), stream.end());
    return TRUE;
}

// 整数を文字列にする。
LPCWSTR XgIntToStr(int nValue)
{
    static WCHAR s_szText[64];
    StringCchPrintfW(s_szText, _countof(s_szText), L"%d", nValue);
    return s_szText;
}

// バイナリを16進にする。
std::wstring XgBinToHex(const void *ptr, size_t size)
{
    std::wstring ret;
    auto pb = static_cast<const BYTE *>(ptr);
    WCHAR sz[8];
    for (size_t i = 0; i < size; ++i)
    {
        StringCchPrintfW(sz, _countof(sz), L"%02X", pb[i]);
        ret += sz;
    }
    return ret;
}

// 16進をバイナリにする。
void XgHexToBin(std::vector<BYTE>& data, const std::wstring& str)
{
    WCHAR sz[3];
    data.clear();
    bool flag = false;
    sz[2] = 0;
    for (auto& ch : str)
    {
        if (flag)
        {
            sz[1] = ch;
            const auto b = static_cast<BYTE>(wcstol(sz, nullptr, 16));
            data.insert(data.end(), b);
        }
        else
        {
            sz[0] = ch;
        }
        flag = !flag;
    }
}

BOOL XgReadFileAll(LPCWSTR file, std::string& strBinary)
{
    strBinary.clear();
    if (FILE *fin = _wfopen(file, L"rb")) {
        CHAR buf[1024];
        for (;;) {
            size_t count = fread(buf, 1, 1024, fin);
            if (!count)
                break;
            strBinary.insert(strBinary.end(), &buf[0], &buf[count]);
        }

        fclose(fin);
        return TRUE;
    }
    return FALSE;
}

// ファイルを読み込む。
BOOL XgWriteFileAll(LPCWSTR file, const std::string& strBinary) noexcept
{
    if (FILE *fout = _wfopen(file, L"wb")) {
        const bool ret = fwrite(strBinary.c_str(), 1, strBinary.size(), fout);
        fclose(fout);
        return !!ret;
    }
    return FALSE;
}

INT XgCharSetToCodePage(INT charset)
{
    switch (charset) {
    case ANSI_CHARSET:          return 1252;
    case DEFAULT_CHARSET:       return CP_ACP;
    case SYMBOL_CHARSET:        return CP_SYMBOL;
    case SHIFTJIS_CHARSET:      return 932;
    case HANGEUL_CHARSET:       return 949;
    case GB2312_CHARSET:        return 936;
    case CHINESEBIG5_CHARSET:   return 950;
    case OEM_CHARSET:           return CP_OEMCP;
    case JOHAB_CHARSET:         return 1255;
    case HEBREW_CHARSET:        return 1255;
    case ARABIC_CHARSET:        return 1256;
    case GREEK_CHARSET:         return 1253;
    case TURKISH_CHARSET:       return 1254;
    case VIETNAMESE_CHARSET:    return 1258;
    case THAI_CHARSET:          return 874;
    case EASTEUROPE_CHARSET:    return 1250;
    case RUSSIAN_CHARSET:       return 1251;
    case MAC_CHARSET:           return CP_MACCP;
    case BALTIC_CHARSET:        return 1257;
    default:                    return CP_ACP;
    }
}

INT XgDetectCodePageFromEcw(const std::string& strBinary)
{
    auto index = strBinary.find("\nCodePage:");
    if (index == strBinary.npos)
        return CP_ACP;

    // The name is "CodePage" but codepage
    index += std::strlen("\nCodePage:");
    INT charset = atoi(&strBinary.c_str()[index]);

    return XgCharSetToCodePage(charset);
}

// テキストファイルを読み込む。
BOOL XgReadTextFileAll(LPCWSTR file, std::wstring& strText, bool ecw)
{
    strText.clear();

    std::string strBinary;
    if (!XgReadFileAll(file, strBinary))
        return FALSE;

    if (strBinary.empty())
        return TRUE;

    if (ecw)
    {
        strText = XgAnsiToUnicode(strBinary, XgDetectCodePageFromEcw(strBinary));
        return TRUE;
    }

    if (strBinary.size() >= 3)
    {
        if (memcmp(strBinary.c_str(), "\xEF\xBB\xBF", 3) == 0)
        {
            // UTF-8 BOM
            strText = XgUtf8ToUnicode(&strBinary[3]);
            return TRUE;
        }
        if (memcmp(strBinary.c_str(), "\xFF\xFE", 2) == 0)
        {
            // UTF-16 LE
            auto ptr = reinterpret_cast<LPWSTR>(&strBinary[2]);
            const size_t len = (strBinary.size() - 1) / sizeof(WCHAR);
            strText.assign(ptr, len);
            return TRUE;
        }
        if (memcmp(strBinary.c_str(), "\xFE\xFF", 2) == 0)
        {
            // UTF-16 BE
            auto ptr = reinterpret_cast<LPWSTR>(&strBinary[2]);
            const size_t len = (strBinary.size() - 1) / sizeof(WCHAR);
            strText.assign(ptr, len);
            XgSwab(reinterpret_cast<LPBYTE>(&strText[0]), len * sizeof(WCHAR));
            return TRUE;
        }
    }

    size_t index = 0;
    BOOL bUTF16LE = FALSE, bUTF16BE = FALSE;
    for (auto ch : strBinary)
    {
        if (ch == 0)
        {
            if (index & 1)
            {
                bUTF16LE = TRUE;
                if (bUTF16BE)
                    break;
            }
            else
            {
                bUTF16BE = TRUE;
                if (bUTF16LE)
                    break;
            }
        }

        ++index;
        if (index >= strBinary.size())
            break;
    }

    if (bUTF16BE && bUTF16LE)
    {
        // binary
        strText = XgUtf8ToUnicode(strBinary);
        return TRUE;
    }

    if (bUTF16BE || bUTF16LE)
    {
        // UTF-16 BE/LE
        auto ptr = reinterpret_cast<LPCWSTR>(strBinary.c_str());
        const size_t len = strBinary.size() / sizeof(WCHAR);
        strText.assign(ptr, len);
        if (bUTF16BE)
            XgSwab(reinterpret_cast<LPBYTE>(&strText[0]), len * sizeof(WCHAR));
        return TRUE;
    }

    const BOOL bNotUTF8 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, strBinary.c_str(), -1,
                                              nullptr, 0) == 0;
    const BOOL bNotAnsi = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, strBinary.c_str(), -1,
                                              nullptr, 0) == 0;
    if (!bNotUTF8 && bNotAnsi)
    {
        strText = XgUtf8ToUnicode(strBinary);
        return TRUE;
    }

    if (bNotUTF8 && !bNotAnsi)
    {
        strText = XgAnsiToUnicode(strBinary);
        return TRUE;
    }

    std::wstring strFromUtf8 = XgUtf8ToUnicode(strBinary);
    if (XgUnicodeToUtf8(strFromUtf8) == strBinary)
    {
        strText = std::move(strFromUtf8);
        return TRUE;
    }

    std::wstring strFromAnsi = XgAnsiToUnicode(strBinary);
    if (XgUnicodeToAnsi(strFromAnsi) == strBinary)
    {
        strText = std::move(strFromAnsi);
        return TRUE;
    }

    strText = XgUtf8ToUnicode(strBinary);
    return TRUE;
}

// 画像ファイルか？
BOOL XgIsImageFile(LPCWSTR pszFileName) noexcept
{
    LPCWSTR pchDotExt = PathFindExtensionW(pszFileName);
    if (lstrcmpiW(pchDotExt, L".bmp") == 0 ||
        lstrcmpiW(pchDotExt, L".emf") == 0 ||
        lstrcmpiW(pchDotExt, L".gif") == 0 ||
        lstrcmpiW(pchDotExt, L".png") == 0 ||
        lstrcmpiW(pchDotExt, L".jpg") == 0)
    {
        return TRUE;
    }
    return FALSE;
}

// テキストファイルか？
BOOL XgIsTextFile(LPCWSTR pszFileName) noexcept
{
    LPCWSTR pchDotExt = PathFindExtensionW(pszFileName);
    return lstrcmpiW(pchDotExt, L".txt") == 0;
}

// 画像を読み込む。
BOOL XgLoadImage(const std::wstring& filename, HBITMAP& hbm, HENHMETAFILE& hEMF)
{
    hbm = nullptr;
    hEMF = nullptr;

    // パス名をセット。
    WCHAR szFullPath[MAX_PATH];
    if (!GetFullPathNameW(filename.c_str(), _countof(szFullPath), szFullPath, nullptr))
        return FALSE;

    LPCWSTR pchDotExt = PathFindExtensionW(szFullPath);
    if (lstrcmpiW(pchDotExt, L".bmp") == 0)
    {
        hbm = LoadBitmapFromFile(szFullPath);
        return hbm != nullptr;
    }
    if (lstrcmpiW(pchDotExt, L".emf") == 0)
    {
        // ロックを防ぐためにメモリーに読み込む。
        HENHMETAFILE hGotEMF = ::GetEnhMetaFile(szFullPath);
        hEMF = ::CopyEnhMetaFile(hGotEMF, nullptr);
        ::DeleteEnhMetaFile(hGotEMF);
        return hEMF != nullptr;
    }

    // GDI+で読み込む。
    if (HINSTANCE hGdiPlus = LoadLibraryA("gdiplus"))
    {
        using namespace Gdiplus;
        typedef GpStatus (WINAPI *FN_GdiplusStartup)(ULONG_PTR*,GDIPCONST GdiplusStartupInput*,GdiplusStartupOutput*);
        typedef VOID (WINAPI *FN_GdiplusShutdown)(ULONG_PTR);
        typedef GpStatus (WINAPI *FN_GdipCreateBitmapFromFile)(GDIPCONST WCHAR*,GpBitmap**);
        typedef GpStatus (WINAPI *FN_GdipCreateHBITMAPFromBitmap)(GpBitmap*,HBITMAP*,ARGB);
        typedef GpStatus (WINAPI *FN_GdipDisposeImage)(GpImage*);

        auto GdiplusStartup = reinterpret_cast<FN_GdiplusStartup>(::GetProcAddress(hGdiPlus, "GdiplusStartup"));
        auto GdiplusShutdown = reinterpret_cast<FN_GdiplusShutdown>(::GetProcAddress(hGdiPlus, "GdiplusShutdown"));
        auto GdipCreateBitmapFromFile = reinterpret_cast<FN_GdipCreateBitmapFromFile>(::GetProcAddress(hGdiPlus, "GdipCreateBitmapFromFile"));
        auto GdipCreateHBITMAPFromBitmap = reinterpret_cast<FN_GdipCreateHBITMAPFromBitmap>(::GetProcAddress(hGdiPlus, "GdipCreateHBITMAPFromBitmap"));
        auto GdipDisposeImage = reinterpret_cast<FN_GdipDisposeImage>(::GetProcAddress(hGdiPlus, "GdipDisposeImage"));

        if (GdiplusStartup &&
            GdiplusShutdown &&
            GdipCreateBitmapFromFile &&
            GdipCreateHBITMAPFromBitmap &&
            GdipDisposeImage)
        {
            GdiplusStartupInput gdiplusStartupInput;
            ULONG_PTR gdiplusToken;

            // GDI+の初期化。
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

            Color c;
            c.SetFromCOLORREF(RGB(255, 255, 255));

            GpBitmap *pBitmap = nullptr;
            GdipCreateBitmapFromFile(szFullPath, &pBitmap);
            if (pBitmap)
            {
                GdipCreateHBITMAPFromBitmap(pBitmap, &hbm, c.ToCOLORREF());
                GdipDisposeImage(pBitmap);
            }

            // GDI+の後処理。
            GdiplusShutdown(gdiplusToken);
        }

        FreeLibrary(hGdiPlus);
    }

    return hbm != nullptr;
}

// 文字列をエスケープする。
std::wstring xg_str_escape(const std::wstring& str)
{
    std::wstring ret;
    ret.reserve(str.size());
    for (auto ch : str)
    {
        switch (ch)
        {
        case L'\a': ret += L"\\a"; break;
        case L'\b': ret += L"\\b"; break;
        case L'\t': ret += L"\\t"; break;
        case L'\n': ret += L"\\n"; break;
        case L'\r': ret += L"\\r"; break;
        case L'\f': ret += L"\\f"; break;
        case L'\v': ret += L"\\v"; break;
        case L'\\': ret += L"\\\\"; break;
        default:
            if (ch < 0x20 || ch == 0x7F) {
                WCHAR sz[8];
                StringCchPrintf(sz, 8, L"\\%03o", ch);
                ret += sz;
            } else {
                ret += ch;
            }
        }
    }
    return ret;
}

// 文字列をアンエスケープする。
std::wstring xg_str_unescape(const std::wstring& str)
{
    std::wstring ret;
    ret.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i)
    {
        auto ch = str[i];
        if (ch != L'\\') {
            ret += ch;
            continue;
        }
        ch = str[++i];
        switch (ch)
        {
        case L'a': ret += L"\a"; break;
        case L'b': ret += L"\b"; break;
        case L't': ret += L"\t"; break;
        case L'n': ret += L"\n"; break;
        case L'r': ret += L"\r"; break;
        case L'f': ret += L"\f"; break;
        case L'v': ret += L"\v"; break;
        case L'\\': ret += L"\\"; break;
        default:
            if (L'0' <= ch && ch <= L'7') { // octal
                int k, octal = 0;
                for (k = 0; k < 3; ++k) {
                    ch = str[i + k];
                    if (!(L'0' <= ch && ch <= L'7'))
                        break;
                    octal *= 8;
                    octal += ch - L'0';
                }
                ret += static_cast<wchar_t>(octal);
                i += k - 1;
            }
            if (ch == L'x' || ch == L'X') {
                ++i;
                int k, hexi = 0;
                for (k = 0; k < 4; ++k) {
                    ch = str[i + k];
                    if (!iswxdigit(ch))
                        break;
                    hexi *= 16;
                    if (iswdigit(ch))
                        hexi += ch - L'0';
                    else if (iswlower(ch))
                        hexi += ch - L'a' + 10;
                    else if (iswupper(ch))
                        hexi += ch - L'A' + 10;
                }
                ret += static_cast<wchar_t>(hexi);
                i += k - 1;
            }
        }
    }
    return ret;
}

void XG_FileManager::set_file(LPCWSTR filename)
{
    m_filename = get_full_path(filename);
}

void XG_FileManager::set_looks(LPCWSTR looks)
{
    m_looks = get_full_path(looks);
}

bool XG_FileManager::load_image(LPCWSTR filename)
{
    std::string binary;
    if (!XgReadFileAll(get_real_path(filename).c_str(), binary))
        return false;

    auto canonical = get_canonical(filename);
    m_path2contents[canonical] = binary;

    ::DeleteObject(m_path2hbm[canonical]);
    ::DeleteEnhMetaFile(m_path2hemf[canonical]);

    return XgLoadImage(get_real_path(filename), m_path2hbm[canonical], m_path2hemf[canonical]);
}

bool XG_FileManager::save_image(const std::wstring& path)
{
    auto it = m_path2contents.find(path);
    if (it == m_path2contents.end()) {
        load_image(path.c_str());
        it = m_path2contents.find(path);
        if (it == m_path2contents.end())
            return false;
    }

    std::wstring converted = path;
    convert(converted);
    auto real = get_real_path(converted);
    return XgWriteFileAll(real.c_str(), it->second);
}

bool XG_FileManager::save_image2(std::wstring& path)
{
    if (path.empty() || path.find(L"$FILES\\") == 0)
        return true;

    auto it = m_path2contents.find(path);
    if (it == m_path2contents.end()) {
        load_image(path.c_str());
        it = m_path2contents.find(path);
        if (it == m_path2contents.end())
            return false;
    }

    ////////////////////////////////////////
    std::wstring title = get_file_title(path);
    std::wstring canonical = L"$FILES\\" + title;
    std::wstring name = PathFindFileNameW(it->first.c_str());
    std::wstring ext = PathFindExtensionW(it->first.c_str());
    name = name.substr(0, name.size() - ext.size());
    int i = 1;
    auto real = get_real_path(canonical);
    while (PathFileExistsW(real.c_str())) {
        i++;
        std::wstring fileNum = L"(" + std::to_wstring(i) + L")";

        canonical = L"$FILES\\" + name + fileNum + ext;
        real = get_real_path(canonical);
    }
    /////////////////////////////////////////////////////////////////////////

    const bool result = XgWriteFileAll(real.c_str(), it->second);
    if (result) {
        path = std::move(canonical);
    }
    return result;
}

bool XG_FileManager::save_images()
{
    for (auto& pair : m_path2contents)
    {
        auto real = get_real_path(pair.first);
        XgWriteFileAll(real.c_str(), pair.second);
    }
    return true;
}

bool XG_FileManager::save_images(const std::unordered_set<std::wstring>& files)
{
    std::unordered_set<std::wstring> erase_targets;

    for (auto& pair : m_path2contents)
    {
        if (files.count(pair.first) == 0)
        {
            erase_targets.insert(pair.first);
            continue;
        }
        auto real = get_real_path(pair.first);
        XgWriteFileAll(real.c_str(), pair.second);
    }

    for (auto& target : erase_targets)
    {
        m_path2contents.erase(target);
        DeleteObject(m_path2hbm[target]);
        m_path2hbm[target] = nullptr;
        DeleteEnhMetaFile(m_path2hemf[target]);
        m_path2hemf[target] = nullptr;
    }

    return true;
}

void XG_FileManager::convert()
{
    convert(m_path2contents);
    convert(m_path2hbm);
    convert(m_path2hemf);
    convert(xg_strBlackCellImage);
}

void XG_FileManager::clear()
{
    *this = XG_FileManager();
}

std::wstring XG_FileManager::get_file_title(const std::wstring& str) const
{
    return PathFindFileNameW(str.c_str());
}

std::wstring XG_FileManager::get_full_path(const std::wstring& str) const
{
    if (str.empty())
        return str;
    WCHAR szPath[MAX_PATH];
    GetFullPathNameW(str.c_str(), _countof(szPath), szPath, nullptr);
    return szPath;
}

std::wstring XG_FileManager::get_block_dir()
{
    if (m_block_dir.empty())
        m_block_dir = get_block_dir_worker();
    return m_block_dir;
}

std::wstring XG_FileManager::get_block_dir_worker() const
{
    WCHAR szPath[MAX_PATH];
    GetModuleFileNameW(nullptr, szPath, _countof(szPath));
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"BLOCK");
    if (PathFileExistsW(szPath))
        return get_full_path(szPath);
    GetModuleFileNameW(nullptr, szPath, _countof(szPath));
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"..\\BLOCK");
    if (PathFileExistsW(szPath))
        return get_full_path(szPath);
    GetModuleFileNameW(nullptr, szPath, _countof(szPath));
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, L"..\\..\\BLOCK");
    if (PathFileExistsW(szPath))
        return get_full_path(szPath);
    return L"";
}

bool XG_FileManager::get_files_dir(std::wstring& dir) const
{
    dir.clear();

    WCHAR szPath[MAX_PATH];
    if (m_looks.size())
    {
        StringCchCopyW(szPath, _countof(szPath), m_looks.c_str());
        PathRemoveFileSpecW(szPath);
        dir = szPath;
        return true;
    }

    StringCchCopyW(szPath, _countof(szPath), m_filename.c_str());
    PathRemoveFileSpecW(szPath);
    PathAppendW(szPath, PathFindFileNameW(m_filename.c_str()));
    PathRemoveExtensionW(szPath);
    StringCchCatW(szPath, MAX_PATH, L"_files");
    dir = szPath;
    return PathIsDirectoryW(szPath);
}

std::wstring XG_FileManager::get_canonical(const std::wstring& path)
{
    if (path.find(L"$FILES\\") == 0 || path.find(L"$BLOCK\\") == 0)
        return path;

    auto full = get_full_path(path);

    std::wstring files_dir;
    if (get_files_dir(files_dir))
    {
        files_dir += L"\\";
        auto dir = full;
        dir.resize(files_dir.size());
        if (lstrcmpiW(dir.c_str(), files_dir.c_str()) == 0)
            return L"$FILES\\" + full.substr(dir.size());
    }

    auto block_dir = get_block_dir();
    if (block_dir.size())
    {
        block_dir += L"\\";
        auto dir = full;
        dir.resize(block_dir.size());
        if (lstrcmpiW(dir.c_str(), block_dir.c_str()) == 0)
            return L"$BLOCK\\" + full.substr(dir.size());
    }

    return full;
}

std::wstring XG_FileManager::get_real_path(const std::wstring& path)
{
    if (path.find(L"$FILES\\") == 0)
    {
        std::wstring files_dir;
        if (get_files_dir(files_dir))
        {
            return files_dir + L"\\" + path.substr(7);
        }
    }

    if (path.find(L"$BLOCK\\") == 0)
    {
        auto block_dir = get_block_dir();
        return block_dir + L"\\" + path.substr(7);
    }

    if (PathIsRelativeW(path.c_str()))
        return get_full_path(path);

    return path;
}

bool XG_FileManager::get_list(std::vector<std::wstring>& paths)
{
    paths.clear();

    WIN32_FIND_DATAW find;

    std::wstring files_dir;
    if (get_files_dir(files_dir))
    {
        auto spec = files_dir;
        spec += L"\\*";

        HANDLE hFind = FindFirstFileW(spec.c_str(), &find);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                auto path = files_dir;
                path += L"\\";
                path += find.cFileName;
                if (XgIsImageFile(path.c_str()))
                {
                    paths.push_back(get_canonical(path));
                }
            } while (FindNextFile(hFind, &find));
            FindClose(hFind);
        }
    }

    {
        auto block_dir = get_block_dir();
        auto spec = block_dir;
        spec += L"\\*";
        HANDLE hFind = FindFirstFileW(spec.c_str(), &find);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    continue;
                auto path = block_dir;
                path += L"\\";
                path += find.cFileName;
                if (XgIsImageFile(path.c_str()))
                {
                    paths.push_back(get_canonical(path));
                }
            } while (FindNextFile(hFind, &find));
            FindClose(hFind);
        }
    }

    return !paths.empty();
}

std::wstring XG_FileManager::get_looks_file()
{
    if (m_looks.size())
        return m_looks;

    std::wstring path;
    get_files_dir(path);
    if (m_filename.size())
    {
        path += L"\\";
        path += PathFindFileNameW(m_filename.c_str());
        const auto idotext = path.rfind(L'.');
        if (idotext != path.npos)
            path.resize(idotext);
        path += L".looks";
        return path;
    }

    return L"";
}

void XG_FileManager::delete_handles() noexcept
{
    for (auto& pair : m_path2hbm)
    {
        DeleteObject(pair.second);
    }
    m_path2hbm.clear();

    for (auto& pair : m_path2hemf)
    {
        DeleteEnhMetaFile(pair.second);
    }
    m_path2hemf.clear();
}

bool XG_FileManager::load_block_image(const std::wstring& path, HBITMAP& hbm, HENHMETAFILE& hEMF)
{
    ::DeleteObject(hbm);
    hbm = nullptr;

    ::DeleteEnhMetaFile(hEMF);
    hEMF = nullptr;

    if (path.empty())
        return true;

    auto real = get_real_path(path);
    return XgLoadImage(real, hbm, hEMF);
}

bool XG_FileManager::load_block_image(const std::wstring& path)
{
    DeleteObject(xg_hbmBlackCell);
    xg_hbmBlackCell = nullptr;
    DeleteEnhMetaFile(xg_hBlackCellEMF);
    xg_hBlackCellEMF = nullptr;

    if (path.empty())
        return true;

    std::string binary;
    if (!XgReadFileAll(get_real_path(path).c_str(), binary))
        return false;

    if (!load_block_image(path, xg_hbmBlackCell, xg_hBlackCellEMF))
        return false;

    auto canonical = get_canonical(path);
    m_path2hbm[canonical] = xg_hbmBlackCell;
    m_path2hemf[canonical] = xg_hBlackCellEMF;
    m_path2contents[canonical] = std::move(binary);

    if (xg_nViewMode == XG_VIEW_SKELETON)
    {
        // 画像が有効ならスケルトンビューを通常ビューに戻す。
        xg_nViewMode = XG_VIEW_NORMAL;
    }

    return true;
}

// コンボボックスからテキストを取得。
BOOL ComboBox_RealGetText(HWND hwndCombo, LPWSTR pszText, int cchText) noexcept
{
    const int iItem = ComboBox_GetCurSel(hwndCombo);
    if (iItem == CB_ERR)
        return ComboBox_GetText(hwndCombo, pszText, cchText);
    if (ComboBox_GetLBTextLen(hwndCombo, iItem) < cchText)
        return ComboBox_GetLBText(hwndCombo, iItem, pszText);
    pszText[0] = 0;
    return FALSE;
}

// コンボボックスにテキストを設定。
BOOL ComboBox_RealSetText(HWND hwndCombo, LPCWSTR pszText) noexcept
{
    const int iItem = ComboBox_FindStringExact(hwndCombo, -1, pszText);
    if (iItem == CB_ERR)
    {
        ComboBox_SetCurSel(hwndCombo, -1);
        return ComboBox_SetText(hwndCombo, pszText);
    }
    return ComboBox_SetCurSel(hwndCombo, iItem);
}
