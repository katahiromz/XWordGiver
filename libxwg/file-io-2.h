// file-io-2.h --- File Input/Output Part 2
// Author: katahiromz
// License: MIT

#pragma once

#include "file-io.h"
#include "board-image.h"

BOOL XgLoadXDFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgLoadEcwFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgLoadCrpFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgLoadXwdFile(MBoardDisplay& display, LPCWSTR filename);

BOOL XgSaveXDFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgSaveCrpFile(MBoardDisplay& display, LPCWSTR filename);

#ifdef USE_JSON
BOOL XgLoadXwjFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgSaveXwjFile(MBoardDisplay& display, LPCWSTR filename);
#endif

BOOL XgLoadCrosswordFile(MBoardDisplay& display, LPCWSTR filename);
BOOL XgSaveCrosswordFile(MBoardDisplay& display, LPCWSTR filename);

BOOL XgCanDotExtLoad(LPCWSTR dotext);
BOOL XgCanDotExtSave(LPCWSTR dotext);

void XgUpdateRecentlyUsed(MStringListW& mru_list, LPCWSTR filename, size_t max_mru_files);
