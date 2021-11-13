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
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            pDialog = reinterpret_cast<XG_Dialog *>(lParam);
            pDialog->m_hWnd = hwnd;
        }
        else
        {
            pDialog = reinterpret_cast<XG_Dialog *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (!pDialog)
                return 0;
            if (uMsg == WM_NCDESTROY)
            {
                pDialog->m_hWnd = NULL;
            }
        }
        return pDialog->DialogProcDx(hwnd, uMsg, wParam, lParam);
    }

    INT_PTR DialogBoxDx(HWND hwnd, LPCTSTR pszName)
    {
        return ::DialogBoxParam(::GetModuleHandle(NULL), pszName, hwnd, DialogProc,
                                reinterpret_cast<LPARAM>(this));
    }
};
