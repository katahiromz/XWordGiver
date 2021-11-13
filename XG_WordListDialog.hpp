#pragma once

#include "XG_Dialog.hpp"
#include "crossword_generation.hpp"

// 「単語リストから生成」ダイアログ。
class XG_WordListDialog : public XG_Dialog
{
public:
    static std::vector<std::wstring> s_words;
    static std::unordered_set<std::wstring> s_wordset;
    static std::unordered_map<std::wstring, std::wstring> s_dict;
    static std::wstring s_str_word_list;

    XG_WordListDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);
        ::SendDlgItemMessageW(hwnd, edt1, EM_SETLIMITTEXT, MAXSHORT, 0);
        SetDlgItemTextW(hwnd, edt1, s_str_word_list.c_str());
        return TRUE;
    }

    BOOL OnOK(HWND hwnd)
    {
        s_words.clear();
        s_wordset.clear();
        s_dict.clear();

        // edt1からテキストを取得する。
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        INT cch = GetWindowTextLengthW(hEdt1);
        if (cch == 0) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADDMOREWORDS), NULL, MB_ICONERROR);
            return FALSE;
        }
        LPWSTR psz = new WCHAR[cch + 1];
        if (!GetWindowTextW(hEdt1, psz, cch + 1))
        {
            delete[] psz;
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_OUTOFMEMORY), NULL, MB_ICONERROR);
            return FALSE;
        }
        std::wstring str = psz;
        delete[] psz;

        // 単語リストと辞書を取得する。
        std::vector<std::wstring> items;
        mstr_split(items, str, L"\n");
        for (auto& item : items) {
            size_t ich = item.find(L'\t');
            std::wstring hint;
            if (ich != item.npos) {
                hint = item.substr(ich + 1);
                item.resize(ich);
                ich = hint.find(L'\t');
                if (ich != hint.npos) {
                    hint.resize(ich);
                }
                xg_str_trim(hint);
            }
            xg_str_replace_all(item, L"-", L"");
            xg_str_replace_all(item, L"'", L"");
            xg_str_trim(item);
            item = XgNormalizeString(item);
            if (!item.empty()) {
                if (hint.size())
                    s_dict[item] = hint;
                s_words.push_back(item);
            }
        }
        items.clear();

        // 2文字未満の単語を削除する。
        XgTrimDict(s_words);

        // 単語が少ない？
        if (s_words.size() < 2) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADDMOREWORDS), NULL, MB_ICONERROR);
            return FALSE;
        }

        using namespace crossword_generation;

        std::wstring nonconnected;
        s_wordset = { s_words.begin(), s_words.end() };

        // すべてでなくてもよい？
        if (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED) {
            while (!check_connectivity<wchar_t>(s_wordset, nonconnected)) {
                // 接続されていない単語を削除。
                s_wordset.erase(nonconnected);
                s_words.erase(std::remove(s_words.begin(), s_words.end(), nonconnected), s_words.end());

                // 単語が少ない？
                if (s_words.size() < 2) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADDMOREWORDS), NULL, MB_ICONERROR);
                    return FALSE;
                }
            }
        }

        if (!check_connectivity<wchar_t>(s_wordset, nonconnected)) {
            // 連結されていない。エラー。
            WCHAR szText[256];
            StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_NOTCONNECTABLE),
                             nonconnected.c_str());
            XgCenterMessageBoxW(hwnd, szText, NULL, MB_ICONERROR);
        } else {
            return TRUE;
        }

        return FALSE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            if (OnOK(hwnd)) {
                ::EndDialog(hwnd, id);
            }
            break;
        case IDCANCEL:
            ::EndDialog(hwnd, id);
            break;
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        }
        return 0;
    }
};

/*static*/ std::vector<std::wstring> XG_WordListDialog::s_words;
/*static*/ std::unordered_set<std::wstring> XG_WordListDialog::s_wordset;
/*static*/ std::unordered_map<std::wstring, std::wstring> XG_WordListDialog::s_dict;
/*static*/ std::wstring XG_WordListDialog::s_str_word_list;
