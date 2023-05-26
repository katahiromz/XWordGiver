#pragma once

#include "XG_Window.hpp"

// キャンセルダイアログ（スマート解決）。
class XG_CancelSmartSolveDialog : public XG_Dialog
{
public:
    const DWORD SLEEP = 250;
    const DWORD INTERVAL = 300;
    const UINT uTimerID = 999;

    XG_CancelSmartSolveDialog() noexcept
    {
    }

    void Restart(HWND hwnd)
    {
        // タイマーを解除する。
        ::KillTimer(hwnd, uTimerID);
        xg_dwlTick1 = ::GetTickCount64();
        XgStartSolve_Smart();
        // タイマーをセットする。
        ::SetTimer(hwnd, uTimerID, INTERVAL, nullptr);
    }

    void DoCancel(HWND hwnd)
    {
        // タイマーを解除する。
        ::KillTimer(hwnd, uTimerID);
        // キャンセルしてスレッドを待つ。
        xg_bCancelled = true;
        XgWaitForThreads();
        // スレッドを閉じる。
        XgCloseThreads();
    }

    void DoRetry(HWND hwnd)
    {
        // キャンセルする。
        DoCancel(hwnd);

        ::InterlockedIncrement(&xg_nRetryCount);
        Restart(hwnd);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 初期化する。
        ::InterlockedExchange(&xg_nRetryCount, 0);
        // プログレスバーの範囲をセットする。
        ::SendDlgItemMessageW(hwnd, ctl1, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        ::SendDlgItemMessageW(hwnd, ctl2, PBM_SETRANGE, 0,
                            MAKELPARAM(0, xg_nRows * xg_nCols));
        // 計算時間を求めるために、開始時間を取得する。
        xg_dwlTick0 = xg_dwlTick1 = ::GetTickCount64();
        // 再計算までの時間を概算する。
        xg_dwlWait = XgGetRetryInterval();
        // 解を求めるのを開始。
        Restart(hwnd);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        return FALSE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case psh1:
            // キャンセルする。
            DoCancel(hwnd);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        case psh2:
            // 再計算しなおす。
            DoRetry(hwnd);
            break;
        default:
            break;
        }
    }

    void OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
    {
        if (cmd == SC_CLOSE) {
            DoCancel(hwnd);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
        }
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        // プログレスバーを更新する。
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            ::SendDlgItemMessageW(hwnd, ctl1 + i, PBM_SETPOS,
                static_cast<WPARAM>(xg_aThreadInfo[i].m_count), 0);
        }
        // 経過時間を表示する。
        {
            WCHAR sz[MAX_PATH];
            DWORDLONG dwlTick = ::GetTickCount64();
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_NOWSOLVING),
                static_cast<DWORD>(dwlTick - xg_dwlTick0) / 1000,
                static_cast<DWORD>(dwlTick - xg_dwlTick0) / 100 % 10, xg_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }
        // 一つ以上のスレッドが終了したか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, uTimerID);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
        } else {
            // 再計算が必要か？
            if (xg_bAutoRetry && ::GetTickCount64() - xg_dwlTick1 > xg_dwlWait) {
                DoRetry(hwnd);
            }
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_SYSCOMMAND, OnSysCommand);
            HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_CALCULATING);
    }
};
