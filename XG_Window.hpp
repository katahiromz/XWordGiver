#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifndef _INC_WINDOWSX
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

    virtual LPCTSTR GetWndClassName() const
    {
        return TEXT("XG_Window");
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
            else
            {
                return 0;
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

    BOOL CreateWindowDx(HWND hwnd, LPCTSTR text, DWORD style, DWORD exstyle = 0,
                        INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT,
                        INT cx = CW_USEDEFAULT, INT cy = CW_USEDEFAULT,
                        HMENU hMenu = NULL)
    {
        auto cls = GetWndClassName();
        ::CreateWindowEx(exstyle, cls, text, style, x, y, cx, cy, hwnd, hMenu,
                         ::GetModuleHandle(NULL), this);
        return m_hWnd != NULL;
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    BOOL RegisterClassDx()
    {
        WNDCLASSEX wcx;
        ZeroMemory(&wcx, sizeof(wcx));
        wcx.cbSize = sizeof(wcx);
        wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcx.lpfnWndProc = WindowProc;
        wcx.cbClsExtra = 0;
        wcx.cbWndExtra = DLGWINDOWEXTRA;
        wcx.hInstance = ::GetModuleHandle(NULL);
        wcx.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
        wcx.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        wcx.hbrBackground = reinterpret_cast<HBRUSH>(INT_PTR(COLOR_3DFACE + 1));
        wcx.lpszClassName = GetWndClassName();
        wcx.hIconSm = nullptr;
        ModifyWndClassDx(wcx);
        return ::RegisterClassEx(&wcx);
    }
};
