//////////////////////////////////////////////////////////////////////////////
// Marks.hpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#ifndef __XWORDGIVER_MARKS_HPP__
#define __XWORDGIVER_MARKS_HPP__

extern std::vector<XG_Pos> xg_vMarks;

//////////////////////////////////////////////////////////////////////////////

// 二重マス単語候補。
extern std::vector<std::wstring>  xg_vMarkedCands;

//////////////////////////////////////////////////////////////////////////////

// マーク文字列を取得する。
void __fastcall XgGetStringOfMarks(std::wstring& str);
// マークされているか（二重マス）？
int __fastcall XgGetMarked(const std::vector<XG_Pos>& vMarks, const XG_Pos& pos);
// マークされているか（二重マス）？
int __fastcall XgGetMarked(const std::vector<XG_Pos>& vMarks, int i, int j);
// マークされているか（二重マス）？
int __fastcall XgGetMarked(int i, int j);
// 二重マスが更新された。
void __fastcall XgMarkUpdate(void);
// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(const XG_Pos& pos);
// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(int i, int j);
// 指定のマスのマーク（二重マス）を解除する。
void __fastcall XgDeleteMark(int i, int j);
// マーク文字列を設定する。
void __fastcall XgSetStringOfMarks(LPCWSTR psz);
// 二重マス単語を取得する。
bool __fastcall XgGetMarkWord(const XG_Board *xw, std::wstring& str);
// 二重マス単語を設定する。
void __fastcall XgSetMarkedWord(const std::wstring& str);
// 二重マス単語を空にする。
void __fastcall XgSetMarkedWord(void);
// 次の二重マス単語を取得する。
void __fastcall XgGetNextMarkedWord(void);
// 前の二重マス単語を取得する。
void __fastcall XgGetPrevMarkedWord(void);

//////////////////////////////////////////////////////////////////////////////

#endif  // ndef __XWORDGIVER_MARKS_HPP__
