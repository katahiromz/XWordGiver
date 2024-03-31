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
    inline static HFONT               xg_hHintsUIFont = nullptr;

    // 縦のカギのコントロール群。
    inline static HWND                xg_hwndVertCaptionStatic = nullptr;
    inline static std::vector<HWND>   xg_ahwndVertStatics;
    inline static std::vector<HWND>   xg_ahwndVertEdits;

    // 横のカギのコントロール群。
    inline static HWND                xg_hwndHorzCaptionStatic = nullptr;
    inline static std::vector<HWND>   xg_ahwndHorzStatics;
    inline static std::vector<HWND>   xg_ahwndHorzEdits;

    // ヒントウィンドウの位置とサイズ。
    inline static int s_nHintsWndX = CW_USEDEFAULT, s_nHintsWndY = CW_USEDEFAULT;
    inline static int s_nHintsWndCX = CW_USEDEFAULT, s_nHintsWndCY = CW_USEDEFAULT;

    // ハイライト。
    inline static HWND s_hwndHighlightVertEdit = nullptr;
    inline static HWND s_hwndHighlightHorzEdit = nullptr;

    // ハイライトを更新する。
    void setHighlight(int nHorz, int nVert) noexcept
    {
        HWND hwndVertOld = s_hwndHighlightVertEdit;
        HWND hwndHorzOld = s_hwndHighlightHorzEdit;
        HWND hwndVert = nullptr, hwndHorz = nullptr;

        if (nVert != -1) {
            for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
                if (xg_vecVertHints.size() <= i || xg_ahwndVertEdits.size() <= i)
                    break;
                if (xg_vecVertHints[i].m_number == nVert) {
                    hwndVert = xg_ahwndVertEdits[i];
                    break;
                }
            }
        }
        if (nHorz != -1) {
            for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                if (xg_vecHorzHints.size() <= i || xg_ahwndHorzEdits.size() <= i)
                    break;
                if (xg_vecHorzHints[i].m_number == nHorz) {
                    hwndHorz = xg_ahwndHorzEdits[i];
                    break;
                }
            }
        }

        s_hwndHighlightVertEdit = hwndVert;
        s_hwndHighlightHorzEdit = hwndHorz;

        if (hwndVert)
            InvalidateRect(hwndVert, nullptr, TRUE);
        if (hwndHorz)
            InvalidateRect(hwndHorz, nullptr, TRUE);
        if (hwndVertOld)
            InvalidateRect(hwndVertOld, nullptr, TRUE);
        if (hwndHorzOld)
            InvalidateRect(hwndHorzOld, nullptr, TRUE);
    }

    // ハイライトに背景色を付ける。
    HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type) noexcept
    {
        if (hwndChild == m_hwndTrackMenuOn)
        {
            constexpr COLORREF rgbColor = RGB(255, 140, 255);
            SetBkColor(hdc, rgbColor);
            SetDCBrushColor(hdc, rgbColor);
            return GetStockBrush(DC_BRUSH);
        }
        else if (hwndChild == s_hwndHighlightVertEdit || hwndChild == s_hwndHighlightHorzEdit)
        {
            constexpr COLORREF rgbColor = RGB(140, 255, 255);
            SetBkColor(hdc, rgbColor);
            SetDCBrushColor(hdc, rgbColor);
            return GetStockBrush(DC_BRUSH);
        }
        else if (hwndChild == GetFocus())
        {
            constexpr COLORREF rgbColor = RGB(255, 255, 140);
            SetBkColor(hdc, rgbColor);
            SetDCBrushColor(hdc, rgbColor);
            return GetStockBrush(DC_BRUSH);
        }
        else
        {
            return FORWARD_WM_CTLCOLOREDIT(hwnd, hdc, hwndChild, ::DefWindowProcW);
        }
    }

    // ヒントが変更されたか？
    static bool AreHintsModified(void) noexcept
    {
        if (xg_bHintsAdded) {
            return true;
        }

        if (::IsWindow(xg_hHintsWnd)) {
            for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndVertEdits[i], EM_GETMODIFY, 0, 0)) {
                    return true;
                }
            }
            for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndHorzEdits[i], EM_GETMODIFY, 0, 0)) {
                    return true;
                }
            }
        }
        return false;
    }

    // ヒントデータを設定する。
    static void SetHintsData(void) noexcept
    {
        for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
            ::SetWindowTextW(xg_ahwndVertEdits[i], xg_vecVertHints[i].m_strHint.data());
        }
        for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
            ::SetWindowTextW(xg_ahwndHorzEdits[i], xg_vecHorzHints[i].m_strHint.data());
        }
    }

    // ヒントデータを更新する。
    static bool UpdateHintData(void)
    {
        bool updated = false;
        if (::IsWindow(xg_hHintsWnd)) {
            WCHAR sz[512];
            for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndVertEdits[i], EM_GETMODIFY, 0, 0)) {
                    updated = true;
                    ::GetWindowTextW(xg_ahwndVertEdits[i], sz, 
                                     static_cast<int>(_countof(sz)));
                    xg_vecVertHints[i].m_strHint = sz;
                    ::SendMessageW(xg_ahwndVertEdits[i], EM_SETMODIFY, FALSE, 0);
                }
            }
            for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                if (::SendMessageW(xg_ahwndHorzEdits[i], EM_GETMODIFY, 0, 0)) {
                    updated = true;
                    ::GetWindowTextW(xg_ahwndHorzEdits[i], sz, 
                                     static_cast<int>(_countof(sz)));
                    xg_vecHorzHints[i].m_strHint = sz;
                    ::SendMessageW(xg_ahwndVertEdits[i], EM_SETMODIFY, FALSE, 0);
                }
            }
        }
        if (updated)
            XG_FILE_MODIFIED(TRUE);
        return updated;
    }

    XG_HintsWnd() noexcept
    {
    }

    LPCTSTR GetWndClassName() const override
    {
        return TEXT("XG_HintsWnd");
    }

    void ModifyWndClassDx(WNDCLASSEX& wcx) override
    {
        // No change
    }

    // WM_SIZE
    // ヒントウィンドウのサイズが変わった。
    void OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/)
    {
        if (xg_hwndVertCaptionStatic == nullptr)
            return;

        xg_svHintsScrollView.clear();

        MRect rcClient;
        ::GetClientRect(hwnd, &rcClient);

        MSize size1, size2;
        {
            HDC hdc = ::CreateCompatibleDC(nullptr);
            WCHAR label[64];
            StringCchPrintf(label, _countof(label), XgLoadStringDx1(IDS_DOWNNUMBER), 100);
            XGStringW strLabel = label;
            ::SelectObject(hdc, ::GetStockObject(SYSTEM_FIXED_FONT));
            ::GetTextExtentPoint32W(hdc, strLabel.data(), static_cast<int>(strLabel.size()), &size1);
            ::SelectObject(hdc, xg_hHintsUIFont);
            ::GetTextExtentPoint32W(hdc, strLabel.data(), static_cast<int>(strLabel.size()), &size2);
            ::DeleteDC(hdc);
        }

        WCHAR szText[512];
        HDC hdc = ::CreateCompatibleDC(nullptr);
        HGDIOBJ hFontOld = ::SelectObject(hdc, xg_hHintsUIFont);
        int y = 0;

        // タテのカギ。
        {
            const MRect rcCtrl(MPoint(0, y + 4), MSize(rcClient.Width(), size1.cy + 4));
            xg_svHintsScrollView.AddCtrlInfo(xg_hwndVertCaptionStatic, rcCtrl);
            y += size1.cy + 8;
        }
        if (rcClient.Width() - size2.cx - 8 > 0) {
            for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
                ::GetWindowTextW(xg_ahwndVertEdits[i], szText,
                                 _countof(szText));
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
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndVertEdits[i], rcCtrl);
                rcCtrl.left = 0;
                rcCtrl.right = size2.cx;
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndVertStatics[i], rcCtrl);
                y += rcCtrl.Height();
            }
        }
        // ヨコのカギ。
        {
            const MRect rcCtrl(MPoint(0, y + 4), MSize(rcClient.Width(), size1.cy + 4));
            xg_svHintsScrollView.AddCtrlInfo(xg_hwndHorzCaptionStatic, rcCtrl);
            y += size1.cy + 8;
        }
        if (rcClient.Width() - size2.cx - 8 > 0) {
            for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                ::GetWindowTextW(xg_ahwndHorzEdits[i], szText, _countof(szText));
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
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndHorzEdits[i], rcCtrl);
                rcCtrl.left = 0;
                rcCtrl.right = size2.cx;
                xg_svHintsScrollView.AddCtrlInfo(xg_ahwndHorzStatics[i], rcCtrl);
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
        bool    m_fVert;
    };

    static LRESULT CALLBACK XgHintEdit_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WNDPROC fn;
        XG_HintEditData *data =
            reinterpret_cast<XG_HintEditData *>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

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
                bool found = false, vertical = false;
                size_t i = 0;
                if (!found) {
                    for (auto& edit : xg_ahwndVertEdits) {
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
                    for (auto& edit : xg_ahwndHorzEdits) {
                        if (edit == hwnd) {
                            found = true;
                            vertical = false;
                            break;
                        }
                        ++i;
                    }
                }
                if (found) {
                    const auto number = (vertical ? xg_vecVertHints[i].m_number : xg_vecHorzHints[i].m_number);
                    PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT, TRUE, MAKELPARAM(number, vertical));
                }
            }

            // フォーカスのあるコントロールが見えるように移動する。
            xg_svHintsScrollView.EnsureCtrlVisible(hwnd);

            // フォーカスを失うコントロールを再描画する。
            if (wParam)
                InvalidateRect(reinterpret_cast<HWND>(wParam), nullptr, TRUE);

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

            // フォーカスを失うコントロールを再描画する。
            if (wParam)
                InvalidateRect(reinterpret_cast<HWND>(wParam), nullptr, TRUE);

            return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);

        case WM_KEYDOWN:
            if (wParam == VK_RETURN) {
                ::PostMessageW(xg_hHintsWnd, WM_SIZE, 0, 0);
                ::SetFocus(nullptr);
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

    // WM_CREATE
    // ヒントウィンドウが作成された。
    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/)
    {
        xg_hHintsWnd = hwnd;

        // 初期化。
        xg_ahwndVertStatics.clear();
        xg_ahwndVertEdits.clear();
        xg_ahwndHorzStatics.clear();
        xg_ahwndHorzEdits.clear();
        xg_svHintsScrollView.SetParent(hwnd);
        xg_svHintsScrollView.ShowScrollBars(FALSE, TRUE);
        s_hwndHighlightVertEdit = s_hwndHighlightHorzEdit = nullptr;

        if (xg_hHintsUIFont) {
            ::DeleteObject(xg_hHintsUIFont);
        }
        xg_hHintsUIFont = ::CreateFontIndirectW(XgGetUIFont());
        if (xg_hHintsUIFont == nullptr) {
            xg_hHintsUIFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
        }

        HWND hwndCtrl;

        hwndCtrl = ::CreateWindowW(
            TEXT("STATIC"), XgLoadStringDx1(IDS_DOWN),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY |
            SS_CENTER | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
        if (hwndCtrl == nullptr)
            return FALSE;
        xg_hwndVertCaptionStatic = hwndCtrl;
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(::GetStockObject(SYSTEM_FIXED_FONT)),
            TRUE);

        hwndCtrl = ::CreateWindowW(
            TEXT("STATIC"), XgLoadStringDx1(IDS_ACROSS),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_NOTIFY |
            SS_CENTER | SS_CENTERIMAGE,
            0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
        if (hwndCtrl == nullptr)
            return FALSE;
        xg_hwndHorzCaptionStatic = hwndCtrl;
        ::SendMessageW(hwndCtrl, WM_SETFONT,
            reinterpret_cast<WPARAM>(::GetStockObject(SYSTEM_FIXED_FONT)),
            TRUE);

        XG_HintEditData *data;
        WCHAR sz[256];
        for (const auto& hint : xg_vecVertHints) {
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_DOWNNUMBER), hint.m_number);
            hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz,
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE,
                0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            ::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, MAKELONG(hint.m_number, TRUE));
            if (hwndCtrl == nullptr)
                return FALSE;

            xg_ahwndVertStatics.emplace_back(hwndCtrl);

            hwndCtrl = ::CreateWindowExW(
                WS_EX_CLIENTEDGE,
                TEXT("EDIT"), hint.m_strHint.data(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE,
                0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            if (hwndCtrl == nullptr)
                return FALSE;

            data = static_cast<XG_HintEditData*>(::LocalAlloc(LMEM_FIXED, sizeof(XG_HintEditData)));
            data->m_fVert = true;
            data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwndCtrl, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(XgHintEdit_WndProc)));
            ::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(data));
            xg_ahwndVertEdits.emplace_back(hwndCtrl);
        }
        for (const auto& hint : xg_vecHorzHints) {
            StringCchPrintf(sz, _countof(sz), XgLoadStringDx1(IDS_ACROSSNUMBER), hint.m_number);
            hwndCtrl = ::CreateWindowW(TEXT("STATIC"), sz,
                WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_NOPREFIX | SS_NOTIFY | SS_CENTERIMAGE,
                0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            ::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA, MAKELONG(hint.m_number, FALSE));
            if (hwndCtrl == nullptr)
                return FALSE;

            xg_ahwndHorzStatics.emplace_back(hwndCtrl);

            hwndCtrl = ::CreateWindowExW(
                WS_EX_CLIENTEDGE,
                TEXT("EDIT"), hint.m_strHint.data(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL | ES_MULTILINE,
                0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
            assert(hwndCtrl);
            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hHintsUIFont),
                TRUE);
            if (hwndCtrl == nullptr)
                return FALSE;

            data = static_cast<XG_HintEditData*>(::LocalAlloc(LMEM_FIXED, sizeof(XG_HintEditData)));
            data->m_fVert = false;
            data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwndCtrl, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(XgHintEdit_WndProc)));
            ::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(data));
            xg_ahwndHorzEdits.emplace_back(hwndCtrl);
        }

        if (xg_ahwndVertEdits.size())
            ::SetFocus(xg_ahwndVertEdits[0]);

        ::PostMessageW(hwnd, WM_SIZE, 0, 0);

        return TRUE;
    }

    // WM_VSCROLL
    // ヒントウィンドウが縦にスクロールされた。
    void OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos) noexcept
    {
        xg_svHintsScrollView.Scroll(SB_VERT, code, pos);
    }

    // WM_DESTROY
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

        xg_hHintsWnd = nullptr;
        xg_hwndVertCaptionStatic = nullptr;
        xg_hwndHorzCaptionStatic = nullptr;
        xg_ahwndVertStatics.clear();
        xg_ahwndVertEdits.clear();
        xg_ahwndHorzStatics.clear();
        xg_ahwndHorzEdits.clear();
        xg_svHintsScrollView.clear();
        s_hwndHighlightVertEdit = s_hwndHighlightHorzEdit = nullptr;

        ::DeleteObject(xg_hHintsUIFont);
        xg_hHintsUIFont = nullptr;
    }

    // WM_COMMAND
    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept
    {
        if (id == ID_JUMPNUMBER) {
            PostMessageW(GetParent(hwnd), WM_COMMAND, id, 0);
            return;
        }
        if (codeNotify == STN_CLICKED) {
            for (size_t i = 0; i < xg_vecVertHints.size(); ++i) {
                if (xg_ahwndVertStatics[i] == hwndCtl) {
                    ::SendMessageW(xg_ahwndVertEdits[i], EM_SETSEL, 0, -1);
                    ::SetFocus(xg_ahwndVertEdits[i]);
                    return;
                }
            }
            for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                if (xg_ahwndHorzStatics[i] == hwndCtl) {
                    ::SendMessageW(xg_ahwndHorzEdits[i], EM_SETSEL, 0, -1);
                    ::SetFocus(xg_ahwndHorzEdits[i]);
                    return;
                }
            }
        }
    }

    // WM_KEYDOWN / WM_KEYUP
    // キーが押された。
    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) noexcept
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
            if (xg_ahwndVertEdits.size())
                ::SetFocus(xg_ahwndVertEdits[0]);
            break;

        case VK_F6:
            if (::GetAsyncKeyState(VK_SHIFT) < 0)
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANEPREV, 0);
            else
                ::SendMessageW(xg_hMainWnd, WM_COMMAND, ID_PANENEXT, 0);
            break;

        default:
            break;
        }
    }

    // ズーム。
    void OnZoom(HWND hwnd, BOOL bZoomIn)
    {
        // フォント情報を取得。
        LOGFONTW *plf = XgGetUIFont();
        int height = labs(plf->lfHeight);
        if (height == 0)
            height = 12;
        HDC hdc = ::CreateCompatibleDC(nullptr);
        int pointsize = MulDiv(height, 72, ::GetDeviceCaps(hdc, LOGPIXELSY));
        ::DeleteDC(hdc);

        // ズームに応じて、フォントサイズを増減。
        if (bZoomIn)
            pointsize += 1;
        else
            pointsize -= 1;

        // 大きすぎない。小さすぎない。
        if (pointsize > 30)
            pointsize = 30;
        if (pointsize < 6)
            pointsize = 6;

        // フォント設定を更新。
        StringCchPrintfW(xg_szUIFont, _countof(xg_szUIFont), L"%s, %d", plf->lfFaceName, pointsize);

        // フォントを再作成。
        if (xg_hHintsUIFont)
            ::DeleteObject(xg_hHintsUIFont);
        xg_hHintsUIFont = ::CreateFontIndirectW(XgGetUIFont());

        // 再描画をOFFにする。
        SetWindowRedraw(hwnd, FALSE);

        // フォントをセット。再描画はWM_SIZEに任せる。
        for (auto hwndCtrl : xg_ahwndVertStatics)
            SetWindowFont(hwndCtrl, xg_hHintsUIFont, FALSE);
        for (auto hwndCtrl : xg_ahwndVertEdits)
            SetWindowFont(hwndCtrl, xg_hHintsUIFont, FALSE);
        for (auto hwndCtrl : xg_ahwndHorzStatics)
            SetWindowFont(hwndCtrl, xg_hHintsUIFont, FALSE);
        for (auto hwndCtrl : xg_ahwndHorzEdits)
            SetWindowFont(hwndCtrl, xg_hHintsUIFont, FALSE);

        // 再描画をONにする。
        SetWindowRedraw(hwnd, TRUE);

        // 再配置。
        ::PostMessageW(hwnd, WM_SIZE, 0, 0);
    }

    // WM_MOUSEWHEEL
    // マウスホイールが回転した。
    void __fastcall OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys)
    {
        if (::GetAsyncKeyState(VK_CONTROL) < 0) {
            if (zDelta < 0) {
                OnZoom(hwnd, FALSE);
            } else if (zDelta > 0) {
                OnZoom(hwnd, TRUE);
            }
        }
        else if (::GetAsyncKeyState(VK_SHIFT) < 0) {
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

    // WM_GETMINMAXINFO
    // ヒント ウィンドウのサイズを制限する。
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept
    {
        lpMinMaxInfo->ptMinTrackSize.x = 256;
        lpMinMaxInfo->ptMinTrackSize.y = 128;
    }

    // ジャンプ先番号。
    INT m_nNumber = 0;
    BOOL m_bVert = FALSE;
    HWND m_hwndTrackMenuOn = NULL;

    // WM_CONTEXTMENU
    // 「ヒント」ウィンドウのコンテキストメニュー。
    void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos) noexcept
    {
        WCHAR szClass[8];
        szClass[0] = 0;
        GetClassNameW(hwndContext, szClass, _countof(szClass));
        if (lstrcmpiW(szClass, L"EDIT") == 0)
            return;
        BOOL bStatic = (lstrcmpiW(szClass, L"STATIC") == 0);

        // メニューを読み込む。
        HMENU hMenu = LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(2));
        HMENU hSubMenu = GetSubMenu(hMenu, 1);

        // スタティックでなければ、番号ジャンプのメニュー項目を削除する。
        LONG nData = (LONG)::GetWindowLongPtrW(hwndContext, GWLP_USERDATA);
        m_nNumber = LOWORD(nData);
        m_bVert = HIWORD(nData);
        if (!bStatic || !m_nNumber)
            DeleteMenu(hSubMenu, ID_JUMPNUMBER, MF_BYCOMMAND);

        // スタティックなら色を変える。
        if (bStatic)
        {
            m_hwndTrackMenuOn = hwndContext;
            InvalidateRect(hwndContext, NULL, TRUE);
        }

        // 右クリックメニューを表示する。
        ::SetForegroundWindow(hwnd);
        constexpr auto flags = TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD;
        const auto nCmd = ::TrackPopupMenu(hSubMenu, flags, xPos, yPos, 0, hwnd, nullptr);

        // 色を戻す。
        if (bStatic)
        {
            m_hwndTrackMenuOn = NULL;
            InvalidateRect(hwndContext, NULL, TRUE);
        }

        ::PostMessageW(hwnd, WM_NULL, 0, 0);
        if (nCmd)
            ::PostMessageW(xg_hMainWnd, WM_COMMAND, nCmd, 0);

        ::DestroyMenu(hMenu);
    }

    // ヒント ウィンドウのウィンドウ プロシージャー。
    LRESULT CALLBACK
    WindowProcDx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
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
        HANDLE_MSG(hWnd, WM_CTLCOLORSTATIC, OnCtlColor);
        default:
            return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
        }
        return 0;
    }
};
