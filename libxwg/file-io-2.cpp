// file-io-2.cpp --- File Input/Output Part 2
// Author: katahiromz
// License: MIT

#include "file-io-2.h"
#include "board-image.h"
#include "board-text-io.h"
#include "marks.h"
#include <map>

#ifdef USE_JSON
	#include "../json/include/nlohmann/json.hpp"
	using json = nlohmann::json;
#endif

// マーク文字列を取得する2。
static void __fastcall XgGetStringOfMarks2(MBoardDisplay& display, MStringW& str) {
	WCHAR sz[64];
	str.clear();
	INT i = 0;
	for (const auto& mark : display.m_vMarks) {
		const WCHAR szLetter[2] = { display.m_solution.GetCell(mark.m_j, mark.m_i), 0 };
		auto letter = XgConvertString(szLetter);
		StringCchPrintfW(sz, _countof(sz), L"MARK%d. (%u, %u): %s\n",
						 i + 1, mark.m_j + 1, mark.m_i + 1, letter.c_str());
		str += sz;
		++i;
	}
}

// XDファイルを読み込む。
BOOL XgLoadXDFile(MBoardDisplay& display, LPCWSTR filename) {
	// とにかく全部読み込む。
	MStringW text;
	if (!XgReadTextFileAll(filename, text, FALSE))
		return FALSE;

	MStringW header, notes;
	MStringListW lines, rows, clues, marks;
	std::vector<MPlaceInfo> vHorzInfo, vVertInfo;
	MStringMapW word2clue;
	std::unordered_map<WCHAR, INT> mapNumCro1;
	std::unordered_map<INT, WCHAR> mapNumCro2;
	XG_VIEW_MODE view_mode = display.m_nViewMode;
	INT nRules = display.m_board.Rules();
	std::vector<MBoardPos> vMarks;

	// 文字列を行に分割する。
	INT iSection = 0;
	INT cEmpty = 0;
	XgStrSplit(lines, text.c_str(), L"\n");

	// 各行について。
	for (auto& line : lines) {
		XgStrTrim(line); // 前後の空白を取り除く。

		if (line.empty() && iSection < 3) {
			if (cEmpty == 1) {
				++iSection;
			}
			++cEmpty; // 空行を数える。
		} else {
			cEmpty = 0;
			switch (iSection) { // セクションに応じて分岐。
			case 0:
				header += line;
				header += L"\r\n";
				break;
			case 1:
				rows.push_back(line);
				break;
			case 2:
				clues.push_back(line);
				break;
			case 3:
				// 二重マス単語。
				if (line.find(L"MARK") == 0 && L'0' <= line[4] && line[4] <= L'9') {
					marks.push_back(line);
				} else if (line.find(L"NUMCRO-") == 0 && L'0' <= line[7] && line[7] <= L'9') {
					// ナンクロモード。
					const auto ich = line.find(L": ");
					INT number = wcstoul(&line.c_str()[7], nullptr, 10);
					WCHAR ch = line.c_str()[ich + 2];
					mapNumCro1[ch] = number;
					mapNumCro2[number] = ch;
				} else if (line.find(L"Box: ") == 0) {
					// ボックスは廃止。
				} else if (line.find(L"ViewMode:") == 0) {
					// ビューモード。
					view_mode = static_cast<XG_VIEW_MODE>(wcstoul(&line[9], nullptr, 0));
					switch (view_mode) {
					case XG_VIEW_NORMAL:
					case XG_VIEW_SKELETON:
						break;
					default:
						if (XgIsUserJapanese())
							view_mode = XG_VIEW_NORMAL;
						else
							view_mode = XG_VIEW_SKELETON;
					}
				} else if (line.find(L"Policy:") == 0) {
					nRules = (wcstoul(&line[7], nullptr, 0) | RULE_DONTDIVIDE);
				} else {
					notes += line;
					notes += L"\r\n";
				}
				break;
			default:
				break;
			}
		}
	}

	// 妥当性チェック。
	if (header.empty())
		return FALSE;
	if (rows.size() < XG_MIN_SIZE || rows[0].size() < XG_MIN_SIZE)
		return FALSE;
	if (rows.size() > XG_MAX_SIZE || rows[0].size() > XG_MAX_SIZE)
		return FALSE;
	for (auto& line : rows) {
		if (line.size() != rows[0].size())
			return FALSE;
	}

	// 正規化。
	for (auto& line : rows) {
		for (auto& ch : line) {
			if (ch == L' ' || ch == L'_' || ch == ZEN_UNDERLINE)
				ch = ZEN_SPACE;
			else if (ch == L'#' || ch == ZEN_SHARP1 || ch == ZEN_SHARP2 || ch == L'.' || ch == ZEN_DOT)
				ch = ZEN_BLACK;
		}
		line = XgNormalizeString(line);
	}

	// 二重マスを解析する。
	for (auto& mark : marks) {
		if (mark.substr(0, 4) == L"MARK" && L'0' <= mark[4] && mark[4] <= L'9') {
			const auto i0 = mark.find(L'(');
			const auto i1 = mark.find(L", ");
			const INT x = _wtoi(&mark[i0 + 1]) - 1;
			const INT y = _wtoi(&mark[i1 + 2]) - 1;
			XgSetMark(vMarks, MBoardPos(x, y));
		}
	}

	// 盤面を構築する。
	const INT nColumns = INT(rows[0].size()), nRows = INT(rows.size());
	MBoard board(nColumns, nRows, nRules);
	for (INT iRow = 0; iRow < nRows; ++iRow) {
		for (INT jColumn = 0; jColumn < nColumns; ++jColumn) {
			board.SetCell(jColumn, iRow, rows[iRow][jColumn]);
		}
	}

	display.m_bSolved = FALSE;
	if (board.IsFull() && clues.size()) {
		// 番号付け。
		if (XgDoNumbering(board, vHorzInfo, vVertInfo)) {
			// 単語からカギへの写像を構築する。
			for (auto& str0 : clues) {
				XgStrTrim(str0);
				if (str0.empty() || (str0[0] != L'A' && str0[0] != L'D'))
					break;
				const auto iDot = str0.find(L'.');
				const auto iTilda = str0.rfind(L'~');
				if (iDot != str0.npos && iTilda != str0.npos) {
					auto word = str0.substr(iTilda + 1);
					XgStrTrim(word);
					auto hint = str0.substr(iDot + 1, iTilda - (iDot + 1));
					XgStrTrim(hint);
					word = XgNormalizeString(word);
					word2clue[word] = hint;
				}
			}
			display.m_bSolved = !word2clue.empty();
		}
	}

	// 盤面を構築する。
	display.m_vHorzInfo = std::move(vHorzInfo);
	display.m_vVertInfo = std::move(vVertInfo);
	display.m_board = board;
	xg_nRules = board.Rules();
	display.m_solution = board;
	if (display.m_bSolved)
		display.m_board.ClearLetters();
	display.m_caret_pos = { 0, 0 };
	display.m_bCheckingAnswer = FALSE;
	display.m_header = std::move(header);
	display.m_notes = std::move(notes);
	display.m_word2clue = std::move(word2clue);
	display.m_mapNumCro1 = std::move(mapNumCro1);
	display.m_mapNumCro2 = std::move(mapNumCro2);
	display.m_vMarks = std::move(vMarks);
	display.m_nViewMode = view_mode;
	if (display.m_mapNumCro1.size())
		XgMakeItNumCro(display);
	return TRUE;
}

// XDファイルを書き込む。
BOOL XgSaveXDFile(MBoardDisplay& display, LPCWSTR filename) {
	// ファイルを作成する。
	FILE *fout = _wfopen(filename, L"wb");
	if (!fout)
		return FALSE;

	auto& board = display.m_bSolved ? display.m_solution : display.m_board;
	auto& vHorzInfo = display.m_vHorzInfo;
	auto& vVertInfo = display.m_vVertInfo;
	auto& word2clue = display.m_word2clue;
	auto& vMarks = display.m_vMarks;

	// アジア系を検出する。
	BOOL bAsian = FALSE;
	for (INT i = 0; i < board.Rows(); ++i) {
		for (INT j = 0; j < board.Columns(); ++j) {
			const WCHAR ch = board.GetCell(j, i);
			if (XgIsCharKanaW(ch) || XgIsCharKanjiW(ch) || ch == ZEN_PROLONG || XgIsCharHangulW(ch)) {
				bAsian = TRUE;
				j = board.Columns();
				i = board.Rows();
			}
		}
	}

	try {
		// ヘッダ。
		auto strHeader = display.m_header;
		XgStrTrim(strHeader);
		if (strHeader.empty()) {
			WCHAR szFileTitle[MAX_PATH];
			StringCchCopyW(szFileTitle, _countof(szFileTitle), PathFindFileNameW(filename));
			PathRemoveExtensionW(szFileTitle);
			fprintf(fout, "Title: %s\n", Utf8FromWide(szFileTitle).c_str());
			fprintf(fout, "Author: \n");
			fprintf(fout, "Editor: \n");
			fprintf(fout, "Copyright: \n");
			SYSTEMTIME st;
			::GetLocalTime(&st);
			fprintf(fout, "Date: %04u-%02u-%02u\n", st.wYear, st.wMonth, st.wDay);
		} else {
			XgStrReplace(strHeader, L"\r\n", L"\n");
			XgStrTrim(strHeader);
			fprintf(fout, "%s\n", Utf8FromWide(strHeader).c_str());
		}
		fprintf(fout, "\n\n");

		// マス。
		for (INT i = 0; i < board.Rows(); ++i) {
			MStringW row;
			for (INT j = 0; j < board.Columns(); ++j) {
				const WCHAR ch = board.GetCell(j, i);
				if (bAsian) {
					if (ch == ZEN_SPACE)
						row += ZEN_UNDERLINE;
					else if (ch == ZEN_BLACK)
						row += ZEN_BLACK;
					else
						row += ch;
				} else {
					if (ch == ZEN_SPACE)
						row += L'_';
					else if (ch == ZEN_BLACK)
						row += L'#';
					else
						row += ch;
				}
			}
			if (bAsian) {
				if (display.m_bHiragana)
					row = XgLCMapString(row, LCMAP_HIRAGANA);
			} else {
				row = XgNaturalizeString(row);
				if (!display.m_bLowercase)
					row = XgLCMapString(row, LCMAP_UPPERCASE);
			}
			fprintf(fout, "%s\n", Utf8FromWide(row).c_str());
		}
		fprintf(fout, "\n\n");

		// ヒント。
		if (display.m_bSolved && (vHorzInfo.size() || vVertInfo.size())) {
			char line[512];
			MStringA strACROSS, strDOWN;
			// タテのカギ。
			for (auto& tate_hint : vVertInfo) {
				auto word = tate_hint.m_word;
				StringCchPrintfA(line, _countof(line), "D%d. %s ~ %s\n",
								 tate_hint.m_number,
								 Utf8FromWide(word2clue[word]).c_str(),
								 Utf8FromWide(word).c_str());
				strDOWN += line;
			}
			// ヨコのカギ。
			for (auto& yoko_hint : vHorzInfo) {
				auto word = yoko_hint.m_word;
				StringCchPrintfA(line, _countof(line), "A%d. %s ~ %s\n",
								 yoko_hint.m_number,
								 Utf8FromWide(word2clue[word]).c_str(),
								 Utf8FromWide(word).c_str());
				strACROSS += line;
			}

			fprintf(fout, "%s\n%s\n\n", strACROSS.c_str(), strDOWN.c_str());
		} else {
			// ヒントなし。
			fprintf(fout, "(No clues)\n\n\n");
		}

		// マーク。
		if (vMarks.size()) {
			MStringW strMarks;
			XgGetStringOfMarks2(display, strMarks);
			fprintf(fout, "%s\n\n", Utf8FromWide(strMarks).c_str());
		}

		// ビューモード。
		fprintf(fout, "ViewMode: %d\n\n", display.m_nViewMode);

		// ナンクロモード。
		if (display.m_bNumCroMode) {
			std::map<INT, WCHAR> mapping;
			for (auto& pair : display.m_mapNumCro1) {
				mapping[pair.second] = pair.first;
			}
			for (auto& pair : mapping) {
				const WCHAR sz[2] = { pair.second, 0 };
				fprintf(fout, "NUMCRO-%04d: %s\n", pair.first, Utf8FromWide(sz).c_str());
			}
			fprintf(fout, "\n");
		}

		// 備考欄。
		if (display.m_notes.size()) {
			auto str = display.m_notes;
			XgStrReplace(str, L"\r\n", L"\n");
			XgStrTrim(str);
			if (str.size()) {
				fprintf(fout, "%s\n", Utf8FromWide(str).c_str());
			}
		}

		// ルール。
		fprintf(fout, "\nPolicy: %d\n", xg_nRules);

		fclose(fout);
		return TRUE;
	}
	catch(...)
	{
		;
	}

	// 正しく書き込めなかった。不正なファイルを消す。
	::DeleteFileW(filename);
	return FALSE;
}

struct ECW_ENTRY {
	MStringW word;
	INT iColumn;
	INT iRow;
	MStringW clue;
	BOOL across;
};

BOOL __fastcall XgParseEcwEntry(std::vector<ECW_ENTRY>& entries, const MStringW& line, BOOL across) {
	std::vector<MStringW> fields;
	XgStrSplit(fields, line, L":");
	if (fields.size() < 3)
		return FALSE;
	XgStrTrim(fields[0]);
	XgStrTrim(fields[1]);
	XgStrTrim(fields[2]);
	if (fields[0].empty())
		return FALSE;
	if (fields[1].empty())
		return FALSE;

	MStringListW column_and_row;
	XgStrSplit(column_and_row, fields[1], L",");
	if (column_and_row.size() != 2)
		return FALSE;

	XgStrTrim(column_and_row[0]);
	XgStrTrim(column_and_row[1]);
	INT iColumn = _wtoi(column_and_row[0].c_str());
	INT iRow = _wtoi(column_and_row[1].c_str());
	if (iColumn <= 0 || iRow <= 0)
		return FALSE;

	ECW_ENTRY entry = { XgNormalizeString(fields[0]), iColumn, iRow, fields[2], across };
	entries.push_back(entry);
	return TRUE;
}

// ECW (EclipseCrossword) 形式のファイルを読み込む。
BOOL XgLoadEcwFile(MBoardDisplay& display, LPCWSTR filename) {
	// とにかく全部読み込む。
	MStringW text;
	if (!XgReadTextFileAll(filename, text, TRUE))
		return FALSE;

	// 行に分割する。
	MStringListW lines;
	XgStrSplit(lines, text.c_str(), L"\n");

	MStringW title, copyright, author;
	MStringMapW word2clue;
	INT width = 0, height = 0;
	std::vector<ECW_ENTRY> entries;
	BOOL across = FALSE, down = FALSE;
	std::vector<MPlaceInfo> vHorzInfo, vVertInfo;

	// 1行ずつ処理する。
	for (auto& line : lines) {
		XgStrTrim(line);
		if (line.empty()) // 空行？
			continue;
		if (line[0] == L';') // コメント行？
			continue;

		size_t index;

		index = line.find(L"Title:");
		if (index == 0) {
			title = &line.c_str()[6];
			XgStrTrim(title);
			continue;
		}

		index = line.find(L"Author:");
		if (index == 0) {
			author = &line.c_str()[7];
			XgStrTrim(author);
			continue;
		}

		index = line.find(L"Width:");
		if (index == 0) {
			width = _wtoi(&line.c_str()[6]);
			if (width <= 1 || width > XG_MAX_SIZE)
				return FALSE;
			continue;
		}

		index = line.find(L"Height:");
		if (index == 0) {
			height = _wtoi(&line.c_str()[7]);
			if (height <= 1 || height > XG_MAX_SIZE)
				return FALSE;
			continue;
		}

		index = line.find(L"CodePage:");
		if (index == 0) {
			continue;
		}

		index = line.find(L"Copyright:");
		if (index == 0) {
			copyright = &line.c_str()[10];
			XgStrTrim(copyright);
			continue;
		}

		if (line[0] == L'*') {
			line = line.substr(1);
			XgStrTrim(line);
			if (lstrcmpiW(line.c_str(), L"Across words") == 0) {
				across = TRUE;
				down = FALSE;
				continue;
			}
			if (lstrcmpiW(line.c_str(), L"Down words") == 0) {
				across = FALSE;
				down = TRUE;
				continue;
			}
		}

		if (across) {
			XgParseEcwEntry(entries, line, TRUE);
			continue;
		}
		if (down) {
			XgParseEcwEntry(entries, line, FALSE);
			continue;
		}
	}

	if (width < XG_MIN_SIZE || height < XG_MIN_SIZE)
		return FALSE;
	if (width > XG_MAX_SIZE || height > XG_MAX_SIZE)
		return FALSE;

	MBoard board(width, height, display.m_board.Rules(), ZEN_BLACK);
	for (auto& entry : entries) {
		for (size_t i = 0; i < entry.word.size(); ++i) {
			if (entry.across) {
				board.SetCell(INT(i) + entry.iColumn - 1, entry.iRow - 1, entry.word[i]);
			} else {
				board.SetCell(entry.iColumn - 1, INT(i) + entry.iRow - 1, entry.word[i]);
			}
		}
	}

	MStringW header;
	header += L"Title: " + title + L"\r\n";
	header += L"Author: " + author + L"\r\n";
	header += L"Copyright: " + copyright;

	display.m_bSolved = FALSE;
	if (board.IsFull()) {
		// 番号付け。
		if (XgDoNumbering(board, vHorzInfo, vVertInfo)) {
			// 単語からカギへの写像を構築する。
			for (auto& entry : entries) {
				word2clue[entry.word] = entry.clue;
			}
			display.m_bSolved = !word2clue.empty();
		}
	}

	// 盤面を構築する。
	display.m_vHorzInfo = std::move(vHorzInfo);
	display.m_vVertInfo = std::move(vVertInfo);
	display.m_board = board;
	xg_nRules = board.Rules();
	display.m_solution = board;
	if (display.m_bSolved)
		display.m_board.ClearLetters();
	display.m_caret_pos = { 0, 0 };
	display.m_bCheckingAnswer = FALSE;
	display.m_header = std::move(header);
	display.m_notes.clear();
	display.m_word2clue = std::move(word2clue);
	display.m_mapNumCro1.clear();
	display.m_mapNumCro2.clear();
	display.m_bNumCroMode = FALSE;
	display.m_vMarks.clear();
	display.m_nViewMode = XG_VIEW_SKELETON;
	return TRUE;
}

// CRP/CRXファイル（Crossword Builder）を読み込む。
BOOL XgLoadCrpFile(MBoardDisplay& display, LPCWSTR filename) {
	// サイズのチェック。
	const INT nWidth = GetPrivateProfileIntW(L"Cross", L"Width", -1, filename);
	const INT nHeight = GetPrivateProfileIntW(L"Cross", L"Height", -1, filename);
	if (nWidth < XG_MIN_SIZE && nHeight < XG_MIN_SIZE)
		return FALSE;
	if (nWidth > XG_MAX_SIZE && nHeight > XG_MAX_SIZE)
		return FALSE;

	// マスの処理。
	std::vector<MStringW> rows;
	for (INT i = 0; i < nHeight; ++i) {
		WCHAR szName[128], szText[523];
		StringCchPrintf(szName, _countof(szName), L"Line%d", i + 1);
		GetPrivateProfileStringW(L"Cross", szName, L"", szText, _countof(szText), filename);

		MStringW str = szText;
		XgStrTrim(str);
		XgStrReplace(str, L",", L"");
		XgStrReplace(str, { ZEN_UNDERLINE } , { ZEN_SPACE });
		str = XgNormalizeString(str);
		if (INT(str.size()) != nWidth)
			return FALSE;

		rows.push_back(str);
	}
	MBoard board(nWidth, nHeight, display.m_board.Rules());
	for (INT iRow = 0; iRow < nHeight; ++iRow) {
		for (INT jColumn = 0; jColumn < nWidth; ++jColumn) {
			board.SetCell(jColumn, iRow, rows[iRow][jColumn]);
		}
	}

	// カギの処理。
	std::vector<MPlaceInfo> vHorzInfo, vVertInfo;
	display.m_bSolved = FALSE;
	MStringMapW word2clue;
	if (board.IsFull()) {
		// 番号付け。
		if (XgDoNumbering(board, vHorzInfo, vVertInfo)) {
			const INT nClueCount = GetPrivateProfileIntW(L"Clue", L"Count", 0, filename);
			if (nClueCount > 0) { // CRP形式の場合。
				for (INT i = 0; i < nClueCount; ++i) {
					WCHAR szName[128], szText[523];
					StringCchPrintfW(szName, _countof(szName), L"Clue%d", i + 1);
					GetPrivateProfileStringW(L"Clue", szName, L"", szText, _countof(szText), filename);
					MStringW str = szText;
					XgStrTrim(str);
					if (str.empty())
						break;
					auto ich0 = str.find(L':');
					if (ich0 != str.npos) {
						auto word = XgNormalizeString(str.substr(0, ich0));
						auto hint = str.substr(ich0 + 1);
						word2clue[word] = hint;
					} else {
						word2clue[str] = L"";
					}
				}
			} else { // CRX形式の場合。
				// タテ。
				for (INT i = 0; i < 512; ++i) {
					WCHAR szName[128], szText[523];
					StringCchPrintfW(szName, _countof(szName), L"Down%d", i + 1);
					GetPrivateProfileStringW(L"Clue", szName, L"", szText, _countof(szText), filename);

					MStringW str = szText;
					XgStrTrim(str);
					if (str.empty())
						break;
					if (str == L"{N/A}")
						continue;
					for (auto& item : vVertInfo) {
						if (item.m_number == i + 1) {
							word2clue[item.m_word] = str;
							break;
						}
					}
				}
				// ヨコ。
				for (INT i = 0; i < 512; ++i) {
					WCHAR szName[128], szText[523];
					StringCchPrintfW(szName, _countof(szName), L"Across%d", i + 1);
					GetPrivateProfileStringW(L"Clue", szName, L"", szText, _countof(szText), filename);

					MStringW str = szText;
					XgStrTrim(str);
					if (str.empty())
						break;
					if (str == L"{N/A}")
						continue;
					for (auto& item : vHorzInfo) {
						if (item.m_number == i + 1) {
							word2clue[item.m_word] = str;
							break;
						}
					}
				}
			}
			display.m_bSolved = !word2clue.empty();
		}
	}

	// 盤面を構築する。
	display.m_vHorzInfo = std::move(vHorzInfo);
	display.m_vVertInfo = std::move(vVertInfo);
	display.m_board = board;
	xg_nRules = board.Rules();
	display.m_solution = board;
	if (display.m_bSolved)
		display.m_board.ClearLetters();
	display.m_caret_pos = { 0, 0 };
	display.m_bCheckingAnswer = FALSE;
	display.m_header.clear();
	display.m_notes.clear();
	display.m_word2clue = std::move(word2clue);
	display.m_mapNumCro1.clear();
	display.m_mapNumCro2.clear();
	display.m_bNumCroMode = FALSE;
	display.m_vMarks.clear();
	display.m_nViewMode = XG_VIEW_NORMAL;
	return TRUE;
}

// XWDファイルのカギ文字列をパースする。
BOOL XgParseClues(MStringMapW& word2clue, const MStringW& text) {
	word2clue.clear();

	for (size_t i = 0;;) {
		auto i3 = text.find(L"≪", i); // U+226A
		if (i3 == text.npos)
			break;
		i3 += wcslen(L"≪"); // U+226A

		auto i4 = text.find(L"≫", i3); // U+226B
		if (i4 == text.npos)
			return FALSE;
		auto word = text.substr(i3, i4 - i3);
		i4 += wcslen(L"≫"); // U+226B

		auto i5 = text.find(L'\n', i4);
		if (i5 == text.npos) {
			auto hint = text.substr(i4);
			XgStrReplace(hint, L"\r", L"");
			XgStrReplace(hint, L"\n", L"");
			XgStrReplace(hint, L"\t", L"");
			XgStrTrim(hint);
			word2clue[word] = hint;
			break;
		} else {
			auto hint = text.substr(i4, i5 - i4);
			XgStrReplace(hint, L"\r", L"");
			XgStrReplace(hint, L"\n", L"");
			XgStrReplace(hint, L"\t", L"");
			XgStrTrim(hint);
			word2clue[word] = hint;
			i = i5;
		}
	}

	return !word2clue.empty();
}

// XWDファイルの二重マスを読み込む。
static void XgSetStringOfMarks(std::vector<MBoardPos>& vMarks, LPCWSTR psz) {
	// 初期化する。
	vMarks.clear();

	// 区切り線があるか？
	LPCWSTR pch = XgIsUserJapanese() ? L"――――――――――――――" : L"----------------------------";
	psz = wcsstr(psz, pch);
	if (psz == nullptr) {
		// 区切り線がなければ二重マスの情報はない。
		return;
	}
	psz += wcslen(pch);

	// マーク文字列を解析する。
	while (*psz && *psz != ZEN_ULEFT) {
		// 読み飛ばし。
		while (*psz == L' ' || *psz == L'\r' || *psz == L'\n' || *psz == L'(')
			psz++;
		if (*psz == L'\0' || *psz == ZEN_ULEFT)
			break;

		// 数字がなければ終了。
		if (!(L'0' <= *psz && *psz <= L'9'))
			break;

		// 行のインデックスを読み込む。
		const INT i = _wtoi(psz) - 1;
		while (L'0' <= *psz && *psz <= L'9')
			psz++;
		if (!*psz || *psz == ZEN_ULEFT)
			break;

		// 読み飛ばし。
		while (*psz == L' ' || *psz == L',')
			psz++;

		// 数字がなければ終了。
		if (!(L'0' <= *psz && *psz <= L'9'))
			break;

		// 列のインデックスを読み込む。
		const INT j = _wtoi(psz) - 1;

		// 読み飛ばし。
		while ((L'0' <= *psz && *psz <= L'9') || *psz == L')')
			psz++;

		while (XgIsCharKanaW(*psz) || XgIsCharHankakuAlphaW(*psz) ||
			   XgIsCharZenkakuAlphaW(*psz) || XgIsCharKanjiW(*psz) ||
			   XgIsCharHangulW(*psz) || *psz == ZEN_PROLONG)
		{
			psz++;
		}

		// 終わりであれば終了。
		if (!*psz || *psz == ZEN_ULEFT)
			break;

		// マークを設定する。
		XgSetMark(vMarks, MBoardPos(j, i));
	}
}

// XWDファイル（クロスワード ギバー）を読み込む。
BOOL XgLoadXwdFile(MBoardDisplay& display, LPCWSTR filename) {
	// とにかく全部読み込む。
	HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	MAutoCloseHandle auto_close(hFile);
	DWORD size = GetFileSize(hFile, nullptr);
	if (size == INVALID_FILE_SIZE)
		return FALSE;
	size /= sizeof(WCHAR);
	MStringW text;
	text.resize(size);
	DWORD has_read, cbSize = DWORD(size * sizeof(WCHAR));
	if (!ReadFile(hFile, text.data(), cbSize, &has_read, nullptr) || has_read != cbSize)
		return FALSE;

	// ヘッダーを取得して、取り除く。
	MStringW header;
	MStringW strHeaderSep = XgIsUserJapanese() ? L"----------(ここから)-----------\r\n" : L"----------(FROM HERE)-----------\r\n";
	auto i0 = text.find(strHeaderSep);
	if (i0 != text.npos) {
		header = text.substr(0, i0);
		text = text.substr(i0 + strHeaderSep.size());
	}
	XgStrTrim(header);

	// フッターの備考欄を取得して、取り除く。
	MStringW notes;
	MStringW strFooterSep = XgIsUserJapanese() ? L"----------(ここまで)-----------" : L"----------(TO HERE)-----------";
	auto i3 = text.find(strFooterSep);
	if (i3 != text.npos) {
		notes = text.substr(i3 + strFooterSep.size());
		MStringW strBelowIsNotes = XgIsUserJapanese() ? L"\r\n(以下、備考欄)" : L"\r\n(Remarks below)";
		if (notes.find(strBelowIsNotes) == 0)
			notes = notes.substr(strBelowIsNotes.size());
		text = text.substr(0, i3);
	}
	XgStrTrim(notes);

	// テキストから盤面を読み込む。
	MBoard board = display.m_board;
	if (!XgBoardFromText(board, text)) {
		return FALSE;
	}

	// 妥当性チェック。
	const INT nColumns = board.Columns(), nRows = board.Rows();
	if (nColumns < XG_MIN_SIZE && nRows < XG_MIN_SIZE)
		return FALSE;
	if (nColumns > XG_MAX_SIZE && nRows > XG_MAX_SIZE)
		return FALSE;

	// カギの処理。
	std::vector<MPlaceInfo> vHorzInfo, vVertInfo;
	display.m_bSolved = FALSE;
	MStringMapW word2clue;
	if (board.IsFull()) {
		// 番号付け。
		if (XgDoNumbering(board, vHorzInfo, vVertInfo)) {
			if (XgParseClues(word2clue, text))
				display.m_bSolved = !word2clue.empty();
		}
	}

	// XWDファイルの二重マスを読み込む。
	std::vector<MBoardPos> vMarks;
	XgSetStringOfMarks(vMarks, text.c_str());

	// 盤面を構築する。
	display.m_vHorzInfo = std::move(vHorzInfo);
	display.m_vVertInfo = std::move(vVertInfo);
	display.m_board = board;
	xg_nRules = board.Rules();
	display.m_solution = board;
	if (display.m_bSolved)
		display.m_board.ClearLetters();
	display.m_caret_pos = { 0, 0 };
	display.m_bCheckingAnswer = FALSE;
	display.m_header = std::move(header);
	display.m_notes = std::move(notes);
	display.m_word2clue = std::move(word2clue);
	display.m_mapNumCro1.clear();
	display.m_mapNumCro2.clear();
	display.m_bNumCroMode = FALSE;
	display.m_vMarks = std::move(vMarks);
	display.m_nViewMode = XG_VIEW_NORMAL;
	return TRUE;
}

#ifdef USE_JSON

// 文字列のルールを解析する。
static INT __fastcall XgParseRules(const MStringW& str)
{
	INT nRules = 0;
	MStringListW rules;
	if (str.find(L" / ") != str.npos) { // もし" / "が含まれていたら
		XgStrSplit(rules, str.c_str(), L"/"); // "/"で分割する。
	} else { // 含まれていなければ
		XgStrSplit(rules, str.c_str(), L" \t"); // 空白で分割する。
	}
	for (auto& rule : rules) {
		XgStrTrim(rule); // 前後の空白を取り除く。
		if (rule.empty())
			continue;
		if (rule == L"連黒禁" || rule == L"Prohibit adjacent blocks") {
			nRules |= RULE_DONTDOUBLEBLACK;
		} else if (rule == L"四隅黒禁" || rule == L"Prohibit blocks at 4 corners") {
			nRules |= RULE_DONTCORNERBLACK;
		} else if (rule == L"三方黒禁" || rule == L"Prohibit three-way blocks") {
			nRules |= RULE_DONTTRIDIRECTIONS;
		} else if (rule == L"分断禁" || rule == L"No division by blocks") {
			nRules |= RULE_DONTDIVIDE;
		} else if (rule == L"黒斜三連禁" || rule == L"Prohibit diagonal 3 blocks") {
			nRules |= RULE_DONTTHREEDIAGONALS;
		} else if (rule == L"黒斜四連禁" || rule == L"Prohibit diagonal 4 blocks") {
			nRules |= RULE_DONTFOURDIAGONALS;
		} else if (rule == L"黒マス点対称" || rule == L"Point symmetry of blocks") {
			nRules |= RULE_POINTSYMMETRY;
		} else if (rule == L"黒マス上下対称" || rule == L"Vertical symmetry of blocks") {
			nRules |= RULE_LINESYMMETRYV;
		} else if (rule == L"黒マス左右対称" || rule == L"Horizontal symmetry of blocks") {
			nRules |= RULE_LINESYMMETRYH;
		}
	}
	return nRules;
}

static auto XgWideFromJson = [](json& j) -> MStringW {
	return WideFromUtf8(j.get<std::string>().c_str());
};

// XWJ (JSON)ファイル（クロスワード ギバー）を読み込む。
BOOL XgLoadXwjFile(MBoardDisplay& display, LPCWSTR filename) {
	// とにかく全部読み込む。
	MStringW wide;
	if (!XgReadTextFileAll(filename, wide, FALSE))
		return FALSE;

	BOOL success = TRUE;
	MStringMapW word2clue;

	try {
		MStringA utf8 = Utf8FromWide(wide);
		json j0 = json::parse(utf8.c_str());
		INT row_count = j0["row_count"];
		INT column_count = j0["column_count"];
		auto cell_data = j0["cell_data"];
		const bool is_solved = j0["is_solved"];
		const bool has_mark = j0["has_mark"];
		const bool has_hints = j0["has_hints"];
		const bool is_numcro = j0.contains("is_numcro") && j0["is_numcro"];
		INT rules = XG_STANDARD_RULES;
		if (j0["policy"].is_number_integer()) {
			rules = INT(j0["policy"]);
		} else if (j0["rules"].is_string()) {
			auto str0 = XgWideFromJson(j0["rules"]);
			rules = XgParseRules(str0);
		}
		MStringW dictionary;
		if (j0["dictionary"].is_string()) {
			dictionary = XgWideFromJson(j0["dictionary"]);
		}
		XG_VIEW_MODE nViewMode;
		if (j0["view_mode"].is_number_integer()) {
			switch (int(j0["view_mode"])) {
			case XG_VIEW_NORMAL:
			default:
				nViewMode = XG_VIEW_NORMAL;
				break;
			case XG_VIEW_SKELETON:
				nViewMode = XG_VIEW_SKELETON;
				break;
			}
		}

		if (row_count < XG_MIN_SIZE || column_count < XG_MIN_SIZE) {
			throw 1;
		}
		if (row_count > XG_MAX_SIZE || column_count > XG_MAX_SIZE) {
			throw 1;
		}

		if (row_count != int(cell_data.size())) {
			throw 2;
		}

		MBoard board(column_count, row_count, rules);

		if (!cell_data.is_array()) {
			throw 3;
		}

		for (INT i = 0; i < row_count; ++i) {
			if (!cell_data[i].is_string()) {
				throw 4;
			}

			std::string str1 = cell_data[i];
			MStringW row = WideFromUtf8(str1.c_str()).c_str();
			if (INT(row.size()) != column_count) {
				throw 5;
			}
			for (INT j = 0; j < column_count; ++j) {
				board.SetCell(j, i, row[j]);
			}
		}

		std::vector<MBoardPos> vMarks;
		if (has_mark) {
			auto& marks = j0["marks"];
			for (auto& mark : marks) {
				INT i = INT(mark[0]) - 1;
				INT j = INT(mark[1]) - 1;
				if (i < 0 || row_count <= i) {
					throw 6;
				}
				if (j < 0 || column_count <= j) {
					throw 6;
				}
				vMarks.emplace_back(j, i);
			}
		}

		if (has_hints) {
			auto hints = j0["hints"];
			auto v = hints["v"];
			auto h = hints["h"];
			for (size_t i = 0; i < v.size(); ++i) {
				auto data = v[i];
				int number = INT(data[0]);
				if (number <= 0) {
					throw 7;
				}
				auto word = XgWideFromJson(data[1]);
				auto hint = XgWideFromJson(data[2]);
				word2clue[word] = hint;
			}
			for (size_t i = 0; i < h.size(); ++i) {
				auto data = h[i];
				int number = INT(data[0]);
				if (number <= 0) {
					throw 7;
				}
				auto word = XgWideFromJson(data[1]);
				auto hint = XgWideFromJson(data[2]);
				word2clue[word] = hint;
			}
		}

		auto header = XgWideFromJson(j0["header"]);
		auto notes = XgWideFromJson(j0["notes"]);
		std::vector<MPlaceInfo> vHorzInfo, vVertInfo;

		if (success) {
			display.m_bSolved = FALSE;
			if (is_solved) {
				// 番号付け。
				if (XgDoNumbering(board, vHorzInfo, vVertInfo)) {
					display.m_bSolved = !word2clue.empty();
				}
			}

			// 盤面を構築する。
			display.m_vHorzInfo = std::move(vHorzInfo);
			display.m_vVertInfo = std::move(vVertInfo);
			display.m_board = board;
			xg_nRules = board.Rules();
			display.m_solution = board;
			if (display.m_bSolved)
				display.m_board.ClearLetters();
			display.m_caret_pos = { 0, 0 };
			display.m_bCheckingAnswer = FALSE;
			display.m_header = std::move(header);
			display.m_notes = std::move(notes);
			display.m_word2clue = std::move(word2clue);
			display.m_vMarks = std::move(vMarks);
			display.m_nViewMode = nViewMode;
			display.m_mapNumCro1.clear();
			display.m_mapNumCro2.clear();
			display.m_bNumCroMode = !!is_numcro;
			if (display.m_bNumCroMode) {
				XgMakeItNumCro(display);
			}
		}
	} catch (json::exception&) {
		success = FALSE;
	} catch (const std::bad_alloc&) {
		success = FALSE;
	} catch (int) {
		success = FALSE;
	}

	return success;
}

// ルールを文字列にする。
static MStringW __fastcall XgGetRulesString(INT rules) {
	MStringW ret;
	BOOL bJapan = XgIsUserJapanese();
	if (rules & RULE_DONTDOUBLEBLACK) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"連黒禁" : L"Prohibit adjacent blocks";
	}
	if (rules & RULE_DONTCORNERBLACK) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"四隅黒禁" : L"Prohibit blocks at 4 corners";
	}
	if (rules & RULE_DONTTRIDIRECTIONS) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"三方黒禁" : L"Prohibit three-way blocks";
	}
	if (rules & RULE_DONTDIVIDE) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"分断禁" : L"No division by blocks";
	}
	if (rules & RULE_DONTTHREEDIAGONALS) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"黒斜三連禁" : L"Prohibit diagonal 3 blocks";
	} else if (rules & RULE_DONTFOURDIAGONALS) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"黒斜四連禁" : L"Prohibit diagonal 4 blocks";
	}
	if (rules & RULE_POINTSYMMETRY) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"黒マス点対称" : L"Point symmetry of blocks";
	}
	if (rules & RULE_LINESYMMETRYV) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"黒マス上下対称" : L"Vertical symmetry of blocks";
	}
	if (rules & RULE_LINESYMMETRYH) {
		if (ret.size())  {
			ret += L" / ";
		}
		ret += bJapan ? L"黒マス左右対称" : L"Horizontal symmetry of blocks";
	}
	ret += L" / ";
	return ret;
}

// XWJ (JSON)ファイル（クロスワード ギバー）を読み込む。
BOOL XgSaveXwjFile(MBoardDisplay& display, LPCWSTR filename) {
	MStringW str, strTable, strMarks;

	// ファイルを作成する。
	HANDLE hFile = ::CreateFileW(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
								 CREATE_ALWAYS, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return FALSE;
	MAutoCloseHandle auto_close(hFile);

	auto& board = display.m_bSolved ? display.m_solution : display.m_board;

	try {
		json j0;
		// 作成者情報。
		auto str0 = MStringW(XgIsUserJapanese() ? L"クロスワード ギバー ver." : L"XWordGiver ver.");
		str0 += XWORDGIVER_VERSION_W;
		j0["creator_info"] = Utf8FromWide(str0).c_str();
		// 行の数。
		j0["row_count"] = board.Rows();
		// 列の数。
		j0["column_count"] = board.Columns();
		// ルール。
		j0["rules"] = Utf8FromWide(XgGetRulesString(xg_nRules)).c_str();
		// ルール（整数値）。
		j0["policy"] = xg_nRules;
		// 辞書名。
		j0["dictionary"] = "";
		// ビューモード。
		j0["view_mode"] = (int)display.m_nViewMode;

		// 盤の切り替え。
		j0["is_solved"] = !!display.m_bSolved;

		// ナンクロモード。
		if (display.m_bNumCroMode) {
			j0["is_numcro"] = true;
			for (auto& pair : display.m_mapNumCro1) {
				auto ch = pair.first;
				auto number = pair.second;
				WCHAR szNum[64];
				StringCchPrintfW(szNum, _countof(szNum), L"%04d", number);
				const WCHAR szChar[2] = { ch, 0 };
				j0["num_cro"][Utf8FromWide(szNum).c_str()] = Utf8FromWide(szChar).c_str();
			}
		}

		// マス。
		for (INT i = 0; i < board.Rows(); ++i) {
			MStringW row;
			for (INT j = 0; j < board.Columns(); ++j) {
				row += board.GetCell(j, i);
			}
			j0["cell_data"].push_back(Utf8FromWide(row).c_str());
		}

		// 二重マス。
		auto& vMarks = display.m_vMarks;
		if (vMarks.size()) {
			j0["has_mark"] = true;

			MStringW mark_word;
			for (auto& mark : display.m_vMarks) {
				const WCHAR ch = board.GetCell(mark.m_j, mark.m_i);
				mark_word += ch;
			}
			j0["mark_word"] = Utf8FromWide(mark_word).c_str();

			str += L"\t\"marks\": [\r\n";
			for (size_t i = 0; i < vMarks.size(); ++i) {
				json mark;
				mark.push_back(vMarks[i].m_i + 1);
				mark.push_back(vMarks[i].m_j + 1);
				const WCHAR sz[2] = { mark_word[i] , 0 };
				mark.push_back(Utf8FromWide(sz).c_str());
				j0["marks"].push_back(mark);
			}
		} else {
			j0["has_mark"] = false;
		}

		// ヒント。
		auto& vHorzInfo = display.m_vHorzInfo;
		auto& vVertInfo = display.m_vVertInfo;
		auto& word2clue = display.m_word2clue;
		if (vHorzInfo.size() && vVertInfo.size() && word2clue.size()) {
			j0["has_hints"] = true;

			json hints;

			// タテのカギ。
			json v;
			for (auto& tate_hint : vVertInfo) {
				json hint;
				hint.push_back(tate_hint.m_number);
				hint.push_back(Utf8FromWide(tate_hint.m_word).c_str());
				hint.push_back(Utf8FromWide(word2clue[tate_hint.m_word]).c_str());
				v.push_back(hint);
			}
			hints["v"] = v;

			// ヨコのカギ。
			json h;
			for (auto& yoko_hint : vHorzInfo) {
				json hint;
				hint.push_back(yoko_hint.m_number);
				hint.push_back(Utf8FromWide(yoko_hint.m_word).c_str());
				hint.push_back(Utf8FromWide(word2clue[yoko_hint.m_word]).c_str());
				h.push_back(hint);
			}
			hints["h"] = h;

			j0["hints"] = hints;
		} else {
			j0["has_hints"] = false;
		}

		// ヘッダー。
		j0["header"] = Utf8FromWide(display.m_header).c_str();

		// 備考欄。
		j0["notes"] = Utf8FromWide(display.m_notes).c_str();

		// テーマ。
		j0["theme"] = "";

		// UTF-8文字列へ変換する。
		std::string utf8 = j0.dump(1, '\t');
		utf8 += '\n';

		std::string replaced;
		for (auto ch : utf8)
		{
			if (ch == '\n')
				replaced += '\r';
			replaced += ch;
		}
		utf8 = std::move(replaced);

		// ファイルに書き込んで、ファイルを閉じる。
		auto size = static_cast<DWORD>(utf8.size() * sizeof(CHAR));
		if (::WriteFile(hFile, utf8.data(), size, &size, nullptr)) {
			return TRUE;
		}
	}
	catch(...)
	{
		;
	}

	return FALSE;
}

#endif // def USE_JSON

// CRPファイル（Crossword Builder）を保存する。
BOOL XgSaveCrpFile(MBoardDisplay& display, LPCWSTR filename) {
	// ファイルを作成する。
	FILE *fout = _wfopen(filename, L"w");
	if (!fout)
		return FALSE;

	auto& board = display.m_bSolved ? display.m_solution : display.m_board;

	// 基本情報。
	const INT nColumns = board.Columns(), nRows = board.Rows();
	fprintf(fout,
		"[Version]\n"
		"Ver=0.3.0\n"
		"\n"
		"[Puzzle]\n"
		"Puzzle=0\n"
		"\n"
		"[Cross]\n"
		"Width=%d\n"
		"Height=%d\n", nColumns, nRows);

	// マス。
	for (INT i = 0; i < nRows; ++i) {
		MStringW row;
		for (INT j = 0; j < nColumns; ++j) {
			if (row.size())
				row += L",";
			const WCHAR ch = board.GetCell(j, i);
			if (ch == ZEN_SPACE)
				row += L'＿'; // U+FF3F
			else
				row += ch;
		}
		fprintf(fout, "Line%d=%s\n", i + 1, SjisFromWide(row).c_str());
	}

	// 二重マス。
	std::vector<std::vector<INT>> marks;
	marks.resize(nRows);
	for (auto& mark : marks) {
		mark.resize(nColumns);
	}

	MStringW answer;
	auto& vMarks = display.m_vMarks;
	if (vMarks.size()) {
		for (size_t i = 0; i < vMarks.size(); ++i) {
			answer += board.GetCell(vMarks[i].m_j, vMarks[i].m_i);
			marks[vMarks[i].m_i][vMarks[i].m_j] = INT(i) + 1;
		}
	}

	for (INT i = 0; i < nRows; ++i) {
		MStringA row;
		for (INT j = 0; j < nColumns; ++j) {
			if (row.size())
				row += ",";
			row += to_mstrA(marks[i][j]);
		}
		fprintf(fout, "MarkUpLine%d=%s\n", i + 1, row.c_str());
	}

	fprintf(fout,
		"\n"
		"[Property]\n"
		"Symmetry=None\n"
		"\n"
		"[Clue]\n");

	// ヒント。
	auto& vHorzInfo = display.m_vHorzInfo;
	auto& vVertInfo = display.m_vVertInfo;
	auto& word2clue = display.m_word2clue;
	if (vHorzInfo.size() && vVertInfo.size()) {
		fprintf(fout, "Count=%d\n", INT(vVertInfo.size() + vHorzInfo.size()));
		INT iHint = 1;
		// タテのカギ。
		for (auto& vert : vVertInfo) {
			fprintf(fout, "Clue%d=%s:%s\n", iHint++,
				SjisFromWide(vert.m_word).c_str(),
				SjisFromWide(word2clue[vert.m_word]).c_str());
		}
		// ヨコのカギ。
		for (auto& horz : vHorzInfo) {
			fprintf(fout, "Clue%d=%s:%s\n", iHint++,
				SjisFromWide(horz.m_word).c_str(),
				SjisFromWide(word2clue[horz.m_word]).c_str());
		}
	}

	fprintf(fout,
		"\n"
		"[Numbering]\n"
		"Hint=%s\n"
		"Answer=%s\n"
		"Theme=%s\n"
		"CantUseChar=%s\n",
		"",
		SjisFromWide(answer).c_str(),
		"",
		"");

	fclose(fout);
	return TRUE;
}

// クロスワードのファイルを読み込む。
BOOL XgLoadCrosswordFile(MBoardDisplay& display, LPCWSTR filename) {
	auto dotext = PathFindExtensionW(filename);
	if (lstrcmpiW(dotext, L".xd") == 0) {
		return XgLoadXDFile(display, filename);
	}
	if (lstrcmpiW(dotext, L".ecw") == 0) {
		return XgLoadEcwFile(display, filename);
	}
	if (lstrcmpiW(dotext, L".crp") == 0 || lstrcmpiW(dotext, L".crx") == 0) {
		return XgLoadCrpFile(display, filename);
	}
	if (lstrcmpiW(dotext, L".xwd") == 0) {
		return XgLoadXwdFile(display, filename);
	}
#ifdef USE_JSON
	if (lstrcmpiW(dotext, L".json") == 0 ||
		lstrcmpiW(dotext, L".jso") == 0 ||
		lstrcmpiW(dotext, L".xwj") == 0)
	{
		return XgLoadXwjFile(display, filename);
	}
#endif
	return FALSE;
}

// クロスワードのファイルを書き込む。
BOOL XgSaveCrosswordFile(MBoardDisplay& display, LPCWSTR filename) {
	auto dotext = PathFindExtensionW(filename);
	if (lstrcmpiW(dotext, L".xd") == 0) {
		return XgSaveXDFile(display, filename);
	}
	if (lstrcmpiW(dotext, L".crp") == 0) {
		return XgSaveCrpFile(display, filename);
	}
#ifdef USE_JSON
	if ((lstrcmpiW(dotext, L".json") == 0) ||
		(lstrcmpiW(dotext, L".jso") == 0) ||
		(lstrcmpiW(dotext, L".xwj") == 0))
	{
		return XgSaveXwjFile(display, filename);
	}
#endif
	return FALSE;
}

// 読み込める拡張子か？
BOOL XgCanDotExtLoad(LPCWSTR dotext) {
	return (lstrcmpiW(dotext, L".xd") == 0) ||
		   (lstrcmpiW(dotext, L".ecw") == 0) ||
		   (lstrcmpiW(dotext, L".crp") == 0) ||
		   (lstrcmpiW(dotext, L".crx") == 0) ||
#ifdef USE_JSON
		   (lstrcmpiW(dotext, L".json") == 0) ||
		   (lstrcmpiW(dotext, L".jso") == 0) ||
		   (lstrcmpiW(dotext, L".xwj") == 0) ||
#endif
		   (lstrcmpiW(dotext, L".xwd") == 0);
}

// 書き込める拡張子か？
BOOL XgCanDotExtSave(LPCWSTR dotext) {
	return (lstrcmpiW(dotext, L".xd") == 0) ||
#ifdef USE_JSON
		   (lstrcmpiW(dotext, L".json") == 0) ||
		   (lstrcmpiW(dotext, L".jso") == 0) ||
		   (lstrcmpiW(dotext, L".xwj") == 0) ||
#endif

		   (lstrcmpiW(dotext, L".crp") == 0);
}

// 最近使ったファイルのリストを更新。
void XgUpdateRecentlyUsed(MStringListW& mru_list, LPCWSTR text, size_t max_mru_items) {
	MStringListW new_list;
	for (auto& item : mru_list) {
		if (lstrcmpiW(item.c_str(), text) == 0)
			continue;
		new_list.push_back(item);
	}

	new_list.emplace(new_list.begin(), text);

	if (new_list.size() > max_mru_items)
		new_list.resize(max_mru_items);

	mru_list = std::move(new_list);
}
