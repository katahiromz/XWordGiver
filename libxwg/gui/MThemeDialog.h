// MThemeDialog.h --- 「テーマ」ダイアログ
// Author: katahiromz
// License: MIT

#pragma once

#include "xwg.h"
#include "MWindowBase.h"
#include "dict.h"

// タグ群の最大長。
#define XG_MAX_TAGSLEN 256

// 「テーマ」ダイアログ。
class MThemeDialog : public MDialogBase {
public:
	MStringW m_theme_tags;
	const MStringW& m_default_theme_tags;
	std::unordered_set<MStringW> m_priority_tags;
	std::unordered_set<MStringW> m_forbidden_tags;
	std::unordered_map<MStringW, size_t>& m_tag_histgram;
	// 出現回数の逆順でソート済みのヒストグラム。
	std::vector<std::pair<size_t, MStringW>> m_histgram_sorted;

	MThemeDialog(MDict& dict)
		: m_theme_tags(dict.m_theme_tags)
		, m_default_theme_tags(dict.m_default_theme_tags)
		, m_priority_tags(dict.m_priority_tags)
		, m_forbidden_tags(dict.m_forbidden_tags)
		, m_tag_histgram(dict.m_tag_histgram)
	{
	}

	// タグリストボックスを初期化。
	void XgInitTagListView(HWND hwndLV) {
		constexpr auto exstyle = LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_GRIDLINES;
		ListView_SetExtendedListViewStyleEx(hwndLV, exstyle, exstyle);

		LV_COLUMN column = { LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT };

		auto tags = XgLoadString(IDS_TAGS);
		column.pszText = &tags[0];
		column.fmt = LVCFMT_LEFT;
		column.cx = 90;
		column.iSubItem = 0;
		ListView_InsertColumn(hwndLV, 0, &column);

		auto tagcount = XgLoadString(IDS_TAGCOUNT);
		column.pszText = &tagcount[0];
		column.fmt = LVCFMT_RIGHT;
		column.cx = 84;
		column.iSubItem = 1;
		ListView_InsertColumn(hwndLV, 1, &column);
	}

	// 「テーマ」ダイアログの初期化。
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// ダイアログを中央寄せする。
		CenterWindowDx(hwnd);

		// 長さを制限する。
		SendDlgItemMessageW(hwnd, cmb1, CB_LIMITTEXT, XG_MAX_TAGSLEN - 1, 0);

		// リストビューを初期化。
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		HWND hLst2 = GetDlgItem(hwnd, lst2);
		HWND hLst3 = GetDlgItem(hwnd, lst3);
		XgInitTagListView(hLst1);
		XgInitTagListView(hLst2);
		XgInitTagListView(hLst3);

		// ヒストグラムを取得し、出現回数の逆順でソート。
		m_histgram_sorted.clear();
		for (auto& pair : m_tag_histgram) {
			m_histgram_sorted.emplace_back(pair.second, pair.first);
		}
		std::sort(m_histgram_sorted.begin(), m_histgram_sorted.end(),
			[](const std::pair<size_t, MStringW>& a, const std::pair<size_t, MStringW>& b) {
				return a.first > b.first;
			}
		);

		// コンボボックスにテキストを設定する。
		SetDlgItemTextW(hwnd, cmb1, m_theme_tags.c_str());
		// プリセットを設定する(lst1も更新される)。
		SetPreset(hwnd, m_theme_tags.c_str());

		// THEME.txt ファイルを読み込む。
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		if (FILE *fp = _wfopen(XgFindLocalFile(L"THEME.txt").c_str(), L"rb")) {
			char buf[256];
			while (fgets(buf, _countof(buf), fp)) {
				MStringW str = WideFromUtf8(buf);
				XgStrTrim(str);
				if (str.empty())
					continue;
				if (str[0] == 0xFEFF || str[0] == L';')
					continue;
				ComboBox_AddString(hCmb1, str.c_str());
			}
			fclose(fp);
		}
		// コンボボックスの横幅。
		SendMessageW(hCmb1, CB_SETHORIZONTALEXTENT, 1000, 0);

		return TRUE;
	}

	// lst1 を「全タグ − (lst2 ∪ lst3)」で再構築する。
	// pszSelect が指定されていれば、そのタグを選択する。
	void UpdateAllTags(HWND hwnd, LPCWSTR pszSelect = nullptr) {
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		HWND hLst2 = GetDlgItem(hwnd, lst2);
		HWND hLst3 = GetDlgItem(hwnd, lst3);

		// lst2 と lst3 にあるタグを集める。
		std::unordered_set<MStringW> used;
		WCHAR szText[XG_MAX_TAGSLEN];
		for (HWND hLst : { hLst2, hLst3 }) {
			const INT cItems = ListView_GetItemCount(hLst);
			for (INT i = 0; i < cItems; ++i) {
				ListView_GetItemText(hLst, i, 0, szText, _countof(szText));
				used.insert(szText);
			}
		}

		// 現在の選択位置を覚えておく。
		INT iPrev = ListView_GetNextItem(hLst1, -1, LVNI_SELECTED);
		if (iPrev < 0)
			iPrev = 0;

		SendMessageW(hLst1, WM_SETREDRAW, FALSE, 0);
		ListView_DeleteAllItems(hLst1);

		INT iItem = 0, iSelect = -1;
		LV_ITEM item = { LVIF_TEXT };
		for (auto& pair : m_histgram_sorted) {
			if (used.count(pair.second))
				continue; // lst2 か lst3 にあるものは表示しない。

			StringCchCopyW(szText, _countof(szText), pair.second.c_str());
			item.iItem = iItem;
			item.iSubItem = 0;
			item.pszText = szText;
			ListView_InsertItem(hLst1, &item);

			WCHAR szCount[64];
			StringCchPrintfW(szCount, _countof(szCount), L"%u", (UINT)pair.first);
			item.iItem = iItem;
			item.iSubItem = 1;
			item.pszText = szCount;
			ListView_SetItem(hLst1, &item);

			if (pszSelect && pair.second == pszSelect)
				iSelect = iItem;
			++iItem;
		}

		// 選択を復元する。
		if (iSelect < 0 && iItem > 0)
			iSelect = (iPrev < iItem) ? iPrev : iItem - 1;
		if (iSelect >= 0) {
			constexpr auto state = LVIS_SELECTED | LVIS_FOCUSED;
			ListView_SetItemState(hLst1, iSelect, state, state);
			ListView_EnsureVisible(hLst1, iSelect, FALSE);
		}

		SendMessageW(hLst1, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(hLst1, nullptr, TRUE);
	}

	// 優先/除外タグの出現回数合計を更新する。
	void UpdateCounters(HWND hwnd) {
		const struct { int lst; int stc; } pairs[] = { { lst2, stc1 }, { lst3, stc2 } };
		WCHAR szText[64];
		for (auto& p : pairs) {
			HWND hLst = GetDlgItem(hwnd, p.lst);
			const INT cItems = ListView_GetItemCount(hLst);
			size_t count = 0;
			for (INT i = 0; i < cItems; ++i) {
				ListView_GetItemText(hLst, i, 1, szText, _countof(szText));
				count += _wtoi(szText);
			}
			SetDlgItemInt(hwnd, p.stc, INT(count), FALSE);
		}
	}

	// lst1 から選択タグを lst2 (優先) または lst3 (除外) へ移動する。
	void AddTag(HWND hwnd, BOOL bPriority) {
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		INT iItem = ListView_GetNextItem(hLst1, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem < 0)
			return; // 選択なし。

		WCHAR szText1[XG_MAX_TAGSLEN], szText2[64];
		ListView_GetItemText(hLst1, iItem, 0, szText1, _countof(szText1));
		ListView_GetItemText(hLst1, iItem, 1, szText2, _countof(szText2));

		HWND hDest = GetDlgItem(hwnd, bPriority ? lst2 : lst3);
		LV_FINDINFO find = { LVFI_STRING, szText1 };
		if (ListView_FindItem(hDest, -1, &find) < 0) {
			LV_ITEM item = { LVIF_TEXT };
			item.iItem = ListView_GetItemCount(hDest);
			item.iSubItem = 0;
			item.pszText = szText1;
			iItem = ListView_InsertItem(hDest, &item);
			item.iItem = iItem;
			item.iSubItem = 1;
			item.pszText = szText2;
			ListView_SetItem(hDest, &item);
		}

		UpdateCounters(hwnd);
		UpdateAllTags(hwnd);   // lst1 から取り除く
		UpdatePreset(hwnd);
	}

	// lst2 (優先) または lst3 (除外) の選択タグを lst1 へ戻す。
	void RemoveTag(HWND hwnd, BOOL bPriority) {
		HWND hSrc = GetDlgItem(hwnd, bPriority ? lst2 : lst3);
		INT iItem = ListView_GetNextItem(hSrc, -1, LVNI_ALL | LVNI_SELECTED);
		if (iItem < 0)
			return;

		WCHAR szText[XG_MAX_TAGSLEN];
		ListView_GetItemText(hSrc, iItem, 0, szText, _countof(szText));
		ListView_DeleteItem(hSrc, iItem);

		// 削除した位置の次の項目を選択しておく。
		const INT cItems = ListView_GetItemCount(hSrc);
		if (cItems > 0) {
			if (iItem >= cItems)
				iItem = cItems - 1;
			constexpr auto state = LVIS_SELECTED | LVIS_FOCUSED;
			ListView_SetItemState(hSrc, iItem, state, state);
		}

		UpdateCounters(hwnd);
		UpdateAllTags(hwnd, szText);   // lst1 に戻して選択
		UpdatePreset(hwnd);
	}

	// 「テーマ」ダイアログで「OK」ボタンが押された。
	BOOL OnOK(HWND hwnd) {
		HWND hLst2 = GetDlgItem(hwnd, lst2);
		HWND hLst3 = GetDlgItem(hwnd, lst3);

		m_priority_tags.clear();
		m_forbidden_tags.clear();

		MStringW strTheme;
		WCHAR szText[XG_MAX_TAGSLEN];
		INT cItems;

		cItems = ListView_GetItemCount(hLst2);
		for (INT iItem = 0; iItem < cItems; ++iItem) {
			ListView_GetItemText(hLst2, iItem, 0, szText, _countof(szText));
			m_priority_tags.emplace(szText);
			if (strTheme.size())
				strTheme += L',';
			strTheme += L'+';
			strTheme += szText;
		}

		cItems = ListView_GetItemCount(hLst3);
		for (INT iItem = 0; iItem < cItems; ++iItem) {
			ListView_GetItemText(hLst3, iItem, 0, szText, _countof(szText));
			m_forbidden_tags.emplace(szText);
			if (strTheme.size())
				strTheme += L',';
			strTheme += L'-';
			strTheme += szText;
		}

		m_theme_tags = strTheme;
		return TRUE;
	}

	// タグの検索。
	void OnEdt1(HWND hwnd) noexcept {
		WCHAR szText[64];
		GetDlgItemTextW(hwnd, edt1, szText, _countof(szText));

		INT iItem;
		HWND hLst1 = GetDlgItem(hwnd, lst1);
		const INT nCount = ListView_GetItemCount(hLst1);
		WCHAR szItem[XG_MAX_TAGSLEN];
		for (iItem = 0; iItem < nCount; ++iItem) {
			ListView_GetItemText(hLst1, iItem, 0, szItem, _countof(szItem));
			if (StrStr(szItem, szText)) {
				constexpr auto state = LVIS_FOCUSED | LVIS_SELECTED;
				ListView_SetItemState(hLst1, iItem, state, state);
				ListView_EnsureVisible(hLst1, iItem, FALSE);
				break;
			}
		}
	}

	// 「テーマ」ダイアログのコマンド処理。
	void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case IDOK:
			if (OnOK(hwnd)) {
				EndDialog(hwnd, IDOK);
			}
			break;
		case IDCANCEL:
			EndDialog(hwnd, id);
			break;
		case psh1: // →
			AddTag(hwnd, TRUE);
			break;
		case psh2: // ←
			RemoveTag(hwnd, TRUE);
			break;
		case psh3: // →
			AddTag(hwnd, FALSE);
			break;
		case psh4: // ←
			RemoveTag(hwnd, FALSE);
			break;
		case psh5: // リセット
			m_theme_tags = m_default_theme_tags;
			EndDialog(hwnd, IDOK);
			break;
		case edt1:
			if (codeNotify == EN_CHANGE) {
				OnEdt1(hwnd);
			}
			break;
		case cmb1:
			if (!m_bUpdatingPreset) {
				if (codeNotify == CBN_EDITCHANGE) {
					SetPreset(hwnd);
				}
				if (codeNotify == CBN_SELCHANGE) {
					::PostMessageW(hwnd, WM_COMMAND, IDYES, 0);
				}
			}
			break;
		case IDYES:
			SetPreset(hwnd);
			break;
		default:
			break;
		}
	}

	// リストビューの選択状態をはっきりさせる。
	LRESULT OnCustomDraw(HWND hwnd, LPNMLVCUSTOMDRAW pCustomDraw) noexcept {
		UINT uState;
		HWND hwndLV = pCustomDraw->nmcd.hdr.hwndFrom;
		switch (pCustomDraw->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;
		case CDDS_ITEMPREPAINT:
			uState = ListView_GetItemState(hwndLV, pCustomDraw->nmcd.dwItemSpec, LVIS_SELECTED);
			if (uState & LVIS_SELECTED) {
				pCustomDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
				pCustomDraw->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
				return CDRF_NEWFONT;
			}
			break;
		default:
			break;
		}
		return CDRF_DODEFAULT;
	}

	LRESULT OnNotify(HWND hwnd, INT idFrom, LPNMHDR pnmhdr) {
		if (pnmhdr->code == NM_CUSTOMDRAW) {
			const auto ret = OnCustomDraw(hwnd, reinterpret_cast<LPNMLVCUSTOMDRAW>(pnmhdr));
			return SetDlgMsgResult(hwnd, NM_CUSTOMDRAW, ret);
		}
		switch (idFrom) {
		case lst1:
			if (pnmhdr->code == NM_DBLCLK) {
				AddTag(hwnd, TRUE);
			}
			break;
		case lst2:
			if (pnmhdr->code == NM_DBLCLK) {
				RemoveTag(hwnd, TRUE);
			} else if (pnmhdr->code == LVN_KEYDOWN) {
				LV_KEYDOWN *pKeyDown = reinterpret_cast<LV_KEYDOWN *>(pnmhdr);
				if (pKeyDown->wVKey == VK_DELETE)
					RemoveTag(hwnd, TRUE);
			}
			break;
		case lst3:
			if (pnmhdr->code == NM_DBLCLK) {
				RemoveTag(hwnd, FALSE);
			} else if (pnmhdr->code == LVN_KEYDOWN) {
				LV_KEYDOWN *pKeyDown = reinterpret_cast<LV_KEYDOWN *>(pnmhdr);
				if (pKeyDown->wVKey == VK_DELETE)
					RemoveTag(hwnd, FALSE);
			}
			break;
		default:
			break;
		}
		return 0;
	}

	// 「テーマ」ダイアログプロシージャ。
	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
		default:
			break;
		}
		return 0;
	}

	void SetPreset(HWND hwnd, LPCWSTR pszText) {
		HWND hLst2 = GetDlgItem(hwnd, lst2);
		HWND hLst3 = GetDlgItem(hwnd, lst3);
		ListView_DeleteAllItems(hLst2);
		ListView_DeleteAllItems(hLst3);

		std::vector<MStringW> strs;
		LPCWSTR pch = wcschr(pszText, L':');
		if (pch)
			++pch;
		else
			pch = pszText;
		MStringW strText = pch;
		XgStrTrim(strText);

		XgStrReplace(strText, L" ", L"");
		XgStrSplit(strs, strText, L",");

		WCHAR szText[XG_MAX_TAGSLEN];

		for (auto& str : strs) {
			if (str.empty())
				continue;

			bool minus = false;
			if (str[0] == L'-') {
				minus = true;
				str = str.substr(1);
			} else if (str[0] == L'+') {
				str = str.substr(1);
			}
			if (str.empty())
				continue;

			HWND hDest = minus ? hLst3 : hLst2;
			LV_ITEM item = { LVIF_TEXT };
			const INT iItem = ListView_GetItemCount(hDest);
			StringCchCopyW(szText, _countof(szText), str.c_str());
			item.iItem = iItem;
			item.pszText = szText;
			item.iSubItem = 0;
			ListView_InsertItem(hDest, &item);

			auto it = m_tag_histgram.find(str);
			const INT count = (it != m_tag_histgram.end()) ? INT(it->second) : 0;
			StringCchPrintfW(szText, _countof(szText), L"%d", count);
			item.iItem = iItem;
			item.pszText = szText;
			item.iSubItem = 1;
			ListView_SetItem(hDest, &item);
		}

		// 最初の項目を選択する。
		constexpr auto state = LVIS_SELECTED | LVIS_FOCUSED;
		ListView_SetItemState(hLst2, 0, state, state);
		ListView_SetItemState(hLst3, 0, state, state);

		UpdateCounters(hwnd);
		UpdateAllTags(hwnd);   // lst1 = 全タグ − (lst2 ∪ lst3)
	}

	void SetPreset(HWND hwnd) {
		WCHAR szText[XG_MAX_TAGSLEN];
		HWND hCmb1 = GetDlgItem(hwnd, cmb1);
		XgGetComboBoxText(hCmb1, szText, _countof(szText));
		SetPreset(hwnd, szText);
	}

	void UpdatePreset(HWND hwnd) {
		HWND hLst2 = GetDlgItem(hwnd, lst2);
		HWND hLst3 = GetDlgItem(hwnd, lst3);

		MStringW str;
		WCHAR szText[64];
		const INT nCount2 = ListView_GetItemCount(hLst2);
		const INT nCount3 = ListView_GetItemCount(hLst3);
		for (INT i = 0; i < nCount2; ++i) {
			if (str.size()) {
				str += L",";
			}
			ListView_GetItemText(hLst2, i, 0, szText, _countof(szText));
			str += L"+";
			str += szText;
		}
		for (INT i = 0; i < nCount3; ++i) {
			if (str.size()) {
				str += L",";
			}
			ListView_GetItemText(hLst3, i, 0, szText, _countof(szText));
			str += L"-";
			str += szText;
		}

		// 長さ制限。
		if (str.size() > XG_MAX_TAGSLEN - 1)
			str.resize(XG_MAX_TAGSLEN - 1);

		m_bUpdatingPreset = TRUE;
		SetDlgItemTextW(hwnd, cmb1, str.c_str());
		m_bUpdatingPreset = FALSE;
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_THEME);
	}

protected:
	BOOL m_bUpdatingPreset = FALSE;
};
