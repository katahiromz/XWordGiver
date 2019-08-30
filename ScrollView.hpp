////////////////////////////////////////////////////////////////////////////
// ScrollView.hpp -- MZC3 scroll view
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef __MZC3_SCROLLVIEW__
#define __MZC3_SCROLLVIEW__

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

    // scroll bar control
    HWND GetHScrollBarCtrl();
    void SetHScrollBarCtrl(HWND hScrollBar);
    HWND GetVScrollBarCtrl();
    void SetVScrollBarCtrl(HWND hScrollBar);

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

    // scroll pos
          MPoint& ScrollPos();
    const MPoint& ScrollPos() const;
    void OffsetScrollPos(int dx, int dy);
    void OffsetScrollPos(const MPoint& pt);
    void ResetScrollPos();

    // extent
          MSize& Extent();
    const MSize& Extent() const;
    void SetExtentForAllCtrls();

    // ensure visible
    void EnsureCtrlVisible(HWND hwndCtrl, bool update_all = true);

    // update
    void UpdateScrollInfo();
    void UpdateCtrlsPos();
    void UpdateAll();

    // NOTE: Call MScrollView::HScroll on parent's WM_HSCROLL.
    void HScroll(int nSB_, int nPos);
    // NOTE: Call MScrollView::VScroll on parent's WM_VSCROLL.
    void VScroll(int nSB_, int nPos);

          MScrollCtrlInfo& operator[](size_t index);
    const MScrollCtrlInfo& operator[](size_t index) const;

    // scroll info
    BOOL GetHScrollInfo(LPSCROLLINFO psi) const;
    BOOL GetVScrollInfo(LPSCROLLINFO psi) const;
    BOOL SetHScrollInfo(const SCROLLINFO *psi, BOOL bRedraw = TRUE);
    BOOL SetVScrollInfo(const SCROLLINFO *psi, BOOL bRedraw = TRUE);

    // scroll position
    int  GetHScrollPos() const;
    void SetHScrollPos(int nPos, BOOL bRedraw = TRUE);
    int  GetVScrollPos() const;
    void SetVScrollPos(int nPos, BOOL bRedraw = TRUE);

    virtual void GetClientRect(LPRECT prcClient) const;

protected:
    HWND        m_hwndParent;
    MPoint      m_ptScrollPos;
    MSize       m_sizExtent;
    std::vector<MScrollCtrlInfo> m_vecInfo;
    HWND        m_hHScrollBar;
    HWND        m_hVScrollBar;

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
    #include "ScrollView_inl.hpp"
#endif

#endif  // ndef __MZC3_SCROLLVIEW__
