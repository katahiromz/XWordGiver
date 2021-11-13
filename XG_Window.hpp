#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
    #include <windowsx.h>
#endif

class XG_Window
{
public:
    HWND m_hWnd;
    operator HWND() const
    {
        return m_hWnd;
    }

    XG_Window() : m_hWnd(NULL)
    {
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        XG_Window *pWindow;
        if (uMsg == WM_NCCREATE)
        {
            if (LPCREATESTRUCT pCS = reinterpret_cast<LPCREATESTRUCT>(lParam))
            {
                SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
                pWindow = reinterpret_cast<XG_Window *>(pCS->lpCreateParams);
                pWindow->m_hWnd = hwnd;
            }
        }
        else
        {
            pWindow = reinterpret_cast<XG_Window *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (!pWindow)
                return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
            if (uMsg == WM_NCDESTROY)
            {
                pWindow->m_hWnd = NULL;
            }
        }
        return pWindow->WindowProcDx(hwnd, uMsg, wParam, lParam);
    }

    BOOL CreateWindowDx(HWND hwnd, LPCWSTR cls, LPCWSTR text,
                        DWORD style, DWORD exstyle = 0,
                        INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT,
                        INT cx = CW_USEDEFAULT, INT cy = CW_USEDEFAULT,
                        HMENU hMenu = NULL)
    {
        ::CreateWindowEx(exstyle, cls, text, style, x, y, cx, cy, hwnd, hMenu,
                         ::GetModuleHandle(NULL), this);
        return m_hWnd != NULL;
    }
};
