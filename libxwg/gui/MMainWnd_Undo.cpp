// MMainWnd_Undo.cpp --- クロスワードギバーの「元に戻す」「やり直す」機能。
// Author: katahiromz
// License: MIT

#include "MMainWnd.h"
#include <memory>
#include <deque>

// 「元に戻す」「やり直す」データの基底
struct MUndoData {
	MMainWnd& m_hMainWnd;
	UINT m_nType;

	MUndoData(MMainWnd& hMainWnd, UINT nType) noexcept
		: m_hMainWnd(hMainWnd)
		, m_nType(nType)
	{
	}
	virtual ~MUndoData()
	{
	}

	virtual void Get() { }
	virtual void Apply() const { }
};


// 盤面の元に戻す情報
struct MUndoData_Board : MUndoData {
	MBoard m_board;
	MBoard m_solution;
	MBoardPos m_caret_pos;

	MUndoData_Board(MMainWnd& hMainWnd) noexcept : MUndoData(hMainWnd, XG_UNDO_BOARD)
	{
	}

	void Get() override {
		m_board = m_hMainWnd.m_board;
		m_solution = m_hMainWnd.m_solution;
		m_caret_pos = m_hMainWnd.m_caret_pos;
	}
	void Apply() const override {
		m_hMainWnd.m_board = m_board;
		m_hMainWnd.m_solution = m_solution;
		m_hMainWnd.m_caret_pos = m_caret_pos;
		xg_nRules = m_board.Rules();
	}
};

// カギの元に戻す情報
struct MUndoData_Clues : MUndoData {
	std::vector<MPlaceInfo> m_vHorzInfo;
	std::vector<MPlaceInfo> m_vVertInfo;
	MStringMapW m_word2clue;

	MUndoData_Clues(MMainWnd& hMainWnd) noexcept : MUndoData(hMainWnd, XG_UNDO_CLUES)
	{
	}

	void Get() override {
		m_vHorzInfo = m_hMainWnd.m_vHorzInfo;
		m_vVertInfo = m_hMainWnd.m_vVertInfo;
		m_word2clue = m_hMainWnd.m_word2clue;
	}
	void Apply() const override {
		BOOL bOpen = !!m_hMainWnd.m_hCluesWnd;
		m_hMainWnd.m_vHorzInfo = m_vHorzInfo;
		m_hMainWnd.m_vVertInfo = m_vVertInfo;
		m_hMainWnd.m_word2clue = m_word2clue;
		if (bOpen) {
			m_hMainWnd.m_hCluesWnd.SetHintsData();
		}
	}
};

// 表示モードの元に戻す情報
struct MUndoData_View : MUndoData {
	BOOL m_bShowAnswer;
	XG_VIEW_MODE m_nViewMode;
	BOOL m_bHiragana;
	BOOL m_bLowercase;
	BOOL m_bNumCroMode;
	BOOL m_bShowCaret;
	BOOL m_bShowDoubleFrame;
	BOOL m_bShowDoubleFrameLetters;
	BOOL m_bCheckingAnswer;

	MUndoData_View(MMainWnd& hMainWnd) noexcept : MUndoData(hMainWnd, XG_UNDO_VIEW)
	{
	}

	void Get() override {
		m_bShowAnswer = m_hMainWnd.m_bShowAnswer;
		m_nViewMode = m_hMainWnd.m_nViewMode;
		m_bHiragana = m_hMainWnd.m_bHiragana;
		m_bLowercase = m_hMainWnd.m_bLowercase;
		m_bNumCroMode = m_hMainWnd.m_bNumCroMode;
		m_bShowCaret = m_hMainWnd.m_bShowCaret;
		m_bShowDoubleFrame = m_hMainWnd.m_bShowDoubleFrame;
		m_bShowDoubleFrameLetters = m_hMainWnd.m_bShowDoubleFrameLetters;
		m_bCheckingAnswer = m_hMainWnd.m_bCheckingAnswer;
	}
	void Apply() const override {
		m_hMainWnd.m_bShowAnswer = m_bShowAnswer;
		m_hMainWnd.m_nViewMode = m_nViewMode;
		m_hMainWnd.m_bHiragana = m_bHiragana;
		m_hMainWnd.m_bLowercase = m_bLowercase;
		m_hMainWnd.m_bNumCroMode = m_bNumCroMode;
		m_hMainWnd.m_bShowCaret = m_bShowCaret;
		m_hMainWnd.m_bShowDoubleFrame = m_bShowDoubleFrame;
		m_hMainWnd.m_bShowDoubleFrameLetters = m_bShowDoubleFrameLetters;
		m_hMainWnd.m_bCheckingAnswer = m_bCheckingAnswer;
	}
};

// 黒マスルールの元に戻す情報
struct MUndoData_Rules : MUndoData {
	INT m_nRules;

	MUndoData_Rules(MMainWnd& hMainWnd) noexcept : MUndoData(hMainWnd, XG_UNDO_RULES)
	{
	}

	void Get() override {
		m_nRules = xg_nRules;
	}
	void Apply() const override {
		m_hMainWnd.SetRules0(m_nRules);
	}
};

// 二重マスの元に戻す情報
struct MUndoData_Marks : MUndoData {
	std::vector<MBoardPos> m_vMarks;

	MUndoData_Marks(MMainWnd& hMainWnd) noexcept : MUndoData(hMainWnd, XG_UNDO_MARKS)
	{
	}

	void Get() override {
		m_vMarks = m_hMainWnd.m_vMarks;
	}
	void Apply() const override {
		m_hMainWnd.m_vMarks = m_vMarks;
		m_hMainWnd.InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_MARKSUPDATE, 0, 0);
	}
};

// すべての元に戻す情報
struct MUndoData_All : MUndoData {
	MBoardDisplay m_display;
	std::vector<MPlaceInfo> m_vHorzInfo;
	std::vector<MPlaceInfo> m_vVertInfo;
	MStringMapW m_word2clue;
	XG_VIEW_MODE m_nViewMode;
	BOOL m_bHiragana;
	BOOL m_bLowercase;
	BOOL m_bNumCroMode;
	BOOL m_bShowCaret;
	BOOL m_bShowDoubleFrame;
	BOOL m_bShowDoubleFrameLetters;
	BOOL m_bCheckingAnswer;
	INT m_nRules;
	std::vector<MBoardPos> m_vMarks;
	MStringW m_strFileName;

	MUndoData_All(MMainWnd& hMainWnd) noexcept : MUndoData(hMainWnd, XG_UNDO_ALL)
	{
	}

	void Get() override {
		m_display.CopyFrom(m_hMainWnd.m_display);
		m_vHorzInfo = m_hMainWnd.m_vHorzInfo;
		m_vVertInfo = m_hMainWnd.m_vVertInfo;
		m_word2clue = m_hMainWnd.m_word2clue;
		m_nViewMode = m_hMainWnd.m_nViewMode;
		m_bHiragana = m_hMainWnd.m_bHiragana;
		m_bLowercase = m_hMainWnd.m_bLowercase;
		m_bNumCroMode = m_hMainWnd.m_bNumCroMode;
		m_bShowCaret = m_hMainWnd.m_bShowCaret;
		m_bShowDoubleFrame = m_hMainWnd.m_bShowDoubleFrame;
		m_bShowDoubleFrameLetters = m_hMainWnd.m_bShowDoubleFrameLetters;
		m_bCheckingAnswer = m_hMainWnd.m_bCheckingAnswer;
		m_nRules = xg_nRules;
		m_vMarks = m_hMainWnd.m_vMarks;
		m_strFileName = m_hMainWnd.m_strFileName;
	}
	void Apply() const override {
		m_hMainWnd.DestroyCluesWnd();
		m_hMainWnd.m_display.CopyFrom(m_display);
		m_hMainWnd.m_vHorzInfo = m_vHorzInfo;
		m_hMainWnd.m_vVertInfo = m_vVertInfo;
		m_hMainWnd.m_word2clue = m_word2clue;
		m_hMainWnd.m_nViewMode = m_nViewMode;
		m_hMainWnd.m_bHiragana = m_bHiragana;
		m_hMainWnd.m_bLowercase = m_bLowercase;
		m_hMainWnd.m_bNumCroMode = m_bNumCroMode;
		m_hMainWnd.m_bShowCaret = m_bShowCaret;
		m_hMainWnd.m_bShowDoubleFrame = m_bShowDoubleFrame;
		m_hMainWnd.m_bShowDoubleFrameLetters = m_bShowDoubleFrameLetters;
		m_hMainWnd.m_bCheckingAnswer = m_bCheckingAnswer;
		m_hMainWnd.m_strFileName = m_strFileName;
		m_hMainWnd.SetRules0(m_nRules);
		m_hMainWnd.m_vMarks = m_vMarks;
		if (m_display.m_bSolved) {
			m_hMainWnd.CreateCluesWnd(m_hMainWnd);
		}
		m_hMainWnd.InvalidateImage();
	}
};

//////////////////////////////////////////////////////////////////////////////
// MUndoInfo

// 「元に戻す」「やり直す」情報。
struct MUndoInfo {
	UINT m_nType;
	std::shared_ptr<MUndoData> m_pBefore;
	std::shared_ptr<MUndoData> m_pAfter;

	MUndoInfo() noexcept { }

	MUndoInfo(UINT nType, MUndoData *before, MUndoData *after)
		: m_nType(nType), m_pBefore(before), m_pAfter(after)
	{
		assert(nType == m_pBefore->m_nType);
		assert(nType == m_pAfter->m_nType);
	}

	MUndoInfo(UINT nType, std::shared_ptr<MUndoData> before, std::shared_ptr<MUndoData> after) noexcept
		: m_nType(nType), m_pBefore(before), m_pAfter(after)
	{
		assert(nType == m_pBefore->m_nType);
		assert(nType == m_pAfter->m_nType);
	}

	MUndoInfo(const MUndoInfo& info) noexcept = default;
	MUndoInfo& operator=(const MUndoInfo& info) noexcept = default;

	void Execute(BOOL reverse = FALSE) {
		if (reverse) {
			m_pAfter.get()->Apply();
		} else {
			m_pBefore.get()->Apply();
		}
	}
};

// 「元に戻す」「やり直す」バッファ。
class MUndoBuffer : public std::deque<MUndoInfo> {
public:
	typedef std::deque<MUndoInfo> super_type;
	static constexpr size_t npos = (size_t)-1;
	enum { c_max_size = 32 };

	BOOL IsModified() const noexcept { return m_saved != m_i; }
	void MarkSaved() noexcept		{ m_saved = m_i; }	 // 保存した
	void MarkUnsavable() noexcept	{ m_saved = npos; }	// Undo では戻れない変更

	MUndoBuffer() noexcept
		: m_i(0)
		, m_enabled(TRUE)
	{
	}

	MUndoBuffer(const MUndoBuffer& ub)
		: std::deque<MUndoInfo>(ub)
		, m_i(ub.m_i)
		, m_enabled(ub.m_enabled)
	{
	}

	BOOL CanUndo() const noexcept {
		if (!m_enabled)
			return FALSE;
		return m_i > 0;
	}
	BOOL CanRedo() const noexcept {
		if (!m_enabled)
			return FALSE;
		return m_i < size();
	}

	BOOL Undo() {
		if (CanUndo()) {
			--m_i;
			(*this)[m_i].Execute(FALSE);
			return TRUE;
		}
		return FALSE;
	}

	BOOL Redo() {
		if (CanRedo()) {
			(*this)[m_i].Execute(TRUE);
			++m_i;
			return TRUE;
		}
		return FALSE;
	}

	void Empty() {
		clear();
		m_i = 0;
	}

	void Commit(const MUndoInfo& ui) {
		if (!m_enabled)
			return;

		resize(m_i);								  // Redo 分を捨てる
		if (m_saved != npos && m_saved > m_i)		 // 保存位置が捨てられた
			m_saved = npos;

		emplace_back(ui);
		++m_i;

		if (size() > c_max_size) {
			pop_front();
			--m_i;
			if (m_saved != npos) {					// 先頭が消えたので保存位置もずらす
				if (m_saved == 0) m_saved = npos;
				else --m_saved;
			}
		}
	}

	void Commit(UINT nType, std::shared_ptr<MUndoData> before, std::shared_ptr<MUndoData> after) {
		assert(nType == before->m_nType);
		assert(nType == after->m_nType);
		Commit({ nType, before, after });
	}

	void Enable(bool enabled) {
		if (!enabled) {
			Empty();
		}
		m_enabled = enabled;
	}

	void clear() noexcept {
		super_type::clear();
		m_i = 0;
		m_saved = 0;
	}

protected:
	size_t m_i;
	size_t m_saved = 0;
	BOOL m_enabled;
};

static MUndoBuffer s_undo_buffer;
static std::shared_ptr<MUndoData_Board> s_pUndoBoard = nullptr;
static std::shared_ptr<MUndoData_Clues> s_pUndoClues = nullptr;
static std::shared_ptr<MUndoData_View> s_pUndoView = nullptr;
static std::shared_ptr<MUndoData_Rules> s_pUndoRules = nullptr;
static std::shared_ptr<MUndoData_Marks> s_pUndoMarks = nullptr;
static std::shared_ptr<MUndoData_All> s_pUndoAll = nullptr;

BOOL MMainWnd::CanUndo() const {
	return s_undo_buffer.CanUndo();
}
BOOL MMainWnd::CanRedo() const {
	return s_undo_buffer.CanRedo();
}

BOOL MMainWnd::Undo() {
	BOOL ret = s_undo_buffer.Undo();
	if (ret)
		m_bFileModified = s_undo_buffer.IsModified();
	UpdateCaretPos();
	InvalidateImage();
	UpdateTitleBarText();
	UpdateToolBarUI();
	UpdateStatusBar();
	return ret;
}
BOOL MMainWnd::Redo() {
	BOOL ret = s_undo_buffer.Redo();
	if (ret)
		m_bFileModified = s_undo_buffer.IsModified();
	UpdateCaretPos();
	InvalidateImage();
	UpdateTitleBarText();
	UpdateToolBarUI();
	UpdateStatusBar();
	return ret;
}

void MMainWnd::ClearUndoBuffer() {
	s_undo_buffer.clear();
	s_pUndoBoard = nullptr;
	s_pUndoClues = nullptr;
	s_pUndoView = nullptr;
	s_pUndoRules = nullptr;
	s_pUndoMarks = nullptr;
	s_pUndoAll = nullptr;
}

void MMainWnd::MarkSaved() {
	s_undo_buffer.MarkSaved();
}

BOOL MMainWnd::OnUndoUpdate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	UINT nType = UINT(wParam);
	BOOL bDontUndo = LOWORD(lParam), bNoModify = HIWORD(lParam);

	if (uMsg == XGWM_UNDO_UPDATED && !bDontUndo) {
		if (bNoModify) {
			m_bFileModified = FALSE;
		} else {
			SendMessageW(hwnd, XGWM_FILECHANGED, 0, 0);
		}
	}

	switch (nType) {
	case XG_UNDO_ALL:
		if (uMsg == XGWM_UNDO_UPDATED) {
			if (bDontUndo) {
				s_pUndoAll = nullptr;
				return TRUE;
			}
			if (s_pUndoAll) {
				auto pUndoAll = std::make_shared<MUndoData_All>(*this);
				pUndoAll->Get();
				s_undo_buffer.Commit(nType, s_pUndoAll, pUndoAll);
				s_pUndoAll = nullptr;
				return TRUE;
			}
		} else if (uMsg == XGWM_UNDO_UPDATING) {
			s_pUndoAll = nullptr;
			auto pUndoAll = std::make_shared<MUndoData_All>(*this);
			pUndoAll->Get();
			s_pUndoAll = pUndoAll;
			return TRUE;
		}
		break;
	case XG_UNDO_BOARD:
		if (uMsg == XGWM_UNDO_UPDATED) {
			if (bDontUndo) {
				s_pUndoBoard = nullptr;
				return TRUE;
			}
			if (s_pUndoBoard) {
				auto pUndoBoard = std::make_shared<MUndoData_Board>(*this);
				pUndoBoard->Get();
				s_undo_buffer.Commit(nType, s_pUndoBoard, pUndoBoard);
				s_pUndoBoard = nullptr;
				InvalidateImage();
				return TRUE;
			}
		} else if (uMsg == XGWM_UNDO_UPDATING) {
			s_pUndoBoard = nullptr;
			auto pUndoBoard = std::make_shared<MUndoData_Board>(*this);
			pUndoBoard->Get();
			s_pUndoBoard = pUndoBoard;
			return TRUE;
		}
		break;
	case XG_UNDO_CLUES:
		if (uMsg == XGWM_UNDO_UPDATED) {
			if (bDontUndo) {
				s_pUndoClues = nullptr;
				return TRUE;
			}
			if (s_pUndoClues) {
				auto pUndoClues = std::make_shared<MUndoData_Clues>(*this);
				pUndoClues->Get();
				s_undo_buffer.Commit(nType, s_pUndoClues, pUndoClues);
				s_pUndoClues = nullptr;
				return TRUE;
			}
		} else if (uMsg == XGWM_UNDO_UPDATING) {
			s_pUndoClues = nullptr;
			auto pUndoClues = std::make_shared<MUndoData_Clues>(*this);
			pUndoClues->Get();
			s_pUndoClues = pUndoClues;
			return TRUE;
		}
		break;
	case XG_UNDO_VIEW:
		if (uMsg == XGWM_UNDO_UPDATED) {
			if (bDontUndo) {
				s_pUndoView = nullptr;
				return TRUE;
			}
			if (s_pUndoView) {
				auto pUndoView = std::make_shared<MUndoData_View>(*this);
				pUndoView->Get();
				s_undo_buffer.Commit(nType, s_pUndoView, pUndoView);
				s_pUndoView = nullptr;
				return TRUE;
			}
		} else if (uMsg == XGWM_UNDO_UPDATING) {
			s_pUndoView = nullptr;
			auto pUndoView = std::make_shared<MUndoData_View>(*this);
			pUndoView->Get();
			s_pUndoView = pUndoView;
			return TRUE;
		}
		break;
	case XG_UNDO_RULES:
		if (uMsg == XGWM_UNDO_UPDATED) {
			if (bDontUndo) {
				s_pUndoRules = nullptr;
				return TRUE;
			}
			if (s_pUndoRules) {
				auto pUndoRules = std::make_shared<MUndoData_Rules>(*this);
				pUndoRules->Get();
				s_undo_buffer.Commit(nType, s_pUndoRules, pUndoRules);
				s_pUndoRules = nullptr;
				return TRUE;
			}
		} else if (uMsg == XGWM_UNDO_UPDATING) {
			s_pUndoRules = nullptr;
			auto pUndoRules = std::make_shared<MUndoData_Rules>(*this);
			pUndoRules->Get();
			s_pUndoRules = pUndoRules;
			return TRUE;
		}
		break;
	case XG_UNDO_MARKS:
		if (uMsg == XGWM_UNDO_UPDATED) {
			if (bDontUndo) {
				s_pUndoMarks = nullptr;
				return TRUE;
			}
			if (s_pUndoMarks) {
				auto pUndoMarks = std::make_shared<MUndoData_Marks>(*this);
				pUndoMarks->Get();
				s_undo_buffer.Commit(nType, s_pUndoMarks, pUndoMarks);
				s_pUndoMarks = nullptr;
				InvalidateImage();
				return TRUE;
			}
		} else if (uMsg == XGWM_UNDO_UPDATING) {
			s_pUndoMarks = nullptr;
			auto pUndoMarks = std::make_shared<MUndoData_Marks>(*this);
			pUndoMarks->Get();
			s_pUndoMarks = pUndoMarks;
			return TRUE;
		}
		break;
	}
	return FALSE;
}
