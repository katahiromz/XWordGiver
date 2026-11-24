// utils.h --- Utility functions
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

MStringW __fastcall xg_str_escape(const MStringW& str);
MStringW __fastcall xg_str_unescape(const MStringW& str);
MStringW __fastcall xg_str_quote(const MStringW& str);
MStringW __fastcall xg_str_unquote(const MStringW& str);

LPWSTR __fastcall XgMakeFilterString(LPWSTR psz) noexcept;
BOOL __fastcall XgGetPathOfShortcutW(LPCWSTR pszLnkFile, LPWSTR pszPath);
void __fastcall XgOpenLocalFile(HWND hwnd, LPCWSTR pszFileName) noexcept;

MStringA XgMakeClipHtmlData(const MStringA& html_utf8,
                            const MStringA& style_utf8 = "");
MStringA XgMakeClipHtmlData(const MStringW& html_wide,
                            const MStringW& style_wide = L"");
HGLOBAL XgCreateTextDataA(const MStringA& str);
HGLOBAL XgCreateTextDataW(const MStringW& str);

MStringW XgGetClipboardUnicodeText(HWND hwnd);
BOOL XgSetClipboardUnicodeText(HWND hwnd, const MStringW& str) noexcept;
