// resize-board.cpp --- Resizing MBoard
// Author: katahiromz
// License: MIT

#include "resize-board.h"

// 列を挿入する。
void __fastcall XgInsertColumns(MBoard& board, INT jColumn, INT cInsert, WCHAR chFill) {
	if (cInsert <= 0)
		return;
	const INT nColumns = board.Columns(), nRows = board.Rows();
	assert(0 <= jColumn && jColumn <= nColumns);

	MBoard new_board(nColumns + cInsert, nRows, board.Rules(), chFill);
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nColumns; ++j) {
			WCHAR ch = board.GetCell0(j, i);
			if (j >= jColumn) {
				new_board.SetCell(j + cInsert, i, ch);
			} else {
				new_board.SetCell(j, i, ch);
			}
		}
	}

	board = std::move(new_board);
}

// 行を挿入する。
void __fastcall XgInsertRows(MBoard& board, INT iRow, INT cInsert, WCHAR chFill) {
	if (cInsert <= 0)
		return;
	const INT nColumns = board.Columns(), nRows = board.Rows();
	assert(0 <= iRow && iRow <= nRows);

	MBoard new_board(nColumns, nRows + cInsert, board.Rules(), chFill);
	for (INT j = 0; j < nColumns; ++j) {
		for (INT i = 0; i < nRows; ++i) {
			WCHAR ch = board.GetCell0(j, i);
			if (i >= iRow) {
				new_board.SetCell(j, i + cInsert, ch);
			} else {
				new_board.SetCell(j, i, ch);
			}
		}
	}

	board = std::move(new_board);
}

// 左側にcCols個の列を挿入する。
void __fastcall XgGrowX0(MBoard& board, PINT x0, INT cCols, WCHAR chFill) {
	assert(cCols > 0);
	XgInsertColumns(board, 0, cCols, chFill);
	*x0 -= cCols;
}

// 右側にcCols個の列を挿入する。
void __fastcall XgGrowX1(MBoard& board, PINT x0, INT cCols, WCHAR chFill) {
	assert(cCols > 0);
	XgInsertColumns(board, board.Columns(), cCols, chFill);
}

// 上側にcRows個の列を挿入する。
void __fastcall XgGrowY0(MBoard& board, PINT y0, INT cRows, WCHAR chFill) {
	assert(cRows > 0);
	XgInsertRows(board, 0, cRows, chFill);
	*y0 -= cRows;
}

// 下側にcRows個の列を挿入する。
void __fastcall XgGrowY1(MBoard& board, PINT y0, INT cRows, WCHAR chFill) {
	assert(cRows > 0);
	XgInsertRows(board, board.Rows(), cRows, chFill);
}

// 列を削除する。
void __fastcall XgDeleteColumn(MBoard& board, INT jColumn) {
	const INT cx = board.Columns(), cy = board.Rows();
	assert(0 <= jColumn && jColumn < cx);

	MBoard new_board(cx - 1, cy, board.Rules());
	for (INT y = 0; y < cy; ++y) {
		for (INT x = 0; x < cx - 1; ++x) {
			if (x < jColumn)
				new_board.SetCell(x, y, board.GetCell0(x, y));
			else
				new_board.SetCell(x, y, board.GetCell0(x + 1, y));
		}
	}

	board = std::move(new_board);
}

// 行を削除する。
void __fastcall XgDeleteRow(MBoard& board, INT iRow) {
	const INT cx = board.Columns(), cy = board.Rows();
	assert(0 <= iRow && iRow < cy);

	MBoard new_board(cx, cy - 1, board.Rules());
	for (INT x = 0; x < cx; ++x) {
		for (INT y = 0; y < cy; ++y) {
			if (y < iRow)
				new_board.SetCell(x, y, board.GetCell0(x, y));
			else
				new_board.SetCell(x, y, board.GetCell0(x, y + 1));
		}
	}

	board = std::move(new_board);
}

// 文字マスの範囲を求める。文字マスがなければ false。
static bool XgFindLetterBounds(const MBoard& board, INT& x0, INT& y0, INT& x1, INT& y1) {
	const INT cx = board.Columns(), cy = board.Rows();
	x0 = cx; y0 = cy; x1 = y1 = -1;
	for (INT y = 0; y < cy; ++y) {
		for (INT x = 0; x < cx; ++x) {
			if (XgIsLetter(board.GetCell0(x, y))) {
				x0 = __min(x0, x); x1 = __max(x1, x);
				y0 = __min(y0, y); y1 = __max(y1, y);
			}
		}
	}
	return x1 >= 0;
}

// (left, top) から w x h の範囲だけを残す。
static void XgCrop(MBoard& board, INT left, INT top, INT w, INT h) {
	MBoard new_board(w, h, board.Rules());
	for (INT y = 0; y < h; ++y)
		for (INT x = 0; x < w; ++x)
			new_board.SetCell(x, y, board.GetCell0(left + x, top + y));
	board = std::move(new_board);
}


// 文字マスが見つからない列を左右端からカットする。
void __fastcall XgTrimBoardX(MBoard& board, PINT x0) {
	INT l, t, r, b;
	if (!XgFindLetterBounds(board, l, t, r, b))
		return; // 文字がなければ何もしない。
	const INT cx = board.Columns();
	const INT w = __max(r - l + 1, XG_MIN_SIZE);
	const INT left = __min(l, cx - w); // 最小サイズに満たなければ隣のマスを取り込む。
	XgCrop(board, left, 0, w, board.Rows());
	*x0 += left;
}

// 文字マスが見つからない行を上下端からカットする。
void __fastcall XgTrimBoardY(MBoard& board, PINT y0) {
	INT l, t, r, b;
	if (!XgFindLetterBounds(board, l, t, r, b))
		return;
	const INT cy = board.Rows();
	const INT h = __max(b - t + 1, XG_MIN_SIZE);
	const INT top = __min(t, cy - h);
	XgCrop(board, 0, top, board.Columns(), h);
	*y0 += top;
}

// 文字マスが見つからない行と列を端からカットする。
void __fastcall XgTrimBoardXY(MBoard& board, PINT x0, PINT y0) {
	XgTrimBoardY(board, y0);
	XgTrimBoardX(board, x0);
}

// 座標が収まるように拡張する。
void __fastcall XgEnsureX(MBoard& board, INT x, PINT x0) {
	if (x < *x0) {
		XgGrowX0(board, x0, *x0 - x);
		return;
	}
	const INT cx = board.Columns();
	if (cx + *x0 <= x) {
		XgGrowX1(board, x0, x - (cx + *x0) + 1);
	}
}

// 座標が収まるように拡張する。
void __fastcall XgEnsureY(MBoard& board, INT y, PINT y0) {
	if (y < *y0) {
		XgGrowY0(board, y0, *y0 - y);
		return;
	}
	const INT cy = board.Rows();
	if (cy + *y0 <= y) {
		XgGrowY1(board, y0, y - (cy + *y0) + 1);
	}
}

// 座標が収まるように拡張する。
void __fastcall XgEnsureXY(MBoard& board, INT x, INT y, PINT x0, PINT y0) {
	XgEnsureX(board, x, x0);
	XgEnsureY(board, y, y0);
}
