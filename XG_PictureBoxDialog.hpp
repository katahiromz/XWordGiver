#pragma once

#include "XG_Window.hpp"

class XG_PictureBoxDialog : public XG_Dialog
{
public:
    std::wstring m_strFile;
    HBITMAP m_hbm = nullptr;
    HENHMETAFILE m_hEMF = nullptr;

    XG_PictureBoxDialog() noexcept
    {
    }

    ~XG_PictureBoxDialog() noexcept
    {
        DoDelete();
    }

    void DoDelete() noexcept
    {
        DeleteObject(m_hbm);
        m_hbm = nullptr;
        DeleteEnhMetaFile(m_hEMF);
        m_hEMF = nullptr;
    }

    void RefreshImage(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);
        WCHAR szText[MAX_PATH];
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));

        HWND hIco1 = GetDlgItem(hwnd, ico1);
        HWND hIco2 = GetDlgItem(hwnd, ico2);

        SendMessageW(hIco1, STM_SETIMAGE, IMAGE_ENHMETAFILE, 0);
        SendMessageW(hIco2, STM_SETIMAGE, IMAGE_BITMAP, 0);

        DoDelete();

        SetWindowPos(hIco1, nullptr, 0, 0, 32, 32, SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);
        SetWindowPos(hIco2, nullptr, 0, 0, 32, 32, SWP_NOMOVE | SWP_NOZORDER | SWP_HIDEWINDOW);

        if (XgLoadImage(szText, m_hbm, m_hEMF))
        {
            if (m_hbm) {
                HBITMAP hbm2 = reinterpret_cast<HBITMAP>(::CopyImage(m_hbm, IMAGE_BITMAP, 32, 32, LR_CREATEDIBSECTION));
                DeleteObject(m_hbm);
                m_hbm = hbm2;
                ShowWindow(hIco2, SW_SHOWNOACTIVATE);
                SendMessageW(hIco2, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hbm);
                return;
            }
            if (m_hEMF) {
                ShowWindow(hIco1, SW_SHOWNOACTIVATE);
                SendMessageW(hIco1, STM_SETIMAGE, IMAGE_ENHMETAFILE, (LPARAM)m_hEMF);
                return;
            }
        }
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        // 画像ファイルリストを取得する。
        std::vector<std::wstring> items;
        XgGetFileManager()->get_list(items);
        for (auto& item : items) {
            ComboBox_AddString(hCmb1, item.c_str());
        }

        ComboBox_RealSetText(hCmb1, m_strFile.c_str());
        RefreshImage(hwnd);

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
        case cmb1:
            if (codeNotify == CBN_SELCHANGE || codeNotify == CBN_EDITCHANGE)
            {
                RefreshImage(hwnd);
            }
            break;
        default:
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
        RefreshImage(hwnd);

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
            RefreshImage(hwnd);
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_PICTUREBOX);
    }
};
