// board-text-io.h --- MBoard Text Input/Output
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

BOOL __fastcall XgTextFromBoard(MStringW& text, const MBoard& board);
BOOL __fastcall XgBoardFromText(MBoard& board, const MStringW& text);
BOOL __fastcall XgBoardFromText2(MBoard& board, const MStringW& text);
MBoard __fastcall XgGetCurrentPat(const MBoard& board);
void __fastcall XgBoardTextIOUnitTest(void);
