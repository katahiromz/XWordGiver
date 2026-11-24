// MMainWnd_Input.cpp --- クロスワードギバーの入力。
// Author: katahiromz
// License: MIT

#include "MMainWnd.h"
#include <map>
#include <string>
#include <cstring>

// アクセント記号付きの文字にする。
WCHAR XgConvertAccent(WCHAR chAccent, WCHAR ch) noexcept {
	switch (chAccent) {
	case L'^':
		switch (ch) {
		case L'A': case L'a':
			ch = 0x00C2; // Â
			break;
		case L'I': case L'i':
			ch = 0x00CE; // Î
			break;
		case L'U': case L'u':
			ch = 0x00DB; // Û
			break;
		case L'E': case L'e':
			ch = 0x00CA; // Ê
			break;
		case L'O': case L'o':
			ch = 0x00D4; // Ô
			break;
		case L'Y': case L'y':
			ch = 0x0176; // LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
			break;
		case L'C': case L'c':
			ch = 0x0108; // LATIN CAPITAL LETTER C WITH CIRCUMFLEX
			break;
		case L'G': case L'g':
			ch = 0x011C; // LATIN CAPITAL LETTER G WITH CIRCUMFLEX
			break;
		case L'H': case L'h':
			ch = 0x0124; // LATIN CAPITAL LETTER H WITH CIRCUMFLEX
			break;
		case L'J': case L'j':
			ch = 0x0134; // LATIN CAPITAL LETTER J WITH CIRCUMFLEX
			break;
		case L'S': case L's':
			ch = 0x015C; // LATIN CAPITAL LETTER S WITH CIRCUMFLEX
			break;
		default:
			break;
		}
		break;
	case L'`':
		switch (ch) {
		case L'A': case L'a':
			ch = 0x00C0; // À
			break;
		case L'I': case L'i':
			ch = 0x00CC; // LATIN CAPITAL LETTER I WITH GRAVE
			break;
		case L'U': case L'u':
			ch = 0x00D9; // Ù
			break;
		case L'E': case L'e':
			ch = 0x00C8; // È
			break;
		case L'O': case L'o':
			ch = 0x00D2; // LATIN CAPITAL LETTER O WITH GRAVE
			break;
		case L'Y': case L'y':
			ch = 0x1EF2; // LATIN CAPITAL LETTER Y WITH GRAVE
			break;
		default:
			break;
		}
		break;
	case L':':
		switch (ch) {
		case L'A': case L'a':
			ch = 0x00C4; // Ä
			break;
		case L'I': case L'i':
			ch = 0x00CF; // Ï
			break;
		case L'U': case L'u':
			ch = 0x00DC; // Ü
			break;
		case L'E': case L'e':
			ch = 0x00CB; // Ë
			break;
		case L'O': case L'o':
			ch = 0x00D6; // Ö
			break;
		case L'Y': case L'y':
			ch = 0x0178; // Ÿ
			break;
		default:
			break;
		}
		break;
	case L'\'':
		switch (ch) {
		case L'A': case L'a':
			ch = 0x00C1; // LATIN CAPITAL LETTER A WITH ACUTE
			break;
		case L'I': case L'i':
			ch = 0x00CD; // LATIN CAPITAL LETTER I WITH ACUTE
			break;
		case L'U': case L'u':
			ch = 0x00DA; // LATIN CAPITAL LETTER U WITH ACUTE
			break;
		case L'E': case L'e':
			ch = 0x00C9; // É
			break;
		case L'O': case L'o':
			ch = 0x00D3; // LATIN CAPITAL LETTER O WITH ACUTE
			break;
		case L'Y': case L'y':
			ch = 0x00DD; // LATIN CAPITAL LETTER Y WITH ACUTE
			break;
		case L'G': case L'g':
			ch = 0x01F4; // LATIN CAPITAL LETTER G WITH ACUTE
			break;
		case L'N': case L'n':
			ch = 0x0143; // LATIN CAPITAL LETTER N WITH ACUTE
			break;
		case L'W': case L'w':
			ch = 0x1E82; // LATIN CAPITAL LETTER W WITH ACUTE
			break;
		default:
			break;
		}
		break;
	case L',':
		switch (ch) {
		case L'C': case L'c':
			ch = 0x00C7; // Ç: LATIN CAPITAL LETTER C WITH CEDILLA
			break;
		case L'D': case L'd':
			ch = 0x1E10; // LATIN CAPITAL LETTER D WITH CEDILLA
			break;
		case L'G': case L'g':
			ch = 0x0122; // LATIN CAPITAL LETTER G CEDILLA
			break;
		case L'K': case L'k':
			ch = 0x0136; // LATIN CAPITAL LETTER K CEDILLA
			break;
		case L'L': case L'l':
			ch = 0x013B; // LATIN CAPITAL LETTER L CEDILLA
			break;
		case L'N': case L'n':
			ch = 0x0145; // LATIN CAPITAL LETTER N CEDILLA
			break;
		case L'R': case L'r':
			ch = 0x0156; // LATIN CAPITAL LETTER R CEDILLA
			break;
		case L'S': case L's':
			ch = 0x015E; // LATIN CAPITAL LETTER S CEDILLA
			break;
		case L'T': case L't':
			ch = 0x0162; // LATIN CAPITAL LETTER T CEDILLA
			break;
		default:
			break;
		}
		break;
	case L'&':
		switch (ch) {
		case L'O': case L'o':
			ch = 0x0152; // Œ
			break;
		default:
			break;
		}
		break;
	case L'.':
		switch (ch) {
		case L'I': case L'i':
			ch = 0x0130; // İ
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return ch;
}

void __fastcall MMainWnd::SetChar(HWND hwnd, WCHAR ch) {
	::SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
	m_board.SetCell(m_caret_pos.m_j, m_caret_pos.m_i, ch);
	m_hCanvasWnd.EnsureCaretVisible();
	InvalidateImage();
	::SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
	CheckAnswer();
}

// 入力モードを切り替える。
void MMainWnd::SetInputMode(HWND hwnd, MInputMode imode, BOOL bForce) {
	const BOOL bChanged = (m_imode != imode);

	if (m_imode != xg_im_ANY || bForce)
		m_imode = imode;

	if (bChanged && m_hInputPalette)
		ReCreateInputPalette(hwnd, imode);

	UpdateStatusBar();
	InvalidateImage();
}

void __fastcall MMainWnd::InputDirection(HWND hwnd, INT nDirection) {
	switch (nDirection) {
	case 1:
		m_bVertInput = TRUE;
		break;
	case 0:
		m_bVertInput = FALSE;
		break;
	case -1:
		m_bVertInput = !m_bVertInput;
		break;
	default:
		break;
	}

	if (m_hInputPalette) {
		m_hInputPalette.UpdateUI(m_hInputPalette);
	}

	UpdateStatusBar();
	m_prev_vk = 0;
	m_chAccent = 0;
}

// 文字送り。
void __fastcall MMainWnd::SetCharFeed(HWND hwnd, INT nMode) {
	switch (nMode) {
	case 1:
		m_bCharFeed = TRUE;
		break;
	case 0:
		m_bCharFeed = FALSE;
		break;
	case -1:
		m_bCharFeed = !m_bCharFeed;
		break;
	default:
		assert(0);
		break;
	}

	if (m_hInputPalette) {
		ReCreateInputPalette(hwnd, m_imode);
	}

	m_prev_vk = 0;
	m_chAccent = 0;
}

// 字送りを実行する。
void __fastcall MMainWnd::CharFeed(HWND hwnd) {
	if (!m_bCharFeed)
		return;

	if (m_bVertInput) {
		++m_caret_pos.m_i;
		if (m_caret_pos.m_i >= m_board.Rows()) {
			m_caret_pos.m_i = 0;
			++m_caret_pos.m_j;
			if (m_caret_pos.m_j >= m_board.Columns()) {
				m_caret_pos.m_j = 0;
			}
		}
	} else {
		++m_caret_pos.m_j;
		if (m_caret_pos.m_j >= m_board.Columns()) {
			m_caret_pos.m_j = 0;
			++m_caret_pos.m_i;
			if (m_caret_pos.m_i >= m_board.Rows()) {
				m_caret_pos.m_i = 0;
			}
		}
	}

	UpdateStatusBar();
	m_hCanvasWnd.EnsureCaretVisible();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
	m_chAccent = 0;
}

// ID_MOSTLEFT
void MMainWnd::OnMostLeft(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_j > 0) {
		m_caret_pos.m_j = 0;
	}
	SendMessageW(m_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_MOSTRIGHT
void MMainWnd::OnMostRight(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_j + 1 < m_board.Columns()) {
		m_caret_pos.m_j = m_board.Columns() - 1;
	}
	SendMessageW(m_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_RIGHT, 0), 0);
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_MOSTUPPER
void MMainWnd::OnMostUpper(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_i > 0) {
		m_caret_pos.m_i = 0;
	}
	SendMessageW(m_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_MOSTLOWER
void MMainWnd::OnMostLower(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_i + 1 < m_board.Rows()) {
		m_caret_pos.m_i = m_board.Rows() - 1;
	}
	SendMessageW(m_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_LEFT
void MMainWnd::OnLeft(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_j > 0) {
		m_caret_pos.m_j--;
		m_hCanvasWnd.EnsureCaretVisible();
	} else {
		// 一番左のキャレットなら、左端へ移動。
		SendMessageW(m_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_LEFT, 0), 0);
	}
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_RIGHT
void MMainWnd::OnRight(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_j + 1 < m_board.Columns()) {
		m_caret_pos.m_j++;
		m_hCanvasWnd.EnsureCaretVisible();
	} else {
		// 一番右のキャレットなら、右端へ移動。
		SendMessageW(m_hCanvasWnd, WM_HSCROLL, MAKEWPARAM(SB_RIGHT, 0), 0);
	}
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_UP
void MMainWnd::OnUp(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_i > 0) {
		m_caret_pos.m_i--;
		m_hCanvasWnd.EnsureCaretVisible();
	} else {
		// 一番上のキャレットなら、上端へ移動。
		SendMessageW(m_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_TOP, 0), 0);
	}
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_DOWN
void MMainWnd::OnDown(HWND hwnd) {
	m_hCanvasWnd.InvalidateCell();
	if (m_caret_pos.m_i + 1 < m_board.Rows()) {
		m_caret_pos.m_i++;
		m_hCanvasWnd.EnsureCaretVisible();
	} else {
		// 一番下のキャレットなら、下端へ移動。
		SendMessageW(m_hCanvasWnd, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
	}
	m_hCanvasWnd.InvalidateCell();
	m_hCanvasWnd.UpdateCaretPos();
	m_prev_vk = 0;
}

// ID_BACK
void MMainWnd::OnBack(HWND hwnd) {
	if (!m_bCharFeed) {
		OnChar(hwnd, L'_', 1);
		return;
	}

	if (m_bVertInput) {
		if (m_caret_pos.m_i == 0) {
			if (m_caret_pos.m_j == 0) {
				m_caret_pos.m_i = m_board.Rows() - 1;
				m_caret_pos.m_j = m_board.Columns() - 1;
			} else {
				m_caret_pos.m_i = m_board.Rows() - 1;
				--m_caret_pos.m_j;
			}
		} else {
			--m_caret_pos.m_i;
		}
	} else {
		if (m_caret_pos.m_j == 0) {
			if (m_caret_pos.m_i == 0) {
				m_caret_pos.m_j = m_board.Columns() - 1;
				m_caret_pos.m_i = m_board.Rows() - 1;
			} else {
				m_caret_pos.m_j = m_board.Columns() - 1;
				--m_caret_pos.m_i;
			}
		} else {
			--m_caret_pos.m_j;
		}
	}

	m_bCharFeed = FALSE;
	OnChar(hwnd, L'_', 1);
	m_bCharFeed = TRUE;

	m_hCanvasWnd.EnsureCaretVisible();
	m_hCanvasWnd.UpdateCaretPos();
	UpdateStatusBar();
	m_prev_vk = 0;
	m_chAccent = 0;
}

// ID_RETURN
void MMainWnd::OnReturn(HWND hwnd) {
	if (m_bVertInput) {
		++m_caret_pos.m_j;
		if (m_caret_pos.m_j >= m_board.Columns()) {
			m_caret_pos.m_j = 0;
		}
		m_caret_pos.m_i = 0;
	} else {
		++m_caret_pos.m_i;
		if (m_caret_pos.m_i >= m_board.Rows()) {
			m_caret_pos.m_i = 0;
		}
		m_caret_pos.m_j = 0;
	}

	m_hCanvasWnd.EnsureCaretVisible();
	m_hCanvasWnd.UpdateCaretPos();
	InvalidateImage();

	m_prev_vk = 0;
	m_chAccent = 0;
}

//////////////////////////////////////////////////////////////////////////////
// ローマ字 → カナ変換 (クロスワード用: 小さな文字は大きな文字にする)
//
// 変換結果は「大きな文字」だけで表す。例:
//   KYA → キヤ,  KKA → ツカ,  NKA → ンカ,  XTU → ツ
// 1打鍵で複数のマスに入る場合があるので、結果は文字列で返す。

// 子音 + 母音(AIUEO) の標準的な行。
struct XG_ROMAJI_ROW {
	const char *cons;   // 子音部分 (例: "K")
	const WCHAR *kana;  // ア段〜オ段の5文字
};

// 拗音の行: 子音部分 + (A,I,U,E,O) → 先頭カナ + (ヤ,イ,ユ,エ,ヨ)
struct XG_ROMAJI_YOON {
	const char *cons;   // 子音部分 (例: "KY", "SH")
	WCHAR head;         // 先頭のカナ (例: キ)
};

static const std::map<MStringA, MStringW>& XgGetRomajiTable() {
	static std::map<MStringA, MStringW> s_table;
	if (!s_table.empty())
		return s_table;
	auto& t = s_table;

	static const char vowels[] = "AIUEO";
	static const WCHAR small_tail[] = L"ヤイユエヨ"; // 拗音の後半 (A,I,U,E,O に対応)
	static const WCHAR plain_tail[] = L"アイウエオ"; // 母音 (A,I,U,E,O に対応)

	// (1) 基本の行。LとXは小さな母音 (ァィゥェォ) なので大きな文字にする。
	static const XG_ROMAJI_ROW rows[] = {
		{ "",  L"アイウエオ" }, { "L", L"アイウエオ" }, { "X", L"アイウエオ" },
		{ "K", L"カキクケコ" }, { "S", L"サシスセソ" }, { "T", L"タチツテト" },
		{ "N", L"ナニヌネノ" }, { "H", L"ハヒフヘホ" }, { "M", L"マミムメモ" },
		{ "R", L"ラリルレロ" }, { "G", L"ガギグゲゴ" }, { "Z", L"ザジズゼゾ" },
		{ "D", L"ダヂヅデド" }, { "B", L"バビブベボ" }, { "P", L"パピプペポ" },
	};
	for (auto& row : rows) {
		for (int i = 0; i < 5; ++i)
			t[MStringA(row.cons) + vowels[i]] = MStringW(1, row.kana[i]);
	}

	// (2) 拗音: キャ → キヤ, シェ → シエ など。
	static const XG_ROMAJI_YOON yoons[] = {
		{ "KY", L'キ' }, { "GY", L'ギ' }, { "SY", L'シ' }, { "SH", L'シ' },
		{ "ZY", L'ジ' }, { "JY", L'ジ' }, { "J",  L'ジ' }, { "TY", L'チ' },
		{ "CY", L'チ' }, { "CH", L'チ' }, { "DY", L'ヂ' }, { "NY", L'ニ' },
		{ "HY", L'ヒ' }, { "BY", L'ビ' }, { "PY", L'ピ' }, { "MY", L'ミ' },
		{ "RY", L'リ' }, { "FY", L'フ' }, { "TH", L'テ' }, { "DH", L'デ' },
	};
	for (auto& yoon : yoons) {
		for (int i = 0; i < 5; ++i)
			t[MStringA(yoon.cons) + vowels[i]] = MStringW(1, yoon.head) + small_tail[i];
	}
	// 拗音のうち、1文字になるもの。
	t["SHI"] = L"シ"; t["CHI"] = L"チ"; t["JI"] = L"ジ";

	// (3) 外来音: ファ → フア, ウィ → ウイ, トゥ → トウ など。
	static const XG_ROMAJI_YOON foreigns[] = {
		{ "F", L'フ' }, { "Q", L'ク' }, { "KW", L'ク' }, { "GW", L'グ' },
		{ "TS", L'ツ' }, { "TW", L'ト' }, { "DW", L'ド' }, { "SW", L'ス' },
		{ "WH", L'ウ' },
	};
	for (auto& f : foreigns) {
		for (int i = 0; i < 5; ++i)
			t[MStringA(f.cons) + vowels[i]] = MStringW(1, f.head) + plain_tail[i];
	}
	t["FU"] = L"フ"; t["TSU"] = L"ツ"; t["WHU"] = L"ウ";

	// (4) ヤ行・ワ行。
	t["YA"] = L"ヤ"; t["YI"] = L"イ"; t["YU"] = L"ユ"; t["YE"] = L"イエ"; t["YO"] = L"ヨ";
	t["WA"] = L"ワ"; t["WI"] = L"ウイ"; t["WU"] = L"ウ"; t["WE"] = L"ウエ"; t["WO"] = L"ヲ";
	t["WYI"] = L"ヰ"; t["WYE"] = L"ヱ";

	// (5) C を使う綴り。
	t["CA"] = L"カ"; t["CI"] = L"シ"; t["CU"] = L"ク"; t["CE"] = L"セ"; t["CO"] = L"コ";

	// (6) 小さな文字 (L/X + ...)。結果は大きな文字。
	for (const char *pre : { "L", "X" }) {
		const MStringA p = pre;
		t[p + "YA"] = L"ヤ"; t[p + "YU"] = L"ユ"; t[p + "YO"] = L"ヨ";
		t[p + "WA"] = L"ワ"; t[p + "TU"] = L"ツ"; t[p + "TSU"] = L"ツ";
		t[p + "KA"] = L"カ"; t[p + "KE"] = L"ケ";
	}

	// (7) 撥音「ン」。N', XN。
	// ※ NN、および N + 子音 (NK など) は変換関数側で処理する。
	t["N'"] = L"ン"; t["XN"] = L"ン";

	return s_table;
}

// buf が、表のどれかのキーの先頭部分 (まだ入力途中) か？
static bool XgIsRomajiPrefix(const MStringA& buf) {
	auto& t = XgGetRomajiTable();
	auto it = t.lower_bound(buf);
	return it != t.end() && it->first.compare(0, buf.size(), buf) == 0;
}

// ローマ字バッファ buf に 1文字(大文字A-Zまたは ')を追加し、確定したカナを返す。
// 未確定の入力は buf に残る。
static MStringW XgRomajiFeed(MStringA& buf, char ch) {
	auto& t = XgGetRomajiTable();
	MStringW out;
	buf += ch;

	while (!buf.empty()) {
		// (0) NN で「ン」を出した直後の状態 (バッファ先頭が小文字の 'n')。
		//     次が母音かYなら、2つ目のNを使う (NNA → ンナ)。
		//     そうでなければ 2つ目のNは使い切ったものとして捨てる (NNK → ンカ, NNN → ン+N)。
		if (buf[0] == 'n') {
			if (buf.size() == 1)
				break;
			if (std::strchr("AIUEOY", buf[1]))
				buf[0] = 'N';
			else
				buf.erase(0, 1);
			continue;
		}
		// (0') NN → 「ン」。2つ目のNは、次の入力で使うかもしれないので 'n' として残す。
		if (buf == "NN") {
			out += L'ン';
			buf = "n";
			break;
		}
		// (a) 完成したか？
		auto it = t.find(buf);
		if (it != t.end()) {
			out += it->second;
			buf.clear();
			break;
		}
		// (b) 促音「ッ」(大きな文字では「ツ」)。KK, TT, SS... と TC(H)。
		//     先頭の子音を「ツ」にして、2文字目以降を続ける。
		if (buf.size() >= 2 &&
			((buf[0] == buf[1] && std::strchr("KSTCHFGZJDBPQR", buf[0])) || buf == "TC"))
		{
			out += L'ツ';
			buf.erase(0, 1);
			continue;
		}
		// (c) まだ入力途中なら待つ。
		if (XgIsRomajiPrefix(buf))
			break;
		// (d) 「N + 子音」「M + B/P/M」は、先頭を撥音「ン」にして続ける。
		//     (例: NKA → ンカ, SHIMBUN → シンブン)
		if (buf.size() >= 2 &&
			((buf[0] == 'N' && !std::strchr("AIUEOY", buf[1])) ||
			 (buf[0] == 'M' && std::strchr("BPM", buf[1]))))
		{
			out += L'ン';
			buf.erase(0, 1);
			continue;
		}
		// (e) どれにも当てはまらない。先頭を捨てて再判定する。
		buf.erase(0, 1);
	}
	return out;
}

// ローマ字を1文字入力し、確定したカナ(大きな文字)を返す。
MStringW MMainWnd::FeedRomaji(WCHAR ch) {
	// 他の操作(カーソル移動など)で m_prev_vk が 0 にされたら、入力途中のローマ字は破棄する。
	if (!m_prev_vk)
		m_romaji.clear();

	MStringW kana = XgRomajiFeed(m_romaji, static_cast<char>(ch)).c_str();

	m_prev_vk = m_romaji.empty() ? 0 : static_cast<UINT>(static_cast<unsigned char>(m_romaji[0]));
	m_chAccent = 0;
	return kana;
}

// WM_CHAR
void MMainWnd::OnChar(HWND hwnd, TCHAR ch, int cRepeat) {
	const WCHAR oldch = m_board.GetCell(m_caret_pos);
	if (oldch == ZEN_BLACK && m_display.m_bSolved)
		return;

	if (ch == L'_') {
		// 候補ウィンドウを破棄する。
		DestroyCandsWnd();
		if (!(m_bSolved && oldch == ZEN_BLACK)) {
			SetChar(hwnd, ZEN_SPACE);
			m_hCanvasWnd.EnsureCaretVisible();
		}
		m_hCanvasWnd.UpdateCaretPos();
		m_prev_vk = 0;
		m_chAccent = 0;
	} else if (ch == L' ') {
		// 候補ウィンドウを破棄する。
		DestroyCandsWnd();
		{
			if (oldch == ZEN_SPACE && !m_bSolved) {
				SetChar(hwnd, ZEN_BLACK);
				m_hCanvasWnd.EnsureCaretVisible();
			} else if (oldch == ZEN_BLACK && !m_bSolved) {
				SetChar(hwnd, ZEN_SPACE);
				m_hCanvasWnd.EnsureCaretVisible();
			} else if (!m_bSolved || !m_bShowAnswer) {
				if (oldch != ZEN_BLACK) {
					SetChar(hwnd, ZEN_SPACE);
					m_hCanvasWnd.EnsureCaretVisible();
				}
			}
		}
		m_hCanvasWnd.UpdateCaretPos();
		m_prev_vk = 0;
		m_chAccent = 0;
	} else if (ch == L'#' || ch == ZEN_SHARP1 || ch == ZEN_SHARP2 || ch == ZEN_BLACK) {
		// 候補ウィンドウを破棄する。
		DestroyCandsWnd();

		if (!m_bSolved) {
			SetChar(hwnd, ZEN_BLACK);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		}
	} else if (m_imode == xg_im_ABC) {
		// 英字入力の場合。
		if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			// 英字小文字を大文字に変換。
			WCHAR sz[2] = { ch, 0 };
			::CharUpperW(sz);
			ch = sz[0];

			if (m_chAccent) { // アクセントがあるか？
				// アクセント記号付きの文字にする。
				ch = XgConvertAccent(m_chAccent, ch);
			} else {
				// 半角英字を全角英字に変換。
				ch = WCHAR(L'Ａ' + (ch - L'A'));
			}
			m_chAccent = 0; // アクセントを解除する。

			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		} else if (XgIsCharZenkakuUpperW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		} else if (XgIsCharZenkakuLowerW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();
			// 全角小文字英字が入力された。
			ch = L'Ａ' + (ch - L'ａ');

			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		} else if ((0x0080 <= ch && ch <= 0x00FF) ||  // ラテン補助
				   (0x0100 <= ch && ch <= 0x017F) || // ラテン文字拡張A
				   (0x1E00 <= ch && ch <= 0x1EFF)) // ラテン文字拡張追加
		{
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			WCHAR sz[2];
			LCMapStringW(XG_JPN_LOCALE,
				LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
				&ch, 1, sz, _countof(sz));
			CharUpperW(sz);
			ch = sz[0];

			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		}
	} else if (m_imode == xg_im_KANA) {
		// カナ入力の場合。
		MStringW kana; // 確定したカナ。拗音・促音・撥音で複数文字になることがある。
		WCHAR newch = 0;
		if (::GetAsyncKeyState(VK_CONTROL) < 0) {
			// [Ctrl]キーが押されている。
		} else if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch) || ch == L'\'') {
			// ローマ字入力。
			ch = static_cast<WCHAR>(reinterpret_cast<UINT_PTR>(CharUpperW(reinterpret_cast<LPWSTR>(static_cast<UINT_PTR>(ch)))));
			kana = FeedRomaji(ch);
		} else if (XgIsCharHiraganaW(ch)) {
			// ひらがな直接入力。
			WCHAR sz[2];
			::LCMapStringW(XG_JPN_LOCALE,
				LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
				&ch, 1, sz, _countof(sz));
			newch = sz[0];
			goto katakana;
		} else if (XgIsCharKatakanaW(ch)) {
katakana:;
			// カタカナ直接入力。小さな字は大きな字にする。
			MStringW str = { ch };
			str = XgNormalizeString(str);
			newch = str[0];
			m_prev_vk = 0;
			m_chAccent = 0;
		} else if (ch == '-' || ch == ZEN_PROLONG) {
			newch = ZEN_PROLONG;
			m_prev_vk = 0;
			m_chAccent = 0;
		} else {
			m_prev_vk = 0;
			m_chAccent = 0;
		}

		if (newch)
			kana = MStringW(1, newch);

		if (!kana.empty()) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			// 1文字ずつマスに入れる。複数文字のときは、字送りがOFFでも次のマスへ進む。
			for (size_t i = 0; i < kana.size(); ++i) {
				if (i > 0 && m_bSolved && m_board.GetCell(m_caret_pos) == ZEN_BLACK)
					break;
				SetChar(hwnd, kana[i]);
				if (i + 1 < kana.size()) {
					const BOOL bOldCharFeed = m_bCharFeed;
					m_bCharFeed = TRUE;
					CharFeed(hwnd);
					m_bCharFeed = bOldCharFeed;
				} else if (m_bCharFeed) {
					CharFeed(hwnd);
				}
			}
			m_hCanvasWnd.EnsureCaretVisible();

			// CharFeed が m_prev_vk を 0 にするので、入力途中のローマ字の状態を戻す。
			m_prev_vk = m_romaji.empty() ? 0 : static_cast<UINT>(static_cast<unsigned char>(m_romaji[0]));
		}
	} else if (m_imode == xg_im_RUSSIA) {
		// ロシア入力の場合。
		if (XgIsCharZenkakuCyrillicW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			// キリル文字直接入力。
			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		}
	} else if (m_imode == xg_im_GREEK) {
		if (XgIsCharGreekW(ch)) {
			// 大文字にする。
			CharUpperBuffW(&ch, 1);

			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		}
	} else if (m_imode == xg_im_DIGITS) {
		// 数字入力の場合。
		if (XgIsCharHankakuNumericW(ch))
			ch += 0xFF10 - L'0'; // 全角数字にする。
		if (XgIsCharZenkakuNumericW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			// 数字直接入力。
			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		}
	} else if (m_imode == xg_im_ANY) {
		// 自由入力の場合。
		WCHAR sz[2] = { ch, 0 };
		::CharUpperW(sz);
		ch = sz[0];

		if (m_chAccent) { // アクセントがあるか？
			// アクセント記号付きの文字にする。
			ch = XgConvertAccent(m_chAccent, ch);
		}
		m_chAccent = 0; // アクセントを解除する。

		sz[0] = ch;
		auto str = XgNormalizeString(sz);
		ch = str[0];

		if (ch >= 0x0020 && ch != 0x007F) { // 制御文字はダメだ。
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			SetChar(hwnd, ch);

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);

			m_hCanvasWnd.EnsureCaretVisible();
		}
	}
}

// WM_KEYDOWN, WM_KEYUP
void MMainWnd::OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) {
	// 特定の条件において、キー入力を拒否する。
	if (!fDown)
		return;

	// 仮想キーコードに応じて、処理する。
	switch (vk) {
	case VK_APPS:   // アプリ キー。
		// アプリメニューを表示する。
		m_hCanvasWnd.ShowContextMenu(m_hCanvasWnd, { -1, -1 });
		m_prev_vk = 0;
		m_chAccent = 0;
		break;

	case VK_PRIOR:  // PgUpキー。
		::SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
		m_prev_vk = 0;
		m_chAccent = 0;
		break;

	case VK_NEXT:   // PgDnキー。
		::SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
		m_prev_vk = 0;
		m_chAccent = 0;
		break;

	case VK_DELETE:
		// 候補ウィンドウを破棄する。
		DestroyCandsWnd();
		// 現在のキャレット位置のマスの中身を消去する。
		if (m_board.GetCell(m_caret_pos) != ZEN_SPACE && !m_bSolved) {
			SetChar(hwnd, ZEN_SPACE);
			m_hCanvasWnd.EnsureCaretVisible();
		}
		m_prev_vk = 0;
		m_chAccent = 0;
		break;
	
	case VK_BACK:
		OnBack(hwnd);
		break;

	case VK_RETURN:
		OnReturn(hwnd);
		break;

	default:
		if (::GetKeyState(VK_CONTROL) < 0 && ::GetKeyState(VK_SHIFT) >= 0) {
			// [Ctrl] キーが押されている。
			WCHAR sz1[2], sz2[2];
			BYTE state[256] = { 0 };
			// 仮想キーコードを文字に変換。
			::ToUnicode(vk, 0, state, sz1, _countof(sz1), 0); // [Shift]なし。
			state[VK_SHIFT] = state[VK_LSHIFT] = state[VK_RSHIFT] = 0x80;
			::ToUnicode(vk, 0, state, sz2, _countof(sz2), 0); // [Shift]あり。
			const WCHAR ch1 = sz1[0], ch2 = sz2[0];
			switch (ch1) {
			case L'/':
				::PostMessageW(hwnd, WM_COMMAND, ID_INPUTHV, 0);
				return;
			case L'.':
				::PostMessageW(hwnd, WM_COMMAND, ID_TOGGLEMARK, 0);
				return;
			case L']':
				::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMIN, 0);
				return;
			case L'[':
				::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMOUT, 0);
				return;
			case L'\\':
				::PostMessageW(hwnd, WM_COMMAND, ID_ZOOM100, 0);
				return;
			case L'^':
				::PostMessageW(hwnd, WM_COMMAND, ID_CHARFEED, 0);
				return;
			case L':':
				::PostMessageW(hwnd, WM_COMMAND, ID_GENERATEFROMWORDLIST, 0);
				return;
			case L';':
				::PostMessageW(hwnd, WM_COMMAND, ID_OPENPATTERNS, 0);
				return;
			default:
				break;
			}
			switch (ch2) {
			case L'/':
				::PostMessageW(hwnd, WM_COMMAND, ID_INPUTHV, 0);
				return;
			case L'.':
				::PostMessageW(hwnd, WM_COMMAND, ID_TOGGLEMARK, 0);
				return;
			case L']':
				::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMIN, 0);
				return;
			case L'[':
				::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMOUT, 0);
				return;
			case L'\\':
				::PostMessageW(hwnd, WM_COMMAND, ID_ZOOM100, 0);
				return;
			case L'^':
				::PostMessageW(hwnd, WM_COMMAND, ID_CHARFEED, 0);
				return;
			case L':':
				::PostMessageW(hwnd, WM_COMMAND, ID_GENERATEFROMWORDLIST, 0);
				return;
			case L';':
				::PostMessageW(hwnd, WM_COMMAND, ID_OPENPATTERNS, 0);
				return;
			default:
				break;
			}
			return;
		}
		if (::GetKeyState(VK_CONTROL) < 0 && ::GetKeyState(VK_SHIFT) < 0) {
			// [Shift]キーと[Ctrl]キーが押されている。
			if (vk == 'G') {
				OnChar(hwnd, 0x011E, 1); // Ğ
				return;
			}
			WCHAR sz1[2], sz2[2];
			BYTE state[256] = { 0 };
			// 仮想キーコードを文字に変換。
			::ToUnicode(vk, 0, state, sz1, _countof(sz1), 0); // [Shift]なしの場合。
			state[VK_SHIFT] = state[VK_LSHIFT] = state[VK_RSHIFT] = 0x80;
			::ToUnicode(vk, 0, state, sz2, _countof(sz2), 0); // [Shift]ありの場合。
			// アクセント記号か？
			const WCHAR ch1 = sz1[0], ch2 = sz2[0];
			if (wcschr(L"^`':,&.", ch1) != nullptr) {
				m_chAccent = ch1;
			} else if (wcschr(L"^`':,&.", ch2) != nullptr) {
				m_chAccent = ch2;
			} else {
				m_chAccent = 0;
			}
		}
		break;
	}
}

// ID_BLOCK
void MMainWnd::OnBlock(HWND hwnd) {
	const BOOL bOldFeed = m_bCharFeed;
	m_bCharFeed = FALSE;
	SendMessageW(hwnd, WM_CHAR, L'#', 0);
	m_bCharFeed = bOldFeed;
	InvalidateImage();
}

// ID_SPACE
void MMainWnd::OnSpace(HWND hwnd) {
	const BOOL bOldFeed = m_bCharFeed;
	m_bCharFeed = FALSE;
	SendMessageW(hwnd, WM_CHAR, L'_', 0);
	m_bCharFeed = bOldFeed;
	InvalidateImage();
}

// WM_IME_CHAR
void MMainWnd::OnImeChar(HWND hwnd, WCHAR ch, LPARAM lParam) {
	if (ch == L'#' || ch == ZEN_BLACK || ch == ZEN_SHARP1 || ch == ZEN_SHARP2) {
		SendMessageW(hwnd, WM_CHAR, L'#', 1);
		return;
	}

	if (ch == L' ' || ch == ZEN_SPACE) {
		SendMessageW(hwnd, WM_CHAR, L' ', 1);
		return;
	}

	if (ch == L'_') {
		SendMessageW(hwnd, WM_CHAR, L'_', 1);
		return;
	}

	// 解があるとき、黒は上書きできない。
	const WCHAR oldch = m_board.GetCell(m_caret_pos);
	if (m_bSolved && oldch == ZEN_BLACK) {
		return;
	}

	if (m_imode == xg_im_RUSSIA) {
		// ロシア入力モードの場合。
		// キリル文字か？
		if (XgIsCharZenkakuCyrillicW(ch)) {
			CharUpperBuffW(&ch, 1);

			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	} else if (m_imode == xg_im_KANJI) {
		// 漢字入力モードの場合。
		// 漢字か？
		if (XgIsCharKanjiW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();
			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	} else if (m_imode == xg_im_KANA) {
		// カナ入力モードの場合。
		if (XgIsCharHiraganaW(ch)) {
			// ひらがな入力。
			WCHAR sz[2];
			LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
				         &ch, 1, sz, _countof(sz));
			ch = sz[0];
			goto katakana;
		} else if (XgIsCharKatakanaW(ch)) {
katakana:;
			// カタカナ入力。
			// 小さな字を大きな字にする。
			MStringW str = { ch };
			str = XgNormalizeString(str);
			ch = str[0];

			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();

			// 文字を設定する。
			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	} else if (m_imode == xg_im_ABC) {
		if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch) ||
			XgIsCharZenkakuUpperW(ch) || XgIsCharZenkakuLowerW(ch) ||
			(0x0080 <= ch && ch <= 0x00FF) || // ラテン補助
			(0x0100 <= ch && ch <= 0x017F) || // ラテン文字拡張A
			(0x1E00 <= ch && ch <= 0x1EFF)) // ラテン文字拡張追加
		{
			WCHAR sz[2];
			LCMapStringW(XG_JPN_LOCALE,
				LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
				&ch, 1, sz, _countof(sz));
			CharUpperW(sz);
			ch = sz[0];

			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();
			// 文字を設定する。
			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	} else if (m_imode == xg_im_GREEK) {
		if (XgIsCharGreekW(ch)) {
			// 大文字にする。
			CharUpperBuffW(&ch, 1);
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();
			// 文字を設定する。
			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	} else if (m_imode == xg_im_DIGITS) {
		// 数字入力の場合。
		if (XgIsCharHankakuNumericW(ch)) {
			ch += 0xFF10 - L'0'; // 全角数字にする。
		}
		if (XgIsCharZenkakuNumericW(ch)) {
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();
			// 文字を設定する。
			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	} else if (m_imode == xg_im_ANY) {
		// 自由入力の場合。
		WCHAR sz[2] = { ch, 0 };
		::CharUpperW(sz);
		ch = sz[0];

		if (m_chAccent) { // アクセントがあるか？
			// アクセント記号付きの文字にする。
			ch = XgConvertAccent(m_chAccent, ch);
		}
		m_chAccent = 0; // アクセントを解除する。

		sz[0] = ch;
		auto str = XgNormalizeString(sz);
		ch = str[0];

		if (ch >= 0x0020 && ch != 0x007F) { // 制御文字はダメだ。
			// 候補ウィンドウを破棄する。
			DestroyCandsWnd();
			// 文字を設定する。
			SetChar(hwnd, ch);
			m_hCanvasWnd.EnsureCaretVisible();

			// 文字送り。
			if (m_bCharFeed)
				CharFeed(hwnd);
		}
	}
}

void MMainWnd::DestroyInputPalette() {
	if (m_hInputPalette)
		::DestroyWindow(m_hInputPalette);
}

BOOL MMainWnd::ReCreateInputPalette(HWND hwnd, MInputMode imode, BOOL bForceKana) {
	DestroyInputPalette();

	if (bForceKana || imode == xg_im_ANY)
		imode = xg_im_KANA;

	switch (imode) {
	case xg_im_ABC:
		if (m_display.m_bLowercase) {
			m_hInputPalette.CreateDialogDx(hwnd, IDD_ABCLOWER);
		} else {
			m_hInputPalette.CreateDialogDx(hwnd, IDD_ABC);
		}
		break;
	case xg_im_KANA:
		if (m_bVerticalLayout) {
			if (m_display.m_bHiragana) {
				m_hInputPalette.CreateDialogDx(hwnd, IDD_HIRATATE);
			} else {
				m_hInputPalette.CreateDialogDx(hwnd, IDD_KATATATE);
			}
		} else {
			if (m_display.m_bHiragana) {
				m_hInputPalette.CreateDialogDx(hwnd, IDD_HIRAYOKO);
			} else {
				m_hInputPalette.CreateDialogDx(hwnd, IDD_KATAYOKO);
			}
		}
		break;
	case xg_im_RUSSIA:
		if (m_display.m_bLowercase) {
			m_hInputPalette.CreateDialogDx(hwnd, IDD_RUSSIALOWER);
		} else {
			m_hInputPalette.CreateDialogDx(hwnd, IDD_RUSSIA);
		}
		break;
	case xg_im_GREEK:
		// TODO: ギリシャ文字入力パレットを追加せよ。
		break;
	case xg_im_DIGITS:
		// TODO: 数字入力パレットを追加せよ。
		break;
	default:
		assert(0);
		return FALSE;
	}

	if (!m_hInputPalette) {
		assert(0);
		return FALSE;
	}

	MRect rc;
	GetWindowRect(m_hInputPalette, &rc);
	if (MInputPalette::s_nWindowX != CW_USEDEFAULT) {
		MoveWindow(m_hInputPalette, MInputPalette::s_nWindowX, MInputPalette::s_nWindowY,
				   rc.Width(), rc.Height(), TRUE);
		SendMessageW(m_hInputPalette, DM_REPOSITION, 0, 0);
	}

	ShowWindow(m_hInputPalette, SW_SHOWNOACTIVATE);
	UpdateWindow(m_hInputPalette);

	return !!m_hInputPalette;
}

BOOL MMainWnd::OnCommandExtra(HWND hwnd, INT id) {
	BOOL bOK = FALSE;

	const BOOL bOldFeed = m_bCharFeed;
	m_bCharFeed = FALSE;

	switch (id) {
	// kana
	case 10000: OnImeChar(hwnd, L'ア', 0); bOK = TRUE; break;
	case 10001: OnImeChar(hwnd, L'イ', 0); bOK = TRUE; break;
	case 10002: OnImeChar(hwnd, L'ウ', 0); bOK = TRUE; break;
	case 10003: OnImeChar(hwnd, L'エ', 0); bOK = TRUE; break;
	case 10004: OnImeChar(hwnd, L'オ', 0); bOK = TRUE; break;
	case 10010: OnImeChar(hwnd, L'カ', 0); bOK = TRUE; break;
	case 10011: OnImeChar(hwnd, L'キ', 0); bOK = TRUE; break;
	case 10012: OnImeChar(hwnd, L'ク', 0); bOK = TRUE; break;
	case 10013: OnImeChar(hwnd, L'ケ', 0); bOK = TRUE; break;
	case 10014: OnImeChar(hwnd, L'コ', 0); bOK = TRUE; break;
	case 10020: OnImeChar(hwnd, L'サ', 0); bOK = TRUE; break;
	case 10021: OnImeChar(hwnd, L'シ', 0); bOK = TRUE; break;
	case 10022: OnImeChar(hwnd, L'ス', 0); bOK = TRUE; break;
	case 10023: OnImeChar(hwnd, L'セ', 0); bOK = TRUE; break;
	case 10024: OnImeChar(hwnd, L'ソ', 0); bOK = TRUE; break;
	case 10030: OnImeChar(hwnd, L'タ', 0); bOK = TRUE; break;
	case 10031: OnImeChar(hwnd, L'チ', 0); bOK = TRUE; break;
	case 10032: OnImeChar(hwnd, L'ツ', 0); bOK = TRUE; break;
	case 10033: OnImeChar(hwnd, L'テ', 0); bOK = TRUE; break;
	case 10034: OnImeChar(hwnd, L'ト', 0); bOK = TRUE; break;
	case 10040: OnImeChar(hwnd, L'ナ', 0); bOK = TRUE; break;
	case 10041: OnImeChar(hwnd, L'ニ', 0); bOK = TRUE; break;
	case 10042: OnImeChar(hwnd, L'ヌ', 0); bOK = TRUE; break;
	case 10043: OnImeChar(hwnd, L'ネ', 0); bOK = TRUE; break;
	case 10044: OnImeChar(hwnd, L'ノ', 0); bOK = TRUE; break;
	case 10050: OnImeChar(hwnd, L'ハ', 0); bOK = TRUE; break;
	case 10051: OnImeChar(hwnd, L'ヒ', 0); bOK = TRUE; break;
	case 10052: OnImeChar(hwnd, L'フ', 0); bOK = TRUE; break;
	case 10053: OnImeChar(hwnd, L'ヘ', 0); bOK = TRUE; break;
	case 10054: OnImeChar(hwnd, L'ホ', 0); bOK = TRUE; break;
	case 10060: OnImeChar(hwnd, L'マ', 0); bOK = TRUE; break;
	case 10061: OnImeChar(hwnd, L'ミ', 0); bOK = TRUE; break;
	case 10062: OnImeChar(hwnd, L'ム', 0); bOK = TRUE; break;
	case 10063: OnImeChar(hwnd, L'メ', 0); bOK = TRUE; break;
	case 10064: OnImeChar(hwnd, L'モ', 0); bOK = TRUE; break;
	case 10070: OnImeChar(hwnd, L'ヤ', 0); bOK = TRUE; break;
	case 10071: OnImeChar(hwnd, L'ユ', 0); bOK = TRUE; break;
	case 10072: OnImeChar(hwnd, L'ヨ', 0); bOK = TRUE; break;
	case 10080: OnImeChar(hwnd, L'ラ', 0); bOK = TRUE; break;
	case 10081: OnImeChar(hwnd, L'リ', 0); bOK = TRUE; break;
	case 10082: OnImeChar(hwnd, L'ル', 0); bOK = TRUE; break;
	case 10083: OnImeChar(hwnd, L'レ', 0); bOK = TRUE; break;
	case 10084: OnImeChar(hwnd, L'ロ', 0); bOK = TRUE; break;
	case 10090: OnImeChar(hwnd, L'ワ', 0); bOK = TRUE; break;
	case 10091: OnImeChar(hwnd, L'ン', 0); bOK = TRUE; break;
	case 10092: OnImeChar(hwnd, L'ー', 0); bOK = TRUE; break;
	case 10100: OnImeChar(hwnd, L'ガ', 0); bOK = TRUE; break;
	case 10101: OnImeChar(hwnd, L'ギ', 0); bOK = TRUE; break;
	case 10102: OnImeChar(hwnd, L'グ', 0); bOK = TRUE; break;
	case 10103: OnImeChar(hwnd, L'ゲ', 0); bOK = TRUE; break;
	case 10104: OnImeChar(hwnd, L'ゴ', 0); bOK = TRUE; break;
	case 10110: OnImeChar(hwnd, L'ザ', 0); bOK = TRUE; break;
	case 10111: OnImeChar(hwnd, L'ジ', 0); bOK = TRUE; break;
	case 10112: OnImeChar(hwnd, L'ズ', 0); bOK = TRUE; break;
	case 10113: OnImeChar(hwnd, L'ゼ', 0); bOK = TRUE; break;
	case 10114: OnImeChar(hwnd, L'ゾ', 0); bOK = TRUE; break;
	case 10120: OnImeChar(hwnd, L'ダ', 0); bOK = TRUE; break;
	case 10121: OnImeChar(hwnd, L'ヂ', 0); bOK = TRUE; break;
	case 10122: OnImeChar(hwnd, L'ヅ', 0); bOK = TRUE; break;
	case 10123: OnImeChar(hwnd, L'デ', 0); bOK = TRUE; break;
	case 10124: OnImeChar(hwnd, L'ド', 0); bOK = TRUE; break;
	case 10130: OnImeChar(hwnd, L'バ', 0); bOK = TRUE; break;
	case 10131: OnImeChar(hwnd, L'ビ', 0); bOK = TRUE; break;
	case 10132: OnImeChar(hwnd, L'ブ', 0); bOK = TRUE; break;
	case 10133: OnImeChar(hwnd, L'ベ', 0); bOK = TRUE; break;
	case 10134: OnImeChar(hwnd, L'ボ', 0); bOK = TRUE; break;
	case 10140: OnImeChar(hwnd, L'パ', 0); bOK = TRUE; break;
	case 10141: OnImeChar(hwnd, L'ピ', 0); bOK = TRUE; break;
	case 10142: OnImeChar(hwnd, L'プ', 0); bOK = TRUE; break;
	case 10143: OnImeChar(hwnd, L'ペ', 0); bOK = TRUE; break;
	case 10144: OnImeChar(hwnd, L'ポ', 0); bOK = TRUE; break;
	case 10150: OnImeChar(hwnd, L'ー', 0); bOK = TRUE; break;
	// ABC
	case 20000: OnChar(hwnd, L'A', 1); bOK = TRUE; break;
	case 20001: OnChar(hwnd, L'B', 1); bOK = TRUE; break;
	case 20002: OnChar(hwnd, L'C', 1); bOK = TRUE; break;
	case 20003: OnChar(hwnd, L'D', 1); bOK = TRUE; break;
	case 20004: OnChar(hwnd, L'E', 1); bOK = TRUE; break;
	case 20005: OnChar(hwnd, L'F', 1); bOK = TRUE; break;
	case 20006: OnChar(hwnd, L'G', 1); bOK = TRUE; break;
	case 20007: OnChar(hwnd, L'H', 1); bOK = TRUE; break;
	case 20008: OnChar(hwnd, L'I', 1); bOK = TRUE; break;
	case 20009: OnChar(hwnd, L'J', 1); bOK = TRUE; break;
	case 20010: OnChar(hwnd, L'K', 1); bOK = TRUE; break;
	case 20011: OnChar(hwnd, L'L', 1); bOK = TRUE; break;
	case 20012: OnChar(hwnd, L'M', 1); bOK = TRUE; break;
	case 20013: OnChar(hwnd, L'N', 1); bOK = TRUE; break;
	case 20014: OnChar(hwnd, L'O', 1); bOK = TRUE; break;
	case 20015: OnChar(hwnd, L'P', 1); bOK = TRUE; break;
	case 20016: OnChar(hwnd, L'Q', 1); bOK = TRUE; break;
	case 20017: OnChar(hwnd, L'R', 1); bOK = TRUE; break;
	case 20018: OnChar(hwnd, L'S', 1); bOK = TRUE; break;
	case 20019: OnChar(hwnd, L'T', 1); bOK = TRUE; break;
	case 20020: OnChar(hwnd, L'U', 1); bOK = TRUE; break;
	case 20021: OnChar(hwnd, L'V', 1); bOK = TRUE; break;
	case 20022: OnChar(hwnd, L'W', 1); bOK = TRUE; break;
	case 20023: OnChar(hwnd, L'X', 1); bOK = TRUE; break;
	case 20024: OnChar(hwnd, L'Y', 1); bOK = TRUE; break;
	case 20025: OnChar(hwnd, L'Z', 1); bOK = TRUE; break;
	case 20026: m_chAccent = L'^'; bOK = TRUE; break;
	case 20027: m_chAccent = L'`'; bOK = TRUE; break;
	case 20028: m_chAccent = L'\''; bOK = TRUE; break;
	case 20029: m_chAccent = L':'; bOK = TRUE; break;
	case 20030: m_chAccent = L','; bOK = TRUE; break;
	case 20031: m_chAccent = L'&'; bOK = TRUE; break;
	case 20032: m_chAccent = L'.'; bOK = TRUE; break;
	case 20033: OnChar(hwnd, 0x011E, 1); bOK = TRUE; break; // Ğ
	// Russian
	case 30000: OnImeChar(hwnd, 0x0410, 0); bOK = TRUE; break;
	case 30001: OnImeChar(hwnd, 0x0411, 0); bOK = TRUE; break;
	case 30002: OnImeChar(hwnd, 0x0412, 0); bOK = TRUE; break;
	case 30003: OnImeChar(hwnd, 0x0413, 0); bOK = TRUE; break;
	case 30004: OnImeChar(hwnd, 0x0414, 0); bOK = TRUE; break;
	case 30005: OnImeChar(hwnd, 0x0415, 0); bOK = TRUE; break;
	case 30006: OnImeChar(hwnd, 0x0401, 0); bOK = TRUE; break;
	case 30007: OnImeChar(hwnd, 0x0416, 0); bOK = TRUE; break;
	case 30008: OnImeChar(hwnd, 0x0417, 0); bOK = TRUE; break;
	case 30009: OnImeChar(hwnd, 0x0418, 0); bOK = TRUE; break;
	case 30010: OnImeChar(hwnd, 0x0419, 0); bOK = TRUE; break;
	case 30011: OnImeChar(hwnd, 0x041A, 0); bOK = TRUE; break;
	case 30012: OnImeChar(hwnd, 0x041B, 0); bOK = TRUE; break;
	case 30013: OnImeChar(hwnd, 0x041C, 0); bOK = TRUE; break;
	case 30014: OnImeChar(hwnd, 0x041D, 0); bOK = TRUE; break;
	case 30015: OnImeChar(hwnd, 0x041E, 0); bOK = TRUE; break;
	case 30016: OnImeChar(hwnd, 0x041F, 0); bOK = TRUE; break;
	case 30017: OnImeChar(hwnd, 0x0420, 0); bOK = TRUE; break;
	case 30018: OnImeChar(hwnd, 0x0421, 0); bOK = TRUE; break;
	case 30019: OnImeChar(hwnd, 0x0422, 0); bOK = TRUE; break;
	case 30020: OnImeChar(hwnd, 0x0423, 0); bOK = TRUE; break;
	case 30021: OnImeChar(hwnd, 0x0424, 0); bOK = TRUE; break;
	case 30022: OnImeChar(hwnd, 0x0425, 0); bOK = TRUE; break;
	case 30023: OnImeChar(hwnd, 0x0426, 0); bOK = TRUE; break;
	case 30024: OnImeChar(hwnd, 0x0427, 0); bOK = TRUE; break;
	case 30025: OnImeChar(hwnd, 0x0428, 0); bOK = TRUE; break;
	case 30026: OnImeChar(hwnd, 0x0429, 0); bOK = TRUE; break;
	case 30027: OnImeChar(hwnd, 0x042A, 0); bOK = TRUE; break;
	case 30028: OnImeChar(hwnd, 0x042B, 0); bOK = TRUE; break;
	case 30029: OnImeChar(hwnd, 0x042C, 0); bOK = TRUE; break;
	case 30030: OnImeChar(hwnd, 0x042D, 0); bOK = TRUE; break;
	case 30031: OnImeChar(hwnd, 0x042E, 0); bOK = TRUE; break;
	case 30032: OnImeChar(hwnd, 0x042F, 0); bOK = TRUE; break;
	// Digits
	case 40000: OnImeChar(hwnd, L'0', 0); bOK = TRUE; break;
	case 40001: OnImeChar(hwnd, L'1', 0); bOK = TRUE; break;
	case 40002: OnImeChar(hwnd, L'2', 0); bOK = TRUE; break;
	case 40003: OnImeChar(hwnd, L'3', 0); bOK = TRUE; break;
	case 40004: OnImeChar(hwnd, L'4', 0); bOK = TRUE; break;
	case 40005: OnImeChar(hwnd, L'5', 0); bOK = TRUE; break;
	case 40006: OnImeChar(hwnd, L'6', 0); bOK = TRUE; break;
	case 40007: OnImeChar(hwnd, L'7', 0); bOK = TRUE; break;
	case 40008: OnImeChar(hwnd, L'8', 0); bOK = TRUE; break;
	case 40009: OnImeChar(hwnd, L'9', 0); bOK = TRUE; break;
	// Greek
	case 41000: OnImeChar(hwnd, 0x03B1, 0); bOK = TRUE; break;
	case 41001: OnImeChar(hwnd, 0x03B2, 0); bOK = TRUE; break;
	case 41002: OnImeChar(hwnd, 0x03B3, 0); bOK = TRUE; break;
	case 41003: OnImeChar(hwnd, 0x03B4, 0); bOK = TRUE; break;
	case 41004: OnImeChar(hwnd, 0x03B5, 0); bOK = TRUE; break;
	case 41005: OnImeChar(hwnd, 0x03B6, 0); bOK = TRUE; break;
	case 41006: OnImeChar(hwnd, 0x03B7, 0); bOK = TRUE; break;
	case 41007: OnImeChar(hwnd, 0x03B8, 0); bOK = TRUE; break;
	case 41008: OnImeChar(hwnd, 0x03B9, 0); bOK = TRUE; break;
	case 41009: OnImeChar(hwnd, 0x03BA, 0); bOK = TRUE; break;
	case 41010: OnImeChar(hwnd, 0x03BB, 0); bOK = TRUE; break;
	case 41011: OnImeChar(hwnd, 0x03BC, 0); bOK = TRUE; break;
	case 41012: OnImeChar(hwnd, 0x03BD, 0); bOK = TRUE; break;
	case 41013: OnImeChar(hwnd, 0x03BE, 0); bOK = TRUE; break;
	case 41014: OnImeChar(hwnd, 0x03BF, 0); bOK = TRUE; break;
	case 41015: OnImeChar(hwnd, 0x03C0, 0); bOK = TRUE; break;
	case 41016: OnImeChar(hwnd, 0x03C1, 0); bOK = TRUE; break;
	case 41017: OnImeChar(hwnd, 0x03C3, 0); bOK = TRUE; break;
	case 41018: OnImeChar(hwnd, 0x03C4, 0); bOK = TRUE; break;
	case 41019: OnImeChar(hwnd, 0x03C5, 0); bOK = TRUE; break;
	case 41020: OnImeChar(hwnd, 0x03C6, 0); bOK = TRUE; break;
	case 41021: OnImeChar(hwnd, 0x03C7, 0); bOK = TRUE; break;
	case 41022: OnImeChar(hwnd, 0x03C8, 0); bOK = TRUE; break;
	case 41023: OnImeChar(hwnd, 0x03C9, 0); bOK = TRUE; break;
	default:
		break;
	}

	m_bCharFeed = bOldFeed;
	return bOK;
}
