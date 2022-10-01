#pragma once

#include "XG_Window.hpp"

// タグ群の最大長。
#define XG_MAX_TAGSLEN 256

// 「テーマ」ダイアログ。
class XG_ThemeDialog : public XG_Dialog
{
public:
    inline static BOOL xg_bUpdatingPreset = FALSE;

    XG_ThemeDialog()
    {
    }

    // タグリストボックスを初期化。
    void XgInitTagListView(HWND hwndLV)
    {
        DWORD exstyle = LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_LABELTIP | LVS_EX_GRIDLINES;
        ListView_SetExtendedListViewStyleEx(hwndLV, exstyle, exstyle);

        LV_COLUMN column = { LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT };

        column.pszText = XgLoadStringDx1(IDS_TAGS);
        column.fmt = LVCFMT_LEFT;
        column.cx = 90;
        column.iSubItem = 0;
        ListView_InsertColumn(hwndLV, 0, &column);

        column.pszText = XgLoadStringDx1(IDS_TAGCOUNT);
        column.fmt = LVCFMT_RIGHT;
        column.cx = 84;
        column.iSubItem = 1;
        ListView_InsertColumn(hwndLV, 1, &column);
    }

    // 「テーマ」ダイアログの初期化。
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央寄せする。
        XgCenterDialog(hwnd);

        // 長さを制限する。
        SendDlgItemMessageW(hwnd, cmb1, CB_LIMITTEXT, XG_MAX_TAGSLEN - 1, 0);

        // リストビューを初期化。
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        HWND hLst3 = GetDlgItem(hwnd, lst3);
        XgInitTagListView(hLst1);
        XgInitTagListView(hLst2);
        XgInitTagListView(hLst3);

        // ヒストグラムを取得。
        std::vector<std::pair<size_t, std::wstring> > histgram;
        for (auto& pair : xg_tag_histgram) {
            histgram.emplace_back(std::make_pair(pair.second, pair.first));
        }
        // 出現回数の逆順でソート。
        std::sort(histgram.begin(), histgram.end(),
            [](const std::pair<size_t, std::wstring>& a, const std::pair<size_t, std::wstring>& b) {
                return a.first > b.first;
            }
        );

        // リストビューを逆順のヒストグラムで埋める。
        INT iItem = 0;
        LV_ITEM item = { LVIF_TEXT };
        WCHAR szText[64];
        for (auto& pair : histgram) {
            StringCbCopyW(szText, sizeof(szText), pair.second.c_str());
            item.iItem = iItem;
            item.pszText = szText;
            item.iSubItem = 0;
            ListView_InsertItem(hLst1, &item);

            StringCbCopyW(szText, sizeof(szText), std::to_wstring(pair.first).c_str());
            item.iItem = iItem;
            item.pszText = szText;
            item.iSubItem = 1;
            ListView_SetItem(hLst1, &item);

            ++iItem;
        }

        // コンボボックスにテキストを設定する。
        SetDlgItemTextW(hwnd, cmb1, xg_strTheme.c_str());
        // プリセットを設定する。
        SetPreset(hwnd, xg_strTheme.c_str());

        // 最初の項目を選択。
        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItem(hLst1, &item);

        // THEME.txt ファイルを読み込む。
        WCHAR szPath[MAX_PATH];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        if (XgFindLocalFile(szPath, _countof(szPath), L"THEME.txt")) {
            if (FILE *fp = _wfopen(szPath, L"rb")) {
                char buf[256];
                while (fgets(buf, 256, fp)) {
                    std::wstring str = XgUtf8ToUnicode(buf);
                    xg_str_trim(str);
                    if (str.empty())
                        continue;
                    if (str[0] == 0xFEFF || str[0] == L';')
                        continue;
                    ComboBox_AddString(hCmb1, str.c_str());
                }
                fclose(fp);
            }
        }
        // コンボボックスの横幅。
        SendMessageW(hCmb1, CB_SETHORIZONTALEXTENT, 1000, 0);

        return TRUE;
    }

    // リストビューにタグ項目を追加する。
    void AddTag(HWND hwnd, BOOL bPriority)
    {
        // 選択中のテキストを取得する。
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        INT iItem = ListView_GetNextItem(hLst1, -1, LVNI_ALL | LVNI_SELECTED);
        if (iItem < 0)
            return; // 選択なし。
        WCHAR szText1[64], szText2[64];
        ListView_GetItemText(hLst1, iItem, 0, szText1, ARRAYSIZE(szText1));
        ListView_GetItemText(hLst1, iItem, 1, szText2, ARRAYSIZE(szText2));

        LV_FINDINFO find = { LVFI_STRING, szText1 };
        if (bPriority) {
            HWND hLst2 = GetDlgItem(hwnd, lst2);
            iItem = ListView_FindItem(hLst2, -1, &find);
            if (iItem >= 0)
                return; // すでにあった。

            // タグ項目を追加。
            INT cItems = ListView_GetItemCount(hLst2);
            LV_ITEM item = { LVIF_TEXT };
            item.iItem = cItems;
            item.iSubItem = 0;
            item.pszText = szText1;
            iItem = ListView_InsertItem(hLst2, &item);
            item.iItem = iItem;
            item.iSubItem = 1;
            item.pszText = szText2;
            ListView_SetItem(hLst2, &item);

            // カウンターを更新。
            size_t count = 0;
            cItems = ListView_GetItemCount(hLst2);
            for (iItem = 0; iItem < cItems; ++iItem) {
                ListView_GetItemText(hLst2, iItem, 1, szText2, ARRAYSIZE(szText2));
                count += _wtoi(szText2);
            }
            SetDlgItemInt(hwnd, stc1, INT(count), FALSE);
        } else {
            HWND hLst3 = GetDlgItem(hwnd, lst3);
            iItem = ListView_FindItem(hLst3, -1, &find);
            if (iItem >= 0)
                return; // すでにあった。

            // タグ項目を追加。
            INT cItems = ListView_GetItemCount(hLst3);
            LV_ITEM item = { LVIF_TEXT };
            item.iItem = cItems;
            item.iSubItem = 0;
            item.pszText = szText1;
            iItem = ListView_InsertItem(hLst3, &item);
            item.iItem = iItem;
            item.iSubItem = 1;
            item.pszText = szText2;
            ListView_SetItem(hLst3, &item);

            // カウンターを更新。
            size_t count = 0;
            cItems = ListView_GetItemCount(hLst3);
            for (iItem = 0; iItem < cItems; ++iItem) {
                ListView_GetItemText(hLst3, iItem, 1, szText2, ARRAYSIZE(szText2));
                count += _wtoi(szText2);
            }
            SetDlgItemInt(hwnd, stc2, INT(count), FALSE);
        }

        // プリセットを更新。
        UpdatePreset(hwnd);
    }

    // リストビューからタグ項目を削除する。
    void RemoveTag(HWND hwnd, BOOL bPriority)
    {
        WCHAR szText[64];
        if (bPriority) {
            HWND hLst2 = GetDlgItem(hwnd, lst2);
            INT iItem = ListView_GetNextItem(hLst2, -1, LVNI_ALL | LVNI_SELECTED);
            ListView_DeleteItem(hLst2, iItem);

            // カウンターを更新。
            INT cItems = ListView_GetItemCount(hLst2);
            size_t count = 0;
            for (iItem = 0; iItem < cItems; ++iItem) {
                ListView_GetItemText(hLst2, iItem, 1, szText, ARRAYSIZE(szText));
                count += _wtoi(szText);
            }
            SetDlgItemInt(hwnd, stc1, INT(count), FALSE);
        } else {
            HWND hLst3 = GetDlgItem(hwnd, lst3);
            INT iItem = ListView_GetNextItem(hLst3, -1, LVNI_ALL | LVNI_SELECTED);
            ListView_DeleteItem(hLst3, iItem);

            // カウンターを更新。
            INT cItems = ListView_GetItemCount(hLst3);
            size_t count = 0;
            for (iItem = 0; iItem < cItems; ++iItem) {
                ListView_GetItemText(hLst3, iItem, 1, szText, ARRAYSIZE(szText));
                count += _wtoi(szText);
            }
            SetDlgItemInt(hwnd, stc2, INT(count), FALSE);
        }

        // プリセットを更新。
        UpdatePreset(hwnd);
    }

    // 「テーマ」ダイアログで「OK」ボタンが押された。
    BOOL OnOK(HWND hwnd)
    {
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        HWND hLst3 = GetDlgItem(hwnd, lst3);

        xg_priority_tags.clear();
        xg_forbidden_tags.clear();

        std::wstring strTheme;
        WCHAR szText[XG_MAX_TAGSLEN];
        INT cItems;

        cItems = ListView_GetItemCount(hLst2);
        for (INT iItem = 0; iItem < cItems; ++iItem) {
            ListView_GetItemText(hLst2, iItem, 0, szText, ARRAYSIZE(szText));
            xg_priority_tags.emplace(szText);
            if (strTheme.size())
                strTheme += L',';
            strTheme += L'+';
            strTheme += szText;
        }

        cItems = ListView_GetItemCount(hLst3);
        for (INT iItem = 0; iItem < cItems; ++iItem) {
            ListView_GetItemText(hLst3, iItem, 0, szText, ARRAYSIZE(szText));
            xg_forbidden_tags.emplace(szText);
            if (strTheme.size())
                strTheme += L',';
            strTheme += L'-';
            strTheme += szText;
        }

        XgSetThemeString(strTheme);

        return TRUE;
    }

    // タグの検索。
    void OnEdt1(HWND hwnd)
    {
        WCHAR szText[64];
        GetDlgItemTextW(hwnd, edt1, szText, ARRAYSIZE(szText));

        HWND hLst1 = GetDlgItem(hwnd, lst1);

        INT iItem, nCount = ListView_GetItemCount(hLst1);
        WCHAR szItem[XG_MAX_TAGSLEN];
        for (iItem = 0; iItem < nCount; ++iItem) {
            ListView_GetItemText(hLst1, iItem, 0, szItem, _countof(szItem));
            if (StrStr(szItem, szText)) {
                UINT state = LVIS_FOCUSED | LVIS_SELECTED;
                ListView_SetItemState(hLst1, iItem, state, state);
                ListView_EnsureVisible(hLst1, iItem, FALSE);
                break;
            }
        }
    }

    // 「テーマ」ダイアログのコマンド処理。
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
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
            XgSetThemeString(xg_strDefaultTheme);
            EndDialog(hwnd, IDOK);
            break;
        case edt1:
            if (codeNotify == EN_CHANGE) {
                OnEdt1(hwnd);
            }
            break;
        case cmb1:
            if (!xg_bUpdatingPreset) {
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
        }
    }

    // リストビューの選択状態をはっきりさせる。
    LRESULT OnCustomDraw(HWND hwnd, LPNMLVCUSTOMDRAW pCustomDraw)
    {
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
        }
        return CDRF_DODEFAULT;
    }

    LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
    {
        if (pnmhdr->code == NM_CUSTOMDRAW) {
            LRESULT ret = OnCustomDraw(hwnd, (LPNMLVCUSTOMDRAW)pnmhdr);
            return SetDlgMsgResult(hwnd, NM_CUSTOMDRAW, ret);
        }
        LV_KEYDOWN *pKeyDown;
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
                pKeyDown = reinterpret_cast<LV_KEYDOWN *>(pnmhdr);
                if (pKeyDown->wVKey == VK_DELETE)
                    RemoveTag(hwnd, TRUE);
            }
            break;
        case lst3:
            if (pnmhdr->code == NM_DBLCLK) {
                RemoveTag(hwnd, FALSE);
            } else if (pnmhdr->code == LVN_KEYDOWN) {
                pKeyDown = reinterpret_cast<LV_KEYDOWN *>(pnmhdr);
                if (pKeyDown->wVKey == VK_DELETE)
                    RemoveTag(hwnd, FALSE);
            }
            break;
        }
        return 0;
    }

    // 「テーマ」ダイアログプロシージャ。
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
        }
        return 0;
    }

    void SetPreset(HWND hwnd, LPCWSTR pszText)
    {
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        HWND hLst3 = GetDlgItem(hwnd, lst3);
        ListView_DeleteAllItems(hLst2);
        ListView_DeleteAllItems(hLst3);

        std::vector<std::wstring> strs;
        LPCWSTR pch = wcschr(pszText, L':');
        if (pch)
            ++pch;
        else
            pch = pszText;
        std::wstring strText = pch;
        xg_str_trim(strText);

        xg_str_replace_all(strText, L" ", L"");
        mstr_split(strs, strText, L",");

        WCHAR szText[64];

        for (auto& str : strs) {
            if (str.empty())
                continue;

            bool minus = false;
            if (str[0] == L'-') {
                minus = true;
                str = str.substr(1);
            }
            if (str[0] == L'+') {
                str = str.substr(1);
            }

            LV_ITEM item = { LVIF_TEXT };
            INT iItem = ListView_GetItemCount(minus ? hLst3 : hLst2);
            StringCbCopyW(szText, sizeof(szText), str.c_str());
            item.iItem = iItem;
            item.pszText = szText;
            item.iSubItem = 0;
            ListView_InsertItem((minus ? hLst3 : hLst2), &item);

            int count = (int)xg_tag_histgram[str];
            StringCbPrintfW(szText, sizeof(szText), L"%u", count);
            item.iItem = iItem;
            item.pszText = szText;
            item.iSubItem = 1;
            ListView_SetItem((minus ? hLst3 : hLst2), &item);
        }

        // 最初の項目を選択する。
        LV_ITEM item;
        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItem(hLst2, &item);

        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItem(hLst3, &item);
    }

    void SetPreset(HWND hwnd)
    {
        WCHAR szText[XG_MAX_TAGSLEN];
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));
        SetPreset(hwnd, szText);
    }

    void UpdatePreset(HWND hwnd)
    {
        HWND hLst2 = GetDlgItem(hwnd, lst2);
        HWND hLst3 = GetDlgItem(hwnd, lst3);

        std::wstring str;
        WCHAR szText[64];
        INT nCount2 = ListView_GetItemCount(hLst2);
        INT nCount3 = ListView_GetItemCount(hLst3);
        for (INT i = 0; i < nCount2; ++i) {
            if (str.size()) {
                str += L",";
            }
            ListView_GetItemText(hLst2, i, 0, szText, ARRAYSIZE(szText));
            str += L"+";
            str += szText;
        }
        for (INT i = 0; i < nCount3; ++i) {
            if (str.size()) {
                str += L",";
            }
            ListView_GetItemText(hLst3, i, 0, szText, ARRAYSIZE(szText));
            str += L"-";
            str += szText;
        }

        // 長さ制限。
        if (str.size() > XG_MAX_TAGSLEN - 1)
            str.resize(XG_MAX_TAGSLEN - 1);

        xg_bUpdatingPreset = TRUE;
        SetDlgItemTextW(hwnd, cmb1, str.c_str());
        xg_bUpdatingPreset = FALSE;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_THEME);
    }
};
