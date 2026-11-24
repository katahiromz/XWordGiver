// image-io.h --- 画像ファイルの読み書き。
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

BOOL XgSavePictureToFile(const MStringW& filePath, HBITMAP hBitmap, ULONG jpegQuality = 90);
