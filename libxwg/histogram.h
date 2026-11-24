// histogram.h --- Word/Pattern length histogram
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include <unordered_map>
#include <vector>

// 単語長またはパターン長のヒストグラム。
using MHistogram = std::unordered_map<INT, INT>;

BOOL __fastcall XgHistogramFitsIn(const MHistogram& x, const MHistogram& y) noexcept;
MHistogram __fastcall XgHistogramFromList(const MStringListW& list);
MHistogram __fastcall XgHistogramFromBoard(const MBoard& board);
void __fastcall XgHistogramUnitTest();
