//////////////////////////////////////////////////////////////////////////////
// XG_CancellationManager.hpp --- Cancellation Manager
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#pragma once

#include "XWordGiver.hpp"

//////////////////////////////////////////////////////////////////////////////
// キャンセル管理クラス（軽量設計、実行速度への影響を最小化）
// Lightweight cancellation manager to ensure stable state during cancellation
// without impacting execution speed.

class XG_CancellationManager
{
private:
    HANDLE m_hCompletionEvent;  // 完了通知用イベント（completion event for thread waiting only）
    XG_Board m_initial_board;   // 初期状態（復元用）(initial state for restoration)
    
public:
    // コンストラクタ
    XG_CancellationManager() noexcept
        : m_hCompletionEvent(nullptr)
    {
        // 手動リセット、初期状態は非シグナル状態のイベントを作成
        // Create manual-reset event, initially non-signaled
        m_hCompletionEvent = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }
    
    // デストラクタ
    ~XG_CancellationManager() noexcept
    {
        if (m_hCompletionEvent != nullptr) {
            ::CloseHandle(m_hCompletionEvent);
            m_hCompletionEvent = nullptr;
        }
    }
    
    // 初期状態を保存
    // Save initial state for potential restoration
    void SaveInitialState() noexcept
    {
        m_initial_board = xg_xword;
    }
    
    // 完了を通知（処理成功時）
    // Notify completion (on successful processing)
    void SetCompleted() noexcept
    {
        ::EnterCriticalSection(&xg_csLock);
        // キャンセル済みでなければ完了を通知
        if (!xg_bCancelled && m_hCompletionEvent != nullptr) {
            ::SetEvent(m_hCompletionEvent);
        }
        ::LeaveCriticalSection(&xg_csLock);
    }
    
    // 完了またはキャンセルを待機（タイムアウト付き）
    // Wait for completion or cancellation with timeout
    // Returns true if completed normally, false if timeout or cancelled
    bool WaitForCompletion(DWORD dwMilliseconds = 5000) const noexcept
    {
        if (m_hCompletionEvent == nullptr)
            return false;
            
        DWORD dwResult = ::WaitForSingleObject(m_hCompletionEvent, dwMilliseconds);
        return (dwResult == WAIT_OBJECT_0);
    }
    
    // キャンセル時に初期状態に復元
    // Restore initial state on cancellation
    void RestoreOnCancel() noexcept
    {
        // キャンセルされた場合のみ復元
        // Only restore if cancelled
        if (xg_bCancelled && !xg_bSolved) {
            xg_xword = m_initial_board;
        }
    }
    
    // リセット（再利用時）
    // Reset for reuse
    void Reset() noexcept
    {
        if (m_hCompletionEvent != nullptr) {
            ::ResetEvent(m_hCompletionEvent);
        }
        m_initial_board.clear();
    }
    
    // コピー禁止
    XG_CancellationManager(const XG_CancellationManager&) = delete;
    XG_CancellationManager& operator=(const XG_CancellationManager&) = delete;
};

//////////////////////////////////////////////////////////////////////////////
