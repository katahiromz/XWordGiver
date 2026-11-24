// dict.cpp --- Dictionary
// Author: katahiromz
// License: MIT

#include "dict.h"
#include "utils.h"

// テーマ文字列をパースする。
void __fastcall
MDict::parse_theme(std::unordered_set<MStringW>& priority,
				  std::unordered_set<MStringW>& forbidden,
				  const MStringW& strTheme)
{
	priority.clear();
	forbidden.clear();

	MStringListW strs;
	XgStrSplit(strs, strTheme, L",");
	for (auto& str : strs) {
		XgStrTrim(str);
		auto item = str;
		if (item.empty())
			continue;
		bool minus = false;
		if (item[0] == L'-') {
			minus = true;
			item = item.substr(1);
		} else if (item[0] == L'+') {
			item = item.substr(1);
		}
		if (item.empty())
			continue;
		if (minus) {
			forbidden.emplace(item);
		} else {
			priority.emplace(item);
		}
	}
}

// クリア。
void __fastcall MDict::clear() noexcept {
	m_filename.clear();
	m_pathname.clear();
	m_comments.clear();
	m_word2clue.clear();
	m_word_list.clear();
	m_word_list_by_len.clear();
	m_theme_tags.clear();
	m_default_theme_tags.clear();
	m_english_name.clear();
	m_japanese_name.clear();
	m_word2tags.clear();
	m_priority_tags.clear();
	m_forbidden_tags.clear();
	m_priority_words.clear();
	m_forbidden_words.clear();
	m_tag_histgram.clear();
}

// ミニ辞書を作る。
HRESULT __fastcall MDict::make_mini_dict(const MStringListW& word_list) {
	clear();
	m_word_list = word_list;
	for (auto& word : word_list) {
		m_word2clue[word] = L"";
	}
	return S_OK;
}

// ミニ辞書を作る。
HRESULT __fastcall MDict::make_mini_dict(const std::unordered_map<MStringW, MStringW>& mini_dict) {
	clear();
	for (auto& pair : mini_dict) {
		if (pair.first.size() <= 1)
			continue;
		m_word_list.push_back(pair.first);
		m_word_list_by_len[(WORD)pair.first.size()].push_back(pair.first);
		m_word2clue[pair.first] = pair.second;
	}
	return S_OK;
}

// 辞書ファイルを読み込む。
HRESULT __fastcall MDict::load_file(LPCWSTR pathname, BOOL bCommentOnly) {
	clear();

	// ファイル名。
	m_filename = PathFindFileNameW(pathname);

	// フルパス名。
	WCHAR full[MAX_PATH];
	GetFullPathNameW(pathname, _countof(full), full, NULL);
	m_pathname = full;

	// ファイルを開く。
	FILE *fin = _wfopen(pathname, L"rb");
	if (!fin) {
		return E_FAIL;
	}
	AutoFClose auto_close(fin);

	// 一行ずつ読み込む。
	char buf[1024];
	bool first = true;
	INT iItem = 0;
	while (fgets(buf, sizeof(buf), fin)) {
		if (first) {
			first = false;
			if (memcmp(buf, "\xEF\xBB\xBF", 3) == 0) { // UTF-8 BOM
				buf[0] = buf[1] = buf[2] = ' ';
			}
		}
		if (!buf[0])
			continue;
		if (buf[0] == '#') { // コメント
			add_comment(buf);
			continue;
		}

		StrTrimA(buf, " \t\r\n");
		if (!buf[0])
			continue;
		if (buf[0] == '#') { // コメント
			add_comment(buf);
			continue;
		}

		MStringW line = WideFromUtf8(buf); // ワイド文字列化
		XgStrTrim(line);
		if (line.empty())
			continue;
		if (line[0] == L'#') { // コメント
			add_comment(line);
			continue;
		}

		if (bCommentOnly) { // コメントのみ？
			++iItem;
			if (iItem > 80)
				break; // 80行を超えたら、読み捨てる。
			continue;
		}

		MStringListW fields;
		XgStrSplit(fields, line, L"\t"); // タブ文字で分割する。
		if (fields.empty())
			continue;

		for (auto& field : fields) {
			XgStrTrim(field); // 各フィールドの前後の空白を取り除く。
		}

		auto& word = fields[0];
		if (word.size() <= 1)
			continue; // 単語の長さが1文字以下なら無視。

		word = XgNormalizeString(word); // 文字列の正規化。

		// 追加。
		m_word_list.push_back(word);
		m_word_list_by_len[(WORD)word.size()].push_back(word);

		// カギ文章。
		if (fields.size() >= 2)
			m_word2clue[word] = fields[1];
		else
			m_word2clue[word] = L"";

		// タグ情報。
		std::unordered_set<MStringW> tags;
		for (size_t iField = 2; iField < fields.size(); ++iField) {
			tags.emplace(fields[iField]);
		}
		m_word2tags.emplace(word, std::move(tags));
	}

	// コメントを解析する。
	parse_comments();

	if (bCommentOnly)
		return S_OK;

	// タグのヒストグラムを計算する。
	for (auto& pair : m_word2tags) {
		for (auto& tag : pair.second) {
			m_tag_histgram[tag] += 1;
		}
	}

	// テーマ タグをセット。
	set_theme_tags(m_default_theme_tags);

	return m_word_list.size() ? S_OK : E_FAIL;
}

// テーマ タグをセット。
void __fastcall MDict::set_theme_tags(const MStringW& str) {
	m_theme_tags = str;
	parse_theme(m_priority_tags, m_forbidden_tags, m_theme_tags);
	update_theme_words();
}

// タグ集合から、優先単語と除外単語を再計算。
void __fastcall MDict::update_theme_words() {
	m_priority_words.clear();
	m_forbidden_words.clear();
	for (const auto& word : m_word_list) {
		auto it = m_word2tags.find(word);
		if (it == m_word2tags.end())
			continue;
		bool is_priority = false, is_forbidden = false;
		for (const auto& tag : it->second) {
			if (m_priority_tags.count(tag) > 0)
				is_priority = true;
			if (m_forbidden_tags.count(tag) > 0)
				is_forbidden = true;
		}
		if (is_forbidden)
			m_forbidden_words.emplace(word); // 除外が優先
		else if (is_priority)
			m_priority_words.emplace(word);
	}
}

// コメントを解析する。
void __fastcall MDict::parse_comments() {
	for (auto& line : m_comments) {
		if (m_english_name.size() && m_japanese_name.size() && m_default_theme_tags.size())
			break;
		if (m_english_name.empty()) {
			auto ich0 = line.find(L"NAME-ENG:");
			if (ich0 != line.npos) {
				m_english_name = line.substr(ich0 + wcslen(L"NAME-ENG:"));
				XgStrTrim(m_english_name);
				continue;
			}
		}
		if (m_japanese_name.empty()) {
			auto ich1 = line.find(L"NAME-JPN:");
			if (ich1 != line.npos) {
				m_japanese_name = line.substr(ich1 + wcslen(L"NAME-JPN:"));
				XgStrTrim(m_japanese_name);
				continue;
			}
		}
		if (m_default_theme_tags.empty()) {
			auto ich2 = line.find(L"DEFAULT:");
			if (ich2 != line.npos) {
				m_default_theme_tags = line.substr(ich2 + wcslen(L"DEFAULT:"));
				XgStrTrim(m_default_theme_tags);
				continue;
			}
		}
	}
}

// 再読み込み。
HRESULT __fastcall MDict::refresh() {
	auto pathname = m_pathname;
	return load_file_keep_theme(pathname.c_str());
}

// 同じ辞書ファイルなら、テーマ タグを保ったまま読み込む。
HRESULT __fastcall MDict::load_file_keep_theme(LPCWSTR pathname) {
	// OnThemeTags はタグ集合を直接設定するので、文字列だけでなく集合も保存する。
	const auto old_path = m_pathname;
	const auto old_theme = m_theme_tags;
	const auto old_priority = m_priority_tags;
	const auto old_forbidden = m_forbidden_tags;

	HRESULT hr = load_file(pathname);
	if (FAILED(hr))
		return hr;

	const bool same = (_wcsicmp(old_path.c_str(), m_pathname.c_str()) == 0);
	if (same && (!old_theme.empty() || !old_priority.empty() || !old_forbidden.empty())) {
		m_theme_tags = old_theme;
		m_priority_tags = old_priority;
		m_forbidden_tags = old_forbidden;
		update_theme_words();
	}
	return hr;
}

// シャッフルした単語リストを作る(const。スレッドセーフ)。
MStringListW __fastcall MDict::make_shuffled_list() const {
	// 除外タグの付いた単語を除いてコピーする。
	MStringListW list;
	list.reserve(m_word_list.size());
	for (const auto& word : m_word_list) {
		if (m_forbidden_words.count(word) == 0)
			list.push_back(word);
	}

	// 優先タグの付いた単語を前に寄せる(後でシャッフルするので順序は不要)。
	size_t iItem = 0;
	if (m_priority_tags.size()) {
		auto mid = std::stable_partition(list.begin(), list.end(),
			[this](const MStringW& w) { return m_priority_words.count(w) > 0; });
		iItem = (size_t)(mid - list.begin());
	}

	// 優先タグとそれ以外を別々にランダムシャッフル。
	XgRandomShuffle(list.begin(), list.begin() + iItem);
	XgRandomShuffle(list.begin() + iItem, list.end());
	return list;
}

// 単語リストをソート。
void __fastcall MDict::sort_word_list() {
	m_word_list = make_shuffled_list();
	m_word_list_by_len.clear();
	for (auto& word : m_word_list) {
		m_word_list_by_len[(WORD)word.size()].push_back(word);
	}
}

// 辞書リストを読み込む。
BOOL __fastcall XgLoadDictList(MDictList& dict_list) {
	dict_list.clear();

	auto dict_dir = XgFindLocalFile(L"DICT");
	for (const auto& files : { dict_dir + L"\\*.dic", dict_dir + L"\\*.tsv" }) {
		WIN32_FIND_DATAW find;
		HANDLE hFind = FindFirstFileW(files.c_str(), &find);
		if (hFind != INVALID_HANDLE_VALUE) {
			do {
				MStringW filename = find.cFileName;
				if (filename == L"." || filename == L"..")
					continue;

				MDict dict;
				auto file = dict_dir + L"\\" + filename;
				HRESULT hr = dict.load_file(file.c_str(), TRUE);
				if (FAILED(hr))
					continue;

				dict_list.push_back(dict);
			} while (FindNextFileW(hFind, &find));
			FindClose(hFind);
		}
	}

	return !dict_list.empty();
}

HRESULT __fastcall XgLoadKanaDict(MDict& dict) {
	auto dict_name = XgFindLocalFile(L"DICT/BasicDict-JPN-Kana-Freedom.tsv");
	return dict.load_file(dict_name.c_str());
}

HRESULT __fastcall XgLoadEnglishDict(MDict& dict) {
	auto dict_name = XgFindLocalFile(L"DICT/BasicDict-ENG-Freedom.tsv");
	return dict.load_file(dict_name.c_str());
}

// 現在の状態で好ましいと思われる単語の最大長を取得する。
// 旧来のスマート解決では最大長を指定する必要があった。
INT __fastcall XgGetPreferredMaxLength(const MDict& dict) noexcept {
	INT ret = 7;

	if (dict.m_word_list.size()) {
		auto& word = dict.m_word_list[0];
		auto ch = word[0];
		if (XgIsCharKanjiW(ch)) {
			// 漢字の熟語は4文字以下が多い。
			ret = 4;
		} else if (XgIsCharKanaW(ch)) {
			// 日本語の単語は4文字程度が多い。
			ret = 5;
		} else {
			// ロシア語や英語は日本語の単語より長い傾向にある。
			ret = 7;
		}
	}

	if (xg_nRules & (RULE_LINESYMMETRYH | RULE_LINESYMMETRYV)) {
		// 線対称のときに偶数長はまずいと思われる。
		if (ret % 2 == 0)
			ret += 1;
	}

	return ret;
}

// 単体テスト。
void __fastcall MDict::unittest() {
	MDict dict;
	HRESULT hr = dict.load_file(XgFindLocalFile(L"DICT/BasicDict-JPN-Kana-Freedom.tsv").c_str());
	assert(SUCCEEDED(hr));
	PrintfW(L"%s\n", dict.m_filename.c_str());
	PrintfW(L"%s\n", dict.m_pathname.c_str());
	PrintfW(L"%s\n", dict.m_english_name.c_str());
	PrintfW(L"%s\n", dict.m_japanese_name.c_str());
	PrintfW(L"%s\n", dict.m_theme_tags.c_str());
	PrintfW(L"%s\n", dict.m_default_theme_tags.c_str());
	for (auto& tag : dict.m_forbidden_tags) {
		PrintfW(L"%s\n", tag.c_str());
	}
	PrintfW(L"%d\n", (int)dict.m_word_list.size());
	dict.sort_word_list();
	PrintfW(L"%d\n", (int)dict.m_word_list.size());
	assert(dict.m_filename.size());
	assert(dict.m_pathname.size());
	assert(dict.m_comments.size());
	assert(dict.m_word2clue.size());
	assert(dict.m_word_list.size());
	assert(dict.m_english_name.size());
	assert(dict.m_japanese_name.size());
	assert(dict.m_theme_tags.size());
	assert(dict.m_word2tags.size());
}
