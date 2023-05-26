#pragma once

#include "XG_Window.hpp"
#include "XG_TextBoxDialog.hpp"
#include "XG_PictureBoxDialog.hpp"
#include "XG_UndoBuffer.hpp"
#include "XG_Settings.hpp"

#define XGWM_REDRAW (WM_USER + 101) // 再描画メッセージ。
#define CXY_GRIP 5 // グリップのサイズ。
#define X0 rc.left
#define X1 ((rc.left + rc.right - CXY_GRIP) / 2)
#define X2 (rc.right - CXY_GRIP)
#define Y0 rc.top
#define Y1 ((rc.top + rc.bottom - CXY_GRIP) / 2)
#define Y2 (rc.bottom - CXY_GRIP)
#define RECT0 X0, Y0, X0 + CXY_GRIP, Y0 + CXY_GRIP // Upper Left
#define RECT1 X1, Y0, X1 + CXY_GRIP, Y0 + CXY_GRIP // Top
#define RECT2 X2, Y0, X2 + CXY_GRIP, Y0 + CXY_GRIP // Upper Right
#define RECT3 X0, Y1, X0 + CXY_GRIP, Y1 + CXY_GRIP // Left
#define RECT4 X2, Y1, X2 + CXY_GRIP, Y1 + CXY_GRIP // Right
#define RECT5 X0, Y2, X0 + CXY_GRIP, Y2 + CXY_GRIP // Lower Left
#define RECT6 X1, Y2, X1 + CXY_GRIP, Y2 + CXY_GRIP // Bottom
#define RECT7 X2, Y2, X2 + CXY_GRIP, Y2 + CXY_GRIP // Lower Right

// ファイル変更フラグ。
extern BOOL xg_bFileModified;

// マス位置を取得する。
VOID XgGetCellPosition(RECT& rc, int i1, int j1, int i2, int j2, BOOL bScroll) noexcept;
// マス位置を設定する。
VOID XgSetCellPosition(LONG& x, LONG& y, int& i, int& j, BOOL bEnd) noexcept;

class XG_BoxWindow : public XG_Window
{
public:
    std::wstring m_type;
    HWND m_hwndParent;
    HRGN m_hRgn;
    int m_i1, m_j1, m_i2, m_j2;
    std::wstring m_strText;
    RECT m_rcOld;
    static inline HWND s_hwndSelected = nullptr;

    XG_BoxWindow(const std::wstring& type, int i1 = 0, int j1 = 0, int i2 = 1, int j2 = 1)
        : m_type(type)
        , m_hwndParent(nullptr)
        , m_hRgn(nullptr)
        , m_i1(i1)
        , m_j1(j1)
        , m_i2(i2)
        , m_j2(j2)
    {
    }

    virtual ~XG_BoxWindow()
    {
        DeleteObject(m_hRgn);
        m_hRgn = nullptr;
    }

    LPCTSTR GetWndClassName() const noexcept override
    {
        return TEXT("XG_BoxWindow");
    }

    void ModifyWndClassDx(WNDCLASSEX& wcx) override
    {
        wcx.style = CS_DBLCLKS;
    }

    HRGN DoGetRgn(HWND hwnd) noexcept
    {
        RECT rc, rcInner1, rcInner2;
        GetWindowRect(hwnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);

        rcInner1 = rc;
        InflateRect(&rcInner1, -CXY_GRIP / 2, -CXY_GRIP / 2);
        rcInner2 = rcInner1;
        InflateRect(&rcInner2, -1, -1);

        HRGN hRgn = CreateRectRgn(0, 0, 0, 0);

        HRGN hInner1Rgn = CreateRectRgnIndirect(&rcInner1);
        UnionRgn(hRgn, hRgn, hInner1Rgn);
        DeleteObject(hInner1Rgn);
        HRGN hInner2Rgn = CreateRectRgnIndirect(&rcInner2);
        CombineRgn(hRgn, hRgn, hInner2Rgn, RGN_DIFF);
        DeleteObject(hInner2Rgn);

        HRGN hGripRgn;

        hGripRgn = CreateRectRgn(RECT0);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT1);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT2);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT3);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT4);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT5);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT6);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        hGripRgn = CreateRectRgn(RECT7);
        UnionRgn(hRgn, hRgn, hGripRgn);
        DeleteObject(hGripRgn);

        RECT rcRect = rc;
        InflateRect(&rcRect, -CXY_GRIP, -CXY_GRIP);
        HRGN hRectRgn = CreateRectRgnIndirect(&rcRect);
        UnionRgn(hRgn, hRgn, hRectRgn);
        DeleteObject(hRectRgn);

        return hRgn;
    }

    void DoDrawGrips(HWND hwnd, HDC hDC, RECT& rc) noexcept
    {
        RECT rcInner1 = rc;
        InflateRect(&rcInner1, -CXY_GRIP / 2, -CXY_GRIP / 2);

        HGDIOBJ hPenOld = SelectObject(hDC, CreatePen(PS_DOT, 1, GetSysColor(COLOR_HIGHLIGHT)));
        SelectObject(hDC, GetStockBrush(NULL_BRUSH));
        Rectangle(hDC, rcInner1.left, rcInner1.top, rcInner1.right, rcInner1.bottom);
        DeleteObject(SelectObject(hDC, hPenOld));

        RECT rcGrip;

        HBRUSH hbr = GetSysColorBrush(COLOR_HIGHLIGHT);
        SetRect(&rcGrip, RECT0);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT1);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT2);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT3);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT4);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT5);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT6);
        FillRect(hDC, &rcGrip, hbr);
        SetRect(&rcGrip, RECT7);
        FillRect(hDC, &rcGrip, hbr);
    }

    void DoSetRgn(HWND hwnd)
    {
        HRGN hRgnOld = m_hRgn;
        m_hRgn = DoGetRgn(hwnd);
        SetWindowRgn(hwnd, m_hRgn, TRUE);
        DeleteObject(hRgnOld);
    }

    BOOL SetPos(int i1, int j1, int i2, int j2)
    {
        if (i1 == -1 || j1 == -1 || i2 == -1 || j2 == -1) {
            return FALSE;
        }
        if (i1 >= i2)
            i2 = i1 + 1;
        if (j1 >= j2)
            j2 = j1 + 1;
        if (i1 != m_i1 || j1 != m_j1 || i2 != m_i2 || j2 != m_j2) {
            m_i1 = i1;
            m_j1 = j1;
            m_i2 = i2;
            m_j2 = j2;
            XG_FILE_MODIFIED(TRUE);
            return TRUE;
        }
        return FALSE;
    }

    BOOL Bound()
    {
        MRect rc;

        XgGetCellPosition(rc, m_i1, m_j1, m_i2, m_j2, TRUE);

        RECT rcWnd;
        GetWindowRect(m_hWnd, &rcWnd);

        if (EqualRect(&rc, &rcWnd))
            return FALSE;

        InflateRect(&rc, CXY_GRIP, CXY_GRIP);

        ::SetWindowPos(m_hWnd, nullptr, rc.left, rc.top, rc.Width(), rc.Height(),
                       SWP_DRAWFRAME | SWP_NOACTIVATE | SWP_NOCOPYBITS |
                       SWP_NOOWNERZORDER | SWP_NOREPOSITION | SWP_NOZORDER);

        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, nullptr);
        return TRUE;
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_hwndParent = GetParent(hwnd);

        GetWindowRect(hwnd, &m_rcOld);
        DoSetRgn(hwnd);
        Bound();
        return TRUE;
    }

    BOOL OnNCActivate(HWND hwnd, BOOL fActive, HWND hwndActDeact, BOOL fMinimized) noexcept
    {
        return FALSE;
    }

    void OnNCPaint(HWND hwnd, HRGN hrgn) noexcept
    {
        RECT rc;
        GetWindowRect(hwnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);

        HDC hDC = GetWindowDC(hwnd);
        DoDrawGrips(hwnd, hDC, rc);
        ReleaseDC(hwnd, hDC);
    }

    BOOL OnEraseBkgnd(HWND hwnd, HDC hdc) noexcept
    {
        return TRUE;
    }

    virtual void OnDraw(HWND hwnd, HDC hDC, const RECT& rc)
    {
        FillRect(hDC, &rc, GetStockBrush(WHITE_BRUSH));
        MoveToEx(hDC, rc.left, rc.top, nullptr);
        LineTo(hDC, rc.right, rc.bottom);
        MoveToEx(hDC, rc.right, rc.top, nullptr);
        LineTo(hDC, rc.left, rc.bottom);
    }

    void OnPaint(HWND hwnd)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);

        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hwnd, &ps);
        OnDraw(hwnd, hDC, rc);
        EndPaint(hwnd, &ps);
    }

    UINT OnNCCalcSize(HWND hwnd, BOOL fCalcValidRects, NCCALCSIZE_PARAMS * lpcsp) noexcept
    {
        lpcsp->rgrc[0] = lpcsp->rgrc[1];
        InflateRect(&lpcsp->rgrc[0], -CXY_GRIP, -CXY_GRIP);
        return WVR_REDRAW | WVR_VALIDRECTS;
    }

    UINT OnNCHitTest(HWND hwnd, int x, int y) noexcept
    {
        RECT rc, rcGrip;
        GetWindowRect(hwnd, &rc);

        POINT pt = { x, y };

        SetRect(&rcGrip, RECT0);
        if (PtInRect(&rcGrip, pt))
            return HTTOPLEFT;
        SetRect(&rcGrip, RECT1);
        if (PtInRect(&rcGrip, pt))
            return HTTOP;
        SetRect(&rcGrip, RECT2);
        if (PtInRect(&rcGrip, pt))
            return HTTOPRIGHT;
        SetRect(&rcGrip, RECT3);
        if (PtInRect(&rcGrip, pt))
            return HTLEFT;
        SetRect(&rcGrip, RECT4);
        if (PtInRect(&rcGrip, pt))
            return HTRIGHT;
        SetRect(&rcGrip, RECT5);
        if (PtInRect(&rcGrip, pt))
            return HTBOTTOMLEFT;
        SetRect(&rcGrip, RECT6);
        if (PtInRect(&rcGrip, pt))
            return HTBOTTOM;
        SetRect(&rcGrip, RECT7);
        if (PtInRect(&rcGrip, pt))
            return HTBOTTOMRIGHT;

        RECT rcInner1 = rc;
        InflateRect(&rcInner1, -CXY_GRIP / 2, -CXY_GRIP / 2);
        RECT rcInner2 = rcInner1;
        InflateRect(&rcInner2, -1, -1);
        if (PtInRect(&rcInner1, pt) && !PtInRect(&rcInner2, pt))
            return HTCAPTION;

        InflateRect(&rc, -CXY_GRIP, -CXY_GRIP);
        if (PtInRect(&rc, pt))
            return HTCAPTION;

        return HTTRANSPARENT;
    }

    void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept
    {
        lpMinMaxInfo->ptMinTrackSize.x = CXY_GRIP * 2;
        lpMinMaxInfo->ptMinTrackSize.y = CXY_GRIP * 2;
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        RECT rc = m_rcOld;
        MapWindowRect(nullptr, m_hwndParent, &rc);
        InvalidateRect(m_hwndParent, &rc, FALSE);

        DoSetRgn(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);

        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, nullptr);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        RECT rc = m_rcOld;
        MapWindowRect(nullptr, m_hwndParent, &rc);
        InvalidateRect(m_hwndParent, &rc, FALSE);

        DoSetRgn(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);

        ::KillTimer(m_hWnd, 999);
        ::SetTimer(m_hWnd, 999, 300, nullptr);
    }

    void OnTimer(HWND hwnd, UINT id) noexcept
    {
        if (id == 999)
        {
            KillTimer(hwnd, 999);

            PostMessageW(hwnd, XGWM_REDRAW, 0, 0);
        }
    }

    void DoRedraw(HWND hwnd) noexcept
    {
        MRect rc = m_rcOld;
        MapWindowRect(nullptr, m_hwndParent, &rc);
        InvalidateRect(m_hwndParent, &rc, TRUE);
        InvalidateRect(hwnd, nullptr, TRUE);

        GetWindowRect(hwnd, &m_rcOld);
        rc = m_rcOld;
        MapWindowRect(nullptr, m_hwndParent, &rc);
        SetWindowPos(hwnd, nullptr, rc.left, rc.top, rc.Width(), rc.Height(),
            SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOREPOSITION |
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_DEFERERASE);
    }

    BOOL CreateDx(HWND hwndParent)
    {
        constexpr auto style = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
        CreateWindowDx(hwndParent, nullptr, style);
        return m_hWnd != nullptr;
    }

    void OnNCRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest) noexcept
    {
        HMENU hMenu = LoadMenu(xg_hInstance, MAKEINTRESOURCEW(3));
        HMENU hSubMenu = GetSubMenu(hMenu, 1);

        POINT pt = { x, y };
        SetForegroundWindow(hwnd);
        constexpr auto flags = TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD;
        const auto id = static_cast<UINT>(TrackPopupMenu(hSubMenu, flags, pt.x, pt.y, 0, hwnd, nullptr));
        DestroyMenu(hMenu);
        s_hwndSelected = hwnd;
        if (id)
            PostMessage(m_hwndParent, WM_COMMAND, id, reinterpret_cast<LPARAM>(hwnd));
    }

    virtual BOOL Prop(HWND hwnd)
    {
        return TRUE;
    }

    void OnNCLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT codeHitTest)
    {
        if (fDoubleClick)
        {
            s_hwndSelected = hwnd;
            PostMessageW(m_hwndParent, WM_COMMAND, ID_BOXPROP, 0);
            return;
        }

        FORWARD_WM_NCLBUTTONDOWN(hwnd, fDoubleClick, x, y, codeHitTest, DefProcDx);
    }

    LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hwnd, WM_NCLBUTTONDBLCLK, OnNCLButtonDown);
        HANDLE_MSG(hwnd, WM_NCRBUTTONDOWN, OnNCRButtonDown);
        HANDLE_MSG(hwnd, WM_NCPAINT, OnNCPaint);
        HANDLE_MSG(hwnd, WM_NCCALCSIZE, OnNCCalcSize);
        HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
        HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        case WM_NCACTIVATE:
            return FALSE;
        case XGWM_REDRAW:
            DoRedraw(hwnd);
            break;
        case WM_CAPTURECHANGED:
        case WM_EXITSIZEMOVE:
            {
                RECT rc;
                GetWindowRect(hwnd, &rc);
                InflateRect(&rc, -CXY_GRIP, -CXY_GRIP);
                MapWindowRect(nullptr, m_hwndParent, &rc);
                auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                sa1->Get();
                int i1, j1, i2, j2;
                XgSetCellPosition(rc.left, rc.top, i1, j1, FALSE);
                XgSetCellPosition(rc.right, rc.bottom, i2, j2, TRUE);
                if (SetPos(i1, j1, i2, j2)) {
                    auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                    sa2->Get();
                    xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定。
                }
                // ボックスの位置を更新。
                PostMessage(m_hwndParent, WM_COMMAND, ID_MOVEBOXES, 0);
                // 表示を更新。
                ::KillTimer(m_hWnd, 999);
                ::SetTimer(m_hWnd, 999, 300, nullptr);
            }
            break;
        default:
            return DefProcDx(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    std::wstring GetPosText() const {
        WCHAR szText[MAX_PATH];
        StringCchPrintfW(szText, _countof(szText),
                         L"(%d, %d) - (%d, %d)", m_j1 + 1, m_i1 + 1, m_j2, m_i2);
        return szText;
    }
    BOOL SetPosText(const std::wstring& str) {
        int i1, j1, i2, j2;
        if (swscanf(str.c_str(), L"(%d, %d) - (%d, %d)", &j1, &i1, &j2, &i2) != 4)
            return FALSE;
        SetPos(i1 - 1, j1 - 1, i2, j2);
        return TRUE;
    }
    virtual std::wstring GetText() const {
        return m_strText;
    }
    virtual BOOL SetText(const std::wstring& str) {
        m_strText = str;
        return TRUE;
    }

    typedef std::map<std::wstring, std::wstring> map_t;

    virtual BOOL ReadMap(const map_t& map) {
        auto type_it = map.find(L"type");
        auto pos_it = map.find(L"pos");
        auto text_it = map.find(L"text");
        if (type_it == map.end() || pos_it == map.end() || text_it == map.end())
            return FALSE;

        if (m_type != type_it->second)
            return FALSE;

        auto pos = pos_it->second;
        auto text = text_it->second;
        xg_str_trim(pos);
        xg_str_trim(text);
        return SetPosText(pos) && SetText(text);
    }
    virtual BOOL WriteMap(map_t& map) {
        map[L"type"] = m_type;
        map[L"pos"] = GetPosText();
        map[L"text"] = GetText();
        return TRUE;
    }
    BOOL ReadLineEx(const std::wstring& line) {
        if (line.find(L"Box:") != 0)
            return FALSE;

        auto str = line.substr(4);
        xg_str_trim(str);
        if (str.find(L"{{") != 0)
            return FALSE;

        map_t map;
        while (str.size())
        {
            const auto index1 = str.find(L"{{");
            const auto index2 = str.find(L"}}", index1);
            if (index1 == str.npos || index2 == str.npos)
                break;

            auto contents = str.substr(index1 + 2, index2 - index1 - 2);
            const size_t index3 = contents.find(L':');
            if (index3 == contents.npos)
                break;

            auto tag = contents.substr(0, index3);
            auto value = contents.substr(index3 + 1);
            xg_str_trim(tag);
            xg_str_trim(value);
            map[tag] = xg_str_unquote(value);

            str = str.substr(index2 + 2);
            xg_str_trim(str);
        }

        return ReadMap(map);
    }
    BOOL ReadLine(const std::wstring& line) {
        if (line.find(L"Box:") != 0)
            return FALSE;

        auto str = line.substr(4);
        xg_str_trim(str);
        if (str.find(L"{{") == 0)
            return ReadLineEx(line);

        const auto index1 = str.find(L":");
        const auto index2 = str.find(L":", index1 + 1);
        if (index1 == str.npos || index2 == str.npos)
            return FALSE;

        auto type = str.substr(0, index1);
        xg_str_trim(type);
        assert(m_type == type);

        auto pos = str.substr(index1 + 1, index2 - index1 - 1);
        xg_str_trim(pos);
        if (!SetPosText(pos))
            return FALSE;

        auto text = str.substr(index2 + 1);
        xg_str_trim(text);
        return SetText(text);
    }
    BOOL WriteLine(FILE *fout)
    {
        map_t map;
        WriteMap(map);

        fprintf(fout, "Box: ");
        bool first = true;
        for (auto& pair : map) {
            if (!first)
                fprintf(fout, ", ");
            auto quoted = xg_str_quote(pair.second);
            fprintf(fout, "{{%s: %s}}",
                XgUnicodeToUtf8(pair.first.c_str()).c_str(),
                XgUnicodeToUtf8(quoted.c_str()).c_str());
            first = false;
        }
        fprintf(fout, "\n");
        return TRUE;
    }
    BOOL ReadJson(const json& box)
    {
        map_t map;
        for (auto& pair : box.items()) {
            map[XgUtf8ToUnicode(pair.key())] = XgUtf8ToUnicode(pair.value());
        }
        return ReadMap(map);
    }
    BOOL WriteJson(json& j)
    {
        map_t map;
        if (!WriteMap(map))
            return FALSE;
        json info;
        for (auto& pair : map) {
            info[XgUnicodeToUtf8(pair.first)] = XgUnicodeToUtf8(pair.second);
        }
        j["boxes"].push_back(info);
        return TRUE;
    }
};

// ボックス。
typedef std::vector<std::shared_ptr<XG_BoxWindow> > boxes_t;
extern boxes_t xg_boxes;

class XG_PictureBoxWindow : public XG_BoxWindow
{
public:
    XG_PictureBoxWindow(int i1 = 0, int j1 = 0, int i2 = 1, int j2 = 1)
        : XG_BoxWindow(L"pic", i1, j1, i2, j2)
    {
    }

    void OnDraw(HWND hwnd, HDC hDC, const RECT& rc) override
    {
        if (XgGetFileManager()->m_path2contents[m_strText].empty())
            XgGetFileManager()->load_image(m_strText.c_str());

        auto hbm = XgGetFileManager()->m_path2hbm[m_strText];
        auto hEMF = XgGetFileManager()->m_path2hemf[m_strText];

        if (hbm) {
            BITMAP bm;
            GetObject(hbm, sizeof(bm), &bm);
            if (HDC hMemDC = CreateCompatibleDC(nullptr)) {
                HGDIOBJ hbmOld = SelectObject(hMemDC, hbm);
                StretchBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                    hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
                SelectObject(hMemDC, hbmOld);
                DeleteDC(hMemDC);
            }
            return;
        }

        if (hEMF) {
            PlayEnhMetaFile(hDC, hEMF, &rc);
            return;
        }

        XG_BoxWindow::OnDraw(hwnd, hDC, rc);
    }

    BOOL Prop(HWND hwnd) override
    {
        XG_PictureBoxDialog dialog;
        dialog.m_strFile = m_strText;
        if (dialog.DoModal(m_hwndParent) == IDOK) {
            SetText(dialog.m_strFile);
            // ファイルが変更された。
            xg_bFileModified = TRUE;
            // 再描画。
            InvalidateRect(hwnd, nullptr, TRUE);
            return TRUE;
        }
        return FALSE;
    }
};

class XG_TextBoxWindow : public XG_BoxWindow
{
public:
    COLORREF m_rgbText = CLR_INVALID;
    COLORREF m_rgbBg = CLR_INVALID;
    std::wstring m_strFontName;
    int m_nFontSizeInPoints = 0;

    XG_TextBoxWindow(int i1 = 0, int j1 = 0, int i2 = 1, int j2 = 1)
        : XG_BoxWindow(L"text", i1, j1, i2, j2)
    {
    }

    void SetTextColor(COLORREF clr) noexcept
    {
        m_rgbText = clr;
        InvalidateRect(m_hWnd, nullptr, TRUE);
    }
    void SetBgColor(COLORREF clr) noexcept
    {
        m_rgbBg = clr;
        InvalidateRect(m_hWnd, nullptr, TRUE);
    }

    void OnDraw(HWND hwnd, HDC hDC, const RECT& rc) override
    {
        MRect rcText = rc;

        // 背景を塗りつぶす。
        {
            HBRUSH hbr;
            if (m_rgbBg != CLR_INVALID)
                hbr = CreateSolidBrush(m_rgbBg);
            else
                hbr = CreateSolidBrush(xg_rgbWhiteCellColor);
            FillRect(hDC, &rcText, hbr);
            DeleteObject(hbr);
        }

        SetBkMode(hDC, TRANSPARENT);
        const COLORREF rgbText = (m_rgbText != CLR_INVALID) ? m_rgbText : xg_rgbBlackCellColor;
        ::SetTextColor(hDC, rgbText);

        // 枠線を描く。
        HPEN hPen = CreatePen(PS_SOLID, 1, rgbText);
        HGDIOBJ hPenOld = SelectObject(hDC, hPen);
        MoveToEx(hDC, rc.left, rc.top, nullptr);
        LineTo(hDC, rc.left, rc.bottom);
        LineTo(hDC, rc.right, rc.bottom);
        LineTo(hDC, rc.right, rc.top);
        LineTo(hDC, rc.left, rc.top);
        SelectObject(hDC, hPenOld);
        DeleteObject(hPen);

        // フォント名をセット。
        LOGFONTW lf;
        ZeroMemory(&lf, sizeof(lf));
        lf.lfCharSet = DEFAULT_CHARSET;
        if (m_strFontName.empty())
            m_strFontName = xg_szCellFont;
        else
            StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), m_strFontName.c_str());

        // 高さを計算する。
        const int nPointSize = (m_nFontSizeInPoints ? m_nFontSizeInPoints : XG_DEF_TEXTBOX_POINTSIZE);
        const int nPixelSize = MulDiv(nPointSize, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        lf.lfHeight = -nPixelSize * xg_nZoomRate / 100;

        // フォント作成・選択。
        HFONT hFont = CreateFontIndirectW(&lf);
        HGDIOBJ hFontOld = SelectObject(hDC, hFont);

        // 文字列の高さを計算する。
        MRect rcCalc = rcText;
        const int cxyMargin = 2 * xg_nZoomRate / 100;
        InflateRect(&rcCalc, -cxyMargin, -cxyMargin);
        UINT uFormat = DT_CENTER | DT_TOP | DT_NOPREFIX | DT_WORDBREAK;
        DrawTextW(hDC, m_strText.c_str(), -1, &rcCalc, uFormat | DT_CALCRECT);

        // 領域の中央に描画する。
        rcText.top = rc.top + (rcText.Height() - rcCalc.Height()) / 2;
        rcText.bottom = rcText.top + rcCalc.Height();
        uFormat = DT_CENTER | DT_TOP | DT_NOPREFIX | DT_WORDBREAK;
        DrawTextW(hDC, m_strText.c_str(), -1, &rcText, uFormat);

        // フォントの選択を解除、破棄。
        SelectObject(hDC, hFontOld);
        DeleteObject(hFont);
    }

    BOOL Prop(HWND hwnd) override
    {
        XG_TextBoxDialog dialog;

        dialog.m_strText = m_strText;

        if (m_rgbText != CLR_INVALID)
            dialog.SetTextColor(m_rgbText, TRUE);
        else
            dialog.SetTextColor(xg_rgbBlackCellColor, FALSE);

        if (m_rgbBg != CLR_INVALID)
            dialog.SetBgColor(m_rgbBg, TRUE);
        else
            dialog.SetBgColor(xg_rgbWhiteCellColor, FALSE);

        dialog.m_strFontName = m_strFontName;
        dialog.m_nFontSizeInPoints = m_nFontSizeInPoints;

        if (dialog.DoModal(m_hwndParent) == IDOK) {
            SetText(dialog.m_strText);

            if (dialog.m_bTextColor)
                m_rgbText = dialog.GetTextColor();
            else
                m_rgbText = CLR_INVALID;

            if (dialog.m_bBgColor)
                m_rgbBg = dialog.GetBgColor();
            else
                m_rgbBg = CLR_INVALID;

            m_strFontName = dialog.m_strFontName;
            m_nFontSizeInPoints = dialog.m_nFontSizeInPoints;

            // ファイルが変更された。
            xg_bFileModified = TRUE;
            // 再描画。
            InvalidateRect(hwnd, nullptr, TRUE);
            return TRUE;
        }
        return FALSE;
    }

    DWORD SwapRandB(DWORD value) const noexcept
    {
        const BYTE r = GetRValue(value);
        const BYTE g = GetGValue(value);
        const BYTE b = GetBValue(value);
        return RGB(b, g, r);
    }

    BOOL ReadMap(const map_t& map) override {
        auto color_it = map.find(L"color");
        if (color_it != map.end()) {
            const auto value = wcstoul(color_it->second.c_str(), nullptr, 16);
            m_rgbText = SwapRandB(value);
        }
        auto bgcolor_it = map.find(L"bgcolor");
        if (bgcolor_it != map.end()) {
            const auto value = wcstoul(bgcolor_it->second.c_str(), nullptr, 16);
            m_rgbBg = SwapRandB(value);
        }
        auto fontname_it = map.find(L"font_name");
        if (fontname_it != map.end()) {
            m_strFontName = fontname_it->second;
        } else {
            m_strFontName.clear();
        }
        auto fontsize_it = map.find(L"font_size");
        if (fontsize_it != map.end()) {
            m_nFontSizeInPoints = wcstoul(fontsize_it->second.c_str(), nullptr, 0);
        } else {
            m_nFontSizeInPoints = 0;
        }
        return XG_BoxWindow::ReadMap(map);
    }
    BOOL WriteMap(map_t& map) override {
        WCHAR szText[64];
        if (m_rgbText != CLR_INVALID) {
            StringCchPrintfW(szText, _countof(szText), L"%06X", SwapRandB(m_rgbText));
            map[L"color"] = szText;
        }
        if (m_rgbBg != CLR_INVALID) {
            StringCchPrintfW(szText, _countof(szText), L"%06X", SwapRandB(m_rgbBg));
            map[L"bgcolor"] = szText;
        }
        if (m_strFontName.size()) {
            map[L"font_name"] = m_strFontName;
        } else {
            map.erase(L"font_name");
        }
        if (m_nFontSizeInPoints) {
            map[L"font_size"] = std::to_wstring(m_nFontSizeInPoints);
        } else {
            map.erase(L"font_size");
        }
        return XG_BoxWindow::WriteMap(map);
    }
};
