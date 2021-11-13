#pragma once

#include "XG_Window.hpp"

struct XG_CandsButtonData
{
    WNDPROC m_fnOldWndProc;
};

extern HWND xg_hCandsWnd;

// 候補を求める位置。
extern INT xg_jCandPos, xg_iCandPos;

// 候補はタテかヨコか。
extern bool xg_bCandVertical;

// 候補を適用する。
void XgApplyCandidate(XG_Board& xword, const std::wstring& strCand);

class XG_CandsWnd : public XG_Window
{
public:
    // 候補ウィンドウの位置とサイズ。
    inline static int s_nCandsWndX = CW_USEDEFAULT, s_nCandsWndY = CW_USEDEFAULT;
    inline static int s_nCandsWndCX = CW_USEDEFAULT, s_nCandsWndCY = CW_USEDEFAULT;

    // 候補ウィンドウのスクロールビュー。
    inline static MScrollView                 xg_svCandsScrollView;

    // 候補ウィンドウのUIフォント。
    inline static HFONT                       xg_hCandsUIFont = NULL;

    // 候補。
    inline static std::vector<std::wstring>   xg_vecCandidates;

    // 候補ボタン。
    inline static std::vector<HWND>           xg_ahwndCandButtons;

    static LRESULT CALLBACK
    XgCandsButton_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WNDPROC fn;
        XG_CandsButtonData *data =
            reinterpret_cast<XG_CandsButtonData *>(
                ::GetWindowLongPtr(hwnd, GWLP_USERDATA));
        switch (uMsg) {
        case WM_CHAR:
            #if 0
                if (wParam == L'\t' || wParam == L'\r' || wParam == L'\n')
                    break;
            #endif
            return ::CallWindowProc(data->m_fnOldWndProc,
                hwnd, uMsg, wParam, lParam);

        case WM_SETFOCUS:
            xg_svCandsScrollView.EnsureCtrlVisible(hwnd);
            return ::CallWindowProc(data->m_fnOldWndProc,
                hwnd, uMsg, wParam, lParam);

        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                ::SetFocus(NULL);
                break;
            }

            if (wParam == VK_PRIOR || wParam == VK_NEXT) {
                HWND hwndParent = ::GetParent(hwnd);
                ::SendMessageW(hwndParent, uMsg, wParam, lParam);
                break;
            }

            if (wParam == VK_F6) {
                if (::GetAsyncKeyState(VK_SHIFT) < 0)
                    ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
                else
                    ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
                break;
            }

            if (wParam == VK_ESCAPE) {
                DestroyWindow(GetParent(hwnd));
                break;
            }

            return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

        case WM_NCDESTROY:
            fn = data->m_fnOldWndProc;
            ::LocalFree(data);
            return ::CallWindowProc(fn, hwnd, uMsg, wParam, lParam);

        case WM_MOUSEWHEEL:
            {
                HWND hwndParent = ::GetParent(hwnd);
                ::SendMessageW(hwndParent, uMsg, wParam, lParam);
            }
            break;

        default:
            return ::CallWindowProc(data->m_fnOldWndProc,
                hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    XG_CandsWnd()
    {
    }

    virtual LPCTSTR GetWndClassName() const
    {
        return TEXT("XG_CandsWnd");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        wcx.hIcon = NULL;
        wcx.hbrBackground = ::CreateSolidBrush(RGB(255, 255, 192));
    }

    // 候補ウィンドウのサイズが変わった。
    void OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/)
    {
        if (xg_ahwndCandButtons.empty())
            return;

        xg_svCandsScrollView.clear();

        MRect rcClient;
        ::GetClientRect(hwnd, &rcClient);

        HDC hdc = ::CreateCompatibleDC(NULL);
        HGDIOBJ hFontOld = ::SelectObject(hdc, xg_hCandsUIFont);
        {
            MPoint pt;
            for (size_t i = 0; i < xg_vecCandidates.size(); ++i) {
                const std::wstring& strLabel = xg_vecCandidates[i];

                MSize siz;
                ::GetTextExtentPoint32W(hdc, strLabel.data(),
                                        static_cast<int>(strLabel.size()), &siz);

                if (pt.x != 0 && pt.x + siz.cx + 16 > rcClient.Width()) {
                    pt.x = 0;
                    pt.y += siz.cy + 16;
                }

                MRect rcCtrl(MPoint(pt.x + 4, pt.y + 4), MSize(siz.cx + 8, siz.cy + 8));
                xg_svCandsScrollView.AddCtrlInfo(xg_ahwndCandButtons[i], rcCtrl);

                pt.x += siz.cx + 16;
            }
        }
        ::SelectObject(hdc, hFontOld);
        ::DeleteDC(hdc);

        xg_svCandsScrollView.SetExtentForAllCtrls();
        xg_svCandsScrollView.Extent().cy += 4;
        xg_svCandsScrollView.EnsureCtrlVisible(::GetFocus(), false);
        xg_svCandsScrollView.UpdateAll();
    }

    // 候補ウィンドウが作成された。
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
    {
        xg_hCandsWnd = hwnd;

        if (xg_hCandsUIFont) {
            ::DeleteObject(xg_hCandsUIFont);
        }
        xg_hCandsUIFont = ::CreateFontIndirectW(XgGetUIFont());
        if (xg_hCandsUIFont == NULL) {
            xg_hCandsUIFont = reinterpret_cast<HFONT>(
                ::GetStockObject(DEFAULT_GUI_FONT));
        }

        // 初期化。
        xg_ahwndCandButtons.clear();
        xg_svCandsScrollView.clear();

        xg_svCandsScrollView.SetParent(hwnd);
        xg_svCandsScrollView.ShowScrollBars(FALSE, TRUE);

        HWND hwndCtrl;
        XG_CandsButtonData *data;
        for (size_t i = 0; i < xg_vecCandidates.size(); ++i) {
            WCHAR szText[64];
            if (xg_bHiragana) {
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                             xg_vecCandidates[i].data(), -1, szText, ARRAYSIZE(szText));
                xg_vecCandidates[i] = szText;
            }
            if (xg_bLowercase) {
                LCMapStringW(JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE,
                             xg_vecCandidates[i].data(), -1, szText, ARRAYSIZE(szText));
                xg_vecCandidates[i] = szText;
            }

            hwndCtrl = ::CreateWindowW(
                TEXT("BUTTON"), xg_vecCandidates[i].data(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
            assert(hwndCtrl);
            if (hwndCtrl == NULL)
                return FALSE;

            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hCandsUIFont),
                FALSE);
            xg_ahwndCandButtons.emplace_back(hwndCtrl);

            data = reinterpret_cast<XG_CandsButtonData *>(
                ::LocalAlloc(LMEM_FIXED, sizeof(XG_CandsButtonData)));
            data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(XgCandsButton_WndProc)));
            ::SetWindowLongPtr(hwndCtrl, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(data));
        }
        OnSize(hwnd, 0, 0, 0);

        if (xg_ahwndCandButtons.size())
            ::SetFocus(xg_ahwndCandButtons[0]);
        else
            ::SetFocus(hwnd);

        return TRUE;
    }

    // 候補ウィンドウが縦にスクロールされた。
    void OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
    {
        xg_svCandsScrollView.Scroll(SB_VERT, code, pos);
    }

    // キーが押された。
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (!fDown)
            return;

        switch (vk) {
        case VK_PRIOR:
            ::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEUP, 0), 0);
            break;

        case VK_NEXT:
            ::PostMessageW(hwnd, WM_VSCROLL, MAKELPARAM(SB_PAGEDOWN, 0), 0);
            break;

        case VK_TAB:
            if (xg_ahwndCandButtons.size())
                ::SetFocus(xg_ahwndCandButtons[0]);
            break;

        case VK_F6:
            if (::GetAsyncKeyState(VK_SHIFT) < 0)
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
            else
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
            break;

        case VK_ESCAPE:
            DestroyWindow(hwnd);
            break;
        }
    }

    // 候補ウィンドウが破棄された。
    void OnDestroy(HWND hwnd)
    {
        // 現在の位置とサイズを保存する。
        MRect rc;
        ::GetWindowRect(hwnd, &rc);
        s_nCandsWndX = rc.left;
        s_nCandsWndY = rc.top;
        s_nCandsWndCX = rc.Width();
        s_nCandsWndCY = rc.Height();

        xg_hCandsWnd = NULL;
        xg_ahwndCandButtons.clear();
        xg_svCandsScrollView.clear();

        ::DeleteObject(xg_hCandsUIFont);
        xg_hCandsUIFont = NULL;

        SetForegroundWindow(xg_hMainWnd);
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (codeNotify == BN_CLICKED) {
            for (size_t i = 0; i < xg_ahwndCandButtons.size(); ++i) {
                if (xg_ahwndCandButtons[i] == hwndCtl)
                {
                    // 候補を適用する。
                    XgApplyCandidate(xg_xword, xg_vecCandidates[i]);

                    // 候補ウィンドウを破棄する。
                    XgDestroyCandsWnd();

                    // イメージを更新する。
                    int x = XgGetHScrollPos();
                    int y = XgGetVScrollPos();
                    XgUpdateImage(xg_hMainWnd, x, y);
                    return;
                }
            }
        }
    }

    // マウスホイールが回転した。
    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
    {
        if (::GetAsyncKeyState(VK_SHIFT) < 0) {
            if (zDelta < 0)
                ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            else if (zDelta > 0)
                ::PostMessageW(hwnd, WM_HSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
        } else {
            if (zDelta < 0)
                ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEDOWN, 0), 0);
            else if (zDelta > 0)
                ::PostMessageW(hwnd, WM_VSCROLL, MAKEWPARAM(SB_LINEUP, 0), 0);
        }
    }

    // 候補ウィンドウのサイズを制限する。
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
    {
        lpMinMaxInfo->ptMinTrackSize.x = 256;
        lpMinMaxInfo->ptMinTrackSize.y = 128;
    }

    // 候補ウィンドウを描画する。
    void OnPaint(HWND hwnd)
    {
        if (xg_vecCandidates.empty()) {
            PAINTSTRUCT ps;
            HDC hdc = ::BeginPaint(hwnd, &ps);
            if (hdc) {
                MRect rcClient;
                ::GetClientRect(hwnd, &rcClient);
                ::SetTextColor(hdc, RGB(0, 0, 0));
                ::SetBkMode(hdc, TRANSPARENT);
                ::DrawTextW(hdc, XgLoadStringDx1(IDS_NOCANDIDATES), -1,
                    &rcClient,
                    DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX);
                ::EndPaint(hwnd, &ps);
            }
        } else {
            FORWARD_WM_PAINT(hwnd, DefWindowProcW);
        }
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg) {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_KEYUP, OnKey);
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        default: return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
};
