#pragma once

#include "XG_Window.hpp"
#include "XG_CancellationManager.hpp"

// キャンセルダイアログ（黒マス追加なし）。
class XG_CancelSolveNoAddBlackDialog : public XG_Dialog
{
public:
    const DWORD SLEEP = 250;
    const DWORD INTERVAL = 300;
    const UINT uTimerID = 999;
    XG_CancellationManager m_cancellation_manager;

    XG_CancelSolveNoAddBlackDialog() noexcept
    {
    }

    void Restart(HWND hwnd) noexcept
    {
        xg_dwlTick1 = ::GetTickCount64();
        // リセット。
        m_cancellation_manager.Reset();
        // 初期状態を保存。
        m_cancellation_manager.SaveInitialState();
        // グローバルポインタを設定。
        xg_pCancellationManager = &m_cancellation_manager;
        // スマート解決なら、黒マスを生成する。
        XgStartSolve_NoAddBlack();
        // タイマーをセットする。
        ::SetTimer(hwnd, uTimerID, INTERVAL, nullptr);
    }

    void DoCancel(HWND hwnd) noexcept
    {
        // タイマーを解除する。
        ::KillTimer(hwnd, uTimerID);
        // キャンセルしてスレッドを待つ。
        ::EnterCriticalSection(&xg_csLock);
        // 生成完了していなければキャンセル
        if (!xg_bSolved) {
            xg_bCancelled = true;
        }
        ::LeaveCriticalSection(&xg_csLock);
        m_cancellation_manager.SetCompleted();
        // スレッドを待つ（タイムアウト付き）。
        if (!m_cancellation_manager.WaitForCompletion(5000)) {
            // タイムアウトの場合は通常の待機。
            XgWaitForThreads();
        }
        // キャンセル時に初期状態に復元。
        m_cancellation_manager.RestoreOnCancel();
        // グローバルポインタをクリア。
        xg_pCancellationManager = nullptr;
        // スレッドを閉じる。
        XgCloseThreads();
    }

    void DoRetry(HWND hwnd) noexcept
    {
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
        // 再計算までの時間を概算する。
        xg_dwlWait = XgGetRetryInterval();
        // 解を求めるのを開始。
        Restart(hwnd);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        // 生成した問題の個数を表示する。
        if (xg_nNumberGenerated > 0) {
            WCHAR sz[MAX_PATH];
            StringCchPrintfW(sz, _countof(sz), XgLoadStringDx1(IDS_PROBLEMSMAKING),
                             xg_nNumberGenerated);
            ::SetDlgItemTextW(hwnd, stc2, sz);
        }
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
            // キャンセルする。
            DoCancel(hwnd);
            // 再計算の回数を増やす。
            ::InterlockedIncrement(&xg_nRetryCount);
            // 再計算する。
            Restart(hwnd);
            break;
        default:
            break;
        }
    }

    void OnSysCommand(HWND hwnd, UINT cmd, int x, int y)
    {
        if (cmd == SC_CLOSE)
        {
            // キャンセルする。
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
            const auto dwTick = ::GetTickCount64();
            StringCchPrintfW(sz, _countof(sz), XgLoadStringDx1(IDS_NOWSOLVING),
                             DWORD(dwTick - xg_dwlTick0) / 1000,
                             DWORD(dwTick - xg_dwlTick0) / 100 % 10, xg_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }
        // 一つ以上のスレッドが終了したか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, uTimerID);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // スレッドを待つ。
            XgWaitForThreads();
            // 解を求めようとした後の後処理。
            XgEndSolve();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
        } else {
            // 再計算が必要か？
            if (xg_bAutoRetry && ::GetTickCount64() - xg_dwlTick1 > xg_dwlWait) {
                // キャンセルする。
                DoCancel(hwnd);
                // 再計算の回数を増やす。
                ::InterlockedIncrement(&xg_nRetryCount);
                // 再計算する。
                Restart(hwnd);
            }
        }
    }

    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
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
