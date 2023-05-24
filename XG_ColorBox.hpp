#pragma once

#include "XG_Window.hpp"

class XG_ColorBox : public XG_Window
{
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
        RGB(0xCC, 0, 0),
    };

    COLORREF m_rgbColor = RGB(255, 255, 255);

    XG_ColorBox()
    {
    }

    COLORREF GetColor() const
    {
        return m_rgbColor;
    }

    void SetColor(COLORREF rgb)
    {
        m_rgbColor = rgb;
        InvalidateRect(m_hWnd, nullptr, TRUE);
    }

    void DoChooseColor()
    {
        CHOOSECOLORW cc = { sizeof(cc), GetParent(m_hWnd) };
        cc.rgbResult = GetColor();
        cc.lpCustColors = s_rgbColorTable;
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;
        if (::ChooseColorW(&cc))
        {
            SetColor(cc.rgbResult);
        }
    }

    virtual void OnOwnerDrawItem(const DRAWITEMSTRUCT *pdis)
    {
        if (pdis->hwndItem != m_hWnd || pdis->CtlType != ODT_BUTTON)
            return;

        BOOL bSelected = !!(pdis->itemState & ODS_SELECTED);
        BOOL bFocus = !!(pdis->itemState & ODS_FOCUS);
        RECT rcItem = pdis->rcItem;

        HDC hdc = pdis->hDC;
        ::DrawFrameControl(hdc, &rcItem, DFC_BUTTON,
            DFCS_BUTTONPUSH |
            (bSelected ? DFCS_PUSHED : 0));

        ::InflateRect(&rcItem, -4, -4);

        HBRUSH hbr = ::CreateSolidBrush(m_rgbColor);
        ::FillRect(hdc, &rcItem, hbr);
        ::DeleteObject(hbr);

        if (bFocus)
        {
            ::InflateRect(&rcItem, 2, 2);
            ::DrawFocusRect(hdc, &rcItem);
        }
    }
};
