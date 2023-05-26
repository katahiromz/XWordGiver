// MPointSizeRect.hpp -- Win32API point, size and rectangle     -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MPOINTSIZERECT_HPP_
#define MZC4_MPOINTSIZERECT_HPP_    5   /* Version 5 */

class MPoint;
class MSize;
class MRect;
//VOID NormalizeRectDx(LPRECT prc);

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#ifndef _INC_WINDOWSX
    #include <windowsx.h>   // macro API
#endif
#include <cassert>          // assert

////////////////////////////////////////////////////////////////////////////

inline VOID GetScreenRectDx(LPRECT prc) noexcept
{
#ifndef SM_XVIRTUALSCREEN
    #define SM_XVIRTUALSCREEN   76
    #define SM_YVIRTUALSCREEN   77
    #define SM_CXVIRTUALSCREEN  78
    #define SM_CYVIRTUALSCREEN  79
#endif
    const INT x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const INT y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    INT cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    INT cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (cx)
    {
        SetRect(prc, x, y, x + cx, y + cy);
    }
    else
    {
        cx = GetSystemMetrics(SM_CXSCREEN);
        cy = GetSystemMetrics(SM_CYSCREEN);
        SetRect(prc, 0, 0, cx, cy);
    }
}

////////////////////////////////////////////////////////////////////////////

class MPoint : public POINT
{
public:
    MPoint() noexcept;
    MPoint(INT x_, INT y_) noexcept;
    MPoint(POINT pt) noexcept;
    MPoint(SIZE siz) noexcept;
    MPoint(DWORD dwPoint) noexcept;
    VOID    Offset(INT dx, INT dy) noexcept;
    VOID    Offset(POINT pt) noexcept;
    VOID    Offset(SIZE siz) noexcept;
    operator LPPOINT() noexcept;
    operator const POINT *() const noexcept;
    BOOL    operator==(POINT pt) const noexcept;
    BOOL    operator!=(POINT pt) const noexcept;
    VOID    operator+=(SIZE siz) noexcept;
    VOID    operator-=(SIZE siz) noexcept;
    VOID    operator+=(POINT pt) noexcept;
    VOID    operator-=(POINT pt) noexcept;
    VOID    SetPoint(INT x_, INT y_) noexcept;
    MPoint  operator+(SIZE siz) const noexcept;
    MPoint  operator-(SIZE siz) const noexcept;
    MPoint  operator-() const noexcept;
    MPoint  operator+(POINT pt) const noexcept;
    MSize   operator-(POINT pt) const noexcept;
    MRect   operator+(LPCRECT prc) const noexcept;
    MRect   operator-(LPCRECT prc) const noexcept;
};

////////////////////////////////////////////////////////////////////////////

class MSize : public SIZE
{
public:
    MSize() noexcept;
    MSize(INT cx_, INT cy_) noexcept;
    MSize(SIZE siz) noexcept;
    MSize(POINT pt) noexcept;
    MSize(DWORD dwSize) noexcept;
    operator LPSIZE() noexcept;
    operator const SIZE *() const noexcept;
    BOOL    operator==(SIZE siz) const noexcept;
    BOOL    operator!=(SIZE siz) const noexcept;
    VOID    operator+=(SIZE siz) noexcept;
    VOID    operator-=(SIZE siz) noexcept;
    VOID    SetSize(INT cx_, INT cy_) noexcept;
    MSize   operator+(SIZE siz) const noexcept;
    MSize   operator-(SIZE siz) const noexcept;
    MSize   operator-() const noexcept;
    MPoint  operator+(POINT pt) const noexcept;
    MPoint  operator-(POINT pt) const noexcept;
    MRect   operator+(LPCRECT prc) const noexcept;
    MRect   operator-(LPCRECT prc) const noexcept;
};

////////////////////////////////////////////////////////////////////////////

class MRect : public RECT
{
public:
    MRect() noexcept;
    MRect(INT l, INT t, INT r, INT b) noexcept;
    MRect(const RECT& rcSrc) noexcept;
    MRect(LPCRECT lpSrcRect) noexcept;
    MRect(POINT pt, SIZE siz) noexcept;
    MRect(POINT topLeft, POINT bottomRight) noexcept;
    operator LPRECT() noexcept;
    operator LPCRECT() const noexcept;

    INT   Width() const noexcept;
    INT   Height() const noexcept;
    MSize Size() const noexcept;

    MPoint&       TopLeft() noexcept;
    const MPoint& TopLeft() const noexcept;
    MPoint&       BottomRight() noexcept;
    const MPoint& BottomRight() const noexcept;
    MPoint        CenterPoint() const noexcept;

    BOOL IsRectEmpty() const noexcept;
    BOOL IsRectNull() const noexcept;
    BOOL PtInRect(POINT pt) const noexcept;
    VOID SetRect(INT x1, INT y1, INT x2, INT y2) noexcept;
    VOID SetRect(POINT topLeft, POINT bottomRight) noexcept;
    VOID SetRectEmpty() noexcept;
    VOID CopyRect(LPCRECT lpSrcRect) noexcept;
    BOOL EqualRect(LPCRECT prc) const noexcept;
    VOID InflateRect(INT x, INT y) noexcept;
    VOID InflateRect(SIZE siz) noexcept;
    VOID InflateRect(LPCRECT prc) noexcept;
    VOID InflateRect(INT l, INT t, INT r, INT b) noexcept;
    VOID DeflateRect(INT x, INT y) noexcept;
    VOID DeflateRect(SIZE siz) noexcept;
    VOID DeflateRect(LPCRECT prc) noexcept;
    VOID DeflateRect(INT l, INT t, INT r, INT b) noexcept;
    VOID OffsetRect(INT x, INT y) noexcept;
    VOID OffsetRect(SIZE siz) noexcept;
    VOID OffsetRect(POINT pt) noexcept;
    VOID NormalizeRect() noexcept;
    VOID MoveToX(INT x) noexcept;
    VOID MoveToY(INT y) noexcept;
    VOID MoveToXY(INT x, INT y) noexcept;
    VOID MoveToXY(POINT pt) noexcept;
    BOOL IntersectRect(LPCRECT prc1, LPCRECT prc2) noexcept;
    BOOL UnionRect(LPCRECT prc1, LPCRECT prc2) noexcept;
    BOOL SubtractRect(LPCRECT prcSrc1, LPCRECT prcSrc2) noexcept;

    VOID operator=(const RECT& rcSrc) noexcept;
    BOOL operator==(const RECT& rc) const noexcept;
    BOOL operator!=(const RECT& rc) const noexcept;
    VOID operator+=(POINT pt) noexcept;
    VOID operator+=(SIZE siz) noexcept;
    VOID operator+=(LPCRECT prc) noexcept;
    VOID operator-=(POINT pt) noexcept;
    VOID operator-=(SIZE siz) noexcept;
    VOID operator-=(LPCRECT prc) noexcept;
    VOID operator&=(const RECT& rc) noexcept;
    VOID operator|=(const RECT& rc) noexcept;
    MRect operator+(POINT pt) const noexcept;
    MRect operator-(POINT pt) const noexcept;
    MRect operator+(LPCRECT prc) const noexcept;
    MRect operator+(SIZE siz) const noexcept;
    MRect operator-(SIZE siz) const noexcept;
    MRect operator-(LPCRECT prc) const noexcept;
    MRect operator&(const RECT& rc2) const noexcept;
    MRect operator|(const RECT& rc2) const noexcept;
    MRect MulDiv(INT nMultiplier, INT nDivisor) const noexcept;
};

VOID NormalizeRectDx(LPRECT prc) noexcept;

////////////////////////////////////////////////////////////////////////////

inline MPoint::MPoint() noexcept
    { x = y = 0; }

inline MPoint::MPoint(INT x_, INT y_) noexcept
    { x = x_; y = y_; }

inline MPoint::MPoint(POINT pt) noexcept
    { reinterpret_cast<POINT&>(*this) = pt; }

inline MPoint::MPoint(SIZE siz) noexcept
    { reinterpret_cast<SIZE&>(*this) = siz; }

inline MPoint::MPoint(DWORD dwPoint) noexcept
    { x = GET_X_LPARAM(dwPoint); y = GET_Y_LPARAM(dwPoint); }

inline VOID MPoint::Offset(INT dx, INT dy) noexcept
    { x += dx; y += dy; }

inline VOID MPoint::Offset(POINT pt) noexcept
    { x += pt.x; y += pt.y; }

inline VOID MPoint::Offset(SIZE siz) noexcept
    { x += siz.cx; y += siz.cy; }

inline MPoint::operator LPPOINT() noexcept
    { return reinterpret_cast<LPPOINT>(this); }

inline MPoint::operator const POINT *() const noexcept
    { return reinterpret_cast<const POINT *>(this); }

inline BOOL MPoint::operator==(POINT pt) const noexcept
    { return (x == pt.x && y == pt.y); }

inline BOOL MPoint::operator!=(POINT pt) const noexcept
    { return (x != pt.x || y != pt.y); }

inline VOID MPoint::operator+=(SIZE siz) noexcept
    { x += siz.cx; y += siz.cy; }

inline VOID MPoint::operator-=(SIZE siz) noexcept
    { x -= siz.cx; y -= siz.cy; }

inline VOID MPoint::operator+=(POINT pt) noexcept
    { x += pt.x; y += pt.y; }

inline VOID MPoint::operator-=(POINT pt) noexcept
    { x -= pt.x; y -= pt.y; }

inline VOID MPoint::SetPoint(INT x_, INT y_) noexcept
    { x = x_; y = y_; }

inline MPoint MPoint::operator+(SIZE siz) const noexcept
    { return MPoint(x + siz.cx, y + siz.cy); }

inline MPoint MPoint::operator-(SIZE siz) const noexcept
    { return MPoint(x - siz.cx, y - siz.cy); }

inline MPoint MPoint::operator-() const noexcept
    { return MPoint(-x, -y); }

inline MPoint MPoint::operator+(POINT pt) const noexcept
    { return MPoint(x + pt.x, y + pt.y); }

inline MSize MPoint::operator-(POINT pt) const noexcept
    { return MSize(x - pt.x, y - pt.y); }

inline MRect MPoint::operator+(LPCRECT prc) const noexcept
    { return MRect(prc) + *this; }

inline MRect MPoint::operator-(LPCRECT prc) const noexcept
    { return MRect(prc) - *this; }

////////////////////////////////////////////////////////////////////////////

inline MSize::MSize() noexcept
    { cx = cy = 0; }

inline MSize::MSize(INT cx_, INT cy_) noexcept
    { cx = cx_; cy = cy_; }

inline MSize::MSize(SIZE siz) noexcept
    { reinterpret_cast<SIZE&>(*this) = siz; }

inline MSize::MSize(POINT pt) noexcept
    { reinterpret_cast<POINT&>(*this) = pt; }

inline MSize::MSize(DWORD dwSize) noexcept
    { cx = GET_X_LPARAM(dwSize); cy = GET_Y_LPARAM(dwSize); }

inline MSize::operator LPSIZE() noexcept
    { return reinterpret_cast<LPSIZE>(this); }

inline MSize::operator const SIZE *() const noexcept
    { return reinterpret_cast<const SIZE *>(this); }

inline BOOL MSize::operator==(SIZE siz) const noexcept
    { return (cx == siz.cx && cy == siz.cy); }

inline BOOL MSize::operator!=(SIZE siz) const noexcept
    { return (cx != siz.cx || cy != siz.cy); }

inline VOID MSize::operator+=(SIZE siz) noexcept
    { cx += siz.cx; cy += siz.cy; }

inline VOID MSize::operator-=(SIZE siz) noexcept
    { cx -= siz.cx; cy -= siz.cy; }

inline VOID MSize::SetSize(INT cx_, INT cy_) noexcept
    { cx = cx_; cy = cy_; }

inline MSize MSize::operator+(SIZE siz) const noexcept
    { return MSize(cx + siz.cx, cy + siz.cy); }

inline MSize MSize::operator-(SIZE siz) const noexcept
    { return MSize(cx - siz.cx, cy - siz.cy); }

inline MSize MSize::operator-() const noexcept
    { return MSize(-cx, -cy); }

inline MPoint MSize::operator+(POINT pt) const noexcept
    { return MPoint(cx + pt.x, cy + pt.y); }

inline MPoint MSize::operator-(POINT pt) const noexcept
    { return MPoint(cx - pt.x, cy - pt.y); }

inline MRect MSize::operator+(LPCRECT prc) const noexcept
    { return MRect(prc) + *this; }

inline MRect MSize::operator-(LPCRECT prc) const noexcept
    { return MRect(prc) - *this; }

template <class Number>
inline MSize operator*(SIZE s, Number n) noexcept
    { return MSize((INT)(s.cx * n), (INT)(s.cy * n)); }

template <class Number>
inline VOID operator*=(SIZE & s, Number n) noexcept
    { s = s * n; }

template <class Number>
inline MSize operator/(SIZE s, Number n) noexcept
    { return MSize((INT)(s.cx / n), (INT)(s.cy / n)); }

template <class Number>
inline VOID operator/=(SIZE & s, Number n) noexcept
    { s = s / n; }

////////////////////////////////////////////////////////////////////////////

inline MRect::MRect() noexcept
    { left = top = right = bottom = 0; }

inline MRect::MRect(INT l, INT t, INT r, INT b) noexcept
    { left = l; top = t; right = r; bottom = b; }

inline MRect::MRect(const RECT& rcSrc) noexcept
    { ::CopyRect(this, &rcSrc); }

inline MRect::MRect(LPCRECT lpSrcRect) noexcept
    { ::CopyRect(this, lpSrcRect); }

inline MRect::MRect(POINT pt, SIZE siz) noexcept
{
    right = (left = pt.x) + siz.cx;
    bottom = (top = pt.y) + siz.cy;
}

inline MRect::MRect(POINT topLeft, POINT bottomRight) noexcept
{
    left = topLeft.x;
    top = topLeft.y;
    right = bottomRight.x;
    bottom = bottomRight.y;
}

inline VOID MRect::InflateRect(LPCRECT prc) noexcept
{
    left -= prc->left;
    top -= prc->top;
    right += prc->right;
    bottom += prc->bottom;
}

inline VOID MRect::InflateRect(INT l, INT t, INT r, INT b) noexcept
{
    left -= l;
    top -= t;
    right += r;
    bottom += b;
}

inline VOID MRect::DeflateRect(LPCRECT prc) noexcept
{
    left += prc->left;
    top += prc->top;
    right -= prc->right;
    bottom -= prc->bottom;
}

inline VOID MRect::DeflateRect(INT l, INT t, INT r, INT b) noexcept
{
    left += l;
    top += t;
    right -= r;
    bottom -= b;
}

inline VOID MRect::NormalizeRect() noexcept
{
    INT nTemp;
    if (left > right)
    {
        nTemp = left;
        left = right;
        right = nTemp;
    }
    if (top > bottom)
    {
        nTemp = top;
        top = bottom;
        bottom = nTemp;
    }
}

inline MRect MRect::operator+(POINT pt) const noexcept
{
    MRect rc(*this);
    ::OffsetRect(&rc, pt.x, pt.y);
    return rc;
}

inline MRect MRect::operator-(POINT pt) const noexcept
{
    MRect rc(*this);
    ::OffsetRect(&rc, -pt.x, -pt.y);
    return rc;
}

inline MRect MRect::operator+(LPCRECT prc) const noexcept
{
    MRect rc(this);
    rc.InflateRect(prc);
    return rc;
}

inline MRect MRect::operator+(SIZE siz) const noexcept
{
    MRect rc(*this);
    ::OffsetRect(&rc, siz.cx, siz.cy);
    return rc;
}

inline MRect MRect::operator-(SIZE siz) const noexcept
{
    MRect rc(*this);
    ::OffsetRect(&rc, -siz.cx, -siz.cy);
    return rc;
}

inline MRect MRect::operator-(LPCRECT prc) const noexcept
{
    MRect rc(this);
    rc.DeflateRect(prc);
    return rc;
}

inline MRect MRect::operator&(const RECT& rc2) const noexcept
{
    MRect rc;
    ::IntersectRect(&rc, this, &rc2);
    return rc;
}

inline MRect MRect::operator|(const RECT& rc2) const noexcept
{
    MRect rc;
    ::UnionRect(&rc, this, &rc2);
    return rc;
}

inline MRect MRect::MulDiv(INT nMultiplier, INT nDivisor) const noexcept
{
    return MRect(
        ::MulDiv(left, nMultiplier, nDivisor),
        ::MulDiv(top, nMultiplier, nDivisor),
        ::MulDiv(right, nMultiplier, nDivisor),
        ::MulDiv(bottom, nMultiplier, nDivisor));
}

inline INT MRect::Width() const noexcept
    { return right - left; }

inline INT MRect::Height() const noexcept
    { return bottom - top; }

inline MSize MRect::Size() const noexcept
    { return MSize(right - left, bottom - top); }

inline MPoint& MRect::TopLeft() noexcept
    { return *((MPoint*) this); }

inline MPoint& MRect::BottomRight() noexcept
    { return *((MPoint*) this + 1); }

inline const MPoint& MRect::TopLeft() const noexcept
    { return *((MPoint*) this); }

inline const MPoint& MRect::BottomRight() const noexcept
    { return *((MPoint*) this + 1); }

inline MPoint MRect::CenterPoint() const noexcept
    { return MPoint((left + right) / 2, (top + bottom) / 2); }

inline MRect::operator LPRECT() noexcept
    { return this; }

inline MRect::operator LPCRECT() const noexcept
    { return this; }

inline BOOL MRect::IsRectEmpty() const noexcept
    { return ::IsRectEmpty(this); }

inline BOOL MRect::IsRectNull() const noexcept
    { return (left == 0 && right == 0 && top == 0 && bottom == 0); }

inline BOOL MRect::PtInRect(POINT pt) const noexcept
    { return ::PtInRect(this, pt); }

inline VOID MRect::SetRect(INT x1, INT y1, INT x2, INT y2) noexcept
    { ::SetRect(this, x1, y1, x2, y2); }

inline VOID MRect::SetRect(POINT topLeft, POINT bottomRight) noexcept
{
    ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
}

inline VOID MRect::SetRectEmpty() noexcept
    { ::SetRectEmpty(this); }

inline VOID MRect::CopyRect(LPCRECT lpSrcRect) noexcept
    { ::CopyRect(this, lpSrcRect); }

inline BOOL MRect::EqualRect(LPCRECT prc) const noexcept
    { return ::EqualRect(this, prc); }

inline VOID MRect::InflateRect(INT x, INT y) noexcept
    { ::InflateRect(this, x, y); }

inline VOID MRect::InflateRect(SIZE siz) noexcept
    { ::InflateRect(this, siz.cx, siz.cy); }

inline VOID MRect::DeflateRect(INT x, INT y) noexcept
    { ::InflateRect(this, -x, -y); }

inline VOID MRect::DeflateRect(SIZE siz) noexcept
    { ::InflateRect(this, -siz.cx, -siz.cy); }

inline VOID MRect::OffsetRect(INT x, INT y) noexcept
    { ::OffsetRect(this, x, y); }

inline VOID MRect::OffsetRect(SIZE siz) noexcept
    { ::OffsetRect(this, siz.cx, siz.cy); }

inline VOID MRect::OffsetRect(POINT pt) noexcept
    { ::OffsetRect(this, pt.x, pt.y); }

inline VOID MRect::MoveToX(INT x) noexcept
    { right = Width() + x; left = x; }

inline VOID MRect::MoveToY(INT y) noexcept
    { bottom = Height() + y; top = y; }

inline VOID MRect::MoveToXY(INT x, INT y) noexcept
    { MoveToX(x); MoveToY(y); }

inline VOID MRect::MoveToXY(POINT pt) noexcept
    { MoveToX(pt.x); MoveToY(pt.y); }

inline BOOL MRect::IntersectRect(LPCRECT prc1, LPCRECT prc2) noexcept
    { return ::IntersectRect(this, prc1, prc2); }

inline BOOL MRect::UnionRect(LPCRECT prc1, LPCRECT prc2) noexcept
    { return ::UnionRect(this, prc1, prc2); }

inline BOOL MRect::SubtractRect(LPCRECT prcSrc1, LPCRECT prcSrc2) noexcept
    { return ::SubtractRect(this, prcSrc1, prcSrc2); }

inline VOID MRect::operator=(const RECT& rcSrc) noexcept
    { ::CopyRect(this, &rcSrc); }

inline BOOL MRect::operator==(const RECT& rc) const noexcept
    { return ::EqualRect(this, &rc); }

inline BOOL MRect::operator!=(const RECT& rc) const noexcept
    { return !::EqualRect(this, &rc); }

inline VOID MRect::operator+=(POINT pt) noexcept
    { ::OffsetRect(this, pt.x, pt.y); }

inline VOID MRect::operator+=(SIZE siz) noexcept
    { ::OffsetRect(this, siz.cx, siz.cy); }

inline VOID MRect::operator+=(LPCRECT prc) noexcept
    { InflateRect(prc); }

inline VOID MRect::operator-=(POINT pt) noexcept
    { ::OffsetRect(this, -pt.x, -pt.y); }

inline VOID MRect::operator-=(SIZE siz) noexcept
    { ::OffsetRect(this, -siz.cx, -siz.cy); }

inline VOID MRect::operator-=(LPCRECT prc) noexcept
    { DeflateRect(prc); }

inline VOID MRect::operator&=(const RECT& rc) noexcept
    { ::IntersectRect(this, this, &rc); }

inline VOID MRect::operator|=(const RECT& rc) noexcept
    { ::UnionRect(this, this, &rc); }

inline VOID NormalizeRectDx(LPRECT prc) noexcept
{
    INT nTemp;
    if (prc->left > prc->right)
    {
        nTemp = prc->left;
        prc->left = prc->right;
        prc->right = nTemp;
    }
    if (prc->top > prc->bottom)
    {
        nTemp = prc->top;
        prc->top = prc->bottom;
        prc->bottom = nTemp;
    }
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MPOINTSIZERECT_HPP_
