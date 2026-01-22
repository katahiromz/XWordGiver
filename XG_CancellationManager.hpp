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

class XG_CancellationManager {
private:
    XG_Board m_initial_board;   // 初期状態（復元用）(initial state for restoration)

public:
    XG_CancellationManager() noexcept { }
    ~XG_CancellationManager() noexcept { }

    // 初期状態を保存
    // Save initial state for potential restoration
    void SaveInitialState() noexcept {
        m_initial_board = xg_xword;
    }

    // キャンセル時に初期状態に復元
    // Restore initial state on cancellation
    void RestoreOnCancel() noexcept {
        // キャンセルされた場合のみ復元
        // Only restore if cancelled
        if (xg_bCancelled && !xg_bSolved) {
            xg_xword = m_initial_board;
        }
    }

    // リセット（再利用時）
    // Reset for reuse
    void Reset() noexcept {
        m_initial_board.clear();
    }

    // コピー禁止
    XG_CancellationManager(const XG_CancellationManager&) = delete;
    XG_CancellationManager& operator=(const XG_CancellationManager&) = delete;
};

//////////////////////////////////////////////////////////////////////////////
