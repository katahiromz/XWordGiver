// algo3.cpp --- アルゴリズム 3(アルゴリズム 1 + アルゴリズム 2)。
// Author: katahiromz
// License: MIT
//
// 方針:
//   ・黒マスパターンの生成(algorithm1)と、文字の充填(algorithm2)を、順番に繰り返す。
//     それぞれが内部で複数スレッドを使うので、フェーズ間の並列化はしない(全スレッドを各フェーズに使う)。
//   ・辞書は1回だけ読み込み、algorithm2 の索引も1回だけ作って、パターンごとに使い回す。
//   ・algorithm2 は、埋められないパターンだと再起動を繰り返して終わらないことがある。
//     そのため 1 パターンごとに制限時間(fill_timeout_ms)を設け、超えたら次のパターンへ行く。
//   ・キャンセルは algorithm3 自身のフラグ(s_cancel)を両フェーズに渡す。
//     フェーズの切り替わり時にキャンセルを取りこぼさない。

#include "algo1.h"
#include "algo2.h"
#include "algo3.h"
#include "dict.h"
#include "parallel-search.h"
#include <atomic>

size_t xg_nPatternsTried3 = 0;      // 埋めてみたパターンの個数(診断用)。

namespace {

	// 外部からのキャンセル要求。
	std::atomic<bool> s_cancel(false);

	// 同じパターンが続けてこの回数出たら、パターンを出し尽くしたとみなして諦める。
	// (アルゴリズム 1 はランダムに1個ずつ作るので、小さい盤面では同じものが何度も出る。)
	const size_t MAX_DUPLICATE_PATTERNS = 20;

	// 規則違反(E_INVALIDARG)と判定されたパターンが続けてこの回数出たら、
	// パターンのせいではなく設定の問題とみなして、エラーとして返す。
	const size_t MAX_INVALID_PATTERNS = 10;

} // anonymous namespace

// キャンセル。
void __fastcall XgAlgo3Cancel(void) noexcept {
	s_cancel = true;
}

// アルゴリズム 3。
HRESULT __fastcall XgAlgo3(const MDict& dict, const MBoard& board, INT nMaxWordLen,
                           std::vector<MBoard>& solutions, MStringW& strError, BOOL bKeepBoard,
                           size_t max_solutions, size_t num_threads, size_t fill_timeout_ms)
{
	// 例外は XgGuarded が HRESULT と strError に変換する。
	return XgGuarded(strError, [&]() -> HRESULT {
		solutions.clear();
		strError.clear();
		xg_nPatternsTried3 = 0;
		s_cancel = false;
		if (max_solutions < 1)
			max_solutions = 1;

		// algo2 の辞書索引を1回だけ作る(パターンごとに使い回す)。
		const auto fillDict = XgAlgo2MakeFillDict(dict);

		std::set<MStringW> tried;  // 試したパターン(文字列)。
		size_t nDuplicate = 0;     // 連続して重複したパターンの数。
		size_t nInvalid = 0;       // 連続して規則違反と判定されたパターンの数。
		BOOL bAborted = FALSE;     // キャンセルされたか?

		// 各フェーズの結果とエラーの受け皿(ループ内で使い回す)。
		std::vector<MBoard> patterns, fills;
		MStringW strError1, strError2;

		while (solutions.size() < max_solutions) {
			if (s_cancel) {
				bAborted = TRUE;
				break;
			}

			// 1. 黒マスパターンを1個作る。
			HRESULT hr = XgAlgo1Generate(dict, board, nMaxWordLen, s_cancel, patterns, strError1, bKeepBoard, 1, num_threads);
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
			if (patterns.empty())
				continue;
			const MBoard& pattern = patterns[0];

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
			++xg_nPatternsTried3;
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
			return S_OK; // 1個でも解があれば成功。
		if (bAborted)
			return E_ABORT;
		return S_FALSE;
	});
}

// 単体テスト。
void __fastcall XgAlgo3UnitTest(void) {
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

	PrintfW(L"XgAlgo3 start\n");
	DWORD tick0 = GetTickCount();
	std::vector<MBoard> solutions;
	MStringW strError;
	MDict dict;
	HRESULT hr = XgLoadKanaDict(dict);
	if (FAILED(hr)) {
		PrintfW(L"No kana dictionary\n");
	}
	hr = XgAlgo3(dict, board, 5, solutions, strError, FALSE, 1, 0, 10000);
	DWORD tick1 = GetTickCount();
	PrintfW(L"XgAlgo3 end: %u ms, %u pattern(s) tried\n",
	       (unsigned)(tick1 - tick0), (unsigned)xg_nPatternsTried3);
	if (hr == E_ABORT) { // キャンセルされた。
		PrintfW(L"XgAlgo3 aborted (canceled)\n");
	} else if (hr == S_FALSE) { // 新しいパターンがなかった。
		PrintfW(L"XgAlgo3 cannot solve\n");
	} else if (FAILED(hr)) { // その他の失敗。
		PrintfW(L"XgAlgo3 failed: %s\n", strError.c_str());
	} else { // 成功。
		PrintfW(L"XgAlgo3 OK (%d solutions)\n", (INT)solutions.size());
		for (auto& solution : solutions) {
			MStringW text;
			XgTextFromBoard(text, solution);
			PrintfW(L"%s\n", text.c_str());
		}
	}
}
