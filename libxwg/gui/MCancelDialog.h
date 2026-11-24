// MCancelDialog.h --- 「キャンセル」ダイアログ。
// Author: katahiromz
// License: MIT
#pragma once

#include "MWindowBase.h"
#include <commctrl.h>
#include <strsafe.h>
#include <atomic>
#include <cassert>
#include <functional>
#include <process.h>

// 使い方:
//   MCancelDialog dialog;
//   dialog.m_fnRun    = []() -> HRESULT { ... };       // 必須。ワーカースレッドで実行。
//   dialog.m_fnCancel = []() { ... };                  // 任意。UIスレッドで実行。
//   dialog.m_fnEnd    = [](HRESULT hr) { ... };        // 任意。UIスレッドで実行。
//   INT_PTR id = dialog.DoModal(hwnd);
//
// 注意:
//   - m_fnRun は m_bCanceled を定期的に確認し、TRUE なら速やかに戻ること。
//   - m_fnRun の中から UIスレッドへ SendMessage 等で同期的に待つ処理をしないこと
//     (ダイアログ破棄時にUIスレッドがスレッド終了を待つため、デッドロックする)。
//   - m_fnEnd は、キャンセルされた場合も含めて、スレッド終了後に必ず1回呼ばれる。
//   - m_fnEnd が未設定のときは、既定で EndDialog する
//     (キャンセル → IDCANCEL、失敗 → IDABORT、それ以外 → IDOK)。
//   - m_fnEnd を設定した場合は、その中で EndDialog を呼ぶこと。
//   - プログレスバーのマーキーには Common Controls 6.0 (マニフェスト) が必要。
class MCancelDialog : public MDialogBase {
public:
	enum {
		INTERVAL = 300,            // 経過時間表示の更新間隔 (ms)
		MARQUEE_INTERVAL = 30,     // マーキーのアニメーション間隔 (ms)
		TIMER_ID = 999,
		WM_THREAD_DONE = WM_APP + 1,
	};
	HANDLE m_hThread = nullptr;
	HWND m_hDlg = nullptr;
	ULONGLONG m_nStartTick = 0;
	std::atomic<bool> m_bCanceled{ false };
	HRESULT m_hr = E_FAIL; // ワーカーが書き込み、WM_THREAD_DONE 後にUIスレッドが読む。
	std::function<HRESULT(void)> m_fnRun;
	std::function<void(void)> m_fnCancel;
	std::function<void(HRESULT hr)> m_fnEnd;

	MCancelDialog() noexcept { }

	~MCancelDialog() {
		JoinThread();
	}

	MCancelDialog(const MCancelDialog&) = delete;
	MCancelDialog& operator=(const MCancelDialog&) = delete;

	// スレッドの終了を待ち、ハンドルを閉じる。
	void JoinThread() {
		if (m_hThread) {
			::WaitForSingleObject(m_hThread, INFINITE);
			::CloseHandle(m_hThread);
			m_hThread = nullptr;
		}
	}

	// タイマーとマーキーを止める。
	void QuitTimers(HWND hwnd) {
		::KillTimer(hwnd, TIMER_ID);
		::SendDlgItemMessageW(hwnd, ctl1, PBM_SETMARQUEE, FALSE, 0);
		::SendDlgItemMessageW(hwnd, ctl2, PBM_SETMARQUEE, FALSE, 0);
	}

	// ワーカースレッド。
	static unsigned __stdcall ThreadProc(void *arg) {
		auto dialog = static_cast<MCancelDialog*>(arg);
		assert(dialog);
		HRESULT hr = E_FAIL;
		try {
			if (dialog->m_fnRun)
				hr = dialog->m_fnRun();
		} catch (...) {
			hr = E_FAIL;
		}
		dialog->m_hr = hr;
		::PostMessageW(dialog->m_hDlg, WM_THREAD_DONE, 0, 0);
		return 0;
	}

	// キャンセルする。終了処理は WM_THREAD_DONE で行う。
	void DoCancel(HWND hwnd) {
		if (m_bCanceled.exchange(true))
			return; // すでにキャンセル済み。
		::EnableWindow(::GetDlgItem(hwnd, psh1), FALSE);
		if (m_fnCancel)
			m_fnCancel();
	}

	// WM_THREAD_DONE
	void OnThreadDone(HWND hwnd) {
		QuitTimers(hwnd);
		JoinThread();
		if (m_fnEnd) {
			m_fnEnd(m_hr);
		} else if (m_bCanceled) {
			::EndDialog(hwnd, IDCANCEL);
		} else {
			::EndDialog(hwnd, FAILED(m_hr) ? IDABORT : IDOK);
		}
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		m_hDlg = hwnd;

		// 実行関数が未設定なら中止する。
		if (!m_fnRun) {
			assert(0);
			::EndDialog(hwnd, IDABORT);
			return FALSE;
		}

		// ダイアログを中央へ移動する。
		CenterWindowDx(hwnd);
		// フォーカスをセットする。
		::SetFocus(::GetDlgItem(hwnd, psh1));

		// マーキー状態にする。
		for (INT id : { ctl1, ctl2 }) {
			HWND hCtl = ::GetDlgItem(hwnd, id);
			LONG_PTR style = ::GetWindowLongPtrW(hCtl, GWL_STYLE);
			::SetWindowLongPtrW(hCtl, GWL_STYLE, (style | PBS_MARQUEE));
			::SendDlgItemMessageW(hwnd, id, PBM_SETMARQUEE, TRUE, MARQUEE_INTERVAL);
		}

		// 経過時間表示用のタイマーを開始する。
		m_bCanceled = false;
		m_nStartTick = ::GetTickCount64();
		::SetTimer(hwnd, TIMER_ID, INTERVAL, nullptr);

		// 最後にスレッドを開始する。
		m_hThread = (HANDLE)_beginthreadex(nullptr, 0, ThreadProc, this, 0, nullptr);
		if (!m_hThread) {
			QuitTimers(hwnd);
			::EndDialog(hwnd, IDABORT);
			return FALSE;
		}
		return FALSE; // フォーカスは自前でセットした。
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case psh1:
		case IDCANCEL: // Escキー
			DoCancel(hwnd);
			break;
		default:
			break;
		}
	}

	// WM_TIMER
	void OnTimer(HWND hwnd, UINT id) {
		if (id != TIMER_ID)
			return;
		// 経過時間を表示する。
		WCHAR text[MAX_PATH];
		DWORD dwElapsed = DWORD(::GetTickCount64() - m_nStartTick);
		StringCchPrintfW(text, _countof(text), XgLoadString(IDS_NOWSOLVING).c_str(),
		                 dwElapsed / 1000, dwElapsed / 100 % 10);
		::SetDlgItemTextW(hwnd, stc1, text);
	}

	// WM_DESTROY
	void OnDestroy(HWND hwnd) {
		// 想定外の経路で破棄されても、スレッドを止めてから後始末する。
		m_bCanceled = true;
		JoinThread();
		QuitTimers(hwnd);
		m_hDlg = nullptr;
	}

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		case WM_SYSCOMMAND:
			// 閉じるボタンだけをキャンセル扱いにし、移動などは既定処理に任せる。
			if ((wParam & 0xFFF0) == SC_CLOSE) {
				DoCancel(hwnd);
				::SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 0);
				return TRUE;
			}
			return FALSE;
		case WM_THREAD_DONE:
			OnThreadDone(hwnd);
			return TRUE;
		default:
			break;
		}
		return 0;
	}

	INT_PTR DoModal(HWND hwnd) {
		return DialogBoxDx(hwnd, IDD_CALCULATING);
	}
};
