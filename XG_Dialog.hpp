#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>
#endif

class XG_Dialog
{
public:
    HWND m_hWnd;
    inline static XG_Dialog *s_pTrapping = NULL;

    operator HWND() const
    {
        return m_hWnd;
    }

    XG_Dialog() : m_hWnd(NULL)
    {
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        return 0;
    }

    static INT_PTR CALLBACK
    DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        XG_Dialog *pDialog;
        if (uMsg == WM_INITDIALOG)
        {
            assert(s_pTrapping != NULL);
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
                    assert(s_pTrapping != NULL);
                    pDialog = s_pTrapping;
                    pDialog->m_hWnd = hwnd;
                    return pDialog->DialogProcDx(hwnd, uMsg, wParam, lParam);
                }
                return 0;
            }
            if (uMsg == WM_NCDESTROY)
            {
                pDialog->m_hWnd = NULL;
            }
        }
        return pDialog->DialogProcDx(hwnd, uMsg, wParam, lParam);
    }

    INT_PTR DialogBoxDx(HWND hwnd, LPCTSTR pszName)
    {
        s_pTrapping = this;
        auto ret = ::DialogBox(::GetModuleHandle(NULL), pszName, hwnd, DialogProc);
        s_pTrapping = NULL;
        return ret;
    }

    INT_PTR DialogBoxDx(HWND hwnd, INT nID)
    {
        return DialogBoxDx(hwnd, MAKEINTRESOURCE(nID));
    }
};
