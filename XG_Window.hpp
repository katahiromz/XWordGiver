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
    WNDPROC m_fnOldWndProc;

    operator HWND() const
    {
        return m_hWnd;
    }

    XG_Window() : m_hWnd(NULL), m_fnOldWndProc(NULL)
    {
    }

    virtual LPCTSTR GetWndClassName() const
    {
        return TEXT("XG_Window");
    }

    BOOL SubclassDx(HWND hwnd)
    {
        m_fnOldWndProc =
            reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd,
                GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        return m_fnOldWndProc != NULL;
    }

    void UnsubclassDx(HWND hwnd)
    {
        if (m_fnOldWndProc)
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_fnOldWndProc));
            m_fnOldWndProc = NULL;
        }
    }

    virtual LRESULT CALLBACK
    DefProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (m_fnOldWndProc)
            return ::CallWindowProc(m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
        else
            return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return DefProcDx(hwnd, uMsg, wParam, lParam);
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

    BOOL RegisterClassDx(HINSTANCE hInstance = ::GetModuleHandle(NULL))
    {
        WNDCLASSEX wcx = { sizeof(wcx) };
        wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        wcx.lpfnWndProc = WindowProc;
        wcx.cbWndExtra = DLGWINDOWEXTRA;
        wcx.hInstance = hInstance;
        wcx.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
        wcx.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
        wcx.hbrBackground = reinterpret_cast<HBRUSH>(INT_PTR(COLOR_3DFACE + 1));
        wcx.lpszClassName = GetWndClassName();
        ModifyWndClassDx(wcx);
        return ::RegisterClassEx(&wcx);
    }
};
