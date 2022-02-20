#pragma once

#include "XG_Window.hpp"

#define XGWM_REDRAW (WM_USER + 101)

#define CXY_GRIP 8
#define X0 rc.left
#define X1 ((rc.left + rc.right - CXY_GRIP) / 2)
#define X2 (rc.right - CXY_GRIP)
#define Y0 rc.top
#define Y1 ((rc.top + rc.bottom - CXY_GRIP) / 2)
#define Y2 (rc.bottom - CXY_GRIP)
#define RECT0 X0, Y0, X0 + CXY_GRIP, Y0 + CXY_GRIP // Upper Left
#define RECT1 X1, Y0, X1 + CXY_GRIP, Y0 + CXY_GRIP // Top
#define RECT2 X2, Y0, X2 + CXY_GRIP, Y0 + CXY_GRIP // Upper Right
#define RECT3 X0, Y1, X0 + CXY_GRIP, Y1 + CXY_GRIP // Left
#define RECT4 X2, Y1, X2 + CXY_GRIP, Y1 + CXY_GRIP // Right
#define RECT5 X0, Y2, X0 + CXY_GRIP, Y2 + CXY_GRIP // Lower Left
#define RECT6 X1, Y2, X1 + CXY_GRIP, Y2 + CXY_GRIP // Bottom
#define RECT7 X2, Y2, X2 + CXY_GRIP, Y2 + CXY_GRIP // Lower Right

class XG_BoxWindow : public XG_Window
{
public:
    HWND m_hwndParent;
    HRGN m_hRgn;
    INT m_i1;
    INT m_j1;
    INT m_i2;
    INT m_j2;
    RECT m_rcOld;

    XG_BoxWindow(INT i1 = 0, INT j1 = 0, INT i2 = 1, INT j2 = 1)
        : m_hwndParent(NULL)
        , m_hRgn(NULL)
        , m_i1(i1)
        , m_j1(j1)
        , m_i2(i2)
        , m_j2(j2)
    {
    }

    virtual ~XG_BoxWindow()
    {
        DeleteObject(m_hRgn);
        m_hRgn = NULL;
    }

    virtual LPCTSTR GetWndClassName() const override
    {
        return TEXT("XG_BoxWindow");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx) override
    {
        wcx.style = CS_DBLCLKS;
    }

    HRGN DoGetRgn(HWND hwnd)
    {
        RECT rc, rcInner1, rcInner2;
        GetWindowRect(hwnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);

        rcInner1 = rc;
        InflateRect(&rcInner1, -CXY_GRIP / 2 + 1, -CXY_GRIP / 2 + 1);
        rcInner2 = rcInner1;
        InflateRect(&rcInner2, -3, -3);

        HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

        HRGN hInner1Rgn = CreateRectRgnIndirect(&rcInner1);
        UnionRgn(hRgn, hRgn, hInner1Rgn);
        DeleteObject(hInner1Rgn);
        HRGN hInner2Rgn = CreateRectRgnIndirect(&rcInner2);
        CombineRgn(hRgn, hRgn, hInner2Rgn, RGN_DIFF);
        DeleteObject(hInner2Rgn);

        HRGN hGripRgn;

        hGripRgn = CreateRectRgn(RECT0);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT1);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT2);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT3);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT4);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT5);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT6);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT7);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        RECT rcRect = rc;
        InflateRect(&rcRect, -CXY_GRIP, -CXY_GRIP);
        HRGN hRectRgn = CreateRectRgnIndirect(&rcRect);
        UnionRgn(hRgn, hRgn, hRectRgn);
        DeleteObject(hRectRgn);

        return hRgn;
    }

    void DoDrawGrips(HWND hwnd, HDC hDC, RECT& rc)
    {
        FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));

        RECT rcInner1 = rc;
        InflateRect(&rcInner1, -CXY_GRIP / 2, -CXY_GRIP / 2);

        HGDIOBJ hPenOld = SelectObject(hDC, CreatePen(PS_DOT, 1, GetSysColor(COLOR_HIGHLIGHT)));
        Rectangle(hDC, rcInner1.left, rcInner1.top, rcInner1.right, rcInner1.bottom);
        DeleteObject(SelectObject(hDC, hPenOld));

        RECT rcGrip;

        SetRect(&rcGrip, RECT0);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT1);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT2);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT3);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT4);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT5);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT6);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
        SetRect(&rcGrip, RECT7);
        FillRect(hDC, &rcGrip, GetSysColorBrush(COLOR_HIGHLIGHT));
    }

    void DoSetRgn(HWND hwnd)
    {
        HRGN hRgnOld = m_hRgn;
        m_hRgn = DoGetRgn(hwnd);
        SetWindowRgn(hwnd, m_hRgn, TRUE);
        DeleteObject(hRgnOld);
    }

    void Bound()
    {
        MRect rc;
        XgGetCellPosition(rc, m_i1, m_j1, m_i2, m_j2);

        RECT rcWnd;
        GetWindowRect(m_hWnd, &rcWnd);

        if (EqualRect(&rc, &rcWnd))
            return;

        InflateRect(&rc, CXY_GRIP, CXY_GRIP);

        ::SetWindowPos(m_hWnd, NULL, rc.left, rc.top, rc.Width(), rc.Height(),
                       SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOCOPYBITS |
                       SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_NOZORDER);

        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, NULL);
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_hwndParent = GetParent(hwnd);

        GetWindowRect(hwnd, &m_rcOld);
        DoSetRgn(hwnd);
        Bound();
        return TRUE;
    }

    BOOL OnNCActivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized)
    {
        return FALSE;
    }

    void OnNCPaint(HWND hwnd, HRGN hrgn)
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);

        HDC hDC = GetWindowDC(hwnd);
        DoDrawGrips(hwnd, hDC, rc);
        ReleaseDC(hwnd, hDC);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc)
    {
        return TRUE;
    }

    virtual void OnDraw(HWND hwnd, HDC hDC, const RECT& rc)
    {
        FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));
        MoveToEx(hDC, rc.left, rc.top, NULL);
        LineTo(hDC, rc.right, rc.bottom);
        MoveToEx(hDC, rc.right, rc.top, NULL);
        LineTo(hDC, rc.left, rc.bottom);
    }

    void OnPaint(HWND hwnd)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        OnDraw(hwnd, hDC, rc);
        EndPaint(hwnd, &ps);
    }

    UINT OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp)
    {
        lpcsp->rgrc[0] = lpcsp->rgrc[1];
        InflateRect(&lpcsp->rgrc[0], -CXY_GRIP, -CXY_GRIP);
        return WVR_REDRAW;
    }

    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        RECT rc, rcGrip;
        GetWindowRect(hwnd, &rc);

        POINT pt = { x, y };

        SetRect(&rcGrip, RECT0);
        if (PtInRect(&rcGrip, pt))
            return HTTOPLEFT;
        SetRect(&rcGrip, RECT1);
        if (PtInRect(&rcGrip, pt))
            return HTTOP;
        SetRect(&rcGrip, RECT2);
        if (PtInRect(&rcGrip, pt))
            return HTTOPRIGHT;
        SetRect(&rcGrip, RECT3);
        if (PtInRect(&rcGrip, pt))
            return HTLEFT;
        SetRect(&rcGrip, RECT4);
        if (PtInRect(&rcGrip, pt))
            return HTRIGHT;
        SetRect(&rcGrip, RECT5);
        if (PtInRect(&rcGrip, pt))
            return HTBOTTOMLEFT;
        SetRect(&rcGrip, RECT6);
        if (PtInRect(&rcGrip, pt))
            return HTBOTTOM;
        SetRect(&rcGrip, RECT7);
        if (PtInRect(&rcGrip, pt))
            return HTBOTTOMRIGHT;

        InflateRect(&rc, -CXY_GRIP, -CXY_GRIP);
        if (PtInRect(&rc, pt))
            return HTCAPTION;

        return HTTRANSPARENT;
    }

    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
    {
        lpMinMaxInfo->ptMinTrackSize.x = CXY_GRIP * 2;
        lpMinMaxInfo->ptMinTrackSize.y = CXY_GRIP * 2;
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        RECT rc = m_rcOld;
        MapWindowRect(NULL, m_hwndParent, &rc);
        InvalidateRect(m_hwndParent, &rc, FALSE);

        DoSetRgn(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);

        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, NULL);
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, NULL);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        RECT rc = m_rcOld;
        MapWindowRect(NULL, m_hwndParent, &rc);
        InvalidateRect(m_hwndParent, &rc, FALSE);

        DoSetRgn(hwnd);
        InvalidateRect(hwnd, NULL, FALSE);

        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, NULL);
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        if (id == 999)
        {
            KillTimer(hwnd, 999);

            PostMessageW(hwnd, XGWM_REDRAW, 0, 0);
        }
    }

    void DoRedraw(HWND hwnd)
    {
        MRect rc = m_rcOld;
        MapWindowRect(NULL, m_hwndParent, &rc);
        InvalidateRect(m_hwndParent, &rc, TRUE);
        InvalidateRect(hwnd, NULL, TRUE);

        GetWindowRect(hwnd, &m_rcOld);
        rc = m_rcOld;
        MapWindowRect(NULL, m_hwndParent, &rc);
        SetWindowPos(hwnd, NULL, rc.left, rc.top, rc.Width(), rc.Height(),
            SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOREPOSITION |
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_DEFERERASE);
    }

    BOOL CreateDx(HWND hwndParent)
    {
        DWORD style = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
        CreateWindowDx(hwndParent, NULL, style);
        return m_hWnd != NULL;
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_NCPAINT, OnNCPaint);
        HANDLE_MSG(hwnd, WM_NCCALCSIZE, OnNCCalcSize);
        HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        case WM_NCACTIVATE:
            return FALSE;
        case XGWM_REDRAW:
            DoRedraw(hwnd);
            break;
        case WM_CAPTURECHANGED:
        case WM_EXITSIZEMOVE:
            {
                RECT rc;
                GetWindowRect(hwnd, &rc);
                InflateRect(&rc, -CXY_GRIP, -CXY_GRIP);
                MapWindowRect(NULL, m_hwndParent, &rc);
                XgSetCellPosition(rc.left, rc.top, m_i1, m_j1, FALSE);
                XgSetCellPosition(rc.right, rc.bottom, m_i2, m_j2, TRUE);
                if (m_i1 >= m_i2)
                    m_i2 = m_i1 + 1;
                if (m_j1 >= m_j2)
                    m_j2 = m_j1 + 1;
                // ボックスの位置を更新。
                PostMessage(m_hwndParent, WM_COMMAND, ID_MOVEBOXES, 0);
                // 表示を更新。
                ::KillTimer(m_hWnd, 999);
                ::SetTimer(m_hWnd, 999, 300, NULL);
            }
            break;
        default:
            return DefProcDx(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
};

class XG_PictureBoxWindow : public XG_BoxWindow
{
public:
    HBITMAP m_hbm;
    HENHMETAFILE m_hEMF;
    std::wstring m_strFile;

    XG_PictureBoxWindow(const std::wstring& str, INT i1 = 0, INT j1 = 0, INT i2 = 1, INT j2 = 1)
        : XG_BoxWindow(i1, j1, i2, j2)
        , m_hbm(NULL)
        , m_hEMF(NULL)
    {
        SetFile(str.c_str());
    }

    void DoDelete()
    {
        ::DeleteObject(m_hbm);
        ::DeleteEnhMetaFile(m_hEMF);
        m_hbm = NULL;
        m_hEMF = NULL;
        m_strFile.clear();
    }

    ~XG_PictureBoxWindow()
    {
        DoDelete();
    }

    virtual void OnDraw(HWND hwnd, HDC hDC, const RECT& rc) override
    {
        if (m_hbm) {
            BITMAP bm;
            GetObject(m_hbm, sizeof(bm), &bm);
            if (HDC hMemDC = CreateCompatibleDC(NULL)) {
                HGDIOBJ hbmOld = SelectObject(hMemDC, m_hbm);
                StretchBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                    hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                SelectObject(hMemDC, hbmOld);
                DeleteDC(hMemDC);
            }
            return;
        }

        if (m_hEMF) {
            PlayEnhMetaFile(hDC, m_hEMF, &rc);
            return;
        }

        XG_BoxWindow::OnDraw(hwnd, hDC, rc);
    }

    BOOL SetFile(LPCWSTR pszFile)
    {
        DoDelete();

        if (XgLoadImage(pszFile, m_hbm, m_hEMF)) {
            m_strFile = pszFile;
            return TRUE;
        }

        return FALSE;
    }
};

class XG_TextBoxWindow : public XG_BoxWindow
{
public:
    std::wstring m_strText;

    XG_TextBoxWindow(const std::wstring& str, INT i1 = 0, INT j1 = 0, INT i2 = 1, INT j2 = 1)
        : XG_BoxWindow(i1, j1, i2, j2)
    {
        SetText(str.c_str());
    }

    ~XG_TextBoxWindow()
    {
    }

    virtual void OnDraw(HWND hwnd, HDC hDC, const RECT& rc) override
    {
        MRect rcText = rc;
        FillRect(hDC, &rcText, GetStockBrush(WHITE_BRUSH));

        MoveToEx(hDC, rc.left, rc.top, NULL);
        LineTo(hDC, rc.left, rc.bottom - 1);
        LineTo(hDC, rc.right - 1, rc.bottom - 1);
        LineTo(hDC, rc.right - 1, rc.top);
        LineTo(hDC, rc.left, rc.top);

        SetBkMode(hDC, TRANSPARENT);
        SetTextColor(hDC, RGB(0, 0, 0));

        LOGFONTW lf = *XgGetUIFont();

        for (INT height = 20; height >= 4; --height) {
            lf.lfHeight = -height * xg_nZoomRate / 100;
            HFONT hFont = CreateFontIndirectW(&lf);
            HGDIOBJ hFontOld = SelectObject(hDC, hFont);
            UINT uFormat = DT_LEFT | DT_TOP | DT_NOPREFIX | DT_WORDBREAK;
            MRect rcCalc = rcText;
            DrawTextW(hDC, m_strText.c_str(), -1, &rcCalc, uFormat | DT_CALCRECT);
            if (rcCalc.Height() > rcText.Height()) {
                SelectObject(hDC, hFontOld);
                DeleteObject(hFont);
                continue;
            }
            rcText.top = rc.top + (rcText.Height() - rcCalc.Height()) / 2;
            rcText.bottom = rcText.top + rcCalc.Height();
            uFormat = DT_CENTER | DT_TOP | DT_NOPREFIX | DT_WORDBREAK;
            DrawTextW(hDC, m_strText.c_str(), -1, &rcText, uFormat);
            SelectObject(hDC, hFontOld);
            DeleteObject(hFont);
            break;
        }
    }

    BOOL SetText(LPCWSTR pszText)
    {
        m_strText = pszText;
        return TRUE;
    }
};
