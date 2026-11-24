// marks.h --- 二重マス。
// Author: katahiromz
// License: MIT

#pragma once

#include "board-image.h"
#include "dict.h"

typedef std::vector<MBoardPos> MMarkLayout;

// 二重マスの配置候補を、分散の良い順に生成する。
// word の各文字が置けるマスから、重複しない配置を探す。
bool __fastcall XgGenerateMarkLayouts(const MBoardDisplay& display, const MStringW& word,
                                      std::vector<MMarkLayout>& layouts,
                                      WCHAR *pchNotFound = nullptr, size_t nMax = 64);

void __fastcall XgGetStringOfMarks(const MBoardDisplay& display, MStringW& str);
void __fastcall XgGetStringOfMarks2(const MBoardDisplay& display, MStringW& str);
INT __fastcall XgGetMarked(std::vector<MBoardPos>& vMarks, const MBoardPos& pos) noexcept;
INT __fastcall XgGetMarked(std::vector<MBoardPos>& vMarks, INT j, INT i) noexcept;
INT __fastcall XgGetMarked(MBoardDisplay& display, const MBoardPos& pos) noexcept;
INT __fastcall XgGetMarked(MBoardDisplay& display, INT j, INT i) noexcept;
void __fastcall XgSetMark(std::vector<MBoardPos>& vMarks, const MBoardPos& pos);
void __fastcall XgSetMark(MBoardDisplay& display, const MBoardPos& pos);
void __fastcall XgSetMark(MBoardDisplay& display, INT j, INT i);
void __fastcall XgDeleteMark(MBoardDisplay& display, INT j, INT i);
void __fastcall XgSetStringOfMarks(MBoardDisplay& display, LPCWSTR psz);
bool __fastcall XgGetMarkWord(MBoardDisplay& display, MStringW& str);
bool __fastcall XgGetMarkedCandidates(MBoardDisplay& display, MStringListW& cands, const MDict& dict);
BOOL __fastcall XgSetMarkedWord(MBoardDisplay& display, const MStringW& str, WCHAR *pchNotFound = nullptr);
