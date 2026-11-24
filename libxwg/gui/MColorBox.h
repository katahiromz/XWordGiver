// MColorBox.h --- カラーボックス
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"

class MColorBox : public MWindowBase {
public:
    inline static COLORREF s_rgbColorTable[] = {
        RGB(0, 0, 0),
        RGB(0x33, 0x33, 0x33),
        RGB(0x66, 0x66, 0x66),
        RGB(0x99, 0x99, 0x99),
        RGB(0xCC, 0xCC, 0xCC),
        RGB(0xFF, 0xFF, 0xFF),
        RGB(0xFF, 0xFF, 0xCC),
        RGB(0xFF, 0xCC, 0xFF),
        RGB(0xFF, 0xCC, 0xCC),
        RGB(0xCC, 0xFF, 0xFF),
        RGB(0xCC, 0xFF, 0xCC),
        RGB(0xCC, 0xCC, 0xFF),
        RGB(0xCC, 0xCC, 0xCC),
        RGB(0, 0, 0xCC),
        RGB(0, 0xCC, 0),
        RGB(0xCC, 0, 0)
    };
    COLORREF m_rgbColor = RGB(255, 255, 255);

    MColorBox() noexcept { }

    COLORREF GetColor() const noexcept {
        return m_rgbColor;
    }

    void SetColor(COLORREF rgb) noexcept {
        m_rgbColor = rgb;
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void DoChooseColor() noexcept {
        CHOOSECOLORW cc = { sizeof(cc), GetParent(m_hwnd) };
        cc.rgbResult = GetColor();
        cc.lpCustColors = s_rgbColorTable;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (::ChooseColorW(&cc)) {
            SetColor(cc.rgbResult);
        }
    }

    virtual void OnOwnerDrawItem(const DRAWITEMSTRUCT *pdis) noexcept {
        if (pdis->hwndItem != m_hwnd || pdis->CtlType != ODT_BUTTON)
            return;

        const BOOL bSelected = !!(pdis->itemState & ODS_SELECTED);
        const BOOL bFocus = !!(pdis->itemState & ODS_FOCUS);
        RECT rcItem = pdis->rcItem;

        HDC hdc = pdis->hDC;
        ::DrawFrameControl(hdc, &rcItem, DFC_BUTTON,
            DFCS_BUTTONPUSH |
            (bSelected ? DFCS_PUSHED : 0));

        ::InflateRect(&rcItem, -4, -4);

        HBRUSH hbr = ::CreateSolidBrush(m_rgbColor);
        ::FillRect(hdc, &rcItem, hbr);
        ::DeleteObject(hbr);

        if (bFocus) {
            ::InflateRect(&rcItem, 2, 2);
            ::DrawFocusRect(hdc, &rcItem);
        }
    }
};
