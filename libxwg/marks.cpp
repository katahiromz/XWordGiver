// marks.cpp --- 二重マス。
// Author: katahiromz
// License: MIT

#include "marks.h"
#include <random>
#include <set>
#include <cmath>
#include <numeric>

// 部分マルチセットかどうか？
static BOOL __fastcall
xg_submultiseteq(const std::unordered_multiset<WCHAR>& ms1, const std::unordered_multiset<WCHAR>& ms2) {
	for (auto& elem : ms1) {
		if (ms1.count(elem) > ms2.count(elem))
			return FALSE;
	}
	return TRUE;
}

// 水平線。
static MStringW XgGetHLine(void) {
	if (XgIsUserJapanese())
		return L"――――――――――――――";
	return L"----------------------------";
}

// マーク文字列を取得する。
void __fastcall XgGetStringOfMarks(const MBoardDisplay& display, MStringW& str) {
	WCHAR sz[64];
	str = XgGetHLine();
	str += L"\r\n";
	auto& board = display.m_solution;
	for (const auto& mark : display.m_vMarks) {
		StringCchPrintfW(sz, _countof(sz), L"(%d, %d)%c\r\n",
			             mark.m_i + 1, mark.m_j + 1,
			             board.GetCell0(mark.m_j, mark.m_i));
		str += sz;
	}
}

// マーク文字列を取得する2。
void __fastcall XgGetStringOfMarks2(const MBoardDisplay& display, MStringW& str) {
	WCHAR sz[64];
	str.clear();
	INT i = 0;
	auto& board = display.m_solution;
	for (const auto& mark : display.m_vMarks) {
		const WCHAR szLetter[2] = { board.GetCell0(mark.m_j, mark.m_i), 0 };
		auto letter = XgConvertString(szLetter, display.m_bLowercase, display.m_bHiragana);
		StringCchPrintfW(sz, _countof(sz), L"MARK%d. (%u, %u): %s\n",
					    i + 1, mark.m_j + 1, mark.m_i + 1, letter.c_str());
		str += sz;
		++i;
	}
}

// マークされているか（二重マス）？
INT __fastcall XgGetMarked(std::vector<MBoardPos>& vMarks, const MBoardPos& pos) noexcept {
	for (INT i = 0; i < INT(vMarks.size()); i++) {
		if (vMarks[i] == pos)
			return i;
	}
	return -1;
}
INT __fastcall XgGetMarked(std::vector<MBoardPos>& vMarks, INT j, INT i) noexcept {
	return XgGetMarked(vMarks, MBoardPos(j, i));
}

// マークされているか（二重マス）？
INT __fastcall XgGetMarked(MBoardDisplay& display, const MBoardPos& pos) noexcept {
	return XgGetMarked(display.m_vMarks, pos);
}

// マークされているか（二重マス）？
INT __fastcall XgGetMarked(MBoardDisplay& display, INT j, INT i) noexcept {
	return XgGetMarked(display, MBoardPos(j, i));
}

// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(std::vector<MBoardPos>& vMarks, const MBoardPos& pos) {
	for (auto& markpos : vMarks) {
		if (markpos == pos)
			return;
	}
	vMarks.emplace_back(pos);
}

// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(MBoardDisplay& display, const MBoardPos& pos) {
	XgSetMark(display.m_vMarks, pos);
}

// 指定のマスにマークする（二重マス）。
void __fastcall XgSetMark(MBoardDisplay& display, INT j, INT i) {
	XgSetMark(display, MBoardPos(j, i));
}

// 指定のマスのマーク（二重マス）を解除する。
void __fastcall XgDeleteMark(MBoardDisplay& display, INT j, INT i) {
	// (i, j)がマークされていなければ無視。
	const INT nMarked = XgGetMarked(display, j, i);
	if (nMarked == -1)
		return;

	// 二重マスを消す。
	display.m_vMarks.erase(display.m_vMarks.begin() + nMarked);
}

// マーク文字列を設定する。
void __fastcall XgSetStringOfMarks(MBoardDisplay& display, LPCWSTR psz) {
	// 初期化する。
	display.m_vMarks.clear();

	// 区切り線があるか？
	auto str = XgGetHLine();
	psz = wcsstr(psz, str.c_str());
	if (psz == nullptr) {
		// 区切り線がなければ二重マスの情報はない。
		return;
	}
	psz += str.size();

	// マーク文字列を解析する。
	INT count = 0;
	while (*psz != UNICODE_NULL && *psz != ZEN_ULEFT) {
		// 1000文字以上はありえない。
		if (count++ > 1000)
			break;

		// 読み飛ばし。
		while (*psz == L' ' || *psz == L'\r' || *psz == L'\n' || *psz == L'(')
			psz++;
		if (*psz == UNICODE_NULL || *psz == ZEN_ULEFT)
			break;

		// 数字がなければ終了。
		if (!(L'0' <= *psz && *psz <= L'9'))
			break;

		// 行のインデックスを読み込む。
		const INT i = _wtoi(psz) - 1;
		while (L'0' <= *psz && *psz <= L'9')
			psz++;
		if (*psz == UNICODE_NULL || *psz == ZEN_ULEFT)
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
		if (*psz == UNICODE_NULL || *psz == ZEN_ULEFT)
			break;

		// マークを設定する。
		XgSetMark(display, j, i);
	}
}

// 二重マス単語を取得する。
bool __fastcall XgGetMarkWord(MBoardDisplay& display, MStringW& str) {
	// 初期化する。
	str.clear();

	// 二重マス単語があるか？
	if (display.m_vMarks.empty())
		return FALSE;

	BOOL bExists = TRUE;

	// 二重マス単語を構築する。
	for (const auto& mark : display.m_vMarks) {
		const WCHAR ch = display.m_solution.GetCell0(mark.m_j, mark.m_i);
		// 空白マスか黒マスであれば、二重マス単語はない。
		if (ch == ZEN_SPACE || ch == ZEN_BLACK)
			bExists = FALSE;
		str += ch;
	}
	return bExists;	 // 存在するか？
}

// 二重マス単語候補を取得する。
bool __fastcall XgGetMarkedCandidates(MBoardDisplay& display, MStringListW& cands, const MDict& dict) {
	// 初期化する。
	cands.clear();

	auto data = display.m_solution.Data();
	auto& board = display.m_solution;
	const INT nColumns = board.Columns(), nRows = board.Rows();

	// マスの文字をマルチセットに変換。
	std::unordered_multiset<WCHAR> msCells;
	msCells.insert(data.begin(), data.end());

	// dictに登録されている単語について繰り返す。
	for (const auto& pair : dict.m_word2clue) {
		// 単語を取り出す。2文字以下は無視。
		auto& word = pair.first;
		if (word.size() <= 2)
			continue;

		// 単語をマルチセットへ変換。
		std::unordered_multiset<WCHAR> ms(word.begin(), word.end());

		// 部分マルチセットになっているか？
		if (xg_submultiseteq(ms, msCells)) {
			// 配置を調べる。
			std::vector<MBoardPos> vPos;
			for (auto wch : word) {
				for (INT i = 0; i < nRows; ++i) {
					for (INT j = 0; j < nColumns; ++j) {
						if (wch == board.GetCell0(j, i) && XgGetMarked(vPos, j, i) == -1) {
							vPos.emplace_back(j, i);
							goto break2;
						}
					}
				}
break2:;
			}

			// 隣り合う配置があれば失敗。
			const INT size = INT(vPos.size());
			for (INT i = 0; i < size - 1; i++) {
				for (INT j = i + 1; j < size; j++) {
					if (vPos[i].m_i == vPos[j].m_i) {
						if (vPos[i].m_j + 1 == vPos[j].m_j || vPos[i].m_j == vPos[j].m_j + 1) {
							goto failed;
						}
					}
					if (vPos[i].m_j == vPos[j].m_j) {
						if (vPos[i].m_i + 1 == vPos[j].m_i || vPos[i].m_i == vPos[j].m_i + 1) {
							goto failed;
						}
					}
				}
			}
			cands.emplace_back(word);
failed:;
		}
	}

	// ソート・一意化する。
	std::sort(cands.begin(), cands.end(),
		[](const MStringW& x, const MStringW& y) noexcept {
			if (x.size() > y.size())
				return true;
			if (x.size() < y.size())
				return false;
			return (x < y);
		}
	);
	cands.erase(std::unique(cands.begin(), cands.end()), cands.end());

	return !cands.empty();
}

// 二重マス単語を設定する。
BOOL __fastcall XgSetMarkedWord(MBoardDisplay& display, const MStringW& str, WCHAR *pchNotFound) {
	auto marks = display.m_vMarks;
	// 初期化する。
	display.m_vMarks.clear();

	auto& board = display.m_solution;
	const INT nColumns = board.Columns(), nRows = board.Rows();

	// 二重マス単語と文字マスの情報に従って二重マスを設定する。
	MStringW word;
	for (const auto ch : str) {
		const INT m = rand() % nRows;
		const INT n = rand() % nColumns;
		for (INT i = 0; i < nRows; i++) {
			for (INT j = 0; j < nColumns; j++) {
				const INT j0 = (j + n) % nColumns;
				const INT i0 = (i + m) % nRows;
				if (ch == board.GetCell0(j0, i0) && XgGetMarked(display, j0, i0) == -1) {
					word.push_back(ch);
					XgSetMark(display, j0, i0);
					goto break2;
				}
			}
		}
		if (pchNotFound) {
			*pchNotFound = ch;
			break;
		}
break2:;
	}

	if (str != word) {
		display.m_vMarks = std::move(marks);
		return FALSE;
	}

	return TRUE;
}

// 配置のスコア(小さいほど良い)。
static double __fastcall XgScoreMarkLayout(const MBoard& board, const MMarkLayout& layout) {
	const INT nColumns = board.Columns(), nRows = board.Rows();

	// 全マスから最寄りの二重マスまでの距離の平均(盤面を満遍なくカバーしているほど小さい)。
	double sum = 0;
	INT count = 0;
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nColumns; ++j) {
			if (board.GetCell0(j, i) == ZEN_BLACK)
				continue;
			double best = 1e18;
			for (const auto& p : layout) {
				const double dx = p.m_j - j, dy = p.m_i - i;
				best = __min(best, dx * dx + dy * dy);
			}
			sum += std::sqrt(best);
			++count;
		}
	}
	double score = count ? (sum / count) : 0.0;

	// 二重マス同士の最小距離(大きいほど良い)と、隣接ペナルティ。
	double minDist = 1e18;
	for (size_t a = 0; a + 1 < layout.size(); ++a) {
		for (size_t b = a + 1; b < layout.size(); ++b) {
			const double dx = layout[a].m_j - layout[b].m_j;
			const double dy = layout[a].m_i - layout[b].m_i;
			minDist = __min(minDist, std::sqrt(dx * dx + dy * dy));
			if (std::abs(dx) + std::abs(dy) == 1)
				score += 100.0;
		}
	}
	if (layout.size() >= 2)
		score -= 0.2 * minDist;
	return score;
}

// 二重マスの配置候補を生成する。
bool __fastcall XgGenerateMarkLayouts(const MBoardDisplay& display, const MStringW& word,
                                      std::vector<MMarkLayout>& layouts,
                                      WCHAR *pchNotFound, size_t nMax)
{
	layouts.clear();
	if (word.empty())
		return false;

	auto& board = display.m_solution;
	const INT nColumns = board.Columns(), nRows = board.Rows();

	// 文字ごとの配置可能マスを集める。
	std::unordered_map<WCHAR, std::vector<MBoardPos>> cellsOf;
	for (INT i = 0; i < nRows; ++i) {
		for (INT j = 0; j < nColumns; ++j) {
			const WCHAR ch = board.GetCell0(j, i);
			if (ch == ZEN_BLACK || ch == ZEN_SPACE)
				continue;
			cellsOf[ch].emplace_back(j, i);
		}
	}

	// 必要数を満たすか確認する。
	std::unordered_map<WCHAR, size_t> need;
	for (const WCHAR ch : word)
		++need[ch];
	for (const WCHAR ch : word) {
		auto it = cellsOf.find(ch);
		if (it == cellsOf.end() || it->second.size() < need[ch]) {
			if (pchNotFound)
				*pchNotFound = ch;
			return false;
		}
	}

	std::mt19937 rng((unsigned)std::random_device()());
	std::uniform_real_distribution<double> uni(0.0, 1.0);

	const size_t len = word.size();
	const INT nTrials = 2000;
	std::set<std::vector<INT>> seen;
	std::vector<std::pair<double, MMarkLayout>> scored;

	std::vector<size_t> order(len);
	for (INT t = 0; t < nTrials; ++t) {
		const bool bFarthest = (t % 2 == 1); // 偶数回はランダム、奇数回は最遠点法。
		MMarkLayout layout(len);
		std::vector<MBoardPos> placed;
		std::vector<char> used((size_t)nColumns * nRows, 0);

		std::iota(order.begin(), order.end(), (size_t)0);
		std::shuffle(order.begin(), order.end(), rng);

		for (size_t k : order) {
			// 未使用の候補マス。
			std::vector<const MBoardPos *> cands;
			for (const auto& p : cellsOf[word[k]]) {
				if (!used[(size_t)p.m_i * nColumns + p.m_j])
					cands.push_back(&p);
			}
			const MBoardPos *pick = nullptr;
			if (!bFarthest || placed.empty()) {
				pick = cands[(size_t)(uni(rng) * cands.size()) % cands.size()];
			} else {
				double bestVal = -1;
				for (auto *p : cands) {
					double d = 1e18;
					for (const auto& q : placed) {
						const double dx = p->m_j - q.m_j, dy = p->m_i - q.m_i;
						d = __min(d, dx * dx + dy * dy);
					}
					d *= 0.7 + 0.6 * uni(rng); // 揺らぎを入れて多様性を出す。
					if (d > bestVal) {
						bestVal = d;
						pick = p;
					}
				}
			}
			layout[k] = *pick;
			placed.push_back(*pick);
			used[(size_t)pick->m_i * nColumns + pick->m_j] = 1;
		}

		// マスの集合が同じ配置は重複とみなす。
		std::vector<INT> key;
		for (const auto& p : layout)
			key.push_back(p.m_i * nColumns + p.m_j);
		std::sort(key.begin(), key.end());
		if (!seen.insert(std::move(key)).second)
			continue;

		scored.emplace_back(XgScoreMarkLayout(board, layout), std::move(layout));
	}

	// 分散の良い順に並べて上位を残す。
	std::stable_sort(scored.begin(), scored.end(),
		[](const auto& a, const auto& b) { return a.first < b.first; });
	if (scored.size() > nMax)
		scored.resize(nMax);
	for (auto& item : scored)
		layouts.emplace_back(std::move(item.second));

	return !layouts.empty();
}
