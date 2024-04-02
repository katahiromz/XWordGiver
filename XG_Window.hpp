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

    operator HWND() const noexcept
    {
        return m_hWnd;
    }

    XG_Window() noexcept : m_hWnd(nullptr), m_fnOldWndProc(nullptr)
    {
    }

    virtual ~XG_Window()
    {
    }

    virtual LPCTSTR GetWndClassName() const
    {
        return TEXT("XG_Window");
    }

    BOOL SubclassDx(HWND hwnd) noexcept
    {
        m_hWnd = hwnd;

        BOOL bNoUserData = !::GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (bNoUserData)
            ::SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)this);

        m_fnOldWndProc =
            reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd,
                GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        return m_fnOldWndProc != nullptr;
    }

    void UnsubclassDx(HWND hwnd) noexcept
    {
        if (m_fnOldWndProc)
        {
            ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_fnOldWndProc));
            m_fnOldWndProc = nullptr;
        }
        m_hWnd = NULL;
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
                        int x = CW_USEDEFAULT, int y = CW_USEDEFAULT,
                        int cx = CW_USEDEFAULT, int cy = CW_USEDEFAULT,
                        HMENU hMenu = nullptr)
    {
        auto cls = GetWndClassName();
        ::CreateWindowEx(exstyle, cls, text, style, x, y, cx, cy, hwnd, hMenu,
                         ::GetModuleHandle(nullptr), this);
        return m_hWnd != nullptr;
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        UNREFERENCED_PARAMETER(wcx);
    }

    BOOL RegisterClassDx(HINSTANCE hInstance = ::GetModuleHandle(nullptr));
};

class XG_Dialog
{
public:
    HWND m_hWnd;
    inline static XG_Dialog *s_pTrapping = nullptr;

    operator HWND() const noexcept
    {
        return m_hWnd;
    }

    XG_Dialog() noexcept : m_hWnd(nullptr)
    {
    }

    virtual ~XG_Dialog()
    {
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        UNREFERENCED_PARAMETER(hwnd);
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        return 0;
    }

    static INT_PTR CALLBACK
    DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    INT_PTR DialogBoxDx(HWND hwnd, LPCTSTR pszName) noexcept
    {
        s_pTrapping = this;
        const auto ret = ::DialogBox(::GetModuleHandle(nullptr), pszName, hwnd, DialogProc);
        s_pTrapping = nullptr;
        return ret;
    }

    INT_PTR DialogBoxDx(HWND hwnd, int nID) noexcept
    {
        return DialogBoxDx(hwnd, MAKEINTRESOURCE(nID));
    }

    BOOL CreateDialogDx(HWND hwnd, int nID) noexcept
    {
        s_pTrapping = this;
        ::CreateDialog(::GetModuleHandle(nullptr), MAKEINTRESOURCE(nID), hwnd, DialogProc);
        s_pTrapping = nullptr;
        return m_hWnd != nullptr;
    }
};
