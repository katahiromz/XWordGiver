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

    MScrollCtrlInfo() noexcept;
    MScrollCtrlInfo(HWND hwndCtrl) noexcept;
    MScrollCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl) noexcept;
    MScrollCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl) noexcept;
};

////////////////////////////////////////////////////////////////////////////
// MScrollView

class MScrollView
{
public:
    MScrollView() noexcept;
    MScrollView(HWND hwndParent) noexcept;
    MScrollView(HWND hwndParent, HWND hHScrollBar, HWND hVScrollBar) noexcept;
    virtual ~MScrollView() noexcept;

    // parent
    HWND GetParent() const noexcept;
    void SetParent(HWND hwndParent) noexcept;

    // parent scroll bars
    void ShowScrollBars(BOOL fHScroll, BOOL fVScroll) noexcept;

    // add/set control info
    void AddCtrlInfo(HWND hwndCtrl);
    void AddCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl);
    void AddCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void SetCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl) noexcept;
    void SetCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl) noexcept;
    void RemoveCtrlInfo(HWND hwndCtrl) noexcept;
    void AddCtrlInfo(UINT idCtrl);
    void AddCtrlInfo(UINT idCtrl, const MRect& rcCtrl);
    void AddCtrlInfo(UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void SetCtrlInfo(UINT idCtrl, const MRect& rcCtrl) noexcept;
    void SetCtrlInfo(UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl) noexcept;
    void RemoveCtrlInfo(UINT idCtrl) noexcept;

    bool empty() const noexcept;
    void clear() noexcept;
    size_t size() const noexcept;

    // find control info
          MScrollCtrlInfo* FindCtrlInfo(HWND hwndCtrl) noexcept;
    const MScrollCtrlInfo* FindCtrlInfo(HWND hwndCtrl) const noexcept;

    // extent
          MSize& Extent() noexcept;
    const MSize& Extent() const noexcept;
    void SetExtentForAllCtrls() noexcept;

    // ensure visible
    void EnsureCtrlVisible(HWND hwndCtrl, bool update_all = true) noexcept;

    // update
    void UpdateCtrlsPos() noexcept;
    void UpdateAll() noexcept;

    // NOTE: Call MScrollView::Scroll on parent's WM_HSCROLL/WM_VSCROLL.
    void Scroll(INT bar, INT nSB_, INT pos) noexcept;
    INT GetNextPos(INT bar, INT nSB_, INT pos) const noexcept;

          MScrollCtrlInfo& operator[](size_t index);
    const MScrollCtrlInfo& operator[](size_t index) const;

protected:
    HWND        m_hwndParent;
    MSize       m_sizExtent;
    std::vector<MScrollCtrlInfo> m_vecInfo;

    BOOL HasChildStyle(HWND hwnd) const noexcept;

private:
    // NOTE: MScrollView is not copyable.
    MScrollView(const MScrollView&) = delete;
    MScrollView& operator=(const MScrollView&) = delete;
};

////////////////////////////////////////////////////////////////////////////

#ifndef MZC_NO_INLINING
    #undef MZC_INLINE
    #define MZC_INLINE inline
    #include "MScrollView_inl.hpp"
#endif

#endif  // ndef __MZC3_SCROLLVIEW__
