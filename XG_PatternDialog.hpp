#pragma once

#include "XG_Window.hpp"
#include "layout.h"
#include "XWordGiver.hpp"

// 黒マスパターンで答えを表示する。
extern BOOL xg_bShowAnswerOnPattern;
// 解を求める（黒マス追加なし）。
bool __fastcall XgOnSolve_NoAddBlack(HWND hwnd, bool bShowAnswer = true);

// パターンデータを読み込む。
BOOL XgLoadPatterns(LPCWSTR pszFileName, patterns_t& patterns);

class XG_PatternDialog : public XG_Dialog
{
public:
    // 黒マスパターンのデータ。
    inline static patterns_t s_patterns;
    inline static LAYOUT_DATA *s_pLayout = NULL;

    // 「黒マスパターン」ダイアログの位置とサイズ。
    inline static INT xg_nPatWndX = CW_USEDEFAULT;
    inline static INT xg_nPatWndY = CW_USEDEFAULT;
    inline static INT xg_nPatWndCX = CW_USEDEFAULT;
    inline static INT xg_nPatWndCY = CW_USEDEFAULT;

    // 黒マスパターンで答えを表示する。
    inline static BOOL xg_bShowAnswerOnPattern = TRUE;

    XG_PatternDialog()
    {
    }

    // タイプによりフィルターを行う。
    BOOL FilterPatBySize(const XG_PATDATA& pat, INT type) {
        switch (type) {
        case -1:
            break;
        case rad1:
            if (pat.num_columns != pat.num_rows)
                return FALSE;
            if (!(pat.num_columns >= 13 && pat.num_rows >= 13))
                return FALSE;
            break;
        case rad2:
            if (pat.num_columns != pat.num_rows)
                return FALSE;
            if (!(8 <= pat.num_columns && pat.num_columns <= 12 &&
                8 <= pat.num_rows && pat.num_rows <= 12))
            {
                return FALSE;
            }
            break;
        case rad3:
            if (pat.num_columns != pat.num_rows)
                return FALSE;
            if (!(pat.num_columns <= 8 && pat.num_rows <= 8))
                return FALSE;
            break;
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

    BOOL RefreshContents(HWND hwnd, INT type)
    {
        // リストボックスをクリアする。
        ListBox_ResetContent(GetDlgItem(hwnd, lst1));

        // パターンデータを読み込む。
        WCHAR szPath[MAX_PATH];
        XgFindLocalFile(szPath, _countof(szPath), L"PAT.txt");
        XgLoadPatterns(szPath, s_patterns);

        // 抽出する。
        patterns_t pats;
        for (auto& pat : s_patterns)
        {
            if (FilterPatBySize(pat, type))
                pats.push_back(pat);
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
    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        XgCenterDialog(hwnd);

        if (s_pLayout)
        {
            LayoutDestroy(s_pLayout);
            s_pLayout = NULL;
        }

        if (xg_bShowAnswerOnPattern)
            CheckDlgButton(hwnd, chx1, BST_CHECKED);
        else
            CheckDlgButton(hwnd, chx1, BST_UNCHECKED);

        CheckRadioButton(hwnd, rad1, rad6, rad6);
        RefreshContents(hwnd, rad6);

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
        s_pLayout = LayoutInit(hwnd, layouts, ARRAYSIZE(layouts));
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
    void OnCopy(HWND hwnd)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        INT i = ListBox_GetCurSel(hLst1);
        if (i == LB_ERR || i >= INT(s_patterns.size()))
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
    void OnOK(HWND hwnd)
    {
        HWND hLst1 = GetDlgItem(hwnd, lst1);
        INT i = ListBox_GetCurSel(hLst1);
        if (i == LB_ERR || i >= INT(s_patterns.size()))
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
        xg_bShowAnswerOnPattern = (IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);

        // ズームを実際のウィンドウに合わせる。
        XgFitZoom(GetParent(hwnd));
        // ステータスバーを更新する。
        XgUpdateStatusBar(GetParent(hwnd));

        {
            auto sa1 = std::make_shared<XG_UndoData_SetAll>();
            auto sa2 = std::make_shared<XG_UndoData_SetAll>();
            sa1->Get();
            {
                // 解を求める（黒マス追加なし）。
                XgOnSolve_NoAddBlack(xg_hMainWnd, xg_bShowAnswerOnPattern);
            }
            sa2->Get();
            // 元に戻す情報を設定する。
            xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
        }

        // 表示を更新する。
        XgUpdateImage(xg_hMainWnd, 0, 0);

        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(xg_hMainWnd);
    }

    // ダウンロードする。
    void OnDownload(HWND hwnd)
    {
        ShellExecuteW(hwnd, NULL, L"https://katahiromz.web.fc2.com/xword/patterns", NULL, NULL, SW_SHOWNORMAL);
    }

    // WM_COMMAND
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
            RefreshContents(hwnd, id);
            break;
        }
    }

    const INT cxCell = 6, cyCell = 6; // 小さなセルのサイズ。

    // WM_MEASUREITEM
    void OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem)
    {
        // リストボックスの lst1 か？
        if (lpMeasureItem->CtlType != ODT_LISTBOX || lpMeasureItem->CtlID != lst1)
            return;

        HDC hDC = CreateCompatibleDC(NULL);
        SelectObject(hDC, GetStockFont(DEFAULT_GUI_FONT));
        TEXTMETRIC tm;
        GetTextMetrics(hDC, &tm);
        DeleteDC(hDC);
        lpMeasureItem->itemWidth = cxCell * 19 + 3;
        lpMeasureItem->itemHeight = cyCell * 18 + 3 + tm.tmHeight;
    }

    // WM_DRAWITEM
    void OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
    {
        // リストボックスの lst1 か？
        if (lpDrawItem->CtlType != ODT_LISTBOX || lpDrawItem->CtlID != lst1)
            return;

        // データがパターンのインデックスか？
        LPARAM lParam = lpDrawItem->itemData;
        if ((int)lParam >= (int)s_patterns.size())
            return;

        // インデックスに対応するパターンを取得。
        const auto& pat = s_patterns[(int)lParam];
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
        StringCbPrintfW(szText, sizeof(szText), L"%u x %u", int(pat.num_columns), int(pat.num_rows));

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
        INT cxItem = rcItem.right - rcItem.left;

        // メモリーデバイスコンテキストを作成。
        if (HDC hdcMem = CreateCompatibleDC(hDC))
        {
            INT cx = cxCell * pat.num_columns, cy = cyCell * pat.num_rows; // 全体のサイズ。
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
                for (INT y = 0; y < pat.num_rows; ++y)
                {
                    RECT rc;
                    for (INT x = 0; x < pat.num_columns; ++x)
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
                for (INT y = 0; y < pat.num_rows + 1; ++y)
                {
                    MoveToEx(hdcMem, 1, 1 + y * cyCell, NULL);
                    LineTo(hdcMem, 1 + pat.num_columns * cxCell, 1 + y * cyCell);
                }
                for (INT x = 0; x < pat.num_columns + 1; ++x)
                {
                    MoveToEx(hdcMem, 1 + x * cxCell, 1, NULL);
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
            s_pLayout = NULL;
        }
    }

    void OnMove(HWND hwnd, int x, int y)
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

        LayoutUpdate(hwnd, s_pLayout, NULL, 0);

        if (!IsMinimized(hwnd) && !IsMaximized(hwnd))
        {
            GetWindowRect(hwnd, &rc);
            xg_nPatWndCX = rc.right - rc.left;
            xg_nPatWndCY = rc.bottom - rc.top;
        }
    }

    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
    {
        lpMinMaxInfo->ptMinTrackSize.x = 600;
        lpMinMaxInfo->ptMinTrackSize.y = 300;
    }

    // 「黒マスパターン」ダイアログプロシージャ。
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_BLOCKPATTERN);
    }
};
