#pragma once

#include "XG_Window.hpp"
#include "XG_Settings.hpp"

struct XG_CandsButtonData
{
    WNDPROC m_fnOldWndProc;
};

// クロスワードをチェックする。
bool __fastcall XgCheckCrossWord(HWND hwnd, bool check_words);

// 候補ウィンドウ。
class XG_CandsWnd : public XG_Window
{
public:
    // 候補ウィンドウの位置とサイズ。
    inline static int s_nCandsWndX = CW_USEDEFAULT, s_nCandsWndY = CW_USEDEFAULT;
    inline static int s_nCandsWndCX = CW_USEDEFAULT, s_nCandsWndCY = CW_USEDEFAULT;

    // 候補ウィンドウのスクロールビュー。
    inline static MScrollView                 xg_svCandsScrollView;

    // 候補ウィンドウのUIフォント。
    inline static HFONT                       xg_hCandsUIFont = nullptr;

    // 候補。
    inline static std::vector<XGStringW>   xg_vecCandidates;

    // 候補ボタン。
    inline static std::vector<HWND>           xg_ahwndCandButtons;

    // 候補を求める位置。
    inline static int xg_jCandPos, xg_iCandPos;

    // 候補はタテかヨコか。
    inline static bool xg_bCandVertical;

    // 候補ウィンドウを作成する。
    BOOL Create(HWND hwnd)
    {
        constexpr auto style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_HSCROLL | WS_VSCROLL;
        constexpr auto exstyle = WS_EX_TOOLWINDOW;
        auto text = XgLoadStringDx1(IDS_CANDIDATES);
        return CreateWindowDx(hwnd, text, style, exstyle,
                              s_nCandsWndX, s_nCandsWndY,
                              s_nCandsWndCX, s_nCandsWndCY);
    }

    bool Open(HWND hwnd, bool vertical)
    {
        // 候補を作成する。
        xg_iCandPos = xg_caret_pos.m_i;
        xg_jCandPos = xg_caret_pos.m_j;
        xg_bCandVertical = vertical;
        if (xg_xword.GetAt(xg_iCandPos, xg_jCandPos) == ZEN_BLACK) {
            ::MessageBeep(0xFFFFFFFF);
            return false;
        }

        // パターンを取得する。
        int lo, hi;
        XGStringW pattern;
        bool left_black, right_black;
        if (xg_bCandVertical) {
            lo = hi = xg_iCandPos;
            while (lo > 0) {
                if (xg_xword.GetAt(lo - 1, xg_jCandPos) != ZEN_BLACK)
                    --lo;
                else
                    break;
            }
            while (hi + 1 < xg_nRows) {
                if (xg_xword.GetAt(hi + 1, xg_jCandPos) != ZEN_BLACK)
                    ++hi;
                else
                    break;
            }

            for (int i = lo; i <= hi; ++i) {
                pattern += xg_xword.GetAt(i, xg_jCandPos);
            }

            right_black = (hi + 1 != xg_nRows);
        } else {
            lo = hi = xg_jCandPos;
            while (lo > 0) {
                if (xg_xword.GetAt(xg_iCandPos, lo - 1) != ZEN_BLACK)
                    --lo;
                else
                    break;
            }
            while (hi + 1 < xg_nCols) {
                if (xg_xword.GetAt(xg_iCandPos, hi + 1) != ZEN_BLACK)
                    ++hi;
                else
                    break;
            }

            for (int j = lo; j <= hi; ++j) {
                pattern += xg_xword.GetAt(xg_iCandPos, j);
            }

            right_black = (hi + 1 != xg_nCols);
        }
        left_black = (lo != 0);

        // 候補を取得する。
        int nSkip = 0;
        std::vector<XGStringW> cands, cands2;
        XgGetCandidatesAddBlack<false>(cands, pattern, nSkip, left_black, right_black);
        XgGetCandidatesAddBlack<true>(cands2, pattern, nSkip, left_black, right_black);
        cands.insert(cands.end(), cands2.begin(), cands2.end());

        // 正規化して右側の空白を取り除く。
        for (auto& cand : cands) {
            cand = XgNormalizeString(cand);
            xg_str_trim_right(cand);
        }

        // ソートして一意化。短い方を優先する。
        std::sort(cands.begin(), cands.end(), [](const XGStringW& x, const XGStringW& y) noexcept {
            if (x.size() == y.size())
                return x < y;
            return x.size() < y.size();
        });
        cands.erase(std::unique(cands.begin(), cands.end()), cands.end());

        // 仮に適用して、正当かどうか確かめ、正当なものだけを
        // 最終的な候補とする。
        XG_CandsWnd::xg_vecCandidates.clear();
        for (const auto& cand : cands) {
            XG_Board xword(xg_xword);
            XgApplyCandidate(xword, cand);
            if (xword.IsValid())
            {
                XG_CandsWnd::xg_vecCandidates.emplace_back(cand);
            }
        }

        // 個数制限。
        if (XG_CandsWnd::xg_vecCandidates.size() > xg_nMaxCandidates)
            XG_CandsWnd::xg_vecCandidates.resize(xg_nMaxCandidates);

        if (XG_CandsWnd::xg_vecCandidates.empty()) {
            if (XgCheckCrossWord(hwnd, false)) {
                ::MessageBeep(0xFFFFFFFF);
            } else {
                return false;
            }
        }

        // ヒントウィンドウを作成する。
        if (Create(xg_hMainWnd)) {
            ::ShowWindow(*this, SW_SHOWNORMAL);
            ::UpdateWindow(*this);
            return true;
        }
        return false;
    }

    static LRESULT CALLBACK
    XgCandsButton_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        WNDPROC fn;
        XG_CandsButtonData *data =
            reinterpret_cast<XG_CandsButtonData *>(
                ::GetWindowLongPtrW(hwnd, GWLP_USERDATA));
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
                ::SetFocus(nullptr);
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

    XG_CandsWnd() noexcept
    {
    }

    LPCTSTR GetWndClassName() const override
    {
        return TEXT("XG_CandsWnd");
    }

    void ModifyWndClassDx(WNDCLASSEX& wcx) override
    {
        wcx.hIcon = nullptr;
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

        HDC hdc = ::CreateCompatibleDC(nullptr);
        HGDIOBJ hFontOld = ::SelectObject(hdc, xg_hCandsUIFont);
        {
            MPoint pt;
            for (size_t i = 0; i < xg_vecCandidates.size(); ++i) {
                const XGStringW& strLabel = xg_vecCandidates[i];

                MSize siz;
                ::GetTextExtentPoint32W(hdc, strLabel.data(),
                                        static_cast<int>(strLabel.size()), &siz);

                if (pt.x != 0 && pt.x + siz.cx + 16 > rcClient.Width()) {
                    pt.x = 0;
                    pt.y += siz.cy + 16;
                }

                const MRect rcCtrl(MPoint(pt.x + 4, pt.y + 4), MSize(siz.cx + 8, siz.cy + 8));
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
        if (xg_hCandsUIFont) {
            ::DeleteObject(xg_hCandsUIFont);
        }
        xg_hCandsUIFont = ::CreateFontIndirectW(XgGetUIFont());
        if (xg_hCandsUIFont == nullptr) {
            xg_hCandsUIFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
        }

        // 初期化。
        xg_ahwndCandButtons.clear();
        xg_svCandsScrollView.clear();

        xg_svCandsScrollView.SetParent(hwnd);
        xg_svCandsScrollView.ShowScrollBars(FALSE, TRUE);

        HWND hwndCtrl;
        XG_CandsButtonData *data;
        for (auto& cand : xg_vecCandidates) {
            WCHAR szText[64];
            if (xg_bHiragana) {
                LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_HIRAGANA,
                             cand.data(), -1, szText, _countof(szText));
                cand = szText;
            }
            if (xg_bLowercase) {
                LCMapStringW(XG_JPN_LOCALE, LCMAP_FULLWIDTH | LCMAP_LOWERCASE,
                             cand.data(), -1, szText, _countof(szText));
                cand = szText;
            }

            hwndCtrl = ::CreateWindowW(
                TEXT("BUTTON"), cand.data(),
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                0, 0, 0, 0, hwnd, nullptr, xg_hInstance, nullptr);
            assert(hwndCtrl);
            if (hwndCtrl == nullptr)
                return FALSE;

            ::SendMessageW(hwndCtrl, WM_SETFONT,
                reinterpret_cast<WPARAM>(xg_hCandsUIFont),
                FALSE);
            xg_ahwndCandButtons.emplace_back(hwndCtrl);

            data = static_cast<XG_CandsButtonData*>(::LocalAlloc(LMEM_FIXED, sizeof(XG_CandsButtonData)));
            data->m_fnOldWndProc = reinterpret_cast<WNDPROC>(
                ::SetWindowLongPtrW(hwndCtrl, GWLP_WNDPROC,
                    reinterpret_cast<LONG_PTR>(XgCandsButton_WndProc)));
            ::SetWindowLongPtrW(hwndCtrl, GWLP_USERDATA,
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
    void OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos) noexcept
    {
        xg_svCandsScrollView.Scroll(SB_VERT, code, pos);
    }

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

        default:
            break;
        }
    }

    // 候補ウィンドウが破棄された。
    void OnDestroy(HWND hwnd) noexcept
    {
        // 現在の位置とサイズを保存する。
        MRect rc;
        ::GetWindowRect(hwnd, &rc);
        s_nCandsWndX = rc.left;
        s_nCandsWndY = rc.top;
        s_nCandsWndCX = rc.Width();
        s_nCandsWndCY = rc.Height();

        xg_ahwndCandButtons.clear();
        xg_svCandsScrollView.clear();

        ::DeleteObject(xg_hCandsUIFont);
        xg_hCandsUIFont = nullptr;

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
                    const int x = XgGetHScrollPos(), y = XgGetVScrollPos();
                    XgUpdateImage(xg_hMainWnd, x, y);
                    return;
                }
            }
        }
    }

    // マウスホイールが回転した。
    void OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys) noexcept
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
    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept
    {
        lpMinMaxInfo->ptMinTrackSize.x = 256;
        lpMinMaxInfo->ptMinTrackSize.y = 128;
    }

    // 候補ウィンドウを描画する。
    void OnPaint(HWND hwnd) noexcept
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

    LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
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

    // 候補を適用する。
    void XgApplyCandidate(XG_Board& xword, const XGStringW& strCand)
    {
        XGStringW cand = XgNormalizeString(strCand);

        int lo, hi;
        if (xg_bCandVertical) {
            for (lo = xg_iCandPos; lo > 0; --lo) {
                if (xword.GetAt(lo - 1, xg_jCandPos) == ZEN_BLACK)
                    break;
            }
            for (hi = xg_iCandPos; hi + 1 < xg_nRows; ++hi) {
                if (xword.GetAt(hi + 1, xg_jCandPos) == ZEN_BLACK)
                    break;
            }

            int m = 0;
            for (int k = lo; k <= hi; ++k, ++m) {
                if (cand.size() <= static_cast<size_t>(m))
                    break;
                xword.SetAt(k, xg_jCandPos, cand[m]);
            }
        }
        else
        {
            for (lo = xg_jCandPos; lo > 0; --lo) {
                if (xword.GetAt(xg_iCandPos, lo - 1) == ZEN_BLACK)
                    break;
            }
            for (hi = xg_jCandPos; hi + 1 < xg_nCols; ++hi) {
                if (xword.GetAt(xg_iCandPos, hi + 1) == ZEN_BLACK)
                    break;
            }

            int m = 0;
            for (int k = lo; k <= hi; ++k, ++m) {
                if (cand.size() <= static_cast<size_t>(m))
                    break;
                xword.SetAt(xg_iCandPos, k, cand[m]);
            }
        }
    }
};
