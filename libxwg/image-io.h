// image-io.h --- 画像ファイルの読み書き。
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include "SaveBitmapToFile.h"

HBITMAP __fastcall XgCreate24BppBitmap(HDC hDC, LONG width, LONG height) noexcept;
BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm);
BOOL __fastcall XgLoadImage(const MStringW& filename, HBITMAP& hbm, HENHMETAFILE& hEMF) noexcept;
HBITMAP __fastcall XgCreateGrayedBitmap(HBITMAP hbm, COLORREF crMask = CLR_INVALID) noexcept;
