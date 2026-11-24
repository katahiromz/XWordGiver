// pat-match.cpp --- クロスワード用のパターンマッチ。
// Author: katahiromz
// License: MIT

#include "pat-match.h"

// 盤面にある全単語を取得。空白を含むものや、2文字未満は無視。
MStringSetW __fastcall XgWordsFromBoard(const MBoard& board) {
	MStringSetW words;
	const INT nColumns = board.Columns(), nRows = board.Rows();
	for (INT iRow = 0; iRow < nRows; ++iRow) {
		for (INT jColumn = 0; jColumn < nColumns; ++jColumn) {
			const WCHAR ch0 = board.GetCell(jColumn, iRow);
			if (ch0 == ZEN_BLACK)
				continue;
			const WCHAR ch1 = board.GetCell(jColumn - 1, iRow);
			if (ch1 == ZEN_BLACK) {
				auto pat = XgGetPatFromBoard(board, jColumn, iRow, FALSE);
				if (pat.size() >= 2 && !XgPatHasSpace(pat))
					words.insert(pat);
			}
			const WCHAR ch2 = board.GetCell(jColumn, iRow - 1);
			if (ch2 == ZEN_BLACK) {
				auto pat = XgGetPatFromBoard(board, jColumn, iRow, TRUE);
				if (pat.size() >= 2 && !XgPatHasSpace(pat))
					words.insert(pat);
			}
		}
	}
	return words;
}

// 盤面のパターンをスキャンする。FALSEを返したら重複単語あり。
BOOL __fastcall XgScanBoardPats(const MBoard& board, MStringListW* pats, MStringSetW* used_words) {
	const INT nCols = board.Columns(), nRows = board.Rows();

	if (pats)
		pats->clear();

	MStringSetW used;
	if (!used_words)
		used_words = &used;
	used_words->clear();

	// 重複なら false を返す。
	auto check = [&](INT j, INT i, BOOL bDown) -> bool {
		auto pat = XgGetPatFromBoard(board, j, i, bDown);
		if (pat.size() <= 1)
			return true;  // 1文字は単語ではない。
		if (pats)
			pats->push_back(pat);
		if (XgPatHasSpace(pat))
			return true;  // 未完成の単語は対象外。
		return used_words->insert(pat).second; // 既に入っていたら重複。
	};

	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nCols; ++j) {
			if (board.GetCell(j, i, ZEN_BLACK) == ZEN_BLACK)
				continue;

			// ヨコ向きの単語の先頭か？(左が黒または盤面外、右が白)
			if (board.GetCell(j - 1, i, ZEN_BLACK) == ZEN_BLACK &&
				board.GetCell(j + 1, i, ZEN_BLACK) != ZEN_BLACK)
			{
				if (!check(j, i, FALSE))
					return FALSE;
			}

			// タテ向きの単語の先頭か？(上が黒または盤面外、下が白)
			if (board.GetCell(j, i - 1, ZEN_BLACK) == ZEN_BLACK &&
				board.GetCell(j, i + 1, ZEN_BLACK) != ZEN_BLACK)
			{
				if (!check(j, i, TRUE))
					return FALSE;
			}
		}
	}

	return TRUE;
}

// 盤面について辞書を引く。
HRESULT __fastcall XgLookupDictForBoard(MBoard& board, const MDict& dict, MStringMapW& word2clue) {
	word2clue.clear();

	if (!board.IsFull())
		return E_FAIL;

	// 黒マスルールに適合するか？
	if (!board.AreBlacksOK())
		return E_FAIL;

	// 重複単語がないか？
	MStringListW pats;
	MStringSetW used_words;
	if (!XgScanBoardPats(board, &pats, &used_words))
		return E_FAIL;

	HRESULT hr = S_OK;
	for (auto& word : used_words) {
		auto it = dict.m_word2clue.find(word);
		if (it != dict.m_word2clue.end()) {
			word2clue[word] = it->second;
		} else {
			word2clue[word] = L"";
			hr = S_FALSE;
		}
	}

	return hr;
}

// 盤面の仕上げを行う。
HRESULT __fastcall XgFinalizeBoard(MBoardDisplay& display, MDict& dict, MStringMapW& word2clue) {
	display.m_vHorzInfo.clear();
	display.m_vVertInfo.clear();
	word2clue.clear();
	display.m_bSolved = FALSE;

	if (!display.m_solution.IsFull())
		return E_FAIL;

	display.m_board.ClearLetters();

	HRESULT hr = XgLookupDictForBoard(display.m_solution, dict, word2clue);
	if (FAILED(hr)) {
		word2clue.clear();
		return hr;
	}

	if (!XgDoNumbering(display.m_solution, display.m_vHorzInfo, display.m_vVertInfo)) {
		word2clue.clear();
		display.m_vHorzInfo.clear();
		display.m_vVertInfo.clear();
		return E_FAIL;
	}

	display.m_bSolved = TRUE;
	return S_OK;
}

// 候補を取得する。
bool __fastcall
XgGetCandidatesAddBlack(const MDict& dict, std::vector<MStringW>& cands, const MStringW& pattern, INT& nSkip,
	bool left_black_check, bool right_black_check)
{
	// パターンの長さ。
	const INT patlen = INT(pattern.size());
	assert(patlen >= 2);

	// 候補をクリアする。
	cands.clear();
	// スピードのために予約する。
	cands.reserve(dict.m_word_list.size() / 32);

	// 初期化する。
	nSkip = 0;
	MStringW result(pattern);

	// パターンが3字以上か？
	if (patlen > 2) {
		// 孤立した文字マスが左端か？
		if (result[0] != ZEN_SPACE && result[1] == ZEN_SPACE) {
			// その右にブロックを置いたものを候補にする。
			result[1] = ZEN_BLACK;
			cands.emplace_back(result);
			result = pattern;
			// これは先に処理されるべきなので、ランダム化から除外する。
			nSkip++;
		} else {
			bool bCharNotFound = true;  // 文字マスがなかったか？

			// 孤立した文字マスが中にあるか？
			for (INT j = 1; j < patlen - 1; j++) {
				if (result[j] != ZEN_SPACE) {
					if (result[j - 1] == ZEN_SPACE && result[j + 1] == ZEN_SPACE) {
						// 孤立した文字マスがあった。
						// その両端にブロックを置く。
						result[j - 1] = result[j + 1] = ZEN_BLACK;
						cands.emplace_back(result);
						result = pattern;
						// これは先に処理されるべきなので、ランダム化から除外する。
						nSkip++;
					}
					bCharNotFound = false;
					break;
				}
			}

			if (bCharNotFound) {
				// 孤立した文字マスが右端か？
				if (result[patlen - 1] != ZEN_SPACE && result[patlen - 2] == ZEN_SPACE) {
					// その左にブロックを置く。
					result[patlen - 2] = ZEN_BLACK;
					cands.emplace_back(result);
					result = pattern;
					// これは先に処理されるべきなので、ランダム化から除外する。
					nSkip++;
				}
			}
		}
	}

	// 候補数の上限（メモリとパフォーマンスのバランス）。
	constexpr size_t MAX_CANDIDATES = 1000;

	// 長さでインデックス化された辞書を使用（高速化）。
	const auto& dict_by_length = dict.m_word_list_by_len;

	// すべての登録されている単語について。
	// 長さがpatlen以下の単語のみを検索。
	for (INT wordlen = 2; wordlen <= patlen; ++wordlen) {
		// 早期終了: 候補数が上限に達したら打ち切る。
		if (cands.size() >= MAX_CANDIDATES)
			break;

		auto it = dict_by_length.find(wordlen);
		if (it == dict_by_length.end())
			continue; // 該当する長さの単語がない。

		const auto& words_of_length = it->second;

		for (auto& data : words_of_length) {
			// 早期終了: 候補数が上限に達したら打ち切る。
			if (cands.size() >= MAX_CANDIDATES)
				break;

			const MStringW& word = data;

			// 単語の置ける区間について調べる。
			const INT patlen_minus_wordlen = patlen - wordlen;
			for (INT j = 0; j <= patlen_minus_wordlen; j++) {
				// 区間[j, j + wordlen - 1]の前後に文字があったらスキップする。
				if (j > 0 && pattern[j - 1] != ZEN_SPACE)
					continue;
				if (j < patlen_minus_wordlen && pattern[j + wordlen] != ZEN_SPACE)
					continue;

				// 区間[j, j + wordlen - 1]に文字マスがあるか？
				bool bCharFound = false;
				const INT j_plus_wordlen = j + wordlen;
				for (INT m = j; m < j_plus_wordlen; m++) {
					assert(pattern[m] != ZEN_BLACK);
					if (pattern[m] != ZEN_SPACE) {
						bCharFound = true;
						break;
					}
				}
				if (!bCharFound)
					continue;

				// パターンが単語にマッチするか？
				bool bMatched = true;
				for (INT m = j, n = 0; n < wordlen; m++, n++) {
					if (pattern[m] != ZEN_SPACE && pattern[m] != word[n]) {
						bMatched = false;
						break;
					}
				}
				if (!bMatched)
					continue;

				// マッチした。
				result = pattern;

				// 区間[j, j + wordlen - 1]の前後に■をおく。
				if (j > 0)
					result[j - 1] = ZEN_BLACK;
				if (j < patlen_minus_wordlen)
					result[j + wordlen] = ZEN_BLACK;

				// 区間[j, j + wordlen - 1]に単語を適用する。
				for (INT k = 0, m = j; k < wordlen; k++, m++)
					result[m] = word[k];

				// 黒マスの連続を除外する。
				if (left_black_check && result[0] == ZEN_BLACK)
					continue;
				if (right_black_check && result[patlen - 1] == ZEN_BLACK)
					continue;

				// 追加する。
				cands.emplace_back(result);

				// 早期終了: 候補数が上限に達したら打ち切る。
				if (cands.size() >= MAX_CANDIDATES)
					break;
			}
		}
	}

	// 候補が空でなければ成功。
	return !cands.empty();
}
