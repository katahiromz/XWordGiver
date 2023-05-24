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

        // リストのクリアする。
        m_bUpdating = TRUE;
        ::SendDlgItemMessageW(hwnd, lst1, LB_RESETCONTENT, 0, 0);
        m_bUpdating = FALSE;

        // リストに候補を追加する。
        for (auto& item : xg_vMarkedCands) {
            INT i = ::SendDlgItemMessageW(hwnd, lst1, LB_ADDSTRING, 0, (LPARAM)item.c_str());
            if (item == xg_strMarked) {
                m_bUpdating = TRUE;
                ::SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, i, 0);
                m_bUpdating = FALSE;
            }
        }

        // すでに解があるかどうかによって切り替え。
        const XG_Board *xw = (xg_bSolved ? &xg_solution : &xg_xword);
        XgGetMarkWord(xw, xg_strMarked);

        // テキストを設定する。
        m_bUpdating = TRUE;
        ::SetDlgItemTextW(hwnd, edt1, xg_strMarked.c_str());
        m_bUpdating = FALSE;
        return TRUE;
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

    // テキストの取得。
    std::wstring GetText(HWND hwnd, BOOL bEdit, BOOL bNormalize = TRUE)
    {
        WCHAR szText[256];
        szText[0] = 0;
        if (bEdit) {
            ::GetDlgItemTextW(hwnd, edt1, szText, _countof(szText));
        } else {
            INT i = GetCurSel(hwnd);
            if (i != LB_ERR)
                ::SendDlgItemMessageW(hwnd, lst1, LB_GETTEXT, i, (LPARAM)szText);
        }
        std::wstring str = szText;
        if (bNormalize) {
            xg_str_trim(str);
            str = XgNormalizeString(str);
        }
        return str;
    }

    // テキストの設定。
    BOOL SetText(HWND hwnd, const std::wstring& str, BOOL bFromEdit)
    {
        if (!bFromEdit) {
            m_bUpdating = TRUE;
            INT iStart, iEnd;
            ::SendDlgItemMessageW(hwnd, edt1, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
            ::SetDlgItemTextW(hwnd, edt1, str.c_str());
            ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, iStart, iEnd);
            m_bUpdating = FALSE;
        }

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
            ::SetDlgItemTextW(hwnd, stc1, XgLoadStringDx1(IDS_MARKINGEMPTY));
        } else {
            if (str.size() && chNotFound != 0) {
                WCHAR szText[256];
                StringCchPrintfW(szText, _countof(szText),
                                 XgLoadStringDx1(IDS_MARKINGTYPE), chNotFound);
                ::SetDlgItemTextW(hwnd, stc1, szText);
            }
            else
            {
                ::SetDlgItemTextW(hwnd, stc1, XgLoadStringDx1(IDS_MARKINGEMPTY));
            }
        }

        if (bFromEdit) {
            INT i = (INT)::SendDlgItemMessageW(hwnd, lst1, LB_FINDSTRINGEXACT, -1, (LPARAM)str.c_str());
            m_bUpdating = TRUE;
            if (i != LB_ERR) {
                ::SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, i, 0);
            } else {
                ::SendDlgItemMessageW(hwnd, lst1, LB_SETCURSEL, -1, 0);
            }
            m_bUpdating = FALSE;
        }

        return bDone;
    }

    VOID OnLst1(HWND hwnd)
    {
        INT i = GetCurSel(hwnd);
        if (i == LB_ERR)
            return;

        // テキストを取得する。
        auto str = GetText(hwnd, FALSE);
        SetText(hwnd, str, FALSE);

        ::KillTimer(hwnd, 999);
        ::SetTimer(hwnd, 999, MARKED_INTERVAL, nullptr);
    }

    VOID OnEdt1(HWND hwnd)
    {
        // テキストを取得する。
        auto str = GetText(hwnd, TRUE);
        SetText(hwnd, str, TRUE);

        ::KillTimer(hwnd, 999);
        ::SetTimer(hwnd, 999, MARKED_INTERVAL, nullptr);
    }

    INT GetCurSel(HWND hwnd)
    {
        INT i = (INT)::SendDlgItemMessageW(hwnd, lst1, LB_GETCURSEL, 0, 0);
        return i;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
        case IDCANCEL:
            // ダイアログを閉じる。
            ::DestroyWindow(hwnd);
            break;
        case edt1:
            if (codeNotify == EN_CHANGE) {
                if (!m_bUpdating)
                    OnEdt1(hwnd);
            }
            break;
        case lst1:
            if (codeNotify == LBN_SELCHANGE) {
                if (!m_bUpdating)
                    OnLst1(hwnd);
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
                    if (iItem == LB_ERR) {
                        xg_iMarkedCand = INT(xg_vMarkedCands.size()) - 1;
                    } else {
                        xg_iMarkedCand = iItem - 1;
                        if (xg_iMarkedCand < 0)
                            xg_iMarkedCand = INT(xg_vMarkedCands.size()) - 1;
                    }
                    auto str = xg_vMarkedCands[xg_iMarkedCand];
                    XgSetMarkedWord(str);
                    XgMarkUpdate();
                    // テキストの更新。
                    SetDlgItemTextW(hwnd, edt1, str.c_str());
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
                    if (iItem == LB_ERR) {
                        xg_iMarkedCand = 0;
                    } else {
                        xg_iMarkedCand = (iItem + 1) % xg_vMarkedCands.size();
                    }
                    auto str = xg_vMarkedCands[xg_iMarkedCand];
                    XgSetMarkedWord(str);
                    XgMarkUpdate();
                    // テキストの更新。
                    SetDlgItemTextW(hwnd, edt1, str.c_str());
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
                    // テキストの更新。
                    SetDlgItemTextW(hwnd, edt1, L"");
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

    void OnMove(HWND hwnd, int x, int y)
    {
        if (m_bInitted) {
            RECT rc;
            ::GetWindowRect(hwnd, &rc);
            xg_nMarkingX = rc.left;
            xg_nMarkingY = rc.top;
        }
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        if (id != 999)
            return;

        ::KillTimer(hwnd, 999);

        auto str1 = GetText(hwnd, TRUE, FALSE);
        auto str2 = GetText(hwnd, TRUE, TRUE);
        if (str1 != str2)
        {
            m_bUpdating = TRUE;
            INT iStart, iEnd;
            ::SendDlgItemMessageW(hwnd, edt1, EM_GETSEL, (WPARAM)&iStart, (LPARAM)&iEnd);
            ::SetDlgItemTextW(hwnd, edt1, str2.c_str());
            ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, iStart, iEnd);
            m_bUpdating = FALSE;
        }
    }

    void OnDestroy(HWND hwnd)
    {
        ::KillTimer(hwnd, 999);
        m_hWnd = nullptr;
        m_bInitted = FALSE;
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        }
        return 0;
    }

    BOOL CreateDialogDx(HWND hwnd)
    {
        if (XG_Dialog::CreateDialogDx(hwnd, IDD_MARKING)) {
            ::ShowWindow(*this, SW_SHOWNORMAL);
            ::UpdateWindow(*this);
            return TRUE;
        }
        return FALSE;
    }
};
