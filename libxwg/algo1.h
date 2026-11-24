// algo1.h --- アルゴリズム 1。
// Author: katahiromz
// License: MIT

#pragma once

#include "dict.h"
#include <atomic>

struct MDict;

// 結果は solutions に、エラーメッセージは strError に入る
// (呼び出し前の内容は破棄される。エラーがなければ strError は空)。
//   ・bKeepBoard が FALSE なら、board の内容を捨てて空の盤面から探索する(従来の動作)。
//   ・bKeepBoard が TRUE なら、board にすでにある黒マスと文字マスを保ったまま探索する。
//     黒マスは対称位置にも反映される。文字マスは黒にならない。
//     前提と矛盾する(対称位置に文字マスがある等)場合は、解なしとして S_FALSE を返す。
HRESULT __fastcall XgAlgo1(const MDict& dict, const MBoard& board, INT nMaxWordLen,
                           std::vector<MBoard>& solutions, MStringW& strError,
                           BOOL bKeepBoard = FALSE,
                           size_t max_solutions = 1, size_t num_threads = 0);
void __fastcall XgAlgo1Cancel(void) noexcept;
void __fastcall XgAlgo1UnitTest(void);

// 辞書を読み込み済みの版(algorithm3 から繰り返し呼ぶ用)。結果は solutions に入る。
//   ・辞書は読み込まない。内部のキャンセルフラグも触らず、cancel を見る。
//   ・timeout_ms が 0 以外なら、その時間を超えたら諦めて XG_E_TIMEOUT(parallel-search.h)を返す。
//   ・bKeepBoard については XgAlgo1 を参照。
HRESULT __fastcall XgAlgo1Generate(const MDict& dict, const MBoard& board, INT nMaxWordLen,
                                   const std::atomic<bool>& cancel,
                                   std::vector<MBoard>& solutions, MStringW& strError,
                                   BOOL bKeepBoard = FALSE,
                                   size_t max_solutions = 1, size_t num_threads = 0,
                                   size_t timeout_ms = 0);
