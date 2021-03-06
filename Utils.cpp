//////////////////////////////////////////////////////////////////////////////
// Utils.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx1(int id)
{
    static WCHAR sz[256];
    INT ret = LoadStringW(xg_hInstance, id, sz, ARRAYSIZE(sz));
    assert(ret != 0);
    UNREFERENCED_PARAMETER(ret);
    return sz;
}

// リソース文字列を読み込む。
LPWSTR __fastcall XgLoadStringDx2(int id)
{
    static WCHAR sz[256];
    INT ret = LoadStringW(xg_hInstance, id, sz, ARRAYSIZE(sz));
    assert(ret != 0);
    UNREFERENCED_PARAMETER(ret);
    return sz;
}

// フィルター文字列を作る。
LPWSTR __fastcall XgMakeFilterString(LPWSTR psz)
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

// 文字列の前後の空白を取り除く。
void __fastcall xg_str_trim(std::wstring& str)
{
    static const LPCWSTR s_white_space = L" \t\r\n\x3000";
    const size_t i = str.find_first_not_of(s_white_space);
    const size_t j = str.find_last_not_of(s_white_space);
    if (i != std::wstring::npos && j != std::wstring::npos)
        str = str.substr(i, j - i + 1);
    else if (i != std::wstring::npos)
        str = str.substr(i);
    else if (j != std::wstring::npos)
        str = str.substr(0, j);
    else
        str.clear();
}

// 文字列を置換する。
void __fastcall xg_str_replace_all(
    std::wstring &s, const std::wstring& from, const std::wstring& to)
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
    const INT cch = MultiByteToWideChar(CP_UTF8, 0, ansi.data(), -1, nullptr, 0);
    if (cch == 0)
        return L"";

    // 変換先のバッファを確保する。
    std::wstring uni(cch - 1, 0);

    // 変換して格納する。
    MultiByteToWideChar(CP_UTF8, 0, ansi.data(), -1, &uni[0], cch);
    return uni;
}

// ダイアログを中央によせる関数。
void __fastcall XgCenterDialog(HWND hwnd)
{
    // 子ウィンドウか？
    bool bChild = !!(::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD);

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
        ::GetClassNameW(hwnd, szClassName, ARRAYSIZE(szClassName));
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
XgCenterMessageBoxW(HWND hwnd, LPCWSTR pszText, LPCWSTR pszCaption, UINT uType)
{
    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    // フックのためにウィンドウのインスタンスを取得する。
    HINSTANCE hInst =
        reinterpret_cast<HINSTANCE>(::GetWindowLongPtr(hwnd, GWLP_HINSTANCE));

    // フックを開始。
    DWORD dwThreadID = ::GetCurrentThreadId();
    s_hMsgBoxHook = ::SetWindowsHookEx(WH_CBT, XgMsgBoxCbtProc, hInst, dwThreadID);

    // メッセージボックスを表示する。
    int nID = MessageBoxW(hwnd, pszText, pszCaption, uType);

    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    return nID;     // メッセージボックスの戻り値。
}

// 中央寄せメッセージボックスを表示する。
int __fastcall
XgCenterMessageBoxIndirectW(LPMSGBOXPARAMS lpMsgBoxParams)
{
    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    // フックのためにウィンドウのインスタンスを取得する。
    HINSTANCE hInst =
        reinterpret_cast<HINSTANCE>(
            ::GetWindowLongPtr(lpMsgBoxParams->hwndOwner, GWLP_HINSTANCE));

    // フックを開始。
    DWORD dwThreadID = ::GetCurrentThreadId();
    s_hMsgBoxHook = ::SetWindowsHookEx(WH_CBT, XgMsgBoxCbtProc, hInst, dwThreadID);

    // メッセージボックスを表示する。
    int nID = MessageBoxIndirectW(lpMsgBoxParams);

    // フックされていたらフックを解除する。
    if (s_hMsgBoxHook != nullptr && UnhookWindowsHookEx(s_hMsgBoxHook))
        s_hMsgBoxHook = nullptr;

    return nID;     // メッセージボックスの戻り値。
}

// ReadMeを開く。
void __fastcall XgOpenReadMe(HWND hwnd)
{
    // 実行ファイルのパスを取得。
    WCHAR szPath[MAX_PATH];
    ::GetModuleFileNameW(nullptr, szPath, ARRAYSIZE(szPath));

    // ReadMeへのパスを作成。
    PathRemoveFileSpec(szPath);
    PathAppend(szPath, XgLoadStringDx1(IDS_README));

    // ReadMeを開く。
    ShellExecuteW(hwnd, nullptr, szPath, nullptr, nullptr, SW_SHOWNORMAL);
}

// Licenseを開く。
void __fastcall XgOpenLicense(HWND hwnd)
{
    // 実行ファイルのパスを取得。
    WCHAR szPath[MAX_PATH];
    ::GetModuleFileNameW(nullptr, szPath, ARRAYSIZE(szPath));

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
    static const int s_anFolders[] = {
        CSIDL_PROGRAM_FILES,
        CSIDL_PROGRAM_FILES_COMMON,
        CSIDL_PROGRAM_FILES_COMMONX86,
        CSIDL_PROGRAM_FILESX86,
        CSIDL_SYSTEM,
        CSIDL_SYSTEMX86,
        CSIDL_WINDOWS
    };

    // 与えられたパスファイル名。
    std::wstring str(pszFile);

    for (size_t i = 0; i < ARRAYSIZE(s_anFolders); ++i) {
        // 特殊フォルダの位置の取得。
        LPITEMIDLIST pidl = NULL;
        if (SUCCEEDED(::SHGetSpecialFolderLocation(
            xg_hMainWnd, s_anFolders[i], &pidl)))
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
    const INT len = ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, NULL, 0, NULL, NULL);
    if (len == 0)
        return "";

    std::string utf8(len - 1, 0);
    ::WideCharToMultiByte(CP_UTF8, 0, wide.data(), -1, &utf8[0], len, NULL, NULL);
    return utf8;
}

// ANSI -> Unicode
std::wstring XgAnsiToUnicode(const std::string& ansi)
{
    const INT len = ::MultiByteToWideChar(SJIS_CODEPAGE, 0, ansi.data(), -1, NULL, 0);
    if (len == 0)
        return L"";

    std::wstring uni(len - 1, 0);
    ::MultiByteToWideChar(SJIS_CODEPAGE, 0, ansi.data(), -1, &uni[0], len);
    return uni;
}

// Unicode -> ANSI
std::string XgUnicodeToAnsi(const std::wstring& wide)
{
    int len = ::WideCharToMultiByte(SJIS_CODEPAGE, 0, wide.data(), -1, NULL, 0, NULL, NULL);
    if (len == 0)
        return "";

    std::string ansi(len - 1, 0);
    ::WideCharToMultiByte(SJIS_CODEPAGE, 0, wide.data(), -1, &ansi[0], len, NULL, NULL);
    return ansi;
}

// JSON文字列を作る。
std::wstring XgJsonEncodeString(const std::wstring& str)
{
    std::wstring encoded;
    wchar_t buf[16];
    size_t i, siz = str.size();

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
            if (0 <= str[i] && str[i] <= L'\x1F') {
                StringCbPrintf(buf, sizeof(buf), L"\\u%04X", str[i]);
                encoded += buf;
            } else {
                encoded += str[i];
            }
        }
    }
    return encoded;
}

// 16進で表す。
char XgToHex(char code)
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
    StringCbCopy(szPath, sizeof(szPath), pszPath);

    DWORD attrs = ::GetFileAttributesW(szPath);
    if (attrs != 0xFFFFFFFF) {
        return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
    }

    LPWSTR pch = wcsrchr(szPath, L'\\');
    if (pch == NULL) {
        return TRUE;
    }
    *pch = 0;

    if (XgMakePathW(szPath)) {
        return CreateDirectoryW(pszPath, NULL);
    }
    return FALSE;
}

//////////////////////////////////////////////////////////////////////////////

// エンディアン変換。
void XgSwab(LPBYTE pbFile, DWORD cbFile)
{
    LPWORD pw = reinterpret_cast<LPWORD>(pbFile);
    DWORD cw = (cbFile >> 1);
    while (cw--) {
        WORD w = *pw;
        BYTE lo = LOBYTE(w);
        BYTE hi = HIBYTE(w);
        *pw = MAKEWORD(hi, lo);
        ++pw;
    }
}

//////////////////////////////////////////////////////////////////////////////

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(
    const std::string& html_utf8, const std::string& style_utf8/* = ""*/)
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

    size_t iHtmlStart = str.find("<html>");
    size_t iHtmlEnd = str.size();
    size_t iFragmentStart = str.find("<!-- StartFragment -->");
    size_t iFragmentEnd = str.find("<!-- EndFragment -->");

    char buf[9];
    size_t i;
    i = str.find("StartHTML:");
    i += 10;
    StringCbPrintfA(buf, sizeof(buf), "%08u", static_cast<UINT>(iHtmlStart));
    str.replace(i, 8, buf);

    i = str.find("EndHTML:");
    i += 8;
    StringCbPrintfA(buf, sizeof(buf), "%08u", static_cast<UINT>(iHtmlEnd));
    str.replace(i, 8, buf);

    i = str.find("StartFragment:");
    i += 14;
    StringCbPrintfA(buf, sizeof(buf), "%08u", static_cast<UINT>(iFragmentStart));
    str.replace(i, 8, buf);

    i = str.find("EndFragment:");
    i += 12;
    StringCbPrintfA(buf, sizeof(buf), "%08u", static_cast<UINT>(iFragmentEnd));
    str.replace(i, 8, buf);

    return str;
}

// HTML形式のクリップボードデータを作成する。
std::string XgMakeClipHtmlData(
    const std::wstring& html_wide, const std::wstring& style_wide/* = L""*/)
{
    return XgMakeClipHtmlData(
        XgUnicodeToUtf8(html_wide), XgUnicodeToUtf8(style_wide));
}

//////////////////////////////////////////////////////////////////////////////
