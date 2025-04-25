
#pragma once

#include <shobjidl.h>  // ITaskbarList3

class TaskbarProgress
{
protected:
    HWND m_hWnd;
    ITaskbarList3* m_pTaskbarList;
    HRESULT m_hrCoInit;

public:
    HRESULT m_hr;
    TaskbarProgress(HWND hwnd);
    virtual ~TaskbarProgress();

    void Set(INT percent = -1);
    void Finish() { Set(100); }
    void Clear();
    void Error();
};
