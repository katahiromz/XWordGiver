// conuni.cpp --- Console Unicode Input/Output for C++/Win32
// Author: katahiromz
// License: MIT

#ifndef _CRT_SECURE_NO_WARNINGS
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include "xwg.h"
#include "conuni.h"
#include <io.h>
#include <atomic>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cwchar>

//////////////////////////////////////////////////////////////////
// Internal helpers

// スタック上の小さなバッファ。足りなければヒープを使う。
// 短い文字列(大半のケース)ではヒープ確保が一切発生しない。
template <typename T, size_t N>
class TempBuffer {
public:
    TempBuffer() : m_heap(nullptr) { }
    ~TempBuffer() { delete[] m_heap; }
    TempBuffer(const TempBuffer&) = delete;
    TempBuffer& operator=(const TempBuffer&) = delete;

    T* stack() { return m_stack; }
    T* alloc(size_t n) { // n 要素分の領域を返す
        if (n <= N)
            return m_stack;
        delete[] m_heap;
        m_heap = new T[n];
        return m_heap;
    }

private:
    T m_stack[N];
    T* m_heap;
};

static const size_t kBufChars = 1024; // スタックバッファの要素数

// ASCII だけのバイト列か? (8バイトずつ判定)
static bool IsAsciiA(const char* p, size_t n) {
    while (n >= 8) {
        uint64_t v;
        memcpy(&v, p, 8);
        if (v & 0x8080808080808080ULL)
            return false;
        p += 8;
        n -= 8;
    }
    while (n-- > 0) {
        if (static_cast<unsigned char>(*p++) & 0x80)
            return false;
    }
    return true;
}

// ASCII だけの UTF-16 文字列か? (4文字ずつ判定)
static bool IsAsciiW(const wchar_t* p, size_t n) {
    while (n >= 4) {
        uint64_t v;
        memcpy(&v, p, 8);
        if (v & 0xFF80FF80FF80FF80ULL)
            return false;
        p += 4;
        n -= 4;
    }
    while (n-- > 0) {
        if (static_cast<unsigned>(*p++) & 0xFF80)
            return false;
    }
    return true;
}

// ASCII(0x00-0x7F)がそのまま同じ値に変換されるコードページか?
static bool IsAsciiCompatibleCP(UINT nCodePage) {
    return nCodePage == CP_ACP || nCodePage == CP_OEMCP ||
           nCodePage == CP_UTF8 || nCodePage == CP_SJIS;
}

// マルチバイト → UTF-16。戻り値は文字数(失敗/空は 0)。
// 変換後の文字数は入力のバイト数以下なので、まず1回の API 呼び出しで済ませる。
template <size_t N>
static int MBToWide(UINT nCodePage, const char* p, size_t n,
                    TempBuffer<wchar_t, N>& tb, const wchar_t*& out)
{
    if (n == 0 || n > INT_MAX)
        return 0;

    wchar_t* d = tb.alloc(n);

    if (IsAsciiCompatibleCP(nCodePage) && IsAsciiA(p, n)) { // ASCII 高速パス
        for (size_t i = 0; i < n; ++i)
            d[i] = static_cast<unsigned char>(p[i]);
        out = d;
        return static_cast<int>(n);
    }

    int cch = ::MultiByteToWideChar(nCodePage, 0, p, static_cast<int>(n),
                                    d, static_cast<int>(n));
    if (cch <= 0) {
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return 0;
        // 念のため従来の2回呼び出し方式にフォールバック
        cch = ::MultiByteToWideChar(nCodePage, 0, p, static_cast<int>(n), nullptr, 0);
        if (cch <= 0)
            return 0;
        d = tb.alloc(static_cast<size_t>(cch));
        cch = ::MultiByteToWideChar(nCodePage, 0, p, static_cast<int>(n), d, cch);
        if (cch <= 0)
            return 0;
    }
    out = d;
    return cch;
}

// UTF-16 → マルチバイト。戻り値はバイト数(失敗/空は 0)。
template <size_t N>
static int WideToMB(UINT nCodePage, const wchar_t* p, size_t n,
                    TempBuffer<char, N>& tb, const char*& out)
{
    if (n == 0 || n > INT_MAX)
        return 0;

    if (IsAsciiCompatibleCP(nCodePage) && IsAsciiW(p, n)) { // ASCII 高速パス
        char* d = tb.alloc(n);
        for (size_t i = 0; i < n; ++i)
            d[i] = static_cast<char>(p[i]);
        out = d;
        return static_cast<int>(n);
    }

    // 変換後は最低でも n バイトになるので、n > N なら最初からヒープで確保する
    if (n <= N) {
        char* d = tb.stack();
        int cb = ::WideCharToMultiByte(nCodePage, 0, p, static_cast<int>(n),
                                       d, static_cast<int>(N), nullptr, nullptr);
        if (cb > 0) {
            out = d;
            return cb;
        }
        if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return 0;
    }

    int cb = ::WideCharToMultiByte(nCodePage, 0, p, static_cast<int>(n),
                                   nullptr, 0, nullptr, nullptr);
    if (cb <= 0)
        return 0;
    char* d = tb.alloc(static_cast<size_t>(cb));
    cb = ::WideCharToMultiByte(nCodePage, 0, p, static_cast<int>(n),
                               d, cb, nullptr, nullptr);
    if (cb <= 0)
        return 0;
    out = d;
    return cb;
}

// FILE* がコンソールに接続されていれば、そのハンドルを返す。
// (_isatty は NUL デバイスも true にするため、GetConsoleMode で判定する)
//
// GetConsoleMode はシステムコールなので、stdin/stdout/stderr についてはハンドル値と
// 判定結果をキャッシュする。ハンドル値が変われば(リダイレクト等)再判定する。
// 上位32ビット: 0=未判定, 1=コンソールではない, 2=コンソール / 下位32ビット: ハンドル値
// (Windows のハンドルは64ビット環境でも32ビットに収まることが保証されている)
static std::atomic<uint64_t> g_consoleCache[3];

static bool GetConsoleHandleOf(FILE* fp, HANDLE& hConsole) {
    if (!fp)
        return false;

    const int no = _fileno(fp);
    if (no < 0)
        return false;

    const intptr_t h = _get_osfhandle(no);
    if (h == -1 || h == -2)
        return false;

    const HANDLE hFile = reinterpret_cast<HANDLE>(h);
    const uint32_t key = static_cast<uint32_t>(static_cast<uintptr_t>(h));

    if (no < 3) {
        const uint64_t v = g_consoleCache[no].load(std::memory_order_relaxed);
        const uint32_t state = static_cast<uint32_t>(v >> 32);
        if (state != 0 && static_cast<uint32_t>(v) == key) {
            if (state == 2) {
                hConsole = hFile;
                return true;
            }
            return false;
        }
    }

    DWORD dwMode;
    const bool bConsole = (::GetConsoleMode(hFile, &dwMode) != 0);
    if (no < 3) {
        const uint64_t state = bConsole ? 2 : 1;
        g_consoleCache[no].store((state << 32) | key, std::memory_order_relaxed);
    }

    if (bConsole)
        hConsole = hFile;
    return bConsole;
}

static void WriteConsoleChunks(HANDLE hConsole, const wchar_t* p, size_t len) {
    const size_t kChunk = 8192;
    while (len > 0) {
        size_t n = (len > kChunk) ? kChunk : len;
        // サロゲートペアを分断しない
        if (n < len && p[n - 1] >= 0xD800 && p[n - 1] <= 0xDBFF)
            --n;
        DWORD cchWritten = 0;
        if (!::WriteConsoleW(hConsole, p, static_cast<DWORD>(n), &cchWritten, nullptr) ||
            cchWritten == 0)
        {
            break;
        }
        p += cchWritten;
        len -= cchWritten;
    }
}

// UTF-16 を UTF-8 としてファイルに出力する。
static void PutWideToFile(const wchar_t* p, size_t len, FILE* fout) {
    TempBuffer<char, kBufChars> tb;
    const char* q = nullptr;
    int cb = WideToMB(CP_UTF8, p, len, tb, q);
    if (cb > 0)
        fwrite(q, 1, static_cast<size_t>(cb), fout);
}

// ワイド文字列を出力する。コンソールなら WriteConsoleW、それ以外は UTF-8。
static void PutWide(const wchar_t* p, size_t len, FILE* fout) {
    if (!fout || len == 0)
        return;

    HANDLE hConsole;
    if (GetConsoleHandleOf(fout, hConsole)) {
        fflush(fout); // stdio のバッファとの順序を保つ
        WriteConsoleChunks(hConsole, p, len);
    } else {
        PutWideToFile(p, len, fout);
    }
}

// ANSI 文字列を出力する。コンソールなら WriteConsoleW、それ以外は UTF-8。
static void PutAnsi(const char* p, size_t len, FILE* fout) {
    if (!fout || len == 0)
        return;

    HANDLE hConsole;
    if (GetConsoleHandleOf(fout, hConsole)) {
        TempBuffer<wchar_t, kBufChars> tb;
        const wchar_t* w = nullptr;
        int cch = MBToWide(CP_ACP, p, len, tb, w);
        if (cch > 0) {
            fflush(fout); // stdio のバッファとの順序を保つ
            WriteConsoleChunks(hConsole, w, static_cast<size_t>(cch));
        }
        return;
    }

    if (IsAsciiA(p, len)) { // ASCII は UTF-8 と同一なので、変換せずそのまま出力
        fwrite(p, 1, len, fout);
        return;
    }

    TempBuffer<wchar_t, kBufChars> tb;
    const wchar_t* w = nullptr;
    int cch = MBToWide(CP_ACP, p, len, tb, w);
    if (cch > 0)
        PutWideToFile(w, static_cast<size_t>(cch), fout);
}

// 書式化。まずスタックバッファに試し、収まらなければヒープに確保し直す。
// 戻り値は文字数(失敗時は -1)。out は NUL 終端されている。
static int FormatV(TempBuffer<char, kBufChars>& tb, LPCSTR pszFormat, va_list va,
                   const char*& out)
{
    char* buf = tb.stack();
    va_list va2;
    va_copy(va2, va);
    int len = _vsnprintf(buf, kBufChars - 1, pszFormat, va2);
    va_end(va2);
    if (len >= 0 && len < static_cast<int>(kBufChars) - 1) {
        out = buf;
        return len;
    }

    va_copy(va2, va);
    len = _vscprintf(pszFormat, va2);
    va_end(va2);
    if (len < 0)
        return -1;

    buf = tb.alloc(static_cast<size_t>(len) + 1);
    len = _vsnprintf(buf, static_cast<size_t>(len) + 1, pszFormat, va);
    if (len < 0)
        return -1;
    out = buf;
    return len;
}

static int FormatV(TempBuffer<wchar_t, kBufChars>& tb, LPCWSTR pszFormat, va_list va,
                   const wchar_t*& out)
{
    wchar_t* buf = tb.stack();
    va_list va2;
    va_copy(va2, va);
    int len = _vsnwprintf(buf, kBufChars - 1, pszFormat, va2);
    va_end(va2);
    if (len >= 0 && len < static_cast<int>(kBufChars) - 1) {
        out = buf;
        return len;
    }

    va_copy(va2, va);
    len = _vscwprintf(pszFormat, va2);
    va_end(va2);
    if (len < 0)
        return -1;

    buf = tb.alloc(static_cast<size_t>(len) + 1);
    len = _vsnwprintf(buf, static_cast<size_t>(len) + 1, pszFormat, va);
    if (len < 0)
        return -1;
    out = buf;
    return len;
}

//////////////////////////////////////////////////////////////////
// Conversion

MStringW WideFromAnsi(const MStringA& ansi, UINT nCodePage) {
    TempBuffer<wchar_t, kBufChars> tb;
    const wchar_t* p = nullptr;
    int cch = MBToWide(nCodePage, ansi.data(), ansi.size(), tb, p);
    if (cch <= 0)
        return MStringW();
    return MStringW(p, static_cast<size_t>(cch));
}

MStringA AnsiFromWide(const MStringW& wide, UINT nCodePage) {
    TempBuffer<char, kBufChars> tb;
    const char* p = nullptr;
    int cb = WideToMB(nCodePage, wide.data(), wide.size(), tb, p);
    if (cb <= 0)
        return MStringA();
    return MStringA(p, static_cast<size_t>(cb));
}

//////////////////////////////////////////////////////////////////
// Output

void FilePutStringA(LPCSTR pszString, FILE* fout) {
    if (!pszString)
        return;
    PutAnsi(pszString, strlen(pszString), fout);
}

void FilePutStringW(LPCWSTR pszString, FILE* fout) {
    if (!pszString)
        return;
    PutWide(pszString, wcslen(pszString), fout);
}

void FileVPrintfA(FILE* fout, LPCSTR pszFormat, va_list va) {
    if (!fout || !pszFormat)
        return;
    TempBuffer<char, kBufChars> tb;
    const char* s = nullptr;
    int len = FormatV(tb, pszFormat, va, s);
    if (len > 0)
        PutAnsi(s, static_cast<size_t>(len), fout);
}

void FileVPrintfW(FILE* fout, LPCWSTR pszFormat, va_list va) {
    if (!fout || !pszFormat)
        return;
    TempBuffer<wchar_t, kBufChars> tb;
    const wchar_t* s = nullptr;
    int len = FormatV(tb, pszFormat, va, s);
    if (len > 0)
        PutWide(s, static_cast<size_t>(len), fout);
}

void FilePrintfA(FILE* fout, LPCSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfA(fout, pszFormat, va);
    va_end(va);
}

void FilePrintfW(FILE* fout, LPCWSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfW(fout, pszFormat, va);
    va_end(va);
}

void PrintfA(LPCSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfA(stdout, pszFormat, va);
    va_end(va);
}

void PrintfW(LPCWSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    FileVPrintfW(stdout, pszFormat, va);
    va_end(va);
}

void DebugVPrintfA(LPCSTR pszFormat, va_list va) {
    if (!pszFormat)
        return;
    TempBuffer<char, kBufChars> tb;
    const char* s = nullptr;
    if (FormatV(tb, pszFormat, va, s) >= 0)
        ::OutputDebugStringA(s);
}

void DebugVPrintfW(LPCWSTR pszFormat, va_list va) {
    if (!pszFormat)
        return;
    TempBuffer<wchar_t, kBufChars> tb;
    const wchar_t* s = nullptr;
    if (FormatV(tb, pszFormat, va, s) >= 0)
        ::OutputDebugStringW(s);
}

void DebugPrintfA(LPCSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    DebugVPrintfA(pszFormat, va);
    va_end(va);
}

void DebugPrintfW(LPCWSTR pszFormat, ...) {
    va_list va;
    va_start(va, pszFormat);
    DebugVPrintfW(pszFormat, va);
    va_end(va);
}

//////////////////////////////////////////////////////////////////
// Input

bool FileGetLineW(MStringW& line, FILE* fin) {
    line.clear();
    if (!fin)
        return false;

    HANDLE hConsole;
    if (GetConsoleHandleOf(fin, hConsole)) { // コンソール入力
        bool bGot = false;
        WCHAR szBuf[1024];
        for (;;) {
            DWORD cchRead = 0;
            if (!::ReadConsoleW(hConsole, szBuf, _countof(szBuf), &cchRead, nullptr) ||
                cchRead == 0)
            {
                break;
            }
            bGot = true;
            line.append(szBuf, cchRead);
            if (line.back() == L'\n')
                break;
        }
        if (!bGot)
            return false;

        if (line[0] == 0x1A) { // Ctrl+Z (EOF)
            line.clear();
            return false;
        }
        while (!line.empty() && (line.back() == L'\n' || line.back() == L'\r'))
            line.pop_back();
        return true;
    }

    // リダイレクト/ファイル入力 (UTF-8)
    // 大半の行は1回の fgets で読み切れるので、追加のバッファへのコピーを避ける。
    char szBuf[4096];
    if (!fgets(szBuf, sizeof(szBuf), fin))
        return false;

    const char* p = szBuf;
    size_t n = strlen(szBuf);

    MStringA longLine; // 行が長い場合のみ使用
    if (n == sizeof(szBuf) - 1 && szBuf[n - 1] != '\n') {
        longLine.assign(szBuf, n);
        while (fgets(szBuf, sizeof(szBuf), fin)) {
            const size_t m = strlen(szBuf);
            longLine.append(szBuf, m);
            if (m > 0 && szBuf[m - 1] == '\n')
                break;
        }
        p = longLine.data();
        n = longLine.size();
    }

    while (n > 0 && (p[n - 1] == '\n' || p[n - 1] == '\r'))
        --n;

    if (n >= 3 && memcmp(p, "\xEF\xBB\xBF", 3) == 0) { // UTF-8 BOM
        p += 3;
        n -= 3;
    }

    TempBuffer<wchar_t, kBufChars> tb;
    const wchar_t* w = nullptr;
    int cch = MBToWide(CP_UTF8, p, n, tb, w);
    if (cch > 0)
        line.assign(w, static_cast<size_t>(cch));
    return true;
}

bool FileGetLineA(MStringA& line, FILE* fin) {
    MStringW wide;
    bool ret = FileGetLineW(wide, fin);
    line = AnsiFromWide(wide);
    return ret;
}

bool GetLineA(MStringA& line) {
    return FileGetLineA(line, stdin);
}

bool GetLineW(MStringW& line) {
    return FileGetLineW(line, stdin);
}
