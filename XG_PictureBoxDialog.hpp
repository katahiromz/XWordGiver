#pragma once

#include "XG_Dialog.hpp"

class XG_PictureBoxDialog : public XG_Dialog
{
public:
    std::wstring m_strFile;

    XG_PictureBoxDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        // 画像ファイルリストを取得する。
        std::vector<std::wstring> items;
        XgGetImageList(items);
        for (auto& item : items) {
            ComboBox_AddString(hCmb1, item.c_str());
        }

        ComboBox_RealSetText(hCmb1, m_strFile.c_str());

        DragAcceptFiles(hwnd, TRUE);
        return TRUE;
    }

    BOOL OnOK(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        WCHAR szText[MAX_PATH];
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));

        m_strFile = szText;
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            if (OnOK(hwnd)) {
                // ダイアログを終了。
                ::EndDialog(hwnd, id);
            }
            break;
        case IDCANCEL:
            // ダイアログを終了。
            ::EndDialog(hwnd, id);
            break;
        case psh1:
            OnPsh1(hwnd);
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR szFile[MAX_PATH];
        ::DragQueryFile(hdrop, 0, szFile, _countof(szFile));

        m_strFile = szFile;

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        ComboBox_RealSetText(hCmb1, m_strFile.c_str());

        DragFinish(hdrop);
    }

    void OnPsh1(HWND hwnd)
    {
        WCHAR szFile[MAX_PATH] = L"";
        OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
        ofn.lpstrFilter = L"Picture Files\0*.bmp;*.emf;*.png;*.jpg;*.gif\0";
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = _countof(szFile);
        ofn.lpstrTitle = L"Choose Picture File";
        ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY |
                    OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileNameW(&ofn)) {
            m_strFile = szFile;

            HWND hCmb1 = GetDlgItem(hwnd, cmb1);
            ComboBox_RealSetText(hCmb1, m_strFile.c_str());
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_PICTUREBOX);
    }
};
