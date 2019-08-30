////////////////////////////////////////////////////////////////////////////
// ScrollView.cpp -- MZC3 scroll view
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#include "XWordGiver.hpp"

#ifdef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE  /*empty*/
    #include "ScrollView_inl.hpp"
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

/*virtual*/ void MScrollView::GetClientRect(LPRECT prcClient) const
{
    assert(prcClient);
    ::GetClientRect(m_hwndParent, prcClient);
}

// ensure visible
void MScrollView::EnsureCtrlVisible(HWND hwndCtrl, bool update_all/* = true*/)
{
    MRect rcClient;
    MScrollView::GetClientRect(&rcClient);

    const int siz = static_cast<int>(size());
    for (int i = 0; i < siz; ++i)
    {
        if (m_vecInfo[i].m_hwndCtrl != hwndCtrl)
            continue;

        if (!::IsWindow(hwndCtrl))
            break;

        MRect& rcCtrl = m_vecInfo[i].m_rcCtrl;
        if (rcCtrl.bottom > m_ptScrollPos.y + rcClient.Height())
        {
            m_ptScrollPos.y = rcCtrl.bottom - rcClient.Height();
        }
        if (rcCtrl.top < m_ptScrollPos.y)
        {
            m_ptScrollPos.y = rcCtrl.top;
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
        if (Extent().cx < rcCtrl.right - 1)
            Extent().cx = rcCtrl.right - 1;
        if (Extent().cy < rcCtrl.bottom - 1)
            Extent().cy = rcCtrl.bottom - 1;
    }
}

// TODO: Avoid flickering!
void MScrollView::UpdateScrollInfo()
{
    MRect rcClient;
    MScrollView::GetClientRect(&rcClient);

    SCROLLINFO si;

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = Extent().cx;
    si.nPage = rcClient.Width();
    if (static_cast<UINT>(si.nMax) < si.nPage)
        ScrollPos().x = 0;
    si.nPos = ScrollPos().x;
    SetHScrollInfo(&si, TRUE);

    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
    si.nMin = 0;
    si.nMax = Extent().cy;
    si.nPage = rcClient.Height();
    if (static_cast<UINT>(si.nMax) < si.nPage)
        ScrollPos().y = 0;
    si.nPos = ScrollPos().y;
    SetVScrollInfo(&si, TRUE);

    ::InvalidateRect(m_hwndParent, NULL, TRUE);
}

void MScrollView::UpdateCtrlsPos()
{
    const int siz = static_cast<int>(size());
    HDWP hDWP = ::BeginDeferWindowPos(siz);
    if (hDWP)
    {
        for (int i = 0; i < siz; ++i)
        {
            MRect& rcCtrl = m_vecInfo[i].m_rcCtrl;
            ::DeferWindowPos(hDWP, m_vecInfo[i].m_hwndCtrl,
                NULL,
                rcCtrl.left - ScrollPos().x, rcCtrl.top - ScrollPos().y,
                rcCtrl.Width(), rcCtrl.Height(),
                SWP_NOACTIVATE | SWP_NOZORDER);
        }
        ::EndDeferWindowPos(hDWP);
    }
}

void MScrollView::HScroll(int nSB_, int nPos)
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetHScrollInfo(&si);

    switch (nSB_)
    {
    case SB_TOP:
        SetHScrollPos(0);
        break;

    case SB_BOTTOM:
        SetHScrollPos(si.nMax - si.nPage);
        break;

    case SB_LINELEFT:
        SetHScrollPos(si.nPos - 16);
        break;

    case SB_LINERIGHT:
        SetHScrollPos(si.nPos + 16);
        break;

    case SB_PAGELEFT:
        SetHScrollPos(si.nPos - si.nPage);
        break;

    case SB_PAGERIGHT:
        SetHScrollPos(si.nPos + si.nPage);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        SetHScrollPos(nPos);
        break;
    }

    ScrollPos().x = GetHScrollPos();
    UpdateAll();
}

void MScrollView::VScroll(int nSB_, int nPos)
{
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    GetVScrollInfo(&si);

    switch (nSB_)
    {
    case SB_TOP:
        SetVScrollPos(0);
        break;

    case SB_BOTTOM:
        SetVScrollPos(si.nMax - si.nPage);
        break;

    case SB_LINELEFT:
        SetVScrollPos(si.nPos - 16);
        break;

    case SB_LINERIGHT:
        SetVScrollPos(si.nPos + 16);
        break;

    case SB_PAGELEFT:
        SetVScrollPos(si.nPos - si.nPage);
        break;

    case SB_PAGERIGHT:
        SetVScrollPos(si.nPos + si.nPage);
        break;

    case SB_THUMBPOSITION:
    case SB_THUMBTRACK:
        SetVScrollPos(nPos);
        break;
    }

    ScrollPos().y = GetVScrollPos();
    UpdateAll();
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
