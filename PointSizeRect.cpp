////////////////////////////////////////////////////////////////////////////
// PointSizeRect.cpp -- Win32 point, size and rectangle wrapper
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#include "XWordGiver.hpp"

#ifdef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE  /*empty*/
    #include "PointSizeRect_inl.hpp"
#endif

////////////////////////////////////////////////////////////////////////////

void MzcNormalizeRect(LPRECT prc)
{
    int nTemp;
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

#ifdef UNITTEST
    #include <cstdio>
    using namespace std;
    int main(void)
    {
        MPoint pt(2, 3);
        MSize siz(3, 4);
        MRect rc(pt, siz);
        printf("%d, %d\n", rc.TopLeft().x, rc.TopLeft().y);
        printf("%d, %d\n", rc.BottomRight().x, rc.BottomRight().y);
        printf("%d, %d\n", rc.Size().cx, rc.Size().cy);
        return 0;
    }
#endif
