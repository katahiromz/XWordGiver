// algo3.h --- アルゴリズム 3(アルゴリズム 2 + アルゴリズム 1)。
// Author: katahiromz
// License: MIT

#pragma once

#include "dict.h"

extern size_t xg_nPatternsTried3;           // 直近の実行で、埋めてみた黒マスパターンの個数(診断用)。

// アルゴリズム 3。
//   解(文字が入った盤面)は solutions に、エラーメッセージは strError に入る
//   (呼び出し前の内容は破棄される。エラーがなければ strError は空)。
//   1. アルゴリズム 2 で、辞書の単語の長さに合った黒マスパターンを作る。
//   2. そのパターンを、アルゴリズム 1 で文字(単語)で埋める。
//   3. 埋められなければ(行き止まり・制限時間切れ)、別のパターンを作ってやり直す。
//
//   board            : 盤面の大きさとルールを与える(中身は無視され、白紙から作る)。
//   nMaxWordLen      : 単語の最大長。
//   max_solutions    : 欲しい解の個数(解ごとに別のパターンを使う)。
//   num_threads      : 各フェーズで使うスレッド数(0 なら自動)。
//   fill_timeout_ms  : 1つのパターンを埋めるのに使う制限時間(ミリ秒)。0 なら無制限。
//                      超えたらそのパターンを諦めて次へ行く。
//
// 戻り値:
//   S_OK:          1個以上の解が見つかった(キャンセルされても、1個でもあれば S_OK)。
//   S_FALSE:       新しいパターンがもう作れなかった(または存在しない)。解はない。
//   E_ABORT:       キャンセルされた。解はない。
//   E_INVALIDARG:  辞書がない、またはパターンが繰り返し規則違反と判定された(strError 参照)。
//   E_OUTOFMEMORY: メモリ不足。
//   その他:        その他のエラー。
HRESULT __fastcall XgAlgo3(const MDict& dict, const MBoard& board, INT nMaxWordLen,
                           std::vector<MBoard>& solutions, MStringW& strError,
                           BOOL bKeepBoard = FALSE, size_t max_solutions = 1, size_t num_threads = 0,
                           size_t fill_timeout_ms = 10000);
void __fastcall XgAlgo3Cancel(void) noexcept;
void __fastcall XgAlgo3UnitTest(void);
