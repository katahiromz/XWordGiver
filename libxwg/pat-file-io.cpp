// pat-file-io.cpp --- Pattern File Input/Output
// Author: katahiromz
// License: MIT

#include "board-text-io.h"
#include "pat-file-io.h"
#include "flip-board.h"
#include "utils.h"
#include <algorithm>

// ソートして一意化する。
void __fastcall XgSortAndUniqueBoards(std::vector<MBoard>& boards, BOOL bExpand) {
	auto tmp = boards;
	if (bExpand) {
		// 反転・転置したパターンも追加する。
		for (auto& board : boards) {
			tmp.push_back(board);
			auto flip_h = XgFlipBoardH(board);
			auto flip_v = XgFlipBoardV(board);
			auto flip_hv = XgFlipBoardH(flip_v);
			auto transpose = XgTransposeBoard(board);
			auto transpose_h = XgFlipBoardH(transpose);
			auto transpose_v = XgFlipBoardV(transpose);
			tmp.push_back(flip_h);
			tmp.push_back(flip_v);
			tmp.push_back(flip_hv);
			tmp.push_back(transpose);
			tmp.push_back(transpose_h);
			tmp.push_back(transpose_v);
		}
	} else {
		// 反転・転置を考慮して一意化する。
		for (auto& board : boards) {
			auto flip_h = XgFlipBoardH(board);
			auto flip_v = XgFlipBoardV(board);
			auto flip_hv = XgFlipBoardH(flip_v);
			auto transpose = XgTransposeBoard(board);
			auto transpose_h = XgFlipBoardH(transpose);
			auto transpose_v = XgFlipBoardV(transpose);
			tmp.erase(
				std::remove_if(tmp.begin(), tmp.end(), [&](const MBoard& b) noexcept {
					return flip_h == b ||
						   flip_v == b ||
						   flip_hv == b ||
						   transpose == b ||
						   transpose_h == b ||
						   transpose_v == b;
				}),
				tmp.end()
			);
			tmp.push_back(board);
		}
	}

	//printf("old boards: %d\n", (INT)boards.size());
	boards = std::move(tmp);
	//printf("new boards: %d\n", (INT)boards.size());

	// ソートする。
	std::sort(boards.begin(), boards.end(), [](const MBoard& x, const MBoard& y) {
		if (x.Columns() < y.Columns())
			return true;
		if (x.Columns() > y.Columns())
			return false;
		if (x.Rows() < y.Rows())
			return true;
		if (x.Rows() > y.Rows())
			return false;
		auto data1 = x.Data();
		auto data2 = y.Data();
		return data1 < data2;
	});

	// さらに一意化する。
	auto last = std::unique(boards.begin(), boards.end(), [](const MBoard& a, const MBoard& b) noexcept {
		return a == b;
	});
	boards.erase(last, boards.end());
}

// ファイルからパターン群を読み込む。
HRESULT __fastcall XgLoadPatFile(LPCWSTR pszFileName, std::vector<MBoard>& boards, INT nRules, MStringListW& comments) {
	boards.clear();
	comments.clear();

	nRules |= RULE_DONTDIVIDE; // 分断禁は例外。

	FILE *fin = _wfopen(pszFileName, L"rb");
	if (!fin) {
		return E_FAIL;
	}
	AutoFClose auto_close(fin);

	char buf[512];
	MStringW strBoard;
	BOOL bInBoard = FALSE;
	BOOL first = TRUE;
	while (fgets(buf, sizeof(buf), fin)) {
		if (first) {
			first = FALSE;
			if (memcmp(buf, "\xEF\xBB\xBF", 3) == 0) { // UTF-8 BOM
				buf[0] = buf[1] = buf[2] = ' ';
			}
		}
		StrTrimA(buf, " \t\r\n");
		if (!buf[0])
			continue;

		auto wide = WideFromUtf8(buf);
		XgStrTrim(wide);
		if (wide.empty())
			continue;

		if (wide[0] == L';') {
			comments.push_back(wide);
			continue;
		}

		if (bInBoard) {
			strBoard += wide;
			strBoard += L"\n";
			if (wide.find(ZEN_LLEFT) != wide.npos && wide.find(ZEN_LRIGHT) != wide.npos) {
				bInBoard = FALSE;
				MBoard board(3, 3, nRules);
				if (XgBoardFromText(board, strBoard)) {
					if (board.AreBlacksOK()) {
						boards.push_back(board);
					}
				}
				strBoard.clear();
			}
		} else {
			if (wide.find(ZEN_ULEFT) != wide.npos && wide.find(ZEN_URIGHT) != wide.npos) {
				bInBoard = TRUE;
				strBoard += wide;
				strBoard += L"\n";
			}
		}
	}

	if (boards.empty())
		return E_FAIL;

	XgSortAndUniqueBoards(boards, TRUE);
	return S_OK;
}

// パターン群をファイルに書き込む。
HRESULT __fastcall XgSavePatFile(LPCWSTR pszFileName, std::vector<MBoard>& boards, MStringListW& comments) {
	FILE *fout = _wfopen(pszFileName, L"w");
	if (!fout) {
		return E_FAIL;
	}
	AutoFClose auto_close(fout);

	for (auto& line : comments) {
		auto utf8 = Utf8FromWide(line);
		fprintf(fout, "%s\n", utf8.c_str());
	}

	XgSortAndUniqueBoards(boards, FALSE);

	//printf("tmp: %d\n", (INT)tmp.size());

	for (auto& board : boards) {
		const INT nColumns = board.Columns(), nRows = board.Rows();

		MStringW text;
		if (!XgTextFromBoard(text, board))
			continue;

		fprintf(fout, "\n");
		fprintf(fout, "%d x %d = %d\n", nColumns, nRows, nColumns * nRows);
		auto utf8 = Utf8FromWide(text);
		fprintf(fout, "%s", utf8.c_str());
	}

	if (ferror(fout))
		return E_FAIL;

	return S_OK;
}

// パターン編集。
BOOL __fastcall XgPatEdit(const MBoard& board, BOOL bAdd) {
	auto filename = XgFindLocalFile(L"PAT.txt");

	// 読み込む。
	std::vector<MBoard> patterns;
	MStringListW comments;
	HRESULT hr = XgLoadPatFile(filename.c_str(), patterns, board.Rules(), comments);
	if (FAILED(hr))
		return FALSE;

	// 現在の盤面から文字マスを削って現在のパターンとする。
	MBoard cur_pat = XgGetCurrentPat(board);

	// 分断禁。
	if (cur_pat.DividedByBlack())
		return FALSE;

	auto flip_h = XgFlipBoardH(cur_pat);
	auto flip_v = XgFlipBoardV(cur_pat);
	auto flip_hv = XgFlipBoardH(flip_v);
	auto transpose = XgTransposeBoard(cur_pat);
	auto transpose_h = XgFlipBoardH(transpose);
	auto transpose_v = XgFlipBoardV(transpose);

	if (bAdd) { // 追加。
		patterns.push_back(cur_pat);
		patterns.push_back(flip_h);
		patterns.push_back(flip_v);
		patterns.push_back(flip_hv);
		patterns.push_back(transpose);
		patterns.push_back(transpose_h);
		patterns.push_back(transpose_v);
	} else { // 削除。
		std::vector<MBoard> temp_pats; // 一時的なパターン群。
		for (const auto& pat : patterns) {
			if (pat == cur_pat ||
				pat == flip_h ||
				pat == flip_v ||
				pat == flip_hv ||
				pat == transpose ||
				pat == transpose_h ||
				pat == transpose_v)
			{
				continue;
			}
			temp_pats.push_back(pat);
		}
		patterns = std::move(temp_pats);
	}

	// ソートして一意化する。
	XgSortAndUniqueBoards(patterns, FALSE);

	// 保存。
	hr = XgSavePatFile(filename.c_str(), patterns, comments);
	return SUCCEEDED(hr);
}

// 単体テスト。
void __fastcall XgPatFileUnitTest(void) {
	std::vector<MBoard> boards;
	MStringListW comments;
	printf("Loading PAT.txt\n");
	HRESULT hr = XgLoadPatFile(XgFindLocalFile(L"PAT.txt").c_str(), boards, 0, comments);
	printf("Loaded PAT.txt\n");
	assert(hr == S_OK);
	printf("boards: %d\n", (INT)boards.size());
	printf("Saving PAT2.txt\n");
	hr = XgSavePatFile(L"PAT2.txt", boards, comments);
	printf("Saved PAT2.txt\n");
	printf("boards: %d\n", (INT)boards.size());
	assert(hr == S_OK);
	DeleteFileW(L"PAT2.txt");
}
