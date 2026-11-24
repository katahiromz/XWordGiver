// clues.cpp --- クロスワードのカギ情報。
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include "dict.h"

//////////////////////////////////////////////////////////////////////////////
// 格納情報。

// 単語の格納情報。
struct MPlaceInfo {
	INT m_jCol; // 列のインデックス。
	INT m_iRow; // 行のインデックス。
	MStringW m_word; // 単語。正規化されているものとする。
	INT m_number; // 番号。1ベース。

	// コンストラクタ。
	MPlaceInfo() noexcept { }
	MPlaceInfo(INT jCol_, INT iRow_, const MStringW& word_)
		: m_jCol(jCol_), m_iRow(iRow_), m_word(word_)
	{
	}
	MPlaceInfo(INT jCol_, INT iRow_, MStringW&& word_) noexcept
		: m_jCol(jCol_), m_iRow(iRow_), m_word(std::move(word_))
	{
	}
	MPlaceInfo(INT jCol_, INT iRow_, const MStringW& word_, INT number_)
		: m_jCol(jCol_), m_iRow(iRow_), m_word(word_), m_number(number_)
	{
	}
	MPlaceInfo(INT jCol_, INT iRow_, MStringW&& word_, INT number_) noexcept
		: m_jCol(jCol_), m_iRow(iRow_), m_word(std::move(word_)), m_number(number_)
	{
	}

	// コピーコンストラクタ。
	MPlaceInfo(const MPlaceInfo& pi) {
		m_jCol = pi.m_jCol;
		m_iRow = pi.m_iRow;
		m_word = pi.m_word;
		m_number = pi.m_number;
	}
	MPlaceInfo(MPlaceInfo&& pi) noexcept {
		m_jCol = pi.m_jCol;
		m_iRow = pi.m_iRow;
		m_word = std::move(pi.m_word);
		m_number = pi.m_number;
	}

	// 代入。
	void __fastcall operator=(const MPlaceInfo& pi) {
		m_jCol = pi.m_jCol;
		m_iRow = pi.m_iRow;
		m_word = pi.m_word;
		m_number = pi.m_number;
	}

	// 代入。
	void __fastcall operator=(MPlaceInfo&& pi) noexcept {
		m_jCol = pi.m_jCol;
		m_iRow = pi.m_iRow;
		m_word = std::move(pi.m_word);
		m_number = pi.m_number;
	}
};

namespace std {
	inline void swap(MPlaceInfo& info1, MPlaceInfo& info2) noexcept {
		std::swap(info1.m_jCol, info2.m_jCol);
		std::swap(info1.m_iRow, info2.m_iRow);
		std::swap(info1.m_word, info2.m_word);
		std::swap(info1.m_number, info2.m_number);
	}
}

//////////////////////////////////////////////////////////////////////////////

// 番号をつける。
BOOL __fastcall XgDoNumbering(
	const MBoard& board,
	std::vector<MPlaceInfo>& vHorzInfo,
	std::vector<MPlaceInfo>& vVertInfo);

MStringW XgTextFromDownCluesStyle0(const std::vector<MPlaceInfo>& vVertInfo, const MStringMap& word2clue = {}, BOOL bShowAnswer = TRUE);
MStringW XgTextFromAcrossCluesStyle0(const std::vector<MPlaceInfo>& vHorzInfo, const MStringMap& word2clue = {}, BOOL bShowAnswer = TRUE);
MStringW XgTextFromDownCluesStyle1(const std::vector<MPlaceInfo>& vVertInfo, const MStringMap& word2clue = {}, BOOL bShowAnswer = TRUE);
MStringW XgTextFromAcrossCluesStyle1(const std::vector<MPlaceInfo>& vHorzInfo, const MStringMap& word2clue = {}, BOOL bShowAnswer = TRUE);
MStringW XgTextFromDownCluesHTML(const std::vector<MPlaceInfo>& vVertInfo, const MStringMap& word2clue = {}, BOOL bShowAnswer = TRUE);
MStringW XgTextFromAcrossCluesHTML(const std::vector<MPlaceInfo>& vHorzInfo, const MStringMap& word2clue = {}, BOOL bShowAnswer = TRUE);
void Clues_UnitTest(void);
