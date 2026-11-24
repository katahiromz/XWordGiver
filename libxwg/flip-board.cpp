// flip-board.cpp
// Author: katahiromz
// License: MIT

#include "xwg.h"

// 盤面を水平方向に反転する。
MBoard __fastcall XgFlipBoardH(const MBoard& board) {
	const INT nCols = board.Columns(), nRows = board.Rows();
	MBoard ret(nCols, nRows, board.Rules());
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nCols; ++j) {
			WCHAR ch = board.GetCell0(j, i);
			ret.SetCell(nCols - (j + 1), i, ch);
		}
	}
	return ret;
}

// 盤面を垂直方向に反転する。
MBoard __fastcall XgFlipBoardV(const MBoard& board) {
	const INT nCols = board.Columns(), nRows = board.Rows();
	MBoard ret(nCols, nRows, board.Rules());
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nCols; ++j) {
			WCHAR ch = board.GetCell0(j, i);
			ret.SetCell(j, nRows - (i + 1), ch);
		}
	}
	return ret;
}

// 盤面を転置する。
MBoard __fastcall XgTransposeBoard(const MBoard& board) {
	const INT nCols = board.Columns(), nRows = board.Rows();
	MBoard ret(nRows, nCols, board.Rules());
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nCols; ++j) {
			WCHAR ch = board.GetCell0(j, i);
			ret.SetCell(i, j, ch); // 転置だから (i, j) でOK。
		}
	}
	return ret;
}

// 単体テスト。
void __fastcall XgFlipBoardUnitTest(void) {
	{
		MBoard b1 = XgMakeBoard({ L"#..", L"###", L"###" });
		MBoard b2 = XgMakeBoard({ L"..#", L"###", L"###" });
		assert(b1 != b2);
		b1 = XgFlipBoardH(b1);
		assert(b1 == b2);
	}
	{
		MBoard b1 = XgMakeBoard({ L"#..#", L"####", L"####" });
		MBoard b2 = XgMakeBoard({ L"####", L"####", L"#..#" });
		assert(b1 != b2);
		b1 = XgFlipBoardV(b1);
		assert(b1 == b2);
	}
	{
		MBoard b1 = XgMakeBoard({ L"#..", L"###", L"###", L"##." });
		MBoard b2 = XgMakeBoard({ L"####", L".###", L".##." });
		assert(b1 != b2);
		b1 = XgTransposeBoard(b1);
		assert(b1 == b2);
	}
}
