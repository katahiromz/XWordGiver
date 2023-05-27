#include "XG_Window.hpp"
#include <cassert>

/*static*/ LRESULT CALLBACK
XG_Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    XG_Window *pWindow;
    if (uMsg == WM_NCCREATE)
    {
        if (LPCREATESTRUCT pCS = reinterpret_cast<LPCREATESTRUCT>(lParam))
        {
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCS->lpCreateParams));
            pWindow = static_cast<XG_Window*>(pCS->lpCreateParams);
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
            pWindow->m_hWnd = nullptr;
        }
    }
    return pWindow->WindowProcDx(hwnd, uMsg, wParam, lParam);
}

BOOL XG_Window::RegisterClassDx(HINSTANCE hInstance/* = ::GetModuleHandle(nullptr)*/)
{
    WNDCLASSEX wcx = { sizeof(wcx) };
    wcx.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcx.lpfnWndProc = WindowProc;
    wcx.cbWndExtra = DLGWINDOWEXTRA;
    wcx.hInstance = hInstance;
    wcx.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
    wcx.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_3DFACE + 1));
    wcx.lpszClassName = GetWndClassName();
    ModifyWndClassDx(wcx);
    return ::RegisterClassEx(&wcx);
}

/*static*/ INT_PTR CALLBACK
XG_Dialog::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    XG_Dialog *pDialog;
    if (uMsg == WM_INITDIALOG || uMsg == WM_CREATE)
    {
        assert(s_pTrapping != nullptr);
        pDialog = s_pTrapping;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LPARAM>(pDialog));
        pDialog->m_hWnd = hwnd;
    }
    else
    {
        pDialog = reinterpret_cast<XG_Dialog *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (!pDialog)
        {
            if (uMsg == WM_MEASUREITEM || uMsg == WM_SIZE)
            {
                assert(s_pTrapping != nullptr);
                pDialog = s_pTrapping;
                pDialog->m_hWnd = hwnd;
                return pDialog->DialogProcDx(hwnd, uMsg, wParam, lParam);
            }
            return 0;
        }
        if (uMsg == WM_NCDESTROY)
        {
            pDialog->m_hWnd = nullptr;
        }
    }
    return pDialog->DialogProcDx(hwnd, uMsg, wParam, lParam);
}
