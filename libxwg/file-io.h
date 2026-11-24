// file-io.h --- File Input/Output
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

BOOL __fastcall XgIsTextFile(LPCWSTR pszFileName) noexcept;
BOOL __fastcall XgIsImageFile(LPCWSTR pszFileName) noexcept;
BOOL __fastcall XgCanWriteFile(HWND hwnd, LPCWSTR pszFile) noexcept;
BOOL __fastcall XgMakePathW(LPCWSTR pszPath) noexcept;
void __fastcall XgSwab(LPBYTE pbFile, size_t cbFile) noexcept;

BOOL __fastcall XgReadFileAll(LPCWSTR file, MStringA& strBinary);
BOOL __fastcall XgWriteFileAll(LPCWSTR file, const MStringA& strBinary) noexcept;
BOOL __fastcall XgReadTextFileAll(LPCWSTR file, MStringW& strText, BOOL ecw = FALSE);
