﻿//////////////////////////////////////////////////////////////////////////////
// XG_UndoBuffer.hpp
// Copyright (C) 2012-2023 Katayama Hirofumi MZ. All Rights Reserved.

#pragma once

#include "XWordGiver.hpp"
#include <memory>   // for std::shared_ptr

using std::shared_ptr;

enum UNDOABLE_COMMAND_ID : UINT {
    UC_SETAT,
    UC_MARKS_UPDATED,
    UC_HINTS_UPDATED,
    UC_NUMCRO,
    UC_VIEWMODE,
    UC_BOXES,
    UC_SETALL
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData {
    UNDOABLE_COMMAND_ID m_cmdId;

    XG_UndoData(UNDOABLE_COMMAND_ID cmdId) noexcept
    {
        m_cmdId = cmdId;
    }
    virtual ~XG_UndoData()
    {
    }

    virtual void Get() { }
    virtual void Apply() const { }
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_SetAt : XG_UndoData {
    XG_Pos  pos;
    WCHAR   ch;

    XG_UndoData_SetAt() : XG_UndoData(UC_SETAT) { }

    ~XG_UndoData_SetAt() override
    {
    }
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_MarksUpdated : XG_UndoData {
    std::vector<XG_Pos> vMarks;
    XGStringW strMarked;

    XG_UndoData_MarksUpdated() : XG_UndoData(UC_MARKS_UPDATED) { }

    ~XG_UndoData_MarksUpdated() override
    {
    }
    void Get() override;
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_HintsUpdated : XG_UndoData {
    std::vector<XG_PlaceInfo>   vVertInfo;
    std::vector<XG_PlaceInfo>   vHorzInfo;
    std::vector<XG_Hint>        vecVertHints;
    std::vector<XG_Hint>        vecHorzHints;
    BOOL bShowHints;

    XG_UndoData_HintsUpdated() : XG_UndoData(UC_HINTS_UPDATED) { }

    ~XG_UndoData_HintsUpdated() override
    {
    }
    void Get() override;
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_NumCro : XG_UndoData {
    bool bNumCro;

    XG_UndoData_NumCro() : XG_UndoData(UC_NUMCRO) { }

    ~XG_UndoData_NumCro() override
    {
    }
    void Get() override;
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_ViewMode : XG_UndoData {
    XG_VIEW_MODE nViewMode;

    XG_UndoData_ViewMode() : XG_UndoData(UC_VIEWMODE) { }

    ~XG_UndoData_ViewMode() override
    {
    }
    void Get() override;
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_Boxes : XG_UndoData {
    XGStringW boxes;

    XG_UndoData_Boxes() : XG_UndoData(UC_BOXES) { }

    ~XG_UndoData_Boxes() override
    {
    }
    void Get() override;
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////

struct XG_UndoData_SetAll : XG_UndoData {
    int                         nRows;
    int                         nCols;
    XG_Board                    xword;
    XG_Board                    solution;
    XG_Pos                      caret_pos;
    std::vector<XG_Pos>         vMarks;
    std::vector<XG_PlaceInfo>   vVertInfo;
    std::vector<XG_PlaceInfo>   vHorzInfo;
    std::vector<XG_Hint>        vecVertHints;
    std::vector<XG_Hint>        vecHorzHints;
    BOOL                        bShowHints;
    bool                        bSolved;
    bool                        bHintsAdded;
    bool                        bShowAnswer;
    XGStringW                strHeader;
    XGStringW                strNotes;
    XGStringW                strFileName;
    bool                        bNumCro;
    XG_VIEW_MODE                nViewMode;
    XGStringW                boxes;

    XG_UndoData_SetAll() : XG_UndoData(UC_SETALL) { }

    ~XG_UndoData_SetAll() override
    {
    }
    void Get() override;
    void Apply() const override;
};

//////////////////////////////////////////////////////////////////////////////
// XG_UndoInfo

struct XG_UndoInfo {
    UINT                    nCommandID;
    shared_ptr<XG_UndoData> pBefore;
    shared_ptr<XG_UndoData> pAfter;

    XG_UndoInfo() noexcept { }

    XG_UndoInfo(UINT id, XG_UndoData *before, XG_UndoData *after) :
        nCommandID(id), pBefore(before), pAfter(after)
    {
        assert(nCommandID == pBefore->m_cmdId);
        assert(nCommandID == pAfter->m_cmdId);
    }

    XG_UndoInfo(UINT id, shared_ptr<XG_UndoData> before,
                         shared_ptr<XG_UndoData> after) noexcept :
        nCommandID(id), pBefore(before), pAfter(after)
    {
        assert(nCommandID == pBefore->m_cmdId);
        assert(nCommandID == pAfter->m_cmdId);
    }

    XG_UndoInfo(const XG_UndoInfo& info) noexcept :
        nCommandID(info.nCommandID), pBefore(info.pBefore),
        pAfter(info.pAfter)
    {
        assert(nCommandID == pBefore->m_cmdId);
        assert(nCommandID == pAfter->m_cmdId);
    }

    XG_UndoInfo& operator=(const XG_UndoInfo& info) noexcept {
        nCommandID = info.nCommandID;
        pBefore = info.pBefore;
        pAfter = info.pAfter;
        assert(nCommandID == pBefore->m_cmdId);
        assert(nCommandID == pAfter->m_cmdId);
        return *this;
    }

    void Execute(bool reverse = false) {
        if (reverse) {
            pAfter.get()->Apply();
        } else {
            pBefore.get()->Apply();
        }
    }
};

//////////////////////////////////////////////////////////////////////////////
// XG_UndoBuffer

class XG_UndoBuffer : public std::deque<XG_UndoInfo>
{
public:
    typedef std::deque<XG_UndoInfo> super_type;

    XG_UndoBuffer() noexcept : m_i(0), m_enabled(true) { }

    XG_UndoBuffer(const XG_UndoBuffer& ub) :
        std::deque<XG_UndoInfo>(ub), m_i(ub.m_i),
        m_enabled(ub.m_enabled) { }

    bool CanUndo() const noexcept;
    bool CanRedo() const noexcept;
    bool Undo();
    bool Redo();

    void Empty() {
        clear();
        m_i = 0;
    }

    void Commit(const XG_UndoInfo& ui);
    void Commit(UINT id,
        shared_ptr<XG_UndoData> before, shared_ptr<XG_UndoData> after);

    void Enable(bool enabled) {
        if (!enabled) {
            Empty();
        }
        m_enabled = enabled;
    }

    void clear() noexcept {
        super_type::clear();
        m_i = 0;
    }

protected:
    size_t  m_i;
    bool    m_enabled;
};

// 「元に戻す」ためのバッファ。
extern XG_UndoBuffer xg_ubUndoBuffer;
