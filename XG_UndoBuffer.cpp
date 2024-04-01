#include "XG_UndoBuffer.hpp"

//////////////////////////////////////////////////////////////////////////////

void XG_UndoData_SetAt::Apply() const {
    xg_xword.SetAt(pos, ch);
    xg_caret_pos = pos;
    XgUpdateCaretPos();
    XG_FILE_MODIFIED(TRUE);
}

void XG_UndoData_MarksUpdated::Get() {
    vMarks = xg_vMarks;
    strMarked = xg_strMarked;
}

void XG_UndoData_MarksUpdated::Apply() const {
    xg_vMarks = vMarks;
    xg_strMarked = strMarked;
    XG_FILE_MODIFIED(TRUE);
}

void XG_UndoData_HintsUpdated::Get() {
    vVertInfo = xg_vVertInfo;
    vHorzInfo = xg_vHorzInfo;
    vecVertHints = xg_vecVertHints;
    vecHorzHints = xg_vecHorzHints;
    bShowHints = xg_bShowClues;
}

void XG_UndoData_HintsUpdated::Apply() const {
    xg_vVertInfo = vVertInfo;
    xg_vHorzInfo = vHorzInfo;
    xg_vecVertHints = vecVertHints;
    xg_vecHorzHints = vecHorzHints;
    XgUpdateHints();
    if (bShowHints) {
        XgShowHints(xg_hMainWnd);
        xg_bShowClues = TRUE;
    } else {
        XgDestroyHintsWnd();
        xg_bShowClues = FALSE;
    }
    XG_FILE_MODIFIED(TRUE);
}

void XG_UndoData_NumCro::Get() {
    bNumCro = xg_bNumCroMode;
}
void XG_UndoData_NumCro::Apply() const {
    xg_bNumCroMode = bNumCro;
    if (xg_bNumCroMode) {
        XgMakeItNumCro(xg_hMainWnd);
    }
}

void XG_UndoData_ViewMode::Get() {
    nViewMode = xg_nViewMode;
}
void XG_UndoData_ViewMode::Apply() const {
    xg_nViewMode = nViewMode;
}

void XG_UndoData_Boxes::Get() {
    boxes = XgStringifyBoxes(xg_boxes);
}
void XG_UndoData_Boxes::Apply() const {
    XgDeStringifyBoxes(boxes);
}

void XG_UndoData_SetAll::Get() {
    nRows = xg_nRows;
    nCols = xg_nCols;
    xword = xg_xword;
    solution = xg_solution;
    caret_pos = xg_caret_pos;
    vMarks = xg_vMarks;
    vVertInfo = xg_vVertInfo;
    vHorzInfo = xg_vHorzInfo;
    vecVertHints = xg_vecVertHints;
    vecHorzHints = xg_vecHorzHints;
    bShowHints = xg_bShowClues;
    bSolved = xg_bSolved;
    bHintsAdded = xg_bHintsAdded;
    bShowAnswer = xg_bShowAnswer;
    strHeader = xg_strHeader;
    strNotes = xg_strNotes;
    strFileName = xg_strFileName;
    bNumCro = xg_bNumCroMode;
    nViewMode = xg_nViewMode;
    boxes = XgStringifyBoxes(xg_boxes);
}

void XG_UndoData_SetAll::Apply() const {
    xg_nRows = nRows;
    xg_nCols = nCols;
    xg_xword = xword;
    xg_solution = solution;
    xg_caret_pos = caret_pos;
    xg_vMarks = vMarks;
    xg_vVertInfo = vVertInfo;
    xg_vHorzInfo = vHorzInfo;
    xg_vecVertHints = vecVertHints;
    xg_vecHorzHints = vecHorzHints;
    xg_bSolved = bSolved;
    xg_bHintsAdded = bHintsAdded;
    xg_bShowAnswer = bShowAnswer;
    xg_strHeader = strHeader;
    xg_strNotes = strNotes;
    xg_strFileName = strFileName;
    XgUpdateHints();
    if (bShowHints) {
        XgShowHints(xg_hMainWnd);
        xg_bShowClues = TRUE;
    } else {
        XgDestroyHintsWnd();
        xg_bShowClues = FALSE;
    }
    XgUpdateCaretPos();
    xg_bNumCroMode = bNumCro;
    if (xg_bNumCroMode) {
        XgMakeItNumCro(xg_hMainWnd);
    }
    xg_nViewMode = nViewMode;
    XgDeStringifyBoxes(boxes);
    XG_FILE_MODIFIED(TRUE);
}

//////////////////////////////////////////////////////////////////////////////

bool XG_UndoBuffer::CanUndo() const noexcept {
    if (!m_enabled)
        return false;
    return m_i > 0;
}

bool XG_UndoBuffer::CanRedo() const noexcept {
    if (!m_enabled)
        return false;
    return m_i < size();
}

bool XG_UndoBuffer::Undo() {
    if (CanUndo()) {
        --m_i;
        operator[](m_i).Execute(false);
        XG_FILE_MODIFIED(TRUE);
        return true;
    }
    return false;
}

bool XG_UndoBuffer::Redo() {
    if (CanRedo()) {
        operator[](m_i).Execute(true);
        ++m_i;
        XG_FILE_MODIFIED(TRUE);
        return true;
    }
    return false;
}

void XG_UndoBuffer::Commit(const XG_UndoInfo& ui) {
    if (!m_enabled)
        return;

    resize(m_i);

    emplace_back(ui);
    ++m_i;
    XG_FILE_MODIFIED(TRUE);
}

static constexpr int c_max_size = 25;

void XG_UndoBuffer::Commit(UINT id,
    shared_ptr<XG_UndoData> before, shared_ptr<XG_UndoData> after)
{
    assert(id == before->m_cmdId);
    assert(id == after->m_cmdId);

    if (!m_enabled)
        return;

    resize(m_i);

    emplace_back(id, before, after);
    ++m_i;

    if (size() > c_max_size) {
        pop_front();
        --m_i;
    }
    XG_FILE_MODIFIED(TRUE);
}

//////////////////////////////////////////////////////////////////////////////
