// algo4.h --- アルゴリズム 4。
// Author: katahiromz
// License: MIT

#pragma once

#include "dict.h"

// アルゴリズム 4。
HRESULT __fastcall
XgAlgo4(xg_algo_quit_fn_t fnQuit, const MDict& dict, const MBoard& board, INT nMaxWordLen,
        size_t max_solutions = 1, BOOL bUsePatData = TRUE, size_t num_threads = 0,
        size_t fill_timeout_ms = 10000);
void __fastcall XgAlgo4Cancel(void) noexcept;
void __fastcall XgAlgo4UnitTest(void);
