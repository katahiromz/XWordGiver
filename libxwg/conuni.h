// conuni.h --- Console Unicode Input/Output for C++/Win32
// Author: katahiromz
// License: MIT

#pragma once

#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <cstdio>
#include <cstdarg>
#include <string>

#ifndef MStringA
    #define MStringA std::string
#endif
#ifndef MStringW
    #define MStringW std::wstring
#endif

#ifdef UNICODE
    using MString = MStringW;
#else
    using MString = MStringA;
#endif

//////////////////////////////////////////////////////////////////
// Conversion

#define CP_SJIS 932

MStringW WideFromAnsi(const MStringA& ansi , UINT nCodePage = CP_ACP);
MStringA  AnsiFromWide(const MStringW& wide, UINT nCodePage = CP_ACP);
#define Utf8FromWide(wide) AnsiFromWide((wide), CP_UTF8)
#define SjisFromWide(wide) AnsiFromWide((wide), CP_SJIS)
#define WideFromUtf8(utf8) WideFromAnsi((utf8), CP_UTF8)
#define WideFromSjis(sjis) WideFromAnsi((sjis), CP_SJIS)

#define AnsiFromAnsi(ansi) MStringA(ansi)
#define WideFromWide(wide) MStringW(wide)
#define Utf8FromUtf8(utf8) MStringA(utf8)

// ANSI(CP_ACP) <-> UTF-8 (via Wide)
#define AnsiFromUtf8(utf8) AnsiFromWide(WideFromUtf8(utf8))
#define Utf8FromAnsi(ansi) Utf8FromWide(WideFromAnsi(ansi))

#ifdef UNICODE
    #define TextFromAnsi WideFromAnsi
    #define TextFromWide WideFromWide
    #define TextFromUtf8 WideFromUtf8
    #define AnsiFromText AnsiFromWide
    #define WideFromText WideFromWide
    #define Utf8FromText Utf8FromWide
    #define TextFromText WideFromWide
#else
    #define TextFromAnsi AnsiFromAnsi
    #define TextFromWide AnsiFromWide
    #define TextFromUtf8 AnsiFromUtf8
    #define AnsiFromText AnsiFromAnsi
    #define WideFromText WideFromAnsi
    #define Utf8FromText Utf8FromAnsi
    #define TextFromText AnsiFromAnsi
#endif

//////////////////////////////////////////////////////////////////
// Output
//
// - 出力先がコンソールなら WriteConsoleW で出力する(コードページに依存しない)。
// - リダイレクト/ファイル出力の場合は UTF-8 で出力する。

void PrintfA(LPCSTR  pszFormat, ...);
void PrintfW(LPCWSTR pszFormat, ...);

void DebugPrintfA(LPCSTR  pszFormat, ...);
void DebugPrintfW(LPCWSTR pszFormat, ...);

void FilePutStringA(LPCSTR  pszString, FILE* fout);
void FilePutStringW(LPCWSTR pszString, FILE* fout);

void FilePrintfA(FILE* fout, LPCSTR  pszFormat, ...);
void FilePrintfW(FILE* fout, LPCWSTR pszFormat, ...);

void FileVPrintfA(FILE* fout, LPCSTR  pszFormat, va_list va);
void FileVPrintfW(FILE* fout, LPCWSTR pszFormat, va_list va);

void DebugVPrintfA(LPCSTR  pszFormat, va_list va);
void DebugVPrintfW(LPCWSTR pszFormat, va_list va);

#ifdef NDEBUG
    #define DPRINTA 1 ? (void)0 : DebugPrintfA
    #define DPRINTW 1 ? (void)0 : DebugPrintfW
#else
    #define DPRINTA DebugPrintfA
    #define DPRINTW DebugPrintfW
#endif

#ifdef UNICODE
    #define Printf PrintfW
    #define DebugPrintf DebugPrintfW
    #define DebugVPrintf DebugVPrintfW
    #define FilePutString FilePutStringW
    #define FilePrintf FilePrintfW
    #define FileVPrintf FileVPrintfW
    #define DPRINT DPRINTW
#else
    #define Printf PrintfA
    #define DebugPrintf DebugPrintfA
    #define DebugVPrintf DebugVPrintfA
    #define FilePutString FilePutStringA
    #define FilePrintf FilePrintfA
    #define FileVPrintf FileVPrintfA
    #define DPRINT DPRINTA
#endif

//////////////////////////////////////////////////////////////////
// Input
//
// - 入力元がコンソールなら ReadConsoleW で読み取る。
// - リダイレクト/ファイル入力の場合は UTF-8 として読み取る。
// - 1行を読み取り、末尾の改行(\r\n, \n)は取り除く。
// - EOF(何も読めなかった場合)は false を返す。

bool FileGetLineA(MStringA&  line, FILE* fin);
bool FileGetLineW(MStringW& line, FILE* fin);

bool GetLineA(MStringA&  line);
bool GetLineW(MStringW& line);

#ifdef UNICODE
    #define FileGetLine FileGetLineW
    #define GetLine GetLineW
#else
    #define FileGetLine FileGetLineA
    #define GetLine GetLineA
#endif
