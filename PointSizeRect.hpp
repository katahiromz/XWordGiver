////////////////////////////////////////////////////////////////////////////
// PointSizeRect.hpp -- Win32 point, size and rectangle wrapper
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef __MZC3_POINTSIZERECT__
#define __MZC3_POINTSIZERECT__

////////////////////////////////////////////////////////////////////////////

class MPoint;
class MSize;
class MRect;

////////////////////////////////////////////////////////////////////////////

class MPoint : public POINT
{
public:
    MPoint();
    MPoint(int x_, int y_);
    MPoint(POINT pt);
    MPoint(SIZE siz);
    MPoint(DWORD dwPoint);
    void    Offset(int dx, int dy);
    void    Offset(POINT pt);
    void    Offset(SIZE siz);
    operator LPPOINT();
    operator const POINT *() const;
    BOOL    operator==(POINT pt) const;
    BOOL    operator!=(POINT pt) const;
    void    operator+=(SIZE siz);
    void    operator-=(SIZE siz);
    void    operator+=(POINT pt);
    void    operator-=(POINT pt);
    void    SetPoint(int x_, int y_);
    MPoint  operator+(SIZE siz) const;
    MPoint  operator-(SIZE siz) const;
    MPoint  operator-() const;
    MPoint  operator+(POINT pt) const;
    MSize   operator-(POINT pt) const;
    MRect   operator+(LPCRECT prc) const;
    MRect   operator-(LPCRECT prc) const;
};

////////////////////////////////////////////////////////////////////////////

class MSize : public SIZE
{
public:
    MSize();
    MSize(int cx_, int cy_);
    MSize(SIZE siz);
    MSize(POINT pt);
    MSize(DWORD dwSize);
    operator LPSIZE();
    operator const SIZE *() const;
    BOOL    operator==(SIZE siz) const;
    BOOL    operator!=(SIZE siz) const;
    void    operator+=(SIZE siz);
    void    operator-=(SIZE siz);
    void    SetSize(int cx_, int cy_);
    MSize   operator+(SIZE siz) const;
    MSize   operator-(SIZE siz) const;
    MSize   operator-() const;
    MPoint  operator+(POINT pt) const;
    MPoint  operator-(POINT pt) const;
    MRect   operator+(LPCRECT prc) const;
    MRect   operator-(LPCRECT prc) const;
};

////////////////////////////////////////////////////////////////////////////

class MRect : public RECT
{
public:
    MRect();
    MRect(int l, int t, int r, int b);
    MRect(const RECT& rcSrc);
    MRect(LPCRECT lpSrcRect);
    MRect(POINT pt, SIZE siz);
    MRect(POINT topLeft, POINT bottomRight);
    operator LPRECT();
    operator LPCRECT() const;

    int   Width() const;
    int   Height() const;
    MSize Size() const;

    MPoint&       TopLeft();
    const MPoint& TopLeft() const;
    MPoint&       BottomRight();
    const MPoint& BottomRight() const;
    MPoint        CenterPoint() const;

    BOOL IsRectEmpty() const;
    BOOL IsRectNull() const;
    BOOL PtInRect(POINT pt) const;
    void SetRect(int x1, int y1, int x2, int y2);
    void SetRect(POINT topLeft, POINT bottomRight);
    void SetRectEmpty();
    void CopyRect(LPCRECT lpSrcRect);
    BOOL EqualRect(LPCRECT prc) const;
    void InflateRect(int x, int y);
    void InflateRect(SIZE siz);
    void InflateRect(LPCRECT prc);
    void InflateRect(int l, int t, int r, int b);
    void DeflateRect(int x, int y);
    void DeflateRect(SIZE siz);
    void DeflateRect(LPCRECT prc);
    void DeflateRect(int l, int t, int r, int b);
    void OffsetRect(int x, int y);
    void OffsetRect(SIZE siz);
    void OffsetRect(POINT pt);
    void NormalizeRect();
    void MoveToX(int x);
    void MoveToY(int y);
    void MoveToXY(int x, int y);
    void MoveToXY(POINT pt);
    BOOL IntersectRect(LPCRECT prc1, LPCRECT prc2);
    BOOL UnionRect(LPCRECT prc1, LPCRECT prc2);
    BOOL SubtractRect(LPCRECT prcSrc1, LPCRECT prcSrc2);

    void operator=(const RECT& rcSrc);
    BOOL operator==(const RECT& rc) const;
    BOOL operator!=(const RECT& rc) const;
    void operator+=(POINT pt);
    void operator+=(SIZE siz);
    void operator+=(LPCRECT prc);
    void operator-=(POINT pt);
    void operator-=(SIZE siz);
    void operator-=(LPCRECT prc);
    void operator&=(const RECT& rc);
    void operator|=(const RECT& rc);
    MRect operator+(POINT pt) const;
    MRect operator-(POINT pt) const;
    MRect operator+(LPCRECT prc) const;
    MRect operator+(SIZE siz) const;
    MRect operator-(SIZE siz) const;
    MRect operator-(LPCRECT prc) const;
    MRect operator&(const RECT& rc2) const;
    MRect operator|(const RECT& rc2) const;
    MRect MulDiv(int nMultiplier, int nDivisor) const;
};

void MzcNormalizeRect(LPRECT prc);

////////////////////////////////////////////////////////////////////////////

#ifndef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE inline
    #include "PointSizeRect_inl.hpp"
#endif

#endif  // ndef __MZC3_POINTSIZERECT__
