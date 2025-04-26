
#include <windows.h>
#include "TaskbarProgress.h"
//#pragma comment(lib, "ole32.lib")  // CoCreateInstance

TaskbarProgress::TaskbarProgress(HWND hwnd)
    : m_hWnd(hwnd)
    , m_hrCoInit(CoInitialize(NULL))
{
    m_hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,
                            IID_PPV_ARGS(&m_pTaskbarList));
    if (FAILED(m_hr))
        return;

    m_hr = m_pTaskbarList->HrInit();
}

TaskbarProgress::~TaskbarProgress()
{
    if (m_pTaskbarList)
    {
        m_pTaskbarList->Release();
        m_pTaskbarList = NULL;
    }

    if (SUCCEEDED(m_hrCoInit))
        CoUninitialize();
}

void TaskbarProgress::Set(INT percent)
{
    if (!m_pTaskbarList || FAILED(m_hr))
        return;

    m_pTaskbarList->SetProgressState(m_hWnd, ((percent < 0) ? TBPF_INDETERMINATE : TBPF_NORMAL));

    if (percent >= 0)
        m_pTaskbarList->SetProgressValue(m_hWnd, percent, 100);
}

void TaskbarProgress::Error()
{
    if (!m_pTaskbarList || FAILED(m_hr))
        return;

    m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
    m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
}

void TaskbarProgress::Clear()
{
    if (!m_pTaskbarList || FAILED(m_hr))
        return;

    m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
    m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
}

INT TaskbarProgress::GetMenuHeight()
{
    HMENU hMenu = ::GetMenu(m_hWnd);
    INT cItems = ::GetMenuItemCount(hMenu);
    RECT rc;
    ::SetRectEmpty(&rc);
    for (INT iItem = 0; iItem < cItems; ++iItem)
    {
        RECT rcItem;
        GetMenuItemRect(m_hWnd, hMenu, iItem, &rcItem);
        ::UnionRect(&rc, &rc, &rcItem);
    }
    return rc.bottom - rc.top;
}

void TaskbarProgress::GetThumbnailSize(PRECT prc)
{
    extern RECT xg_rcCanvas;
    RECT rc = xg_rcCanvas;

    SIZE siz;
    void __fastcall XgGetXWordExtentForDisplay(LPSIZE psiz) noexcept;
    XgGetXWordExtentForDisplay(&siz);

    OffsetRect(&rc, 0, GetMenuHeight());

    if (rc.right - rc.left > siz.cx)
        rc.right = rc.left + siz.cx;
    if (rc.bottom - rc.top > siz.cy)
        rc.bottom = rc.top + siz.cy;

    *prc = rc;
}

void TaskbarProgress::SetThumbnail()
{
    RECT rc;
    GetThumbnailSize(&rc);

    m_pTaskbarList->SetThumbnailClip(m_hWnd, &rc);
}
