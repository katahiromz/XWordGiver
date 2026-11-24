// xwg.cpp --- Japanese Crossword Generator
// Author: katahiromz
// License: MIT

#include "xwg.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <utility>
#include <initializer_list>
#include <io.h> // _isatty
#include <gdiplus.h>

HINSTANCE xg_hInstance = nullptr; // アプリのインスタンス。
HWND xg_hMainWnd = nullptr; // メイン ウィンドウ。
HANDLE xg_hHeap = ::GetProcessHeap(); // ヒープ。
CRITICAL_SECTION xg_csLock; // クリティカルセクション。
xg_algo_quit_fn_t xg_algo_quit_fn = nullptr; // アルゴリズム終了関数。
INT xg_nRules = XG_STANDARD_RULES;
HRESULT xg_hrCoInit = E_FAIL;
ULONG_PTR g_gdiplusToken = 0;

// XgNormalizeString関数で使う写像。
std::unordered_map<WCHAR, WCHAR> xg_normalize_map;

// ユーザーが日本語か？
BOOL __fastcall XgIsUserJapanese(void) noexcept {
	return PRIMARYLANGID(LANGIDFROMLCID(GetThreadLocale())) == LANG_JAPANESE;
}

// XWordGiverを初期化。
HRESULT __fastcall XgInitialize(void) {
	InitializeCriticalSection(&xg_csLock);
	xg_hrCoInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(xg_hrCoInit))
		return xg_hrCoInit;
	xg_hInstance = GetModuleHandleW(nullptr);
	xg_hMainWnd = nullptr;
	xg_hHeap = ::GetProcessHeap();
	std::srand(GetTickCount() + (GetCurrentThreadId() << 10));
	xg_algo_quit_fn = nullptr;
	xg_normalize_map = {
		// 空白マス
		{ L' ', ZEN_SPACE },
		{ L'_', ZEN_SPACE },
		{ ZEN_UNDERLINE, ZEN_SPACE },
		// 黒マス
		{ L'#', ZEN_BLACK },
		{ ZEN_SHARP1, ZEN_BLACK },
		{ ZEN_SHARP2, ZEN_BLACK },
		{ L'.', ZEN_BLACK }, // クロスワード ギバーでは立入禁止は黒マスと見なす
		{ ZEN_DOT, ZEN_BLACK },
		// 小文字を大文字に
		{ L'ァ', L'ア' },
		{ L'ィ', L'イ' },
		{ L'ゥ', L'ウ' },
		{ L'ェ', L'エ' },
		{ L'ォ', L'オ' },
		{ L'ッ', L'ツ' },
		{ L'ャ', L'ヤ' },
		{ L'ュ', L'ユ' },
		{ L'ョ', L'ヨ' },
		{ L'ヵ', L'カ' },
		{ L'ヶ', L'ケ' },
	};

	// GDI+ 初期化
	using namespace Gdiplus;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

	return S_OK;
}

// XWordGiverを破棄。後始末。
void __fastcall XgUninitialize(void) {
	using namespace Gdiplus;
	GdiplusShutdown(g_gdiplusToken);
	g_gdiplusToken = 0;

	xg_normalize_map.clear();
	DeleteCriticalSection(&xg_csLock);
	xg_hMainWnd = nullptr;
	if (SUCCEEDED(xg_hrCoInit))
		CoUninitialize();
	xg_hrCoInit = E_FAIL;
}

// リソース文字列を読み込む。
MStringW __fastcall XgLoadString(INT id) {
	WCHAR sz[1024];
	sz[0] = UNICODE_NULL;
	INT ret = LoadStringW(xg_hInstance, id, sz, _countof(sz));
	assert(ret);
	return sz;
}

// 文字種の変換。
MStringW __fastcall XgLCMapString(const MStringW & str, DWORD dwMapFlags) {
	if (str.empty())
		return MStringW();

	// 日本語ロケール
	const LCID lcid = MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT);
	const INT cchSrc = static_cast<int>(str.size());

	// 必要な文字数を取得
	INT cchDest = ::LCMapStringW(lcid, dwMapFlags, str.c_str(), cchSrc, nullptr, 0);
	if (cchDest <= 0)
		return str; // 失敗時は元の文字列を返す

	MStringW result(size_t(cchDest), UNICODE_NULL);
	cchDest = ::LCMapStringW(lcid, dwMapFlags, str.c_str(), cchSrc, &result[0], cchDest);
	if (cchDest <= 0)
		return str;

	result.resize(cchDest);
	return result;
}

// クロスワードで扱いやすいように文字列を正規化する。
MStringW __fastcall XgNormalizeString(const MStringW& str) {
	MStringW ret = XgLCMapString(str, LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE);
	for (auto& ch : ret) {
		auto it = xg_normalize_map.find(ch);
		if (it == xg_normalize_map.end())
			continue;
		ch = it->second;
	}
	return ret;
}

// 文字種の変換。
MStringW __fastcall XgConvertString(const MStringW& str, BOOL bLowercase, BOOL bHiragana) {
	MStringW ret;
	ret.reserve(str.size());
	for (auto ch : str) {
		WCHAR newch = ch;
		// 小さな字を大きな字にする。
		auto it = xg_normalize_map.find(ch);
		if (it != xg_normalize_map.end())
			ch = it->second;
		if (XgIsCharKanaW(ch) || XgIsCharKanjiW(ch) || ch == ZEN_PROLONG || XgIsCharHangulW(ch) ||
			ch == ZEN_UNDERLINE)
		{
			// カナか漢字かハングルなら全角文字。
			if (bHiragana) {
				LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA, &ch, 1, &newch, 1);
			} else {
				LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_KATAKANA, &ch, 1, &newch, 1);
			}
		} else {
			// それ以外は半角文字。
			if (bLowercase) {
				LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_LOWERCASE, &ch, 1, &newch, 1);
			} else {
				LCMapStringW(XG_JPN_LOCALE, LCMAP_HALFWIDTH | LCMAP_UPPERCASE, &ch, 1, &newch, 1);
			}
		}
		ret += newch;
	}
	return ret;
}

// 自然な文字列に変換する。
MStringW __fastcall XgNaturalizeString(const MStringW& str) {
	MStringW ret = str;

	// 全角英数を半角英数・小文字にする。
	for (auto& wch : ret) {
		if (L'Ａ' <= wch && wch <= L'Ｚ')
			wch = L'a' + (wch - L'Ａ');
		else if (L'ａ' <= wch && wch <= L'ｚ')
			wch = L'a' + (wch - L'ａ');
		else if (L'０' <= wch && wch <= L'９')
			wch = L'0' + (wch - L'０');
	}

	return ret;
}

// 黒マスが隣り合っているか？
BOOL __fastcall MBoard::DoubleBlack() const noexcept {
	const INT n1 = Columns() - 1;
	const INT n2 = Rows() - 1;
	INT i = Rows();
	for (--i; i >= 0; --i) {
		for (INT j = 0; j < n1; j++) {
			if (GetCell0(j, i) == ZEN_BLACK && GetCell0(j + 1, i) == ZEN_BLACK)
				return TRUE; // 隣り合っていた。
		}
	}
	INT j = Columns();
	for (--j; j >= 0; --j) {
		for (INT i0 = 0; i0 < n2; i0++) {
			if (GetCell0(j, i0) == ZEN_BLACK && GetCell0(j, i0 + 1) == ZEN_BLACK)
				return TRUE; // 隣り合っていた。
		}
	}
	return FALSE; // 隣り合っていなかった。
}

// 三方向が黒マスで囲まれたマスがあるかどうか？
BOOL __fastcall MBoard::TriBlackAround() const noexcept {
	for (INT i = Rows() - 2; i >= 1; --i) {
		for (INT j = Columns() - 2; j >= 1; --j) {
			if ((GetCell0(j, i - 1) == ZEN_BLACK) + (GetCell0(j, i + 1) == ZEN_BLACK) +
				(GetCell0(j - 1, i) == ZEN_BLACK) + (GetCell0(j + 1, i) == ZEN_BLACK) >= 3)
			{
				return TRUE; // あった。
			}
		}
	}
	return FALSE; // なかった。
}

// 黒マスで分断されているかどうか？
BOOL __fastcall MBoard::DividedByBlack() const {
	if (NonSpaceCount() <= 1)
		return FALSE;

	const INT nCols = Columns(), nRows = Rows();
	INT nCount = nRows * nCols;

	// 各マスに対応するフラグ群。
	std::vector<BYTE> pb(nCount, 0);

	// 位置のキュー（待ち行列）。
	// 黒マスではないマスを探し、positionsに追加する。
	std::queue<MBoardPos> positions;
	if (GetCell0(0, 0) != ZEN_BLACK) {
		positions.emplace(0, 0);
	} else {
		for (INT i = 0; i < nRows; ++i) {
			for (INT j = 0; j < nCols; ++j) {
				if (GetCell0(j, i) != ZEN_BLACK) {
					positions.emplace(j, i);
					i = nRows;
					j = nCols;
					break;
				}
			}
		}
	}

	// 連続領域の塗りつぶし。
	while (!positions.empty()) {
		// 位置をキューの一番上から取り出す。
		MBoardPos pos = positions.front();
		positions.pop();
		// フラグが立っていないか？
		if (!pb[pos.m_i * nCols + pos.m_j]) {
			// フラグを立てる。
			pb[pos.m_i * nCols + pos.m_j] = TRUE;
			// 上。
			if (pos.m_i > 0 && GetCell0(pos.m_j, pos.m_i - 1) != ZEN_BLACK)
				positions.emplace(pos.m_j, pos.m_i - 1);
			// 下。
			if (pos.m_i < nRows - 1 && GetCell0(pos.m_j, pos.m_i + 1) != ZEN_BLACK)
				positions.emplace(pos.m_j, pos.m_i + 1);
			// 左。
			if (pos.m_j > 0 && GetCell0(pos.m_j - 1, pos.m_i) != ZEN_BLACK)
				positions.emplace(pos.m_j - 1, pos.m_i);
			// 右。
			if (pos.m_j < nCols - 1 && GetCell0(pos.m_j + 1, pos.m_i) != ZEN_BLACK)
				positions.emplace(pos.m_j + 1, pos.m_i);
		}
	}

	// すべてのマスについて。
	while (nCount-- > 0) {
		// フラグが立っていないのに、黒マスではないマスがあったら、失敗。
		if (pb[nCount] == 0 && GetAt0(nCount) != ZEN_BLACK) {
			// フラグ群を解放。
			return TRUE; // 分断されている。
		}
	}

	return FALSE; // 分断されていない。
}

// 黒斜三連か？
BOOL __fastcall MBoard::ThreeDiagonals() const noexcept {
	const INT nCols = Columns(), nRows = Rows();
	for (INT i = 0; i < nRows - 2; i++) {
		for (INT j = 0; j < nCols - 2; j++) {
			if (GetCell0(j, i) != ZEN_BLACK)
				continue;
			if (GetCell0(j + 1, i + 1) != ZEN_BLACK)
				continue;
			if (GetCell0(j + 2, i + 2) != ZEN_BLACK)
				continue;
			return TRUE;
		}
	}
	for (INT i = 0; i < nRows - 2; i++) {
		for (INT j = 2; j < nCols; j++) {
			if (GetCell0(j, i) != ZEN_BLACK)
				continue;
			if (GetCell0(j - 1, i + 1) != ZEN_BLACK)
				continue;
			if (GetCell0(j - 2, i + 2) != ZEN_BLACK)
				continue;
			return TRUE;
		}
	}
	return FALSE;
}

// 黒斜四連か？
BOOL __fastcall MBoard::FourDiagonals() const noexcept {
	const INT nCols = Columns(), nRows = Rows();
	for (INT i = 0; i < nRows - 3; i++) {
		for (INT j = 0; j < nCols - 3; j++) {
			if (GetCell0(j, i) != ZEN_BLACK)
				continue;
			if (GetCell0(j + 1, i + 1) != ZEN_BLACK)
				continue;
			if (GetCell0(j + 2, i + 2) != ZEN_BLACK)
				continue;
			if (GetCell0(j + 3, i + 3) != ZEN_BLACK)
				continue;
			return TRUE;
		}
	}
	for (INT i = 0; i < nRows - 3; i++) {
		for (INT j = 3; j < nCols; j++) {
			if (GetCell0(j, i) != ZEN_BLACK)
				continue;
			if (GetCell0(j - 1, i + 1) != ZEN_BLACK)
				continue;
			if (GetCell0(j - 2, i + 2) != ZEN_BLACK)
				continue;
			if (GetCell0(j - 3, i + 3) != ZEN_BLACK)
				continue;
			return TRUE;
		}
	}
	return FALSE;
}

// 黒マスが点対称か？
BOOL __fastcall MBoard::IsPointSymmetry() const noexcept {
	const INT nCols = Columns(), nRows = Rows();
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nCols; ++j) {
			if ((GetCell0(j, i) == ZEN_BLACK) !=
				(GetCell0(nCols - (j + 1), nRows - (i + 1)) == ZEN_BLACK))
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

// 黒マスが線対称（タテ）か？
BOOL __fastcall MBoard::IsLineSymmetryV() const noexcept {
	const INT nCols = Columns(), nRows = Rows();
	const INT nHalfRows = nRows / 2;
	for (INT i = 0; i < nHalfRows; ++i) {
		for (INT j = 0; j < nCols; ++j) {
			if ((GetCell0(j, i) == ZEN_BLACK) != (GetCell0(j, nRows - (i + 1)) == ZEN_BLACK))
				return FALSE;
		}
	}
	return TRUE;
}

// 黒マスが線対称（ヨコ）か？
BOOL __fastcall MBoard::IsLineSymmetryH() const noexcept {
	const INT nCols = Columns(), nRows = Rows();
	const INT nHalfCols = nCols / 2;
	for (INT j = 0; j < nHalfCols; ++j) {
		for (INT i = 0; i < nRows; ++i) {
			if ((GetCell0(j, i) == ZEN_BLACK) != (GetCell0(nCols - (j + 1), i) == ZEN_BLACK))
				return FALSE;
		}
	}
	return TRUE;
}

// 対称位置を取得。
INT __fastcall MBoard::GetMirroredPositions(INT jColumn, INT iRow, MBoardPos out[8]) const noexcept {
	const INT nCols = Columns(), nRows = Rows();
	const INT rules = Rules();
	INT n = 1;
	out[0] = MBoardPos(jColumn, iRow);
	if (rules & RULE_LINESYMMETRYV) { // タテ：行を上下反転。
		for (INT k = 0; k < n; ++k)
			out[n + k] = MBoardPos(out[k].m_j, nRows - (out[k].m_i + 1));
		n *= 2;
	}
	if (rules & RULE_LINESYMMETRYH) { // ヨコ：列を左右反転。
		for (INT k = 0; k < n; ++k)
			out[n + k] = MBoardPos(nCols - (out[k].m_j + 1), out[k].m_i);
		n *= 2;
	}
	if (rules & RULE_POINTSYMMETRY) { // 点対称：行も列も反転。
		for (INT k = 0; k < n; ++k)
			out[n + k] = MBoardPos(nCols - (out[k].m_j + 1), nRows - (out[k].m_i + 1));
		n *= 2;
	}
	return n;
}

// 対称性ルールに従って、対称位置にも同じマスをセットする。カウントあり。
void __fastcall MBoard::MirroredSetCell(INT jColumn, INT iRow, WCHAR chNew) noexcept {
	if (jColumn < 0 || jColumn >= Columns() || iRow < 0 || iRow >= Rows())
		return;
	MBoardPos pos[8];
	const INT n = GetMirroredPositions(jColumn, iRow, pos);
	for (INT k = 0; k < n; ++k)
		SetCell(pos[k].m_j, pos[k].m_i, chNew);
}

// おそらく黒マスを置けるか？（対称位置も含め、置いた後の状態で判定する）
BOOL __fastcall MBoard::MaybeCanPutBlack(INT jColumn, INT iRow) const noexcept {
	const INT rules = Rules();
	const INT nCols = Columns(), nRows = Rows();

	if (jColumn < 0 || jColumn >= nCols || iRow < 0 || iRow >= nRows)
		return FALSE;

	// 実際に黒になるマスの集合（自分自身と、その対称位置）。
	MBoardPos pos[8];
	const INT n = GetMirroredPositions(jColumn, iRow, pos);

	// 置いた後に黒になるか？（範囲外は黒ではない）
	auto will_be_black = [&](INT j, INT i) -> bool {
		for (INT k = 0; k < n; ++k) {
			if (pos[k].m_j == j && pos[k].m_i == i)
				return true;
		}
		return GetCell(j, i, ZEN_SPACE) == ZEN_BLACK;
	};

	static const INT dj[4] = { -1, 1, 0, 0 };
	static const INT di[4] = { 0, 0, -1, 1 };

	// 斜めの連続の上限。0なら判定しない。
	const INT diag_limit = (rules & RULE_DONTTHREEDIAGONALS) ? 3 :
						   (rules & RULE_DONTFOURDIAGONALS) ? 4 : 0;

	for (INT k = 0; k < n; ++k) {
		const INT j = pos[k].m_j, i = pos[k].m_i;

		// 四隅黒禁。
		if (rules & RULE_DONTCORNERBLACK) {
			if ((j == 0 || j == nCols - 1) && (i == 0 || i == nRows - 1))
				return FALSE;
		}

		// 連黒禁。隣の黒マス（置く黒マス同士を含む）があれば不可。
		if (rules & RULE_DONTDOUBLEBLACK) {
			for (INT d = 0; d < 4; ++d) {
				if (will_be_black(j + dj[d], i + di[d]))
					return FALSE;
			}
		}

		// 三方黒禁。このマスと隣のマスが、三方を黒で囲まれないか。
		if (rules & RULE_DONTTRIDIRECTIONS) {
			for (INT c = -1; c < 4; ++c) {
				const INT cj = (c < 0) ? j : j + dj[c];
				const INT ci = (c < 0) ? i : i + di[c];
				// TriBlackAround と同じく、内側のマスだけが対象。
				if (cj < 1 || cj > nCols - 2 || ci < 1 || ci > nRows - 2)
					continue;
				INT count = 0;
				for (INT d = 0; d < 4; ++d) {
					if (will_be_black(cj + dj[d], ci + di[d]))
						++count;
				}
				if (count >= 3)
					return FALSE;
			}
		}

		// 黒斜三連禁／黒斜四連禁。このマスを通る斜めの連続の長さを数える。
		if (diag_limit > 0) {
			for (INT sign = -1; sign <= 1; sign += 2) { // ＼ と ／
				INT run = 1;
				for (INT s = 1; s < diag_limit && will_be_black(j + s, i + sign * s); ++s)
					++run;
				for (INT s = 1; s < diag_limit && will_be_black(j - s, i - sign * s); ++s)
					++run;
				if (run >= diag_limit)
					return FALSE;
			}
		}
	}

	return TRUE;
}

// ルールに適合するか？
BOOL __fastcall MBoard::AreBlacksOK(INT rules) const {
	if ((rules & RULE_DONTCORNERBLACK) && CornerBlack())
		return FALSE;
	if ((rules & RULE_DONTDOUBLEBLACK) && DoubleBlack())
		return FALSE;
	if ((rules & RULE_DONTTRIDIRECTIONS) && TriBlackAround())
		return FALSE;
	if (rules & RULE_DONTTHREEDIAGONALS) {
		if (ThreeDiagonals())
			return FALSE;
	}
	if (rules & RULE_DONTFOURDIAGONALS) {
		if (FourDiagonals())
			return FALSE;
	}
	if ((rules & RULE_POINTSYMMETRY) && !IsPointSymmetry())
		return FALSE;
	if ((rules & RULE_LINESYMMETRYV) && !IsLineSymmetryV())
		return FALSE;
	if ((rules & RULE_LINESYMMETRYH) && !IsLineSymmetryH())
		return FALSE;
	if ((rules & RULE_DONTDIVIDE) && DividedByBlack())
		return FALSE;

	return TRUE;
}
BOOL __fastcall MBoard::AreBlacksOK() const {
	return AreBlacksOK(Rules());
}

// 違反するルールを返す。
INT __fastcall MBoard::GetViolatedRules() const {
	return GetViolatedRules(Rules());
}

// 違反するルールを返す。
INT __fastcall MBoard::GetViolatedRules(INT rules) const {
	INT ret = 0;
	if ((rules & RULE_DONTCORNERBLACK) && CornerBlack())
		ret |= RULE_DONTCORNERBLACK;
	if ((rules & RULE_DONTDOUBLEBLACK) && DoubleBlack())
		ret |= RULE_DONTDOUBLEBLACK;
	if ((rules & RULE_DONTTRIDIRECTIONS) && TriBlackAround())
		ret |= RULE_DONTTRIDIRECTIONS;
	if (rules & RULE_DONTTHREEDIAGONALS) {
		if (ThreeDiagonals())
			ret |= RULE_DONTTHREEDIAGONALS;
	}
	if (rules & RULE_DONTFOURDIAGONALS) {
		if (FourDiagonals())
			ret |= RULE_DONTFOURDIAGONALS;
	}
	if ((rules & RULE_POINTSYMMETRY) && !IsPointSymmetry())
		ret |= RULE_POINTSYMMETRY;
	if ((rules & RULE_LINESYMMETRYV) && !IsLineSymmetryV())
		ret |= RULE_LINESYMMETRYV;
	if ((rules & RULE_LINESYMMETRYH) && !IsLineSymmetryH())
		ret |= RULE_LINESYMMETRYH;
	if ((rules & RULE_DONTDIVIDE) && DividedByBlack())
		ret |= RULE_DONTDIVIDE;

	return ret;
}

// 多分大丈夫か？
BOOL __fastcall MBoard::MaybeOK() const {
	// MBoard::MaybeCanPutBlack で逐一チェックしているのなら、いくつかのチェックは要らない。
	const INT rules = Rules() & ~(RULE_DONTCORNERBLACK | RULE_DONTDOUBLEBLACK |
								  RULE_DONTTRIDIRECTIONS |
								  RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
	return AreBlacksOK(rules);
}

// 文字マスをクリアする。
void MBoard::ClearLetters() {
	const INT nColumns = Columns(), nRows = Rows();
	for (INT iRow = 0; iRow < nRows; ++iRow) {
		for (INT jColumn = 0; jColumn < nColumns; ++jColumn) {
			auto ch = GetCell0(jColumn, iRow);
			if (XgIsLetter(ch)) {
				SetCell(jColumn, iRow, ZEN_SPACE);
			}
		}
	}
}

// 文字列の配列から盤面を作る。'#'=黒マス、'.'=空白マス、それ以外=その文字で埋めたマス。
// 例: XgMakeBoard({ L"..#", L"...", L"#.." })
MBoard __fastcall XgMakeBoard(const std::vector<const wchar_t *>& rows, INT nRules) {
	const INT nRows = (INT)rows.size();
	assert(nRows > 0);
	const INT nColumns = (INT)std::wcslen(rows[0]);
	assert(nColumns > 0);
	MBoard board(nColumns, nRows, nRules);
	board.clear(); // 全マスを ZEN_SPACE にする
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nColumns; ++j) {
			wchar_t ch = rows[i][j];
			if (ch == L'#')
				board.SetCell(j, i, ZEN_BLACK);
			else if (ch != L'.')
				board.SetCell(j, i, ch);
		}
	}
	return board;
}

// ローカルのファイルを探す。
MStringW __fastcall XgFindLocalFile(LPCWSTR pszFileName, LPCWSTR pszPathToAdd) {
	WCHAR szBase[MAX_PATH];
	DWORD len = GetModuleFileNameW(NULL, szBase, _countof(szBase));
	if (len == 0 || len >= _countof(szBase))
		return pszFileName;
	PathRemoveFileSpecW(szBase);

	for (INT up = 0; up < 3; ++up) {
		WCHAR szPath[MAX_PATH];
		lstrcpynW(szPath, szBase, _countof(szPath));
		for (INT i = 0; i < up; ++i)
			PathRemoveFileSpecW(szPath);
		if (pszPathToAdd)
			PathAppendW(szPath, pszPathToAdd);
		PathAppendW(szPath, pszFileName);
		if (PathFileExistsW(szPath))
			return szPath;
	}

	if (pszPathToAdd)
		return XgFindLocalFile(pszFileName, nullptr);

	return pszFileName;
}

// 盤面のルールチェックを行う。
BOOL __fastcall XgRuleCheck(const MBoard& board, BOOL bLoose) {
	const INT nRules = board.Rules();

	// 連黒禁。
	if ((nRules & RULE_DONTDOUBLEBLACK) && board.DoubleBlack()) {
		return FALSE;
	}
	// 四隅黒禁。
	if ((nRules & RULE_DONTCORNERBLACK) && board.CornerBlack()) {
		return FALSE;
	}
	// 三方黒禁。
	if ((nRules & RULE_DONTTRIDIRECTIONS) && board.TriBlackAround()) {
		return FALSE;
	}
	// 分断禁。
	if ((nRules & RULE_DONTDIVIDE) && board.DividedByBlack()) {
		return FALSE;
	}
	// 黒斜三連禁。
	if ((nRules & RULE_DONTTHREEDIAGONALS) && board.ThreeDiagonals()) {
		return FALSE;
	} else {
		// 黒斜四連禁。
		if ((nRules & RULE_DONTFOURDIAGONALS) && board.FourDiagonals()) {
			return FALSE;
		}
	}

	if (!bLoose) { // 判定がゆるくなければ、対称もチェックする。
		// 黒マス点対称。
		if ((nRules & RULE_POINTSYMMETRY) && !board.IsPointSymmetry()) {
			return FALSE;
		}
		// 黒マス線対称。
		if ((nRules & RULE_LINESYMMETRYV) && !board.IsLineSymmetryV()) {
			return FALSE;
		}
		if ((nRules & RULE_LINESYMMETRYH) && !board.IsLineSymmetryH()) {
			return FALSE;
		}
	}

	// 合格。
	return TRUE;
}

// 単体テスト。
void __fastcall XgUnitTest(void) {
	// XgFindLocalFileのテスト
	auto file = XgFindLocalFile(L"PAT.txt");
	assert(file.find(L'/') != file.npos || file.find(L'\\') != file.npos);

	const DWORD fullkata = LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE;

	// 黒マスをまとめて置くヘルパー（個数もカウントされる）。
	auto put = [](MBoard& b, std::initializer_list<MBoardPos> list) {
		for (const MBoardPos& p : list)
			b.SetCell(p.m_j, p.m_i, ZEN_BLACK);
	};

	/////////////////////////////////////////////////////////////////////////
	// XgLCMapString
	{
		assert(XgLCMapString(L"", fullkata) == L"");
		assert(XgLCMapString(L"ｱｲｳあいうABC", fullkata) == L"アイウアイウＡＢＣ");
		assert(XgLCMapString(L"abc", fullkata) == L"ＡＢＣ");
		assert(XgLCMapString(L"123", LCMAP_FULLWIDTH) == L"１２３");
		assert(XgLCMapString(L"ｶﾞ", LCMAP_FULLWIDTH) == L"ガ"); // 濁点の結合
		assert(XgLCMapString(L"あ", LCMAP_KATAKANA) == L"ア");
		assert(XgLCMapString(L"アイウ", LCMAP_KATAKANA) == L"アイウ"); // 冪等
	}

	/////////////////////////////////////////////////////////////////////////
	// XgNormalizeString
	{
		assert(XgNormalizeString(L"") == L"");
		assert(XgNormalizeString(L"ｱｲｳあいうABC #") == L"アイウアイウＡＢＣ　■");
		// 空白マス
		assert(XgNormalizeString(L" ") == MStringW(1, ZEN_SPACE));
		assert(XgNormalizeString(L"_") == MStringW(1, ZEN_SPACE));
		assert(XgNormalizeString(MStringW(1, ZEN_UNDERLINE)) == MStringW(1, ZEN_SPACE));
		assert(XgNormalizeString(MStringW(1, ZEN_SPACE)) == MStringW(1, ZEN_SPACE));
		// 黒マス
		assert(XgNormalizeString(L"#") == MStringW(1, ZEN_BLACK));
		assert(XgNormalizeString(MStringW(1, ZEN_SHARP1)) == MStringW(1, ZEN_BLACK));
		assert(XgNormalizeString(MStringW(1, ZEN_SHARP2)) == MStringW(1, ZEN_BLACK));
		assert(XgNormalizeString(L".") == MStringW(1, ZEN_BLACK));
		assert(XgNormalizeString(MStringW(1, ZEN_DOT)) == MStringW(1, ZEN_BLACK));
		assert(XgNormalizeString(MStringW(1, ZEN_BLACK)) == MStringW(1, ZEN_BLACK));
		// 小文字→大文字
		assert(XgNormalizeString(L"ァィゥェォッャュョヵヶ") == L"アイウエオツヤユヨカケ");
		assert(XgNormalizeString(L"ぁぃぅぇぉっゃゅょ") == L"アイウエオツヤユヨ");
		assert(XgNormalizeString(L"ｧｨｩｪｫｯｬｭｮ") == L"アイウエオツヤユヨ");
		// 混在・冪等
		assert(XgNormalizeString(L"ｷｬｯﾄ_#") == L"キヤツト　■");
		MStringW once = XgNormalizeString(L"ｷｬｯﾄ_#.a1");
		assert(XgNormalizeString(once) == once);
	}

	/////////////////////////////////////////////////////////////////////////
	// MAutoCloseHandle
	{
		MAutoCloseHandle h;
		assert(h.m_h == NULL);
		h = CreateEventW(NULL, TRUE, FALSE, NULL);
		assert(h.m_h != NULL);
		assert((HANDLE)h == h.m_h);
		h.Close();
		assert(h.m_h == NULL);
		h.Close(); // 二重Closeは安全
		assert(h.m_h == NULL);

		h = CreateEventW(NULL, TRUE, FALSE, NULL);
		HANDLE first = h.m_h;
		h = CreateEventW(NULL, TRUE, FALSE, NULL); // 代入で前のハンドルを閉じる
		assert(h.m_h != NULL && h.m_h != first);

		MAutoCloseHandle inv(INVALID_HANDLE_VALUE);
		inv.Close(); // INVALID_HANDLE_VALUEは閉じない（クラッシュしない）
	}

	/////////////////////////////////////////////////////////////////////////
	// AutoFClose
	{
		AutoFClose fp;
		assert(fp.m_fp == NULL);
		fp.Close(); // NULLでも安全
		FILE *f = fopen("NUL", "rb");
		if (f) {
			fp = f;
			assert((FILE*)fp == f);
			fp.Close();
			assert(fp.m_fp == NULL);
		}
	}

	/////////////////////////////////////////////////////////////////////////
	// Shared / SharedLong
	{
		SharedLong a;
		assert(a.Create(123));
		LONG v = 0;
		assert(a.Read(&v));
		assert(v == 123);
		assert(a.Write(456));
		assert(a.Read(&v));
		assert(v == 456);

		SharedLong reader;
		assert(reader.Open(FALSE));
		v = 0;
		assert(reader.Read(&v));
		assert(v == 456);
		assert(!reader.Write(1)); // 読み取り専用では書けない
		reader.Read(&v);
		assert(v == 456);

		SharedLong writer;
		assert(writer.Open(TRUE));
		assert(writer.Write(789));
		assert(a.Read(&v));
		assert(v == 789);
		assert(reader.Read(&v));
		assert(v == 789);
	}

	/////////////////////////////////////////////////////////////////////////
	// MBoard: 構築・サイズ・ルール
	{
		MBoard b(4, 5, XG_STANDARD_RULES);
		assert(b.Columns() == 4);
		assert(b.Rows() == 5);
		assert(b.Count() == 20);
		assert(b.Rules() == XG_STANDARD_RULES);
		assert(b.NonSpaceCount() == 0);
		assert(b.empty());
		assert(!b.IsFull());
		assert(b.m_ptr == b.m_allocated);

		b.Rules() = RULE_DONTCORNERBLACK;
		assert(b.Rules() == RULE_DONTCORNERBLACK);

		// SetSize: ルールは維持、中身は初期化。
		b.SetCell(0, 0, L'ア');
		b.SetSize(6, 3);
		assert(b.Columns() == 6 && b.Rows() == 3 && b.Count() == 18);
		assert(b.Rules() == RULE_DONTCORNERBLACK);
		assert(b.empty());
		assert(b.NonSpaceCount() == 0);

		// 最小・最大サイズ
		MBoard mn(XG_MIN_SIZE, XG_MIN_SIZE, 0);
		assert(mn.Count() == XG_MIN_SIZE * XG_MIN_SIZE);
		MBoard mx(XG_MAX_SIZE, XG_MAX_SIZE, 0);
		assert(mx.Count() == 256 * 256);
		assert(mx.GetCell0(255, 255) == ZEN_SPACE);
	}

	/////////////////////////////////////////////////////////////////////////
	// MBoard: セルアクセスと非空白カウント
	{
		MBoard b(4, 4, 0);

		// SetCell（カウントあり）
		b.SetCell(1, 2, L'あ');
		assert(b.GetCell(1, 2) == L'あ');
		assert(b.GetCell0(1, 2) == L'あ');
		assert(b.GetAt(1 + 2 * 4) == L'あ');
		assert(b.GetAt0(1 + 2 * 4) == L'あ');
		assert(b.NonSpaceCount() == 1);
		assert(!b.empty());
		b.SetCell(1, 2, L'い'); // 非空白→非空白：変化なし
		assert(b.NonSpaceCount() == 1);
		b.SetCell(1, 2, ZEN_SPACE); // 非空白→空白
		assert(b.NonSpaceCount() == 0);
		b.SetCell(1, 2, ZEN_SPACE); // 空白→空白
		assert(b.NonSpaceCount() == 0);
		assert(b.empty());

		// 黒マスも「非空白」
		b.SetCell(0, 0, ZEN_BLACK);
		assert(b.NonSpaceCount() == 1);

		// 範囲外は何もしない
		b.SetCell(4, 0, L'ア');
		b.SetCell(0, 4, L'ア');
		b.SetCell(-1, 0, L'ア');
		b.SetCell(0, -1, L'ア');
		assert(b.NonSpaceCount() == 1);
		assert(b.GetCell(4, 0) == ZEN_BLACK);
		assert(b.GetCell(4, 0, ZEN_SPACE) == ZEN_SPACE);
		assert(b.GetCell(-1, 0, L'X') == L'X');

		// SetAt / GetAt
		b.clear();
		b.RecountNonSpaces();
		assert(b.NonSpaceCount() == 0);
		b.SetAt(5, L'ア');
		assert(b.GetAt(5) == L'ア');
		assert(b.NonSpaceCount() == 1);
		b.SetAt(5, ZEN_SPACE);
		assert(b.NonSpaceCount() == 0);
		b.SetAt(-1, L'ア');
		b.SetAt(16, L'ア');
		assert(b.NonSpaceCount() == 0);
		assert(b.GetAt(-1) == ZEN_BLACK);
		assert(b.GetAt(16) == ZEN_BLACK);
		assert(b.GetAt(16, L'X') == L'X');
		assert(b.GetAt(15) == ZEN_SPACE);

		// SetCell0 / SetAt0（カウントなし）→ RecountNonSpaces
		b.SetCell0(0, 0, L'ア');
		b.SetAt0(15, L'イ');
		assert(b.NonSpaceCount() == 0);
		b.RecountNonSpaces();
		assert(b.NonSpaceCount() == 2);
		assert(!b.empty());

		// clear
		b.clear();
		b.RecountNonSpaces();
		assert(b.empty());
		assert(b.NonSpaceCount() == 0);

		// IsFull
		for (INT i = 0; i < 4; ++i)
			for (INT j = 0; j < 4; ++j)
				b.SetCell(j, i, L'ア');
		assert(b.NonSpaceCount() == 16);
		assert(b.IsFull());
		b.SetCell(3, 3, ZEN_SPACE);
		assert(!b.IsFull());
	}

	/////////////////////////////////////////////////////////////////////////
	// MBoard: コピー・ムーブ・所有権
	{
		MBoard a(4, 4, XG_STANDARD_RULES);
		a.SetCell(1, 1, ZEN_BLACK);

		MBoard b(a); // コピー構築
		assert(b.m_ptr && b.m_ptr != a.m_ptr);
		assert(b.GetCell0(1, 1) == ZEN_BLACK);
		assert(b.Rules() == a.Rules());
		assert(b.NonSpaceCount() == 1);
		b.SetCell(2, 2, ZEN_BLACK);
		assert(a.GetCell0(2, 2) == ZEN_SPACE); // 独立している

		MBoard c(3, 3, 0);
		c = a; // コピー代入
		assert(c.Columns() == 4 && c.Rows() == 4);
		assert(c.GetCell0(1, 1) == ZEN_BLACK);
		assert(c.m_ptr != a.m_ptr);
		c = c; // 自己代入
		assert(c.GetCell0(1, 1) == ZEN_BLACK);

		MBoard d(std::move(b)); // ムーブ構築
		assert(d.GetCell0(2, 2) == ZEN_BLACK);
		assert(b.m_ptr == NULL && b.m_allocated == NULL);

		MBoard e(3, 3, 0);
		e = std::move(d); // ムーブ代入
		assert(e.Columns() == 4);
		assert(e.GetCell0(2, 2) == ZEN_BLACK);
		assert(d.m_ptr == NULL && d.m_allocated == NULL);

		// 非所有（ラッパー）
		MBod* raw = MBod::create(3, 3, 0);
		{
			MBoard w(raw);
			assert(w.m_ptr == raw && w.m_allocated == NULL);
			w.SetCell(0, 0, L'ア');
			assert(w.GetAt(0) == L'ア');
			w.Close();
			assert(w.m_ptr == NULL);
		}
		assert(raw->m_magic == BOD_MAGIC); // 解放されていない
		delete raw;

		// 所有
		{
			MBoard o(MBod::create(3, 3, 0), TRUE);
			assert(o.m_ptr == o.m_allocated && o.m_ptr != NULL);
		}

		// Close の二重呼び出し
		MBoard f(3, 3, 0);
		f.Close();
		f.Close();
		assert(f.m_ptr == NULL);
	}

	/////////////////////////////////////////////////////////////////////////
	// 判定: CornerBlack
	{
		const MBoardPos corners[] = { MBoardPos(0, 0), MBoardPos(3, 0), MBoardPos(0, 3), MBoardPos(3, 3) };
		for (const MBoardPos& p : corners) {
			MBoard b(4, 4, 0);
			assert(!b.CornerBlack());
			put(b, { p });
			assert(b.CornerBlack());
		}
		MBoard b(4, 4, 0);
		put(b, { MBoardPos(1, 1), MBoardPos(2, 0), MBoardPos(0, 2), MBoardPos(3, 1) });
		assert(!b.CornerBlack());
	}

	// 判定: DoubleBlack
	{
		MBoard b(4, 4, 0);
		assert(!b.DoubleBlack());
		put(b, { MBoardPos(1, 1), MBoardPos(2, 2) }); // 斜めは隣り合いではない
		assert(!b.DoubleBlack());

		MBoard h(4, 4, 0);
		put(h, { MBoardPos(1, 1), MBoardPos(2, 1) });
		assert(h.DoubleBlack());

		MBoard v(4, 4, 0);
		put(v, { MBoardPos(1, 1), MBoardPos(1, 2) });
		assert(v.DoubleBlack());

		MBoard he(4, 4, 0); // 右下端
		put(he, { MBoardPos(2, 3), MBoardPos(3, 3) });
		assert(he.DoubleBlack());

		MBoard ve(4, 4, 0);
		put(ve, { MBoardPos(3, 2), MBoardPos(3, 3) });
		assert(ve.DoubleBlack());

		MBoard hs(4, 4, 0); // 行をまたぐ並びは隣り合いではない
		put(hs, { MBoardPos(3, 0), MBoardPos(0, 1) });
		assert(!hs.DoubleBlack());
	}

	// 判定: TriBlackAround（内部マスのみ対象）
	{
		MBoard b(4, 4, 0);
		assert(!b.TriBlackAround());
		put(b, { MBoardPos(1, 0), MBoardPos(0, 1) }); // 2方向のみ
		assert(!b.TriBlackAround());
		put(b, { MBoardPos(2, 1) }); // (1,1)が3方向
		assert(b.TriBlackAround());

		MBoard all(4, 4, 0); // 4方向
		put(all, { MBoardPos(1, 0), MBoardPos(0, 1), MBoardPos(2, 1), MBoardPos(1, 2) });
		assert(all.TriBlackAround());

		MBoard other(4, 4, 0); // 右下の内部マス(2,2)
		put(other, { MBoardPos(2, 1), MBoardPos(1, 2), MBoardPos(3, 2) });
		assert(other.TriBlackAround());

		// 仕様: 外周マスは判定対象外
		MBoard edge(4, 4, 0);
		put(edge, { MBoardPos(0, 0), MBoardPos(0, 2), MBoardPos(1, 1) }); // (0,1)が3方向
		assert(!edge.TriBlackAround());
	}

	// 判定: DividedByBlack
	{
		MBoard b(4, 4, 0);
		assert(!b.DividedByBlack()); // 空盤面
		put(b, { MBoardPos(1, 1) });
		assert(!b.DividedByBlack()); // 黒マス1個

		MBoard col(4, 4, 0); // 黒の縦線で分断
		put(col, { MBoardPos(1, 0), MBoardPos(1, 1), MBoardPos(1, 2), MBoardPos(1, 3) });
		assert(col.DividedByBlack());

		MBoard row(4, 4, 0); // 黒の横線で分断
		put(row, { MBoardPos(0, 2), MBoardPos(1, 2), MBoardPos(2, 2), MBoardPos(3, 2) });
		assert(row.DividedByBlack());

		MBoard gap(4, 4, 0); // 隙間があれば分断されない
		put(gap, { MBoardPos(1, 0), MBoardPos(1, 1), MBoardPos(1, 2) });
		assert(!gap.DividedByBlack());

		MBoard corner(4, 4, 0); // 隅の孤立
		put(corner, { MBoardPos(1, 0), MBoardPos(0, 1) });
		assert(corner.DividedByBlack());

		MBoard start(4, 4, 0); // (0,0)が黒でも正しく動く
		put(start, { MBoardPos(0, 0), MBoardPos(1, 0), MBoardPos(0, 1) });
		assert(!start.DividedByBlack());

		MBoard full(4, 4, 0); // 全部黒
		for (INT i = 0; i < 16; ++i)
			full.SetAt(i, ZEN_BLACK);
		assert(!full.DividedByBlack());
	}

	// 判定: ThreeDiagonals / FourDiagonals
	{
		MBoard none(4, 4, 0);
		assert(!none.ThreeDiagonals());
		assert(!none.FourDiagonals());

		MBoard two(4, 4, 0);
		put(two, { MBoardPos(0, 0), MBoardPos(1, 1) });
		assert(!two.ThreeDiagonals());

		MBoard d3(4, 4, 0); // ＼
		put(d3, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2) });
		assert(d3.ThreeDiagonals());
		assert(!d3.FourDiagonals());

		MBoard d3b(4, 4, 0); // ＼（右下寄り）
		put(d3b, { MBoardPos(1, 1), MBoardPos(2, 2), MBoardPos(3, 3) });
		assert(d3b.ThreeDiagonals());

		MBoard a3(4, 4, 0); // ／
		put(a3, { MBoardPos(3, 0), MBoardPos(2, 1), MBoardPos(1, 2) });
		assert(a3.ThreeDiagonals());
		assert(!a3.FourDiagonals());

		MBoard a3b(4, 4, 0); // ／（左下寄り）
		put(a3b, { MBoardPos(3, 1), MBoardPos(2, 2), MBoardPos(1, 3) });
		assert(a3b.ThreeDiagonals());

		MBoard d4(4, 4, 0);
		put(d4, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2), MBoardPos(3, 3) });
		assert(d4.ThreeDiagonals());
		assert(d4.FourDiagonals());

		MBoard a4(4, 4, 0);
		put(a4, { MBoardPos(3, 0), MBoardPos(2, 1), MBoardPos(1, 2), MBoardPos(0, 3) });
		assert(a4.ThreeDiagonals());
		assert(a4.FourDiagonals());

		MBoard broken(4, 4, 0); // 途切れた斜め
		put(broken, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(3, 3) });
		assert(!broken.ThreeDiagonals());
		assert(!broken.FourDiagonals());

		MBoard small3(3, 3, 0); // 3x3でも三連は判定可能
		put(small3, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2) });
		assert(small3.ThreeDiagonals());
		assert(!small3.FourDiagonals());
	}

	// 判定: 対称性
	{
		MBoard e(4, 4, 0); // 空は全部対称
		assert(e.IsPointSymmetry());
		assert(e.IsLineSymmetryV());
		assert(e.IsLineSymmetryH());

		MBoard p(4, 4, 0);
		put(p, { MBoardPos(0, 0) });
		assert(!p.IsPointSymmetry());
		put(p, { MBoardPos(3, 3) });
		assert(p.IsPointSymmetry());
		assert(!p.IsLineSymmetryV());
		assert(!p.IsLineSymmetryH());

		MBoard v(4, 4, 0); // 上下対称
		put(v, { MBoardPos(0, 0), MBoardPos(0, 3) });
		assert(v.IsLineSymmetryV());
		assert(!v.IsLineSymmetryH());
		assert(!v.IsPointSymmetry());

		MBoard h(4, 4, 0); // 左右対称
		put(h, { MBoardPos(0, 0), MBoardPos(3, 0) });
		assert(h.IsLineSymmetryH());
		assert(!h.IsLineSymmetryV());
		assert(!h.IsPointSymmetry());

		MBoard odd(5, 5, 0); // 奇数サイズの中央
		put(odd, { MBoardPos(2, 2) });
		assert(odd.IsPointSymmetry());
		assert(odd.IsLineSymmetryV());
		assert(odd.IsLineSymmetryH());

		MBoard odd2(5, 5, 0);
		put(odd2, { MBoardPos(2, 0), MBoardPos(2, 4) });
		assert(odd2.IsLineSymmetryV());
		assert(odd2.IsLineSymmetryH());
		assert(odd2.IsPointSymmetry());

		// 黒マス以外の文字は区別しない
		MBoard letters(4, 4, 0);
		letters.SetCell(0, 0, L'ア');
		assert(letters.IsPointSymmetry());
	}

	/////////////////////////////////////////////////////////////////////////
	// AreBlacksOK(rules) / AreBlacksOK() / MaybeOK
	{
		// ルール0なら常に有効。
		MBoard any(4, 4, 0);
		put(any, { MBoardPos(0, 0), MBoardPos(1, 0), MBoardPos(1, 1) });
		assert(any.AreBlacksOK(0));
		assert(any.AreBlacksOK());

		// 四隅黒禁
		MBoard c(4, 4, 0);
		put(c, { MBoardPos(0, 0) });
		assert(!c.AreBlacksOK(RULE_DONTCORNERBLACK));
		assert(c.AreBlacksOK(RULE_DONTDOUBLEBLACK));
		assert(c.AreBlacksOK(0));

		// 連黒禁
		MBoard d(4, 4, 0);
		put(d, { MBoardPos(1, 1), MBoardPos(2, 1) });
		assert(!d.AreBlacksOK(RULE_DONTDOUBLEBLACK));
		assert(d.AreBlacksOK(RULE_DONTCORNERBLACK));

		// 三方黒禁
		MBoard t(4, 4, 0);
		put(t, { MBoardPos(1, 0), MBoardPos(0, 1), MBoardPos(2, 1) });
		assert(!t.AreBlacksOK(RULE_DONTTRIDIRECTIONS));
		assert(t.AreBlacksOK(RULE_DONTCORNERBLACK | RULE_DONTDOUBLEBLACK));

		// 分断禁
		MBoard dv(4, 4, 0);
		put(dv, { MBoardPos(1, 0), MBoardPos(1, 1), MBoardPos(1, 2), MBoardPos(1, 3) });
		assert(!dv.AreBlacksOK(RULE_DONTDIVIDE));
		assert(dv.AreBlacksOK(0));

		// 黒斜三連禁
		MBoard d3(4, 4, 0);
		put(d3, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2) });
		assert(!d3.AreBlacksOK(RULE_DONTTHREEDIAGONALS));
		assert(d3.AreBlacksOK(RULE_DONTFOURDIAGONALS)); // 四連はない
		assert(!d3.AreBlacksOK(RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS));

		// 黒斜四連禁
		MBoard d4(4, 4, 0);
		put(d4, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2), MBoardPos(3, 3) });
		assert(!d4.AreBlacksOK(RULE_DONTFOURDIAGONALS));
		assert(!d4.AreBlacksOK(RULE_DONTTHREEDIAGONALS));

		// 点対称ルール
		MBoard ps(4, 4, 0);
		assert(ps.AreBlacksOK(RULE_POINTSYMMETRY)); // 空は対称
		put(ps, { MBoardPos(1, 0) });
		assert(!ps.AreBlacksOK(RULE_POINTSYMMETRY));
		assert(ps.AreBlacksOK(0));
		put(ps, { MBoardPos(2, 3) });
		assert(ps.AreBlacksOK(RULE_POINTSYMMETRY));

		// 線対称（タテ）ルール
		MBoard lv(4, 4, 0);
		put(lv, { MBoardPos(1, 0) });
		assert(!lv.AreBlacksOK(RULE_LINESYMMETRYV));
		put(lv, { MBoardPos(1, 3) });
		assert(lv.AreBlacksOK(RULE_LINESYMMETRYV));
		assert(!lv.AreBlacksOK(RULE_LINESYMMETRYH)); // (1,0)の左右対称がない
		assert(!lv.AreBlacksOK(RULE_LINESYMMETRYV | RULE_LINESYMMETRYH));

		// 線対称（ヨコ）ルール
		MBoard lh(4, 4, 0);
		put(lh, { MBoardPos(0, 1) });
		assert(!lh.AreBlacksOK(RULE_LINESYMMETRYH));
		put(lh, { MBoardPos(3, 1) });
		assert(lh.AreBlacksOK(RULE_LINESYMMETRYH));
		assert(!lh.AreBlacksOK(RULE_LINESYMMETRYV));

		// 標準ルール（点対称を含む）では非対称な盤面は不正
		MBoard sym(4, 4, XG_STANDARD_RULES);
		put(sym, { MBoardPos(1, 0) });
		assert(!sym.IsPointSymmetry());
		assert(!sym.AreBlacksOK());
		put(sym, { MBoardPos(2, 3) });
		assert(sym.AreBlacksOK());

		// AreBlacksOK() はボードのルールを使う
		MBoard s(4, 4, XG_STANDARD_RULES);
		assert(s.AreBlacksOK());
		put(s, { MBoardPos(0, 0) });
		assert(!s.AreBlacksOK());
		s.Rules() = 0;
		assert(s.AreBlacksOK());

		// MaybeOK：局所ルール（四隅・連黒・三方・斜め）は MaybeCanPutBlack に任せ、
		// 大域ルール（分断・対称性）だけを判定する。
		const INT rules_nosym = XG_STANDARD_RULES & ~RULE_POINTSYMMETRY;

		MBoard m(4, 4, rules_nosym);
		put(m, { MBoardPos(0, 0) }); // 四隅黒禁は無視される
		assert(m.MaybeOK());
		assert(!m.AreBlacksOK());
		put(m, { MBoardPos(1, 0) }); // 連黒禁は無視される
		assert(m.MaybeOK());
		assert(!m.AreBlacksOK());

		MBoard m2(4, 4, rules_nosym);
		put(m2, { MBoardPos(1, 0), MBoardPos(1, 1), MBoardPos(1, 2), MBoardPos(1, 3) }); // 分断は判定する
		assert(!m2.MaybeOK());

		MBoard m3(5, 5, rules_nosym);
		put(m3, { MBoardPos(2, 1), MBoardPos(1, 2), MBoardPos(3, 2) }); // 三方黒禁は無視される（(2,2)が三方囲み）
		assert(m3.MaybeOK());
		assert(!m3.AreBlacksOK());

		MBoard m4(4, 4, rules_nosym);
		put(m4, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2) }); // 斜三連禁は無視される
		assert(m4.MaybeOK());
		assert(!m4.AreBlacksOK());

		MBoard m4b(4, 4, RULE_DONTFOURDIAGONALS);
		put(m4b, { MBoardPos(0, 0), MBoardPos(1, 1), MBoardPos(2, 2), MBoardPos(3, 3) }); // 斜四連禁も無視される
		assert(m4b.MaybeOK());
		assert(!m4b.AreBlacksOK());

		// 対称性は MaybeOK でも判定される
		MBoard m5(4, 4, RULE_POINTSYMMETRY);
		put(m5, { MBoardPos(1, 0) });
		assert(!m5.MaybeOK());
		put(m5, { MBoardPos(2, 3) });
		assert(m5.MaybeOK());

		MBoard m6(4, 4, RULE_LINESYMMETRYV);
		put(m6, { MBoardPos(1, 0) });
		assert(!m6.MaybeOK());
		put(m6, { MBoardPos(1, 3) });
		assert(m6.MaybeOK());

		MBoard m7(4, 4, RULE_LINESYMMETRYH);
		put(m7, { MBoardPos(0, 1) });
		assert(!m7.MaybeOK());
		put(m7, { MBoardPos(3, 1) });
		assert(m7.MaybeOK());
	}

	/////////////////////////////////////////////////////////////////////////
	// MaybeCanPutBlack
	{
		MBoard b(4, 4, XG_STANDARD_RULES);
		// 四隅は置けない
		assert(!b.MaybeCanPutBlack(0, 0));
		assert(!b.MaybeCanPutBlack(3, 0));
		assert(!b.MaybeCanPutBlack(0, 3));
		assert(!b.MaybeCanPutBlack(3, 3));
		// 通常マスは置ける
		assert(b.MaybeCanPutBlack(1, 1));
		assert(b.MaybeCanPutBlack(1, 0));
		// 隣に黒があると置けない
		put(b, { MBoardPos(1, 1) });
		assert(!b.MaybeCanPutBlack(2, 1));
		assert(!b.MaybeCanPutBlack(0, 1));
		assert(!b.MaybeCanPutBlack(1, 0));
		assert(!b.MaybeCanPutBlack(1, 2));
		assert(b.MaybeCanPutBlack(2, 2)); // 斜めはOK
		assert(b.MaybeCanPutBlack(3, 1));

		// ルール0なら制限なし
		MBoard free(4, 4, 0);
		put(free, { MBoardPos(1, 1) });
		assert(free.MaybeCanPutBlack(0, 0));
		assert(free.MaybeCanPutBlack(2, 1));

		// 四隅黒禁のみ
		MBoard only_corner(4, 4, RULE_DONTCORNERBLACK);
		put(only_corner, { MBoardPos(1, 1) });
		assert(!only_corner.MaybeCanPutBlack(3, 3));
		assert(only_corner.MaybeCanPutBlack(2, 1));

		// 連黒禁のみ
		MBoard only_double(4, 4, RULE_DONTDOUBLEBLACK);
		put(only_double, { MBoardPos(1, 1) });
		assert(only_double.MaybeCanPutBlack(3, 3));
		assert(!only_double.MaybeCanPutBlack(2, 1));

		// 盤面外の隣は黒扱いにしない（辺の黒マスで誤判定しない）
		MBoard edge(4, 4, RULE_DONTDOUBLEBLACK);
		assert(edge.MaybeCanPutBlack(0, 1));
		assert(edge.MaybeCanPutBlack(3, 2));

		// 範囲外は置けない。
		assert(!b.MaybeCanPutBlack(4, 0));
		assert(!b.MaybeCanPutBlack(-1, 0));
		assert(!b.MaybeCanPutBlack(0, 4));

		// 対称位置同士が隣り合う場合（ヨコ対称で (1,1) と (2,1)）。
		MBoard mh(4, 4, RULE_DONTDOUBLEBLACK | RULE_LINESYMMETRYH);
		assert(!mh.MaybeCanPutBlack(1, 1));
		assert(mh.MaybeCanPutBlack(0, 1)); // (0,1) と (3,1) は離れている
		MBoard mh0(4, 4, RULE_DONTDOUBLEBLACK);
		assert(mh0.MaybeCanPutBlack(1, 1)); // 対称なしならOK

		// 対称位置が既存の黒マスの隣になる場合（点対称）。
		MBoard mp(5, 5, RULE_DONTDOUBLEBLACK | RULE_POINTSYMMETRY);
		put(mp, { MBoardPos(3, 3) });
		assert(!mp.MaybeCanPutBlack(1, 2)); // 対称位置 (3,2) が (3,3) の隣
		assert(mp.MaybeCanPutBlack(1, 1));  // 対称位置 (3,3) は既に黒。隣は黒でない

		// 三方黒禁。(1,0),(0,1) が黒のとき、(2,1) を置くと (1,1) が三方囲み。
		MBoard tri(5, 5, RULE_DONTTRIDIRECTIONS);
		put(tri, { MBoardPos(1, 0), MBoardPos(0, 1) });
		assert(!tri.MaybeCanPutBlack(2, 1));
		assert(tri.MaybeCanPutBlack(3, 3));
		MBoard tri0(5, 5, 0);
		put(tri0, { MBoardPos(1, 0), MBoardPos(0, 1) });
		assert(tri0.MaybeCanPutBlack(2, 1));

		// 斜め三連禁。対称位置を含めて三連になる場合。
		MBoard dg(5, 5, RULE_DONTTHREEDIAGONALS | RULE_POINTSYMMETRY);
		put(dg, { MBoardPos(2, 2) });
		assert(!dg.MaybeCanPutBlack(1, 1)); // (1,1),(2,2),(3,3)
		MBoard dg0(5, 5, RULE_DONTTHREEDIAGONALS);
		put(dg0, { MBoardPos(2, 2) });
		assert(dg0.MaybeCanPutBlack(1, 1)); // 対称なしなら二連だけ

		// 斜め四連禁（三連禁なし）：三連は許可、四連は不可。
		MBoard d4(5, 5, RULE_DONTFOURDIAGONALS);
		put(d4, { MBoardPos(0, 0), MBoardPos(1, 1) });
		assert(d4.MaybeCanPutBlack(2, 2));
		put(d4, { MBoardPos(2, 2) });
		assert(!d4.MaybeCanPutBlack(3, 3));
		assert(d4.MaybeCanPutBlack(4, 4)); // 途切れている
	}

	/////////////////////////////////////////////////////////////////////////
	// MirroredSetCell
	{
		// ルール0：対称化しない。
		MBoard n(4, 4, 0);
		n.MirroredSetCell(1, 0, ZEN_BLACK);
		assert(n.GetCell0(1, 0) == ZEN_BLACK);
		assert(n.NonSpaceCount() == 1);

		// 点対称
		MBoard p(5, 5, RULE_POINTSYMMETRY);
		p.MirroredSetCell(1, 0, ZEN_BLACK);
		assert(p.GetCell0(1, 0) == ZEN_BLACK);
		assert(p.GetCell0(3, 4) == ZEN_BLACK);
		assert(p.NonSpaceCount() == 2);
		assert(p.IsPointSymmetry());
		p.MirroredSetCell(2, 2, ZEN_BLACK); // 中央は1マスだけ
		assert(p.NonSpaceCount() == 3);
		assert(p.IsPointSymmetry());
		p.MirroredSetCell(1, 0, ZEN_SPACE); // 消すと対称位置も消える
		assert(p.GetCell0(3, 4) == ZEN_SPACE);
		assert(p.NonSpaceCount() == 1);

		// 線対称（タテ）：上下
		MBoard v(4, 4, RULE_LINESYMMETRYV);
		v.MirroredSetCell(1, 0, ZEN_BLACK);
		assert(v.GetCell0(1, 3) == ZEN_BLACK);
		assert(v.NonSpaceCount() == 2);
		assert(v.IsLineSymmetryV());
		assert(!v.IsLineSymmetryH());

		// 線対称（ヨコ）：左右
		MBoard h(4, 4, RULE_LINESYMMETRYH);
		h.MirroredSetCell(0, 1, ZEN_BLACK);
		assert(h.GetCell0(3, 1) == ZEN_BLACK);
		assert(h.NonSpaceCount() == 2);
		assert(h.IsLineSymmetryH());
		assert(!h.IsLineSymmetryV());

		// タテ・ヨコの両方：4マス
		MBoard vh(4, 4, RULE_LINESYMMETRYV | RULE_LINESYMMETRYH);
		vh.MirroredSetCell(1, 0, ZEN_BLACK);
		assert(vh.GetCell0(1, 0) == ZEN_BLACK);
		assert(vh.GetCell0(2, 0) == ZEN_BLACK);
		assert(vh.GetCell0(1, 3) == ZEN_BLACK);
		assert(vh.GetCell0(2, 3) == ZEN_BLACK);
		assert(vh.NonSpaceCount() == 4);
		assert(vh.IsLineSymmetryV() && vh.IsLineSymmetryH() && vh.IsPointSymmetry());

		// 点対称＋タテ：ヨコの位置も含めて4マス
		MBoard pv(4, 4, RULE_POINTSYMMETRY | RULE_LINESYMMETRYV);
		pv.MirroredSetCell(1, 0, ZEN_BLACK);
		assert(pv.NonSpaceCount() == 4);
		assert(pv.IsLineSymmetryH());

		// 範囲外は何もしない。
		MBoard o(4, 4, RULE_POINTSYMMETRY);
		o.MirroredSetCell(4, 0, ZEN_BLACK);
		o.MirroredSetCell(-1, 0, ZEN_BLACK);
		o.MirroredSetCell(0, 4, ZEN_BLACK);
		assert(o.NonSpaceCount() == 0);
		assert(o.empty());

		// 標準ルールで置いた盤面は AreBlacksOK の点対称判定を通る。
		MBoard s(5, 5, XG_STANDARD_RULES);
		s.MirroredSetCell(1, 2, ZEN_BLACK);
		assert(s.IsPointSymmetry());
		assert(s.AreBlacksOK());
	}
}
