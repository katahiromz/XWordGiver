#pragma once

#include "XG_Window.hpp"

class XG_UILanguageDialog : public XG_Dialog
{
public:
    LANGID m_LangID;
    std::vector<LANGID> m_ids;

    XG_UILanguageDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);

        // 言語リスト。
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        for (auto id : m_ids)
        {
            WCHAR szText[128];
            QStringW str;
            StringCchPrintfW(szText, _countof(szText), L"%04X: ", id);
            str += szText;
            GetLocaleInfoW(MAKELCID(id, SORT_DEFAULT), LOCALE_SENGLANGUAGE, szText, _countof(szText));
            str += szText;
            str += L" (";
            GetLocaleInfoW(MAKELCID(id, SORT_DEFAULT), LOCALE_SENGCOUNTRY, szText, _countof(szText));
            str += szText;
            str += L")";
            const auto i = ComboBox_AddString(hCmb1, str.c_str());
            if (id == m_LangID)
                ComboBox_SetCurSel(hCmb1, i);
        }
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept
    {
        switch(id)
        {
        case IDOK:
            {
                HWND hCmb1 = GetDlgItem(hwnd, cmb1);
                const auto i = ComboBox_GetCurSel(hCmb1);
                if (i == CB_ERR)
                    return;
                m_LangID = m_ids[i];
            }
            ::EndDialog(hwnd, id);
            break;

        case IDCANCEL:
            ::EndDialog(hwnd, id);
            break;

        default:
            break;
        }
    }

    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_UILANGID);
    }
};
