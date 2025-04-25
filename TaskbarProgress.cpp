
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

void TaskbarProgress::Clear()
{
    if (!m_pTaskbarList || FAILED(m_hr))
        return;

    m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
}
