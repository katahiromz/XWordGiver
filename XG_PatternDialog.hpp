﻿#pragma once

#include "XG_Window.hpp"
#include "layout.h"
#include "XWordGiver.hpp"

// 解を求める（黒マス追加なし）。
bool __fastcall XgOnSolve_NoAddBlack(HWND hwnd);
// 解を求める（黒マス追加なし）。結果を表示しない。
bool __fastcall XgOnSolve_NoAddBlackNoResults(HWND hwnd);

#define XG_MAX_PAT_SIZE1 20
#define XG_MAX_PAT_SIZE2 30
#define XG_MAX_PAT_SIZE3 45

class XG_PatternDialog : public XG_Dialog
{
public:
    const int cxCell1 = 5, cyCell1 = 5; // 小さなセルのサイズ。
    const int cxCell2 = 3, cyCell2 = 3; // さらに小さなセルのサイズ。
    const int cxCell3 = 2, cyCell3 = 2; // さらにさらに小さなセルのサイズ。

    // 黒マスパターンのデータ。
    inline static patterns_t s_patterns;
    inline static LAYOUT_DATA *s_pLayout = nullptr;

    // 「黒マスパターン」ダイアログの位置とサイズ。
    inline static int xg_nPatWndX = CW_USEDEFAULT;
    inline static int xg_nPatWndY = CW_USEDEFAULT;
    inline static int xg_nPatWndCX = CW_USEDEFAULT;
    inline static int xg_nPatWndCY = CW_USEDEFAULT;

    XG_PatternDialog() noexcept
    {
    }

    // タイプによりフィルターを行う。
    XG_NOINLINE
    BOOL FilterPatBySize(const XG_PATDATA& pat, int type0, int type1) noexcept {
        if (pat.num_columns > XG_MAX_PAT_SIZE3 || pat.num_rows > XG_MAX_PAT_SIZE3)
            return FALSE;

        switch (type0) {
        case rad1:
            if (pat.num_columns + pat.num_rows < 28)
                return FALSE;
            break;
        case rad2:
            if (!(pat.num_columns + pat.num_rows < 28 &&
                  pat.num_columns + pat.num_rows > 16))
            {
                return FALSE;
            }
            break;
        case rad3:
            if (!(pat.num_columns + pat.num_rows <= 16))
                return FALSE;
            break;
        default:
            assert(0);
        }

        switch (type1) {
        case rad4:
            if (pat.num_columns <= pat.num_rows)
                return FALSE;
            break;
        case rad5:
            if (pat.num_columns >= pat.num_rows)
                return FALSE;
            break;
        case rad6:
            if (pat.num_columns != pat.num_rows)
                return FALSE;
            break;
        default:
            assert(0);
        }

        return TRUE;
    }

    XG_NOINLINE
    BOOL RefreshContents(HWND hwnd, int type0, int type1)
    {
        // リストボックスをクリアする。
        ListBox_ResetContent(GetDlgItem(hwnd, lst1));

        // パターンデータを読み込む。
        WCHAR szPath[MAX_PATH];
        XgFindLocalFile(szPath, _countof(szPath), L"PAT.txt");
        if (!XgLoadPatterns(szPath, s_patterns))
            return FALSE;

        // ソートして一意化する。
        XgSortAndUniquePatterns(s_patterns, TRUE);

        // 抽出する。
        patterns_t pats;
        for (auto& pat : s_patterns) {
            // ルールに適合するか？
            if (!XgPatternRuleIsOK(pat)) {
                continue;
            }

            if (FilterPatBySize(pat, type0, type1))
            {
                pats.push_back(pat);
            }
        }
        s_patterns = std::move(pats);

        // かき混ぜる。
        xg_random_shuffle(s_patterns.begin(), s_patterns.end());

        // インデックスとして追加する。
        for (size_t i = 0; i < s_patterns.size(); ++i)
        {
            SendDlgItemMessageW(hwnd, lst1, LB_ADDSTRING, 0, i);
        }

        return TRUE;
    }

    // WM_INITDIALOG
    XG_NOINLINE
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        if (s_pLayout)
        {
            LayoutDestroy(s_pLayout);
            s_pLayout = nullptr;
        }

        if (xg_bShowAnswerOnGenerate)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);

        CheckRadioButton(hwnd, rad1, rad3, rad2);
        CheckRadioButton(hwnd, rad4, rad6, rad6);
        RefreshContents(hwnd, rad2, rad6);

        static const LAYOUT_INFO layouts[] =
        {
            { stc1, BF_LEFT | BF_TOP },
            { rad1, BF_LEFT | BF_TOP },
            { rad2, BF_LEFT | BF_TOP },
            { rad3, BF_LEFT | BF_TOP },
            { rad4, BF_LEFT | BF_TOP },
            { rad5, BF_LEFT | BF_TOP },
            { rad6, BF_LEFT | BF_TOP },
            { stc2, BF_LEFT | BF_TOP },
            { lst1, BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM },
            { psh1, BF_LEFT | BF_BOTTOM },
            { chx1, BF_LEFT | BF_BOTTOM },
            { IDOK, BF_RIGHT | BF_BOTTOM },
            { IDCANCEL, BF_RIGHT | BF_BOTTOM },
        };
        s_pLayout = LayoutInit(hwnd, layouts, _countof(layouts));
        LayoutEnableResize(s_pLayout, TRUE);

        if (xg_nPatWndX != CW_USEDEFAULT && xg_nPatWndCX != CW_USEDEFAULT)
        {
            MoveWindow(hwnd, xg_nPatWndX, xg_nPatWndY, xg_nPatWndCX, xg_nPatWndCY, TRUE);
        }
        else
        {
            RECT rc;
            XgCenterDialog(hwnd);
            GetWindowRect(hwnd, &rc);
            xg_nPatWndX = rc.left;
            xg_nPatWndY = rc.top;
            xg_nPatWndCX = rc.right - rc.left;
            xg_nPatWndCY = rc.bottom - rc.top;
        }

        SetFocus(GetDlgItem(hwnd, lst1));
        return FALSE;
    }

    // 黒マスパターンのコピー。
    XG_NOINLINE
    void OnCopy(HWND hwnd)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        const auto i = ListBox_GetCurSel(hLst1);
        if (i == LB_ERR || i >= static_cast<int>(s_patterns.size()))
            return;

        auto& pat = s_patterns[i];
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                XgPasteBoard(xg_hMainWnd, pat.text);
                XgCopyBoard(xg_hMainWnd);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
        }
        XgUpdateImage(xg_hMainWnd, 0, 0);

        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(xg_hMainWnd);

        EndDialog(hwnd, IDCANCEL);
    }

    // 黒マスパターンで「OK」ボタンを押した。
    XG_NOINLINE
    void OnOK(HWND hwnd)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        const auto i = ListBox_GetCurSel(hLst1);
        if (i == LB_ERR || i >= static_cast<int>(s_patterns.size()))
            return;

        auto& pat = s_patterns[i];
        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                // コピー＆貼り付け。
                XgPasteBoard(xg_hMainWnd, pat.text);
                XgCopyBoard(xg_hMainWnd);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
        }

        // ダイアログを閉じる。
        EndDialog(hwnd, IDOK);

        // 答えを表示するか？
        xg_bShowAnswerOnGenerate = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
    }

    // ダウンロードする。
    void OnDownload(HWND hwnd) noexcept
    {
        ShellExecuteW(hwnd, nullptr, L"https://katahiromz.web.fc2.com/xword/patterns", nullptr, nullptr, SW_SHOWNORMAL);
    }

    XG_NOINLINE
    int GetType0() noexcept
    {
        if (IsDlgButtonChecked(m_hWnd, rad1) == BST_CHECKED)
            return rad1;
        if (IsDlgButtonChecked(m_hWnd, rad2) == BST_CHECKED)
            return rad2;
        if (IsDlgButtonChecked(m_hWnd, rad3) == BST_CHECKED)
            return rad3;
        return -1;
    }

    XG_NOINLINE
    int GetType1() noexcept
    {
        if (IsDlgButtonChecked(m_hWnd, rad4) == BST_CHECKED)
            return rad4;
        if (IsDlgButtonChecked(m_hWnd, rad5) == BST_CHECKED)
            return rad5;
        if (IsDlgButtonChecked(m_hWnd, rad6) == BST_CHECKED)
            return rad6;
        return -1;
    }

    // WM_COMMAND
    XG_NOINLINE
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        switch (id)
        {
        case IDOK:
            OnOK(hwnd);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            OnCopy(hwnd);
            break;
        case psh2:
            OnDownload(hwnd);
            break;
        case lst1:
            if (codeNotify == LBN_DBLCLK)
            {
                OnOK(hwnd);
            }
            break;
        case rad1:
        case rad2:
        case rad3:
        case rad4:
        case rad5:
        case rad6:
            RefreshContents(hwnd, GetType0(), GetType1());
            break;
        default:
            break;
        }
    }

    // WM_MEASUREITEM
    XG_NOINLINE
    void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) noexcept
    {
        // リストボックスの lst1 か？
        if (lpMeasureItem->CtlType != ODT_LISTBOX || lpMeasureItem->CtlID != lst1)
            return;

        HDC hDC = CreateCompatibleDC(nullptr);
        SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
        TEXTMETRIC tm;
        GetTextMetrics(hDC, &tm);
        DeleteDC(hDC);
        lpMeasureItem->itemWidth = cxCell1 * (XG_MAX_PAT_SIZE1 + 1) + 3;
        lpMeasureItem->itemHeight = cyCell1 * XG_MAX_PAT_SIZE1 + 3 + tm.tmHeight;
    }

    // WM_DRAWITEM
    XG_NOINLINE
    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
    {
        // リストボックスの lst1 か？
        if (lpDrawItem->CtlType != ODT_LISTBOX || lpDrawItem->CtlID != lst1)
            return;

        // データがパターンのインデックスか？
        auto lParam = lpDrawItem->itemData;
        if (static_cast<int>(lParam) >= static_cast<int>(s_patterns.size()))
            return;

        // インデックスに対応するパターンを取得。
        const auto& pat = s_patterns[lParam];
        HDC hDC = lpDrawItem->hDC;
        RECT rcItem = lpDrawItem->rcItem;

        // 必要ならフォーカス枠を描く。
        if (lpDrawItem->itemAction & ODA_FOCUS)
        {
            DrawFocusRect(hDC, &rcItem);
        }

        // その他は無視。
        if (!(lpDrawItem->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
            return;

        // サイズを表すテキスト。
        WCHAR szText[64];
        StringCchPrintfW(szText, _countof(szText), L"%d x %d = %d",
                         static_cast<int>(pat.num_columns), static_cast<int>(pat.num_rows),
                         static_cast<int>(pat.num_columns * pat.num_rows));

        // 背景とテキストを描画する。
        SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
        SetBkMode(hDC, TRANSPARENT);
        TEXTMETRIC tm;
        GetTextMetrics(hDC, &tm);
        if (lpDrawItem->itemState & ODS_SELECTED)
        {
            FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_HIGHLIGHT));
            SetTextColor(hDC, COLOR_HIGHLIGHTTEXT);
        }
        else
        {
            FillRect(hDC, &rcItem, GetSysColorBrush(COLOR_WINDOW));
            SetTextColor(hDC, COLOR_WINDOWTEXT);
        }
        DrawTextW(hDC, szText, -1, &rcItem, DT_CENTER | DT_TOP | DT_SINGLELINE);

        // 必要ならフォーカス枠を描く。
        if (lpDrawItem->itemState & ODS_FOCUS)
        {
            DrawFocusRect(hDC, &rcItem);
        }

        // パターンデータ。
        const auto& data = pat.data;

        // 描画項目のサイズ。
        const int cxItem = rcItem.right - rcItem.left;

        INT cxCell = cxCell1, cyCell = cyCell1;
        if (pat.num_columns > XG_MAX_PAT_SIZE1 || pat.num_rows > XG_MAX_PAT_SIZE1)
        {
            const int cx = cxCell1 * XG_MAX_PAT_SIZE1, cy = cyCell1 * XG_MAX_PAT_SIZE1;
            cxCell = cx / pat.num_columns;
            cyCell = cy / pat.num_rows;
            cxCell = cyCell = std::min(cxCell, cyCell);
        }

        // メモリーデバイスコンテキストを作成。
        if (HDC hdcMem = CreateCompatibleDC(hDC))
        {
            const int cx = cxCell * pat.num_columns, cy = cyCell * pat.num_rows; // 全体のサイズ。
            // ビットマップを作成する。
            if (HBITMAP hbm = XgCreate24BppBitmap(hdcMem, cx + 3, cy + 3))
            {
                // ビットマップを選択。
                HGDIOBJ hbmOld = SelectObject(hdcMem, hbm);
                // 四角形を描く。
                SelectObject(hdcMem, GetStockPen(BLACK_PEN)); // 黒いペン
                SelectObject(hdcMem, GetStockBrush(WHITE_BRUSH)); // 白いブラシ
                Rectangle(hdcMem, 0, 0, cx + 2, cy + 2);
                // 黒マスを描画する。
                for (int y = 0; y < pat.num_rows; ++y)
                {
                    RECT rc;
                    for (int x = 0; x < pat.num_columns; ++x)
                    {
                        rc.left = 1 + x * cxCell;
                        rc.top = 1 + y * cyCell;
                        rc.right = rc.left + cxCell;
                        rc.bottom = rc.top + cyCell;
                        if (data[x + pat.num_columns * y] == ZEN_BLACK)
                            FillRect(hdcMem, &rc, GetStockBrush(BLACK_BRUSH));
                    }
                }
                // 境界線を描画する。
                for (int y = 0; y < pat.num_rows + 1; ++y)
                {
                    MoveToEx(hdcMem, 1, 1 + y * cyCell, nullptr);
                    LineTo(hdcMem, 1 + pat.num_columns * cxCell, 1 + y * cyCell);
                }
                for (int x = 0; x < pat.num_columns + 1; ++x)
                {
                    MoveToEx(hdcMem, 1 + x * cxCell, 1, nullptr);
                    LineTo(hdcMem, 1 + x * cxCell, 1 + pat.num_rows * cyCell);
                }
                // ビットマップイメージをhDCに転送する。
                BitBlt(hDC,
                       rcItem.left + (cxItem - (cx + 3)) / 2,
                       rcItem.top + tm.tmHeight,
                       cx + 3, cy + 3, hdcMem, 0, 0, SRCCOPY);
                // ビットマップの選択を解除する。
                SelectObject(hdcMem, hbmOld);
                // ビットマップを破棄する。
                DeleteObject(hbm);
            }
            // メモリーデバイスコンテキストを破棄する。
            DeleteDC(hdcMem);
        }
    }

    void OnDestroy(HWND hwnd)
    {
        if (s_pLayout)
        {
            LayoutDestroy(s_pLayout);
            s_pLayout = nullptr;
        }
    }

    void OnMove(HWND hwnd, int x, int y) noexcept
    {
        RECT rc;
        if (!IsMinimized(hwnd) && !IsMaximized(hwnd))
        {
            GetWindowRect(hwnd, &rc);
            xg_nPatWndX = rc.left;
            xg_nPatWndY = rc.top;
        }
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        RECT rc;

        LayoutUpdate(hwnd, s_pLayout, nullptr, 0);

        if (!IsMinimized(hwnd) && !IsMaximized(hwnd))
        {
            GetWindowRect(hwnd, &rc);
            xg_nPatWndCX = rc.right - rc.left;
            xg_nPatWndCY = rc.bottom - rc.top;
        }
    }

    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept
    {
        lpMinMaxInfo->ptMinTrackSize.x = 600;
        lpMinMaxInfo->ptMinTrackSize.y = 300;
    }

    // 「黒マスパターン」ダイアログプロシージャ。
    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_MEASUREITEM, OnMeasureItem);
            HANDLE_MSG(hwnd, WM_DRAWITEM, OnDrawItem);
            HANDLE_MSG(hwnd, WM_MOVE, OnMove);
            HANDLE_MSG(hwnd, WM_SIZE, OnSize);
            HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
            HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_BLOCKPATTERN);
    }
};
