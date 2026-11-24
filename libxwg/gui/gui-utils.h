// gui-utils.h
// Author: katahiromz
// License: MIT

#pragma once

#include "board-image.h"

HMENU __fastcall XgLoadPopupMenu(HWND hwnd, INT nPos) noexcept;
MStringW __fastcall XgUrlEncode(const MStringW& url);
void __fastcall XgDoWebSearch(HWND hwnd, LPCWSTR str);
BOOL XgCopyBoard(HWND hwnd, MBoardDisplay& display, BOOL bText, BOOL bDIB, BOOL bEMF);
BOOL XgGetComboBoxText(HWND hwndCombo, LPWSTR text, INT length);
