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

    XG_Window() : m_hWnd(nullptr), m_fnOldWndProc(nullptr)
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
        return m_fnOldWndProc != nullptr;
    }

    void UnsubclassDx(HWND hwnd)
    {
        if (m_fnOldWndProc)
        {
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_fnOldWndProc));
            m_fnOldWndProc = nullptr;
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
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL CreateWindowDx(HWND hwnd, LPCTSTR text, DWORD style, DWORD exstyle = 0,
                        INT x = CW_USEDEFAULT, INT y = CW_USEDEFAULT,
                        INT cx = CW_USEDEFAULT, INT cy = CW_USEDEFAULT,
                        HMENU hMenu = nullptr)
    {
        auto cls = GetWndClassName();
        ::CreateWindowEx(exstyle, cls, text, style, x, y, cx, cy, hwnd, hMenu,
                         ::GetModuleHandle(nullptr), this);
        return m_hWnd != nullptr;
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
    }

    BOOL RegisterClassDx(HINSTANCE hInstance = ::GetModuleHandle(nullptr));
};

class XG_Dialog
{
public:
    HWND m_hWnd;
    inline static XG_Dialog *s_pTrapping = nullptr;

    operator HWND() const
    {
        return m_hWnd;
    }

    XG_Dialog() : m_hWnd(nullptr)
    {
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return 0;
    }

    static INT_PTR CALLBACK
    DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    INT_PTR DialogBoxDx(HWND hwnd, LPCTSTR pszName)
    {
        s_pTrapping = this;
        auto ret = ::DialogBox(::GetModuleHandle(nullptr), pszName, hwnd, DialogProc);
        s_pTrapping = nullptr;
        return ret;
    }

    INT_PTR DialogBoxDx(HWND hwnd, INT nID)
    {
        return DialogBoxDx(hwnd, MAKEINTRESOURCE(nID));
    }

    BOOL CreateDialogDx(HWND hwnd, INT nID)
    {
        s_pTrapping = this;
        ::CreateDialog(::GetModuleHandle(nullptr), MAKEINTRESOURCE(nID), hwnd, DialogProc);
        s_pTrapping = nullptr;
        return m_hWnd != nullptr;
    }
};
