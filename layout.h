#pragma once
#include <assert.h>

typedef struct LAYOUT_INFO {
    UINT m_nCtrlID;
    UINT m_uEdges; /* BF_* flags */
    HWND m_hwndCtrl;
    SIZE m_margin1;
    SIZE m_margin2;
} LAYOUT_INFO;

typedef struct LAYOUT_DATA {
    HWND m_hwndParent;
    HWND m_hwndGrip;
    LAYOUT_INFO *m_pLayouts;
    UINT m_cLayouts;
} LAYOUT_DATA;

inline void
_layout_ModifySystemMenu(LAYOUT_DATA *pData, BOOL bEnableResize) noexcept
{
    if (bEnableResize)
    {
        GetSystemMenu(pData->m_hwndParent, TRUE); /* revert */
    }
    else
    {
        HMENU hSysMenu = GetSystemMenu(pData->m_hwndParent, FALSE);
        RemoveMenu(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);
        RemoveMenu(hSysMenu, SC_SIZE, MF_BYCOMMAND);
        RemoveMenu(hSysMenu, SC_RESTORE, MF_BYCOMMAND);
    }
}

inline HDWP
_layout_MoveGrip(LAYOUT_DATA *pData, HDWP hDwp OPTIONAL) noexcept
{
    if (!IsWindowVisible(pData->m_hwndGrip))
        return hDwp;

    SIZE size = { GetSystemMetrics(SM_CXVSCROLL), GetSystemMetrics(SM_CYHSCROLL) };
    constexpr auto uFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER;
    RECT rcClient;
    GetClientRect(pData->m_hwndParent, &rcClient);

    if (hDwp)
    {
        hDwp = DeferWindowPos(hDwp, pData->m_hwndGrip, nullptr,
                              rcClient.right - size.cx, rcClient.bottom - size.cy,
                              size.cx, size.cy, uFlags);
    }
    else
    {
        SetWindowPos(pData->m_hwndGrip, nullptr,
                     rcClient.right - size.cx, rcClient.bottom - size.cy,
                     size.cx, size.cy, uFlags);
    }
    return hDwp;
}

inline void
LayoutShowGrip(LAYOUT_DATA *pData, BOOL bShow)
{
    if (!bShow)
    {
        ShowWindow(pData->m_hwndGrip, SW_HIDE);
        return;
    }

    if (pData->m_hwndGrip == nullptr)
    {
        constexpr auto style = WS_CHILD | WS_CLIPSIBLINGS | SBS_SIZEGRIP;
        pData->m_hwndGrip = CreateWindowExW(0, L"SCROLLBAR", nullptr, style,
                                            0, 0, 0, 0, pData->m_hwndParent,
                                            nullptr, GetModuleHandleW(nullptr), nullptr);
    }
    _layout_MoveGrip(pData, nullptr);
    ShowWindow(pData->m_hwndGrip, SW_SHOWNOACTIVATE);
}

inline void
_layout_GetPercents(LPRECT prcPercents, UINT uEdges) noexcept
{
    prcPercents->left = (uEdges & BF_LEFT) ? 0 : 100;
    prcPercents->right = (uEdges & BF_RIGHT) ? 100 : 0;
    prcPercents->top = (uEdges & BF_TOP) ? 0 : 100;
    prcPercents->bottom = (uEdges & BF_BOTTOM) ? 100 : 0;
}

inline HDWP
_layout_DoMoveItem(LAYOUT_DATA *pData, HDWP hDwp, const LAYOUT_INFO *pLayout,
                   const RECT *rcClient)
{
    RECT rcChild, NewRect, rcPercents;

    if (!GetWindowRect(pLayout->m_hwndCtrl, &rcChild))
        return hDwp;
    MapWindowPoints(nullptr, pData->m_hwndParent, reinterpret_cast<LPPOINT>(&rcChild), 2);

    LONG nWidth = rcClient->right - rcClient->left;
    LONG nHeight = rcClient->bottom - rcClient->top;

    _layout_GetPercents(&rcPercents, pLayout->m_uEdges);
    NewRect.left = pLayout->m_margin1.cx + nWidth * rcPercents.left / 100;
    NewRect.top = pLayout->m_margin1.cy + nHeight * rcPercents.top / 100;
    NewRect.right = pLayout->m_margin2.cx + nWidth * rcPercents.right / 100;
    NewRect.bottom = pLayout->m_margin2.cy + nHeight * rcPercents.bottom / 100;

    if (!EqualRect(&NewRect, &rcChild))
    {
        hDwp = DeferWindowPos(hDwp, pLayout->m_hwndCtrl, nullptr, NewRect.left, NewRect.top,
                              NewRect.right - NewRect.left, NewRect.bottom - NewRect.top,
                              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREPOSITION);
    }
    return hDwp;
}

inline void
_layout_ArrangeLayout(LAYOUT_DATA *pData)
{
    RECT rcClient;
    HDWP hDwp = BeginDeferWindowPos(pData->m_cLayouts + 1);
    if (hDwp == nullptr)
        return;

    GetClientRect(pData->m_hwndParent, &rcClient);

    for (UINT iItem = 0; iItem < pData->m_cLayouts; ++iItem)
        hDwp = _layout_DoMoveItem(pData, hDwp, &pData->m_pLayouts[iItem], &rcClient);

    hDwp = _layout_MoveGrip(pData, hDwp);
    EndDeferWindowPos(hDwp);

    /* STATIC controls need refreshing. */
    for (UINT iItem = 0; iItem < pData->m_cLayouts; ++iItem)
    {
        HWND hwndCtrl = pData->m_pLayouts[iItem].m_hwndCtrl;
        WCHAR szClass[8];
        GetClassNameW(hwndCtrl, szClass, _countof(szClass));
        if (lstrcmpiW(szClass, L"STATIC") == 0)
        {
            InvalidateRect(hwndCtrl, nullptr, TRUE);
        }
    }
}

inline void
_layout_InitLayouts(LAYOUT_DATA *pData)
{
    RECT rcClient, rcChild, rcPercents;

    GetClientRect(pData->m_hwndParent, &rcClient);
    LONG nWidth = rcClient.right - rcClient.left;
    LONG nHeight = rcClient.bottom - rcClient.top;

    for (UINT iItem = 0; iItem < pData->m_cLayouts; ++iItem)
    {
        LAYOUT_INFO *pInfo = &pData->m_pLayouts[iItem];
        if (pInfo->m_hwndCtrl == nullptr)
        {
            pInfo->m_hwndCtrl = GetDlgItem(pData->m_hwndParent, pInfo->m_nCtrlID);
            if (pInfo->m_hwndCtrl == nullptr)
                continue;
        }

        ::GetWindowRect(pInfo->m_hwndCtrl, &rcChild);
        ::MapWindowPoints(nullptr, pData->m_hwndParent, reinterpret_cast<LPPOINT>(&rcChild), 2);

        _layout_GetPercents(&rcPercents, pInfo->m_uEdges);
        pInfo->m_margin1.cx = rcChild.left - nWidth * rcPercents.left / 100;
        pInfo->m_margin1.cy = rcChild.top - nHeight * rcPercents.top / 100;
        pInfo->m_margin2.cx = rcChild.right - nWidth * rcPercents.right / 100;
        pInfo->m_margin2.cy = rcChild.bottom - nHeight * rcPercents.bottom / 100;
    }
}

/* NOTE: Please call LayoutUpdate on parent's WM_SIZE. */
inline void
LayoutUpdate(HWND ignored1, LAYOUT_DATA *pData, LPCVOID ignored2, UINT ignored3)
{
    UNREFERENCED_PARAMETER(ignored1);
    UNREFERENCED_PARAMETER(ignored2);
    UNREFERENCED_PARAMETER(ignored3);
    if (pData == nullptr)
        return;
    assert(IsWindow(pData->m_hwndParent));
    _layout_ArrangeLayout(pData);
}

inline void
LayoutEnableResize(LAYOUT_DATA *pData, BOOL bEnable)
{
    LayoutShowGrip(pData, bEnable);
    _layout_ModifySystemMenu(pData, bEnable);
}

inline LAYOUT_DATA *
LayoutInit(HWND hwndParent, const LAYOUT_INFO *pLayouts, int cLayouts)
{
    BOOL bShowGrip;
    SIZE_T cb;
    auto pData = static_cast<LAYOUT_DATA*>(HeapAlloc(GetProcessHeap(), 0, sizeof(LAYOUT_DATA)));
    if (pData == nullptr)
    {
        assert(0);
        return nullptr;
    }

    if (cLayouts < 0) /* NOTE: If cLayouts was negative, then don't show size grip */
    {
        cLayouts = -cLayouts;
        bShowGrip = FALSE;
    }
    else
    {
        bShowGrip = TRUE;
    }

    cb = cLayouts * sizeof(LAYOUT_INFO);
    pData->m_cLayouts = cLayouts;
    pData->m_pLayouts = static_cast<LAYOUT_INFO*>(HeapAlloc(GetProcessHeap(), 0, cb));
    if (pData->m_pLayouts == nullptr)
    {
        assert(0);
        HeapFree(GetProcessHeap(), 0, pData);
        return nullptr;
    }
    memcpy(pData->m_pLayouts, pLayouts, cb);

    /* NOTE: The parent window must have initially WS_SIZEBOX style. */
    assert(IsWindow(hwndParent));
    assert(GetWindowLongPtrW(hwndParent, GWL_STYLE) & WS_SIZEBOX);

    pData->m_hwndParent = hwndParent;

    pData->m_hwndGrip = nullptr;
    if (bShowGrip)
        LayoutShowGrip(pData, bShowGrip);

    _layout_InitLayouts(pData);
    return pData;
}

inline void
LayoutDestroy(LAYOUT_DATA *pData) noexcept
{
    if (!pData)
        return;
    HeapFree(GetProcessHeap(), 0, pData->m_pLayouts);
    HeapFree(GetProcessHeap(), 0, pData);
}
