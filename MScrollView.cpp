////////////////////////////////////////////////////////////////////////////
// MScrollView.cpp -- MZC3 scroll view
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#include "MScrollView.hpp"

#ifdef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE  /*empty*/
    #include "MScrollView_inl.hpp"
#endif

////////////////////////////////////////////////////////////////////////////

void MScrollView::SetCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    assert(rcCtrl.left >= 0);
    assert(rcCtrl.top >= 0);
    MScrollCtrlInfo* info = FindCtrlInfo(hwndCtrl);
    if (info)
        info->m_rcCtrl = rcCtrl;
    else
        AddCtrlInfo(hwndCtrl, rcCtrl);
}

void MScrollView::SetCtrlInfo(
    HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    assert(ptCtrl.x >= 0);
    assert(ptCtrl.y >= 0);
    MScrollCtrlInfo* info = FindCtrlInfo(hwndCtrl);
    if (info)
        info->m_rcCtrl = MRect(ptCtrl, sizCtrl);
    else
        AddCtrlInfo(hwndCtrl, ptCtrl, sizCtrl);
}

MScrollCtrlInfo* MScrollView::FindCtrlInfo(HWND hwndCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    size_t siz = size();
    for (size_t i = 0; i < siz; ++i)
    {
        if (m_vecInfo[i].m_hwndCtrl == hwndCtrl)
            return &m_vecInfo[i];
    }
    return NULL;
}

const MScrollCtrlInfo* MScrollView::FindCtrlInfo(HWND hwndCtrl) const
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    size_t siz = size();
    for (size_t i = 0; i < siz; ++i)
    {
        if (m_vecInfo[i].m_hwndCtrl == hwndCtrl)
            return &m_vecInfo[i];
    }
    return NULL;
}

void MScrollView::RemoveCtrlInfo(HWND hwndCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    std::vector<MScrollCtrlInfo>::iterator it, end = m_vecInfo.end();
    for (it = m_vecInfo.begin(); it != end; ++it)
    {
        if (it->m_hwndCtrl == hwndCtrl)
        {
            m_vecInfo.erase(it);
            break;
        }
    }
}

// ensure visible
void MScrollView::EnsureCtrlVisible(HWND hwndCtrl, bool update_all/* = true*/)
{
    MRect rcClient;
    GetClientRect(m_hwndParent, &rcClient);

    INT yScroll = ::GetScrollPos(m_hwndParent, SB_VERT);
    const int siz = static_cast<int>(size());
    for (int i = 0; i < siz; ++i)
    {
        if (m_vecInfo[i].m_hwndCtrl != hwndCtrl)
            continue;

        if (!::IsWindow(hwndCtrl))
            break;

        MRect& rcCtrl = m_vecInfo[i].m_rcCtrl;
        if (rcCtrl.bottom > yScroll + rcClient.Height())
        {
            ::SetScrollPos(m_hwndParent, SB_VERT, rcCtrl.bottom - rcClient.Height(), FALSE);
        }
        if (rcCtrl.top < yScroll)
        {
            ::SetScrollPos(m_hwndParent, SB_VERT, rcCtrl.top, FALSE);
        }
        break;
    }

    if (update_all)
        UpdateAll();
}

void MScrollView::SetExtentForAllCtrls()
{
    Extent().cx = Extent().cy = 0;
    const int siz = static_cast<int>(size());
    for (int i = 0; i < siz; ++i)
    {
        if (!::IsWindowVisible(m_vecInfo[i].m_hwndCtrl))
            continue;

        MRect& rcCtrl = m_vecInfo[i].m_rcCtrl;
        if (Extent().cx < rcCtrl.right)
            Extent().cx = rcCtrl.right;
        if (Extent().cy < rcCtrl.bottom)
            Extent().cy = rcCtrl.bottom;
    }

    MRect rc;
    ::GetClientRect(m_hwndParent, &rc);

    SCROLLINFO si = { sizeof(si) };
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    si.nMax = Extent().cx;
    si.nPage = rc.Width();
    ::SetScrollInfo(m_hwndParent, SB_HORZ, &si, FALSE);
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    si.nMax = Extent().cy;
    si.nPage = rc.Height();
    ::SetScrollInfo(m_hwndParent, SB_VERT, &si, FALSE);

    ::InvalidateRect(m_hwndParent, NULL, TRUE);
}

void MScrollView::UpdateCtrlsPos()
{
    INT xScroll = ::GetScrollPos(m_hwndParent, SB_HORZ);
    INT yScroll = ::GetScrollPos(m_hwndParent, SB_VERT);
    const int siz = static_cast<int>(size());
    HDWP hDWP = ::BeginDeferWindowPos(siz);
    if (hDWP)
    {
        for (int i = 0; i < siz; ++i)
        {
            MRect& rcCtrl = m_vecInfo[i].m_rcCtrl;
            hDWP = ::DeferWindowPos(hDWP, m_vecInfo[i].m_hwndCtrl,
                NULL,
                rcCtrl.left - xScroll, rcCtrl.top - yScroll,
                rcCtrl.Width(), rcCtrl.Height(),
                SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS);
        }
        ::EndDeferWindowPos(hDWP);
    }
}

INT MScrollView::GetNextPos(INT bar, INT nSB_, INT pos) const
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    ::GetScrollInfo(m_hwndParent, bar, &si);

    switch (nSB_)
    {
    case SB_TOP:
        return 0;

    case SB_BOTTOM:
        return si.nMax - si.nPage;

    case SB_LINELEFT:
        return std::max(0, INT(si.nPos - 16));

    case SB_LINERIGHT:
        return std::min(INT(si.nMax - si.nPage), INT(si.nPos + 16));

    case SB_PAGELEFT:
        return std::max(0, INT(si.nPos - si.nPage));

    case SB_PAGERIGHT:
        return std::min(INT(si.nMax - si.nPage), INT(si.nPos + si.nPage));

    case SB_THUMBPOSITION:
        pos = std::max(0, INT(pos));
        pos = std::min(INT(si.nMax - si.nPage), INT(pos));
        return pos;

    case SB_THUMBTRACK:
        pos = std::max(0, INT(si.nTrackPos));
        pos = std::min(INT(si.nMax - si.nPage), INT(pos));
        return pos;
    }

    return -1;
}

void MScrollView::Scroll(INT bar, INT nSB_, INT pos)
{
    INT nNextPos = GetNextPos(bar, nSB_, pos);
    if (nNextPos == -1)
        return;

    INT nOldPos = ::GetScrollPos(m_hwndParent, bar);
    if (bar == SB_HORZ)
    {
        INT dx = nNextPos - nOldPos;
        ::SetScrollPos(m_hwndParent, bar, nNextPos, TRUE);
        ::ScrollWindow(m_hwndParent, -dx, 0, NULL, NULL);
    }
    else if (bar == SB_VERT)
    {
        INT dy = nNextPos - nOldPos;
        ::SetScrollPos(m_hwndParent, bar, nNextPos, TRUE);
        ::ScrollWindow(m_hwndParent, 0, -dy, NULL, NULL);
    }
}

////////////////////////////////////////////////////////////////////////////

#ifdef SCROLLVIEW_UNITTEST
    HINSTANCE g_hInstance = NULL;
    HWND g_hMainWnd = NULL;
    MScrollView g_sv;
    std::vector<HWND> g_ahwndCtrls;
    const int c_nCtrls = 100;
    const int c_height = 25;
    bool g_bInited = false;

    static const LPCTSTR s_pszName = TEXT("MZC3 ScrollView Test");

    void OnSize(HWND hWnd)
    {
        if (!g_bInited)
            return;

        g_sv.clear();

        MRect rcClient;
        ::GetClientRect(hWnd, &rcClient);

        for (int i = 0; i < c_nCtrls; ++i)
        {
            MRect rcCtrl(MPoint(0, c_height * i),
                         MSize(rcClient.Width(), c_height));
            g_sv.AddCtrlInfo(g_ahwndCtrls[i], rcCtrl);
        }

        g_sv.SetExtentForAllCtrls();
        g_sv.UpdateAll();
    }

    BOOL OnCreate(HWND hWnd)
    {
        g_sv.SetParent(hWnd);
        g_sv.ShowScrollBars(FALSE, TRUE);

        HWND hwndCtrl;

        TCHAR sz[32];
        for (int i = 0; i < c_nCtrls; ++i)
        {
            ::wsprintf(sz, TEXT("%d"), i);
            #if 1
                // text box
                hwndCtrl = ::CreateWindowEx(
                    WS_EX_CLIENTEDGE,
                    TEXT("EDIT"), sz,
                    WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                    0, 0, 0, 0, hWnd, NULL, g_hInstance, NULL);
            #else
                // button
                hwndCtrl = ::CreateWindowEx(
                    0,
                    TEXT("BUTTON"), sz,
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_TEXT | BS_CENTER | BS_VCENTER,
                    0, 0, 0, 0, hWnd, NULL, g_hInstance, NULL);
            #endif
            if (hwndCtrl == NULL)
                return FALSE;

            ::SendMessage(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(::GetStockObject(DEFAULT_GUI_FONT)),
                FALSE);
            g_ahwndCtrls.emplace_back(hwndCtrl);
        }

        g_bInited = true;
        OnSize(hWnd);

        return TRUE;
    }

    void OnDestroy(HWND hWnd)
    {
        for (int i = 0; i < c_nCtrls; ++i)
        {
            ::DestroyWindow(g_ahwndCtrls[i]);
        }
        ::PostQuitMessage(0);
    }

    LRESULT CALLBACK
    WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        case WM_CREATE:
            return OnCreate(hWnd) ? 0 : -1;

        case WM_DESTROY:
            OnDestroy(hWnd);
            break;

        case WM_SIZE:
            OnSize(hWnd);
            break;

        case WM_HSCROLL:
            g_sv.HScroll(LOWORD(wParam), HIWORD(wParam));
            break;

        case WM_VSCROLL:
            g_sv.VScroll(LOWORD(wParam), HIWORD(wParam));
            break;

        default:
            return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    extern "C"
    int WINAPI WinMain(
        HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        int         nCmdShow)
    {
        g_hInstance = hInstance;

        WNDCLASS wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.style = 0;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = hInstance;
        wc.hIcon = ::LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(
            static_cast<LONG_PTR>(COLOR_3DFACE + 1));
        wc.lpszClassName = s_pszName;
        if (!::RegisterClass(&wc))
        {
            return 1;
        }

        g_hMainWnd = ::CreateWindow(s_pszName, s_pszName,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hInstance, NULL);
        if (g_hMainWnd == NULL)
        {
            return 2;
        }

        ::ShowWindow(g_hMainWnd, nCmdShow);
        ::UpdateWindow(g_hMainWnd);

        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        return static_cast<int>(msg.wParam);
    }
#endif  // def SCROLLVIEW_UNITTEST

////////////////////////////////////////////////////////////////////////////
