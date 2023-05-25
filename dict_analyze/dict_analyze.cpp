#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <strsafe.h>
#include "layout.h"
#include "resource.h"

typedef std::vector<std::wstring> word_list_t;
typedef std::set<std::wstring> tags_t;
typedef std::map<std::wstring, tags_t> tag_info_t;

LAYOUT_DATA *s_layout = NULL;

inline bool XgIsCharKatakanaW(WCHAR ch)
{
    return ((L'\x30A1' <= ch && ch <= L'\x30F3') || ch == L'\x30FC' || ch == L'\x30F4');
}
inline bool XgIsCharZenkakuUpperW(WCHAR ch)
{
    return (L'\xFF21' <= ch && ch <= L'\xFF3A');
}
inline bool XgIsCharKanjiW(WCHAR ch)
{
    return ((0x3400 <= ch && ch <= 0x9FFF) ||
            (0xF900 <= ch && ch <= 0xFAFF) || ch == L'\x3007');
}
inline bool XgIsCharZenkakuCyrillicW(WCHAR ch)
{
    return 0x0400 <= ch && ch <= 0x04FF;
}
inline bool XgIsCharZenkakuNumericW(WCHAR ch)
{
    return 0xFF10 <= ch && ch <= 0xFF19;
}
inline bool XgIsCharHankakuNumericW(WCHAR ch)
{
    return L'0' <= ch && ch <= L'9';
}

template <typename T_STR>
inline bool
mstr_replace_all(T_STR& str, const T_STR& from, const T_STR& to)
{
    bool ret = false;
    size_t i = 0;
    for (;;) {
        i = str.find(from, i);
        if (i == T_STR::npos)
            break;
        ret = true;
        str.replace(i, from.size(), to);
        i += to.size();
    }
    return ret;
}
template <typename T_STR>
inline bool
mstr_replace_all(T_STR& str,
                 const typename T_STR::value_type *from,
                 const typename T_STR::value_type *to)
{
    return mstr_replace_all(str, T_STR(from), T_STR(to));
}

template <typename T_CHAR>
inline void mstr_trim(std::basic_string<T_CHAR>& str, const T_CHAR *spaces)
{
    typedef std::basic_string<T_CHAR> string_type;
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == string_type::npos) || (j == string_type::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

template <typename T_STR_CONTAINER>
inline void
mstr_split(T_STR_CONTAINER& container,
           const typename T_STR_CONTAINER::value_type& str,
           const typename T_STR_CONTAINER::value_type& chars)
{
    container.clear();
    size_t i = 0, k = str.find_first_of(chars);
    while (k != T_STR_CONTAINER::value_type::npos)
    {
        container.push_back(str.substr(i, k - i));
        i = k + 1;
        k = str.find_first_of(chars, i);
    }
    container.push_back(str.substr(i));
}

template <typename T_STR_CONTAINER>
inline void
mstr_split_insert(T_STR_CONTAINER& container,
                  const typename T_STR_CONTAINER::value_type& str,
                  const typename T_STR_CONTAINER::value_type& chars)
{
    container.clear();
    size_t i = 0, k = str.find_first_of(chars);
    while (k != T_STR_CONTAINER::value_type::npos)
    {
        container.insert(str.substr(i, k - i));
        i = k + 1;
        k = str.find_first_of(chars, i);
    }
    container.insert(str.substr(i));
}

template <typename T_STR_CONTAINER>
inline typename T_STR_CONTAINER::value_type
mstr_join(const T_STR_CONTAINER& container,
          const typename T_STR_CONTAINER::value_type& sep)
{
    typename T_STR_CONTAINER::value_type result;
    typename T_STR_CONTAINER::const_iterator it, end;
    it = container.begin();
    end = container.end();
    if (it != end)
    {
        result = *it;
        for (++it; it != end; ++it)
        {
            result += sep;
            result += *it;
        }
    }
    return result;
}

inline LPWSTR LoadStringDx(INT nID)
{
    static size_t s_index = 0;
    const size_t cchBuffMax = 1024;
    static WCHAR s_sz[4][cchBuffMax];

    WCHAR *pszBuff = s_sz[s_index];
    s_index = (s_index + 1) % _countof(s_sz);
    pszBuff[0] = 0;
    if (!::LoadStringW(NULL, nID, pszBuff, INT(cchBuffMax)))
        assert(0);
    return pszBuff;
}

inline LPWSTR LoadVPrintfDx(INT nID, va_list va) {
    static WCHAR s_szText[1024];
    StringCbVPrintfW(s_szText, sizeof(s_szText), LoadStringDx(nID), va);
    return s_szText;
}

inline LPWSTR LoadPrintfDx(INT nID, ...) {
    va_list va;
    va_start(va, nID);
    LPWSTR psz = LoadVPrintfDx(nID, va);
    va_end(va);
    return psz;
}

void DoAddText(HWND hwnd, LPCWSTR pszText) {
    std::wstring strText = pszText;
    strText += L"\r\n";
    HWND hEdt1 = GetDlgItem(hwnd, edt1);
    INT cch = Edit_GetTextLength(hEdt1);
    if (cch > 1 * 1024 * 1024) {
        Edit_SetSel(hEdt1, 0, 1024);
        Edit_ReplaceSel(hEdt1, L"");
        cch = Edit_GetTextLength(hEdt1);
    }
    Edit_SetSel(hEdt1, cch, cch);
    Edit_ReplaceSel(hEdt1, strText.c_str());
    Edit_ScrollCaret(hEdt1);
    cch = Edit_GetTextLength(hEdt1);
    Edit_SetSel(hEdt1, cch, cch);
    EnableWindow(GetDlgItem(hwnd, psh1), TRUE);
}

inline void DoAddText(HWND hwnd, INT nIDS_) {
    DoAddText(hwnd, LoadStringDx(nIDS_));
}

inline void DoVPrintf(HWND hwnd, INT nIDS_, va_list va) {
    static WCHAR s_szText[1024];
    StringCbVPrintfW(s_szText, sizeof(s_szText), LoadStringDx(nIDS_), va);
    DoAddText(hwnd, s_szText);
}

inline void DoPrintf(HWND hwnd, INT nIDS_, ...) {
    va_list va;
    va_start(va, nIDS_);
    DoVPrintf(hwnd, nIDS_, va);
    va_end(va);
}

bool DoAnalyzeDict(HWND hwnd, LPCWSTR pszFileName, const word_list_t& list, const tag_info_t& tag_info)
{
    std::vector<std::pair<size_t, std::wstring> > score_items;
    size_t score = 0, score_max = 0;

    size_t types[5] = { 0 };

    // count word for each length
    std::map<size_t, size_t> length_count;
    std::wstring strPrevious;
    bool doubled = false;
    for (auto& item : list) {
        if (item == strPrevious) {
            DoPrintf(hwnd, 141, item.c_str());
            doubled = true;
        }
        length_count[item.size()] += 1;
        if (item.size()) {
            WCHAR ch = item[0];
            if (XgIsCharKatakanaW(ch)) {
                ++types[0];
            } else if (XgIsCharZenkakuUpperW(ch)) {
                ++types[1];
            } else if (XgIsCharKanjiW(ch)) {
                ++types[2];
            } else if (XgIsCharZenkakuCyrillicW(ch)) {
                ++types[3];
            } else {
                ++types[4];
            }
        }
        strPrevious = item;
    }
    if (!doubled) {
        DoPrintf(hwnd, 146);
    }

    DoPrintf(hwnd, 140, UINT(list.size()));

    if (!!types[0] + !!types[1] + !!types[2] + !!types[3] + !!types[4] >= 2) {
        size_t imax = 0, max_type = 0;
        for (size_t i = 0; i < 5; ++i) {
            if (max_type < types[i]) {
                max_type = types[i];
                imax = i;
            }
        }
        switch (imax) {
        case 0:
            DoPrintf(hwnd, 122);
            break;
        case 1:
            DoPrintf(hwnd, 123);
            break;
        case 2:
            DoPrintf(hwnd, 124);
            break;
        case 3:
            DoPrintf(hwnd, 125);
            break;
        case 4:
            DoPrintf(hwnd, 126);
            break;
        }
        return false;
    }

    BOOL bKana = FALSE;
    for (size_t i = 0; i < 5; ++i) {
        if (types[i]) {
            switch (i) {
            case 0:
                DoPrintf(hwnd, 127);
                bKana = TRUE;
                break;
            case 1:
                DoPrintf(hwnd, 128);
                break;
            case 2:
                DoPrintf(hwnd, 129);
                return false;
            case 3:
                DoPrintf(hwnd, 130);
                return false;
            case 4:
                DoPrintf(hwnd, 131);
                return false;
            }
            break;
        }
    }

    for (auto& pair : length_count) {
        DoPrintf(hwnd, 136, UINT(pair.first), UINT(pair.second));
    }

    if (wcsstr(pszFileName, LoadStringDx(137)) ||
        wcsstr(pszFileName, LoadStringDx(138)))
    {
        DoPrintf(hwnd, 139);
        return true;
    }

    // word length histogram
    std::vector<std::wstring> items101;
    std::wstring str101 = LoadStringDx(bKana ? 101 : 132);
    mstr_split(items101, str101, L",");
    for (auto& item : items101) {
        size_t ich1 = item.find(L':');
        size_t ich2 = item.find(L'*');
        if (ich1 == std::wstring::npos || ich2 == std::wstring::npos)
            continue;
        size_t char_count = _wtoi(item.c_str());
        size_t needed_count = _wtoi(item.substr(ich1 + 1).c_str());
        size_t ratio = _wtoi(item.substr(ich2 + 1).c_str());
        score_max += ratio * needed_count;
        if (length_count[char_count] < needed_count) {
            auto diff = needed_count - length_count[char_count];
            score += ratio * length_count[char_count];
            score_items.push_back(
                std::make_pair(
                    ratio * diff,
                    LoadPrintfDx(114, UINT(char_count), UINT(diff))
                )
            );
        } else {
            score += ratio * needed_count;
        }
    }

    // 1st char
    std::vector<std::wstring> items102;
    std::wstring str102 = LoadStringDx(bKana ? 102 : 133);
    mstr_split(items102, str102, L",");
    for (auto& item : items102) {
        size_t ich = item.find(L':');
        if (ich == std::wstring::npos)
            continue;
        size_t needed_count = _wtoi(item.c_str());
        auto chars = item.substr(ich + 1);
        for (auto ch : chars) {
            size_t count = 0;
            for (auto& word : list) {
                if (word.size() <= 1)
                    continue;
                if (word.size() >= 1 && word[0] == ch) {
                    ++count;
                }
            }
            score_max += needed_count;
            if (count < needed_count) {
                auto diff = needed_count - count;
                score += count;
                score_items.push_back(
                    std::make_pair(
                        diff + 20,
                        LoadPrintfDx(115, ch, UINT(diff))
                    )
                );
            } else {
                score += needed_count;
            }
        }
    }

    // 2nd char
    std::vector<std::wstring> items103;
    std::wstring str103 = LoadStringDx(bKana ? 103 : 134);
    mstr_split(items103, str103, L",");
    for (auto& item : items103) {
        size_t ich = item.find(L':');
        if (ich == std::wstring::npos)
            continue;
        size_t needed_count = _wtoi(item.c_str());
        auto chars = item.substr(ich + 1);
        for (auto ch : chars) {
            size_t count = 0;
            for (auto& word : list) {
                if (word.size() <= 1)
                    continue;
                if (word.size() >= 2 && word[1] == ch) {
                    ++count;
                }
            }
            score_max += needed_count;
            if (count < needed_count) {
                auto diff = needed_count - count;
                score += count;
                score_items.push_back(
                    std::make_pair(
                        diff,
                        LoadPrintfDx(116, ch, UINT(diff))
                    )
                );
            } else {
                score += needed_count;
            }
        }
    }

    // 3rd char
    std::vector<std::wstring> items104;
    std::wstring str104 = LoadStringDx(bKana ? 104 : 135);
    mstr_split(items104, str104, L",");
    for (auto& item : items104) {
        size_t ich = item.find(L':');
        if (ich == std::wstring::npos)
            continue;
        size_t needed_count = _wtoi(item.c_str());
        auto chars = item.substr(ich + 1);
        for (auto ch : chars) {
            size_t count = 0;
            for (auto& word : list) {
                if (word.size() <= 1)
                    continue;
                if (word.size() >= 3 && word[2] == ch) {
                    ++count;
                }
            }
            score_max += needed_count;
            if (count < needed_count) {
                auto diff = needed_count - count;
                score += count;
                score_items.push_back(
                    std::make_pair(
                        diff,
                        LoadPrintfDx(117, ch, UINT(diff))
                    )
                );
            } else {
                score += needed_count;
            }
        }
    }

    // tag info
    if (tag_info.empty()) {
        DoPrintf(hwnd, 143);
    } else {
        DoPrintf(hwnd, 142);
    }

    std::sort(score_items.begin(), score_items.end(),
        [](const std::pair<size_t, std::wstring>& a,
           const std::pair<size_t, std::wstring>& b)
        {
            return (a.first > b.first);
        }
    );

    size_t count = 0;
    const size_t c_max = 20;
    for (auto& item : score_items) {
        if (count++ > c_max)
            break;
        DoAddText(hwnd, item.second.c_str());
    }

    if (tag_info.size()) {
        tags_t tags;
        for (auto& item : list) {
            auto it = tag_info.find(item);
            if (it != tag_info.end()) {
                tags.insert(it->second.begin(), it->second.end());
            }
        }
        tags.erase(L"");
        typedef std::map<size_t, std::wstring> tag_histgram_t;
        typedef tag_histgram_t::reverse_iterator it_t;
        tag_histgram_t tag_histgram;
        for (auto& tag : tags) {
            size_t count = 0;
            for (auto& item : list) {
                auto it = tag_info.find(item);
                if (it != tag_info.end()) {
                    if (it->second.count(tag)) {
                        ++count;
                    }
                }
            }
            tag_histgram[count] = tag;
        }
        size_t number = 0;
        for (it_t it = tag_histgram.rbegin(); it != tag_histgram.rend(); ++it) {
            auto& pair = *it;
            size_t count = pair.first;
            auto& tag = pair.second;
            DoPrintf(hwnd, 145, pair.second.c_str(), UINT(count));

            if (number <= 10) {
                size_t needed_count = std::min(list.size() / 10, (size_t)50);
                if (count < needed_count) {
                    auto diff = needed_count - count;
                    DoPrintf(hwnd, 144, tag.c_str(), UINT(diff));
                    ++number;
                }
            } else {
                if (count < 10) {
                    DoPrintf(hwnd, 147, tag.c_str());
                }
            }
        }
    }

    if (count >= c_max) {
        DoAddText(hwnd, 120);
    }

    DoPrintf(hwnd, 121, (score * 100 / score_max));
    return true;
}

bool DoLoadDict(HWND hwnd, const WCHAR *fname, word_list_t& list, tag_info_t& tag_info) {
    static char s_asz[1024];
    static WCHAR s_wsz[1024];
    list.clear();
    tag_info.clear();
    if (FILE *fp = _wfopen(fname, L"rb")) {
        bool first_line = true;
        while (fgets(s_asz, sizeof(s_asz), fp)) {
            std::string str;
            if (first_line) {
                if (std::memcmp(s_asz, "\xEF\xBB\xBF", 3) != 0) {
                    DoPrintf(hwnd, 118, PathFindFileNameW(fname));
                    std::fclose(fp);
                    return false;
                }
                str = &s_asz[3];
                first_line = false;
            } else {
                str = s_asz;
            }
            if (str.empty() || str[0] == L'#')
                continue;
            ::MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, s_wsz, _countof(s_wsz));
            std::wstring wstr = s_wsz;
            auto ich1 = wstr.find(L'\t');
            if (ich1 != wstr.npos) {
                auto ich2 = wstr.find(L'\t', ich1 + 1);
                if (ich2 != wstr.npos) {
                    std::wstring strTags = wstr.substr(ich2 + 1);
                    mstr_trim(strTags, L" \t\r\n");
                    tags_t tags;
                    mstr_split_insert(tags, strTags, L",");
                    wstr = wstr.substr(0, ich1);
                    tag_info[wstr] = tags;
                } else {
                    wstr = wstr.substr(0, ich1);
                }
            }
            mstr_trim(wstr, L" \t\r\n");
            WCHAR szText[256];
            LCMapStringW(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT),
                         LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                         wstr.c_str(), -1,
                         szText, _countof(szText));
            wstr = szText;
            std::wstring from = LoadStringDx(IDS_SMALL);
            std::wstring to = LoadStringDx(IDS_LARGE);
            for (auto& ch : wstr) {
                for (size_t i = 0; i < from.size(); ++i) {
                    if (ch == from[i]) {
                        ch = to[i];
                    }
                }
            }
            list.push_back(wstr);
        }
        tag_info.erase(L"");
        std::fclose(fp);
        return true;
    }
    return false;
}

void JustDoIt(HWND hwnd, LPCWSTR pszFileName)
{
    LPWSTR pszFileTitle = PathFindFileNameW(pszFileName);

    DoAddText(hwnd, IDS_APPNAME);
    DoPrintf(hwnd, IDS_READINGSTARTED, pszFileTitle);
    SetDlgItemTextW(hwnd, stc1, pszFileName);

    if (lstrcmpiW(PathFindExtensionW(pszFileName), L".tsv") != 0) {
        DoPrintf(hwnd, IDS_EXTDIFFERENT);
        return;
    }

    word_list_t list;
    tag_info_t tag_info;
    if (DoLoadDict(hwnd, pszFileName, list, tag_info)) {
        DoPrintf(hwnd, IDS_READINGDONE, pszFileTitle);
        DoPrintf(hwnd, IDS_ANALYZESTART, pszFileTitle);
        std::sort(list.begin(), list.end());
        if (DoAnalyzeDict(hwnd, pszFileName, list, tag_info)) {
            DoPrintf(hwnd, IDS_ANALYZEDONE, pszFileTitle);
            SetDlgItemTextW(hwnd, stc2, LoadStringDx(119));
        } else {
            DoPrintf(hwnd, IDS_ANALYZEFAILED, pszFileTitle);
        }
    } else {
        DoPrintf(hwnd, IDS_READINGFAILED, pszFileTitle);
    }
}

inline BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(hwndFocus);
    UNREFERENCED_PARAMETER(lParam);

    DragAcceptFiles(hwnd, TRUE);
    static const LAYOUT_INFO info[] = {
        { stc1, BF_LEFT | BF_RIGHT | BF_TOP, },
        { stc3, BF_LEFT | BF_TOP, },
        { edt1, BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM },
        { stc2, BF_LEFT | BF_BOTTOM },
        { IDOK, BF_RIGHT | BF_BOTTOM },
        { psh1, BF_RIGHT | BF_TOP },
    };
    s_layout = LayoutInit(hwnd, info, _countof(info));
    LayoutEnableResize(s_layout, TRUE);

    EnableWindow(GetDlgItem(hwnd, psh1), FALSE);

    INT argc;
    if (LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc)) {
        if (argc >= 2) {
            JustDoIt(hwnd, wargv[1]);
        }
        LocalFree(wargv);
    }

    return TRUE;
}

inline void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    UNREFERENCED_PARAMETER(hwndCtl);
    UNREFERENCED_PARAMETER(codeNotify);

    switch (id) {
    case IDOK:
    case IDCANCEL:
        LayoutDestroy(s_layout);
        EndDialog(hwnd, id);
        break;
    case psh1:
        SetDlgItemTextW(hwnd, edt1, L"");
        EnableWindow(GetDlgItem(hwnd, psh1), FALSE);
        break;
    }
}

inline void OnDropFiles(HWND hwnd, HDROP hdrop)
{
    WCHAR szFile[MAX_PATH];
    DragQueryFileW(hdrop, 0, szFile, MAX_PATH);
    JustDoIt(hwnd, szFile);
    DragFinish(hdrop);
}

inline void OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    UNREFERENCED_PARAMETER(state);
    UNREFERENCED_PARAMETER(cx);
    UNREFERENCED_PARAMETER(cy);
    LayoutUpdate(hwnd, s_layout, NULL, 0);
}

INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);
    InitCommonControls();
    DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_MAIN), NULL, DialogProc);
    return 0;
}
