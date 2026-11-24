// algo2.h --- アルゴリズム 2。
// Author: katahiromz
// License: MIT

#pragma once

#include "dict.h"
#include <atomic>
#include <memory>

struct MFillDict; // 辞書の索引(不透明)。構築後は読み取り専用。

// 結果は solutions に、エラーメッセージは strError に入る
// (呼び出し前の内容は破棄される。エラーがなければ strError は空)。
HRESULT __fastcall XgAlgo2(const MDict& dict, const MBoard& board,
                           std::vector<MBoard>& solutions, MStringW& strError,
                           size_t max_solutions = 1, size_t num_threads = 0);
void __fastcall XgAlgo2Cancel(void) noexcept;
void __fastcall XgAlgo2UnitTest(void);

// ---- algorithm3 など、繰り返し呼ぶ側のための入口 ----

// 辞書の索引を作る(重いので、1回作って使い回す)。
std::shared_ptr<const MFillDict> __fastcall XgAlgo2MakeFillDict(const MDict& dict);

// 索引を再利用して、board の空きマスを埋める。結果は solutions に、エラーメッセージは strError に入る。
//   ・辞書は読み込まない。内部のキャンセルフラグも触らず、cancel を見る。
//   ・timeout_ms が 0 以外なら、その時間を超えたら諦めて XG_E_TIMEOUT(parallel-search.h)を返す。
//   ・戻り値は XgAlgo2 と同じ(S_OK / S_FALSE / E_ABORT / E_INVALIDARG / E_OUTOFMEMORY)。
HRESULT __fastcall XgAlgo2Fill(const MFillDict& fd, const MBoard& board,
                               const std::atomic<bool>& cancel,
                               std::vector<MBoard>& solutions, MStringW& strError,
                               size_t max_solutions = 1, size_t num_threads = 0,
                               size_t timeout_ms = 0);
