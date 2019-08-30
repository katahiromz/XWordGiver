//////////////////////////////////////////////////////////////////////////////
// Input.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2019 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, Shift_JIS)

#include "XWordGiver.hpp"

// 入力方向を切り替える。
void __fastcall XgInputDirection(HWND hwnd, INT nDirection)
{
    switch (nDirection) {
    case 1:
        xg_bTateInput = TRUE;
        break;
    case 0:
        xg_bTateInput = FALSE;
        break;
    case -1:
        xg_bTateInput = !xg_bTateInput;
        break;
    }

    if (xg_hwndInputPalette) {
        if (xg_bTateInput) {
            SetDlgItemTextW(xg_hwndInputPalette, 20052, XgLoadStringDx1(1170));
        } else {
            SetDlgItemTextW(xg_hwndInputPalette, 20052, XgLoadStringDx1(1169));
        }
    }

    xg_prev_vk = 0;
}

// 文字送りを切り替える。
void __fastcall XgSetCharFeed(HWND hwnd, INT nMode)
{
    switch (nMode) {
    case 1:
        xg_bCharFeed = TRUE;
        break;
    case 0:
        xg_bCharFeed = FALSE;
        break;
    case -1:
        xg_bCharFeed = !xg_bCharFeed;
        break;
    }

    if (xg_hwndInputPalette) {
        if (xg_bCharFeed) {
            CheckDlgButton(xg_hwndInputPalette, 20050, BST_CHECKED);
        } else {
            CheckDlgButton(xg_hwndInputPalette, 20050, BST_UNCHECKED);
        }
    }

    xg_prev_vk = 0;
}

// 改行する。
void __fastcall XgReturn(HWND hwnd)
{
    if (xg_bTateInput) {
        ++xg_caret_pos.m_j;
        if (xg_caret_pos.m_j >= xg_nCols) {
            xg_caret_pos.m_j = 0;
        }
        xg_caret_pos.m_i = 0;
    } else {
        ++xg_caret_pos.m_i;
        if (xg_caret_pos.m_i >= xg_nRows) {
            xg_caret_pos.m_i = 0;
        }
        xg_caret_pos.m_j = 0;
    }

    XgEnsureCaretVisible(hwnd);
    INT x = XgGetHScrollPos();
    INT y = XgGetVScrollPos();
    XgUpdateImage(hwnd, x, y);

    xg_prev_vk = 0;
}

// 二重マス切り替え。
void __fastcall XgToggleMark(HWND hwnd)
{
    INT i = xg_caret_pos.m_i;
    INT j = xg_caret_pos.m_j;

    // マークされていないか？
    if (XgGetMarked(i, j) == -1) {
        // マークされていないマス。マークをセットする。
        XG_Board *pxw;

        if (xg_bSolved && xg_bShowAnswer)
            pxw = &xg_solution;
        else
            pxw = &xg_xword;

        if (pxw->GetAt(i, j) != ZEN_BLACK)
            XgSetMark(i, j);
        else
            ::MessageBeep(0xFFFFFFFF);
    } else {
        // マークがセットされているマス。マークを解除する。
        XgDeleteMark(i, j);
    }

    // イメージを更新する。
    INT x = XgGetHScrollPos();
    INT y = XgGetVScrollPos();
    XgMarkUpdate();
    XgUpdateImage(hwnd, x, y);
}

// 文字をクリア。
void __fastcall XgClearNonBlocks(HWND hwnd)
{
    xg_caret_pos.clear();

    auto sa1 = std::make_shared<XG_UndoData_SetAll>();
    auto sa2 = std::make_shared<XG_UndoData_SetAll>();
    sa1->Get();

    if (xg_bSolved) {
        xg_bShowAnswer = false;
    }
    for (INT i = 0; i < xg_nRows; ++i) {
        for (INT j = 0; j < xg_nCols; ++j) {
            WCHAR oldch = xg_xword.GetAt(i, j);
            if (oldch != ZEN_BLACK && oldch != ZEN_SPACE) {
                xg_xword.SetAt(i, j, ZEN_SPACE);
            }
        }
    }

    sa2->Get();
    xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);

    INT x = XgGetHScrollPos();
    INT y = XgGetVScrollPos();
    XgUpdateImage(hwnd, x, y);

    xg_prev_vk = 0;
}

// 字送りを実行する。
void __fastcall XgCharFeed(HWND hwnd)
{
    if (!xg_bCharFeed)
        return;

    if (xg_bTateInput) {
        ++xg_caret_pos.m_i;
        if (xg_caret_pos.m_i >= xg_nRows) {
            xg_caret_pos.m_i = 0;
            ++xg_caret_pos.m_j;
            if (xg_caret_pos.m_j >= xg_nCols) {
                xg_caret_pos.m_j = 0;
            }
        }
    } else {
        ++xg_caret_pos.m_j;
        if (xg_caret_pos.m_j >= xg_nCols) {
            xg_caret_pos.m_j = 0;
            ++xg_caret_pos.m_i;
            if (xg_caret_pos.m_i >= xg_nRows) {
                xg_caret_pos.m_i = 0;
            }
        }
    }

    xg_prev_vk = 0;
}

// BackSpaceを実行する。
void __fastcall XgCharBack(HWND hwnd)
{
    if (!xg_bCharFeed) {
        MainWnd_OnChar(hwnd, L'_', 1);
        return;
    }

    if (xg_bTateInput) {
        if (xg_caret_pos.m_i == 0) {
            if (xg_caret_pos.m_j == 0) {
                xg_caret_pos.m_i = xg_nRows - 1;
                xg_caret_pos.m_j = xg_nCols - 1;
            } else {
                xg_caret_pos.m_i = xg_nRows - 1;
                --xg_caret_pos.m_j;
            }
        } else {
            --xg_caret_pos.m_i;
        }
    } else {
        if (xg_caret_pos.m_j == 0) {
            if (xg_caret_pos.m_i == 0) {
                xg_caret_pos.m_j = xg_nCols - 1;
                xg_caret_pos.m_i = xg_nRows - 1;
            } else {
                xg_caret_pos.m_j = xg_nCols - 1;
                --xg_caret_pos.m_i;
            }
        } else {
            --xg_caret_pos.m_j;
        }
    }

    xg_bCharFeed = false;
    MainWnd_OnChar(hwnd, L'_', 1);
    xg_bCharFeed = true;

    xg_prev_vk = 0;
}

// 文字が入力された。
void __fastcall MainWnd_OnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    WCHAR oldch = xg_xword.GetAt(xg_caret_pos);
    if (oldch == ZEN_BLACK && xg_bSolved)
        return;

    int x, y;
    auto sa1 = std::make_shared<XG_UndoData_SetAt>();
    auto sa2 = std::make_shared<XG_UndoData_SetAt>();
    sa1->pos = sa2->pos = xg_caret_pos;
    sa1->ch = oldch;

    if (ch == L'_') {
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        if (!(xg_bSolved && oldch == ZEN_BLACK)) {
            sa2->ch = ZEN_SPACE;
            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, ZEN_SPACE);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
        xg_prev_vk = 0;
    } else if (ch == L' ') {
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        {
            if (oldch == ZEN_SPACE && !xg_bSolved) {
                sa2->ch = ZEN_BLACK;
                xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
                xg_xword.SetAt(xg_caret_pos, ZEN_BLACK);

                if (xg_bCharFeed)
                    XgCharFeed(hwnd);

                XgEnsureCaretVisible(hwnd);
                x = XgGetHScrollPos();
                y = XgGetVScrollPos();
                XgUpdateImage(hwnd, x, y);
            } else if (oldch == ZEN_BLACK && !xg_bSolved) {
                sa2->ch = ZEN_SPACE;
                xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
                xg_xword.SetAt(xg_caret_pos, ZEN_SPACE);

                if (xg_bCharFeed)
                    XgCharFeed(hwnd);

                XgEnsureCaretVisible(hwnd);
                x = XgGetHScrollPos();
                y = XgGetVScrollPos();
                XgUpdateImage(hwnd, x, y);
            } else if (!xg_bSolved || !xg_bShowAnswer) {
                if (oldch != ZEN_BLACK) {
                    sa2->ch = ZEN_SPACE;
                    xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
                    xg_xword.SetAt(xg_caret_pos, ZEN_SPACE);

                    if (xg_bCharFeed)
                        XgCharFeed(hwnd);

                    XgEnsureCaretVisible(hwnd);
                    x = XgGetHScrollPos();
                    y = XgGetVScrollPos();
                    XgUpdateImage(hwnd, x, y);
                }
            }
        }
        xg_prev_vk = 0;
    } else if (ch == L'#') {
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();

        if (!xg_bSolved) {
            sa2->ch = ZEN_BLACK;
            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, ZEN_BLACK);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    } else if (xg_imode == xg_im_ABC) {
        // 英字入力の場合。
        if (::GetAsyncKeyState(VK_CONTROL) < 0) {
            // [Ctrl]キーが押されている。
        } else if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            // 英字小文字を大文字に変換。
            if (XgIsCharHankakuLowerW(ch)) {
                ch = ch + L'A' - L'a';
            }

            // 半角英字を全角英字に変換。
            ch = ZEN_LARGE_A + (ch - L'A');

            sa2->ch = ch;
            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, ch);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else if (XgIsCharZenkakuUpperW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 全角大文字英字が入力された。
            sa2->ch = ch;

            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, ch);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        } else if (XgIsCharZenkakuLowerW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 全角小文字英字が入力された。
            ch = ZEN_LARGE_A + (ch - ZEN_SMALL_A);

            sa2->ch = ch;
            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, ch);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    } else if (xg_imode == xg_im_KANA) {
        WCHAR newch = 0;
        // カナ入力の場合。
        if (::GetAsyncKeyState(VK_CONTROL) < 0) {
            // [Ctrl]キーが押されている。
        }
        else if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch))
        {
            ch = (WCHAR)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)ch);

            // ローマ字入力。
            if (ch == L'A' || ch == L'I' || ch == L'U' ||
                ch == L'E' || ch == L'O' || ch == L'N')
            {
                switch (xg_prev_vk) {
                case 0:
                    switch (ch) {
                    case L'A': newch = ZEN_A; break;
                    case L'I': newch = ZEN_I; break;
                    case L'U': newch = ZEN_U; break;
                    case L'E': newch = ZEN_E; break;
                    case L'O': newch = ZEN_O; break;
                    case L'N': xg_prev_vk = ch; break;
                    }
                    break;
                case L'K':
                    switch (ch) {
                    case L'A': newch = ZEN_KA; break;
                    case L'I': newch = ZEN_KI; break;
                    case L'U': newch = ZEN_KU; break;
                    case L'E': newch = ZEN_KE; break;
                    case L'O': newch = ZEN_KO; break;
                    }
                    break;
                case L'S':
                    switch (ch) {
                    case L'A': newch = ZEN_SA; break;
                    case L'I': newch = ZEN_SI; break;
                    case L'U': newch = ZEN_SU; break;
                    case L'E': newch = ZEN_SE; break;
                    case L'O': newch = ZEN_SO; break;
                    }
                    break;
                case L'T':
                    switch (ch) {
                    case L'A': newch = ZEN_TA; break;
                    case L'I': newch = ZEN_CHI; break;
                    case L'U': newch = ZEN_TSU; break;
                    case L'E': newch = ZEN_TE; break;
                    case L'O': newch = ZEN_TO; break;
                    }
                    break;
                case L'N':
                    switch (ch) {
                    case L'A': newch = ZEN_NA; break;
                    case L'I': newch = ZEN_NI; break;
                    case L'U': newch = ZEN_NU; break;
                    case L'E': newch = ZEN_NE; break;
                    case L'O': newch = ZEN_NO; break;
                    case L'N': newch = ZEN_NN; break;
                    }
                    break;
                case L'H':
                    switch (ch) {
                    case L'A': newch = ZEN_HA; break;
                    case L'I': newch = ZEN_HI; break;
                    case L'U': newch = ZEN_FU; break;
                    case L'E': newch = ZEN_HE; break;
                    case L'O': newch = ZEN_HO; break;
                    }
                    break;
                case L'F':
                    switch (ch) {
                    case L'U': newch = ZEN_FU; break;
                    case L'O': newch = ZEN_HO; break;
                    }
                    break;
                case L'M':
                    switch (ch) {
                    case L'A': newch = ZEN_MA; break;
                    case L'I': newch = ZEN_MI; break;
                    case L'U': newch = ZEN_MU; break;
                    case L'E': newch = ZEN_ME; break;
                    case L'O': newch = ZEN_MO; break;
                    }
                    break;
                case L'Y':
                    switch (ch) {
                    case L'A': newch = ZEN_YA; break;
                    case L'I': newch = ZEN_I; break;
                    case L'U': newch = ZEN_YU; break;
                    case L'E': newch = ZEN_E; break;
                    case L'O': newch = ZEN_YO; break;
                    }
                    break;
                case L'R':
                    switch (ch) {
                    case L'A': newch = ZEN_RA; break;
                    case L'I': newch = ZEN_RI; break;
                    case L'U': newch = ZEN_RU; break;
                    case L'E': newch = ZEN_RE; break;
                    case L'O': newch = ZEN_RO; break;
                    }
                    break;
                case L'W':
                    switch (ch) {
                    case L'A': newch = ZEN_WA; break;
                    case L'I': newch = ZEN_WI; break;
                    case L'U': newch = ZEN_U; break;
                    case L'E': newch = ZEN_WE; break;
                    case L'O': newch = ZEN_WO; break;
                    }
                    break;
                case L'G':
                    switch (ch) {
                    case L'A': newch = ZEN_GA; break;
                    case L'I': newch = ZEN_GI; break;
                    case L'U': newch = ZEN_GU; break;
                    case L'E': newch = ZEN_GE; break;
                    case L'O': newch = ZEN_GO; break;
                    }
                    break;
                case L'Z':
                    switch (ch) {
                    case L'A': newch = ZEN_ZA; break;
                    case L'I': newch = ZEN_JI; break;
                    case L'U': newch = ZEN_ZU; break;
                    case L'E': newch = ZEN_ZE; break;
                    case L'O': newch = ZEN_ZO; break;
                    }
                    break;
                case L'J':
                    switch (ch) {
                    case L'I': newch = ZEN_JI; break;
                    case L'E': newch = ZEN_ZE; break;
                    }
                    break;
                case L'D':
                    switch (ch) {
                    case L'A': newch = ZEN_DA; break;
                    case L'I': newch = ZEN_DI; break;
                    case L'U': newch = ZEN_DU; break;
                    case L'E': newch = ZEN_DE; break;
                    case L'O': newch = ZEN_DO; break;
                    }
                    break;
                case L'B':
                    switch (ch) {
                    case L'A': newch = ZEN_BA; break;
                    case L'I': newch = ZEN_BI; break;
                    case L'U': newch = ZEN_BU; break;
                    case L'E': newch = ZEN_BE; break;
                    case L'O': newch = ZEN_BO; break;
                    }
                    break;
                case L'P':
                    switch (ch) {
                    case L'A': newch = ZEN_PA; break;
                    case L'I': newch = ZEN_PI; break;
                    case L'U': newch = ZEN_PU; break;
                    case L'E': newch = ZEN_PE; break;
                    case L'O': newch = ZEN_PO; break;
                    }
                    break;
                }
                if (newch)
                    xg_prev_vk = 0;
            } else if (xg_prev_vk == L'C' && ch == 'H') {
                xg_prev_vk = L'T';
            } else {
                xg_prev_vk = ch;
            }
        } else if (XgIsCharHiraganaW(ch)) {
            // ひらがな直接入力。
            std::array<WCHAR,2> sz;
            ::LCMapStringW(JPN_LOCALE,
                LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                &ch, 1, sz.data(), static_cast<int>(sz.size()));
            newch = sz[0];
            goto katakana;
        } else if (XgIsCharKatakanaW(ch)) {
katakana:;
            // カタカナ直接入力。
            // 小さな字を大きな字にする。
            for (size_t i = 0; i < xg_small.size(); i++) {
                if (static_cast<WCHAR>(ch) == xg_small[i][0]) {
                    newch = xg_large[i][0];
                    break;
                }
            }
            if (newch) {
                xg_prev_vk = 0;
            }
        } else if (ch == '-' || ch == ZEN_PROLONG) {
            newch = ZEN_PROLONG;
            xg_prev_vk = 0;
        } else {
            xg_prev_vk = 0;
        }

        if (newch) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            sa2->ch = newch;
            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, newch);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    } else if (xg_imode == xg_im_HANGUL) {
        // ハングル入力の場合。
        if (XgIsCharHangulW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // ハングル直接入力。
            sa2->ch = ch;
            xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
            xg_xword.SetAt(xg_caret_pos, ch);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    }
}

// キーが押された。
void __fastcall MainWnd_OnKey(HWND hwnd, UINT vk, bool fDown, int /*cRepeat*/, UINT /*flags*/)
{
    int x, y;
    WCHAR ch;

    // 特定の条件において、キー入力を拒否する。
    if (!fDown)
        return;

    // 仮想キーコードに応じて、処理する。
    switch (vk) {
    case VK_APPS:   // アプリ キー。
        // アプリメニューを表示する。
        {
            // 編集メニューを取得する。
            HMENU hMenu = ::GetMenu(hwnd);
            HMENU hEditMenu = ::GetSubMenu(hMenu, 1);

            // 現在のキャレット位置。
            POINT pt;
            pt.x = xg_nMargin + xg_caret_pos.m_j * xg_nCellSize;
            pt.y = xg_nMargin + xg_caret_pos.m_i * xg_nCellSize;
            pt.x += xg_nCellSize / 2;
            pt.y += xg_nCellSize / 2;

            // ツールバーが可視ならば、位置を補正する。
            RECT rc;
            if (::IsWindowVisible(xg_hToolBar)) {
                ::GetWindowRect(xg_hToolBar, &rc);
                pt.y += (rc.bottom - rc.top);
            }

            // スクリーン座標に変換する。
            ::ClientToScreen(hwnd, &pt);

            // 右クリックメニューを表示する。
            ::SetForegroundWindow(hwnd);
            ::TrackPopupMenu(hEditMenu, TPM_LEFTALIGN,
                             pt.x, pt.y, 0, hwnd, NULL);
            ::PostMessageW(hwnd, WM_NULL, 0, 0);
        }
        xg_prev_vk = 0;
        break;

    case VK_PRIOR:  // PgUpキー。
        ::SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
        xg_prev_vk = 0;
        break;

    case VK_NEXT:   // PgDnキー。
        ::SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
        xg_prev_vk = 0;
        break;

    case VK_DELETE:
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        // 現在のキャレット位置のマスの中身を消去する。
        {
            bool flag = false;
            auto sa1 = std::make_shared<XG_UndoData_SetAt>();
            auto sa2 = std::make_shared<XG_UndoData_SetAt>();
            sa1->pos = sa2->pos = xg_caret_pos;
            sa1->ch = ch = xg_xword.GetAt(xg_caret_pos);

            if (ch != ZEN_SPACE && !xg_bSolved) {
                sa1->ch = ZEN_SPACE;
                xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
                xg_xword.SetAt(xg_caret_pos, ZEN_SPACE);
                flag = true;

                XgEnsureCaretVisible(hwnd);
                x = XgGetHScrollPos();
                y = XgGetVScrollPos();
                XgUpdateImage(hwnd, x, y);
            }
        }
        xg_prev_vk = 0;
        break;

    default:
        break;
    }
}

// IMEから文字が入力された。
void __fastcall MainWnd_OnImeChar(HWND hwnd, WCHAR ch, LPARAM /*lKeyData*/)
{
    int x, y;

    if (ch == ZEN_BLACK) {
        SendMessageW(hwnd, WM_CHAR, L'#', 1);
        return;
    }

    if (ch == ZEN_SPACE) {
        SendMessageW(hwnd, WM_CHAR, L' ', 1);
        return;
    }

    if (ch == L'_') {
        SendMessageW(hwnd, WM_CHAR, L'_', 1);
        return;
    }

    // 解があるとき、黒は上書きできない。
    WCHAR oldch = xg_xword.GetAt(xg_caret_pos);
    if (xg_bSolved && oldch == ZEN_BLACK) {
        return;
    }

    if (xg_imode == xg_im_KANJI) {
        // 漢字入力モードの場合。
        // 漢字か？
        if (XgIsCharKanjiW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            xg_xword.SetAt(xg_caret_pos, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);
            
            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    } else if (xg_imode == xg_im_HANGUL) {
        // ハングル入力モードの場合。
        // ハングルか？
        if (XgIsCharHangulW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            xg_xword.SetAt(xg_caret_pos, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    } else if (xg_imode == xg_im_KANA) {
        // カナ入力モードの場合。
        if (XgIsCharHiraganaW(ch)) {
            // ひらがな入力。
            std::array<WCHAR,2> sz;
            LCMapStringW(JPN_LOCALE,
                LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                &ch, 1, sz.data(), static_cast<int>(sz.size()));
            ch = sz[0];
            goto katakana;
        } else if (XgIsCharKatakanaW(ch)) {
katakana:;
            // カタカナ入力。
            // 小さな字を大きな字にする。
            for (size_t i = 0; i < xg_small.size(); i++) {
                if (ch == xg_small[i][0]) {
                    ch = xg_large[i][0];
                    break;
                }
            }
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 文字を設定する。
            xg_xword.SetAt(xg_caret_pos, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            x = XgGetHScrollPos();
            y = XgGetVScrollPos();
            XgUpdateImage(hwnd, x, y);
        }
    }
}

// 入力パレットの初期化。
BOOL InputPal_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    xg_hwndInputPalette = hwnd;
    XgInputDirection(hwnd, xg_bTateInput);
    XgSetCharFeed(hwnd, xg_bCharFeed);
    return FALSE;
}

// 入力パレットのコマンドが来た。
void InputPal_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id) {
    case IDOK:
    case IDCANCEL:
    case IDCLOSE:
        DestroyWindow(hwnd);
        return;
    }

    HWND hButton = GetDlgItem(hwnd, id);

    WCHAR sz[5];
    GetWindowTextW(hButton, sz, 5);

    if (sz[0] && sz[1] == 0) {
        switch (sz[0]) {
        case ZEN_SPACE:
            MainWnd_OnImeChar(xg_hMainWnd, L'_', 0);
            break;
        case ZEN_UP:
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTUPPER, 0);
            } else {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_UP, 0);
            }
            break;
        case ZEN_DOWN:
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTLOWER, 0);
            } else {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_DOWN, 0);
            }
            break;
        case ZEN_LEFT:
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTLEFT, 0);
            } else {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_LEFT, 0);
            }
            break;
        case ZEN_RIGHT:
            if (GetAsyncKeyState(VK_CONTROL) < 0) {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_MOSTRIGHT, 0);
            } else {
                SendMessageW(xg_hMainWnd, WM_COMMAND, ID_RIGHT, 0);
            }
            break;
        default:
            MainWnd_OnImeChar(xg_hMainWnd, sz[0], 0);
            break;
        }
    } else {
        switch (id) {
        case 10093: // 辞書
            SendMessageW(xg_hMainWnd, WM_COMMAND, ID_ONLINEDICT, 0);
            break;
        case 20050: // 文字送り
            SendMessageW(xg_hMainWnd, WM_COMMAND, ID_CHARFEED, 0);
            break;
        case 20052: // ﾀﾃ/ﾖｺ入力
            SendMessageW(xg_hMainWnd, WM_COMMAND, ID_INPUTHV, 0);
            break;
        case 20054: // BS
            XgCharBack(xg_hMainWnd);
            break;
        case 20064: // NL
            XgReturn(xg_hMainWnd);
            break;
        }
    }

    ::SetForegroundWindow(xg_hMainWnd);
}

void InputPal_OnDestroy(HWND hwnd)
{
    xg_hwndInputPalette = NULL;
}

// キーが押された。
void InputPal_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    if (!fDown)
        return;

    switch (vk) {
    case VK_F6:
        if (::GetAsyncKeyState(VK_SHIFT) < 0)
            ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
        else
            ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
        break;

    case VK_ESCAPE:
        DestroyWindow(hwnd);
        break;
    }
}

// 入力パレットのダイアログプロシジャー。
INT_PTR CALLBACK
XgInputPaletteDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, InputPal_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, InputPal_OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, InputPal_OnDestroy);
        HANDLE_MSG(hwnd, WM_KEYDOWN, InputPal_OnKey);
    }
    return 0;
}

// 入力パレットを破棄する。
BOOL XgDestroyInputPalette(void)
{
    if (xg_hwndInputPalette) {
        ::DestroyWindow(xg_hwndInputPalette);
        xg_hwndInputPalette = NULL;
        return TRUE;
    }
    return FALSE;
}

// 入力パレットを作成する。
BOOL XgCreateInputPalette(HWND hwndOwner)
{
    XgDestroyInputPalette();

    CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(14), hwndOwner,
                  XgInputPaletteDlgProc);

    ShowWindow(xg_hwndInputPalette, SW_SHOWNOACTIVATE);
    UpdateWindow(xg_hwndInputPalette);

    return xg_hwndInputPalette != NULL;
}
