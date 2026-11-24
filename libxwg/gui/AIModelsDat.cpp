// AIModelsDat.cpp --- Shared loader/parser for AIModels.dat
// Author: katahiromz
// License: MIT
#include "xwg.h"
#include <windows.h>
#include <cstdio>
#include "AIModelsDat.h"

// ---------------------------------------------------------------------------
// ファイル探索・読み込み
// File lookup / reading
// ---------------------------------------------------------------------------

// 実行ファイルがあるフォルダを取得する。
// Get the folder that contains the running executable.
static MStringW GetAIModelsDatExeDirectoryW() {
	wchar_t path[MAX_PATH];
	DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
	if (n == 0 || n >= MAX_PATH)
		return L".";
	MStringW s = path;
	size_t pos = s.find_last_of(L"\\/");
	return (pos == MStringW::npos) ? MStringW(L".") : s.substr(0, pos);
}

// ファイルをUTF-8 (BOM可)として丸ごと読み込み、ワイド文字列に変換する。
// Read a whole file as UTF-8 (BOM optional) and convert it to a wide string.
static bool ReadAIModelsFileTextRaw(const MStringW& path, MStringW& outText) {
	FILE* fp = _wfopen(path.c_str(), L"rb");
	if (!fp)
		return false;

	MStringA data;
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
		data.append(buf, n);
	fclose(fp);

	// UTF-8 BOM (EF BB BF) を取り除く。
	if (data.size() >= 3 && memcmp(&data[0], "\xEF\xBB\xBF", 3) == 0) {
		data = data.substr(3);
	}

	outText = WideFromUtf8(data);
	return true;
}

// 実行ファイルのフォルダ、次いでカレントディレクトリの順に AIModels.dat を探す。
// Look for AIModels.dat next to the executable, then in the current directory.
static bool ReadAIModelsFile(MStringW& outText) {
	MStringW path = GetAIModelsDatExeDirectoryW();
	path += L"\\AIModels.dat";
	if (ReadAIModelsFileTextRaw(path, outText))
		return true;
	return ReadAIModelsFileTextRaw(MStringW(L"AIModels.dat"), outText);
}

// ---------------------------------------------------------------------------
// パース
// Parsing
// ---------------------------------------------------------------------------

// AIModels.dat をパースし、「(セクション名, 項目行)」のペア列に分解する。
// コメント行・空行は読み飛ばし、セクション見出し（[...]）自体は出力に含めない
// （その時点までに見た最後のセクション名を、以降の各項目行に紐付けるだけ）。
// LoadKnownAIModels / LoadKnownAIProviders / LoadAIModelsData など、
// AIModels.dat内の特定セクションだけを拾いたい処理は、すべてこの関数の
// 結果をフィルタリングするだけで実装できる。
static void ParseAIModelsDatLines(const MStringW& text, AIModelsDatLines& out) {
	out.clear();

	MStringW section;
	size_t pos = 0;
	while (pos <= text.size()) {
		size_t nl = text.find(L'\n', pos);
		MStringW line = (nl == MStringW::npos) ? text.substr(pos) : text.substr(pos, nl - pos);
		pos = (nl == MStringW::npos) ? text.size() + 1 : nl + 1;

		if (!line.empty() && line[line.size() - 1] == L'\r')
			line = line.substr(0, line.size() - 1);

		size_t s = line.find_first_not_of(L" \t");
		if (s == MStringW::npos)
			continue; // 空行
		size_t e = line.find_last_not_of(L" \t");
		line = line.substr(s, e - s + 1);

		if (line.empty() || line[0] == L';')
			continue; // コメント行

		if (line[0] == L'[' && line[line.size() - 1] == L']') {
			section = line.substr(1, line.size() - 2);
			continue;
		}

		out.push_back(std::make_pair(section, line));
	}
}

// ---------------------------------------------------------------------------
// アクセサ
// accessor
// ---------------------------------------------------------------------------

AIModelsDatLines GetAIModelsDatLines() {
	MStringW text;
	AIModelsDatLines aiModelsDatLines;
	if (ReadAIModelsFile(text))
		ParseAIModelsDatLines(text, aiModelsDatLines);
	return aiModelsDatLines;
}

// ---------------------------------------------------------------------------
// 補助
// Helpers
// ---------------------------------------------------------------------------

// 前後の空白（スペース・タブ）を除去する。
// Trim leading/trailing spaces and tabs.
MStringW TrimW(const MStringW& s) {
	size_t b = s.find_first_not_of(L" \t");
	if (b == MStringW::npos)
		return MStringW();
	size_t e = s.find_last_not_of(L" \t");
	return s.substr(b, e - b + 1);
}

// "a,b,c" の形式の文字列をカンマで分割する。
// Split a "a,b,c"-style string on commas.
MStringListW SplitCsvLine(const MStringW& s) {
	MStringListW out;
	size_t start = 0;
	for (;;) {
		size_t comma = s.find(L',', start);
		if (comma == MStringW::npos) {
			out.push_back(TrimW(s.substr(start)));
			break;
		}
		// 各フィールドの前後の空白は許容し、結果からは取り除く。
		// Leading/trailing whitespace around each field is allowed and stripped.
		out.push_back(TrimW(s.substr(start, comma - start)));
		start = comma + 1;
	}
	return out;
}
