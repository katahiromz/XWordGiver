////////////////////////////////////////////////////////////////////////////
// MScrollView_inl.hpp -- MZC3 scroll view
// This file is part of MZC3.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// MScrollCtrlInfo

MZC_INLINE MScrollCtrlInfo::MScrollCtrlInfo() noexcept
{
}

MZC_INLINE MScrollCtrlInfo::MScrollCtrlInfo(HWND hwndCtrl) noexcept :
    m_hwndCtrl(hwndCtrl)
{
}

MZC_INLINE
MScrollCtrlInfo::MScrollCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl) noexcept :
    m_hwndCtrl(hwndCtrl), m_rcCtrl(rcCtrl)
{
}

MZC_INLINE MScrollCtrlInfo::MScrollCtrlInfo(
    HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl) noexcept :
    m_hwndCtrl(hwndCtrl), m_rcCtrl(ptCtrl, sizCtrl)
{
}

////////////////////////////////////////////////////////////////////////////
// MScrollView

MZC_INLINE MScrollView::MScrollView() noexcept :
    m_hwndParent(nullptr)
{
}

MZC_INLINE MScrollView::MScrollView(HWND hwndParent) noexcept :
    m_hwndParent(nullptr)
{
    MScrollView::SetParent(hwndParent);
}

MZC_INLINE MScrollView::MScrollView(
    HWND hwndParent, HWND hHScrollBar, HWND hVScrollBar) noexcept :
    m_hwndParent(nullptr)
{
    UNREFERENCED_PARAMETER(hHScrollBar);
    UNREFERENCED_PARAMETER(hVScrollBar);
    MScrollView::SetParent(hwndParent);
}

MZC_INLINE /*virtual*/ MScrollView::~MScrollView() noexcept
{
}

MZC_INLINE HWND MScrollView::GetParent() const noexcept
{
    return m_hwndParent;
}

MZC_INLINE void MScrollView::SetParent(HWND hwndParent) noexcept
{
    assert(::IsWindow(hwndParent));
    m_hwndParent = hwndParent;
    DWORD style = ::GetWindowLong(m_hwndParent, GWL_STYLE);
    style |= WS_CLIPCHILDREN;
    ::SetWindowLong(m_hwndParent, GWL_STYLE, style);
}

MZC_INLINE
void MScrollView::ShowScrollBars(BOOL fHScroll, BOOL fVScroll) noexcept
{
    ::ShowScrollBar(m_hwndParent, SB_HORZ, fHScroll);
    ::ShowScrollBar(m_hwndParent, SB_VERT, fVScroll);
}

MZC_INLINE void MScrollView::AddCtrlInfo(HWND hwndCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    m_vecInfo.emplace_back(hwndCtrl);
}

MZC_INLINE void MScrollView::AddCtrlInfo(HWND hwndCtrl, const MRect& rcCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    assert(rcCtrl.left >= 0);
    assert(rcCtrl.top >= 0);
    m_vecInfo.emplace_back(hwndCtrl, rcCtrl);
}

MZC_INLINE void MScrollView::AddCtrlInfo(
    HWND hwndCtrl, const MPoint& ptCtrl, const MSize& sizCtrl)
{
    assert(::IsWindow(hwndCtrl));
    assert(HasChildStyle(hwndCtrl));
    assert(ptCtrl.x >= 0);
    assert(ptCtrl.y >= 0);
    m_vecInfo.emplace_back(hwndCtrl, ptCtrl, sizCtrl);
}

MZC_INLINE MSize& MScrollView::Extent() noexcept
{
    return m_sizExtent;
}

MZC_INLINE const MSize& MScrollView::Extent() const noexcept
{
    return m_sizExtent;
}

MZC_INLINE BOOL MScrollView::HasChildStyle(HWND hwnd) const noexcept
{
    return (::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD);
}

MZC_INLINE bool MScrollView::empty() const noexcept
{
    return m_vecInfo.empty();
}

MZC_INLINE void MScrollView::clear() noexcept
{
    m_vecInfo.clear();
}

MZC_INLINE size_t MScrollView::size() const noexcept
{
    return m_vecInfo.size();
}

MZC_INLINE void MScrollView::UpdateAll() noexcept
{
    UpdateCtrlsPos();
}

MZC_INLINE void MScrollView::AddCtrlInfo(UINT idCtrl)
{
    AddCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl));
}

MZC_INLINE void MScrollView::AddCtrlInfo(UINT idCtrl, const MRect& rcCtrl)
{
    AddCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), rcCtrl);
}

MZC_INLINE void MScrollView::AddCtrlInfo(
    UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl)
{
    AddCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), ptCtrl, sizCtrl);
}

MZC_INLINE void MScrollView::SetCtrlInfo(UINT idCtrl, const MRect& rcCtrl) noexcept
{
    SetCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), rcCtrl);
}

MZC_INLINE void MScrollView::SetCtrlInfo(
    UINT idCtrl, const MPoint& ptCtrl, const MSize& sizCtrl) noexcept
{
    SetCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl), ptCtrl, sizCtrl);
}

MZC_INLINE void MScrollView::RemoveCtrlInfo(UINT idCtrl) noexcept
{
    RemoveCtrlInfo(::GetDlgItem(m_hwndParent, idCtrl));
}

MZC_INLINE MScrollCtrlInfo& MScrollView::operator[](size_t index) noexcept
{
    assert(index < size());
    return m_vecInfo[index];
}

MZC_INLINE const MScrollCtrlInfo& MScrollView::operator[](size_t index) const noexcept
{
    assert(index < size());
    return m_vecInfo[index];
}

////////////////////////////////////////////////////////////////////////////
