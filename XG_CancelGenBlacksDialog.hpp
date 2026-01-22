#pragma once

#include "XG_Window.hpp"
#include "XG_CancellationManager.hpp"

// キャンセルダイアログ。
class XG_CancelGenBlacksDialog : public XG_Dialog
{
public:
    const DWORD SLEEP = 250;
    const DWORD INTERVAL = 300;
    const UINT uTimerID = 999;
    XG_CancellationManager m_cancellation_manager;

    XG_CancelGenBlacksDialog() noexcept
    {
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
        m_cancellation_manager.SetCompleted();
        ::LeaveCriticalSection(&xg_csLock);
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

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // リセット。
        m_cancellation_manager.Reset();
        // 初期状態を保存。
        m_cancellation_manager.SaveInitialState();
        // グローバルポインタを設定。
        xg_pCancellationManager = &m_cancellation_manager;
        // 解を求めるのを開始。
        XgStartGenerateBlacks();
        // リトライ回数をリセット。
        ::InterlockedExchange(&xg_nRetryCount, 0);
        // タイマーをセットする。
        ::SetTimer(hwnd, uTimerID, INTERVAL, nullptr);
        // 生成したパターンの個数を表示する。
        if (xg_nNumberGenerated > 0) {
            WCHAR sz[MAX_PATH];
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_PATMAKING), xg_nNumberGenerated);
            ::SetDlgItemTextW(hwnd, stc2, sz);
        }
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        return FALSE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept
    {
        switch (id)
        {
        case psh1:
            // キャンセルする。
            DoCancel(hwnd);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;
        default:
            break;
        }
    }

    void OnSysCommand(HWND hwnd, UINT cmd, int x, int y) noexcept
    {
        if (cmd == SC_CLOSE)
        {
            // キャンセルする。
            DoCancel(hwnd);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
        }
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        {
            // 経過時間を表示する。
            WCHAR sz[MAX_PATH];
            const auto dwTick = ::GetTickCount64();
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_CALCULATING),
                            DWORD(dwTick - xg_dwlTick0) / 1000,
                            DWORD(dwTick - xg_dwlTick0) / 100 % 10);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }

        // 終了したスレッドがあるか？
        if (XgIsAnyThreadTerminated()) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, uTimerID);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // スレッドを待つ。
            XgWaitForThreads();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
            // スレッドを閉じる。
            XgCloseThreads();
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
