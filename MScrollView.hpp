////////////////////////////////////////////////////////////////////////////
// MScrollView.hpp -- MZC3 scroll view
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef __MZC3_SCROLLVIEW__
#define __MZC3_SCROLLVIEW__

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <windowsx.h>
#include <vector>
#include "MPointSizeRect.hpp"

////////////////////////////////////////////////////////////////////////////
// MScrollCtrlInfo

struct MScrollCtrlInfo
{
    HWND    m_hwndCtrl;
    MRect   m_rcCtrl;

    MScrollCtrlInfo();
    MScrollCtrlInfo(HWND hwndCtrl);
    MScrollCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl);
    MScrollCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
};

////////////////////////////////////////////////////////////////////////////
// MScrollView

class MScrollView
{
public:
    MScrollView();
    MScrollView(HWND hwndParent);
    MScrollView(HWND hwndParent, HWND hHScrollBar, HWND hVScrollBar);
    virtual ~MScrollView();

    // parent
    HWND GetParent() const;
    void SetParent(HWND hwndParent);

    // parent scroll bars
    void ShowScrollBars(BOOL fHScroll, BOOL fVScroll);

    // add/set control info
    void AddCtrlInfo(HWND hwndCtrl);
    void AddCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl);
    void AddCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void SetCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl);
    void SetCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void RemoveCtrlInfo(HWND hwndCtrl);
    void AddCtrlInfo(UINT idCtrl);
    void AddCtrlInfo(UINT idCtrl, const MRect& rcCtrl);
    void AddCtrlInfo(UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void SetCtrlInfo(UINT idCtrl, const MRect& rcCtrl);
    void SetCtrlInfo(UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void RemoveCtrlInfo(UINT idCtrl);

    bool empty() const;
    void clear();
    size_t size() const;

    // find control info
          MScrollCtrlInfo* FindCtrlInfo(HWND hwndCtrl);
    const MScrollCtrlInfo* FindCtrlInfo(HWND hwndCtrl) const;

    // extent
          MSize& Extent();
    const MSize& Extent() const;
    void SetExtentForAllCtrls();

    // ensure visible
    void EnsureCtrlVisible(HWND hwndCtrl, bool update_all = true);

    // update
    void UpdateCtrlsPos();
    void UpdateAll();

    // NOTE: Call MScrollView::Scroll on parent's WM_HSCROLL/WM_VSCROLL.
    void Scroll(INT bar, INT nSB_, INT pos);
    INT GetNextPos(INT bar, INT nSB_, INT pos) const;

          MScrollCtrlInfo& operator[](size_t index);
    const MScrollCtrlInfo& operator[](size_t index) const;

protected:
    HWND        m_hwndParent;
    MSize       m_sizExtent;
    std::vector<MScrollCtrlInfo> m_vecInfo;

    BOOL HasChildStyle(HWND hwnd) const;

private:
    // NOTE: MScrollView is not copyable.
    MScrollView(const MScrollView&);
    MScrollView& operator=(const MScrollView&);
};

////////////////////////////////////////////////////////////////////////////

#ifndef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE inline
    #include "MScrollView_inl.hpp"
#endif

#endif  // ndef __MZC3_SCROLLVIEW__
