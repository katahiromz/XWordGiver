#pragma once

#include "XG_WordListDialog.hpp"

// キャンセルダイアログ（単語リストから生成）。
class XG_CancelFromWordsDialog : public XG_Dialog
{
public:
    const DWORD SLEEP = 250;
    const DWORD INTERVAL = 300;
    const UINT uTimerID = 999;

    XG_CancelFromWordsDialog() noexcept
    {
    }

    void Restart(HWND hwnd)
    {
        using namespace crossword_generation;
        reset();
        from_words_t<XGStringW, false>::do_generate(XG_WordListDialog::s_wordset);
        // タイマーをセットする。
        ::SetTimer(hwnd, uTimerID, INTERVAL, nullptr);
    }

    void DoCancel(HWND hwnd) noexcept
    {
        // タイマーを解除する。
        ::KillTimer(hwnd, uTimerID);
        // キャンセルしてスレッドを待つ。
        crossword_generation::s_canceled = true;
        ::Sleep(SLEEP);
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 初期化する。
        ::InterlockedExchange(&xg_nRetryCount, 0);
        // プログレスバーの範囲をセットする。
        ::SendDlgItemMessageW(hwnd, ctl1, PBM_SETRANGE, 0, MAKELPARAM(0, XG_WordListDialog::s_wordset.size() + 1));
        ::SendDlgItemMessageW(hwnd, ctl2, PBM_SETRANGE, 0, MAKELPARAM(0, XG_WordListDialog::s_wordset.size() + 1));
        // 解を求めるのを開始。
        Restart(hwnd);
        // フォーカスをセットする。
        ::SetFocus(::GetDlgItem(hwnd, psh1));
        return FALSE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch(id)
        {
        case psh1:
            // キャンセルする。
            DoCancel(hwnd);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh2:
            // キャンセルする。
            DoCancel(hwnd);
            // 再開する。
            ::InterlockedIncrement(&xg_nRetryCount);
            xg_dwlTick1 = ::GetTickCount64();
            Restart(hwnd);
            break;

        default:
            break;
        }
    }

    void OnSysCommand(HWND hwnd, UINT cmd, int x, int y) noexcept
    {
        if (cmd == SC_CLOSE) {
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
        // プログレスバーを更新する。
        for (DWORD i = 0; i < xg_dwThreadCount; i++) {
            ::SendDlgItemMessageW(hwnd, ctl1 + i, PBM_SETPOS,
                static_cast<WPARAM>(xg_aThreadInfo[i].m_count), 0);
        }
        // 経過時間を表示する。
        {
            WCHAR sz[MAX_PATH];
            const auto dwlTick = ::GetTickCount64();
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_NOWSOLVING),
                            DWORD(dwlTick - xg_dwlTick0) / 1000,
                            DWORD(dwlTick - xg_dwlTick0) / 100 % 10, xg_nRetryCount);
            ::SetDlgItemTextW(hwnd, stc1, sz);
        }
        // 一つ以上のスレッドが終了したか？
        if (crossword_generation::s_generated) {
            // スレッドが終了した。タイマーを解除する。
            ::KillTimer(hwnd, uTimerID);
            // 計算時間を求めるために、終了時間を取得する。
            xg_dwlTick2 = ::GetTickCount64();
            // ダイアログを終了する。
            ::EndDialog(hwnd, IDOK);
        } else {
            // 再計算が必要か？
            if (xg_bAutoRetry && ::GetTickCount64() - xg_dwlTick1 > xg_dwlWait) {
                // キャンセルする。
                DoCancel(hwnd);
                // 再開する。
                ::InterlockedIncrement(&xg_nRetryCount);
                xg_dwlTick1 = ::GetTickCount64();
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
