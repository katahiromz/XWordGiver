// parallel-search.h --- 探索アルゴリズム共通の並列実行基盤。
// Author: katahiromz
// License: MIT
//
// algo1.cpp / algorithm2.cpp に重複していた次のものをここに集約する。
//   ・Luby 列、再起動(リスタート)ループ、ノード数カウント
//   ・解の登録(重複排除・上限判定)
//   ・スレッドの起動 / 最初の終了待ち / キャンセル / 停止 / 結果の集約
//   ・入口関数の例外 → HRESULT 変換
//
// 同期プリミティブは次の3つだけ:
//   shared.stop : 全ワーカーへの停止指示(std::atomic<bool>)
//   cancel      : 外部からのキャンセル(呼び出し側の std::atomic<bool>)
//   cv          : 「誰かが終わった」通知(std::condition_variable)
#pragma once

#include "xwg.h"
#include "board-text-io.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <set>
#include <system_error>
#include <thread>
#include <vector>

struct MInvalidArgument { MStringW m_error; };

// 再起動(リスタート)制御。内部用。
inline const HRESULT XG_E_RESTART = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1000);
// 制限時間を超えた。XgParallelSearch の戻り値(解が見つからなかった場合)。
inline const HRESULT XG_E_TIMEOUT = MAKE_HRESULT(SEVERITY_ERROR, FACILITY_ITF, 0x1001);

// Luby 列(1,1,2,1,1,2,4,1,1,2,...)。i >= 1。
inline size_t XgLuby(size_t i) {
	for (size_t k = 1;; ++k) {
		const size_t full = ((size_t)1 << k) - 1;
		if (i == full)
			return (size_t)1 << (k - 1);
		if (i < full)
			return XgLuby(i - ((size_t)1 << (k - 1)) + 1);
	}
}

// 全ワーカーで共有する状態(アルゴリズム固有の索引などは含めない)。
struct MSearchShared {
	explicit MSearchShared(size_t max_solutions) : max_solutions(max_solutions) { }

	const size_t max_solutions;
	std::atomic<bool> stop{false};
	std::mutex mtx;
	std::vector<MBoard> solutions;
	std::set<MStringW> texts; // 重複排除用。

	// 解を登録する。必要な個数に達したら TRUE。
	BOOL AddSolution(const MBoard& board) {
		MStringW text;
		XgTextFromBoard(text, board);
		std::lock_guard<std::mutex> lock(mtx);
		if (solutions.size() >= max_solutions)
			return TRUE; // 他スレッドがすでに達成した。
		if (!texts.insert(text).second)
			return FALSE; // 再起動や他スレッドで同じ解を再発見した。
		solutions.push_back(board);
		if (solutions.size() >= max_solutions) {
			stop = true;
			return TRUE;
		}
		return FALSE;
	}
};

// 1ワーカー分の探索の共通部分。各アルゴリズムはこれを継承する。
//   class MSolver : public MSolverBase {
//       HRESULT Run(const MBoard& b) { return RunWithRestarts([&] { return Recurse(b); }); }
//       HRESULT Recurse(...) { HRESULT hr; if (FAILED(hr = Tick())) return hr; ... }
//   };
// Recurse の戻り値: S_OK=完了 / S_FALSE=この枝は失敗 / E_ABORT=停止 / XG_E_RESTART=再起動。
class MSolverBase {
public:
	MSolverBase(MSearchShared& shared, size_t id, size_t base_limit)
		: m_shared(shared), m_base_limit(base_limit)
	{
		// スレッドごとに異なるシードにする。
		m_rng.seed(std::random_device()() + (unsigned)id * 0x9E3779B9u);
	}

protected:
	MSearchShared& m_shared;
	std::mt19937 m_rng;

	BOOL IsStop() const { return m_shared.stop.load(std::memory_order_relaxed); }

	// Recurse の先頭で呼ぶ。停止なら E_ABORT、ノード数超過なら XG_E_RESTART。
	HRESULT Tick() {
		if (IsStop())
			return E_ABORT;
		if (++m_node_count > m_node_limit)
			return XG_E_RESTART;
		return S_OK;
	}

	// ランダム再起動 + Luby 列でノード数上限を決めながら探索する。
	template <class F>
	HRESULT RunWithRestarts(F&& recurse) {
		for (size_t attempt = 1; ; ++attempt) {
			m_node_count = 0;
			m_node_limit = m_base_limit * XgLuby(attempt);
			HRESULT hr = recurse();
			if (hr != XG_E_RESTART)
				return hr;
		}
	}

private:
	size_t m_base_limit;
	size_t m_node_count = 0;
	size_t m_node_limit = 0;
};

// num_threads 本のワーカーを走らせ、最初の1本が終わるか cancel が立つまで待ち、
// 全員を止めて結果をまとめる。解は shared.solutions に入っている。
//   make(id) : Run(const MBoard&) -> HRESULT を持つ solver を返す(ワーカースレッド内で呼ばれる)。
// 戻り値:
//   S_OK     : 解が1個以上ある
//   E_ABORT  : キャンセルされた
//   S_FALSE  : 全探索が終わったが解はなかった
//   XG_E_TIMEOUT : timeout_ms を超えた(解はなかった)
//   その他   : ワーカーのエラー(E_OUTOFMEMORY など)
// timeout_ms が 0 なら制限時間なし。
// スレッドを作れなければ std::system_error を投げる。
template <class MakeSolver>
HRESULT XgParallelSearch(MSearchShared& shared, const MBoard& board, size_t num_threads,
                         const std::atomic<bool>& cancel, size_t timeout_ms, MakeSolver make)
{
	if (num_threads == 0) {
		const unsigned hc = std::thread::hardware_concurrency();
		num_threads = (hc > 1) ? hc - 1 : 1;
	}

	// ↓ ワーカーより長生きさせる(破棄は宣言の逆順)。
	std::mutex m;
	std::condition_variable cv;
	bool anyDone = false;
	std::vector<HRESULT> results(num_threads, E_ABORT); // 各ワーカーは自分の要素だけ書く。

	// 例外・早期 return でも必ず止めて join する。
	struct Joiner {
		MSearchShared& shared;
		std::vector<std::thread> threads;
		void JoinAll() {
			shared.stop = true;
			for (auto& t : threads)
				if (t.joinable())
					t.join();
		}
		~Joiner() { JoinAll(); }
	} joiner{ shared, {} };

	for (size_t i = 0; i < num_threads; ++i) {
		joiner.threads.emplace_back([&, i] {
			HRESULT hr;
			try {
				hr = make(i).Run(board);
			} catch (const std::bad_alloc&) {
				hr = E_OUTOFMEMORY;
			} catch (...) {
				hr = E_FAIL;
			}
			results[i] = hr;
			if (hr != E_ABORT)
				shared.stop = true; // これ以上探しても無駄なので全員止める。
			{
				std::lock_guard<std::mutex> lock(m);
				anyDone = true;
			}
			cv.notify_all();
		});
	}

	// いずれかのワーカーが終了するか、キャンセル・タイムアウトするまで待つ。
	BOOL bCanceled = FALSE, bTimedOut = FALSE;
	const auto t0 = std::chrono::steady_clock::now();
	{
		std::unique_lock<std::mutex> lock(m);
		while (!cv.wait_for(lock, std::chrono::milliseconds(50), [&] { return anyDone; })) {
			if (cancel) {
				bCanceled = TRUE;
				break;
			}
			if (timeout_ms && std::chrono::steady_clock::now() - t0 >= std::chrono::milliseconds(timeout_ms)) {
				bTimedOut = TRUE;
				break;
			}
		}
	}
	joiner.JoinAll();

	// 結果をまとめる。
	BOOL bFalse = FALSE;
	HRESULT hrError = S_OK;
	for (HRESULT hr : results) {
		if (hr == S_FALSE)
			bFalse = TRUE;
		else if (FAILED(hr) && hr != E_ABORT && SUCCEEDED(hrError))
			hrError = hr;
	}

	if (!shared.solutions.empty())
		return S_OK; // 1個でも解があれば成功(上限切れ・探索完了でも)。
	if (bCanceled)
		return E_ABORT;
	if (bFalse)
		return S_FALSE;
	if (FAILED(hrError))
		return hrError;
	if (bTimedOut)
		return XG_E_TIMEOUT;
	return E_ABORT;
}

// 入口関数の共通 try/catch。body は HRESULT() を返す関数オブジェクト。
template <class F>
HRESULT XgGuarded(MStringW& strError, F&& body) {
	try {
		strError.clear();
		return body();
	} catch (const std::bad_alloc&) {
		strError = L"memory";
		return E_OUTOFMEMORY;
	} catch (const MInvalidArgument& error) {
		strError = error.m_error;
		return E_INVALIDARG;
	} catch (const std::system_error&) { // スレッド生成失敗。
		strError = L"thread";
		return E_FAIL;
	}
}
