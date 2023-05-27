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
    void SetCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl);
    void SetCtrlInfo(HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void RemoveCtrlInfo(HWND hwndCtrl) noexcept;
    void AddCtrlInfo(UINT idCtrl);
    void AddCtrlInfo(UINT idCtrl, const MRect& rcCtrl);
    void AddCtrlInfo(UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
    void SetCtrlInfo(UINT idCtrl, const MRect& rcCtrl);
    void SetCtrlInfo(UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl);
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
    void Scroll(int bar, int nSB_, int pos) noexcept;
    int GetNextPos(int bar, int nSB_, int pos) const noexcept;

          MScrollCtrlInfo& operator[](size_t index) noexcept;
    const MScrollCtrlInfo& operator[](size_t index) const noexcept;

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

inline MScrollCtrlInfo::MScrollCtrlInfo() noexcept : m_hwndCtrl(nullptr)
{
}

inline MScrollCtrlInfo::MScrollCtrlInfo(HWND hwndCtrl) noexcept :
    m_hwndCtrl(hwndCtrl)
{
}

inline
MScrollCtrlInfo::MScrollCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl) noexcept :
    m_hwndCtrl(hwndCtrl), m_rcCtrl(rcCtrl)
{
}

inline MScrollCtrlInfo::MScrollCtrlInfo(
    HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl) noexcept :
    m_hwndCtrl(hwndCtrl), m_rcCtrl(ptCtrl, sizCtrl)
{
}

inline MScrollView::MScrollView() noexcept :
    m_hwndParent(nullptr)
{
}

inline MScrollView::MScrollView(HWND hwndParent) noexcept :
    m_hwndParent(nullptr)
{
    MScrollView::SetParent(hwndParent);
}

inline MScrollView::MScrollView(
    HWND hwndParent, HWND hHScrollBar, HWND hVScrollBar) noexcept :
    m_hwndParent(nullptr)
{
    UNREFERENCED_PARAMETER(hHScrollBar);
    UNREFERENCED_PARAMETER(hVScrollBar);
    MScrollView::SetParent(hwndParent);
}

inline /*virtual*/ MScrollView::~MScrollView() noexcept
{
}

inline HWND MScrollView::GetParent() const noexcept
{
    return m_hwndParent;
}

inline void MScrollView::SetParent(HWND hwndParent) noexcept
{
    assert(::IsWindow(hwndParent));
    m_hwndParent = hwndParent;
    DWORD style = ::GetWindowLong(m_hwndParent, GWL_STYLE);
    style |= WS_CLIPCHILDREN;
    ::SetWindowLong(m_hwndParent, GWL_STYLE, style);
}

inline
void MScrollView::ShowScrollBars(BOOL fHScroll, BOOL fVScroll) noexcept
{
    ::ShowScrollBar(m_hwndParent, SB_HORZ, fHScroll);
    ::ShowScrollBar(m_hwndParent, SB_VERT, fVScroll);
}

inline void MScrollView::AddCtrlInfo(HWND hwndCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    m_vecInfo.emplace_back(hwndCtrl);
}

inline void MScrollView::AddCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    assert(rcCtrl.left >= 0);
    assert(rcCtrl.top >= 0);
    m_vecInfo.emplace_back(hwndCtrl, rcCtrl);
}

inline void MScrollView::AddCtrlInfo(
    HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    assert(ptCtrl.x >= 0);
    assert(ptCtrl.y >= 0);
    m_vecInfo.emplace_back(hwndCtrl, ptCtrl, sizCtrl);
}

inline MSize& MScrollView::Extent() noexcept
{
    return m_sizExtent;
}

inline const MSize& MScrollView::Extent() const noexcept
{
    return m_sizExtent;
}

inline BOOL MScrollView::HasChildStyle(HWND hwnd) const noexcept
{
    return (::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD);
}

inline bool MScrollView::empty() const noexcept
{
    return m_vecInfo.empty();
}

inline void MScrollView::clear() noexcept
{
    m_vecInfo.clear();
}

inline size_t MScrollView::size() const noexcept
{
    return m_vecInfo.size();
}

inline void MScrollView::UpdateAll() noexcept
{
    UpdateCtrlsPos();
}

inline void MScrollView::AddCtrlInfo(UINT idCtrl)
{
    AddCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl));
}

inline void MScrollView::AddCtrlInfo(UINT idCtrl, const MRect& rcCtrl)
{
    AddCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), rcCtrl);
}

inline void MScrollView::AddCtrlInfo(
    UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl)
{
    AddCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), ptCtrl, sizCtrl);
}

inline void MScrollView::SetCtrlInfo(UINT idCtrl, const MRect& rcCtrl)
{
    SetCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), rcCtrl);
}

inline void MScrollView::SetCtrlInfo(
    UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl)
{
    SetCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), ptCtrl, sizCtrl);
}

inline void MScrollView::RemoveCtrlInfo(UINT idCtrl) noexcept
{
    RemoveCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl));
}

inline MScrollCtrlInfo& MScrollView::operator[](size_t index) noexcept
{
    assert(index < size());
    return m_vecInfo[index];
}

inline const MScrollCtrlInfo& MScrollView::operator[](size_t index) const noexcept
{
    assert(index < size());
    return m_vecInfo[index];
}

#endif  // ndef __MZC3_SCROLLVIEW__
