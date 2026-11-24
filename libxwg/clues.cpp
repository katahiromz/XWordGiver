// clues.cpp --- クロスワードのカギ情報。
// Author: katahiromz
// License: MIT

#include "clues.h"

// 単語データが存在するか？
inline bool XgWordDataExists(const MDict& dict, const MStringW& word) {
	return dict.m_word2clue.find(word) != dict.m_word2clue.end();
}

//////////////////////////////////////////////////////////////////////////////

// 番号をつける。
BOOL __fastcall XgDoNumbering(
	const MBoard& board,
	std::vector<MPlaceInfo>& vHorzInfo,
	std::vector<MPlaceInfo>& vVertInfo)
{
	const INT nCols = board.Columns(), nRows = board.Rows();
	// 使った単語の集合。
	std::unordered_set<MStringW> used_words;
	used_words.reserve(nRows * nCols / 4); // スピードのため、予約。

	assert(board.IsFull());

	// カギをクリアする。
	vVertInfo.clear();
	vHorzInfo.clear();

	// 各列について、縦向きにスキャンする。
	for (INT j = 0; j < nCols; j++) {
		for (INT i = 0; i < nRows - 1; i++) {
			// 文字マスの連続が見つかったか？
			if (board.GetCell0(j, i) != ZEN_BLACK && board.GetCell0(j, i + 1) != ZEN_BLACK) {
				INT lo, hi;

				// 単語が置ける区間を求める。
				lo = hi = i;
				while (hi + 1 < nRows) {
					if (board.GetCell0(j, hi + 1) == ZEN_BLACK)
						break;
					hi++;
				}
				assert(hi == nRows - 1 || board.GetCell0(j, hi + 1) == ZEN_BLACK);

				// 次の位置を設定する。
				i = hi + 1;

				// 空白があるかを調べると同時に、その区間にある単語を取得する。
				MStringW word;
				for (INT k = lo; k <= hi; k++) {
					if (board.GetCell0(j, k) == ZEN_SPACE)
						goto space_found_1;
					word += board.GetCell0(j, k);
				}

				// 空白が見つからなかった。
				// すでに使用した単語があるか？
				if (used_words.count(word))
					return FALSE; // すでに使用した単語が使われたので、失敗。

				// 一度使った単語として登録。
				used_words.emplace(word);

				// 縦のカギに登録。
				vVertInfo.emplace_back(j, lo, std::move(word));
space_found_1:;
			}
		}
	}

	// 各行について横向きにスキャンする。
	for (INT i = 0; i < nRows; i++) {
		for (INT j = 0; j < nCols - 1; j++) {
			// 文字マスの連続が見つかったか？
			if (board.GetCell0(j, i) != ZEN_BLACK && board.GetCell0(j + 1, i) != ZEN_BLACK) {
				INT lo, hi;

				// 単語が置ける区間を求める。
				lo = hi = j;
				while (hi + 1 < nCols) {
					if (board.GetCell0(hi + 1, i) == ZEN_BLACK)
						break;
					hi++;
				}
				assert(hi == nCols - 1 || board.GetCell0(hi + 1, i) == ZEN_BLACK);

				// 次の位置を設定する。
				j = hi + 1;

				// 空白があるかを調べると同時に、その区間にある単語を取得する。
				MStringW word;
				for (INT k = lo; k <= hi; k++) {
					if (board.GetCell0(k, i) == ZEN_SPACE)
						goto space_found_2;
					word += board.GetCell0(k, i);
				}

				// 空白が見つからなかった。
				// すでに使用した単語があるか？
				if (used_words.count(word))
					return FALSE; // 使用した単語が使われたため、失敗。

				// 一度使った単語として登録。
				used_words.emplace(word);

				// 横のカギとして登録。
				vHorzInfo.emplace_back(lo, i, std::move(word));
space_found_2:;
			}
		}
	}

	// カギの格納情報に順序をつける。
	std::vector<MPlaceInfo *> data;
	const INT size1 = INT(vVertInfo.size());
	const INT size2 = INT(vHorzInfo.size());
	data.reserve(size1 + size2);

	for (INT k = 0; k < size1; k++) {
		data.emplace_back(&vVertInfo[k]);
	}
	for (INT k = 0; k < size2; k++) {
		data.emplace_back(&vHorzInfo[k]);
	}

	// 位置情報でソート。
	std::sort(data.begin(), data.end(), [](const MPlaceInfo *ppi1, const MPlaceInfo *ppi2){
		if (ppi1->m_iRow < ppi2->m_iRow)
			return true;
		if (ppi1->m_iRow > ppi2->m_iRow)
			return false;
		if (ppi1->m_jCol < ppi2->m_jCol)
			return true;
		return false;
	});

	// 順序付けられたカギの格納情報に番号を設定する。
	INT number = 1;
	const INT size = static_cast<INT>(data.size());
	for (INT k = 0; k < size; k++) {
		// 番号を設定する。
		data[k]->m_number = number;
		if (k + 1 < size) {
			// 次の格納情報が同じ位置ならば
			if (data[k]->m_jCol == data[k + 1]->m_jCol && data[k]->m_iRow == data[k + 1]->m_iRow) {
				// 無視する。
			} else {
				number++; // 違う位置なら、番号を増やす。
			}
		}
	}

	// カギの格納情報を番号順に並べ替える。
	auto compare_number = [](const MPlaceInfo& pi1, const MPlaceInfo& pi2){
		return pi1.m_number < pi2.m_number;
	};
	std::sort(vVertInfo.begin(), vVertInfo.end(), compare_number);
	std::sort(vHorzInfo.begin(), vHorzInfo.end(), compare_number);

	return TRUE;
}

// タテのカギのラベル
static MStringW XgGetDownNumber(void) {
	if (XgIsUserJapanese())
		return L"【タテ%d】";
	return L"[D%d]";
}

// ヨコのカギのラベル
static MStringW XgGetAccrossNumber(void) {
	if (XgIsUserJapanese())
		return L"【ヨコ%d】";
	return L"[A%d]";
}

// タテのカギをテキストに変換する（スタイル0）。
MStringW XgTextFromDownCluesStyle0(const std::vector<MPlaceInfo>& vVertInfo, const MStringMap& word2clue, BOOL bShowAnswer) {
	MStringW ret;

	// タテのカギの文字列を構成する。
	ret += XgIsUserJapanese() ? L"☆☆☆ タテのカギ ☆☆☆" : L"*** DOWN ***";
	ret += L"\r\n";

	WCHAR text[128];
	for (auto& info : vVertInfo) {
		StringCchPrintf(text, _countof(text), XgGetDownNumber().c_str(), info.m_number);
		ret += text;

		if (bShowAnswer) {
			ret += L'≪'; // U+226A
			ret += info.m_word;
			ret += L'≫'; // U+226B
		}

		// ヒント文章を追加する。
		auto it = word2clue.find(info.m_word);
		if (it != word2clue.end()) {
			ret += it->second;
		}

		ret += L"\r\n";   // 改行。
	}

	ret += L"\r\n";
	return ret;
}

// ヨコのカギをテキストに変換する（スタイル0）。
MStringW XgTextFromAcrossCluesStyle0(const std::vector<MPlaceInfo>& vHorzInfo, const MStringMap& word2clue, BOOL bShowAnswer) {
	MStringW ret;

	// ヨコのカギの文字列を構成する。
	ret += XgIsUserJapanese() ? L"☆☆☆ ヨコのカギ ☆☆☆" : L"*** ACROSS ***";
	ret += L"\r\n";

	WCHAR text[128];
	for (auto& info : vHorzInfo) {
		StringCchPrintfW(text, _countof(text), XgGetAccrossNumber().c_str(), info.m_number);
		ret += text;

		if (bShowAnswer) {
			ret += L'≪'; // U+226A
			ret += info.m_word;
			ret += L'≫'; // U+226B
		}

		// ヒント文章を追加する。
		auto it = word2clue.find(info.m_word);
		if (it != word2clue.end()) {
			ret += it->second;
		}

		ret += L"\r\n";   // 改行。
	}

	ret += L"\r\n";
	return ret;
}

// タテのカギをテキストに変換する（スタイル1）。
MStringW XgTextFromDownCluesStyle1(const std::vector<MPlaceInfo>& vVertInfo, const MStringMap& word2clue, BOOL bShowAnswer) {
	MStringW ret;

	// タテのカギの文字列を構成する。
	ret += XgIsUserJapanese() ? L"☆☆☆ タテのカギ ☆☆☆" : L"*** DOWN ***";
	ret += L"\r\n";

	WCHAR text[128];
	for (auto& info : vVertInfo) {
		StringCchPrintf(text, _countof(text), L"%d. ", info.m_number);
		ret += text;

		if (bShowAnswer) {
			ret += L'≪'; // U+226A
			ret += info.m_word;
			ret += L'≫'; // U+226B
		}

		// ヒント文章を追加する。
		auto it = word2clue.find(info.m_word);
		if (it != word2clue.end()) {
			ret += it->second;
		}

		ret += L"\r\n";   // 改行。
	}

	ret += L"\r\n";
	return ret;
}

// ヨコのカギをテキストに変換する（スタイル1）。
MStringW XgTextFromAcrossCluesStyle1(const std::vector<MPlaceInfo>& vHorzInfo, const MStringMap& word2clue, BOOL bShowAnswer) {
	MStringW ret;

	// ヨコのカギの文字列を構成する。
	ret += XgIsUserJapanese() ? L"☆☆☆ ヨコのカギ ☆☆☆" : L"*** ACROSS ***";
	ret += L"\r\n";

	WCHAR text[128];
	for (auto& info : vHorzInfo) {
		StringCchPrintf(text, _countof(text), L"%d. ", info.m_number);
		ret += text;

		if (bShowAnswer) {
			ret += L'≪'; // U+226A
			ret += info.m_word;
			ret += L'≫'; // U+226B
		}

		// ヒント文章を追加する。
		auto it = word2clue.find(info.m_word);
		if (it != word2clue.end()) {
			ret += it->second;
		}

		ret += L"\r\n";   // 改行。
	}

	ret += L"\r\n";
	return ret;
}

// ヨコのカギをテキストに変換する（HTML）。
MStringW XgTextFromAcrossCluesHTML(const std::vector<MPlaceInfo>& vHorzInfo, const MStringMap& word2clue, BOOL bShowAnswer) {
	MStringW ret;
	ret += L"<p align=\"center\"><b>";
	ret += XgIsUserJapanese() ? L"ヨコのカギ" : L"ACROSS";
	ret += L"</b></p>\r\n<ol>\r\n";

	WCHAR text[128];
	for (const auto& info : vHorzInfo) {
		StringCchPrintf(text, _countof(text), L"<li value=\"%d\">", info.m_number);
		ret += text;

		// ヒント文章を追加する。
		auto it = word2clue.find(info.m_word);
		if (it != word2clue.end()) {
			ret += it->second;
		}

		ret += L"</li>\r\n";
	}

	ret += L"</ol>\r\n";
	return ret;
}

// タテのカギをテキストに変換する（HTML）。
MStringW XgTextFromDownCluesHTML(const std::vector<MPlaceInfo>& vVertInfo, const MStringMap& word2clue, BOOL bShowAnswer) {
	MStringW ret;
	ret += L"<p align=\"center\"><b>";
	ret += XgIsUserJapanese() ? L"タテのカギ" : L"DOWN";
	ret += L"</b></p>\r\n<ol>\r\n";

	WCHAR text[128];
	for (const auto& info : vVertInfo) {
		StringCchPrintf(text, _countof(text), L"<li value=\"%d\">", info.m_number);
		ret += text;

		// ヒント文章を追加する。
		auto it = word2clue.find(info.m_word);
		if (it != word2clue.end()) {
			ret += it->second;
		}

		ret += L"</li>\r\n";
	}

	ret += L"</ol>\r\n";
	return ret;
}

// 単体テスト。
void Clues_UnitTest(void) {
	std::vector<MPlaceInfo> vHorzInfo = {
		{ 0, 0, L"テストイチ", 1 },
		{ 1, 1, L"テストニ", 2 },
	};
	std::vector<MPlaceInfo> vVertInfo = {
		{ 0, 0, L"テストサン", 1 },
		{ 1, 1, L"テストヨン", 2 },
	};
	MStringMapW word2clue = {
		{ L"テストイチ", L"テストイチのテストです。" },
		{ L"テストニ", L"テストニのテストです。" },
		{ L"テストサン", L"テストサンのテストです。" },
		{ L"テストヨン", L"テストヨンのテストです。" },
	};

	// Style 0
	auto str1 = XgTextFromAcrossCluesStyle0(vHorzInfo, word2clue, TRUE);
	PrintfW(L"str1: %s\n", str1.c_str());
	auto str2 = XgTextFromDownCluesStyle0(vVertInfo, word2clue, FALSE);
	PrintfW(L"str2: %s\n", str2.c_str());

	// Style 1
	auto str3 = XgTextFromAcrossCluesStyle1(vHorzInfo, word2clue, TRUE);
	PrintfW(L"str3: %s\n", str3.c_str());
	auto str4 = XgTextFromDownCluesStyle1(vVertInfo, word2clue, FALSE);
	PrintfW(L"str4: %s\n", str4.c_str());
}
