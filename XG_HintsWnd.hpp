#pragma once

#include "XG_Window.hpp"
#include "XG_UndoBuffer.hpp"

#define XGWM_HIGHLIGHT (WM_USER + 100)

class XG_HintsWnd : public XG_Window
{
public:
    // ヒントウィンドウのスクロールビュー。
    inline static MScrollView         xg_svHintsScrollView;

    // ヒントウィンドウのUIフォント。
    inline static HFONT               xg_hHintsUIFont = NULL;

    // 縦のカギのコントロール群。
    inline static HWND                xg_hwndTateCaptionStatic = NULL;
    inline static std::vector<HWND>   xg_ahwndTateStatics;
    inline static std::vector<HWND>   xg_ahwndTateEdits;

    // 横のカギのコントロール群。
    inline static HWND                xg_hwndYokoCaptionStatic = NULL;
    inline static std::vector<HWND>   xg_ahwndYokoStatics;
    inline static std::vector<HWND>   xg_ahwndYokoEdits;

    // ヒントウィンドウの位置とサイズ。
    inline static int s_nHintsWndX = CW_USEDEFAULT, s_nHintsWndY = CW_USEDEFAULT;
    inline static int s_nHintsWndCX = CW_USEDEFAULT, s_nHintsWndCY = CW_USEDEFAULT;

    // ハイライト。
    inline static HWND s_hwndHighlightTateEdit = NULL;
    inline static HWND s_hwndHighlightYokoEdit = NULL;

    // ハイライトを更新する。
    void setHighlight(INT nYoko, INT nTate)
    {
        HWND hwndTateOld = s_hwndHighlightTateEdit;
        HWND hwndYokoOld = s_hwndHighlightYokoEdit;
        HWND hwndTate = NULL, hwndYoko = NULL;

        if (nTate != -1) {
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                if (xg_vecTateHints[i].m_number == nTate) {
                    hwndTate = xg_ahwndTateEdits[i];
                    break;
                }
            }
        }
        if (nYoko != -1) {
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                if (xg_vecYokoHints[i].m_number == nYoko) {
                    hwndYoko = xg_ahwndYokoEdits[i];
                    break;
                }
            }
        }

        s_hwndHighlightTateEdit = hwndTate;
        s_hwndHighlightYokoEdit = hwndYoko;

        if (hwndTate)
            InvalidateRect(hwndTate, NULL, TRUE);
        if (hwndYoko)
            InvalidateRect(hwndYoko, NULL, TRUE);
        if (hwndTateOld)
            InvalidateRect(hwndTateOld, NULL, TRUE);
        if (hwndYokoOld)
            InvalidateRect(hwndYokoOld, NULL, TRUE);
    }

    // ハイライトに背景色を付ける。
    HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
    {
        if (hwndChild == s_hwndHighlightTateEdit || hwndChild == s_hwndHighlightYokoEdit)
        {
            const COLORREF rgbYellow = RGB(255, 255, 120);
            SetBkColor(hdc, rgbYellow);
            SetDCBrushColor(hdc, rgbYellow);
            return GetStockBrush(DC_BRUSH);
        }
        else
        {
            return FORWARD_WM_CTLCOLOREDIT(hwnd, hdc, hwndChild, ::DefWindowProcW);
        }
    }

    // ヒントが変更されたか？
    static bool AreHintsModified(void)
    {
        if (xg_bHintsAdded) {
            return true;
        }

        if (::IsWindow(xg_hHintsWnd)) {
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndTateEdits[i], EM_GETMODIFY, 0, 0)) {
                    return true;
                }
            }
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndYokoEdits[i], EM_GETMODIFY, 0, 0)) {
                    return true;
                }
            }
        }
        return false;
    }

    // ヒントデータを設定する。
    static void SetHintsData(void)
    {
        for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
            ::SetWindowTextW(xg_ahwndTateEdits[i], xg_vecTateHints[i].m_strHint.data());
        }
        for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
            ::SetWindowTextW(xg_ahwndYokoEdits[i], xg_vecYokoHints[i].m_strHint.data());
        }
    }

    // ヒントデータを更新する。
    static bool UpdateHintData(void)
    {
        bool updated = false;
        if (::IsWindow(xg_hHintsWnd)) {
            WCHAR sz[512];
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndTateEdits[i], EM_GETMODIFY, 0, 0)) {
                    updated = true;
                    ::GetWindowTextW(xg_ahwndTateEdits[i], sz, 
                                     static_cast<int>(ARRAYSIZE(sz)));
                    xg_vecTateHints[i].m_strHint = sz;
                    ::SendMessageW(xg_ahwndTateEdits[i], EM_SETMODIFY, FALSE, 0);
                }
            }
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndYokoEdits[i], EM_GETMODIFY, 0, 0)) {
                    updated = true;
                    ::GetWindowTextW(xg_ahwndYokoEdits[i], sz, 
                                     static_cast<int>(ARRAYSIZE(sz)));
                    xg_vecYokoHints[i].m_strHint = sz;
                    ::SendMessageW(xg_ahwndTateEdits[i], EM_SETMODIFY, FALSE, 0);
                }
            }
        }
        if (updated)
            XG_FILE_MODIFIED(TRUE);
        return updated;
    }

    XG_HintsWnd()
    {
    }

    virtual LPCTSTR GetWndClassName() const
    {
        return TEXT("XG_HintsWnd");
    }

    virtual void ModifyWndClassDx(WNDCLASSEX& wcx)
    {
        // No change
    }

    // ヒントウィンドウのサイズが変わった。
    void OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/)
    {
        if (xg_hwndTateCaptionStatic == NULL)
            return;

        xg_svHintsScrollView.clear();

        MRect rcClient;
        ::GetClientRect(hwnd, &rcClient);

        MSize size1, size2;
        {
            HDC hdc = ::CreateCompatibleDC(NULL);
            WCHAR label[64];
            StringCbPrintf(label, sizeof(label), XgLoadStringDx1(IDS_DOWNNUMBER), 100);
            std::wstring strLabel = label;
            ::SelectObject(hdc, ::GetStockObject(SYSTEM_FIXED_FONT));
            ::GetTextExtentPoint32W(hdc, strLabel.data(), int(strLabel.size()), &size1);
            ::SelectObject(hdc, xg_hHintsUIFont);
            ::GetTextExtentPoint32W(hdc, strLabel.data(), int(strLabel.size()), &size2);
            ::DeleteDC(hdc);
        }

        WCHAR szText[512];
        HDC hdc = ::CreateCompatibleDC(NULL);
        HGDIOBJ hFontOld = ::SelectObject(hdc, xg_hHintsUIFont);
        int y = 0;

        // タテのカギ。
        {
            MRect rcCtrl(MPoint(0, y + 4), 
                         MSize(rcClient.Width(), size1.cy + 4));
            xg_svHintsScrollView.AddCtrlInfo(xg_hwndTateCaptionStatic, rcCtrl);
            y += size1.cy + 8;
        }
        if (rcClient.Width() - size2.cx - 8 > 0) {
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                ::GetWindowTextW(xg_ahwndTateEdits[i], szText,
                                 ARRAYSIZE(szText));
                MRect rcCtrl(MPoint(size2.cx, y),
                             MSize(rcClient.Width() - size2.cx - 8, 0));
                if (szText[0] == 0) {
                    szText[0] = L' ';
                    szText[1] = 0;
                }
                ::DrawTextW(hdc, szText, -1, &rcCtrl,
                    DT_LEFT | DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK);
                rcCtrl.right = rcClient.right;
                rcCtrl.bottom += 8;
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndTateEdits[i], rcCtrl);
                rcCtrl.left = 0;
                rcCtrl.right = size2.cx;
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndTateStatics[i], rcCtrl);
                y += rcCtrl.Height();
            }
        }
        // ヨコのカギ。
        {
            MRect rcCtrl(MPoint(0, y + 4),
                         MSize(rcClient.Width(), size1.cy + 4));
            xg_svHintsScrollView.AddCtrlInfo(xg_hwndYokoCaptionStatic, rcCtrl);
            y += size1.cy + 8;
        }
        if (rcClient.Width() - size2.cx - 8 > 0) {
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                ::GetWindowTextW(xg_ahwndYokoEdits[i], szText, ARRAYSIZE(szText));
                MRect rcCtrl(MPoint(size2.cx, y),
                             MSize(rcClient.Width() - size2.cx - 8, 0));
                if (szText[0] == 0) {
                    szText[0] = L' ';
                    szText[1] = 0;
                }
                ::DrawTextW(hdc, szText, -1, &rcCtrl,
                    DT_LEFT | DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK);
                rcCtrl.right = rcClient.right;
                rcCtrl.bottom += 8;
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndYokoEdits[i], rcCtrl);
                rcCtrl.left = 0;
                rcCtrl.right = size2.cx;
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndYokoStatics[i], rcCtrl);
                y += rcCtrl.Height();
            }
        }

        ::SelectObject(hdc, hFontOld);
        ::DeleteDC(hdc);

        xg_svHintsScrollView.SetExtentForAllCtrls();
        xg_svHintsScrollView.UpdateAll();
    }

    struct XG_HintEditData
    {
        WNDPROC m_fnOldWndProc;
        bool    m_fTate;
    };

    static LRESULT CALLBACK XgHintEdit_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WNDPROC fn;
        XG_HintEditData *data =
            reinterpret_cast<XG_HintEditData *>(
                ::GetWindowLongPtr(hwnd, GWLP_USERDATA));

        switch (uMsg) {
        case WM_CHAR:
            if (wParam == L'\r' || wParam == L'\n') {
                // 改行が押された。必要ならばデータを更新する。
                if (AreHintsModified()) {
                    auto hu1 = std::make_shared<XG_UndoData_HintsUpdated>();
                    auto hu2 = std::make_shared<XG_UndoData_HintsUpdated>();
                    hu1->Get();
                    {
                        // ヒントを更新する。
                        UpdateHintData();
                    }
                    hu2->Get();
                    xg_ubUndoBuffer.Commit(UC_HINTS_UPDATED, hu1, hu2);
                }
            }
            return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

        case WM_SETFOCUS: // フォーカスを得た。
            if (wParam) {
                // フォーカスを失うコントロールの選択を解除する。
                HWND hwndLoseFocus = reinterpret_cast<HWND>(wParam);
                ::SendMessageW(hwndLoseFocus, EM_SETSEL, 0, 0);
            }
            // ハイライト。
            {
                size_t i;
                bool found = false, vertical = false;
                if (!found) {
                    i = 0;
                    for (auto& edit : xg_ahwndTateEdits) {
                        if (edit == hwnd) {
                            found = true;
                            vertical = true;
                            break;
                        }
                        ++i;
                    }
                }
                if (!found) {
                    i = 0;
                    for (auto& edit : xg_ahwndYokoEdits) {
                        if (edit == hwnd) {
                            found = true;
                            vertical = false;
                            break;
                        }
                        ++i;
                    }
                }
                if (found) {
                    INT number;
                    if (vertical)
                        number = xg_vecTateHints[i].m_number;
                    else
                        number = xg_vecYokoHints[i].m_number;
                    PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT, TRUE, MAKELPARAM(number, vertical));
                }
            }

            // フォーカスのあるコントロールが見えるように移動する。
            xg_svHintsScrollView.EnsureCtrlVisible(hwnd);
            return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

        case WM_KILLFOCUS:  // フォーカスを失った。
            // ヒントに更新があれば、データを更新する。
            if (AreHintsModified()) {
                auto hu1 = std::make_shared<XG_UndoData_HintsUpdated>();
                auto hu2 = std::make_shared<XG_UndoData_HintsUpdated>();
                hu1->Get();
                {
                    // ヒントを更新する。
                    UpdateHintData();
                }
                hu2->Get();
                xg_ubUndoBuffer.Commit(UC_HINTS_UPDATED, hu1, hu2);
            }
            // レイアウトを調整する。
            ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
            PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT, TRUE, MAKELPARAM(-1, false));
            return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
                ::SetFocus(NULL);
                break;
            }

            if (wParam == VK_TAB) {
                HWND hwndNext;
                if (::GetAsyncKeyState(VK_SHIFT) < 0)
                    hwndNext = ::GetNextDlgTabItem(xg_hHintsWnd, hwnd, TRUE);
                else
                    hwndNext = ::GetNextDlgTabItem(xg_hHintsWnd, hwnd, FALSE);
                ::SendMessageW(hwnd, EM_SETSEL, 0, 0);
                ::SendMessageW(hwndNext, EM_SETSEL, 0, -1);
                ::SetFocus(hwndNext);
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
            return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    // ヒントウィンドウが作成された。
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
    {
        xg_hHintsWnd = hwnd;

        // 初期化。
        xg_ahwndTateStatics.clear();
        xg_ahwndTateEdits.clear();
        xg_ahwndYokoStatics.clear();
        xg_ahwndYokoEdits.clear();
        xg_svHintsScrollView.SetParent(hwnd);
        xg_svHintsScrollView.ShowScrollBars(FALSE, TRUE);
        s_hwndHighlightTateEdit = s_hwndHighlightYokoEdit = NULL;

        if (xg_hHintsUIFont) {
            ::DeleteObject(xg_hHintsUIFont);
        }
        xg_hHintsUIFont = ::CreateFontIndirectW(XgGetUIFont());
        if (xg_hHintsUIFont == NULL) {
            xg_hHintsUIFont =
                reinterpret_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
        }

        HWND hwndCtrl;

        hwndCtrl = ::CreateWindowW(
            TEXT("STATIC"), XgLoadStringDx1(IDS_DOWN),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY |
            SS_CENTER | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        if (hwndCtrl == NULL)
            return FALSE;
        xg_hwndTateCaptionStatic = hwndCtrl;
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(::GetStockObject(SYSTEM_FIXED_FONT)),
            TRUE);

        hwndCtrl = ::CreateWindowW(
            TEXT("STATIC"), XgLoadStringDx1(IDS_ACROSS),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY |
            SS_CENTER | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
        if (hwndCtrl == NULL)
            return FALSE;
        xg_hwndYokoCaptionStatic = hwndCtrl;
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(::GetStockObject(SYSTEM_FIXED_FONT)),
            TRUE);

        XG_HintEditData *data;
        WCHAR sz[256];
        for (const auto& hint : xg_vecTateHints) {
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_DOWNNUMBER), hint.m_number);
            hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz,
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE,
                0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            if (hwndCtrl == NULL)
                return FALSE;

            xg_ahwndTateStatics.emplace_back(hwndCtrl);

            hwndCtrl = ::CreateWindowExW(
                WS_EX_CLIENTEDGE,
                TEXT("EDIT"), hint.m_strHint.data(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE,
                0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            if (hwndCtrl == NULL)
                return FALSE;

            data = reinterpret_cast<XG_HintEditData *>(
                ::LocalAlloc(LMEM_FIXED, sizeof(XG_HintEditData)));
            data->m_fTate = true;
            data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(XgHintEdit_WndProc)));
            ::SetWindowLongPtr(hwndCtrl, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(data));
            xg_ahwndTateEdits.emplace_back(hwndCtrl);
        }
        for (const auto& hint : xg_vecYokoHints) {
            StringCbPrintf(sz, sizeof(sz), XgLoadStringDx1(IDS_ACROSSNUMBER), hint.m_number);
            hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz,
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE,
                0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            if (hwndCtrl == NULL)
                return FALSE;

            xg_ahwndYokoStatics.emplace_back(hwndCtrl);

            hwndCtrl = ::CreateWindowExW(
                WS_EX_CLIENTEDGE,
                TEXT("EDIT"), hint.m_strHint.data(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE,
                0, 0, 0, 0, hwnd, NULL, xg_hInstance, NULL);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            if (hwndCtrl == NULL)
                return FALSE;

            data = reinterpret_cast<XG_HintEditData *>(
                ::LocalAlloc(LMEM_FIXED, sizeof(XG_HintEditData)));
            data->m_fTate = false;
            data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtr(hwndCtrl, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(XgHintEdit_WndProc)));
            ::SetWindowLongPtr(hwndCtrl, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(data));
            xg_ahwndYokoEdits.emplace_back(hwndCtrl);
        }

        if (xg_ahwndTateEdits.size())
            ::SetFocus(xg_ahwndTateEdits[0]);

        ::PostMessageW(hwnd, WM_SIZE, 0, 0);

        return TRUE;
    }

    // ヒントウィンドウが縦にスクロールされた。
    void OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos)
    {
        xg_svHintsScrollView.Scroll(SB_VERT, code, pos);
    }

    // ヒントウィンドウが破棄された。
    void OnDestroy(HWND hwnd)
    {
        if (xg_hHintsWnd) {
            // ヒントデータを更新する。
            UpdateHintData();
        }

        // 現在の位置とサイズを保存する。
        MRect rc;
        ::GetWindowRect(hwnd, &rc);
        XG_HintsWnd::s_nHintsWndX = rc.left;
        XG_HintsWnd::s_nHintsWndY = rc.top;
        XG_HintsWnd::s_nHintsWndCX = rc.Width();
        XG_HintsWnd::s_nHintsWndCY = rc.Height();

        xg_hHintsWnd = NULL;
        xg_hwndTateCaptionStatic = NULL;
        xg_hwndYokoCaptionStatic = NULL;
        xg_ahwndTateStatics.clear();
        xg_ahwndTateEdits.clear();
        xg_ahwndYokoStatics.clear();
        xg_ahwndYokoEdits.clear();
        xg_svHintsScrollView.clear();
        s_hwndHighlightTateEdit = s_hwndHighlightYokoEdit = NULL;

        ::DeleteObject(xg_hHintsUIFont);
        xg_hHintsUIFont = NULL;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        if (codeNotify == STN_CLICKED) {
            for (size_t i = 0; i < xg_vecTateHints.size(); ++i) {
                if (xg_ahwndTateStatics[i] == hwndCtl) {
                    ::SendMessageW(xg_ahwndTateEdits[i], EM_SETSEL, 0, -1);
                    ::SetFocus(xg_ahwndTateEdits[i]);
                    return;
                }
            }
            for (size_t i = 0; i < xg_vecYokoHints.size(); ++i) {
                if (xg_ahwndYokoStatics[i] == hwndCtl) {
                    ::SendMessageW(xg_ahwndYokoEdits[i], EM_SETSEL, 0, -1);
                    ::SetFocus(xg_ahwndYokoEdits[i]);
                    return;
                }
            }
        }
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
            if (xg_ahwndTateEdits.size())
                ::SetFocus(xg_ahwndTateEdits[0]);
            break;

        case VK_F6:
            if (::GetAsyncKeyState(VK_SHIFT) < 0)
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
            else
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
            break;
        }
    }


    // マウスホイールが回転した。
    void __fastcall
    OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
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

    // ヒント ウィンドウのサイズを制限する。
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
    {
        lpMinMaxInfo->ptMinTrackSize.x = 256;
        lpMinMaxInfo->ptMinTrackSize.y = 128;
    }

    // 「ヒント」ウィンドウのコンテキストメニュー。
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
    {
        WCHAR szClass[8];
        szClass[0] = 0;
        GetClassNameW(hwndContext, szClass, ARRAYSIZE(szClass));
        if (lstrcmpiW(szClass, L"EDIT") == 0)
            return;

        HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(2));
        HMENU hSubMenu = GetSubMenu(hMenu, 1);

        // 右クリックメニューを表示する。
        ::SetForegroundWindow(hwnd);
        INT nCmd = ::TrackPopupMenu(
            hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD,
            xPos, yPos, 0, hwnd, NULL);
        ::PostMessageW(hwnd, WM_NULL, 0, 0);
        if (nCmd)
            ::PostMessageW(xg_hMainWnd, WM_COMMAND, nCmd, 0);

        ::DestroyMenu(hMenu);
    }

    // ヒント ウィンドウのウィンドウ プロシージャー。
    virtual LRESULT CALLBACK
    WindowProcDx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
        HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hWnd, WM_SIZE, OnSize);
        HANDLE_MSG(hWnd, WM_VSCROLL, OnVScroll);
        HANDLE_MSG(hWnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hWnd, WM_KEYUP, OnKey);
        HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hWnd, WM_MOUSEWHEEL, OnMouseWheel);
        HANDLE_MSG(hWnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
        HANDLE_MSG(hWnd, WM_CONTEXTMENU, OnContextMenu);
        HANDLE_MSG(hWnd, WM_CTLCOLOREDIT, OnCtlColor);
        default:
            return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        return 0;
    }
};
