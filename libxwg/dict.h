// dict.h --- Dictionary
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>

// std::random_shuffleの代わり。
#include <random>
#include <algorithm>
#include <chrono>
template <typename T_IT>
inline void __fastcall XgRandomShuffle(const T_IT& begin, const T_IT& end) {
#ifndef NO_RANDOM
	std::seed_seq seq{
		(unsigned)std::random_device()(),
		(unsigned)(GetTickCount() + (GetCurrentThreadId() << 4)),
		(unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count()
	};
	std::mt19937 g(seq);
	std::shuffle(begin, end, g);
#endif
}

// 辞書。
struct MDict {
	MStringW m_filename; // ファイル名。
	MStringW m_pathname; // パスファイル名。
	MStringListW m_comments; // コメント行群。
	std::unordered_map<MStringW, MStringW> m_word2clue; // 単語からカギへ。
	std::unordered_map<MStringW, std::unordered_set<MStringW>> m_word2tags; // 単語からタグ情報へ。
	MStringListW m_word_list; // 単語リスト。
	MStringW m_theme_tags; // テーマ タグ。
	MStringW m_default_theme_tags; // デフォルトのテーマ タグ。
	MStringW m_english_name; // 英語名。
	MStringW m_japanese_name; // 日本語名。
	// 優先タグの付いたタグ集合。
	std::unordered_set<MStringW> m_priority_tags;
	// 除外タグの付いたタグ集合。
	std::unordered_set<MStringW> m_forbidden_tags;
	// 優先タグの付いた単語集合。
	std::unordered_set<MStringW> m_priority_words;
	// 除外タグの付いた単語集合。
	std::unordered_set<MStringW> m_forbidden_words;
	// m_word_listを長さで分類したもの。
	std::unordered_map<WORD, MStringListW> m_word_list_by_len;
	// タグのヒストグラム。
	std::unordered_map<MStringW, size_t> m_tag_histgram;

	// ファイルから読み込み。
	HRESULT __fastcall load_file(LPCWSTR pathname, BOOL bCommentOnly = FALSE);
	// ミニ辞書を作る。
	HRESULT __fastcall make_mini_dict(const MStringListW& word_list);
	// ミニ辞書を作る。
	HRESULT __fastcall make_mini_dict(const std::unordered_map<MStringW, MStringW>& mini_dict);

	// 再読み込み。
	HRESULT __fastcall refresh();

	// ファイルから読み込む。同じ辞書ファイルなら、テーマ タグ(文字列とタグ集合)を保つ。
	// load_file は既定のテーマ タグに戻してしまうので、ユーザーの選択を残したいときはこちらを使う。
	HRESULT __fastcall load_file_keep_theme(LPCWSTR pathname);

	// クリア。
	void __fastcall clear() noexcept;

	// テーマ タグをセット。
	void __fastcall set_theme_tags(const MStringW& str);

	// m_priority_tags / m_forbidden_tags から、優先単語集合と除外単語集合を再計算する。
	// タグ集合を直接書き換えたあとに呼ぶこと。
	void __fastcall update_theme_words();

	// 単語リストをソート。
	void __fastcall sort_word_list();

	// 除外単語を除き、優先単語を前にしてシャッフルした単語リストを作る。
	// メンバを変更しないので、複数スレッドから同時に呼べる。
	MStringListW __fastcall make_shuffled_list() const;

	// 単体テスト。
	static void __fastcall unittest();

protected:
	// コメント行を追加。
	void __fastcall add_comment(const char *str) {
		add_comment(WideFromUtf8(str));
	}
	void __fastcall add_comment(const MStringW& str) {
		m_comments.push_back(str);
	}

	void __fastcall parse_comments();

	static void __fastcall parse_theme(
		std::unordered_set<MStringW>& priority,
		std::unordered_set<MStringW>& forbidden,
		const MStringW& strTheme);
};
using MDictList = std::vector<MDict>;

BOOL __fastcall XgLoadDictList(MDictList& dict_list);
HRESULT __fastcall XgLoadKanaDict(MDict& dict);
HRESULT __fastcall XgLoadEnglishDict(MDict& dict);
INT __fastcall XgGetPreferredMaxLength(const MDict& dict) noexcept;
