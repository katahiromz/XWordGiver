// resize-board.h --- Resizing MBoard
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

void __fastcall XgInsertColumns(MBoard& board, INT jColumn, INT cInsert = 1, WCHAR chFill = ZEN_SPACE);
void __fastcall XgInsertRows   (MBoard& board, INT iRow,    INT cInsert = 1, WCHAR chFill = ZEN_SPACE);
void __fastcall XgDeleteColumn(MBoard& board, INT jColumn);
void __fastcall XgDeleteRow   (MBoard& board, INT iRow);
void __fastcall XgGrowX0(MBoard& board, PINT x0, INT cCols, WCHAR chFill = ZEN_SPACE);
void __fastcall XgGrowX1(MBoard& board, PINT x0, INT cCols, WCHAR chFill = ZEN_SPACE);
void __fastcall XgGrowY0(MBoard& board, PINT y0, INT cRows, WCHAR chFill = ZEN_SPACE);
void __fastcall XgGrowY1(MBoard& board, PINT y0, INT cRows, WCHAR chFill = ZEN_SPACE);
void __fastcall XgTrimBoardX(MBoard& board, PINT x0);
void __fastcall XgTrimBoardY(MBoard& board, PINT y0);
void __fastcall XgTrimBoardXY(MBoard& board, PINT x0, PINT y0);
void __fastcall XgEnsureX(MBoard& board, INT x, PINT x0);
void __fastcall XgEnsureY(MBoard& board, INT y, PINT y0);
void __fastcall XgEnsureXY(MBoard& board, INT x, INT y, PINT x0, PINT y0);
