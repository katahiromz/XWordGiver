// histogram.cpp --- Word/Pattern length histogram
// Author: katahiromz
// License: MIT

#include "histogram.h"

// ヒストグラムの包含関係を確認する。
BOOL __fastcall XgHistogramFitsIn(const MHistogram& x, const MHistogram& y) noexcept {
	for (auto& pair : x) {
		auto it = y.find(pair.first);
		if (it != y.end()) {
			if (it->second < pair.second)
				return FALSE;
		} else {
			return FALSE;
		}
	}
	return TRUE;
}

// 単語リストからヒストグラムを取得する。
MHistogram __fastcall XgHistogramFromList(const MStringListW& list) {
	MHistogram ret;
	for (auto& entry : list) {
		if (entry.size() >= 2)
			++ret[(INT)entry.size()];
	}
	return ret;
}

// 盤面からヒストグラムを取得する。
MHistogram __fastcall XgHistogramFromBoard(const MBoard& board) {
	const INT nColumns = board.Columns(), nRows = board.Rows();
	MHistogram ret;

	// 横方向
	for (INT i = 0; i < nRows; ++i) {
		INT nLen = 0;
		for (INT j = 0; j < nColumns; ++j) {
			WCHAR ch = board.GetCell(j, i, ZEN_BLACK);
			if (ch == ZEN_BLACK) {
				if (nLen > 1)
					++ret[nLen];
				nLen = 0;
			} else {
				++nLen;
			}
		}
		if (nLen > 1) // 行末で終わる連続
			++ret[nLen];
	}

	// 縦方向
	for (INT j = 0; j < nColumns; ++j) {
		INT nLen = 0;
		for (INT i = 0; i < nRows; ++i) {
			WCHAR ch = board.GetCell(j, i, ZEN_BLACK);
			if (ch == ZEN_BLACK) {
				if (nLen > 1)
					++ret[nLen];
				nLen = 0;
			} else {
				++nLen;
			}
		}
		if (nLen > 1) // 列末で終わる連続
			++ret[nLen];
	}

	return ret;
}

// 単体テスト。
void __fastcall XgHistogramUnitTest() {
	assert(XgHistogramFromList({}) == MHistogram{});

	{
		MStringListW list = { L"ねこ", L"いぬ", L"さかな", L"うさぎ", L"とり" };
		assert(XgHistogramFromList(list) == (MHistogram{ {2, 3}, {3, 2} }));
	}

	{
		MStringListW list = { L"あいうえお", L"かきくけこ", L"さしすせそ" };
		assert(XgHistogramFromList(list) == (MHistogram{ {5, 3} }));
	}

	{
		MStringListW list = { L"ねこ", L"ねこ" };
		assert(XgHistogramFromList(list) == (MHistogram{ {2, 2} }));
	}

	{
		MStringListW list = { L"あ", L"い", L"うえ" };
		assert(XgHistogramFromList(list) == (MHistogram{ {2, 1} }));
	}

	assert(XgHistogramFitsIn(MHistogram{}, MHistogram{}));
	assert(XgHistogramFitsIn(MHistogram{}, MHistogram{ {3, 5} }));

	{
		MHistogram h = { {2, 3}, {3, 2} };
		assert(XgHistogramFitsIn(h, h));
	}

	assert(XgHistogramFitsIn(MHistogram{ {5, 3} }, MHistogram{ {5, 4} }));
	assert(!XgHistogramFitsIn(MHistogram{ {5, 3} }, MHistogram{ {5, 2} }));

	assert(!XgHistogramFitsIn(MHistogram{ {4, 1} }, MHistogram{ {5, 10} }));
	assert(!XgHistogramFitsIn(MHistogram{ {4, 1} }, MHistogram{}));

	assert(XgHistogramFitsIn(MHistogram{ {3, 1} }, MHistogram{ {3, 1}, {7, 9} }));

	{
		MHistogram x = { {2, 1}, {3, 2}, {4, 1} };
		MHistogram y = { {2, 5}, {3, 1}, {4, 5} };
		assert(!XgHistogramFitsIn(x, y));
	}

	{
		MHistogram small3 = { {3, 1} };
		MHistogram big = { {3, 2} };
		assert(XgHistogramFitsIn(small3, big));
		assert(!XgHistogramFitsIn(big, small3));
	}

	{
		MBoard b = XgMakeBoard({ L".....", L".....", L".....", L".....", L"....." });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {5, 10} }));
	}

	{
		MBoard b = XgMakeBoard({ L"....", L"....", L"...." });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {4, 3}, {3, 4} }));
	}

	{
		MBoard b = XgMakeBoard({ L"###", L"###", L"###" });
		assert(XgHistogramFromBoard(b) == MHistogram{});
	}

	{
		MBoard b = XgMakeBoard({ L"#.#", L".#.", L"#.#" });
		assert(XgHistogramFromBoard(b) == MHistogram{});
	}

	{
		MBoard b = XgMakeBoard({ L"#...", L"####", L"####" });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {3, 1} }));
	}

	{
		MBoard b = XgMakeBoard({ L"###", L"#.#", L"#.#", L"#.#" });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {3, 1} }));
	}

	{
		MBoard b = XgMakeBoard({ L"#..", L"###", L"###" });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {2, 1} }));
	}

	{
		MBoard b = XgMakeBoard({ L"..#", L"###", L"###" });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {2, 1} }));
	}

	{
		MBoard b = XgMakeBoard({ L"#.#", L"#.#", L"#.#" });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {3, 1} }));
	}

	{
		MBoard b = XgMakeBoard({ L"..#...", L"######", L"######" });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {2, 1}, {3, 1} }));
	}

	{
		MBoard b = XgMakeBoard({ L"あい.", L"う.え", L".お." });
		assert(XgHistogramFromBoard(b) == (MHistogram{ {3, 6} }));
	}
}
