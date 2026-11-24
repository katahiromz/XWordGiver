// algo1.cpp --- アルゴリズム 1。
// Author: katahiromz
// License: MIT

#include "algo1.h"
#include "dict.h"
#include "parallel-search.h"
#include <atomic>
#include <thread>

namespace {

	// 外部からのキャンセル要求。
	std::atomic<bool> s_cancel(false);

	// 辞書の単語の長さの分布。構築後は読み取り専用で、全スレッドが共有する。
	//   ・辞書に1語もない長さの白連続は作らない(長さ 0, 1 は単語にならないので常に許す)。
	//   ・白連続を黒で閉じる確率は、辞書の長さの分布に従わせる(ハザード率)。
	//	   hazard[L] = (長さ L の単語数) / (長さ L 以上の単語数)
	//	 例えば長さ 3 の単語しかなければ、3 マス続いたところで必ず黒になる。
	struct MLenStat {
		INT nMaxLen = 0;				// 使える単語の最大長(辞書と nMaxWordLen の小さい方)。
		std::vector<BOOL> valid;		// valid[L]: 長さ L の白連続を許すか。
		std::vector<double> hazard;	 // hazard[L]: 長さ L の白連続を黒で閉じる確率。

		// 長さ nLen の白連続は、黒マスや盤面の端で閉じてよいか?
		BOOL IsValid(INT nLen) const {
			if (nLen <= 1)
				return TRUE;
			return nLen <= nMaxLen && valid[nLen];
		}
		// 長さ nLen の白連続を黒で閉じる確率。
		double Hazard(INT nLen) const {
			return (nLen > nMaxLen) ? 1.0 : hazard[nLen];
		}
	};

	// 辞書から MLenStat を作る。除外タグの単語は数えない。
	MLenStat XgMakeLenStat(const MDict& dict, INT nMaxWordLen) {
		// 長さごとの単語数。
		std::vector<size_t> count;
		for (const auto& word : dict.m_word_list) {
			const size_t len = word.size();
			if (len < 2 || len > (size_t)nMaxWordLen)
				continue;
			if (dict.m_forbidden_words.count(word) > 0)
				continue;
			if (count.size() <= len)
				count.resize(len + 1);
			++count[len];
		}

		MLenStat stat;
		if (count.empty())
			return stat; // 使える単語がない(nMaxLen == 0)。

		stat.nMaxLen = (INT)count.size() - 1;
		stat.valid.assign(count.size(), FALSE);
		stat.hazard.assign(count.size(), 0.0);

		// 長さ 0(黒が連続する)は避け、長さ 1 も少しだけ許す。調整したい場合はここを変える。
		stat.hazard[0] = 0.0;
		stat.hazard[1] = 0.1;

		size_t tail = 0; // 長さ L 以上の単語数。
		for (INT len = stat.nMaxLen; len >= 2; --len) {
			tail += count[len];
			stat.valid[len] = (count[len] > 0);
			stat.hazard[len] = tail ? (double)count[len] / (double)tail : 1.0;
		}
		return stat;
	}

	// 1ワーカー分の探索状態(再起動・乱数・停止判定は MSolverBase が持つ)。
	class MSolver : public MSolverBase {
	public:
		MSolver(MSearchShared& shared, const MBoard& board, const MLenStat& len, size_t id)
			// 1回の試行に最低でも全マス分のノードが必要なので、盤面サイズに比例させる。
			: MSolverBase(shared, id, (size_t)board.Rows() * board.Columns() * 20)
			, m_len(len) { }

		HRESULT Run(const MBoard& board) {
			return RunWithRestarts([&] { return Recurse(board, 0); });
		}

	private:
		const MLenStat& m_len; // 辞書の単語長の分布。

		// 次のマスへ。
		HRESULT Next(const MBoard& b, INT jCol, INT iRow) {
			if (jCol + 1 < b.Columns())
				return Recurse(b, MAKELONG(jCol + 1, iRow));
			return Recurse(b, MAKELONG(0, iRow + 1));
		}
		// 黒マスを置いて次へ(置けなければ S_FALSE)。
		HRESULT PutBlack(const MBoard& b, INT jCol, INT iRow) {
			if (!b.MaybeCanPutBlack(jCol, iRow))
				return S_FALSE;
			MBoard copy(b);
			copy.MirroredSetCell(jCol, iRow, ZEN_BLACK);

			// 黒にしたことで、他のマスが書き換わっていないか確認する。
			//   ・すでに白と決めた手前のマスが書き換わるなら置けない。
			//	 (書き換えると、辞書の長さを確認済みの白連続が壊れてしまう。)
			//	 この場合、このマスは対称位置に合わせて白になる。
			//   ・文字マスが黒で上書きされるなら置けない。
			const INT nCols = b.Columns(), nRows = b.Rows();
			for (INT i = 0; i < nRows; ++i) {
				for (INT j = 0; j < nCols; ++j) {
					if (copy.GetCell0(j, i) == b.GetCell0(j, i))
						continue;
					if (j == jCol && i == iRow)
						continue; // このマス自身。
					const BOOL bEarlier = (i < iRow) || (i == iRow && j < jCol);
					if (bEarlier || XgIsLetter(b.GetCell0(j, i)))
						return S_FALSE;
				}
			}
			return Next(copy, jCol, iRow);
		}

		// 再帰。スタックを節約するため、引数をまとめる。
		HRESULT Recurse(const MBoard& board, INT jCol_iRow) {
			const INT jCol = LOWORD(jCol_iRow), iRow = HIWORD(jCol_iRow);

			// 停止要求なら E_ABORT、ノード数が上限を超えたら XG_E_RESTART。
			HRESULT hrTick = Tick();
			if (FAILED(hrTick))
				return hrTick;

			// ルールの適合性をチェックする。
			const INT nRules = board.Rules();
			if ((nRules & RULE_DONTCORNERBLACK) && board.CornerBlack())
				return S_FALSE;
			if ((nRules & RULE_DONTDOUBLEBLACK) && board.DoubleBlack())
				return S_FALSE;
			if ((nRules & RULE_DONTTRIDIRECTIONS) && board.TriBlackAround())
				return S_FALSE;
			if ((nRules & RULE_DONTDIVIDE) && board.DividedByBlack())
				return S_FALSE;
			if (nRules & RULE_DONTTHREEDIAGONALS) {
				if (board.ThreeDiagonals())
					return S_FALSE;
			} else if (nRules & RULE_DONTFOURDIAGONALS) {
				if (board.FourDiagonals())
					return S_FALSE;
			}

			const INT nRows = board.Rows();
			if (jCol == 0 && iRow == nRows) {
				return m_shared.AddSolution(board) ? S_OK : S_FALSE;
			}

			// このマスの左と上に続く白連続の長さ(このマスは含まない)。
			INT nLeft = 0, nUp = 0;
			while (nLeft < jCol && board.GetCell0(jCol - 1 - nLeft, iRow) != ZEN_BLACK)
				++nLeft;
			while (nUp < iRow && board.GetCell0(jCol, iRow - 1 - nUp) != ZEN_BLACK)
				++nUp;

			// 対称位置からすでに黒になっているマスは、素通りする。
			// ただし、ここで閉じる白連続の長さが辞書にない長さなら失敗。
			if (board.GetCell0(jCol, iRow) == ZEN_BLACK) {
				if (!m_len.IsValid(nLeft) || !m_len.IsValid(nUp))
					return S_FALSE;
				return Next(board, jCol, iRow);
			}

			// 文字マス(確定した白)は黒にできない。
			const BOOL bFixedWhite = XgIsLetter(board.GetCell0(jCol, iRow));

			// 白にできるか? 白連続が長すぎず、盤面の端で閉じる場合は辞書にある長さであること。
			BOOL bCanWhite = (nLeft + 1 <= m_len.nMaxLen && nUp + 1 <= m_len.nMaxLen);
			if (jCol == board.Columns() - 1 && !m_len.IsValid(nLeft + 1))
				bCanWhite = FALSE;
			if (iRow == nRows - 1 && !m_len.IsValid(nUp + 1))
				bCanWhite = FALSE;
			// 先読み: すぐ右/下がすでに黒マスなら、ここで白連続が閉じる。
			if (jCol + 1 < board.Columns() &&
				board.GetCell0(jCol + 1, iRow) == ZEN_BLACK && !m_len.IsValid(nLeft + 1))
				bCanWhite = FALSE;
			if (iRow + 1 < nRows &&
				board.GetCell0(jCol, iRow + 1) == ZEN_BLACK && !m_len.IsValid(nUp + 1))
				bCanWhite = FALSE;
			// 黒にできるか? 文字マスでなく、左と上の白連続が、ここで辞書にある長さで閉じること。
			const BOOL bCanBlack = !bFixedWhite &&
								   m_len.IsValid(nLeft) && m_len.IsValid(nUp);

			if (!bCanWhite && !bCanBlack)
				return S_FALSE; // 行き止まり。
			if (!bCanWhite)
				return PutBlack(board, jCol, iRow);
			if (!bCanBlack)
				return Next(board, jCol, iRow); // 白のまま

			// 自由に選べる: 辞書の長さの分布に従って、黒を先に試すかどうか決める
			// (スレッドごとの m_rng を使う)。黒は縦横の白連続を同時に閉じるので、
			// どちらか一方が閉じたい長さなら閉じる(平均や積だと、長い連続が出すぎる)。
			const double pBlack = __max(m_len.Hazard(nLeft), m_len.Hazard(nUp));
			std::uniform_real_distribution<double> dist(0.0, 1.0);
			HRESULT hr;
			if (dist(m_rng) < pBlack) {
				hr = PutBlack(board, jCol, iRow);
				if (hr != S_FALSE) return hr;
				return Next(board, jCol, iRow);
			} else {
				hr = Next(board, jCol, iRow); // 白のまま
				if (hr != S_FALSE) return hr;
				return PutBlack(board, jCol, iRow);
			}
		}
	};

	// 探索の本体。
	HRESULT XgAlgo1Search(const MBoard& board, const MLenStat& len,
						  const std::atomic<bool>& cancel,
						  std::vector<MBoard>& solutions, MStringW& strError,
						  size_t max_solutions, size_t num_threads, size_t timeout_ms)
	{
		MSearchShared shared(max_solutions);
		HRESULT hr = XgParallelSearch(shared, board, num_threads, cancel, timeout_ms,
			[&](size_t id) { return MSolver(shared, board, len, id); });

		solutions = std::move(shared.solutions); // コピーなし
		if (hr == E_OUTOFMEMORY)
			strError = L"memory";
		return hr;
	}

} // anonymous namespace

// キャンセル。
void __fastcall XgAlgo1Cancel(void) noexcept {
	s_cancel = true;
}

// 黒マスパターンの生成(辞書を読み込み済みの版。algorithm3 から繰り返し呼ぶ用)。
// 辞書の単語の長さに合わせて、辞書にある長さの白連続だけができるようにする。
//   ・辞書は読み込まない。内部のキャンセルフラグも触らず、cancel を見る。
//   ・timeout_ms が 0 以外なら、その時間を超えたら諦めて XG_E_TIMEOUT を返す。
//   ・結果は solutions に、エラーメッセージは strError に入る。
//   ・bKeepBoard が TRUE なら、board の黒マスと文字マスを保ったまま探索する。
HRESULT __fastcall XgAlgo1Generate(const MDict& dict, const MBoard& board, INT nMaxWordLen,
								   const std::atomic<bool>& cancel,
								   std::vector<MBoard>& solutions, MStringW& strError, BOOL bKeepBoard,
								   size_t max_solutions, size_t num_threads, size_t timeout_ms)
{
	// 例外は XgGuarded が HRESULT と strError に変換する。
	return XgGuarded(strError, [&]() -> HRESULT {
		solutions.clear();
		strError.clear();

		MBoard copy(board);
		if (!bKeepBoard) {
			copy.clear();
		} else {
			// 事前の黒マスを対称位置へ反映する。
			// 対称位置に文字マスがあって黒で上書きされるなら、矛盾なので解なし。
			const INT nCols = board.Columns(), nRows = board.Rows();
			for (INT i = 0; i < nRows; ++i) {
				for (INT j = 0; j < nCols; ++j) {
					if (board.GetCell0(j, i) != ZEN_BLACK)
						continue;
					const MBoard before(copy);
					copy.MirroredSetCell(j, i, ZEN_BLACK);
					for (INT i2 = 0; i2 < nRows; ++i2) {
						for (INT j2 = 0; j2 < nCols; ++j2) {
							const WCHAR chOld = before.GetCell0(j2, i2);
							if (XgIsLetter(chOld) && copy.GetCell0(j2, i2) != chOld)
								return S_FALSE;
						}
					}
				}
			}
		}

		const MLenStat len = XgMakeLenStat(dict, nMaxWordLen);
		if (len.nMaxLen < 2)
			throw MInvalidArgument { L"dict" }; // 使える単語がない。

		return XgAlgo1Search(copy, len, cancel, solutions, strError,
			(max_solutions < 1) ? 1 : max_solutions, num_threads, timeout_ms);
	});
}

// アルゴリズム 1(黒マス配置の生成)。
// bKeepBoard が TRUE なら、board の黒マスと文字マスを保ったまま探索する。
// nMaxWordLen は単語の最大長(辞書にもっと長い単語があっても、これを超える長さは作らない)。
HRESULT __fastcall XgAlgo1(const MDict& dict, const MBoard& board, INT nMaxWordLen,
						   std::vector<MBoard>& solutions, MStringW& strError,
						   BOOL bKeepBoard, size_t max_solutions, size_t num_threads)
{
	return XgGuarded(strError, [&]() -> HRESULT {
		s_cancel = false;
		solutions.clear();
		strError.clear();

		return XgAlgo1Generate(dict, board, nMaxWordLen, s_cancel, solutions, strError, bKeepBoard,
							   max_solutions, num_threads, 0);
	});
}

// 単体テスト。
void __fastcall XgAlgo1UnitTest(void) {
	enum {
		test_rules = RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | \
					 RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE |
					 RULE_DONTTHREEDIAGONALS
	};
#if 1
	MBoard board = XgMakeBoard({
		L"　　■　■　■　　　　■　■　　■　■　　■　",
		L"　■　　　　　■　■　　　　■　　　　■　　　",
		L"　　　　■　　　　　■　■　　　　■　　　■　",
		L"■　■　　■　■　　　　　■　　　　■　■　　",
		L"　　　■　　　　　■　■　　　　■　　　　　■",
		L"■　　　　■　　■　　　　■　■　　　■　■　",
		L"　　■　　　■　　　　　■　　　　■　　　　　",
		L"　■　　■　　　■　　　　　■　　　■　　■　",
		L"　　　　　■　　　　■　　　　　■　　　■　　",
		L"　■　■　　　■　■　　　　■　　■　　　　■",
		L"■　　　　　■　　　　■　■　　　　　■　　　",
		L"　　■　■　　　　■　　　　　■　■　　■　■",
		L"　■　　　■　　　　■　■　　　　　■　　　　",
		L"　　　■　　　　■　　　　■　■　　　　　■　",
		L"　■　　■　■　　■　■　　　　■　■　■　　", }, test_rules);
#elif 0
	MBoard board = XgMakeBoard({
		L"　　■　■　　　■　　　■　　",
		L"　　　　　■　　　　　■　　■",
		L"■　　■　　　　　■　　　■　",
		L"　■　　　　　■　　　■　　　",
		L"　　　■　■　　■　■　　　■",
		L"■　■　　　■　　　　　■　　",
		L"　　　　■　　　　■　　　　■",
		L"　■　■　　　■　　　■　■　",
		L"■　　　　■　　　　■　　　　",
		L"　　■　　　　　■　　　■　■",
		L"■　　　■　■　　■　■　　　",
		L"　　　■　　　■　　　　　■　",
		L"　■　　　■　　　　　■　　■",
		L"■　　■　　　　　■　　　　　",
		L"　　■　　　■　　　■　■　　", }, test_rules);
#elif 0
	MBoard board = XgMakeBoard({
		L"　■　■　　■　　　",
		L"　　　　　■　　■　",
		L"■　　■　　　　　■",
		L"　　　　■　■　　　",
		L"　　■　　　　　■　",
		L"　■　　　　　■　　",
		L"　　　■　■　　　　",
		L"■　　　　　■　　■",
		L"　■　　■　　　　　",
		L"　　　■　　■　■　", }, test_rules);
#else
	MBoard board = XgMakeBoard({
		L".#..#...",
		L"..#...#.",
		L"....#...",
		L".#.#..#." }, test_rules);
#endif

	// 1回分のテスト実行。
	auto run = [](const MBoard& board, BOOL bKeepBoard) {
		PrintfW(L"XgAlgo1 start (threads: %u, keep: %d)\n",
				(unsigned)std::thread::hardware_concurrency(), (INT)bKeepBoard);
		DWORD tick0 = GetTickCount();
		std::vector<MBoard> solutions;
		MStringW strError;
		MDict dict;
		HRESULT hr = XgLoadKanaDict(dict);
		if (FAILED(hr)) {
			PrintfW(L"No kana dictionary\n");
		}
		hr = XgAlgo1(dict, board, 5, solutions, strError, bKeepBoard, 1, 0);
		DWORD tick1 = GetTickCount();
		PrintfW(L"XgAlgo1 end: %d\n", (tick1 - tick0) / 1000);
		if (hr == E_ABORT) { // キャンセルされた。
			PrintfW(L"XgAlgo1 aborted (canceled)\n");
		} else if (hr == S_FALSE) { // 全探索したが解が見つからなかった。
			PrintfW(L"XgAlgo1 cannot solve\n");
		} else if (FAILED(hr)) { // その他の失敗。
			PrintfW(L"XgAlgo1 failed: %s\n", strError.c_str());
		} else { // 成功。
			PrintfW(L"XgAlgo1 OK (%d solutions)\n", (INT)solutions.size());
			for (auto& solution : solutions) {
				MStringW text;
				XgTextFromBoard(text, solution);
				PrintfW(L"%s\n", text.c_str());
			}
		}
	};

	// (1) 空の盤面から探索する(board のパターンは捨てられる)。
	run(board, FALSE);

	// (2) 黒マスと文字マスを保ったまま探索する。
	MBoard board2 = XgMakeBoard({
		L"ア　　　　　　　　　",
		L"　■　　　　　　　　",
		L"　　　　　　　　　　",
		L"　　　　　　　　■　",
		L"　　　　　　　　　　",
		L"　　　　　　　　　　", }, test_rules);
	run(board2, TRUE);
}
