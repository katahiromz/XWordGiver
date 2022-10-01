#pragma once

#include "XG_Window.hpp"

#define MARKED_INTERVAL (3 * 1000)

// [二重マス単語の候補と配置]ダイアログ。
class XG_MarkingDialog : public XG_Dialog
{
public:
    BOOL m_bUpdating = FALSE;
    BOOL m_bInitted = FALSE;

    XG_MarkingDialog()
    {
    }

    BOOL RefreshCandidates(HWND hwnd)
    {
        // 二重マス単語の候補を取得する。
        XgGetMarkedCandidates();

        // コンボボックスのクリアする。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_RESETCONTENT, 0, 0);

        // コンボボックスに候補を追加する。
        for (auto& item : xg_vMarkedCands) {
            ::SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)item.c_str());
        }

        // すでに解があるかどうかによって切り替え。
        const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);
        XgGetMarkWord(xw, xg_strMarked);

        // テキストを設定する。
        HWND hCmb1 = ::GetDlgItem(hwnd, cmb1);
        DWORD dwSel = ComboBox_GetEditSel(hCmb1);
        ComboBox_RealSetText(hCmb1, xg_strMarked.c_str());
        ComboBox_SetEditSel(hCmb1, LOWORD(dwSel), HIWORD(dwSel));
        return TRUE;
    }

    INT GetCurSel(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        // コンボボックスのテキストを取得する。
        WCHAR szText[256];
        szText[0] = 0;
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));
        std::wstring str = szText;
        xg_str_trim(str);
        str = XgNormalizeString(str);

        return ComboBox_FindStringExact(hCmb1, -1, szText);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 移動。
        if (xg_nMarkingX != CW_USEDEFAULT || xg_nMarkingY != CW_USEDEFAULT) {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            MoveWindow(hwnd, xg_nMarkingX, xg_nMarkingY, rc.right - rc.left, rc.bottom - rc.top, TRUE);
            SendMessageW(hwnd, DM_REPOSITION, 0, 0);
        }
        // 候補を初期化する。
        RefreshCandidates(hwnd);
        // 初期化終わり。
        m_bInitted = TRUE;
        return TRUE;
    }

    BOOL OnCmb1(HWND hwnd, const std::wstring& str)
    {
        BOOL bDone = FALSE;
        auto marked = xg_strMarked;
        auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
        auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
        mu1->Get();
        WCHAR chNotFound = 0;
        {
            bDone = XgSetMarkedWord(str, &chNotFound);
        }
        mu2->Get();

        if (bDone) {
            if (xg_strMarked != marked) {
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
            }
            SetDlgItemTextW(hwnd, stc1, NULL);
        } else {
            if (str.size() && chNotFound != 0)
            {
                WCHAR szText[256];
                StringCchPrintfW(szText, _countof(szText),
                                 XgLoadStringDx1(IDS_MARKINGTYPE), chNotFound);
                SetDlgItemTextW(hwnd, stc1, szText);
            }
            else
            {
                SetDlgItemTextW(hwnd, stc1, XgLoadStringDx1(IDS_MARKINGEMPTY));
            }
        }

        return bDone;
    }

    VOID OnCmb1(HWND hwnd)
    {
        HWND hCmb1 = GetDlgItem(hwnd, cmb1);

        // コンボボックスのテキストを取得する。
        WCHAR szText[256];
        szText[0] = 0;
        ComboBox_RealGetText(hCmb1, szText, _countof(szText));
        std::wstring str = szText;
        xg_str_trim(str);
        str = XgNormalizeString(str);

        OnCmb1(hwnd, str);
        ::SetTimer(hwnd, 999, MARKED_INTERVAL, NULL);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
        case IDCANCEL:
            // ダイアログを閉じる。
            DestroyWindow(hwnd);
            break;
        case cmb1:
            switch (codeNotify)
            {
            case CBN_EDITCHANGE:
            case CBN_SELENDOK:
                if (!m_bUpdating)
                    OnCmb1(hwnd);
                break;
            }
            break;
        case psh1:
            if (xg_vMarkedCands.size()) {
                auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
                auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
                mu1->Get();
                {
                    // 前の候補。
                    INT iItem = GetCurSel(hwnd);
                    if (iItem == CB_ERR) {
                        xg_iMarkedCand = INT(xg_vMarkedCands.size()) - 1;
                    } else {
                        xg_iMarkedCand = iItem - 1;
                        if (xg_iMarkedCand < 0)
                            xg_iMarkedCand = INT(xg_vMarkedCands.size()) - 1;
                    }
                    XgSetMarkedWord(xg_vMarkedCands[xg_iMarkedCand]);
                    XgMarkUpdate();
                    // コンボボックスの更新。
                    m_bUpdating = TRUE;
                    ComboBox_RealSetText(::GetDlgItem(hwnd, cmb1), xg_vMarkedCands[xg_iMarkedCand].c_str());
                    m_bUpdating = FALSE;
                }
                mu2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
            }
            break;
        case psh2:
            if (xg_vMarkedCands.size())
            {
                auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
                auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
                mu1->Get();
                {
                    // 次の候補。
                    INT iItem = GetCurSel(hwnd);
                    if (iItem == CB_ERR) {
                        xg_iMarkedCand = 0;
                    } else {
                        xg_iMarkedCand = (iItem + 1) % xg_vMarkedCands.size();
                    }
                    XgSetMarkedWord(xg_vMarkedCands[xg_iMarkedCand]);
                    XgMarkUpdate();
                    // コンボボックスの更新。
                    m_bUpdating = TRUE;
                    ComboBox_RealSetText(::GetDlgItem(hwnd, cmb1), xg_vMarkedCands[xg_iMarkedCand].c_str());
                    m_bUpdating = FALSE;
                }
                mu2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
            }
            break;
        case psh3:
            {
                auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
                auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
                mu1->Get();
                {
                    // 別の配置。
                    XgSetMarkedWord(xg_strMarked);
                    XgMarkUpdate();
                }
                mu2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
            }
            break;
        case psh5:
            {
                auto mu1 = std::make_shared<XG_UndoData_MarksUpdated>();
                auto mu2 = std::make_shared<XG_UndoData_MarksUpdated>();
                mu1->Get();
                {
                    // クリア。
                    xg_vMarks.clear();
                    xg_iMarkedCand = -1;
                    XgMarkUpdate();
                    // コンボボックスの更新。
                    m_bUpdating = TRUE;
                    ComboBox_RealSetText(::GetDlgItem(hwnd, cmb1), L"");
                    m_bUpdating = FALSE;
                }
                mu2->Get();
                // 元に戻す情報を設定する。
                xg_ubUndoBuffer.Commit(UC_MARKS_UPDATED, mu1, mu2);
            }
            break;
        }
        // 盤面を更新する。
        XgUpdateImage(xg_hMainWnd);
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        if (id == 999) {
            ::KillTimer(hwnd, 999);

            // コンボボックスの更新。
            WCHAR szText[256];
            szText[0] = 0;
            ComboBox_RealGetText(::GetDlgItem(hwnd, cmb1), szText, _countof(szText));
            std::wstring str = szText;
            xg_str_trim(str);
            std::wstring strNormalized = XgNormalizeString(str);

            ComboBox_RealSetText(::GetDlgItem(hwnd, cmb1), strNormalized.c_str());

            // 盤面を更新する。
            XgUpdateImage(xg_hMainWnd);
        }
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        if (m_bInitted)
        {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            xg_nMarkingX = rc.left;
            xg_nMarkingY = rc.top;
        }
    }

    void OnDestroy(HWND hwnd)
    {
        m_hWnd = NULL;
        m_bInitted = FALSE;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (uMsg != WM_TIMER)
        {
            if (::KillTimer(hwnd, 999))
                ::SetTimer(hwnd, 999, MARKED_INTERVAL, NULL);
        }
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        }
        return 0;
    }

    BOOL CreateDialogDx(HWND hwnd)
    {
        if (XG_Dialog::CreateDialogDx(hwnd, IDD_MARKING))
        {
            ::ShowWindow(*this, SW_SHOWNORMAL);
            ::UpdateWindow(*this);
            return TRUE;
        }
        return FALSE;
    }
};
