// algo2.cpp --- アルゴリズム 2。
// Author: katahiromz
// License: MIT
//
// 方針:
//   ・辞書索引や単語集合は最初に1回だけ構築し、以降は読み取り専用で全スレッドが共有する(MData)。
//   ・各ワーカーは自前の乱数・ノード数カウンタ(MSolver)を持ち、独立に
//     「ランダム再起動 + Luby 列」の探索を行う(スレッドごとに別の探索順になる)。
//   ・スレッドの起動・停止・結果の集約や解の登録は parallel-search.h に集約している。

#include "algo2.h"
#include "dict.h"
#include "pat-match.h"
#include "parallel-search.h"
#include <atomic>
#include <thread>
#include <bitset>

namespace {

// 外部からのキャンセル要求。
std::atomic<bool> s_cancel(false);

// スロット(連続する非黒マスの並び)。
struct MSlot {
	INT x, y;     // 開始位置。
	BOOL bDown;   // 縦向きか？
	INT len;      // 長さ。
};

// 盤面から長さ2以上のスロットをすべて列挙する。
void XgEnumSlots(const MBoard& board, std::vector<MSlot>& slots) {
	const INT nColumns = board.Columns(), nRows = board.Rows();
	for (INT dir = 0; dir < 2; ++dir) {
		const BOOL bDown = (dir == 1);
		const INT nLines = bDown ? nColumns : nRows; // 線の本数。
		const INT nCells = bDown ? nRows : nColumns; // 線の長さ。
		for (INT line = 0; line < nLines; ++line) {
			INT pos = 0;
			while (pos < nCells) {
				INT x = bDown ? line : pos;
				INT y = bDown ? pos : line;
				if (board.GetCell(x, y, ZEN_BLACK) == ZEN_BLACK) {
					++pos; // 黒マスは飛ばす。
					continue;
				}
				INT end = pos;
				while (end < nCells) {
					INT x2 = bDown ? line : end;
					INT y2 = bDown ? end : line;
					if (board.GetCell(x2, y2, ZEN_BLACK) == ZEN_BLACK)
						break;
					++end;
				}
				if (end - pos >= 2) {
					MSlot slot = { x, y, bDown, end - pos };
					slots.push_back(slot);
				}
				pos = end;
			}
		}
	}
}

// 単語長ごとの索引(ビット集合による候補検索用)。
struct MWordIndex {
	std::vector<MStringW> words;                 // 単語(シャッフル順)。
	size_t nblocks = 0;                          // 64ビットブロック数。
	size_t nPriority = 0;                        // 先頭にある優先単語の個数。
	// pos[i][ch] = 「i文字目が ch である単語」の集合(ビット集合)。
	std::vector<std::unordered_map<WCHAR, std::vector<uint64_t>>> pos;
	std::unordered_map<MStringW, size_t> word2idx; // 単語 → 添字。
};

} // anonymous namespace

// 辞書の索引。構築後は読み取り専用なので、全スレッドで、また複数回の探索で共有できる。
// 外部には不透明な型として見せる(algorithm2.h)。algorithm3 は1回だけ作って使い回す。
struct MFillDict {
	std::unordered_map<size_t, MWordIndex> index;
	std::unordered_set<MStringW> word_set;   // 辞書の単語集合(完成した単語の検証用)。
};

namespace {

// 1回の探索で全スレッドが共有するデータ。構築後は読み取り専用。
struct MData {
	const MFillDict& db;
	MStringSetW initial_words;               // 初期盤面にすでにあった単語。
	std::vector<MSlot> slots;                // 盤面のスロット(黒マスは探索中に変化しない)。
	explicit MData(const MFillDict& db) : db(db) { }
};

// 索引を構築する。
void XgBuildIndex(MFillDict& data, const MStringListW& word_list_in,
                  const std::unordered_set<MStringW>& priority_words)
{
	data.index.clear();
	data.word_set.clear();

	// 優先単語を先頭に寄せる(シャッフル順は保つ)。
	// 各長さの単語列でも優先単語が先頭にまとまり、idx.nPriority が正しくなる。
	MStringListW word_list(word_list_in);
	if (!priority_words.empty()) {
		std::stable_partition(word_list.begin(), word_list.end(),
			[&](const MStringW& w) { return priority_words.count(w) > 0; });
	}

	for (auto& word : word_list) {
		if (word.size() < 2 || !data.word_set.insert(word).second)
			continue; // 短すぎる or 重複。
		auto& idx = data.index[word.size()];
		idx.word2idx[word] = idx.words.size();
		idx.words.push_back(word);
		// 上で優先単語を先頭に寄せたので、各長さの単語列でも優先単語は先頭にまとまっている。
		if (priority_words.count(word) > 0)
			++idx.nPriority;
	}
	for (auto& pair : data.index) {
		const size_t len = pair.first;
		auto& idx = pair.second;
		idx.nblocks = (idx.words.size() + 63) / 64;
		idx.pos.resize(len);
		for (size_t k = 0; k < idx.words.size(); ++k) {
			for (size_t p = 0; p < len; ++p) {
				auto& v = idx.pos[p][idx.words[k][p]];
				if (v.empty())
					v.assign(idx.nblocks, 0);
				v[k >> 6] |= (uint64_t)1 << (k & 63);
			}
		}
	}
}

// パターンに合う未使用の候補をビット集合で求め、個数を返す。
size_t XgCandBits(const MWordIndex& idx, const MStringW& pat,
                  const MStringSetW& used_words, std::vector<uint64_t>& bits)
{
	const size_t n = idx.words.size();
	bits.assign(idx.nblocks, ~(uint64_t)0);
	if (n & 63)
		bits.back() = ((uint64_t)1 << (n & 63)) - 1;
	for (size_t p = 0; p < pat.size(); ++p) {
		if (pat[p] == ZEN_SPACE)
			continue;
		auto it = idx.pos[p].find(pat[p]);
		if (it == idx.pos[p].end())
			return 0; // その文字を持つ単語がない。
		const auto& v = it->second;
		for (size_t b = 0; b < idx.nblocks; ++b)
			bits[b] &= v[b];
	}
	// 使用済みの単語を除く。
	for (auto& w : used_words) {
		if (w.size() != pat.size())
			continue;
		auto it = idx.word2idx.find(w);
		if (it != idx.word2idx.end())
			bits[it->second >> 6] &= ~((uint64_t)1 << (it->second & 63));
	}
	size_t count = 0;
	for (auto x : bits)
		count += std::bitset<64>(x).count();
	return count;
}

// 候補ビット集合のうち、優先単語(先頭 nPriority 個)の個数を数える。
size_t XgCountPriorityBits(const std::vector<uint64_t>& bits, size_t nPriority) {
	size_t count = 0;
	const size_t full = nPriority >> 6;
	for (size_t b = 0; b < full && b < bits.size(); ++b)
		count += std::bitset<64>(bits[b]).count();
	if ((nPriority & 63) && full < bits.size())
		count += std::bitset<64>(bits[full] & (((uint64_t)1 << (nPriority & 63)) - 1)).count();
	return count;
}

// 1ワーカー分の探索状態(再起動・乱数・停止判定は MSolverBase が持つ)。
class MSolver : public MSolverBase {
public:
	MSolver(MSearchShared& shared, const MData& data, size_t id)
		: MSolverBase(shared, id, 300), m_d(data) { }

	HRESULT Run(const MBoard& board) {
		return RunWithRestarts([&] { return Recurse(board); });
	}

private:
	const MData& m_d;

	// 再帰(MRV + ビット集合索引)。
	HRESULT Recurse(const MBoard& board) {
		// 停止要求なら E_ABORT、ノード数が上限を超えたら XG_E_RESTART。
		HRESULT hrTick = Tick();
		if (FAILED(hrTick))
			return hrTick;

		// 1パス目: 各スロットのパターンを取得し、完成単語を検証して使用済み集合を作る。
		std::vector<MStringW> pats(m_d.slots.size());
		MStringSetW used_words;
		for (size_t i = 0; i < m_d.slots.size(); ++i) {
			pats[i] = XgGetPatFromBoard(board, m_d.slots[i].x, m_d.slots[i].y, m_d.slots[i].bDown);
			if (XgPatHasSpace(pats[i]))
				continue;
			if (!used_words.insert(pats[i]).second)
				return S_FALSE; // 重複単語。
			if (m_d.db.word_set.count(pats[i]) == 0 && m_d.initial_words.count(pats[i]) == 0)
				return S_FALSE; // 辞書にない単語ができた。
		}

		if (board.IsFull()) { // 全部埋まった？
			if (m_shared.AddSolution(board))
				return S_OK; // 必要な個数に達した。
			return S_FALSE; // まだ足りないので探索を続ける。
		}

		// 2パス目: 候補数が最少のスロットを探す。
		size_t bestSlot = (size_t)-1, bestCount = (size_t)-1, bestPrio = 0, nTies = 0;
		std::vector<uint64_t> bits, bestBits;
		for (size_t i = 0; i < m_d.slots.size(); ++i) {
			if (!XgPatHasSpace(pats[i]))
				continue;
			auto it = m_d.db.index.find(pats[i].size());
			if (it == m_d.db.index.end())
				return S_FALSE; // この長さの単語がない。
			size_t count = XgCandBits(it->second, pats[i], used_words, bits);
			if (count == 0)
				return S_FALSE; // 候補 0 個。矛盾(早期枝刈り)。
			// 優先単語の候補数(同数のときの tie-break に使う)。
			const size_t prio = it->second.nPriority ? XgCountPriorityBits(bits, it->second.nPriority) : 0;
			if (count < bestCount || (count == bestCount && prio > bestPrio)) {
				bestCount = count;
				bestPrio = prio;
				bestSlot = i;
				nTies = 1;
				bestBits.swap(bits);
			} else if (count == bestCount && prio == bestPrio) {
				// 候補数も優先候補数も同じなら乱択(リザーバサンプリング)。
				++nTies;
				if (m_rng() % nTies == 0) {
					bestSlot = i;
					bestBits.swap(bits);
				}
			}
		}
		if (bestSlot == (size_t)-1)
			return S_FALSE; // 埋めるべきスロットがない。

		// 最良のスロットの各候補について再帰。
		const MSlot& slot = m_d.slots[bestSlot];
		const auto& idx = m_d.db.index.find(pats[bestSlot].size())->second;
		std::vector<size_t> order;
		order.reserve(bestCount);
		for (size_t k = 0; k < idx.words.size(); ++k) {
			if (bestBits[k >> 6] & ((uint64_t)1 << (k & 63)))
				order.push_back(k);
		}
		// 候補順をランダム化。ただし優先単語を先に試す(優先群・非優先群を別々にシャッフル)。
		auto mid = std::lower_bound(order.begin(), order.end(), idx.nPriority);
		std::shuffle(order.begin(), mid, m_rng);
		std::shuffle(mid, order.end(), m_rng);
		for (size_t k : order) {
			if (IsStop())
				return E_ABORT; // キャンセルされた。
			MBoard copy(board);
			XgApplyCandidate(copy, slot.x, slot.y, idx.words[k], slot.bDown);
			HRESULT hr = Recurse(copy);
			if (hr != S_FALSE)
				return hr; // 終了。
		}
		return S_FALSE; // すべての候補が失敗。
	}
};

// 探索の本体。索引 fd を使って、board の空きマスを埋める。
HRESULT XgFillImpl(const MFillDict& fd, const MBoard& board, const MStringSetW& initial_words,
                   const std::atomic<bool>& cancel,
                   std::vector<MBoard>& solutions, MStringW& strError,
                   size_t max_solutions, size_t num_threads, size_t timeout_ms)
{
	// スロットを1回だけ列挙する(以降は読み取り専用)。
	MData data(fd);
	data.initial_words = initial_words;
	XgEnumSlots(board, data.slots);

	MSearchShared shared(max_solutions);
	HRESULT hr = XgParallelSearch(shared, board, num_threads, cancel, timeout_ms,
		[&](size_t id) { return MSolver(shared, data, id); });

	solutions = std::move(shared.solutions); // コピーなし
	if (hr == E_OUTOFMEMORY)
		strError = L"memory";
	return hr;
}

} // anonymous namespace

// キャンセル。
void __fastcall XgAlgo2Cancel(void) noexcept {
	s_cancel = true;
}

// 辞書の索引を作る。
std::shared_ptr<const MFillDict> __fastcall XgAlgo2MakeFillDict(const MDict& dict) {
	auto fd = std::make_shared<MFillDict>();
	XgBuildIndex(*fd, dict.make_shuffled_list(), dict.m_priority_words);
	return fd;
}

// 索引を再利用して、盤面の空きマスを埋める(algorithm3 から繰り返し呼ぶ用)。
// 辞書は読み込まず、s_cancel も触らない。キャンセルは cancel で、制限時間は timeout_ms で指定する。
// 戻り値は XgAlgo2 と同じ。制限時間を超えると XG_E_TIMEOUT。
// 単語パターンの事前チェックは省くので、埋められないパターンは E_INVALIDARG ではなく S_FALSE になる。
HRESULT __fastcall XgAlgo2Fill(const MFillDict& fd, const MBoard& board,
                               const std::atomic<bool>& cancel,
                               std::vector<MBoard>& solutions, MStringW& strError,
                               size_t max_solutions, size_t num_threads, size_t timeout_ms)
{
	return XgGuarded(strError, [&]() -> HRESULT {
		solutions.clear();
		strError.clear();

		// 黒マスルールに適合するか？
		if (!board.AreBlacksOK())
			throw MInvalidArgument { L"rules" };

		// 重複単語がないか？
		MStringListW pats;
		MStringSetW used_words;
		if (!XgScanBoardPats(board, &pats, &used_words))
			throw MInvalidArgument { L"duplicate" };

		return XgFillImpl(fd, board, used_words, cancel, solutions, strError,
			(max_solutions < 1) ? 1 : max_solutions, num_threads, timeout_ms);
	});
}

// アルゴリズム 2。
// 戻り値:
//   S_OK:          解が見つかった。
//   S_FALSE:       全探索が終わったが解は見つからなかった。
//   E_ABORT:       キャンセルされた。
//   E_INVALIDARG:  黒マスルールに適合していないか、重複単語があるか、文字が適合しない単語パターンがある。
//   E_OUTOFMEMORY: メモリ不足。
//   その他:        その他のエラーが発生した。
HRESULT __fastcall XgAlgo2(const MDict& dict, const MBoard& board,
                           std::vector<MBoard>& solutions, MStringW& strError,
                           size_t max_solutions, size_t num_threads)
{
	// 例外は XgGuarded が HRESULT と strError に変換する。
	return XgGuarded(strError, [&]() -> HRESULT {
		solutions.clear();
		strError.clear();
		s_cancel = false;

		// 黒マスルールに適合するか？
		if (!board.AreBlacksOK())
			throw MInvalidArgument { L"rules" };

		// 重複単語がないか？
		MStringListW pats;
		MStringSetW used_words;
		if (!XgScanBoardPats(board, &pats, &used_words))
			throw MInvalidArgument { L"duplicate" };

		// 適合しない単語パターンがあるか？
		auto word_list = dict.make_shuffled_list();
		for (auto& pat : pats) {
			if (!XgPatHasSpace(pat))
				continue; // すでに完成した単語だった
			BOOL found = FALSE;
			for (auto& word : word_list) {
				if (XgPatMatch(pat, word) && used_words.count(word) == 0) {
					found = TRUE;
					break;
				}
			}
			if (!found)
				throw MInvalidArgument { L"pattern" };
		}

		// 優先単語(テーマ)。
		std::unordered_set<MStringW> priority_words = dict.m_priority_words;

		MFillDict fd;
		XgBuildIndex(fd, word_list, priority_words);
		return XgFillImpl(fd, board, used_words, s_cancel, solutions, strError,
			(max_solutions < 1) ? 1 : max_solutions, num_threads, 0);
	});
}

// アルゴリズム 2 の単体テスト。
void __fastcall XgAlgo2UnitTest(void) {
	enum {
		test_rules = RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | \
		             RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE |
		             RULE_DONTTHREEDIAGONALS
	};
#if 1
	MBoard b = XgMakeBoard({
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
#elif 1
	MBoard b = XgMakeBoard({
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
#elif 1
	MBoard b = XgMakeBoard({
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
	MBoard b = XgMakeBoard({
		L".#..#...",
		L"..#...#.",
		L"....#...",
		L".#.#..#." }, test_rules);
#endif

	PrintfW(L"XgAlgo2 start (threads: %u)\n", (unsigned)std::thread::hardware_concurrency());
	DWORD tick0 = GetTickCount();
	std::vector<MBoard> solutions;
	MStringW strError;
	MDict dict;
	HRESULT hr = XgLoadKanaDict(dict);
	if (FAILED(hr)) {
		PrintfW(L"No kana dictionary\n");
	}
	hr = XgAlgo2(dict, b, solutions, strError, 1);
	DWORD tick1 = GetTickCount();
	PrintfW(L"XgAlgo2 end: %d\n", (tick1 - tick0) / 1000);
	if (hr == E_ABORT) { // キャンセルされた。
		PrintfW(L"XgAlgo2 aborted (canceled)\n");
	} else if (hr == S_FALSE) { // 全探索したが解が見つからなかった。
		PrintfW(L"XgAlgo2 cannot solve\n");
	} else if (FAILED(hr)) { // その他の失敗。
		PrintfW(L"XgAlgo2 failed: %s\n", strError.c_str());
	} else { // 成功。
		PrintfW(L"XgAlgo2 OK (%d solutions)\n", (INT)solutions.size());
		for (auto& solution : solutions) {
			MStringW text;
			XgTextFromBoard(text, solution);
			PrintfW(L"%s\n", text.c_str());
		}
	}
}
