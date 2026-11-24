// board-text-io.cpp --- MBoard Text Input/Output
// Author: katahiromz
// License: MIT

#include "board-text-io.h"
#include <vector>

// 盤面をテキストに出力する。
BOOL __fastcall XgTextFromBoard(MStringW& text, const MBoard& board) {
	const INT nColumns = board.Columns(), nRows = board.Rows();

	text.clear();

	// 一番上の行。
	text += ZEN_ULEFT;
	for (INT i = 0; i < nColumns; ++i)
		text += ZEN_HLINE;
	text += ZEN_URIGHT;
	text += L"\n";

	for (INT y = 0; y < nRows; ++y) {
		text += ZEN_VLINE; // 縦線
		for (INT x = 0; x < nColumns; ++x) {
			text += board.GetCell(x, y);
		}
		text += ZEN_VLINE; // 縦線
		text += L"\n";
	}

	// 一番下の行。
	text += ZEN_LLEFT;
	for (INT i = 0; i < nColumns; ++i)
		text += ZEN_HLINE;
	text += ZEN_LRIGHT;
	text += L"\n";

	return TRUE;
}

// テキストから盤面を読み込む。
BOOL __fastcall XgBoardFromText(MBoard& board, const MStringW& text) {
	MStringW str = XgNormalizeString(text);
	const size_t cch = str.size();

	// 盤面の左上の角を探す。
	size_t pos = str.find(ZEN_ULEFT);
	if (pos == str.npos)
		return FALSE;
	++pos;

	// 上辺の罫線を数えて列数を決める。
	INT nColumns = 0;
	while (pos < cch && str[pos] == ZEN_HLINE) {
		++nColumns;
		++pos;
	}
	if (pos >= cch || str[pos] != ZEN_URIGHT)
		return FALSE;
	++pos;
	if (nColumns < XG_MIN_SIZE || nColumns > XG_MAX_SIZE)
		return FALSE;

	// ASCIIの空白文字（改行など）を読み飛ばす。
	auto skip_spaces = [&]() {
		while (pos < cch && (str[pos] == L' ' || str[pos] == L'\t' ||
		                     str[pos] == L'\r' || str[pos] == L'\n' ||
		                     str[pos] == ZEN_SPACE))
		{
			++pos;
		}
	};

	// 各行を読み取る。
	MStringListW lines;
	for (;;) {
		skip_spaces();
		if (pos >= cch)
			return FALSE;

		if (str[pos] == ZEN_LLEFT) // 下辺に到達。
			break;

		if (str[pos] != ZEN_VLINE)
			return FALSE;
		++pos;

		if (lines.size() >= (size_t)XG_MAX_SIZE)
			return FALSE;

		MStringW line;
		for (INT x = 0; x < nColumns; ++x) {
			if (pos >= cch)
				return FALSE;

			WCHAR ch = str[pos++];
			switch (ch) {
			case L' ':
			case ZEN_UNDERLINE:
				ch = ZEN_SPACE; // 空白マス
				break;
			case ZEN_SHARP1:
			case ZEN_SHARP2:
			case ZEN_DOT:
				ch = ZEN_BLACK; // 黒マス
				break;
			case ZEN_ULEFT: case ZEN_URIGHT:
			case ZEN_LLEFT: case ZEN_LRIGHT:
			case ZEN_VLINE: case ZEN_HLINE:
				return FALSE; // マスの位置に罫線があってはならない。
			default:
				if (ch < 0x20) // 制御文字は不可。
					return FALSE;
				break;
			}
			line += ch;
		}

		if (pos >= cch || str[pos] != ZEN_VLINE)
			return FALSE;
		++pos;

		lines.push_back(std::move(line));
	}

	// 下辺の確認。
	++pos; // ZEN_LLEFT
	for (INT x = 0; x < nColumns; ++x) {
		if (pos >= cch || str[pos] != ZEN_HLINE)
			return FALSE;
		++pos;
	}
	if (pos >= cch || str[pos] != ZEN_LRIGHT)
		return FALSE;

	INT nRows = (INT)lines.size();
	if (nRows < XG_MIN_SIZE || nRows > XG_MAX_SIZE)
		return FALSE;

	// 失敗時に board を壊さないよう、一時オブジェクトに構築してから入れ替える。
	MBoard tmp(nColumns, nRows, (INT)board.Rules());
	tmp.clear();
	for (INT y = 0; y < nRows; ++y) {
		for (INT x = 0; x < nColumns; ++x) {
			tmp.SetCell(x, y, lines[y][x]); // NonSpaceCountも更新される
		}
	}

	board = std::move(tmp);
	return TRUE;
}

// テキストから盤面を読み込む（エクセル）。
BOOL __fastcall XgBoardFromText2(MBoard& board, const MStringW& text) {
	MStringW str = text;
	if (str.empty())
		return FALSE;

	XgStrTrim(str);
	XgStrReplace(str, L"\r", L"");

	// 改行で分割する。
	MStringListW lines;
	XgStrSplit(lines, str.c_str(), L"\n");
	if (lines.empty())
		return FALSE;

	// 最大タブ数を取得する。
	INT max_tab_count = 0;
	for (auto& line : lines) {
		INT count = 0;
		for (auto ch : line) {
			if (ch == L'\t')
				++count;
		}
		if (max_tab_count < count)
			max_tab_count = count;
	}

	// 足りないタブ文字を追加する。
	for (auto& line : lines) {
		INT count = 0;
		for (auto ch : line) {
			if (ch == L'\t')
				++count;
		}
		if (count < max_tab_count) {
			for (INT i = 0; i < max_tab_count - count; ++i)
				line += L'\t';
		}
	}

	// 必要ならばタブ文字の前後に空白を挿入する。
	for (auto& line : lines) {
		if (line[0] == L'\t')
			line = L' ' + line;
		while (XgStrReplace(line, L"\t\t", L"\t \t"))
			;
		if (line.back() == L'\t')
			line += L' ';
	}

	// タブ文字を取り除く。
	for (auto& line : lines)
		XgStrReplace(line, L"\t", L"");

	// 列が同じサイズでなければ失敗。
	for (auto& line : lines) {
		if (line.size() != lines[0].size()) {
			return FALSE;
		}
	}

	// サイズが範囲外なら却下。
	if (lines.size() < XG_MIN_SIZE || lines[0].size() < XG_MIN_SIZE)
		return FALSE;
	if (lines.size() > XG_MAX_SIZE || lines[0].size() > XG_MAX_SIZE)
		return FALSE;

	// 文字を置き換える。
	for (auto& line : lines) {
		for (auto& ch : line) {
			if (ch == L' ' || ch == L'_' || ch == ZEN_UNDERLINE)
				ch = ZEN_SPACE;
			else if (ch == L'#' || ch == ZEN_SHARP1 || ch == ZEN_SHARP2 || ch == L'.' || ch == ZEN_DOT)
				ch = ZEN_BLACK;
		}
		line = XgNormalizeString(line);
	}

	// マスをセットする。
	const INT nColumns = INT(lines[0].size()), nRows = INT(lines.size());
	board = MBoard(nColumns, nRows, board.Rules());
	for (INT iRow = 0; iRow < nRows; ++iRow) {
		for (INT jColumn = 0; jColumn < nColumns; ++jColumn) {
			board.SetCell(jColumn, iRow, lines[iRow][jColumn]);
		}
	}

	return TRUE;
}

// パターンを現在の盤面から取得。
MBoard __fastcall XgGetCurrentPat(const MBoard& board) {
	MStringW text;
	XgTextFromBoard(text, board);

	for (auto& ch : text) {
		switch (ch) {
		case ZEN_SPACE: case ZEN_BLACK: case '\n': case '\r':
		case L'┏': case L'━':  case L'┓': case L'┃': case L'┗': case L'┛':
			break;
		default:
			ch = ZEN_SPACE; // 文字マスを空白にする。
			break;
		}
	}

	MBoard new_board = board;
	XgBoardFromText(new_board, text);
	return new_board;
}

//////////////////////////////////////////////////////////////////////////////
// 単体テスト。

// テスト用ヘルパー：罫線付きテキストの部品。
static inline MStringW XgTestTop(INT n) {
	return MStringW(1, ZEN_ULEFT) + MStringW(n, ZEN_HLINE) + MStringW(1, ZEN_URIGHT);
}
static inline MStringW XgTestBottom(INT n) {
	return MStringW(1, ZEN_LLEFT) + MStringW(n, ZEN_HLINE) + MStringW(1, ZEN_LRIGHT);
}
static inline MStringW XgTestRow(const MStringW& cells) {
	return MStringW(1, ZEN_VLINE) + cells + MStringW(1, ZEN_VLINE);
}

// テスト用ヘルパー：盤面が期待する行データと一致するか？
static bool XgTestBoardEquals(const MBoard& board, const MStringListW& rows) {
	if (rows.empty())
		return false;
	if (board.Rows() != (INT)rows.size() || board.Columns() != (INT)rows[0].size())
		return false;

	INT nNonSpaces = 0;
	for (INT y = 0; y < board.Rows(); ++y) {
		for (INT x = 0; x < board.Columns(); ++x) {
			if (board.GetCell(x, y) != rows[y][x])
				return false;
			if (rows[y][x] != ZEN_SPACE)
				++nNonSpaces;
		}
	}
	return board.NonSpaceCount() == nNonSpaces;
}

void __fastcall XgBoardTextIOUnitTest(void) {
	const MStringW B(1, ZEN_BLACK), S(1, ZEN_SPACE);

	// --- 1. XgTextFromBoard → XgBoardFromText の往復 ---
	{
		MStringListW rows = {
			B + L"ア" + S,
			L"カ" + S + L"ア",
			S + L"カ" + B,
		};
		MBoard src(3, 3, XG_STANDARD_RULES);
		src.clear();
		for (INT y = 0; y < 3; ++y)
			for (INT x = 0; x < 3; ++x)
				src.SetCell(x, y, rows[y][x]);

		MStringW text;
		BOOL ok = XgTextFromBoard(text, src);
		PrintfW(L"%s\n", text.c_str());
		assert(ok);

		MBoard dst(3, 3, 0);
		ok = XgBoardFromText(dst, text);
		assert(ok);
		assert(XgTestBoardEquals(dst, rows));
		assert(dst.Rules() == 0); // 既存のルールが引き継がれる
	}

	// --- 2. 別サイズへの変更（改行なし／LF／CRLF） ---
	{
		MStringListW rows = {
			B + L"ア" + S + L"カ",
			S + S + B + L"ア",
			L"カア" + S + B,
		};
		const LPCWSTR newlines[] = { L"", L"\n", L"\r\n" };
		for (LPCWSTR nl : newlines) {
			MStringW text = XgTestTop(4) + nl;
			for (auto& row : rows)
				text += XgTestRow(row) + nl;
			text += XgTestBottom(4) + nl;

			MBoard board(5, 5, RULE_DONTDOUBLEBLACK);
			BOOL ok = XgBoardFromText(board, text);
			assert(ok);
			assert(XgTestBoardEquals(board, rows));
			assert(board.Rules() == RULE_DONTDOUBLEBLACK);
		}
	}

	// --- 3. 別表記の正規化（＿ 半角スペース → 空白、♯ ＃ ． → 黒マス） ---
	{
		MStringW text = XgTestTop(3);
		text += XgTestRow(MStringW(L"＿ ") + L"ア");
		text += XgTestRow(MStringW(1, ZEN_SHARP1) + MStringW(1, ZEN_SHARP2) + MStringW(1, ZEN_DOT));
		text += XgTestRow(MStringW(L"ア") + S + B);
		text += XgTestBottom(3);

		MStringListW expected = {
			S + S + L"ア",
			B + B + B,
			L"ア" + S + B,
		};
		MBoard board(3, 3, XG_STANDARD_RULES);
		BOOL ok = XgBoardFromText(board, text);
		assert(ok);
		assert(XgTestBoardEquals(board, expected));
	}

	// --- 4. 前後にゴミがあっても読める ---
	{
		MStringListW rows = { B + S + S, S + L"ア" + S, S + S + B };
		MStringW text = L"abc\r\n" + XgTestTop(3) + L"\r\n";
		for (auto& row : rows)
			text += XgTestRow(row) + L"\r\n";
		text += XgTestBottom(3) + L"\r\nxyz";

		MBoard board(3, 3, XG_STANDARD_RULES);
		BOOL ok = XgBoardFromText(board, text);
		assert(ok);
		assert(XgTestBoardEquals(board, rows));
	}

	// --- 5. 失敗するケース（失敗時は board が変更されないこと） ---
	{
		const MStringW row3 = XgTestRow(B + S + L"ア");

		MStringListW bad;
		bad.push_back(L"");                                                         // 空文字列
		bad.push_back(L"あいうえお");                                               // 罫線なし
		bad.push_back(XgTestTop(3) + row3 + row3 + row3);                           // 下辺なし
		bad.push_back(XgTestTop(3) + row3 + XgTestBottom(3));          // 行数が少なすぎる(1行)
		bad.push_back(XgTestTop(1) + XgTestRow(B) + XgTestRow(S) +
		              XgTestRow(B) + XgTestBottom(1));                 // 列数が少なすぎる(1列)
		bad.push_back(XgTestTop(3) + row3 + XgTestRow(B + S) + row3 +
		              XgTestBottom(3));                                             // 行が短い
		bad.push_back(XgTestTop(3) + row3 + XgTestRow(B + S + L"ア" + L"カ") + row3 +
		              XgTestBottom(3));                                             // 行が長い
		bad.push_back(XgTestTop(3) + row3 + row3 + row3 + XgTestBottom(4));         // 下辺の幅が違う
		bad.push_back(XgTestTop(3) + row3 + XgTestRow(B + MStringW(1, ZEN_HLINE) + S) +
		              row3 + XgTestBottom(3));                                      // マスに罫線
		bad.push_back(XgTestTop(3) + row3 + row3 + row3 +
		              MStringW(1, ZEN_LLEFT) + MStringW(3, ZEN_HLINE));             // 右下の角なし

		for (const auto& text : bad) {
			MBoard board(5, 4, RULE_DONTCORNERBLACK);
			board.clear();
			board.SetCell(1, 2, L'ア');

			BOOL ok = XgBoardFromText(board, text);
			assert(!ok);

			// 変更されていない。
			assert(board.Columns() == 5);
			assert(board.Rows() == 4);
			assert(board.Rules() == RULE_DONTCORNERBLACK);
			assert(board.GetCell(1, 2) == L'ア');
			assert(board.NonSpaceCount() == 1);
		}
	}
}
