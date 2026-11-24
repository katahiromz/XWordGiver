// algo4.cpp --- アルゴリズム 4。
// Author: katahiromz
// License: MIT

#include "algo1.h"
#include "algo2.h"
#include "algo3.h"
#include "algo4.h"
#include "dict.h"
#include "board-text-io.h"
#include "parallel-search.h"
#include "pat-file-io.h"
#include "pat-match.h"
#include <atomic>

namespace {
	// 外部からのキャンセル要求。
	std::atomic<bool> s_cancel(false);

	// 同じパターンが続けてこの回数出たら、パターンを出し尽くしたとみなして諦める。
	// (アルゴリズム 1 はランダムに1個ずつ作るので、小さい盤面では同じものが何度も出る。)
	const size_t MAX_DUPLICATE_PATTERNS = 20;

	// 規則違反(E_INVALIDARG)と判定されたパターンが続けてこの回数出たら、
	// パターンのせいではなく設定の問題とみなして、エラーとして返す。
	const size_t MAX_INVALID_PATTERNS = 10;

	// 優先タグがあるとき、必要な解の個数のこの倍数だけ候補を集め、
	// その中から優先単語を最も多く含む解を選ぶ。
	const size_t PRIORITY_OVERSAMPLE = 3;

	// 完成した盤面に含まれる優先単語(テーマ)の個数。
	size_t XgCountPriorityWords(const MDict& dict, const MBoard& board) {
		MStringSetW words;
		if (!XgScanBoardPats(board, nullptr, &words))
			return 0;
		size_t count = 0;
		for (const auto& word : words) {
			if (dict.m_priority_words.count(word) > 0)
				++count;
		}
		return count;
	}

	// 優先単語の多い順に並べ替えて、上位 keep 個だけ残す。
	void XgKeepBestByPriority(const MDict& dict, std::vector<MBoard>& solutions, size_t keep) {
		if (dict.m_priority_words.empty() || solutions.empty())
			return;
		std::vector<std::pair<size_t, size_t>> scored; // (優先単語数, 元の添字)
		scored.reserve(solutions.size());
		for (size_t i = 0; i < solutions.size(); ++i)
			scored.emplace_back(XgCountPriorityWords(dict, solutions[i]), i);
		std::stable_sort(scored.begin(), scored.end(),
			[](const std::pair<size_t, size_t>& a, const std::pair<size_t, size_t>& b) {
				return a.first > b.first;
			});
		std::vector<MBoard> best;
		best.reserve(__min(keep, solutions.size()));
		for (size_t i = 0; i < scored.size() && best.size() < keep; ++i)
			best.push_back(std::move(solutions[scored[i].second]));
		solutions = std::move(best);
	}

} // anonymous namespace

// キャンセル。
void __fastcall XgAlgo4Cancel(void) noexcept {
	s_cancel = true;
}

// アルゴリズム 4。
HRESULT __fastcall
XgAlgo4(xg_algo_quit_fn_t fnQuit, const MDict& dict, const MBoard& board, INT nMaxWordLen,
	    size_t max_solutions, BOOL bUsePatData, size_t num_threads, size_t fill_timeout_ms)
{
	assert(fnQuit);
	MStringW strError;
	std::vector<MBoard> solutions;
	const bool bPriority = !dict.m_priority_words.empty();
	HRESULT hr = XgGuarded(strError, [&]() -> HRESULT {
		s_cancel = false;
		if (max_solutions < 1)
			max_solutions = 1;

		// 優先タグがあるときは多めに集めて、後で優先単語の多い解を選ぶ。
		const size_t nWanted = bPriority ? max_solutions * PRIORITY_OVERSAMPLE : max_solutions;

		// algo2 の辞書索引を1回だけ作る(パターンごとに使い回す)。
		const auto fillDict = XgAlgo2MakeFillDict(dict);

		// PAT.txtからパターン群を読み込む。
		const INT nRules = board.Rules();
		std::vector<MBoard> patterns;
		if (bUsePatData) {
			MStringListW comments;
			hr = XgLoadPatFile(XgFindLocalFile(L"PAT.txt").c_str(), patterns, nRules, comments);
		}

		// 一致しないパターンは除外。
		patterns.erase(std::remove_if(patterns.begin(), patterns.end(), [&](const MBoard& b) {
			if (b.Columns() != board.Columns() || b.Rows() != board.Rows())
				return true;
			return !b.AreBlacksOK(nRules);
		}), patterns.end());

		XgRandomShuffle(patterns.begin(), patterns.end());

		// algo1 / algo2 の結果とエラーの受け皿(ループ内で使い回す)。
		std::vector<MBoard> generated, fills;
		MStringW strError1, strError2;

		std::unordered_set<MStringW> tried;  // 試したパターン(文字列)。
		size_t nDuplicate = 0;     // 連続して重複したパターンの数。
		size_t nInvalid = 0;       // 連続して規則違反と判定されたパターンの数。
		BOOL bAborted = FALSE;     // キャンセルされたか?
		size_t iPattern = 0;

		while (solutions.size() < nWanted) {
			if (s_cancel) {
				bAborted = TRUE;
				break;
			}

			// 1. 黒マスパターンを1個作る。
			MBoard pattern;
			if (bUsePatData && iPattern < patterns.size()) {
				pattern = patterns[iPattern++];
			} else {
				hr = XgAlgo1Generate(dict, board, nMaxWordLen, s_cancel, generated, strError1, TRUE, 1, num_threads, 0);
				if (hr == E_ABORT) {
					bAborted = TRUE;
					break;
				}
				if (hr == S_FALSE)
					break; // パターンが存在しない。
				if (FAILED(hr)) {
					strError = strError1;
					return hr;
				}
				if (generated.empty())
					continue;
				pattern = std::move(generated[0]);
			}

			// 試したことのあるパターンは飛ばす。
			MStringW text;
			XgTextFromBoard(text, pattern);
			if (!tried.insert(text).second) {
				if (++nDuplicate >= MAX_DUPLICATE_PATTERNS)
					break; // もう新しいパターンは出ない。
				continue;
			}
			nDuplicate = 0;

			// 2. そのパターンを文字で埋める(制限時間つき)。
			hr = XgAlgo2Fill(*fillDict, pattern, s_cancel, fills, strError2, 1, num_threads, fill_timeout_ms);
			if (hr == S_OK) {
				nInvalid = 0;
				if (!fills.empty())
					solutions.push_back(std::move(fills[0]));
				continue;
			}
			if (hr == E_ABORT) {
				bAborted = TRUE;
				break;
			}
			if (hr == S_FALSE || hr == XG_E_TIMEOUT) {
				nInvalid = 0;
				continue; // 3. 埋められなかった。別のパターンへ。
			}
			if (hr == E_INVALIDARG) {
				if (++nInvalid >= MAX_INVALID_PATTERNS) {
					strError = strError2;
					return hr;
				}
				continue;
			}
			strError = strError2; // メモリ不足など。
			return hr;
		}

		if (!solutions.empty())
			hr = S_OK; // 1個でも解があれば成功。
		else if (bAborted)
			hr = E_ABORT;
		else
			hr = S_FALSE;

		return hr;
	});
	// 優先単語を多く含む解を前にして、必要な個数に絞る(エラー終了時も同様)。
	if (bPriority)
		XgKeepBestByPriority(dict, solutions, max_solutions);
	fnQuit(hr, solutions, max_solutions, strError);
	return hr;
}

// 単体テスト。
void __fastcall XgAlgo4UnitTest(void) {
	enum {
		test_rules = RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | \
		             RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE |
		             RULE_DONTTHREEDIAGONALS
	};
	// 盤面の大きさとルールだけを与える(中身は白紙)。
	MBoard board = XgMakeBoard({
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　",
		L"　　　　　　　　　　　　　　　　", }, test_rules);

	auto quit_fn = [&](HRESULT hr, std::vector<MBoard>& sols, size_t max_sols, MStringW& error) -> void {
		std::vector<MBoard> solutions = std::move(sols);
		MStringW strError = std::move(error);
		if (hr == E_ABORT) { // キャンセルされた。
			PrintfW(L"XgAlgo4 aborted (canceled)\n");
		} else if (hr == S_FALSE) { // 解がなかった。
			PrintfW(L"XgAlgo4 cannot solve\n");
		} else if (FAILED(hr)) { // タイムアウトなどの失敗。
			PrintfW(L"XgAlgo4 failed: %s\n", strError.c_str());
		} else { // 成功。
			PrintfW(L"XgAlgo4 OK (%d solutions)\n", (INT)solutions.size());
			for (auto& solution : solutions) {
				MStringW text;
				XgTextFromBoard(text, solution);
				PrintfW(L"%s\n", text.c_str());
			}
		}
	};

	PrintfW(L"XgAlgo4 start\n");
	DWORD tick0 = GetTickCount();
	MDict dict;
	HRESULT hr = XgLoadKanaDict(dict);
	if (FAILED(hr)) {
		PrintfW(L"No kana dictionary\n");
	}
	hr = XgAlgo4(quit_fn, dict, board, 5, 1, TRUE);
	DWORD tick1 = GetTickCount();
	PrintfW(L"XgAlgo4 end: %u ms\n", (unsigned)(tick1 - tick0));
}
