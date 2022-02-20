#pragma once

#include "XG_BoxWindow.hpp"

// ボックス。
extern std::vector<std::unique_ptr<XG_BoxWindow> > xg_boxes;

class XG_CanvasWindow : public XG_Window
{
public:
    HWND m_hwndParent;

    XG_CanvasWindow() : m_hwndParent(NULL)
    {
    }

    ~XG_CanvasWindow()
    {
    }

    virtual LPCTSTR GetWndClassName() const override
    {
        return TEXT("XG_CanvasWindow");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx) override
    {
        wcx.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_hwndParent = GetParent(hwnd);
        return TRUE;
    }

    // 左クリックされた。
    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        int i, j;
        RECT rc;
        POINT pt;
        INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

        // ダブルクリックは無視。
        if (!fDoubleClick)
            return;

        // 左ボタンが離された位置を求める。
        pt.x = x + XgGetHScrollPos();
        pt.y = y + XgGetVScrollPos();

        // それぞれのマスについて調べる。
        for (i = 0; i < xg_nRows; i++) {
            for (j = 0; j < xg_nCols; j++) {
                // マスの矩形を求める。
                ::SetRect(&rc,
                    static_cast<int>(xg_nMargin + j * nCellSize),
                    static_cast<int>(xg_nMargin + i * nCellSize),
                    static_cast<int>(xg_nMargin + (j + 1) * nCellSize),
                    static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

                // マスの中か？
                if (::PtInRect(&rc, pt)) {
                    // キャレットを移動して、イメージを更新する。
                    XgSetCaretPos(i, j);
                    XgEnsureCaretVisible(hwnd);
                    XgUpdateStatusBar(hwnd);

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
                    XgUpdateImage(hwnd);
                    return;
                }
            }
        }
    }

    // マウスの左ボタンが離された。
    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        int i, j;
        RECT rc;
        POINT pt;
        INT nCellSize = xg_nCellSize * xg_nZoomRate / 100;

        // 左ボタンが離された位置を求める。
        pt.x = x + XgGetHScrollPos();
        pt.y = y + XgGetVScrollPos();

        // それぞれのマスについて調べる。
        for (i = 0; i < xg_nRows; i++) {
            for (j = 0; j < xg_nCols; j++) {
                // マスの矩形を求める。
                ::SetRect(&rc,
                    static_cast<int>(xg_nMargin + j * nCellSize),
                    static_cast<int>(xg_nMargin + i * nCellSize),
                    static_cast<int>(xg_nMargin + (j + 1) * nCellSize),
                    static_cast<int>(xg_nMargin + (i + 1) * nCellSize));

                // マスの中か？
                if (::PtInRect(&rc, pt)) {
                    // キャレットを移動して、イメージを更新する。
                    XgSetCaretPos(i, j);
                    XgEnsureCaretVisible(hwnd);
                    XgUpdateStatusBar(hwnd);

                    // イメージを更新する。
                    XgUpdateImage(hwnd);
                    return;
                }
            }
        }
    }

    // マウスの中央ボタンが押された。
    void OnMButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        if (!xg_bMButtonDragging)
        {
            xg_bMButtonDragging = TRUE;
            xg_ptMButtonDragging = { x, y };
            ::SetCapture(hwnd);
            ::SetCursor(::LoadCursor(xg_hInstance, MAKEINTRESOURCEW(IDC_MYHAND)));
        }
    }

    // マウスの中央ボタンでドラッグされた。
    void OnMouseScroll(HWND hwnd, int x, int y)
    {
        SCROLLINFO si;

        // 横スクロール情報を取得する。
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        XgGetHScrollInfo(&si);
        INT x0 = si.nPos;
        // 縦スクロール情報を取得する。
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        XgGetVScrollInfo(&si);
        INT y0 = si.nPos;

        x0 += xg_ptMButtonDragging.x - x;
        y0 += xg_ptMButtonDragging.y - y;

        // スクロール情報を設定し、イメージを更新する。
        si.fMask = SIF_POS;
        si.nPos = x0;
        XgSetHScrollInfo(&si, FALSE);
        si.fMask = SIF_POS;
        si.nPos = y0;
        XgSetVScrollInfo(&si, FALSE);

        xg_ptMButtonDragging.x = x;
        xg_ptMButtonDragging.y = y;

        // 画面を更新する。
        XgUpdateImage(hwnd, x0, y0);

        RECT rcClient;
        XgGetRealClientRect(hwnd, &rcClient);
        InvalidateRect(hwnd, &rcClient, TRUE);

        ::SetCursor(::LoadCursor(xg_hInstance, MAKEINTRESOURCEW(IDC_MYHAND)));
    }

    // ウィンドウ上でマウスが動いた。
    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (xg_bMButtonDragging && ::GetCapture() == hwnd)
        {
            OnMouseScroll(hwnd, x, y);
        }
    }

    // マウスの中央ボタンが離された。
    void OnMButtonUp(HWND hwnd, int x, int y, UINT flags)
    {
        if (xg_bMButtonDragging && ::GetCapture() == hwnd)
        {
            OnMouseScroll(hwnd, x, y);
            xg_bMButtonDragging = FALSE;
            ::ReleaseCapture();
            ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
        }
    }

    // 右クリックされた。
    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        OnLButtonUp(hwnd, x, y, keyFlags);

        HMENU hMenu = XgLoadPopupMenu(hwnd, 0);
        HMENU hSubMenu = GetSubMenu(hMenu, 0);

        // スクリーン座標へ変換する。
        POINT pt;
        pt.x = x;
        pt.y = y;
        ::ClientToScreen(hwnd, &pt);

        // 右クリックメニューを表示する。
        ::SetForegroundWindow(hwnd);
        ::TrackPopupMenu(
            hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN,
            pt.x, pt.y, 0, hwnd, NULL);
        ::PostMessageW(hwnd, WM_NULL, 0, 0);

        ::DestroyMenu(hMenu);
    }

    // マウスホイールが回転した。
    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
    {
        POINT pt = {xPos, yPos};

        RECT rc;
        if (::GetWindowRect(xg_hHintsWnd, &rc) && ::PtInRect(&rc, pt)) {
            INT x = xPos, y = yPos;
            FORWARD_WM_MOUSEWHEEL(xg_hHintsWnd, rc.left, rc.top,
                zDelta, fwKeys, ::SendMessageW);
        } else {
            if (::GetAsyncKeyState(VK_CONTROL) < 0) {
                if (zDelta < 0)
                    ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMOUT, 0);
                else if (zDelta > 0)
                    ::PostMessageW(hwnd, WM_COMMAND, ID_ZOOMIN, 0);
            } else if (::GetAsyncKeyState(VK_SHIFT) < 0) {
                if (zDelta < 0)
                    ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                else if (zDelta > 0)
                    ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            } else {
                if (zDelta < 0)
                    ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
                else if (zDelta > 0)
                    ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
            }
        }
    }

    // 背景を描画する。
    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        return TRUE; // 描画しない。
    }

    // ウィンドウを描画する。
    void OnPaint(HWND hwnd)
    {
        // ツールバーがなければ、初期化の前なので、無視する。
        if (xg_hToolBar == NULL)
            return;

        // クロスワードの描画サイズを取得する。
        SIZE siz;
        ForDisplay for_display;
        XgGetXWordExtent(&siz);

        // 描画を開始する。
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(hwnd, &ps);
        assert(hdc);
        if (hdc) {
            // クライアント領域を得る。
            MRect rcClient;
            XgGetRealClientRect(hwnd, &rcClient);

            // スクロール位置を取得する。
            int x = XgGetHScrollPos();
            int y = XgGetVScrollPos();

            // 背景を描画する。
            INT iSaveDC = SaveDC(hdc);
            ::ExcludeClipRect(hdc, rcClient.left - x, rcClient.top - y,
                              rcClient.left - x + siz.cx,
                              rcClient.top - y + siz.cy);
            FillRect(hdc, &rcClient, GetSysColorBrush(COLOR_3DFACE));
            RestoreDC(hdc, iSaveDC);

            // イメージがない場合は、イメージを取得する。
            if (xg_hbmImage == nullptr) {
                if (xg_bSolved && xg_bShowAnswer)
                    xg_hbmImage = XgCreateXWordImage(xg_solution, &siz, true);
                else
                    xg_hbmImage = XgCreateXWordImage(xg_xword, &siz, true);
            }

            // ビットマップ イメージをウィンドウに転送する。
            HDC hdcMem = ::CreateCompatibleDC(hdc);
            ::IntersectClipRect(hdc, rcClient.left, rcClient.top,
                rcClient.right, rcClient.bottom);
            HGDIOBJ hbmOld = ::SelectObject(hdcMem, xg_hbmImage);
            ::BitBlt(hdc, rcClient.left - x, rcClient.top - y,
                siz.cx, siz.cy, hdcMem, 0, 0, SRCCOPY);
            ::SelectObject(hdcMem, hbmOld);
            ::DeleteDC(hdcMem);

            // 描画を終了する。
            ::EndPaint(hwnd, &ps);
        }
    }

    // 横スクロールする。
    void OnHScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, int pos)
    {
        SCROLLINFO si;

        // 横スクロール情報を取得する。
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        XgGetHScrollInfo(&si);

        // コードに応じて位置情報を設定する。
        switch (code) {
        case SB_LEFT:
            si.nPos = si.nMin;
            break;

        case SB_RIGHT:
            si.nPos = si.nMax;
            break;

        case SB_LINELEFT:
            si.nPos -= 10;
            if (si.nPos < si.nMin)
                si.nPos = si.nMin;
            break;

        case SB_LINERIGHT:
            si.nPos += 10;
            if (si.nPos > si.nMax)
                si.nPos = si.nMax;
            break;

        case SB_PAGELEFT:
            si.nPos -= si.nPage;
            if (si.nPos < si.nMin)
                si.nPos = si.nMin;
            break;

        case SB_PAGERIGHT:
            si.nPos += si.nPage;
            if (si.nPos > si.nMax)
                si.nPos = si.nMax;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            si.nPos = pos;
            break;
        }

        // スクロール情報を設定し、イメージを更新する。
        XgSetHScrollInfo(&si, TRUE);
        XgUpdateImage(hwnd, si.nPos, XgGetVScrollPos());
        XgUpdateCaretPos();

        // ボックスの位置を更新。
        PostMessage(hwnd, WM_COMMAND, ID_MOVEBOXES, 0);
    }

    // 縦スクロールする。
    void OnVScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, int pos)
    {
        SCROLLINFO si;

        // 縦スクロール情報を取得する。
        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
        XgGetVScrollInfo(&si);

        // コードに応じて位置情報を設定する。
        switch (code) {
        case SB_TOP:
            si.nPos = si.nMin;
            break;

        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;

        case SB_LINEUP:
            si.nPos -= 10;
            if (si.nPos < si.nMin)
                si.nPos = si.nMin;
            break;

        case SB_LINEDOWN:
            si.nPos += 10;
            if (si.nPos > si.nMax)
                si.nPos = si.nMax;
            break;

        case SB_PAGEUP:
            si.nPos -= si.nPage;
            if (si.nPos < si.nMin)
                si.nPos = si.nMin;
            break;

        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            if (si.nPos > si.nMax)
                si.nPos = si.nMax;
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK:
            si.nPos = pos;
            break;
        }

        // スクロール情報を設定し、イメージを更新する。
        XgSetVScrollPos(si.nPos, TRUE);
        XgUpdateImage(hwnd, XgGetHScrollPos(), si.nPos);
        XgUpdateCaretPos();

        // ボックスの位置を更新。
        PostMessage(hwnd, WM_COMMAND, ID_MOVEBOXES, 0);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        FORWARD_WM_COMMAND(m_hwndParent, id, hwndCtl, codeNotify, PostMessageW);
    }

    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        POINT pt;
        DragQueryPoint(hdrop, &pt);
        ClientToScreen(hwnd, &pt);

        DoDropFile(hwnd, hdrop, pt);
    }

    // ファイルがドロップされた。
    BOOL DoDropFile(HWND hwnd, HDROP hDrop, POINT pt)
    {
        // 最初のファイルのパス名を取得する。
        WCHAR szFile[MAX_PATH], szTarget[MAX_PATH];
        ::DragQueryFileW(hDrop, 0, szFile, ARRAYSIZE(szFile));
        ::DragFinish(hDrop);

        // LOOKSファイルだった場合は自動で適用する。
        LPWSTR pchDotExt = PathFindExtensionW(szFile);
        if (lstrcmpiW(pchDotExt, L".looks") == 0)
        {
            XG_SettingsDialog dialog;
            dialog.m_pszAutoFile = szFile;
            dialog.DoModal(hwnd);
            return TRUE;
        }

        // 画像ファイルだったら画像ボックスを作成する。
        if (lstrcmpiW(pchDotExt, L".bmp") == 0 ||
            lstrcmpiW(pchDotExt, L".emf") == 0 ||
            lstrcmpiW(pchDotExt, L".png") == 0 ||
            lstrcmpiW(pchDotExt, L".gif") == 0 ||
            lstrcmpiW(pchDotExt, L".jpg") == 0)
        {
            ScreenToClient(hwnd, &pt);

            INT i1, j1;
            XgSetCellPosition(pt.x, pt.y, i1, j1, FALSE);
            INT i2 = i1 + 2, j2 = j1 + 2;

            if (i2 >= xg_nRows) {
                i2 = xg_nRows;
                i1 = i2 - 2;
            }
            if (j2 >= xg_nCols) {
                j2 = xg_nCols;
                j1 = j2 - 2;
            }

            auto ptr = new XG_PictureBoxWindow(szFile, i1, j1, i2, j2);
            if (ptr->CreateDx(hwnd)) {
                xg_boxes.emplace_back(ptr);
                return TRUE;
            } else {
                delete ptr;
            }

            return FALSE;
        }

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (XgGetPathOfShortcutW(szFile, szTarget))
            StringCbCopy(szFile, sizeof(szFile), szTarget);

        // ファイルを開く。
        if (!XgDoLoad(hwnd, szFile)) {
            XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_CANTLOAD), nullptr, MB_ICONERROR);
            return FALSE;
        }

        // 候補ウィンドウを破棄する。
        XgDestroyCandsWnd();
        // ヒントウィンドウを破棄する。
        XgDestroyHintsWnd();
        // キャレット位置を更新。
        XgSetCaretPos();
        // ズームを実際のウィンドウに合わせる。
        XgFitZoom(hwnd);
        // テーマを更新する。
        XgSetThemeString(xg_strTheme);
        XgUpdateTheme(hwnd);
        // イメージを更新する。
        XgMarkUpdate();
        XgUpdateImage(hwnd, 0, 0);
        // ヒントを表示する。
        XgShowHints(hwnd);
        // ツールバーのUIを更新する。
        XgUpdateToolBarUI(hwnd);
        // ルールを更新する。
        XgUpdateRules(hwnd);
        return TRUE;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
            HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
            HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
            HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
            HANDLE_MSG(hwnd, WM_MBUTTONDOWN, OnMButtonDown);
            HANDLE_MSG(hwnd, WM_MBUTTONUP, OnMButtonUp);
            HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
            HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
            HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
            HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
            HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
            HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        default:
            return DefProcDx(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
};
