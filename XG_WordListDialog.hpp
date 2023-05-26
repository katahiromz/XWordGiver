#pragma once

#include "XG_Window.hpp"
#include "crossword_generation.hpp"

// 「単語リストから生成」ダイアログ。
class XG_WordListDialog : public XG_Dialog
{
public:
    inline static std::vector<std::wstring> s_words;
    inline static std::unordered_set<std::wstring> s_wordset;
    inline static std::wstring s_str_word_list;

    RECT m_rcBitmap;
    SIZE m_sizBitmap;
    HBITMAP m_hbmBitmap = nullptr;

    XG_WordListDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept
    {
        XgCenterDialog(hwnd);
        // 文字数の限界を増やす。
        ::SendDlgItemMessageW(hwnd, edt1, EM_SETLIMITTEXT, MAXSHORT, 0);

        // コントロールstc1の位置とサイズを取得。
        HWND hStc1 = GetDlgItem(hwnd, stc1);
        assert(hStc1 != nullptr);
        GetWindowRect(hStc1, &m_rcBitmap);
        MapWindowRect(nullptr, hwnd, &m_rcBitmap);
        DestroyWindow(hStc1);

        // ビットマップをリソースから読み込む。
        m_hbmBitmap = LoadBitmapW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDB_FROMWORDS));
        assert(m_hbmBitmap != nullptr);

        // ビットマップのサイズを取得。
        BITMAP bm;
        GetObject(m_hbmBitmap, sizeof(bm), &bm);
        m_sizBitmap.cx = bm.bmWidth;
        m_sizBitmap.cy = bm.bmHeight;

        // 再描画。
        InvalidateRect(hwnd, &m_rcBitmap, TRUE);

        // テキストを設定。
        if (s_str_word_list.empty())
            SetDlgItemTextW(hwnd, edt1, L"Here are the example words for XWordGiver");
        else
            SetDlgItemTextW(hwnd, edt1, s_str_word_list.c_str());

        return TRUE;
    }

    BOOL OnOK(HWND hwnd)
    {
        s_words.clear();
        s_wordset.clear();

        // edt1からテキストを取得する。
        HWND hEdt1 = GetDlgItem(hwnd, edt1);
        const auto cch = GetWindowTextLengthW(hEdt1);
        if (cch == 0) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADDMOREWORDS), nullptr, MB_ICONERROR);
            return FALSE;
        }
        LPWSTR psz = new WCHAR[cch + 1];
        if (!GetWindowTextW(hEdt1, psz, cch + 1))
        {
            delete[] psz;
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_OUTOFMEMORY), nullptr, MB_ICONERROR);
            return FALSE;
        }
        std::wstring str = psz;
        delete[] psz;

        // 単語リストと辞書を取得する。
        std::vector<std::wstring> items;
        mstr_split(items, str, L" \t\r\n\x3000");
        for (auto& item : items) {
            // ハイフン、アポストロフィ、ピリオド、カンマ、カッコを取り除く。
            std::wstring tmp;
            for (auto ch : item) {
                if (ch == L'-' || ch == 0xFF0D)
                    continue;
                if (ch == L'\'' || ch == 0xFF07)
                    continue;
                if (ch == L'.' || ch == 0xFF0E)
                    continue;
                if (ch == L',' || ch == 0xFF0C)
                    continue;
                if (ch == L'(' || ch == 0xFF08)
                    continue;
                if (ch == L')' || ch == 0xFF09)
                    continue;
                tmp += ch;
            }
            item = std::move(tmp);
            // 前後の空白を取り除く。
            xg_str_trim(item);
            // 文字列を標準化。
            item = XgNormalizeString(item);
            // 2文字以上なら追加。
            if (item.size() >= 2) {
                s_words.push_back(item);
            }
        }
        items.clear();

        // 2文字未満の単語を削除する。
        XgTrimDict(s_words);

        // 単語が少ない？
        if (s_words.size() < 2) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADDMOREWORDS), nullptr, MB_ICONERROR);
            return FALSE;
        }
        // 多すぎる？
        constexpr size_t max_num = 300;
        if (s_words.size() > max_num) {
            WCHAR szText[128];
            StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_TOOMANYWORDS), static_cast<int>(max_num));
            XgCenterMessageBoxW(hwnd, szText, nullptr, MB_ICONERROR);
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
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ADDMOREWORDS), nullptr, MB_ICONERROR);
                    return FALSE;
                }
            }
        }

        if (!check_connectivity<wchar_t>(s_wordset, nonconnected)) {
            // 連結されていない。エラー。
            WCHAR szText[256];
            StringCchPrintfW(szText, _countof(szText), XgLoadStringDx1(IDS_NOTCONNECTABLE),
                             nonconnected.c_str());
            XgCenterMessageBoxW(hwnd, szText, nullptr, MB_ICONERROR);
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
                // ビットマップを破棄。
                ::DeleteObject(m_hbmBitmap);
                m_hbmBitmap = nullptr;
                // ダイアログを終了。
                ::EndDialog(hwnd, id);
            }
            break;
        case IDCANCEL:
            // ビットマップを破棄。
            ::DeleteObject(m_hbmBitmap);
            m_hbmBitmap = nullptr;
            // ダイアログを終了。
            ::EndDialog(hwnd, id);
            break;
        default:
            break;
        }
    }

    void OnPaint(HWND hwnd) noexcept
    {
        // ここでビットマップを使って、イメージを描画する。
        PAINTSTRUCT ps;
        if (HDC hdc = BeginPaint(hwnd, &ps))
        {
            if (HDC hdcMem = CreateCompatibleDC(hdc))
            {
                HGDIOBJ hbmOld = SelectObject(hdcMem, m_hbmBitmap);
                SetStretchBltMode(hdc, STRETCH_HALFTONE);
                StretchBlt(hdc, m_rcBitmap.left, m_rcBitmap.top,
                    m_rcBitmap.right - m_rcBitmap.left,
                    m_rcBitmap.bottom - m_rcBitmap.top,
                    hdcMem,
                    0, 0, m_sizBitmap.cx, m_sizBitmap.cy,
                    SRCCOPY);
                SelectObject(hdcMem, hbmOld);
                DeleteObject(hdcMem);
            }
            else
            {
                assert(0);
            }
            EndPaint(hwnd, &ps);
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_WORDLIST);
    }
};
