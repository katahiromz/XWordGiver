// algo5.h --- アルゴリズム 5(単語群からクロスワードを生成する)。
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"

// アルゴリズム 5。
//   board        : 黒マスルール(board.Rules())だけを使う。盤面の大きさ・中身は使わない。
//   words        : 使う単語群。各単語は2文字以上。
//   bUseAllWords : TRUE なら全単語を1回ずつ使った盤面だけを解とする。
//                  FALSE なら使い切らなくてよく、使える単語をなるべく多く使った盤面を解とする。
//   max_solutions: 欲しい解の個数。
//   num_threads  : スレッド数(0なら自動)。
//   timeout_ms   : 制限時間(ミリ秒)。0なら制限なし。
// 戻り値:
//   S_OK / E_ABORT(キャンセル) / S_FALSE(解なし) / XG_E_TIMEOUT / E_INVALIDARG / E_OUTOFMEMORY など。
HRESULT __fastcall
XgAlgo5(xg_algo_quit_fn_t fnQuit, const MBoard& board, const MStringSetW& words,
        BOOL bUseAllWords = TRUE,
        size_t max_solutions = 1, size_t num_threads = 0,
        size_t timeout_ms = 10000);
void __fastcall XgAlgo5Cancel(void) noexcept;
void __fastcall XgAlgo5UnitTest(void);
