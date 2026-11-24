// flip-board.cpp
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

MBoard __fastcall XgFlipBoardH(const MBoard& board);
MBoard __fastcall XgFlipBoardV(const MBoard& board);
MBoard __fastcall XgTransposeBoard(const MBoard& board);
void __fastcall XgFlipBoardUnitTest(void);
