// AIModelsDat.cpp --- Shared loader/parser for AIModels.dat
// Author: katahiromz
// License: MIT
#include <windows.h>
#include <cstdio>
#include "AIModelsDat.h"

// ---------------------------------------------------------------------------
// ファイル探索・読み込み
// File lookup / reading
// ---------------------------------------------------------------------------

// 実行ファイルがあるフォルダを取得する。
// Get the folder that contains the running executable.
static std::wstring GetAIModelsDatExeDirectoryW()
{
	wchar_t path[MAX_PATH];
	DWORD n = GetModuleFileNameW(nullptr, path, MAX_PATH);
	if (n == 0 || n >= MAX_PATH)
		return L".";
	std::wstring s(path, n);
	size_t pos = s.find_last_of(L"\\/");
	return (pos == std::wstring::npos) ? L"." : s.substr(0, pos);
}

// ファイルをUTF-8 (BOM可)として丸ごと読み込み、ワイド文字列に変換する。
// Read a whole file as UTF-8 (BOM optional) and convert it to a wide string.
static bool ReadAIModelsFileTextRaw(const std::wstring& path, std::wstring& outText)
{
	FILE* fp = _wfopen(path.c_str(), L"rb");
	if (!fp)
		return false;

	std::string data;
	char buf[4096];
	size_t n;
	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
		data.append(buf, n);
	fclose(fp);

	// UTF-8 BOM (EF BB BF) を取り除く。
	if (data.size() >= 3 &&
		(unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB && (unsigned char)data[2] == 0xBF)
	{
		data.erase(0, 3);
	}

	int len = MultiByteToWideChar(CP_UTF8, 0, data.c_str(), (int)data.size(), nullptr, 0);
	outText.assign((size_t)len, L'\0');
	if (len > 0)
		MultiByteToWideChar(CP_UTF8, 0, data.c_str(), (int)data.size(), &outText[0], len);
	return true;
}

// 実行ファイルのフォルダ、次いでカレントディレクトリの順に AIModels.dat を探す。
// Look for AIModels.dat next to the executable, then in the current directory.
static bool ReadAIModelsFile(std::wstring& outText)
{
	if (ReadAIModelsFileTextRaw(GetAIModelsDatExeDirectoryW() + L"\\AIModels.dat", outText))
		return true;
	return ReadAIModelsFileTextRaw(L"AIModels.dat", outText);
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
static void ParseAIModelsDatLines(const std::wstring& text, AIModelsDatLines& out)
{
	out.clear();

	std::wstring section;
	size_t pos = 0;
	while (pos <= text.size())
	{
		size_t nl = text.find(L'\n', pos);
		std::wstring line = (nl == std::wstring::npos) ? text.substr(pos) : text.substr(pos, nl - pos);
		pos = (nl == std::wstring::npos) ? text.size() + 1 : nl + 1;

		if (!line.empty() && line.back() == L'\r')
			line.pop_back();

		size_t s = line.find_first_not_of(L" \t");
		if (s == std::wstring::npos)
			continue; // 空行
		size_t e = line.find_last_not_of(L" \t");
		line = line.substr(s, e - s + 1);

		if (line.empty() || line[0] == L';')
			continue; // コメント行

		if (line.front() == L'[' && line.back() == L']')
		{
			section = line.substr(1, line.size() - 2);
			continue;
		}

		out.push_back(std::make_pair(section, line));
	}
}

// ---------------------------------------------------------------------------
// キャッシュ付きアクセサ
// Cached accessor
// ---------------------------------------------------------------------------

static AIModelsDatLines s_aiModelsDatLines;
static bool s_aiModelsDatLoaded = false;

const AIModelsDatLines& GetAIModelsDatLines()
{
	if (!s_aiModelsDatLoaded)
	{
		std::wstring text;
		if (ReadAIModelsFile(text))
			ParseAIModelsDatLines(text, s_aiModelsDatLines);
		s_aiModelsDatLoaded = true; // 見つからなかった場合も再試行はしない（従来の挙動を踏襲）
	}
	return s_aiModelsDatLines;
}

void InvalidateAIModelsDatCache()
{
	s_aiModelsDatLines.clear();
	s_aiModelsDatLoaded = false;
}

// ---------------------------------------------------------------------------
// 補助
// Helpers
// ---------------------------------------------------------------------------

// 前後の空白（スペース・タブ）を除去する。
// Trim leading/trailing spaces and tabs.
std::wstring TrimW(const std::wstring& s)
{
	size_t b = s.find_first_not_of(L" \t");
	if (b == std::wstring::npos)
		return {};
	size_t e = s.find_last_not_of(L" \t");
	return s.substr(b, e - b + 1);
}

// "a,b,c" の形式の文字列をカンマで分割する。
// Split a "a,b,c"-style string on commas.
std::vector<std::wstring> SplitCsvLine(const std::wstring& s)
{
	std::vector<std::wstring> out;
	size_t start = 0;
	for (;;) {
		size_t comma = s.find(L',', start);
		if (comma == std::wstring::npos) {
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
