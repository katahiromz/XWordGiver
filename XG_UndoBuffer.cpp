//////////////////////////////////////////////////////////////////////////////
// UndoBuffer.cpp
// Copyright (C) 2012-2020 Katayama Hirofumi MZ. All Rights Reserved.

#include "XWordGiver.hpp"
#include "GUI.hpp"
#include "XG_UndoBuffer.hpp"

//////////////////////////////////////////////////////////////////////////////

/*virtual*/ void XG_UndoData_SetAt::Apply() const {
    xg_xword.SetAt(pos, ch);
    xg_caret_pos = pos;
}

/*virtual*/ void XG_UndoData_MarksUpdated::Get() {
    vMarks = xg_vMarks;
}

/*virtual*/ void XG_UndoData_MarksUpdated::Apply() const {
    xg_vMarks = vMarks;
}

/*virtual*/ void XG_UndoData_HintsUpdated::Get() {
    vTateInfo = xg_vTateInfo;
    vYokoInfo = xg_vYokoInfo;
    vecTateHints = xg_vecTateHints;
    vecYokoHints = xg_vecYokoHints;
}

/*virtual*/ void XG_UndoData_HintsUpdated::Apply() const {
    xg_vTateInfo = vTateInfo;
    xg_vYokoInfo = vYokoInfo;
    xg_vecTateHints = vecTateHints;
    xg_vecYokoHints = vecYokoHints;
}

/*virtual*/ void XG_UndoData_SetAll::Get() {
    nRows = xg_nRows;
    nCols = xg_nCols;
    xword = xg_xword;
    solution = xg_solution;
    caret_pos = xg_caret_pos;
    vMarks = xg_vMarks;
    vTateInfo = xg_vTateInfo;
    vYokoInfo = xg_vYokoInfo;
    vecTateHints = xg_vecTateHints;
    vecYokoHints = xg_vecYokoHints;
    bSolved = xg_bSolved;
    bHintsAdded = xg_bHintsAdded;
    bShowAnswer = xg_bShowAnswer;
    strHeader = xg_strHeader;
    strNotes = xg_strNotes;
    strFileName = xg_strFileName;
    bShowHints = !!::IsWindow(xg_hHintsWnd);
}

/*virtual*/ void XG_UndoData_SetAll::Apply() const {
    xg_nRows = nRows;
    xg_nCols = nCols;
    xg_xword = xword;
    xg_solution = solution;
    xg_caret_pos = caret_pos;
    xg_vMarks = vMarks;
    xg_vTateInfo = vTateInfo;
    xg_vYokoInfo = vYokoInfo;
    xg_vecTateHints = vecTateHints;
    xg_vecYokoHints = vecYokoHints;
    xg_bSolved = bSolved;
    xg_bHintsAdded = bHintsAdded;
    xg_bShowAnswer = bShowAnswer;
    xg_strHeader = strHeader;
    xg_strNotes = strNotes;
    xg_strFileName = strFileName;
    if (bShowHints) {
        XgShowHints(xg_hMainWnd);
    } else {
        XgDestroyHintsWnd();
    }
}

//////////////////////////////////////////////////////////////////////////////

bool XG_UndoBuffer::CanUndo() const {
    if (!m_enabled)
        return false;
    return m_i > 0;
}

bool XG_UndoBuffer::CanRedo() const {
    if (!m_enabled)
        return false;
    return m_i < size();
}

bool XG_UndoBuffer::Undo() {
    if (CanUndo()) {
        --m_i;
        operator[](m_i).Execute(false);
        return true;
    }
    return false;
}

bool XG_UndoBuffer::Redo() {
    if (CanRedo()) {
        operator[](m_i).Execute(true);
        ++m_i;
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
}

static const int c_max_size = 25;

void XG_UndoBuffer::Commit(UINT id,
    shared_ptr<XG_UndoData> before, shared_ptr<XG_UndoData> after)
{
    if (!m_enabled)
        return;

    resize(m_i);

    emplace_back(id, before, after);
    ++m_i;

    if (size() > c_max_size) {
        pop_front();
        --m_i;
    }
}

//////////////////////////////////////////////////////////////////////////////
