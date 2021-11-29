//////////////////////////////////////////////////////////////////////////////
// Input.cpp --- XWord Giver (Japanese Crossword Generator)
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

#include "XWordGiver.hpp"
#include "GUI.hpp"
#include "XG_UndoBuffer.hpp"

// 入力パレットを表示するか？
bool xg_bShowInputPalette = false;

// 直前のアクセント記号。
WCHAR xg_chAccent = 0;

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
        if (xg_bTateOki) {
            if (xg_bTateInput) {
                SetDlgItemTextW(xg_hwndInputPalette, 20052, XgLoadStringDx1(IDS_VINPUT));
            } else {
                SetDlgItemTextW(xg_hwndInputPalette, 20052, XgLoadStringDx1(IDS_HINPUT));
            }
        } else {
            if (xg_bTateInput) {
                SetDlgItemTextW(xg_hwndInputPalette, 20052, XgLoadStringDx1(IDS_VINPUT2));
            } else {
                SetDlgItemTextW(xg_hwndInputPalette, 20052, XgLoadStringDx1(IDS_HINPUT2));
            }
        }
    }

    XgUpdateStatusBar(hwnd);
    xg_prev_vk = 0;
    xg_chAccent = 0;
}

// 文字と「元に戻す」情報をセット。
void __fastcall XgSetChar(HWND hwnd, WCHAR ch)
{
    auto sa1 = std::make_shared<XG_UndoData_SetAt>();
    sa1->pos = xg_caret_pos;
    sa1->ch = xg_xword.GetAt(xg_caret_pos);

    auto sa2 = std::make_shared<XG_UndoData_SetAt>();
    sa2->pos = xg_caret_pos;
    sa2->ch = ch;

    xg_ubUndoBuffer.Commit(UC_SETAT, sa1, sa2);
    xg_xword.SetAt(xg_caret_pos, ch);
}

// アクセント記号付きの文字にする。
WCHAR XgConvertAccent(WCHAR chAccent, WCHAR ch)
{
    switch (chAccent) {
    case L'^':
        switch (ch) {
        case L'A': case L'a':
            ch = 0x00C2; // Â
            break;
        case L'I': case L'i':
            ch = 0x00CE; // Î
            break;
        case L'U': case L'u':
            ch = 0x00DB; // Û
            break;
        case L'E': case L'e':
            ch = 0x00CA; // Ê
            break;
        case L'O': case L'o':
            ch = 0x00D4; // Ô
            break;
        case L'Y': case L'y':
            ch = 0x0176; // LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
            break;
        case L'C': case L'c':
            ch = 0x0108; // LATIN CAPITAL LETTER C WITH CIRCUMFLEX
            break;
        case L'G': case L'g':
            ch = 0x011C; // LATIN CAPITAL LETTER G WITH CIRCUMFLEX
            break;
        case L'H': case L'h':
            ch = 0x0124; // LATIN CAPITAL LETTER H WITH CIRCUMFLEX
            break;
        case L'J': case L'j':
            ch = 0x0134; // LATIN CAPITAL LETTER J WITH CIRCUMFLEX
            break;
        case L'S': case L's':
            ch = 0x015C; // LATIN CAPITAL LETTER S WITH CIRCUMFLEX
            break;
        }
        break;
    case L'`':
        switch (ch) {
        case L'A': case L'a':
            ch = 0x00C0; // À
            break;
        case L'I': case L'i':
            ch = 0x00CC; // LATIN CAPITAL LETTER I WITH GRAVE
            break;
        case L'U': case L'u':
            ch = 0x00D9; // Ù
            break;
        case L'E': case L'e':
            ch = 0x00C8; // È
            break;
        case L'O': case L'o':
            ch = 0x00D2; // LATIN CAPITAL LETTER O WITH GRAVE
            break;
        case L'Y': case L'y':
            ch = 0x1EF2; // LATIN CAPITAL LETTER Y WITH GRAVE
            break;
        }
        break;
    case L':':
        switch (ch) {
        case L'A': case L'a':
            ch = 0x00C4; // Ä
            break;
        case L'I': case L'i':
            ch = 0x00CF; // Ï
            break;
        case L'U': case L'u':
            ch = 0x00DC; // Ü
            break;
        case L'E': case L'e':
            ch = 0x00CB; // Ë
            break;
        case L'O': case L'o':
            ch = 0x00D6; // Ö
            break;
        case L'Y': case L'y':
            ch = 0x0178; // Ÿ
            break;
        }
        break;
        break;
    case L'\'':
        switch (ch) {
        case L'A': case L'a':
            ch = 0x00C1; // LATIN CAPITAL LETTER A WITH ACUTE
            break;
        case L'I': case L'i':
            ch = 0x00CD; // LATIN CAPITAL LETTER I WITH ACUTE
            break;
        case L'U': case L'u':
            ch = 0x00DA; // LATIN CAPITAL LETTER U WITH ACUTE
            break;
        case L'E': case L'e':
            ch = 0x00C9; // É
            break;
        case L'O': case L'o':
            ch = 0x00D3; // LATIN CAPITAL LETTER O WITH ACUTE
            break;
        case L'Y': case L'y':
            ch = 0x00DD; // LATIN CAPITAL LETTER Y WITH ACUTE
            break;
        case L'G': case L'g':
            ch = 0x01F4; // LATIN CAPITAL LETTER G WITH ACUTE
            break;
        case L'N': case L'n':
            ch = 0x0143; // LATIN CAPITAL LETTER N WITH ACUTE
            break;
        case L'W': case L'w':
            ch = 0x1E82; // LATIN CAPITAL LETTER W WITH ACUTE
            break;
        }
        break;
    case L',':
        switch (ch) {
        case L'C': case L'c':
            ch = 0x00C7; // Ç: LATIN CAPITAL LETTER C WITH CEDILLA
            break;
        case L'D': case L'd':
            ch = 0x1E10; // LATIN CAPITAL LETTER D WITH CEDILLA
            break;
        case L'G': case L'g':
            ch = 0x0122; // LATIN CAPITAL LETTER G CEDILLA
            break;
        case L'K': case L'k':
            ch = 0x0136; // LATIN CAPITAL LETTER K CEDILLA
            break;
        case L'L': case L'l':
            ch = 0x013B; // LATIN CAPITAL LETTER L CEDILLA
            break;
        case L'N': case L'n':
            ch = 0x0145; // LATIN CAPITAL LETTER N CEDILLA
            break;
        case L'R': case L'r':
            ch = 0x0156; // LATIN CAPITAL LETTER R CEDILLA
            break;
        case L'S': case L's':
            ch = 0x015E; // LATIN CAPITAL LETTER S CEDILLA
            break;
        case L'T': case L't':
            ch = 0x0162; // LATIN CAPITAL LETTER T CEDILLA
            break;
        }
        break;
    case L'&':
        switch (ch) {
        case L'O': case L'o':
            ch = 0x0152; // Œ
            break;
        }
        break;
    case L'.':
        switch (ch) {
        case L'I': case L'i':
            ch = 0x0130; // İ
            break;
        }
        break;
    }
    return ch;
}

// 文字送りを切り替える。
void __fastcall XgSetCharFeed(HWND hwnd, INT nMode)
{
    switch (nMode) {
    case 1:
        xg_bCharFeed = true;
        break;
    case 0:
        xg_bCharFeed = false;
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
    xg_chAccent = 0;
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

    XgUpdateStatusBar(hwnd);
    XgEnsureCaretVisible(hwnd);
    XgUpdateImage(hwnd);

    xg_prev_vk = 0;
    xg_chAccent = 0;
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

    XgUpdateStatusBar(hwnd);
    XgEnsureCaretVisible(hwnd);
    xg_prev_vk = 0;
    xg_chAccent = 0;
}

// BackSpaceを実行する。
void __fastcall XgCharBack(HWND hwnd)
{
    if (!xg_bCharFeed) {
        XgOnChar(hwnd, L'_', 1);
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
    XgOnChar(hwnd, L'_', 1);
    xg_bCharFeed = true;

    XgEnsureCaretVisible(hwnd);
    XgUpdateStatusBar(hwnd);
    xg_prev_vk = 0;
    xg_chAccent = 0;
}

// 文字が入力された。
void __fastcall XgOnChar(HWND hwnd, TCHAR ch, int cRepeat)
{
    WCHAR oldch = xg_xword.GetAt(xg_caret_pos);
    if (oldch == ZEN_BLACK && xg_bSolved)
        return;

    if (ch == L'_') {
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        if (!(xg_bSolved && oldch == ZEN_BLACK)) {
            XgSetChar(hwnd, ZEN_SPACE);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
        xg_prev_vk = 0;
        xg_chAccent = 0;
    } else if (ch == L' ') {
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        {
            if (oldch == ZEN_SPACE && !xg_bSolved) {
                XgSetChar(hwnd, ZEN_BLACK);
                XgEnsureCaretVisible(hwnd);
                XgUpdateImage(hwnd);
            } else if (oldch == ZEN_BLACK && !xg_bSolved) {
                XgSetChar(hwnd, ZEN_SPACE);
                XgEnsureCaretVisible(hwnd);
                XgUpdateImage(hwnd);
            } else if (!xg_bSolved || !xg_bShowAnswer) {
                if (oldch != ZEN_BLACK) {
                    XgSetChar(hwnd, ZEN_SPACE);
                    XgEnsureCaretVisible(hwnd);
                    XgUpdateImage(hwnd);
                }
            }
        }
        xg_prev_vk = 0;
        xg_chAccent = 0;
    } else if (ch == L'#') {
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();

        if (!xg_bSolved) {
            XgSetChar(hwnd, ZEN_BLACK);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_ABC) {
        // 英字入力の場合。
        if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            // 英字小文字を大文字に変換。
            WCHAR sz[2] = { ch, 0 };
            ::CharUpperW(sz);
            ch = sz[0];

            if (xg_chAccent) { // アクセントがあるか？
                // アクセント記号付きの文字にする。
                ch = XgConvertAccent(xg_chAccent, ch);
            } else {
                // 半角英字を全角英字に変換。
                ch = ZEN_LARGE_A + (ch - L'A');
            }
            xg_chAccent = 0; // アクセントを解除する。

            XgSetChar(hwnd, ch);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        } else if (XgIsCharZenkakuUpperW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        } else if (XgIsCharZenkakuLowerW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 全角小文字英字が入力された。
            ch = ZEN_LARGE_A + (ch - ZEN_SMALL_A);

            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        } else if ((0x0080 <= ch && ch <= 0x00FF) ||  // ラテン補助
                   (0x0100 <= ch && ch <= 0x017F) || // ラテン文字拡張A
                   (0x1E00 <= ch && ch <= 0x1EFF)) // ラテン文字拡張追加
        {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            WCHAR sz[2];
            LCMapStringW(JPN_LOCALE,
                LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                &ch, 1, sz, ARRAYSIZE(sz));
            CharUpperW(sz);
            ch = sz[0];

            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_KANA) {
        WCHAR newch = 0;
        // カナ入力の場合。
        if (::GetAsyncKeyState(VK_CONTROL) < 0) {
            // [Ctrl]キーが押されている。
        } else if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch)) {
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
                if (newch) {
                    xg_prev_vk = 0;
                    xg_chAccent = 0;
                }
            } else if (xg_prev_vk == L'C' && ch == 'H') {
                xg_prev_vk = L'T';
            } else {
                xg_prev_vk = ch;
            }
        } else if (XgIsCharHiraganaW(ch)) {
            // ひらがな直接入力。
            WCHAR sz[2];
            ::LCMapStringW(JPN_LOCALE,
                LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                &ch, 1, sz, ARRAYSIZE(sz));
            newch = sz[0];
            goto katakana;
        } else if (XgIsCharKatakanaW(ch)) {
katakana:;
            // カタカナ直接入力。
            // 小さな字を大きな字にする。
            for (size_t i = 0; i < ARRAYSIZE(xg_small); i++) {
                if (static_cast<WCHAR>(ch) == xg_small[i][0]) {
                    newch = xg_large[i][0];
                    break;
                }
            }
            if (newch) {
                xg_prev_vk = 0;
                xg_chAccent = 0;
            }
        } else if (ch == '-' || ch == ZEN_PROLONG) {
            newch = ZEN_PROLONG;
            xg_prev_vk = 0;
            xg_chAccent = 0;
        } else {
            xg_prev_vk = 0;
            xg_chAccent = 0;
        }

        if (newch) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            XgSetChar(hwnd, newch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_RUSSIA) {
        // ロシア入力の場合。
        if (XgIsCharZenkakuCyrillicW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            // キリル文字直接入力。
            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_GREEK) {
        if (XgIsCharGreekW(ch)) {
            // 大文字にする。
            CharUpperBuffW(&ch, 1);

            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_DIGITS) {
        // 数字入力の場合。
        if (XgIsCharHankakuNumericW(ch))
            ch += 0xFF10 - L'0'; // 全角数字にする。
        if (XgIsCharZenkakuNumericW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            // 数字直接入力。
            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_ANY) {
        // 自由入力の場合。
        WCHAR sz[2] = { ch, 0 };
        auto str = XgNormalizeString(sz);
        ch = str[0];
        if (ch >= 0x0020 && ch != 0x007F) { // 制御文字はダメだ。
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            XgSetChar(hwnd, ch);
            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
    }
}

// キーが押された。
void __fastcall XgOnKey(HWND hwnd, UINT vk, bool fDown, int /*cRepeat*/, UINT /*flags*/)
{
    // 特定の条件において、キー入力を拒否する。
    if (!fDown)
        return;

    // 仮想キーコードに応じて、処理する。
    switch (vk) {
    case VK_APPS:   // アプリ キー。
        // アプリメニューを表示する。
        {
            HMENU hMenu = XgLoadPopupMenu(hwnd, 0);
            HMENU hSubMenu = GetSubMenu(hMenu, 0);

            INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

            // 現在のキャレット位置。
            POINT pt;
            pt.x = xg_nMargin + xg_caret_pos.m_j * nCellSize;
            pt.y = xg_nMargin + xg_caret_pos.m_i * nCellSize;
            pt.x += nCellSize / 2;
            pt.y += nCellSize / 2;
            pt.x -= XgGetHScrollPos();
            pt.y -= XgGetVScrollPos();

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
            ::TrackPopupMenu(
                hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                pt.x, pt.y, 0, hwnd, NULL);
            ::PostMessageW(hwnd, WM_NULL, 0, 0);

            ::DestroyMenu(hMenu);
        }
        xg_prev_vk = 0;
        xg_chAccent = 0;
        break;

    case VK_PRIOR:  // PgUpキー。
        ::SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEUP, 0), 0);
        xg_prev_vk = 0;
        xg_chAccent = 0;
        break;

    case VK_NEXT:   // PgDnキー。
        ::SendMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_PAGEDOWN, 0), 0);
        xg_prev_vk = 0;
        xg_chAccent = 0;
        break;

    case VK_DELETE:
        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        // 現在のキャレット位置のマスの中身を消去する。
        if (xg_xword.GetAt(xg_caret_pos) != ZEN_SPACE && !xg_bSolved) {
            XgSetChar(hwnd, ZEN_SPACE);
            XgEnsureCaretVisible(hwnd);
            XgUpdateImage(hwnd);
        }
        xg_prev_vk = 0;
        xg_chAccent = 0;
        break;

    default:
        if (::GetKeyState(VK_CONTROL) < 0 && ::GetKeyState(VK_SHIFT) >= 0) {
            // [Ctrl] キーが押されている。
            WCHAR sz1[2], sz2[2];
            BYTE state[256] = { 0 };
            // 仮想キーコードを文字に変換。
            ::ToUnicode(vk, 0, state, sz1, _countof(sz1), 0); // [Shift]なし。
            state[VK_SHIFT] = state[VK_LSHIFT] = state[VK_RSHIFT] = 0x80;
            ::ToUnicode(vk, 0, state, sz2, _countof(sz2), 0); // [Shift]あり。
            WCHAR ch1 = sz1[0], ch2 = sz2[0];
            switch (ch1) {
            case L'.':
                ::PostMessageW(hwnd, WM_COMMAND, ID_TOGGLEMARK, 0);
                return;
            case L']':
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMIN, 0);
                return;
            case L'[':
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMOUT, 0);
                return;
            case L'\\':
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOM100, 0);
                return;
            case L'^':
                ::PostMessageW(hwnd, WM_COMMAND, ID_CHARFEED, 0);
                return;
            case L':':
                ::PostMessageW(hwnd, WM_COMMAND, ID_GENERATEFROMWORDLIST, 0);
                return;
            case L';':
                ::PostMessageW(hwnd, WM_COMMAND, ID_OPENPATTERNS, 0);
                return;
            }
            switch (ch2) {
            case L'.':
                ::PostMessageW(hwnd, WM_COMMAND, ID_TOGGLEMARK, 0);
                return;
            case L']':
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMIN, 0);
                return;
            case L'[':
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMOUT, 0);
                return;
            case L'\\':
                ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOM100, 0);
                return;
            case L'^':
                ::PostMessageW(hwnd, WM_COMMAND, ID_CHARFEED, 0);
                return;
            case L':':
                ::PostMessageW(hwnd, WM_COMMAND, ID_GENERATEFROMWORDLIST, 0);
                return;
            case L';':
                ::PostMessageW(hwnd, WM_COMMAND, ID_OPENPATTERNS, 0);
                return;
            }
            return;
        }
        if (::GetKeyState(VK_CONTROL) < 0 && ::GetKeyState(VK_SHIFT) < 0) {
            // [Shift]キーと[Ctrl]キーが押されている。
            if (vk == 'G') {
                XgOnChar(hwnd, 0x011E, 1); // Ğ
                return;
            }
            WCHAR sz1[2], sz2[2];
            BYTE state[256] = { 0 };
            // 仮想キーコードを文字に変換。
            ::ToUnicode(vk, 0, state, sz1, _countof(sz1), 0); // [Shift]なしの場合。
            state[VK_SHIFT] = state[VK_LSHIFT] = state[VK_RSHIFT] = 0x80;
            ::ToUnicode(vk, 0, state, sz2, _countof(sz2), 0); // [Shift]ありの場合。
            // アクセント記号か？
            WCHAR ch1 = sz1[0], ch2 = sz2[0];
            if (wcschr(L"^`':,&.", ch1) != NULL) {
                xg_chAccent = ch1;
            } else if (wcschr(L"^`':,&.", ch2) != NULL) {
                xg_chAccent = ch2;
            } else {
                xg_chAccent = 0;
            }
        }
        break;
    }
}

// IMEから文字が入力された。
void __fastcall XgOnImeChar(HWND hwnd, WCHAR ch, LPARAM /*lKeyData*/)
{
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

    if (xg_imode == xg_im_RUSSIA) {
        // ロシア入力モードの場合。
        // キリル文字か？
        if (XgIsCharZenkakuCyrillicW(ch)) {
            CharUpperBuffW(&ch, 1);

            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();

            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);
            
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_KANJI) {
        // 漢字入力モードの場合。
        // 漢字か？
        if (XgIsCharKanjiW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);
            
            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_KANA) {
        // カナ入力モードの場合。
        if (XgIsCharHiraganaW(ch)) {
            // ひらがな入力。
            WCHAR sz[2];
            LCMapStringW(JPN_LOCALE,
                LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                &ch, 1, sz, ARRAYSIZE(sz));
            ch = sz[0];
            goto katakana;
        } else if (XgIsCharKatakanaW(ch)) {
katakana:;
            // カタカナ入力。
            // 小さな字を大きな字にする。
            for (size_t i = 0; i < ARRAYSIZE(xg_small); i++) {
                if (ch == xg_small[i][0]) {
                    ch = xg_large[i][0];
                    break;
                }
            }
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 文字を設定する。
            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_ABC) {
        if (XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch) ||
            XgIsCharZenkakuUpperW(ch) || XgIsCharZenkakuLowerW(ch) ||
            (0x0080 <= ch && ch <= 0x00FF) || // ラテン補助
            (0x0100 <= ch && ch <= 0x017F) || // ラテン文字拡張A
            (0x1E00 <= ch && ch <= 0x1EFF)) // ラテン文字拡張追加
        {
            WCHAR sz[2];
            LCMapStringW(JPN_LOCALE,
                LCMAP_FULLWIDTH | LCMAP_KATAKANA | LCMAP_UPPERCASE,
                &ch, 1, sz, ARRAYSIZE(sz));
            CharUpperW(sz);
            ch = sz[0];

            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 文字を設定する。
            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_GREEK) {
        if (XgIsCharGreekW(ch)) {
            // 大文字にする。
            CharUpperBuffW(&ch, 1);
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 文字を設定する。
            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_DIGITS) {
        // 数字入力の場合。
        if (XgIsCharHankakuNumericW(ch)) {
            ch += 0xFF10 - L'0'; // 全角数字にする。
        }
        if (XgIsCharZenkakuNumericW(ch)) {
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 文字を設定する。
            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgUpdateImage(hwnd);
        }
    } else if (xg_imode == xg_im_ANY) {
        // 自由入力の場合。
        WCHAR sz[2] = { ch, 0 };
        auto str = XgNormalizeString(sz);
        ch = str[0];
        if (ch >= 0x0020 && ch != 0x007F) { // 制御文字はダメだ。
            // 候補ウィンドウを破棄する。
            XgDestroyCandsWnd();
            // 文字を設定する。
            XgSetChar(hwnd, ch);
            XgEnsureCaretVisible(hwnd);

            if (xg_bCharFeed)
                XgCharFeed(hwnd);

            XgUpdateImage(hwnd);
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
        xg_bShowInputPalette = false;
        DestroyWindow(hwnd);
        return;
    }

    HWND hButton = GetDlgItem(hwnd, id);

    WCHAR sz[10];
    GetWindowTextW(hButton, sz, 10);

    if (sz[0] && sz[1] == 0) {
        switch (sz[0]) {
        case ZEN_SPACE:
            XgOnImeChar(xg_hMainWnd, L'_', 0);
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
            XgOnImeChar(xg_hMainWnd, sz[0], 0);
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
        case 20070: // カナに
            XgSetInputMode(xg_hMainWnd, xg_im_KANA);
            break;
        case 20071: // 縦置き/横置き
            if (xg_imode == xg_im_KANA) {
                xg_bTateOki = !xg_bTateOki;
                XgCreateInputPalette(xg_hMainWnd);
            }
            break;
        case 20072: // 英字に
            XgSetInputMode(xg_hMainWnd, xg_im_ABC);
            break;
        case 20073: // ロシアに
            XgSetInputMode(xg_hMainWnd, xg_im_RUSSIA);
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

void InputPal_OnMove(HWND hwnd, int x, int y)
{
    MRect rc;
    ::GetWindowRect(hwnd, &rc);
    xg_nInputPaletteWndX = rc.left;
    xg_nInputPaletteWndY = rc.top;
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
        HANDLE_MSG(hwnd, WM_MOVE, InputPal_OnMove);
    }
    return 0;
}

// 入力パレットを破棄する。
BOOL XgDestroyInputPalette(void)
{
    xg_bShowInputPalette = false;
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

    switch (xg_imode) {
    case xg_im_ABC:
        if (xg_bLowercase) {
            CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_ABCLOWER), hwndOwner,
                          XgInputPaletteDlgProc);
        } else {
            CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_ABC), hwndOwner,
                          XgInputPaletteDlgProc);
        }
        break;
    case xg_im_KANA:
        if (xg_bTateOki) {
            if (xg_bHiragana) {
                CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_HIRATATE), hwndOwner,
                              XgInputPaletteDlgProc);
            } else {
                CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_KATATATE), hwndOwner,
                              XgInputPaletteDlgProc);
            }
        } else {
            if (xg_bHiragana) {
                CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_HIRAYOKO), hwndOwner,
                              XgInputPaletteDlgProc);
            } else {
                CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_KATAYOKO), hwndOwner,
                              XgInputPaletteDlgProc);
            }
        }
        break;
    case xg_im_RUSSIA:
        if (xg_bLowercase) {
            CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_RUSSIALOWER), hwndOwner,
                          XgInputPaletteDlgProc);
        } else {
            CreateDialogW(xg_hInstance, MAKEINTRESOURCEW(IDD_RUSSIA), hwndOwner,
                          XgInputPaletteDlgProc);
        }
        break;
    case xg_im_GREEK:
        // TODO: ギリシャ文字入力パレットを追加せよ。
        break;
    case xg_im_DIGITS:
        // TODO: 数字入力パレットを追加せよ。
        break;
    case xg_im_ANY:
        break;
    default:
        return FALSE;
    }

    MRect rc;
    GetWindowRect(xg_hwndInputPalette, &rc);
    if (xg_nInputPaletteWndX != CW_USEDEFAULT) {
        MoveWindow(xg_hwndInputPalette,
            xg_nInputPaletteWndX, xg_nInputPaletteWndY,
            rc.Width(), rc.Height(), TRUE);
        SendMessageW(xg_hwndInputPalette, DM_REPOSITION, 0, 0);
    }

    ShowWindow(xg_hwndInputPalette, SW_SHOWNOACTIVATE);
    UpdateWindow(xg_hwndInputPalette);
    xg_bShowInputPalette = true;

    return xg_hwndInputPalette != NULL;
}

// 入力モードを切り替える。
void __fastcall XgSetInputMode(HWND hwnd, XG_InputMode mode)
{
    bool flag = (xg_imode != mode);

    xg_imode = mode;

    if (flag && xg_hwndInputPalette) {
        XgCreateInputPalette(hwnd);
    }

    XgUpdateStatusBar(hwnd);
    XgUpdateImage(hwnd, 0, 0);
}

void __fastcall XgSetInputModeFromDict(HWND hwnd)
{
    WCHAR ch;
    if (xg_dict_1.size()) {
        ch = xg_dict_1[0].m_word[0];
    } else if (xg_dict_2.size()) {
        ch = xg_dict_2[0].m_word[0];
    } else {
        return;
    }

    if (xg_imode != xg_im_ANY) { // 自由入力でなければ
        // 辞書の文字に応じて入力モードを切り替える。
        if (XgIsCharHiraganaW(ch) || XgIsCharKatakanaW(ch)) {
            XgSetInputMode(hwnd, xg_im_KANA);
        } else if (XgIsCharKanjiW(ch)) {
            XgSetInputMode(hwnd, xg_im_KANJI);
        } else if (XgIsCharZenkakuCyrillicW(ch)) {
            XgSetInputMode(hwnd, xg_im_RUSSIA);
        } else if (XgIsCharZenkakuNumericW(ch) || XgIsCharHankakuNumericW(ch)) {
            XgSetInputMode(hwnd, xg_im_DIGITS);
        } else if (XgIsCharGreekW(ch)) {
            XgSetInputMode(hwnd, xg_im_GREEK);
        } else if (XgIsCharZenkakuUpperW(ch) || XgIsCharZenkakuLowerW(ch) ||
                   XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch) ||
                   (0x0080 <= ch && ch <= 0x00FF) || // ラテン補助
                   (0x0100 <= ch && ch <= 0x017F) || // ラテン文字拡張A
                   (0x1E00 <= ch && ch <= 0x1EFF)) // ラテン文字拡張追加
        {
            XgSetInputMode(hwnd, xg_im_ABC);
        }
    }
}

bool __fastcall XgOnCommandExtra(HWND hwnd, INT id)
{
    bool bOK = false;

    bool bOldFeed = xg_bCharFeed;
    xg_bCharFeed = false;

    switch (id)
    {
    // kana
    case 10000: XgOnImeChar(hwnd, ZEN_A, 0); bOK = true; break;
    case 10001: XgOnImeChar(hwnd, ZEN_I, 0); bOK = true; break;
    case 10002: XgOnImeChar(hwnd, ZEN_U, 0); bOK = true; break;
    case 10003: XgOnImeChar(hwnd, ZEN_E, 0); bOK = true; break;
    case 10004: XgOnImeChar(hwnd, ZEN_O, 0); bOK = true; break;
    case 10010: XgOnImeChar(hwnd, ZEN_KA, 0); bOK = true; break;
    case 10011: XgOnImeChar(hwnd, ZEN_KI, 0); bOK = true; break;
    case 10012: XgOnImeChar(hwnd, ZEN_KU, 0); bOK = true; break;
    case 10013: XgOnImeChar(hwnd, ZEN_KE, 0); bOK = true; break;
    case 10014: XgOnImeChar(hwnd, ZEN_KO, 0); bOK = true; break;
    case 10020: XgOnImeChar(hwnd, ZEN_SA, 0); bOK = true; break;
    case 10021: XgOnImeChar(hwnd, ZEN_SI, 0); bOK = true; break;
    case 10022: XgOnImeChar(hwnd, ZEN_SU, 0); bOK = true; break;
    case 10023: XgOnImeChar(hwnd, ZEN_SE, 0); bOK = true; break;
    case 10024: XgOnImeChar(hwnd, ZEN_SO, 0); bOK = true; break;
    case 10030: XgOnImeChar(hwnd, ZEN_TA, 0); bOK = true; break;
    case 10031: XgOnImeChar(hwnd, ZEN_CHI, 0); bOK = true; break;
    case 10032: XgOnImeChar(hwnd, ZEN_TSU, 0); bOK = true; break;
    case 10033: XgOnImeChar(hwnd, ZEN_TE, 0); bOK = true; break;
    case 10034: XgOnImeChar(hwnd, ZEN_TO, 0); bOK = true; break;
    case 10040: XgOnImeChar(hwnd, ZEN_NA, 0); bOK = true; break;
    case 10041: XgOnImeChar(hwnd, ZEN_NI, 0); bOK = true; break;
    case 10042: XgOnImeChar(hwnd, ZEN_NU, 0); bOK = true; break;
    case 10043: XgOnImeChar(hwnd, ZEN_NE, 0); bOK = true; break;
    case 10044: XgOnImeChar(hwnd, ZEN_NO, 0); bOK = true; break;
    case 10050: XgOnImeChar(hwnd, ZEN_HA, 0); bOK = true; break;
    case 10051: XgOnImeChar(hwnd, ZEN_HI, 0); bOK = true; break;
    case 10052: XgOnImeChar(hwnd, ZEN_FU, 0); bOK = true; break;
    case 10053: XgOnImeChar(hwnd, ZEN_HE, 0); bOK = true; break;
    case 10054: XgOnImeChar(hwnd, ZEN_HO, 0); bOK = true; break;
    case 10060: XgOnImeChar(hwnd, ZEN_MA, 0); bOK = true; break;
    case 10061: XgOnImeChar(hwnd, ZEN_MI, 0); bOK = true; break;
    case 10062: XgOnImeChar(hwnd, ZEN_MU, 0); bOK = true; break;
    case 10063: XgOnImeChar(hwnd, ZEN_ME, 0); bOK = true; break;
    case 10064: XgOnImeChar(hwnd, ZEN_MO, 0); bOK = true; break;
    case 10070: XgOnImeChar(hwnd, ZEN_YA, 0); bOK = true; break;
    case 10071: XgOnImeChar(hwnd, ZEN_YU, 0); bOK = true; break;
    case 10072: XgOnImeChar(hwnd, ZEN_YO, 0); bOK = true; break;
    case 10080: XgOnImeChar(hwnd, ZEN_RA, 0); bOK = true; break;
    case 10081: XgOnImeChar(hwnd, ZEN_RI, 0); bOK = true; break;
    case 10082: XgOnImeChar(hwnd, ZEN_RU, 0); bOK = true; break;
    case 10083: XgOnImeChar(hwnd, ZEN_RE, 0); bOK = true; break;
    case 10084: XgOnImeChar(hwnd, ZEN_RO, 0); bOK = true; break;
    case 10090: XgOnImeChar(hwnd, ZEN_WA, 0); bOK = true; break;
    case 10091: XgOnImeChar(hwnd, ZEN_NN, 0); bOK = true; break;
    case 10092: XgOnImeChar(hwnd, ZEN_PROLONG, 0); bOK = true; break;
    case 10100: XgOnImeChar(hwnd, ZEN_GA, 0); bOK = true; break;
    case 10101: XgOnImeChar(hwnd, ZEN_GI, 0); bOK = true; break;
    case 10102: XgOnImeChar(hwnd, ZEN_GU, 0); bOK = true; break;
    case 10103: XgOnImeChar(hwnd, ZEN_GE, 0); bOK = true; break;
    case 10104: XgOnImeChar(hwnd, ZEN_GO, 0); bOK = true; break;
    case 10110: XgOnImeChar(hwnd, ZEN_ZA, 0); bOK = true; break;
    case 10111: XgOnImeChar(hwnd, ZEN_JI, 0); bOK = true; break;
    case 10112: XgOnImeChar(hwnd, ZEN_ZU, 0); bOK = true; break;
    case 10113: XgOnImeChar(hwnd, ZEN_ZE, 0); bOK = true; break;
    case 10114: XgOnImeChar(hwnd, ZEN_ZO, 0); bOK = true; break;
    case 10120: XgOnImeChar(hwnd, ZEN_DA, 0); bOK = true; break;
    case 10121: XgOnImeChar(hwnd, ZEN_DI, 0); bOK = true; break;
    case 10122: XgOnImeChar(hwnd, ZEN_DU, 0); bOK = true; break;
    case 10123: XgOnImeChar(hwnd, ZEN_DE, 0); bOK = true; break;
    case 10124: XgOnImeChar(hwnd, ZEN_DO, 0); bOK = true; break;
    case 10130: XgOnImeChar(hwnd, ZEN_BA, 0); bOK = true; break;
    case 10131: XgOnImeChar(hwnd, ZEN_BI, 0); bOK = true; break;
    case 10132: XgOnImeChar(hwnd, ZEN_BU, 0); bOK = true; break;
    case 10133: XgOnImeChar(hwnd, ZEN_BE, 0); bOK = true; break;
    case 10134: XgOnImeChar(hwnd, ZEN_BO, 0); bOK = true; break;
    case 10140: XgOnImeChar(hwnd, ZEN_PA, 0); bOK = true; break;
    case 10141: XgOnImeChar(hwnd, ZEN_PI, 0); bOK = true; break;
    case 10142: XgOnImeChar(hwnd, ZEN_PU, 0); bOK = true; break;
    case 10143: XgOnImeChar(hwnd, ZEN_PE, 0); bOK = true; break;
    case 10144: XgOnImeChar(hwnd, ZEN_PO, 0); bOK = true; break;
    case 10150: XgOnImeChar(hwnd, ZEN_PROLONG, 0); bOK = true; break;
    // ABC
    case 20000: XgOnChar(hwnd, L'A', 1); bOK = true; break;
    case 20001: XgOnChar(hwnd, L'B', 1); bOK = true; break;
    case 20002: XgOnChar(hwnd, L'C', 1); bOK = true; break;
    case 20003: XgOnChar(hwnd, L'D', 1); bOK = true; break;
    case 20004: XgOnChar(hwnd, L'E', 1); bOK = true; break;
    case 20005: XgOnChar(hwnd, L'F', 1); bOK = true; break;
    case 20006: XgOnChar(hwnd, L'G', 1); bOK = true; break;
    case 20007: XgOnChar(hwnd, L'H', 1); bOK = true; break;
    case 20008: XgOnChar(hwnd, L'I', 1); bOK = true; break;
    case 20009: XgOnChar(hwnd, L'J', 1); bOK = true; break;
    case 20010: XgOnChar(hwnd, L'K', 1); bOK = true; break;
    case 20011: XgOnChar(hwnd, L'L', 1); bOK = true; break;
    case 20012: XgOnChar(hwnd, L'M', 1); bOK = true; break;
    case 20013: XgOnChar(hwnd, L'N', 1); bOK = true; break;
    case 20014: XgOnChar(hwnd, L'O', 1); bOK = true; break;
    case 20015: XgOnChar(hwnd, L'P', 1); bOK = true; break;
    case 20016: XgOnChar(hwnd, L'Q', 1); bOK = true; break;
    case 20017: XgOnChar(hwnd, L'R', 1); bOK = true; break;
    case 20018: XgOnChar(hwnd, L'S', 1); bOK = true; break;
    case 20019: XgOnChar(hwnd, L'T', 1); bOK = true; break;
    case 20020: XgOnChar(hwnd, L'U', 1); bOK = true; break;
    case 20021: XgOnChar(hwnd, L'V', 1); bOK = true; break;
    case 20022: XgOnChar(hwnd, L'W', 1); bOK = true; break;
    case 20023: XgOnChar(hwnd, L'X', 1); bOK = true; break;
    case 20024: XgOnChar(hwnd, L'Y', 1); bOK = true; break;
    case 20025: XgOnChar(hwnd, L'Z', 1); bOK = true; break;
    case 20026: xg_chAccent = L'^'; bOK = true; break;
    case 20027: xg_chAccent = L'`'; bOK = true; break;
    case 20028: xg_chAccent = L'\''; bOK = true; break;
    case 20029: xg_chAccent = L':'; bOK = true; break;
    case 20030: xg_chAccent = L','; bOK = true; break;
    case 20031: xg_chAccent = L'&'; bOK = true; break;
    case 20032: xg_chAccent = L'.'; bOK = true; break;
    case 20033: XgOnChar(hwnd, 0x011E, 1); bOK = true; break; // Ğ
    // Russian
    case 30000: XgOnImeChar(hwnd, 0x0410, 0); bOK = true; break;
    case 30001: XgOnImeChar(hwnd, 0x0411, 0); bOK = true; break;
    case 30002: XgOnImeChar(hwnd, 0x0412, 0); bOK = true; break;
    case 30003: XgOnImeChar(hwnd, 0x0413, 0); bOK = true; break;
    case 30004: XgOnImeChar(hwnd, 0x0414, 0); bOK = true; break;
    case 30005: XgOnImeChar(hwnd, 0x0415, 0); bOK = true; break;
    case 30006: XgOnImeChar(hwnd, 0x0401, 0); bOK = true; break;
    case 30007: XgOnImeChar(hwnd, 0x0416, 0); bOK = true; break;
    case 30008: XgOnImeChar(hwnd, 0x0417, 0); bOK = true; break;
    case 30009: XgOnImeChar(hwnd, 0x0418, 0); bOK = true; break;
    case 30010: XgOnImeChar(hwnd, 0x0419, 0); bOK = true; break;
    case 30011: XgOnImeChar(hwnd, 0x041A, 0); bOK = true; break;
    case 30012: XgOnImeChar(hwnd, 0x041B, 0); bOK = true; break;
    case 30013: XgOnImeChar(hwnd, 0x041C, 0); bOK = true; break;
    case 30014: XgOnImeChar(hwnd, 0x041D, 0); bOK = true; break;
    case 30015: XgOnImeChar(hwnd, 0x041E, 0); bOK = true; break;
    case 30016: XgOnImeChar(hwnd, 0x041F, 0); bOK = true; break;
    case 30017: XgOnImeChar(hwnd, 0x0420, 0); bOK = true; break;
    case 30018: XgOnImeChar(hwnd, 0x0421, 0); bOK = true; break;
    case 30019: XgOnImeChar(hwnd, 0x0422, 0); bOK = true; break;
    case 30020: XgOnImeChar(hwnd, 0x0423, 0); bOK = true; break;
    case 30021: XgOnImeChar(hwnd, 0x0424, 0); bOK = true; break;
    case 30022: XgOnImeChar(hwnd, 0x0425, 0); bOK = true; break;
    case 30023: XgOnImeChar(hwnd, 0x0426, 0); bOK = true; break;
    case 30024: XgOnImeChar(hwnd, 0x0427, 0); bOK = true; break;
    case 30025: XgOnImeChar(hwnd, 0x0428, 0); bOK = true; break;
    case 30026: XgOnImeChar(hwnd, 0x0429, 0); bOK = true; break;
    case 30027: XgOnImeChar(hwnd, 0x042A, 0); bOK = true; break;
    case 30028: XgOnImeChar(hwnd, 0x042B, 0); bOK = true; break;
    case 30029: XgOnImeChar(hwnd, 0x042C, 0); bOK = true; break;
    case 30030: XgOnImeChar(hwnd, 0x042D, 0); bOK = true; break;
    case 30031: XgOnImeChar(hwnd, 0x042E, 0); bOK = true; break;
    case 30032: XgOnImeChar(hwnd, 0x042F, 0); bOK = true; break;
    // Digits
    case 40000: XgOnImeChar(hwnd, L'0', 0); bOK = true; break;
    case 40001: XgOnImeChar(hwnd, L'1', 0); bOK = true; break;
    case 40002: XgOnImeChar(hwnd, L'2', 0); bOK = true; break;
    case 40003: XgOnImeChar(hwnd, L'3', 0); bOK = true; break;
    case 40004: XgOnImeChar(hwnd, L'4', 0); bOK = true; break;
    case 40005: XgOnImeChar(hwnd, L'5', 0); bOK = true; break;
    case 40006: XgOnImeChar(hwnd, L'6', 0); bOK = true; break;
    case 40007: XgOnImeChar(hwnd, L'7', 0); bOK = true; break;
    case 40008: XgOnImeChar(hwnd, L'8', 0); bOK = true; break;
    case 40009: XgOnImeChar(hwnd, L'9', 0); bOK = true; break;
    // Greek
    case 41000: XgOnImeChar(hwnd, 0x03B1, 0); bOK = true; break;
    case 41001: XgOnImeChar(hwnd, 0x03B2, 0); bOK = true; break;
    case 41002: XgOnImeChar(hwnd, 0x03B3, 0); bOK = true; break;
    case 41003: XgOnImeChar(hwnd, 0x03B4, 0); bOK = true; break;
    case 41004: XgOnImeChar(hwnd, 0x03B5, 0); bOK = true; break;
    case 41005: XgOnImeChar(hwnd, 0x03B6, 0); bOK = true; break;
    case 41006: XgOnImeChar(hwnd, 0x03B7, 0); bOK = true; break;
    case 41007: XgOnImeChar(hwnd, 0x03B8, 0); bOK = true; break;
    case 41008: XgOnImeChar(hwnd, 0x03B9, 0); bOK = true; break;
    case 41009: XgOnImeChar(hwnd, 0x03BA, 0); bOK = true; break;
    case 41010: XgOnImeChar(hwnd, 0x03BB, 0); bOK = true; break;
    case 41011: XgOnImeChar(hwnd, 0x03BC, 0); bOK = true; break;
    case 41012: XgOnImeChar(hwnd, 0x03BD, 0); bOK = true; break;
    case 41013: XgOnImeChar(hwnd, 0x03BE, 0); bOK = true; break;
    case 41014: XgOnImeChar(hwnd, 0x03BF, 0); bOK = true; break;
    case 41015: XgOnImeChar(hwnd, 0x03C0, 0); bOK = true; break;
    case 41016: XgOnImeChar(hwnd, 0x03C1, 0); bOK = true; break;
    case 41017: XgOnImeChar(hwnd, 0x03C3, 0); bOK = true; break;
    case 41018: XgOnImeChar(hwnd, 0x03C4, 0); bOK = true; break;
    case 41019: XgOnImeChar(hwnd, 0x03C5, 0); bOK = true; break;
    case 41020: XgOnImeChar(hwnd, 0x03C6, 0); bOK = true; break;
    case 41021: XgOnImeChar(hwnd, 0x03C7, 0); bOK = true; break;
    case 41022: XgOnImeChar(hwnd, 0x03C8, 0); bOK = true; break;
    case 41023: XgOnImeChar(hwnd, 0x03C9, 0); bOK = true; break;
    default:
        break;
    }

    xg_bCharFeed = bOldFeed;

    return bOK;
}
