////////////////////////////////////////////////////////////////////////////
// PointSizeRect_inl.hpp -- Win32 point, size and rectangle wrapper
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

MZC_INLINE MPoint::MPoint()
    { x = y = 0; }

MZC_INLINE MPoint::MPoint(int x_, int y_)
    { x = x_; y = y_; }

MZC_INLINE MPoint::MPoint(POINT pt)
    { *reinterpret_cast<POINT *>(this) = pt; }

MZC_INLINE MPoint::MPoint(SIZE siz)
    { *reinterpret_cast<SIZE *>(this) = siz; }

MZC_INLINE MPoint::MPoint(DWORD dwPoint)
    { x = GET_X_LPARAM(dwPoint); y = GET_Y_LPARAM(dwPoint); }

MZC_INLINE void MPoint::Offset(int dx, int dy)
    { x += dx; y += dy; }

MZC_INLINE void MPoint::Offset(POINT pt)
    { x += pt.x; y += pt.y; }

MZC_INLINE void MPoint::Offset(SIZE siz)
    { x += siz.cx; y += siz.cy; }

MZC_INLINE MPoint::operator LPPOINT()
    { return reinterpret_cast<LPPOINT>(this); }

MZC_INLINE MPoint::operator const POINT *() const
    { return reinterpret_cast<const POINT *>(this); }

MZC_INLINE BOOL MPoint::operator==(POINT pt) const
    { return (x == pt.x && y == pt.y); }

MZC_INLINE BOOL MPoint::operator!=(POINT pt) const
    { return (x != pt.x || y != pt.y); }

MZC_INLINE void MPoint::operator+=(SIZE siz)
    { x += siz.cx; y += siz.cy; }

MZC_INLINE void MPoint::operator-=(SIZE siz)
    { x -= siz.cx; y -= siz.cy; }

MZC_INLINE void MPoint::operator+=(POINT pt)
    { x += pt.x; y += pt.y; }

MZC_INLINE void MPoint::operator-=(POINT pt)
    { x -= pt.x; y -= pt.y; }

MZC_INLINE void MPoint::SetPoint(int x_, int y_)
    { x = x_; y = y_; }

MZC_INLINE MPoint MPoint::operator+(SIZE siz) const
    { return MPoint(x + siz.cx, y + siz.cy); }

MZC_INLINE MPoint MPoint::operator-(SIZE siz) const
    { return MPoint(x - siz.cx, y - siz.cy); }

MZC_INLINE MPoint MPoint::operator-() const
    { return MPoint(-x, -y); }

MZC_INLINE MPoint MPoint::operator+(POINT pt) const
    { return MPoint(x + pt.x, y + pt.y); }

MZC_INLINE MSize MPoint::operator-(POINT pt) const
    { return MSize(x - pt.x, y - pt.y); }

MZC_INLINE MRect MPoint::operator+(LPCRECT prc) const
    { return MRect(prc) + *this; }

MZC_INLINE MRect MPoint::operator-(LPCRECT prc) const
    { return MRect(prc) - *this; }

////////////////////////////////////////////////////////////////////////////

MZC_INLINE MSize::MSize()
    { cx = cy = 0; }

MZC_INLINE MSize::MSize(int cx_, int cy_)
    { cx = cx_; cy = cy_; }

MZC_INLINE MSize::MSize(SIZE siz)
    { *reinterpret_cast<SIZE *>(this) = siz; }

MZC_INLINE MSize::MSize(POINT pt)
    { *reinterpret_cast<POINT *>(this) = pt; }

MZC_INLINE MSize::MSize(DWORD dwSize)
    { cx = GET_X_LPARAM(dwSize); cy = GET_Y_LPARAM(dwSize); }

MZC_INLINE MSize::operator LPSIZE()
    { return reinterpret_cast<LPSIZE>(this); }

MZC_INLINE MSize::operator const SIZE *() const
    { return reinterpret_cast<const SIZE *>(this); }

MZC_INLINE BOOL MSize::operator==(SIZE siz) const
    { return (cx == siz.cx && cy == siz.cy); }

MZC_INLINE BOOL MSize::operator!=(SIZE siz) const
    { return (cx != siz.cx || cy != siz.cy); }

MZC_INLINE void MSize::operator+=(SIZE siz)
    { cx += siz.cx; cy += siz.cy; }

MZC_INLINE void MSize::operator-=(SIZE siz)
    { cx -= siz.cx; cy -= siz.cy; }

MZC_INLINE void MSize::SetSize(int cx_, int cy_)
    { cx = cx_; cy = cy_; }

MZC_INLINE MSize MSize::operator+(SIZE siz) const
    { return MSize(cx + siz.cx, cy + siz.cy); }

MZC_INLINE MSize MSize::operator-(SIZE siz) const
    { return MSize(cx - siz.cx, cy - siz.cy); }

MZC_INLINE MSize MSize::operator-() const
    { return MSize(-cx, -cy); }

MZC_INLINE MPoint MSize::operator+(POINT pt) const
    { return MPoint(cx + pt.x, cy + pt.y); }

MZC_INLINE MPoint MSize::operator-(POINT pt) const
    { return MPoint(cx - pt.x, cy - pt.y); }

MZC_INLINE MRect MSize::operator+(LPCRECT prc) const
    { return MRect(prc) + *this; }

MZC_INLINE MRect MSize::operator-(LPCRECT prc) const
    { return MRect(prc) - *this; }

template <class Number>
MZC_INLINE MSize operator*(SIZE s, Number n)
    { return MSize((int)(s.cx * n), (int)(s.cy * n)); }

template <class Number>
MZC_INLINE void operator*=(SIZE & s, Number n)
    { s = s * n; }

template <class Number>
MZC_INLINE MSize operator/(SIZE s, Number n) 
    { return MSize((int)(s.cx / n), (int)(s.cy / n)); }

template <class Number>
MZC_INLINE void operator/=(SIZE & s, Number n)
    { s = s / n; }

////////////////////////////////////////////////////////////////////////////

MZC_INLINE MRect::MRect()
    { left = top = right = bottom = 0; }

MZC_INLINE MRect::MRect(int l, int t, int r, int b)
    { left = l; top = t; right = r; bottom = b; }

MZC_INLINE MRect::MRect(const RECT& rcSrc)
    { ::CopyRect(this, &rcSrc); }

MZC_INLINE MRect::MRect(LPCRECT lpSrcRect)
    { ::CopyRect(this, lpSrcRect); }

MZC_INLINE MRect::MRect(POINT pt, SIZE siz)
{
    right = (left = pt.x) + siz.cx;
    bottom = (top = pt.y) + siz.cy;
}

MZC_INLINE MRect::MRect(POINT topLeft, POINT bottomRight)
{
    left = topLeft.x;
    top = topLeft.y;
    right = bottomRight.x;
    bottom = bottomRight.y;
}

MZC_INLINE void MRect::InflateRect(LPCRECT prc)
{
    left -= prc->left;
    top -= prc->top;
    right += prc->right;
    bottom += prc->bottom;
}

MZC_INLINE void MRect::InflateRect(int l, int t, int r, int b)
{
    left -= l;
    top -= t;
    right += r;
    bottom += b;
}

MZC_INLINE void MRect::DeflateRect(LPCRECT prc)
{
    left += prc->left;
    top += prc->top;
    right -= prc->right;
    bottom -= prc->bottom;
}

MZC_INLINE void MRect::DeflateRect(int l, int t, int r, int b)
{
    left += l;
    top += t;
    right -= r;
    bottom -= b;
}

MZC_INLINE void MRect::NormalizeRect()
{
    int nTemp;
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

MZC_INLINE MRect MRect::operator+(POINT pt) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, pt.x, pt.y);
    return rc;
}

MZC_INLINE MRect MRect::operator-(POINT pt) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, -pt.x, -pt.y);
    return rc;
}

MZC_INLINE MRect MRect::operator+(LPCRECT prc) const
{
    MRect rc(this);
    rc.InflateRect(prc);
    return rc;
}

MZC_INLINE MRect MRect::operator+(SIZE siz) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, siz.cx, siz.cy);
    return rc;
}

MZC_INLINE MRect MRect::operator-(SIZE siz) const
{
    MRect rc(*this);
    ::OffsetRect(&rc, -siz.cx, -siz.cy);
    return rc;
}

MZC_INLINE MRect MRect::operator-(LPCRECT prc) const
{
    MRect rc(this);
    rc.DeflateRect(prc);
    return rc;
}

MZC_INLINE MRect MRect::operator&(const RECT& rc2) const
{
    MRect rc;
    ::IntersectRect(&rc, this, &rc2);
    return rc;
}

MZC_INLINE MRect MRect::operator|(const RECT& rc2) const
{
    MRect rc;
    ::UnionRect(&rc, this, &rc2);
    return rc;
}

MZC_INLINE MRect MRect::MulDiv(int nMultiplier, int nDivisor) const
{
    return MRect(
        ::MulDiv(left, nMultiplier, nDivisor),
        ::MulDiv(top, nMultiplier, nDivisor),
        ::MulDiv(right, nMultiplier, nDivisor),
        ::MulDiv(bottom, nMultiplier, nDivisor));
}

MZC_INLINE int MRect::Width() const
    { return right - left; }

MZC_INLINE int MRect::Height() const
    { return bottom - top; }

MZC_INLINE MSize MRect::Size() const
    { return MSize(right - left, bottom - top); }

MZC_INLINE MPoint& MRect::TopLeft()
    { return *(reinterpret_cast<MPoint *>(this)); }

MZC_INLINE MPoint& MRect::BottomRight()
    { return *(reinterpret_cast<MPoint *>(this) + 1); }

MZC_INLINE const MPoint& MRect::TopLeft() const
    { return *(reinterpret_cast<const MPoint *>(this)); }

MZC_INLINE const MPoint& MRect::BottomRight() const
    { return *(reinterpret_cast<const MPoint *>(this) + 1); }

MZC_INLINE MPoint MRect::CenterPoint() const
    { return MPoint((left + right) / 2, (top + bottom) / 2); }

MZC_INLINE MRect::operator LPRECT()
    { return this; }

MZC_INLINE MRect::operator LPCRECT() const
    { return this; }

MZC_INLINE BOOL MRect::IsRectEmpty() const
    { return ::IsRectEmpty(this); }

MZC_INLINE BOOL MRect::IsRectNull() const
    { return (left == 0 && right == 0 && top == 0 && bottom == 0); }

MZC_INLINE BOOL MRect::PtInRect(POINT pt) const
    { return ::PtInRect(this, pt); }

MZC_INLINE void MRect::SetRect(int x1, int y1, int x2, int y2)
    { ::SetRect(this, x1, y1, x2, y2); }

MZC_INLINE void MRect::SetRect(POINT topLeft, POINT bottomRight)
{
    ::SetRect(this, topLeft.x, topLeft.y, bottomRight.x, bottomRight.y);
}

MZC_INLINE void MRect::SetRectEmpty()
    { ::SetRectEmpty(this); }

MZC_INLINE void MRect::CopyRect(LPCRECT lpSrcRect)
    { ::CopyRect(this, lpSrcRect); }

MZC_INLINE BOOL MRect::EqualRect(LPCRECT prc) const
    { return ::EqualRect(this, prc); }

MZC_INLINE void MRect::InflateRect(int x, int y)
    { ::InflateRect(this, x, y); }

MZC_INLINE void MRect::InflateRect(SIZE siz)
    { ::InflateRect(this, siz.cx, siz.cy); }

MZC_INLINE void MRect::DeflateRect(int x, int y)
    { ::InflateRect(this, -x, -y); }

MZC_INLINE void MRect::DeflateRect(SIZE siz)
    { ::InflateRect(this, -siz.cx, -siz.cy); }

MZC_INLINE void MRect::OffsetRect(int x, int y)
    { ::OffsetRect(this, x, y); }

MZC_INLINE void MRect::OffsetRect(SIZE siz)
    { ::OffsetRect(this, siz.cx, siz.cy); }

MZC_INLINE void MRect::OffsetRect(POINT pt)
    { ::OffsetRect(this, pt.x, pt.y); }

MZC_INLINE void MRect::MoveToX(int x)
    { right = Width() + x; left = x; }

MZC_INLINE void MRect::MoveToY(int y)
    { bottom = Height() + y; top = y; }

MZC_INLINE void MRect::MoveToXY(int x, int y)
    { MoveToX(x); MoveToY(y); }

MZC_INLINE void MRect::MoveToXY(POINT pt)
    { MoveToX(pt.x); MoveToY(pt.y); }

MZC_INLINE BOOL MRect::IntersectRect(LPCRECT prc1, LPCRECT prc2)
    { return ::IntersectRect(this, prc1, prc2); }

MZC_INLINE BOOL MRect::UnionRect(LPCRECT prc1, LPCRECT prc2)
    { return ::UnionRect(this, prc1, prc2); }

MZC_INLINE BOOL MRect::SubtractRect(LPCRECT prcSrc1, LPCRECT prcSrc2)
    { return ::SubtractRect(this, prcSrc1, prcSrc2); }

MZC_INLINE void MRect::operator=(const RECT& rcSrc)
    { ::CopyRect(this, &rcSrc); }

MZC_INLINE BOOL MRect::operator==(const RECT& rc) const
    { return ::EqualRect(this, &rc); }

MZC_INLINE BOOL MRect::operator!=(const RECT& rc) const
    { return !::EqualRect(this, &rc); }

MZC_INLINE void MRect::operator+=(POINT pt)
    { ::OffsetRect(this, pt.x, pt.y); }

MZC_INLINE void MRect::operator+=(SIZE siz)
    { ::OffsetRect(this, siz.cx, siz.cy); }

MZC_INLINE void MRect::operator+=(LPCRECT prc)
    { InflateRect(prc); }

MZC_INLINE void MRect::operator-=(POINT pt)
    { ::OffsetRect(this, -pt.x, -pt.y); }

MZC_INLINE void MRect::operator-=(SIZE siz)
    { ::OffsetRect(this, -siz.cx, -siz.cy); }

MZC_INLINE void MRect::operator-=(LPCRECT prc)
    { DeflateRect(prc); }

MZC_INLINE void MRect::operator&=(const RECT& rc)
    { ::IntersectRect(this, this, &rc); }

MZC_INLINE void MRect::operator|=(const RECT& rc)
    { ::UnionRect(this, this, &rc); }
