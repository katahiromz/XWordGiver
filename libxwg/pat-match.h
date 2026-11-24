// pat-match.h --- クロスワード用のパターンマッチ。
// Author: katahiromz
// License: MIT

#pragma once

#include "dict.h"
#include "board-image.h"

BOOL __fastcall XgScanBoardPats(const MBoard& board, MStringListW* pats = nullptr, MStringSetW* used_words = nullptr);
MStringSetW __fastcall XgWordsFromBoard(const MBoard& board);
HRESULT __fastcall XgLookupDictForBoard(MBoard& board, const MDict& dict, MStringMapW& word2clue);
HRESULT __fastcall XgFinalizeBoard(MBoardDisplay& display, MDict& dict, MStringMapW& word2clue);

bool __fastcall XgGetCandidatesAddBlack(const MDict& dict,
	std::vector<MStringW>& cands, const MStringW& pattern, int& nSkip,
	bool left_black_check, bool right_black_check);

// パターン文字列に全角空白が含まれているか？
inline BOOL __fastcall XgPatHasSpace(const MStringW& pat) noexcept {
	for (auto ch : pat) {
		if (ch == ZEN_SPACE)
			return TRUE;
	}
	return FALSE;
}

// 盤面上のパターンを取得する。
// bDownがTRUEのときは、タテ向き。bDownがFALSEのときは、ヨコ向き。
inline MStringW __fastcall XgGetPatFromBoard(const MBoard& board, INT jColumn, INT iRow, BOOL bDown) {
	MStringW ret;
	if (bDown) {
		while (board.GetCell(jColumn, iRow - 1, ZEN_BLACK) != ZEN_BLACK) {
			--iRow;
		}
		for (;;) {
			WCHAR ch = board.GetCell(jColumn, iRow, ZEN_BLACK);
			if (ch == ZEN_BLACK)
				return ret;
			ret += ch;
			++iRow;
		}
	} else {
		while (board.GetCell(jColumn - 1, iRow, ZEN_BLACK) != ZEN_BLACK) {
			--jColumn;
		}
		for (;;) {
			WCHAR ch = board.GetCell(jColumn, iRow, ZEN_BLACK);
			if (ch == ZEN_BLACK)
				return ret;
			ret += ch;
			++jColumn;
		}
	}
}

// 盤面上にパターンを適用する。
// bDownがTRUEのときは、タテ向き。bDownがFALSEのときは、ヨコ向き。
inline void XgApplyCandidate(MBoard& board, INT jColumn, INT iRow, const MStringW& cand, BOOL bDown) {
	if (bDown) {
		while (board.GetCell(jColumn, iRow - 1) != ZEN_BLACK)
			--iRow;
		for (auto ch : cand) {
			board.SetCell(jColumn, iRow, ch);
			++iRow;
		}
	} else {
		while (board.GetCell(jColumn - 1, iRow) != ZEN_BLACK)
			--jColumn;
		for (auto ch : cand) {
			board.SetCell(jColumn, iRow, ch);
			++jColumn;
		}
	}
}

// パターン文字列が単語にマッチするか？
inline BOOL __fastcall XgPatMatch(const MStringW& pat, const MStringW& word) noexcept {
	if (pat.size() != word.size())
		return FALSE;

	for (size_t ich = 0; ich < pat.size(); ++ich) {
		if (pat[ich] == word[ich] || pat[ich] == ZEN_SPACE)
			continue;
		return FALSE;
	}

	return TRUE;
}

// パターン文字列にマッチする候補群を取得する。
inline BOOL __fastcall XgGetPatCands(MStringListW& cands, const MStringListW& word_list, const MStringW& pat) {
	cands.clear();
	for (auto& word : word_list) {
		if (XgPatMatch(pat, word)) {
			cands.push_back(word);
		}
	}
	return !cands.empty();
}
