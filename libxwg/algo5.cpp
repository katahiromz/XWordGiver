// algo5.cpp --- アルゴリズム 5。
// Author: katahiromz
// License: MIT
//
// 方針:
//   ・最も長い単語を原点に置き、残りの単語を「すでに置いた文字と交差するように」1個ずつ足していく。
//   ・新しく足す単語は、(1) 既存の文字と一致して交差する、(2) 前後のマスが文字でない、
//     (3) 新しく文字を置くマスの左右(または上下)が文字でない、(4) 既存の単語と重ならない、を満たす。
//     つまり、単語どうしは必ず「交差」でだけ接する(平行に隣り合う単語は作らない)。
//   ・bUseAllWords が TRUE なら、全単語を置き終えたら、文字のないマスを黒マスにして盤面を作り、
//     黒マスルールを検査する。
//   ・bUseAllWords が FALSE なら、単語は使い切らなくてよい。もうこれ以上置けない盤面(極大)を解とし、
//     黒マスルールを満たせなければ、単語を減らした盤面を解とする(使える単語をなるべく多く使う)。
//   ・探索は parallel-search.h の再起動つき並列探索で行う。

#include "algo5.h"
#include "parallel-search.h"
#include <atomic>
#include <algorithm>

namespace {
	// 外部からのキャンセル要求。
	std::atomic<bool> s_cancel(false);

	// 再起動1回あたりのノード数の基準(Luby 列を掛ける)。
	const size_t BASE_NODE_LIMIT = 3000;

	// 候補。単語の番号、先頭マスの位置、向き、交差する個数。
	struct MCand {
		size_t m_word;
		INT m_x, m_y;
		BOOL m_vertical;
		INT m_cross;
	};

	// 置かれた範囲。
	struct MBounds {
		INT m_x0, m_y0, m_x1, m_y1; // 両端を含む。
	};

	// 1ワーカー分のソルバー。
	class MSolver5 : public MSolverBase {
	public:
		MSolver5(MSearchShared& shared, size_t id, const MStringListW& words, INT rules,
		         BOOL bUseAllWords)
			: MSolverBase(shared, id, BASE_NODE_LIMIT)
			, m_words(words)
			, m_rules(rules)
			, m_bUseAllWords(bUseAllWords)
			, m_placed(words.size(), 0)
		{
		}

		HRESULT Run(const MBoard&) {
			return RunWithRestarts([&] { return Search(); });
		}

	private:
		const MStringListW& m_words;          // 0番目が最も長い単語(根)。
		const INT m_rules;
		const BOOL m_bUseAllWords;            // 全単語を使うか？
		size_t m_nValid = 0;                  // 黒マスルールを満たした盤面を見つけた回数。
		std::vector<char> m_placed;           // 置いた単語か？

		// 疎な盤面(置いた文字だけを持つ。ないマスは空白)。
		std::unordered_map<uint32_t, WCHAR> m_grid;
		struct MChange { uint32_t m_key; WCHAR m_old; bool m_existed; };
		std::vector<MChange> m_log;           // 巻き戻し用。
		MBounds m_bounds = { 0, 0, 0, 0 };

		static uint32_t Key(INT x, INT y) noexcept {
			return (uint32_t(x + 0x8000) & 0xFFFF) << 16 | (uint32_t(y + 0x8000) & 0xFFFF);
		}
		static INT KeyX(uint32_t key) noexcept { return INT(key >> 16) - 0x8000; }
		static INT KeyY(uint32_t key) noexcept { return INT(key & 0xFFFF) - 0x8000; }

		WCHAR Get(INT x, INT y) const {
			auto it = m_grid.find(Key(x, y));
			return (it == m_grid.end()) ? WCHAR(ZEN_SPACE) : it->second;
		}

		void Set(INT x, INT y, WCHAR ch) {
			const uint32_t key = Key(x, y);
			auto it = m_grid.find(key);
			if (it == m_grid.end()) {
				m_log.push_back({ key, 0, false });
				m_grid.emplace(key, ch);
			} else {
				m_log.push_back({ key, it->second, true });
				it->second = ch;
			}
		}

		void Rollback(size_t mark) {
			while (m_log.size() > mark) {
				const MChange c = m_log.back();
				m_log.pop_back();
				if (c.m_existed)
					m_grid[c.m_key] = c.m_old;
				else
					m_grid.erase(c.m_key);
			}
		}

		// 単語を置けるか判定する。置けるなら交差する個数(0以上)、置けないなら -1。
		INT Fits(const MStringW& word, INT sx, INT sy, BOOL vertical) const {
			const INT dx = vertical ? 0 : 1, dy = vertical ? 1 : 0;
			const INT n = INT(word.size());

			// 単語の前後のマスは文字であってはならない。
			if (XgIsLetter(Get(sx - dx, sy - dy)) || XgIsLetter(Get(sx + dx * n, sy + dy * n)))
				return -1;

			// 盤面の最大サイズを超えない。
			const INT ex = sx + dx * (n - 1), ey = sy + dy * (n - 1);
			const INT nx0 = __min(m_bounds.m_x0, sx), ny0 = __min(m_bounds.m_y0, sy);
			const INT nx1 = __max(m_bounds.m_x1, ex), ny1 = __max(m_bounds.m_y1, ey);
			if (nx1 - nx0 + 1 > XG_MAX_SIZE || ny1 - ny0 + 1 > XG_MAX_SIZE)
				return -1;

			INT cross = 0;
			bool prevExisted = false;
			for (INT i = 0; i < n; ++i) {
				const INT cx = sx + dx * i, cy = sy + dy * i;
				const WCHAR ch = Get(cx, cy);
				if (XgIsLetter(ch)) { // 既存の文字と交差する。
					if (ch != word[i])
						return -1;
					if (prevExisted) // 既存の単語と重なってしまう。
						return -1;
					prevExisted = true;
					++cross;
				} else { // 新しく文字を置くマス。
					prevExisted = false;
					// 直交方向の両隣が文字だと、意図しない単語ができてしまう。
					if (XgIsLetter(Get(cx + dy, cy + dx)) || XgIsLetter(Get(cx - dy, cy - dx)))
						return -1;
				}
			}
			return cross;
		}

		// 候補を置く。
		void Place(const MCand& c) {
			const MStringW& word = m_words[c.m_word];
			const INT dx = c.m_vertical ? 0 : 1, dy = c.m_vertical ? 1 : 0;
			const INT n = INT(word.size());
			for (INT i = 0; i < n; ++i)
				Set(c.m_x + dx * i, c.m_y + dy * i, word[i]);
			m_bounds.m_x0 = __min(m_bounds.m_x0, c.m_x);
			m_bounds.m_y0 = __min(m_bounds.m_y0, c.m_y);
			m_bounds.m_x1 = __max(m_bounds.m_x1, c.m_x + dx * (n - 1));
			m_bounds.m_y1 = __max(m_bounds.m_y1, c.m_y + dy * (n - 1));
		}

		// 未使用の単語を、既存の文字と交差させる候補をすべて集める。
		void CollectCandidates(std::vector<MCand>& cands) const {
			for (auto& kv : m_grid) {
				const WCHAR ch = kv.second;
				if (!XgIsLetter(ch))
					continue;
				const INT x = KeyX(kv.first), y = KeyY(kv.first);
				for (size_t w = 0; w < m_words.size(); ++w) {
					if (m_placed[w])
						continue;
					const MStringW& word = m_words[w];
					for (size_t k = 0; k < word.size(); ++k) {
						if (word[k] != ch)
							continue;
						for (int v = 0; v < 2; ++v) {
							const BOOL vertical = (v != 0);
							const INT sx = vertical ? x : x - INT(k);
							const INT sy = vertical ? y - INT(k) : y;
							const INT cross = Fits(word, sx, sy, vertical);
							if (cross > 0)
								cands.push_back({ w, sx, sy, vertical, cross });
						}
					}
				}
			}
		}

		// 盤面ができた。盤面にして黒マスルールを検査し、解として登録する。
		HRESULT Finish() {
			const INT w = __max(m_bounds.m_x1 - m_bounds.m_x0 + 1, XG_MIN_SIZE);
			const INT h = __max(m_bounds.m_y1 - m_bounds.m_y0 + 1, XG_MIN_SIZE);
			MBoard board(w, h, m_rules, ZEN_BLACK); // 文字のないマスは黒マス。
			for (auto& kv : m_grid) {
				if (XgIsLetter(kv.second))
					board.SetCell(KeyX(kv.first) - m_bounds.m_x0, KeyY(kv.first) - m_bounds.m_y0, kv.second);
			}
			if (!board.AreBlacksOK(m_rules))
				return S_FALSE;
			++m_nValid;
			return m_shared.AddSolution(board) ? S_OK : S_FALSE;
		}

		// 再帰探索。S_OK=必要な解が揃った / S_FALSE=この枝は失敗 / E_ABORT / XG_E_RESTART。
		HRESULT Recurse(size_t nPlaced) {
			HRESULT hr;
			if (FAILED(hr = Tick()))
				return hr;

			// 全単語を置き終えた。
			if (nPlaced == m_words.size())
				return Finish();

			std::vector<MCand> cands;
			CollectCandidates(cands);
			if (cands.empty()) // もう置けない。
				return m_bUseAllWords ? S_FALSE : Finish();

			const size_t nValid0 = m_nValid;

			// 同じ配置を複数の交差マスから見つけたものを除去する。
			auto key = [](const MCand& c) { return std::make_tuple(c.m_word, c.m_x, c.m_y, c.m_vertical); };
			std::sort(cands.begin(), cands.end(), [&](const MCand& a, const MCand& b) { return key(a) < key(b); });
			cands.erase(std::unique(cands.begin(), cands.end(), [&](const MCand& a, const MCand& b) {
				return key(a) == key(b);
			}), cands.end());

			// ランダムに並べたうえで、交差の多い(=コンパクトな)候補を先に試す。
			std::shuffle(cands.begin(), cands.end(), m_rng);
			std::stable_sort(cands.begin(), cands.end(), [](const MCand& a, const MCand& b) {
				return a.m_cross > b.m_cross;
			});

			for (auto& c : cands) {
				const size_t mark = m_log.size();
				const MBounds bounds = m_bounds;
				Place(c);
				m_placed[c.m_word] = 1;
				hr = Recurse(nPlaced + 1);
				m_placed[c.m_word] = 0;
				Rollback(mark);
				m_bounds = bounds;
				if (hr != S_FALSE)
					return hr;
			}

			// 単語を使い切らなくてよい場合、これ以上単語を足した盤面がルールに合わなかったときは、
			// この盤面自身を解の候補にする(足した盤面がルールに合っていたなら、そちらを優先)。
			if (!m_bUseAllWords && m_nValid == nValid0)
				return Finish();
			return S_FALSE;
		}

		// 1回分の探索。根の単語をヨコ・タテの両方で試す(順序はランダム)。
		HRESULT Search() {
			const bool first = (m_rng() & 1) != 0;
			for (int t = 0; t < 2; ++t) {
				m_grid.clear();
				m_log.clear();
				std::fill(m_placed.begin(), m_placed.end(), 0);
				m_bounds = { 0, 0, 0, 0 };

				MCand root = { 0, 0, 0, (t == 0) ? first : !first, 0 };
				Place(root);
				m_placed[0] = 1;

				HRESULT hr = Recurse(1);
				if (hr != S_FALSE)
					return hr;
			}
			return S_FALSE;
		}
	};
} // anonymous namespace

// キャンセル。
void __fastcall XgAlgo5Cancel(void) noexcept {
	s_cancel = true;
}

// アルゴリズム 5。
HRESULT __fastcall
XgAlgo5(xg_algo_quit_fn_t fnQuit, const MBoard& board, const MStringSetW& words, BOOL bUseAllWords,
        size_t max_solutions, size_t num_threads, size_t timeout_ms)
{
	assert(fnQuit);
	MStringW strError;
	// 例外は XgGuarded が HRESULT と strError に変換する。
	return XgGuarded(strError, [&]() -> HRESULT {
		s_cancel = false;
		if (max_solutions < 1)
			max_solutions = 1;

		if (words.empty())
			throw MInvalidArgument { L"word" };

		// 単語群を検査して、長い順(同じ長さなら辞書順)に並べる。先頭が探索の根になる。
		MStringListW list(words.begin(), words.end());
		for (auto& word : list) {
			if (word.size() < 2)
				throw MInvalidArgument { L"word" };
			for (auto ch : word) {
				if (!XgIsLetter(ch))
					throw MInvalidArgument { L"word" };
			}
		}
		std::sort(list.begin(), list.end(), [](const MStringW& a, const MStringW& b) {
			if (a.size() != b.size())
				return a.size() > b.size();
			return a < b;
		});

		const INT rules = board.Rules();
		MSearchShared shared(max_solutions);
		HRESULT hr = XgParallelSearch(shared, board, num_threads, s_cancel, timeout_ms,
			[&](size_t id) { return MSolver5(shared, id, list, rules, bUseAllWords); });

		if (hr == XG_E_TIMEOUT)
			strError = L"timeout";

		fnQuit(hr, shared.solutions, max_solutions, strError);
		return hr;
	});
}

// 単体テスト。
void __fastcall XgAlgo5UnitTest(void) {
	enum {
		test_rules = RULE_DONTDIVIDE
	};
	MBoard b = XgMakeBoard({
		L"　　　",
		L"　　　",
		L"　　　", }, test_rules);
	MStringSetW words = {
		L"ＴＨＩＳ",
		L"ＩＳ",
		L"ＸＷＯＲＤＧＩＶＥＲ",
		L"ＳＡＭＰＬＥ",
		L"ＡＮＤ",
		L"ＴＥＳＴ",
	};
	BOOL bUseAllWords = FALSE; // TRUE にすると全単語を使う。

	auto quit_fn = [&](HRESULT hr, std::vector<MBoard>& sols, size_t max_sols, MStringW& error) -> void {
		std::vector<MBoard> solutions = std::move(sols);
		MStringW strError = std::move(error);
		if (hr == E_ABORT) { // キャンセルされた。
			PrintfW(L"XgAlgo5 aborted (canceled)\n");
		} else if (hr == S_FALSE) { // 解がなかった。
			PrintfW(L"XgAlgo5 cannot solve\n");
		} else if (FAILED(hr)) { // タイムアウトなどの失敗。
			PrintfW(L"XgAlgo5 failed: %s\n", strError.c_str());
		} else { // 成功。
			PrintfW(L"XgAlgo5 OK (%d solutions)\n", (INT)solutions.size());
			for (auto& solution : solutions) {
				MStringW text;
				XgTextFromBoard(text, solution);
				PrintfW(L"%s\n", text.c_str());
			}
		}
	};

	PrintfW(L"XgAlgo5 start\n");
	DWORD tick0 = GetTickCount();
	XgAlgo5(quit_fn, b, words, bUseAllWords, 1);
	DWORD tick1 = GetTickCount();
	PrintfW(L"XgAlgo5 end: %u ms\n", (unsigned)(tick1 - tick0));
}
