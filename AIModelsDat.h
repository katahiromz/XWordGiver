// AIModelsDat.h --- Shared loader/parser for AIModels.dat
// Author: katahiromz
// License: MIT
#pragma once

#include <string>
#include <vector>
#include <utility>

// (セクション名, 項目行) のペア列。
// セクション見出し（[...]）自体は含まれず、その時点までに見た最後のセクション名が
// 以降の各項目行に紐付けられる。コメント行（;で始まる）・空行は読み飛ばされる。
//
// A list of (section name, item line) pairs. Section headers ([...]) themselves
// are not included -- only the most recently seen section name is attached to
// each item line that follows it. Comment lines (starting with ';') and blank
// lines are skipped.
typedef std::vector<std::pair<std::wstring, std::wstring>> AIModelsDatLines;

// AIModels.dat をパースした (section, line) 列を返す。
// 実行ファイルのフォルダ、次いでカレントディレクトリの順にファイルを探し、
// 初回のみ読み込んでキャッシュする（以降はキャッシュを返す）。
// ファイルが見つからない場合は空の列を返す。
//
// Returns the parsed (section, line) pairs of AIModels.dat. The file is
// looked up next to the executable, then in the current directory; it is
// read only on first use and cached afterwards. Returns an empty list if the
// file cannot be found.
const AIModelsDatLines& GetAIModelsDatLines();

// キャッシュを破棄する。次回 GetAIModelsDatLines() 呼び出し時に
// AIModels.dat を読み直させたい場合に使う（通常は呼ぶ必要はない）。
//
// Discards the cache so the next call to GetAIModelsDatLines() re-reads
// AIModels.dat from disk. Not normally needed.
void InvalidateAIModelsDatCache();

// "a,b,c" の形式の文字列をカンマで分割する。
// 各フィールド前後の空白は許容し、結果からは取り除く。
// PROVIDER_INFO セクションの行（provider=envKey,host,port,...）を
// パースする際などに使う。
//
// Split a "a,b,c"-style string on commas. Leading/trailing whitespace
// around each field is allowed and stripped. Used e.g. to parse
// PROVIDER_INFO lines (provider=envKey,host,port,...).
std::vector<std::wstring> SplitCsvLine(const std::wstring& s);

// 文字列の前後の空白を取り除く。
std::wstring TrimW(const std::wstring& s);
