// MMainWnd.cpp --- クロスワードギバー メインウィンドウ。
// Author: katahiromz
// License: MIT

#include "xwg.h"
#include "algo1.h"
#include "algo2.h"
#include "algo3.h"
#include "algo4.h"
#include "algo5.h"
#include "board-image.h"
#include "clues.h"
#include "flip-board.h"
#include "file-io-2.h"
#include "gui-utils.h"
#include "image-io.h"
#include "pat-match.h"
#include "resize-board.h"
#include "utils.h"
#include "image-io-2.h"
#include "MMainWnd.h"
#include "MNewDialog.h"
#include "MAboutDialog.h"
#include "MUILanguageDialog.h"
#include "MWordListDialog.h"
#include "MNotesDialog.h"
#include "MPatternDialog.h"
#include "MRegKey.h"
#include "MRulePresetDialog.h"
#include "MGenDialog.h"
#include "MPatGenDialog.h"
#include "MCancelDialog.h"
#include "MDictsDialog.h"
#include "MAIHelperDialog.h"
#include "MSettingsDialog.h"
#include "MFileSettingsDialog.h"
#include "MHiddenDialog.h"
#include "MThemeDialog.h"
#include "MGenerativeDialog.h"
#include "WonSetThreadUILanguage.h"
#include "EnvVars.h"
#include <set>
#include "resource.h"

#define XG_MAX_RECENT 10 // 最近使ったファイルは10個まで。

//////////////////////////////////////////////////////////////////////

MMainWnd::MMainWnd(INT argc, LPWSTR* wargv)
	: m_argc(argc)
	, m_wargv(wargv)
	, m_hMarksDlg(m_dict, m_display)
	, m_hJumpDialog(m_display)
	, m_hCanvasWnd(m_display, m_imode)
	, m_hCluesWnd(m_vHorzInfo, m_vVertInfo, m_word2clue, m_lfCluesLogFont)
	, m_hInputPalette(m_imode, m_bCharFeed, m_bVertInput, m_bVerticalLayout, m_bShowInputPalette)
	, m_hCandsWnd(m_dict, m_display, m_lfCandsLogFont)
	, m_ai_helper(*this)
{
	::GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(m_lfCluesLogFont), &m_lfCluesLogFont);
	::GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(m_lfCandsLogFont), &m_lfCandsLogFont);
	m_hJumpDialog.m_fnDoIt = [this]() {
		switch (m_hJumpDialog.m_nType) {
		case 0:
			m_caret_pos = { m_hJumpDialog.m_jColumn, m_hJumpDialog.m_iRow };
			UpdateCaretPos();
			UpdateStatusBar();
			InvalidateImage();
			break;
		case 1:
			JumpNumber(m_hJumpDialog.m_nNumber, m_hJumpDialog.m_bVert);
			break;
		case 2:
			JumpWord(m_hJumpDialog.m_strWord);
			break;
		}
	};
}

MMainWnd::~MMainWnd() {
	delete m_pTaskbarProgress;
	m_pTaskbarProgress = nullptr;
}

LPCTSTR MMainWnd::GetWndClassNameDx() const {
	return TEXT("XWordGiver Main Window");
}

VOID MMainWnd::ModifyWndClassDx(WNDCLASSEX& wcx) {
	assert(wcx.hInstance);

	wcx.hIcon = LoadIconW(wcx.hInstance, MAKEINTRESOURCEW(1));
	wcx.lpszMenuName = MAKEINTRESOURCEW(1);
	wcx.hIconSm = (HICON)LoadImage(wcx.hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);

	assert(wcx.hIcon);
	assert(wcx.hIconSm);
}

void MMainWnd::JumpNumber(INT nNumber, BOOL bVert) {
	if (bVert) { // タテ。
		for (auto& vert : m_vVertInfo) {
			if (nNumber == vert.m_number) {
				m_caret_pos = { vert.m_jCol, vert.m_iRow };
				break;
			}
		}
	} else { // ヨコ。
		for (auto& horz : m_vHorzInfo) {
			if (nNumber == horz.m_number) {
				m_caret_pos = { horz.m_jCol, horz.m_iRow };
				break;
			}
		}
	}

	UpdateCaretPos();
	UpdateStatusBar();
	InvalidateImage();
	SetFocus(m_hwnd);
}

void MMainWnd::ClearAllClues() {
	m_hCluesWnd.UpdateHintData();

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_CLUES, 0);
	for (auto& pair : m_word2clue) {
		pair.second.clear();
	}
	m_hCluesWnd.SetHintsData();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_CLUES, 0);
}

void MMainWnd::ClearThisClue(INT nNumber, BOOL bVert) {
	m_hCluesWnd.UpdateHintData();

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_CLUES, 0);

	BOOL bUpdated = FALSE;
	if (bVert) { // タテ。
		for (auto& vert : m_vVertInfo) {
			if (nNumber == vert.m_number) {
				m_word2clue[vert.m_word].clear();
				bUpdated = TRUE;
				break;
			}
		}
	} else { // ヨコ。
		for (auto& horz : m_vHorzInfo) {
			if (nNumber == horz.m_number) {
				m_word2clue[horz.m_word].clear();
				bUpdated = TRUE;
				break;
			}
		}
	}

	m_hCluesWnd.SetHintsData();

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_CLUES, !bUpdated);
}

void MMainWnd::JumpWord(const MStringW& word) {
	auto& board = XgGetDisplayBoard(m_display);
	const INT nColumns = board.Columns(), nRows = board.Rows();
	for (INT iRow = 0; iRow < nRows; ++iRow) {
		for (INT jColumn = 0; jColumn < nColumns; ++jColumn) {
			auto pat0 = XgGetPatFromBoard(board, jColumn, iRow, FALSE);
			if (pat0 == word) {
				m_caret_pos = { jColumn, iRow };
				UpdateCaretPos();
				UpdateStatusBar();
				InvalidateImage();
				return;
			}
			auto pat1 = XgGetPatFromBoard(board, jColumn, iRow, TRUE);
			if (pat1 == word) {
				m_caret_pos = { jColumn, iRow };
				UpdateCaretPos();
				UpdateStatusBar();
				InvalidateImage();
				return;
			}
		}
	}
	for (INT iRow = 0; iRow < nRows; ++iRow) {
		for (INT jColumn = 0; jColumn < nColumns; ++jColumn) {
			auto pat0 = XgGetPatFromBoard(board, jColumn, iRow, FALSE);
			if (pat0.find(word) != pat0.npos) {
				m_caret_pos = { jColumn, iRow };
				UpdateCaretPos();
				UpdateStatusBar();
				InvalidateImage();
				return;
			}
			auto pat1 = XgGetPatFromBoard(board, jColumn, iRow, TRUE);
			if (pat1.find(word) != pat1.npos) {
				m_caret_pos = { jColumn, iRow };
				UpdateCaretPos();
				UpdateStatusBar();
				InvalidateImage();
				return;
			}
		}
	}
}

// 変更内容があれば、保存を確認する。
BOOL MMainWnd::DoConfirmSave(HWND hwnd) {
	if (m_bFileModified) {
		WCHAR text[MAX_PATH];
		MStringW filename = PathFindFileNameW(m_strFileName.c_str());
		StringCchPrintfW(text, _countof(text), XgLoadString(IDS_QUERYSAVE).c_str(),
						 filename.c_str());
		switch (MsgBoxDx(text, MB_ICONINFORMATION | MB_YESNOCANCEL)) {
		case IDYES:
			return OnSave(hwnd);
		case IDNO:
			m_bFileModified = FALSE;
			return TRUE;
		case IDCANCEL:
			return FALSE;
		}
	}
	return TRUE;
}

// ファイルを読み込む。
BOOL MMainWnd::LoadFile0(HWND hwnd, LPCWSTR file) {
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);

	// メインファイルを読み込む。
	if (!XgLoadCrosswordFile(m_display, file)) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, MAKELPARAM(TRUE, TRUE)); // Reject
		return FALSE;
	}

	// ついでにLOOKSファイルも読み込む。
	if (!m_bNoReadLooks) {
		WCHAR path[MAX_PATH];
		StringCchCopyW(path, _countof(path), file);
		PathRemoveExtensionW(path);
		MStringW title = PathFindFileNameW(path);
		StringCchCatW(path, _countof(path), L"_files\\");
		StringCchCatW(path, _countof(path), title.c_str());
		StringCchCatW(path, _countof(path), L".looks");
		XgImportLooksFile(m_display, path);
	}

	m_bShowAnswer = m_bShowAnswerOnOpen;
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, MAKELPARAM(FALSE, TRUE));

	DestroyCandsWnd();
	DestroyCluesWnd();
	InvalidateImage();
	if (m_bSolved) {
		CreateCluesWnd(hwnd);
	}
	return TRUE;
}

// ファイルを読み込む。
BOOL MMainWnd::LoadFile1(HWND hwnd, LPCWSTR file) {
	if (!LoadFile0(hwnd, file))
		return FALSE;

	WCHAR path[MAX_PATH];
	GetFullPathNameW(file, _countof(path), path, nullptr);
	m_strFileName = path;
	m_bFileModified = FALSE;
	XgUpdateRecentlyUsed(m_recently_used_files, path, XG_MAX_RECENT);
	ClearUndoBuffer();
	UpdateTitleBarText();
	return TRUE;
}

// ファイルを読み込む。
BOOL MMainWnd::LoadFile(HWND hwnd, LPCWSTR file) {
	if (!LoadFile1(hwnd, file)) {
		ErrorBoxDx(IDS_CANTLOAD);
		return FALSE;
	}
	return TRUE;
}

// ファイルを保存する。
BOOL MMainWnd::SaveFile0(HWND hwnd, LPCWSTR file) {
	if (lstrcmpiW(PathFindExtensionW(file), L".crp") == 0 && (m_header.size() || m_notes.size())) {
		if (MsgBoxDx(IDS_CRPSAVEWARNING, MB_ICONWARNING | MB_YESNOCANCEL) != IDYES)
			return TRUE;
	}
	return XgSaveCrosswordFile(m_display, file);
}

// ファイルを保存する。
BOOL MMainWnd::SaveFile1(HWND hwnd, LPCWSTR file, BOOL bUpdateMRU) {
	if (!SaveFile0(hwnd, file))
		return FALSE;

	// ついでにLOOKSファイルも書き込む。
	if (!m_bNoWriteLooks) {
		WCHAR path[MAX_PATH];
		StringCchCopyW(path, _countof(path), file);
		PathRemoveExtensionW(path);
		MStringW title = PathFindFileNameW(path);
		StringCchCatW(path, _countof(path), L"_files");
		CreateDirectoryW(path, nullptr);
		StringCchCatW(path, _countof(path), L"\\");
		StringCchCatW(path, _countof(path), title.c_str());
		StringCchCatW(path, _countof(path), L".looks");
		XgExportLooksFile(m_display, path);
	}

	// MRU (最近使ったファイル)を更新。
	WCHAR path[MAX_PATH];
	GetFullPathNameW(file, _countof(path), path, nullptr);
	if (bUpdateMRU)
		XgUpdateRecentlyUsed(m_recently_used_files, path, XG_MAX_RECENT);

	// 保存できない形式ならば拡張子を外す。
	if (!XgCanDotExtSave(PathFindExtensionW(path)))
		PathRemoveBackslashW(path);

	m_strFileName = path;
	m_bFileModified = FALSE;
	MarkSaved();

	UpdateTitleBarText();
	UpdateToolBarUI();
	return TRUE;
}

// ファイルを保存する。
BOOL MMainWnd::SaveFile(HWND hwnd, LPCWSTR file, BOOL bUpdateMRU) {
	if (!SaveFile1(hwnd, file, bUpdateMRU)) {
		ErrorBoxDx(IDS_CANTSAVE);
		return FALSE;
	}
	return TRUE;
}

// 新規作成。
void MMainWnd::New(INT nColumns, INT nRows, INT nRules, INT id, BOOL bNoGUI) {
	if (!bNoGUI && !DoConfirmSave(m_hwnd))
		return;

	DestroyCluesWnd();
	DestroyMarksDlg();
	DestroyCandsWnd();

	auto& board = m_display.m_board;
	const INT cx = board.Columns(), cy = board.Rows();
	auto new_board = MBoard(nColumns, nRows, nRules);
	if (id == rad1) {
		new_board.clear(ZEN_SPACE);
	} else if (id == rad2) {
		new_board.clear(ZEN_BLACK);
	} else if (id == rad3) {
		for (INT y = 0; y < cy; ++y) {
			for (INT x = 0; x < cx; ++x) {
				new_board.SetCell(x, y, board.GetCell(x, y));
			}
		}
	} else {
		assert(0);
	}

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
	m_display.m_board = new_board;
	m_display.m_solution = std::move(new_board);
	m_display.m_bSolved = FALSE;
	m_display.m_caret_pos = { 0, 0 };
	m_vHorzInfo.clear();
	m_vVertInfo.clear();
	m_vMarks.clear();
	m_bNumCroMode = FALSE;
	m_display.m_mapNumCro1.clear();
	m_display.m_mapNumCro2.clear();
	m_word2clue.clear();
	m_strFileName = XgLoadString(IDS_UNTITLED);
	m_header.clear();
	m_notes.clear();
	UpdateScrollInfo(0, 0);
	UpdateCaretPos();
	m_dict.load_file(m_dict_name.c_str()); // テーマタグはリセットする。
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, MAKELPARAM(FALSE, TRUE));
	UpdateTitleBarText();
	UpdateStatusBar();
}

// 生成する。
void MMainWnd::Generate(HWND hwnd, INT nColumns, INT nRows, INT nRules, BOOL bChoosePAT, BOOL bShowAnswerOnGenerate, INT nMaxWordLen, BOOL bNoGUI) {
	DestroyCluesWnd();
	DestroyCandsWnd();
	DestroyMarksDlg();

	MBoard board(nColumns, nRows, nRules);
	std::vector<MBoard> solutions;
	MStringW strError;

	MCancelDialog dialog1;
	auto fnQuit = [&](HRESULT hr, std::vector<MBoard>& sols, size_t max_sols, MStringW& error) -> void {
		solutions = std::move(sols);
		strError = std::move(error);
	};
	dialog1.m_fnRun = [&]() -> HRESULT {
		return XgAlgo4(fnQuit, m_dict, board, nMaxWordLen, 1, bChoosePAT);
	};
	dialog1.m_fnCancel = []() -> void {
		XgAlgo4Cancel();
	};
	dialog1.m_fnEnd = [&](HRESULT hr) -> void {
		if (FAILED(hr)) {
			EndDialog(dialog1, IDCANCEL);
		} else {
			EndDialog(dialog1, IDOK);
		}
	};
	BeginTaskbarProgress();
	auto id = dialog1.DoModal(hwnd);
	EndTaskbarProgress();
	if (id == IDOK && solutions.size()) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		m_board = solutions[0];
		m_solution = solutions[0];
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		m_bShowAnswer = bShowAnswerOnGenerate;
		FinalizeBoard();
		UpdateScrollInfo(0, 0);
		InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		if (m_bAutoSave)
			NumberingSave(hwnd, FALSE, bNoGUI);
		MyPlaySound(MY_SOUND_SUCCESS);
		if (!bNoGUI && !m_bNoGeneratedMsg)
			ShowResult(IDS_MADEPROBLEM, GetTickCount64() - dialog1.m_nStartTick, 1);
	} else {
		if (dialog1.m_hr == E_ABORT) {
			MyPlaySound(MY_SOUND_CANCELED);
			if (!bNoGUI && !m_bNoCanceledMsg) {
				ShowResult(IDS_CANCELLED, GetTickCount64() - dialog1.m_nStartTick);
			}
		} else if (dialog1.m_hr == S_FALSE) {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowResult(IDS_CANTMAKEPROBLEM, GetTickCount64() - dialog1.m_nStartTick);
			EndTaskbarProgress();
		} else {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowError(dialog1.m_hr, strError);
			EndTaskbarProgress();
		}
	}
}

// 解を求める（黒マス追加あり）。
void MMainWnd::Solve(HWND hwnd, BOOL bNoGUI) {
	if (m_bSolved)
		return;

	DestroyMarksDlg();
	DestroyCandsWnd();
	DestroyCluesWnd();

	auto board = m_board;
	auto nMaxWordLen = 5;
	std::vector<MBoard> solutions;
	MStringW strError;

	MCancelDialog dialog;
	auto fnQuit = [&](HRESULT hr, std::vector<MBoard>& sols, size_t max_sols, MStringW& error) -> void {
		solutions = std::move(sols);
		strError = std::move(error);
	};
	dialog.m_fnRun = [&]() -> HRESULT {
		return XgAlgo4(fnQuit, m_dict, board, nMaxWordLen, 1, FALSE);
	};
	dialog.m_fnCancel = []() -> void {
		XgAlgo4Cancel();
	};
	dialog.m_fnEnd = [&](HRESULT hr) -> void {
		if (FAILED(hr)) {
			EndDialog(dialog, IDCANCEL);
		} else {
			EndDialog(dialog, IDOK);
		}
	};
	BeginTaskbarProgress();
	auto id = dialog.DoModal(hwnd);
	EndTaskbarProgress();
	if (id == IDOK && solutions.size()) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		m_board = solutions[0];
		m_solution = solutions[0];
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		FinalizeBoard();
		UpdateScrollInfo(0, 0);
		InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		if (m_bAutoSave)
			NumberingSave(hwnd, FALSE, bNoGUI);
		MyPlaySound(MY_SOUND_SUCCESS);
		if (!bNoGUI && !m_bNoGeneratedMsg)
			ShowResult(IDS_MADEPROBLEM, GetTickCount64() - dialog.m_nStartTick, 1);
	} else {
		if (dialog.m_hr == E_ABORT) {
			MyPlaySound(MY_SOUND_CANCELED);
			if (!bNoGUI && !m_bNoCanceledMsg)
				ShowResult(IDS_CANCELLED, GetTickCount64() - dialog.m_nStartTick);
		} else if (dialog.m_hr == S_FALSE) {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowResult(IDS_CANTMAKEPROBLEM, GetTickCount64() - dialog.m_nStartTick);
			EndTaskbarProgress();
		} else {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowError(dialog.m_hr, strError);
			EndTaskbarProgress();
		}
	}
}

// 解を求める（黒マス追加なし）。
void MMainWnd::SolveNoAddBlack(HWND hwnd, BOOL bNoGUI) {
	if (m_bSolved)
		return;

	DestroyMarksDlg();
	DestroyCandsWnd();
	DestroyCluesWnd();

	auto board = m_board;
	std::vector<MBoard> solutions;
	MStringW strError;

	MCancelDialog dialog;
	dialog.m_fnRun = [&]() -> HRESULT {
		return XgAlgo2(m_dict, board, solutions, strError, 1);
	};
	dialog.m_fnCancel = []() -> void {
		XgAlgo2Cancel();
	};
	dialog.m_fnEnd = [&](HRESULT hr) -> void {
		if (FAILED(hr)) {
			EndDialog(dialog, IDCANCEL);
		} else {
			EndDialog(dialog, IDOK);
		}
	};
	BeginTaskbarProgress();
	auto id = dialog.DoModal(hwnd);
	EndTaskbarProgress();
	if (id == IDOK && solutions.size()) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		m_board = solutions[0];
		m_solution = solutions[0];
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		FinalizeBoard();
		UpdateScrollInfo(0, 0);
		InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		if (m_bAutoSave)
			NumberingSave(hwnd, FALSE, bNoGUI);
		MyPlaySound(MY_SOUND_SUCCESS);
		if (!bNoGUI && !m_bNoGeneratedMsg)
			ShowResult(IDS_MADEPROBLEM, GetTickCount64() - dialog.m_nStartTick, 1);
	} else {
		if (dialog.m_hr == E_ABORT) {
			MyPlaySound(MY_SOUND_CANCELED);
			if (!bNoGUI && !m_bNoCanceledMsg)
				ShowResult(IDS_CANCELLED, GetTickCount64() - dialog.m_nStartTick);
		} else if (dialog.m_hr == S_FALSE) {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowResult(IDS_CANTMAKEPROBLEM, GetTickCount64() - dialog.m_nStartTick);
			EndTaskbarProgress();
		} else {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowError(dialog.m_hr, strError);
			EndTaskbarProgress();
		}
	}
}

// 黒マスパターンを生成。
void MMainWnd::GenerateBlacks(HWND hwnd, INT nMaxWordLen, BOOL bNoGUI) {
	DestroyMarksDlg();
	DestroyCandsWnd();
	DestroyCluesWnd();

	auto board = m_board;
	board.clear();

	MCancelDialog dialog;
	std::vector<MBoard> solutions;
	MStringW strError;
	dialog.m_fnRun = [&]() -> HRESULT {
		return XgAlgo1(m_dict, board, nMaxWordLen, solutions, strError, FALSE, 1);
	};
	dialog.m_fnCancel = []() -> void {
		XgAlgo1Cancel();
	};
	dialog.m_fnEnd = [&](HRESULT hr) -> void {
		if (FAILED(hr)) {
			EndDialog(dialog, IDCANCEL);
		} else {
			EndDialog(dialog, IDOK);
		}
	};
	BeginTaskbarProgress();
	auto id = dialog.DoModal(hwnd);
	EndTaskbarProgress();
	if (id == IDOK && solutions.size()) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		m_bSolved = FALSE;
		m_board = solutions[0];
		m_solution = solutions[0];
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		m_vHorzInfo.clear();
		m_vVertInfo.clear();
		m_vMarks.clear();
		m_display.m_mapNumCro1.clear();
		m_display.m_mapNumCro2.clear();
		m_bNumCroMode = FALSE;
		m_word2clue.clear();
		UpdateScrollInfo(0, 0);
		InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		if (m_bAutoSave)
			NumberingSave(hwnd, TRUE, bNoGUI);
		MyPlaySound(MY_SOUND_SUCCESS);
		if (!bNoGUI && !m_bNoGeneratedMsg) {
			ShowResult(IDS_BLOCKSGENERATED, GetTickCount64() - dialog.m_nStartTick, 1);
		}
	} else {
		if (dialog.m_hr == E_ABORT) {
			MyPlaySound(MY_SOUND_CANCELED);
			if (!bNoGUI && !m_bNoCanceledMsg) {
				ShowResult(IDS_CANCELLED, GetTickCount64() - dialog.m_nStartTick);
			}
		} else if (dialog.m_hr == S_FALSE) {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowResult(IDS_CANTMAKEPATTERN, GetTickCount64() - dialog.m_nStartTick);
			EndTaskbarProgress();
		} else {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowError(dialog.m_hr, strError);
			EndTaskbarProgress();
		}
	}
}

void MMainWnd::DestroyCandsWnd() {
	if (m_hCandsWnd)
		::DestroyWindow(m_hCandsWnd);
}

// ルールを設定。
void MMainWnd::SetRules(INT nRules) {
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_RULES, 0);
	SetRules0(nRules);
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_RULES, 0);
	UpdateStatusBar();
	UpdateTitleBarText();
}

// ルールを設定（内部用）。
void MMainWnd::SetRules0(INT nRules) {
	xg_nRules = nRules | RULE_DONTDIVIDE;
	m_board.Rules() = xg_nRules;
	m_solution.Rules() = xg_nRules;
}

// ルールを切り替える。
void MMainWnd::ToggleRules(INT flags, INT mask) {
	INT rules = xg_nRules;
	if ((rules & mask) == flags) {
		rules &= ~mask;
	} else {
		rules &= ~mask;
		rules |= flags;
	}
	SetRules(rules);
}

// 答え合わせ。
void MMainWnd::CheckAnswer() {
	if (!m_bSolved || !m_bCheckingAnswer)
		return;

	// 答えと一致するか確認。
	for (INT iRow = 0; iRow < m_board.Rows(); ++iRow) {
		for (INT jCol = 0; jCol < m_board.Columns(); ++jCol) {
			if (m_board.GetCell(jCol, iRow) != m_solution.GetCell(jCol, iRow)) {
				return; // 間違い。
			}
		}
	}

	// 正解ですと表示。
	MsgBoxDx(IDS_CONGRATS, IDS_CORRECTANSWER, MB_ICONINFORMATION);
}

// 二重マス単語ダイアログを作成。
BOOL MMainWnd::CreateMarksDlg() {
	return m_hMarksDlg.CreateDx(m_hwnd);
}

// 二重マス単語ダイアログを破棄。
void MMainWnd::DestroyMarksDlg() {
	if (m_hMarksDlg)
		::PostMessageW(m_hMarksDlg, WM_COMMAND, IDCANCEL, 0);
}

// 結果を表示。
void MMainWnd::ShowResult(INT ids, ULONGLONG time) {
	WCHAR text[256];
	StringCchPrintfW(text, _countof(text), XgLoadString(ids).c_str(),
					 DWORD(time) / 1000, DWORD(time) / 100 % 10);
	MsgBoxDx(text, XgLoadString(IDS_RESULTS).c_str(), MB_ICONINFORMATION);
}

// 結果を表示（複数可）。
void MMainWnd::ShowResult(INT ids, ULONGLONG time, size_t nCount) {
	WCHAR text[256];
	StringCchPrintfW(text, _countof(text), XgLoadString(ids).c_str(),
					 UINT(nCount), DWORD(time) / 1000, DWORD(time) / 100 % 10);
	MsgBoxDx(text, XgLoadString(IDS_RESULTS).c_str(), MB_ICONINFORMATION);
}

void MMainWnd::ShowError(HRESULT hr, const MStringW& strError) {
	assert(FAILED(hr));

	if (hr == E_OUTOFMEMORY) {
		WCHAR text[128];
		LoadStringW(xg_hInstance, IDS_OUTOFMEMORY, text, _countof(text));
		MessageBoxW(m_hwnd, text, nullptr, MB_ICONERROR);
		return;
	}

	if (hr == E_INVALIDARG) {
		if (strError == L"dict") {
			MsgBoxDx(strError.c_str(), MB_ICONERROR);
			return;
		}
		if (strError == L"duplicate") {
			ErrorBoxDx(IDS_WORDDUPLICATE);
			return;
		}
		if (strError == L"rules") {
			if (RuleCheck(m_hwnd, FALSE, FALSE))
				ErrorBoxDx(IDS_UNKNOWNERROR);
			return;
		}
		if (strError == L"pattern") {
			ErrorBoxDx(IDS_WRONGPATTERN);
			return;
		}
		if (strError == L"word") {
			ErrorBoxDx(IDS_INVALIDWORDS);
			return;
		}
	}

	ErrorBoxDx(IDS_UNKNOWNERROR);
}

// ヒントウィンドウを作成する。
BOOL MMainWnd::CreateCluesWnd(HWND hwnd) {
	m_hCluesWnd.m_hwndCanvas = m_hCanvasWnd;
	return m_hCluesWnd.CreateDx(hwnd);
}

// ヒントウィンドウを破棄する。
void MMainWnd::DestroyCluesWnd() {
	if (m_hCluesWnd) {
		::DestroyWindow(m_hCluesWnd);
	}
}

// 盤面を仕上げる。
HRESULT MMainWnd::FinalizeBoard() {
	DestroyCluesWnd();

	HRESULT hr = XgFinalizeBoard(m_display, m_dict, m_word2clue);
	if (FAILED(hr))
		return hr;

	CreateCluesWnd(m_hwnd);

	// ヒントウィンドウのハイライトを設定する。
	PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT2, 0, 0);
	return S_OK;
}

// ID_RULECHECK
BOOL MMainWnd::RuleCheck(HWND hwnd, BOOL bMessageOnSuccess, BOOL bLoose) {
	auto& board = XgGetDisplayBoard(m_display);
	const INT nRules = board.Rules();

	// 連黒禁。
	if ((nRules & RULE_DONTDOUBLEBLACK) && board.DoubleBlack()) {
		ErrorBoxDx(IDS_ADJACENTBLOCK);
		return FALSE;
	}
	// 四隅黒禁。
	if ((nRules & RULE_DONTCORNERBLACK) && board.CornerBlack()) {
		ErrorBoxDx(IDS_CORNERBLOCK);
		return FALSE;
	}
	// 三方黒禁。
	if ((nRules & RULE_DONTTRIDIRECTIONS) && board.TriBlackAround()) {
		ErrorBoxDx(IDS_TRIBLOCK);
		return FALSE;
	}
	// 分断禁。
	if ((nRules & RULE_DONTDIVIDE) && board.DividedByBlack()) {
		ErrorBoxDx(IDS_DIVIDED);
		return FALSE;
	}
	// 黒斜三連禁。
	if ((nRules & RULE_DONTTHREEDIAGONALS) && board.ThreeDiagonals()) {
		ErrorBoxDx(IDS_THREEDIAGONALS);
		return FALSE;
	} else {
		// 黒斜四連禁。
		if ((nRules & RULE_DONTFOURDIAGONALS) && board.FourDiagonals()) {
			ErrorBoxDx(IDS_FOURDIAGONALS);
			return FALSE;
		}
	}

	if (!bLoose) { // 判定がゆるくなければ、対称もチェックする。
		// 黒マス点対称。
		if ((nRules & RULE_POINTSYMMETRY) && !board.IsPointSymmetry()) {
			ErrorBoxDx(IDS_NOTPOINTSYMMETRY);
			return FALSE;
		}
		// 黒マス線対称。
		if ((nRules & RULE_LINESYMMETRYV) && !board.IsLineSymmetryV()) {
			ErrorBoxDx(IDS_NOTLINESYMMETRYV);
			return FALSE;
		}
		if ((nRules & RULE_LINESYMMETRYH) && !board.IsLineSymmetryH()) {
			ErrorBoxDx(IDS_NOTLINESYMMETRYH);
			return FALSE;
		}
	}

	// 合格。
	if (bMessageOnSuccess) {
		MsgBoxDx(IDS_RULESPASSED, IDS_PASSED, MB_ICONINFORMATION);
	}
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Toolbar

void MMainWnd::UpdateToolBarUI() {
	::SendMessageW(m_hToolBar, TB_ENABLEBUTTON, ID_SOLVE, !m_bSolved);
	::SendMessageW(m_hToolBar, TB_ENABLEBUTTON, ID_SOLVENOADDBLACK, !m_bSolved);
	::SendMessageW(m_hToolBar, TB_ENABLEBUTTON, ID_PRINTANSWER, m_bSolved);
	::SendMessageW(m_hToolBar, TB_ENABLEBUTTON, ID_UNDO, CanUndo());
	::SendMessageW(m_hToolBar, TB_ENABLEBUTTON, ID_REDO, CanRedo());
}

BOOL MMainWnd::CreateToolBar(HWND hwnd) {
	// イメージリストの準備をする。
	m_hImageList = ::ImageList_Create(32, 32, ILC_COLOR24, 0, 0);
	if (!m_hImageList)
		return FALSE;
	m_hGrayedImageList = ::ImageList_Create(32, 32, ILC_COLOR24, 0, 0);
	if (!m_hGrayedImageList)
		return FALSE;

	::ImageList_SetBkColor(m_hImageList, CLR_NONE);
	::ImageList_SetBkColor(m_hGrayedImageList, CLR_NONE);

	HBITMAP hbm = static_cast<HBITMAP>(::LoadImageW(
		xg_hInstance, MAKEINTRESOURCE(1),
		IMAGE_BITMAP,
		0, 0,
		LR_COLOR));
	::ImageList_Add(m_hImageList, hbm, nullptr);
	HBITMAP hbmGrayed = XgCreateGrayedBitmap(hbm);
	::ImageList_Add(m_hGrayedImageList, hbmGrayed, nullptr);
	::DeleteObject(hbm);
	::DeleteObject(hbmGrayed);

	// ツールバーのボタン情報。
	static const TBBUTTON atbb[] = {
		{0, ID_NEW, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{1, ID_OPEN, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{2, ID_SAVEAS, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
		{14, ID_UNDO, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{15, ID_REDO, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
		{6, ID_COPY, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{7, ID_PASTE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
		{3, ID_GENERATE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
		{8, ID_SOLVE, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{9, ID_SOLVENOADDBLACK, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{0, 0, TBSTATE_ENABLED, TBSTYLE_SEP},
		{12, ID_PRINTPROBLEM, TBSTATE_ENABLED, TBSTYLE_BUTTON},
		{13, ID_PRINTANSWER, TBSTATE_ENABLED, TBSTYLE_BUTTON},
	};

	// ツールバーを作成する。
	m_hToolBar = ::CreateWindowW(
		TOOLBARCLASSNAMEW, nullptr, 
		WS_CHILD | CCS_TOP | TBSTYLE_TOOLTIPS | WS_CLIPSIBLINGS,
		0, 0, 0, 0,
		hwnd,
		reinterpret_cast<HMENU>(static_cast<UINT_PTR>(IDW_TOOLBAR)),
		xg_hInstance,
		nullptr);
	if (!m_hToolBar)
		return FALSE;

	// ツールバーを初期化する。
	::SendMessageW(m_hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	::SendMessageW(m_hToolBar, TB_SETBITMAPSIZE, 0, MAKELPARAM(32, 32));
	::SendMessageW(m_hToolBar, TB_SETIMAGELIST, 0, LPARAM(m_hImageList));
	::SendMessageW(m_hToolBar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)m_hGrayedImageList);
	::SendMessageW(m_hToolBar, TB_ADDBUTTONS, std::size(atbb), LPARAM(&atbb));
	::SendMessageW(m_hToolBar, WM_SIZE, 0, 0);

	ShowToolBar(m_bShowToolBar);
	return TRUE;
}

void MMainWnd::ShowToolBar(BOOL bShow) {
	::ShowWindow(m_hToolBar, (bShow ? SW_SHOWNOACTIVATE : SW_HIDE));
	m_bShowToolBar = bShow;
	PostMessageW(m_hwnd, WM_SIZE, 0, 0);
}

void MMainWnd::DestroyToolBar() {
	if (m_hToolBar) {
		DestroyWindow(m_hToolBar);
		m_hToolBar = nullptr;
	}
	// イメージリストを破棄する。
	if (m_hImageList) {
		::ImageList_Destroy(m_hImageList);
		m_hImageList = nullptr;
	}
	if (m_hGrayedImageList) {
		::ImageList_Destroy(m_hGrayedImageList);
		m_hGrayedImageList = nullptr;
	}
}

// 設定をリセットする。
void MMainWnd::ResetSettings() {
	m_display.Reset();
	m_nWindowX = CW_USEDEFAULT;
	m_nWindowY = CW_USEDEFAULT;
	m_nWindowCX = CW_USEDEFAULT;
	m_nWindowCY = CW_USEDEFAULT;
	m_hCluesWnd.m_nWindowX = CW_USEDEFAULT;
	m_hCluesWnd.m_nWindowY = CW_USEDEFAULT;
	m_hCluesWnd.m_nWindowCX = 420;
	m_hCluesWnd.m_nWindowCY = 250;
	m_hMarksDlg.m_nWindowX = CW_USEDEFAULT;
	m_hMarksDlg.m_nWindowY = CW_USEDEFAULT;
	m_ai_helper.m_nX = CW_USEDEFAULT;
	m_ai_helper.m_nY = CW_USEDEFAULT;
	m_ai_helper.m_nCX = CW_USEDEFAULT;
	m_ai_helper.m_nCY = CW_USEDEFAULT;
	MInputPalette::s_nWindowX = CW_USEDEFAULT;
	MInputPalette::s_nWindowY = CW_USEDEFAULT;
	m_hMarksDlg.m_nWindowX = CW_USEDEFAULT;
	m_hMarksDlg.m_nWindowY = CW_USEDEFAULT;

	m_bShowToolBar = TRUE;
	m_bShowStatusBar = TRUE;
	m_imode = xg_im_ANY;
	m_bCharFeed = TRUE;
	m_bVertInput = FALSE;
	m_bShowInputPalette = FALSE;

	GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(m_lfCandsLogFont), &m_lfCandsLogFont);
	GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(m_lfCluesLogFont), &m_lfCluesLogFont);
	m_recently_used_files.clear();
	GetObjectW(GetStockFont(DEFAULT_GUI_FONT), sizeof(m_lfAIHelper), &m_lfAIHelper);

	m_bShowAnswerOnOpen = FALSE;
	m_bShowAnswerOnGenerate = TRUE;
	m_bAutoSave = FALSE;
	m_bNoGeneratedMsg = FALSE;
	m_bNoCanceledMsg = FALSE;
	m_bNoReadLooks = FALSE;
	m_bNoWriteLooks = FALSE;

	StringCchCopyW(m_szNumberingFileName1, _countof(m_szNumberingFileName1), L"Crossword-%Wx%H-%4N.xd");
	StringCchCopyW(m_szNumberingFileName2, _countof(m_szNumberingFileName2), L"Pattern-%Wx%H-%4N.xd");

	LPITEMIDLIST pidl;
	WCHAR szPath[MAX_PATH];
	::SHGetSpecialFolderLocation(nullptr, CSIDL_PERSONAL, &pidl);
	::SHGetPathFromIDListW(pidl, szPath);
	::CoTaskMemFree(pidl);
	PathAppendW(szPath, L"XWordGiver");
	m_dirs_save_to.clear();
	m_dirs_save_to.emplace_back(szPath);

	// 音声
	ZeroMemory(m_aszSoundFiles, sizeof(m_aszSoundFiles));

	// AI関連。
	m_ai_helper.Reset();
	m_bServerAccessConfirmed = FALSE;
}

// 設定を読み込む。
BOOL MMainWnd::LoadSettings() {
	ResetSettings();
	m_display.Load();

	// アプリ名キーを開く。
	MRegKey app_key(HKEY_CURRENT_USER, XG_REGKEY_APP, FALSE);
	if (!app_key)
		return FALSE;

	DWORD dwValue;
	WCHAR text[512];

	// 言語設定は最初に取得して設定する。
	if (!app_key.QueryDword(L"UILangID", dwValue)) {
		m_wUILangID = LANGID(dwValue);
		SetThreadUILanguage(m_wUILangID);
	}

	// ウィンドウの位置とサイズ。
	if (!app_key.QueryDword(L"WindowX", dwValue)) {
		m_nWindowX = dwValue;
	}
	if (!app_key.QueryDword(L"WindowY", dwValue)) {
		m_nWindowY = dwValue;
	}
	if (!app_key.QueryDword(L"WindowCX", dwValue)) {
		m_nWindowCX = dwValue;
	}
	if (!app_key.QueryDword(L"WindowCY", dwValue)) {
		m_nWindowCY = dwValue;
	}
	if (!app_key.QueryDword(L"MainWndMaximized", dwValue)) {
		m_bMaximized = !!dwValue;
	}
	if (!app_key.QueryDword(L"PatWndX", dwValue)) {
		MPatternDialog::s_nWindowX = dwValue;
	}
	if (!app_key.QueryDword(L"PatWndY", dwValue)) {
		MPatternDialog::s_nWindowY = dwValue;
	}
	if (!app_key.QueryDword(L"PatWndCX", dwValue)) {
		MPatternDialog::s_nWindowCX = dwValue;
	}
	if (!app_key.QueryDword(L"PatWndCY", dwValue)) {
		MPatternDialog::s_nWindowCY = dwValue;
	}
	if (!app_key.QueryDword(L"HintsX", dwValue)) {
		m_hCluesWnd.m_nWindowX = dwValue;
	}
	if (!app_key.QueryDword(L"HintsY", dwValue)) {
		m_hCluesWnd.m_nWindowY = dwValue;
	}
	if (!app_key.QueryDword(L"HintsCX", dwValue)) {
		m_hCluesWnd.m_nWindowCX = dwValue;
	}
	if (!app_key.QueryDword(L"HintsCY", dwValue)) {
		m_hCluesWnd.m_nWindowCY = dwValue;
	}
	if (!app_key.QueryDword(L"MarksX", dwValue)) {
		m_hMarksDlg.m_nWindowX = dwValue;
	}
	if (!app_key.QueryDword(L"MarksY", dwValue)) {
		m_hMarksDlg.m_nWindowY = dwValue;
	}
	if (!app_key.QueryDword(L"HelperX", dwValue)) {
		m_ai_helper.m_nX = dwValue;
	}
	if (!app_key.QueryDword(L"HelperY", dwValue)) {
		m_ai_helper.m_nY = dwValue;
	}
	if (!app_key.QueryDword(L"HelperCX", dwValue)) {
		m_ai_helper.m_nCX = dwValue;
	}
	if (!app_key.QueryDword(L"HelperCY", dwValue)) {
		m_ai_helper.m_nCY = dwValue;
	}
	if (!app_key.QueryDword(L"IPaletteX", dwValue)) {
		MInputPalette::s_nWindowX = dwValue;
	}
	if (!app_key.QueryDword(L"IPaletteY", dwValue)) {
		MInputPalette::s_nWindowY = dwValue;
	}
	if (!app_key.QueryDword(L"MarkingX", dwValue)) {
		m_hMarksDlg.m_nWindowX = dwValue;
	}
	if (!app_key.QueryDword(L"MarkingY", dwValue)) {
		m_hMarksDlg.m_nWindowY = dwValue;
	}

	// コントロールの表示切替。
	if (!app_key.QueryDword(L"ShowToolBar", dwValue)) {
		m_bShowToolBar = !!dwValue;
	}
	if (!app_key.QueryDword(L"ShowStatusBar", dwValue)) {
		m_bShowStatusBar = !!dwValue;
	}

	// 入力関連。
	if (!app_key.QueryDword(L"CharFeed", dwValue)) {
		m_bCharFeed = !!dwValue;
	}
	if (!app_key.QueryDword(L"VertInput", dwValue)) {
		m_bVertInput = !!dwValue;
	}
	if (!app_key.QueryDword(L"ShowInputPalette", dwValue)) {
		m_bShowInputPalette = !!dwValue;
	}
	if (!app_key.QueryDword(L"VerticalLayout", dwValue)) {
		m_bVerticalLayout = !!dwValue;
	}
	if (!app_key.QueryDword(L"InputMode", dwValue)) {
		m_imode = (MInputMode)dwValue;
	}

	// 論理フォント。
	app_key.QueryStruct(L"CandsLogFont", m_lfCandsLogFont);
	app_key.QueryStruct(L"CluesLogFont", m_lfCluesLogFont);
	app_key.QueryStruct(L"AIHelperLogFont", m_lfAIHelper);

	// 細かい設定。
	if (!app_key.QueryDword(L"ShowAnswerOnOpen", dwValue)) {
		m_bShowAnswerOnOpen = !!dwValue;
	}
	if (!app_key.QueryDword(L"ShowAnswerOnGenerate", dwValue)) {
		m_bShowAnswerOnGenerate = !!dwValue;
	}
	if (!app_key.QueryDword(L"AutoSave", dwValue)) {
		m_bAutoSave = !!dwValue;
	}
	if (!app_key.QueryDword(L"NoGeneratedMsg", dwValue)) {
		m_bNoGeneratedMsg = !!dwValue;
	}
	if (!app_key.QueryDword(L"NoCanceledMsg", dwValue)) {
		m_bNoCanceledMsg = !!dwValue;
	}
	if (!app_key.QueryDword(L"NoReadLooks", dwValue)) {
		m_bNoReadLooks = !!dwValue;
	}
	if (!app_key.QueryDword(L"NoWriteLooks", dwValue)) {
		m_bNoWriteLooks = !!dwValue;
	}

	// 連番ファイル名。
	if (!app_key.QuerySz(L"NumberingFilename1", text, _countof(text))) {
		StringCchCopyW(m_szNumberingFileName1, _countof(m_szNumberingFileName1), text);
	}
	if (!app_key.QuerySz(L"NumberingFilename2", text, _countof(text))) {
		StringCchCopyW(m_szNumberingFileName2, _countof(m_szNumberingFileName2), text);
	}

	// 保存先のリストを取得する。
	if (!app_key.QueryDword(L"SaveToCount", dwValue)) {
		m_dirs_save_to.clear();
		INT nDirCount = dwValue;
		for (INT i = 0; i < nDirCount; i++) {
			WCHAR szFormat[64];
			StringCchPrintf(szFormat, _countof(szFormat), L"SaveTo %d", i + 1);
			if (!app_key.QuerySz(szFormat, text, _countof(text))) {
				m_dirs_save_to.emplace_back(text);
			} else {
				break;
			}
		}
	}

	// 保存先のリストが空だったら初期化する。
	if (m_dirs_save_to.empty()) {
		LPITEMIDLIST pidl;
		WCHAR szPath[MAX_PATH];
		::SHGetSpecialFolderLocation(nullptr, CSIDL_PERSONAL, &pidl);
		::SHGetPathFromIDListW(pidl, szPath);
		::CoTaskMemFree(pidl);
		PathAppendW(szPath, L"XWordGiver");
		m_dirs_save_to.clear();
		m_dirs_save_to.emplace_back(szPath);
	}

	// 最近使ったファイルのリストを取得する。
	if (!app_key.QueryDword(L"Recents", dwValue)) {
		m_recently_used_files.clear();
		for (INT i = 0; i < INT(dwValue); ++i) {
			WCHAR szFormat[64];
			StringCchPrintfW(szFormat, _countof(szFormat), L"Recent %d", i);
			WCHAR szFile[MAX_PATH];
			if (!app_key.QuerySz(szFormat, szFile, _countof(szFile))) {
				if (PathFileExistsW(szFile))
					m_recently_used_files.emplace_back(szFile);
			} else {
				break;
			}
		}
		if (m_recently_used_files.size() > XG_MAX_RECENT)
			m_recently_used_files.resize(XG_MAX_RECENT);
	}

	// 音声。
	if (!app_key.QuerySz(L"SoundFile0", text, _countof(text))) {
		StringCchCopyW(m_aszSoundFiles[0], _countof(m_aszSoundFiles[0]), text);
	}
	if (!app_key.QuerySz(L"SoundFile1", text, _countof(text))) {
		StringCchCopyW(m_aszSoundFiles[1], _countof(m_aszSoundFiles[1]), text);
	}
	if (!app_key.QuerySz(L"SoundFile2", text, _countof(text))) {
		StringCchCopyW(m_aszSoundFiles[2], _countof(m_aszSoundFiles[2]), text);
	}

	// AI関連。
	if (!app_key.QuerySz(L"AIProvider", text, _countof(text))) {
		auto& str = m_ai_helper.m_provider;
		str = text;
		// 以前、プロバイダ名を間違っていたのでここで修正。
		if (str == L"gemini")
			str = L"google";
		if (str == L"grok")
			str = L"xai";
		if (str == L"kimi")
			str = L"moonshot";
		if (str == L"chatgpt")
			str = L"openai";
		if (str == L"claude")
			str = L"anthropic";
		if (str == L"qwen")
			str = L"dashscope";
	}
	if (!app_key.QuerySz(L"AIModel", text, _countof(text))) {
		m_ai_helper.m_model = text;
	}
	// プロンプトの読み込み。
	if (!app_key.QueryDword(L"PromptTextCount", dwValue) && dwValue == (DWORD)PROMPT_TYPE_MAX) {
		m_ai_helper.m_promptMapping.clear();
		for (INT i = 0; i < INT(dwValue); ++i) {
			WCHAR szFormat[64];
			StringCchPrintf(szFormat, _countof(szFormat), L"PromptText %d", i);
			LPWSTR sz = (LPWSTR)malloc(MAX_PROMPT_TEXT); // プロンプトは長くなりがち。
			if (!app_key.QuerySz(szFormat, sz, MAX_PROMPT_TEXT)) {
				m_ai_helper.m_promptMapping.emplace_back(sz);
				free(sz);
			} else {
				free(sz);
				break;
			}
		}
	}
	// サーバーへのアクセスを確認したか？
	if (!app_key.QueryDword(L"ServerAccessConfirmed", dwValue)) {
		m_bServerAccessConfirmed = !!dwValue;
	}

	// プロンプトの個数が変わっているか？
	BOOL bPromptSizeChange = 
		(m_ai_helper.m_promptMapping.size() != (size_t)PROMPT_TYPE_MAX);

	// プロンプトの言語が変わっているか？
	BOOL bPromptLangChanged = TRUE;
	if (!app_key.QueryDword(L"PromptLangID", dwValue)) {
		bPromptLangChanged = (LANGID)dwValue != GetThreadUILanguage();
	}

	// 製品バージョンが変わっているか？
	BOOL bVersionChange = TRUE;
	if (!app_key.QuerySz(L"ProductVersion", text, _countof(text))) {
		bVersionChange = (lstrcmpW(text, XWORDGIVER_VERSION_W) != 0);
	}

	// 変わっていたら、プロンプトをリセットする。
	if (bPromptSizeChange || bPromptLangChanged || bVersionChange) {
		m_ai_helper.ResetPrompts(m_ai_helper.m_promptMapping);
	}

	return TRUE;
}

// 設定を保存する。
BOOL MMainWnd::SaveSettings() {
	m_display.Save();

	// アプリ名キーを開く。キーがなければ作成する。
	MRegKey app_key(HKEY_CURRENT_USER, XG_REGKEY_APP, TRUE);
	if (!app_key)
		return FALSE;

	// UI言語
	app_key.SetDword(L"UILangID", m_wUILangID);

	// ウィンドウの位置とサイズ。
	app_key.SetDword(L"WindowX", m_nWindowX);
	app_key.SetDword(L"WindowY", m_nWindowY);
	app_key.SetDword(L"WindowCX", m_nWindowCX);
	app_key.SetDword(L"WindowCY", m_nWindowCY);
	app_key.SetDword(L"MainWndMaximized", !!m_bMaximized);
	app_key.SetDword(L"PatWndX", MPatternDialog::s_nWindowX);
	app_key.SetDword(L"PatWndY", MPatternDialog::s_nWindowY);
	app_key.SetDword(L"PatWndCX", MPatternDialog::s_nWindowCX);
	app_key.SetDword(L"PatWndCY", MPatternDialog::s_nWindowCY);
	app_key.SetDword(L"HintsX", m_hCluesWnd.m_nWindowX);
	app_key.SetDword(L"HintsY", m_hCluesWnd.m_nWindowY);
	app_key.SetDword(L"HintsCX", m_hCluesWnd.m_nWindowCX);
	app_key.SetDword(L"HintsCY", m_hCluesWnd.m_nWindowCY);
	app_key.SetDword(L"MarksX", m_hMarksDlg.m_nWindowX);
	app_key.SetDword(L"MarksY", m_hMarksDlg.m_nWindowY);
	app_key.SetDword(L"HelperX", m_ai_helper.m_nX);
	app_key.SetDword(L"HelperY", m_ai_helper.m_nY);
	app_key.SetDword(L"HelperCX", m_ai_helper.m_nCX);
	app_key.SetDword(L"HelperCY", m_ai_helper.m_nCY);
	app_key.SetDword(L"IPaletteX", MInputPalette::s_nWindowX);
	app_key.SetDword(L"IPaletteY", MInputPalette::s_nWindowY);
	app_key.SetDword(L"MarkingX", m_hMarksDlg.m_nWindowX);
	app_key.SetDword(L"MarkingY", m_hMarksDlg.m_nWindowY);

	// コントロールの表示切替。
	app_key.SetDword(L"ShowToolBar", m_bShowToolBar);
	app_key.SetDword(L"ShowStatusBar", m_bShowStatusBar);

	// 入力関連。
	app_key.SetDword(L"CharFeed", !!m_bCharFeed);
	app_key.SetDword(L"VertInput", !!m_bVertInput);
	app_key.SetDword(L"ShowInputPalette", !!m_bShowInputPalette);
	app_key.SetDword(L"VerticalLayout", m_bVerticalLayout);
	app_key.SetDword(L"InputMode", static_cast<DWORD>(m_imode));

	// 論理フォント。
	app_key.SetStruct(L"CandsLogFont", m_lfCandsLogFont);
	app_key.SetStruct(L"CluesLogFont", m_lfCluesLogFont);
	app_key.SetStruct(L"AIHelperLogFont", m_lfAIHelper);

	// 細かい設定。
	app_key.SetDword(L"ShowAnswerOnOpen", m_bShowAnswerOnOpen);
	app_key.SetDword(L"ShowAnswerOnGenerate", m_bShowAnswerOnGenerate);
	app_key.SetDword(L"AutoSave", m_bAutoSave);
	app_key.SetDword(L"NoGeneratedMsg", m_bNoGeneratedMsg);
	app_key.SetDword(L"NoCanceledMsg", m_bNoCanceledMsg);
	app_key.SetDword(L"NoReadLooks", m_bNoReadLooks);
	app_key.SetDword(L"NoWriteLooks", m_bNoWriteLooks);

	// 連番ファイル名。
	app_key.SetSz(L"NumberingFilename1", m_szNumberingFileName1);
	app_key.SetSz(L"NumberingFilename2", m_szNumberingFileName2);

	// 保存先のリストを設定する。
	INT nCount = INT(m_dirs_save_to.size());
	app_key.SetDword(L"SaveToCount", nCount);
	for (INT i = 0; i < nCount; i++) {
		WCHAR szFormat[64];
		StringCchPrintf(szFormat, _countof(szFormat), L"SaveTo %d", i + 1);
		app_key.SetSz(szFormat, m_dirs_save_to[i].c_str());
	}

	// 最近使ったファイルのリストを設定する。
	nCount = INT(m_recently_used_files.size());
	app_key.SetDword(L"Recents", nCount);
	for (INT i = 0; i < nCount; i++) {
		WCHAR szFormat[64];
		StringCchPrintfW(szFormat, _countof(szFormat), L"Recent %d", i);
		app_key.SetSz(szFormat, m_recently_used_files[i].c_str());
	}

	// 音声。
	app_key.SetSz(L"SoundFile0", m_aszSoundFiles[0]);
	app_key.SetSz(L"SoundFile1", m_aszSoundFiles[1]);
	app_key.SetSz(L"SoundFile2", m_aszSoundFiles[2]);

	// AI関連。
	app_key.SetSz(L"AIProvider", m_ai_helper.m_provider.c_str());
	app_key.SetSz(L"AIModel", m_ai_helper.m_model.c_str());
	nCount = INT(m_ai_helper.m_promptMapping.size());
	app_key.SetDword(L"PromptTextCount", nCount);
	app_key.SetDword(L"PromptLangID", GetThreadUILanguage());
	for (INT i = 0; i < nCount; i++) {
		WCHAR szFormat[64];
		StringCchPrintf(szFormat, _countof(szFormat), L"PromptText %d", i);
		app_key.SetSz(szFormat, m_ai_helper.m_promptMapping[i].c_str());
	}
	app_key.SetDword(L"ServerAccessConfirmed", m_bServerAccessConfirmed);

	// 製品番号
	app_key.SetSz(L"ProductVersion", XWORDGIVER_VERSION_W);

	return TRUE;
}

// ファイルメニューを更新。
void MMainWnd::InitFileMenu(HMENU hMenu) {
	if (GetMenuPosFromID(hMenu, ID_NEW) == -1)
		return; // ファイルメニューではない。

	const INT nRecentOffset = 7; // TODO: 「ファイル」メニューを変更したら、ここを調整。
	INT nCount = GetMenuItemCount(hMenu);
	HMENU hRecentMenu = GetSubMenu(hMenu, nCount - nRecentOffset);
	if (hRecentMenu && GetMenuPosFromID(hRecentMenu, ID_RECENT_00) != -1) { // 最近使ったファイルか？
		// 最近使ったファイルのメニュー項目をすべて削除。
		INT id2 = ID_RECENT_00;
		while (::DeleteMenu(hRecentMenu, id2++, MF_BYCOMMAND))
			;

		// 最近使ったファイルのメニュー項目を新しく追加。
		INT id = ID_RECENT_00, iItem = 0;
		for (auto& item : m_recently_used_files) {
			MStringW str;
			str += L'&';
			str += static_cast<WCHAR>(L'0' + iItem);
			str += L' ';
			str += item;
			::AppendMenuW(hRecentMenu, MF_STRING, id++, str.c_str());
			++iItem;
		}
		// 最近使ったファイルが空の場合。
		if (m_recently_used_files.empty()) {
			::AppendMenuW(hRecentMenu, MF_STRING | MF_GRAYED, ID_RECENT_00, XgLoadString(IDS_NONE).c_str());
		}
	}
}

// 辞書メニューを更新。
void MMainWnd::InitDictMenu(HMENU hMenu) {
	if (GetMenuPosFromID(hMenu, ID_DICTIONARY00) == -1)
		return; // 辞書メニューではない。

	// 辞書項目をすべて削除する。
	for (INT id = ID_DICTIONARY00; id <= ID_DICTIONARY63; ++id) {
		if (!RemoveMenu(hMenu, id, MF_BYCOMMAND))
			break;
	}

	if (m_dict_list.empty()) { // 辞書リストが空？
		// 「(なし)」を追加。
		AppendMenuW(hMenu, MF_STRING | MF_GRAYED, ID_DICTIONARY00, XgLoadString(IDS_NONE).c_str());
		return;
	}

	// 辞書項目を追加する。
	INT count = 0, id = ID_DICTIONARY00;
	for (auto& dict : m_dict_list) {
		// わかりやすい名前があれば取得する。
		MStringW friendly_name;
		if (XgIsUserJapanese()) { // 日本語か？
			if (dict.m_japanese_name.size()) {
				friendly_name = dict.m_japanese_name;
			}
		} else {
			if (dict.m_english_name.size()) {
				friendly_name = dict.m_english_name;
			}
		}

		// メニュー項目のテキストをセット。
		MStringW text;
		if (friendly_name.size() && dict.m_filename != friendly_name) {
			text = dict.m_filename;
			text += L'\t';
			text += friendly_name;
		} else {
			text = dict.m_filename;
		}

		// 番号付きのメニュー項目を追加する。
		WCHAR szText[MAX_PATH];
		StringCchPrintfW(szText, _countof(szText), L"&%c ",
						 L"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF"[count]);
		StringCchCatW(szText, _countof(szText), text.c_str());
		AppendMenuW(hMenu, MF_STRING | MF_ENABLED, id, szText);

		++count;
		++id;
	}

	// 現在の辞書にチェックマークを入れる。
	INT index = 0;
	for (auto& dict : m_dict_list) {
		if (_wcsicmp(dict.m_pathname.c_str(), m_dict_name.c_str()) == 0) {
			CheckMenuRadioItem(hMenu, ID_DICTIONARY00, ID_DICTIONARY00 + (count - 1),
							   ID_DICTIONARY00 + index, MF_BYCOMMAND);
		}
		++index;
	}
}

// 適当な辞書を選ぶ。
BOOL MMainWnd::ChooseDict() {
	m_dict.clear();

	if (!ChooseDict0())
		return FALSE;

	HRESULT hr = m_dict.load_file(m_dict_name.c_str());
	if (FAILED(hr))
		return FALSE;

	m_dict.sort_word_list();
	return TRUE;
}

// 適当な辞書を選ぶ。
BOOL MMainWnd::ChooseDict0() {
	if (m_dict_name.size() && PathFileExistsW(m_dict_name.c_str()))
		return TRUE;

	if (XgIsUserJapanese()) {
		for (auto& dict : m_dict_list) {
			if (dict.m_filename.find(L"-JPN-Kana-") != dict.m_filename.npos) {
				m_dict_name = dict.m_pathname;
				return TRUE;
			}
		}
	} else {
		for (auto& dict : m_dict_list) {
			if (dict.m_filename.find(L"-ENG-") != dict.m_filename.npos) {
				m_dict_name = dict.m_pathname;
				return TRUE;
			}
		}
	}

	if (m_dict_list.size()) {
		m_dict_name = m_dict_list[0].m_pathname;
		return TRUE;
	}

	m_dict_name.clear();
	return FALSE;
}

// 連番ファイル名を生成する。
static MStringW
XgGenerateNumberingFilename(HWND hwnd, MBoard& board, LPCWSTR pszText, LPSYSTEMTIME pLocalTime, INT iFile)
{
	WCHAR szN[32], szN1[32], szN2[32], szN3[32], szN4[32], szN5[32], szN6[32];
	WCHAR szW[32], szH[32];
	WCHAR szYear[32], szMonth[32], szDay[32];
	WCHAR szHour[32], szMinute[32], szSecond[32];
	WCHAR szComputer[64], szUser[64];
	LPCWSTR aszWeekDay[7] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };

	MStringW str = pszText;

	// 連番（%N, %1N, %2N, %3N, %4N, %5N, %6N）。
	StringCchPrintfW(szN, _countof(szN), L"%d", iFile);
	StringCchPrintfW(szN1, _countof(szN1), L"%01d", iFile);
	StringCchPrintfW(szN2, _countof(szN2), L"%02d", iFile);
	StringCchPrintfW(szN3, _countof(szN3), L"%03d", iFile);
	StringCchPrintfW(szN4, _countof(szN4), L"%04d", iFile);
	StringCchPrintfW(szN5, _countof(szN5), L"%05d", iFile);
	StringCchPrintfW(szN6, _countof(szN6), L"%06d", iFile);
	XgStrReplace(str, L"%N", szN);
	XgStrReplace(str, L"%1N", szN1);
	XgStrReplace(str, L"%2N", szN2);
	XgStrReplace(str, L"%3N", szN3);
	XgStrReplace(str, L"%4N", szN4);
	XgStrReplace(str, L"%5N", szN5);
	XgStrReplace(str, L"%6N", szN6);

	// マスのサイズ（%W, %H）。
	StringCchPrintfW(szW, _countof(szW), L"%d", board.Columns());
	StringCchPrintfW(szH, _countof(szH), L"%d", board.Rows());
	XgStrReplace(str, L"%W", szW);
	XgStrReplace(str, L"%H", szH);

	// 日時（%Y/%M/%D %h:%m:%s）。
	StringCchPrintfW(szYear, _countof(szYear), L"%04d", pLocalTime->wYear);
	StringCchPrintfW(szMonth, _countof(szMonth), L"%02d", pLocalTime->wMonth);
	StringCchPrintfW(szDay, _countof(szDay), L"%02d", pLocalTime->wDay);
	StringCchPrintfW(szHour, _countof(szHour), L"%02d", pLocalTime->wHour);
	StringCchPrintfW(szMinute, _countof(szMinute), L"%02d", pLocalTime->wMinute);
	StringCchPrintfW(szSecond, _countof(szSecond), L"%02d", pLocalTime->wSecond);
	XgStrReplace(str, L"%Y", szYear);
	XgStrReplace(str, L"%M", szMonth);
	XgStrReplace(str, L"%D", szDay);
	XgStrReplace(str, L"%h", szHour);
	XgStrReplace(str, L"%m", szMinute);
	XgStrReplace(str, L"%s", szSecond);

	// 曜日（%w）。
	XgStrReplace(str, L"%w", aszWeekDay[pLocalTime->wDayOfWeek]);

	// コンピュータ名（%C）。
	DWORD cchComputer = _countof(szComputer);
	::GetComputerNameW(szComputer, &cchComputer);
	XgStrReplace(str, L"%C", szComputer);

	// ユーザ名（%U）。
	DWORD cchUser = _countof(szUser);
	::GetUserNameW(szUser, &cchUser);
	XgStrReplace(str, L"%U", szUser);

	// 二重パーセント（%%）。
	XgStrReplace(str, L"%%", L"%");

	return str;
}

// 連番保存。
BOOL MMainWnd::NumberingSave(HWND hwnd, BOOL bPattern, BOOL bNoError) {
	auto pszDir = m_dirs_save_to[0].data();
	if (!XgMakePathW(pszDir))
		return FALSE;

	WCHAR szFormat[MAX_PATH];
	StringCchCopyW(szFormat, _countof(szFormat), pszDir);
	if (bPattern)
		PathAppendW(szFormat, m_szNumberingFileName2);
	else
		PathAppendW(szFormat, m_szNumberingFileName1);

	// 現在の日時を取得。
	SYSTEMTIME stNow;
	::GetLocalTime(&stNow);

	MStringW strPath, oldName;
	BOOL bSameName = FALSE;
	for (INT iFile = 0; iFile <= 99999; ++iFile) {
		// 連番ファイル名を生成する。
		strPath = XgGenerateNumberingFilename(hwnd, m_board, szFormat, &stNow, iFile);
		if (oldName == strPath) { // 前回と同じファイル名だった場合。
			bSameName = TRUE; // フラグを立てて、後で処理する。
			break;
		}
		// ファイル名のファイルが存在すれば、終了。
		if (!PathFileExistsW(strPath.c_str()))
			break;
		// 古い名前を記憶する。
		oldName = strPath;
	}

	if (bSameName) { // 同じファイル名の場合。
		// おそらく連番がない。
		// ファイルタイトルにチルダ（~）を追加してファイル名の重複を避けることにする。

		// 拡張子のないファイル名を取得する。
		WCHAR szFileName[MAX_PATH];
		StringCchCopyW(szFileName, _countof(szFileName), strPath.c_str());
		MStringW strDotExt = PathFindExtensionW(szFileName);
		if (strDotExt.empty())
			strDotExt = L".xd";
		PathRemoveExtensionW(szFileName);

		// 拡張子のないファイル名にチルダ（~）と拡張子をいくつか追加して再試行する。
		for (INT iRetry = 1; iRetry < MAX_PATH; ++iRetry) {
			MStringW strName = szFileName;
			strName += MStringW(iRetry, L'~'); // チルダをiRetry個追加。
			strName += strDotExt.c_str(); // 拡張子を追加。
			if (!PathFileExistsW(strName.c_str())) {
				strPath = std::move(strName);
				break;
			}
		}
	}

	// 保存する。
	if (bNoError)
		return SaveFile1(hwnd, strPath.c_str(), TRUE);
	else
		return SaveFile(hwnd, strPath.c_str(), TRUE);
}

//////////////////////////////////////////////////////////////////////
// Canvas Window

// キャンバス ウィンドウを作成する。
BOOL MMainWnd::CreateCanvasWnd(HWND hwnd) {
	DWORD style = WS_OVERLAPPED | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | WS_CLIPSIBLINGS;
	DWORD exstyle = 0;
	// キャンバスの表示サイズが変わったら、タスクバーのサムネイルを更新する。
	m_hCanvasWnd.m_fnLayoutChanged = [this]() {
		if (m_pTaskbarProgress)
			m_pTaskbarProgress->SetThumbnail();
	};
	if (!m_hCanvasWnd.CreateWindowDx(hwnd, nullptr, style, exstyle))
		return FALSE;
	UpdateScrollInfo(0, 0);
	return TRUE;
}

// キャンバス ウィンドウを破棄する。
void MMainWnd::DestroyCanvasWnd() {
	if (m_hCanvasWnd) {
		DestroyWindow(m_hCanvasWnd);
	}
}

//////////////////////////////////////////////////////////////////////
// Status Bar

// ステータスバーを作成。
BOOL MMainWnd::CreateStatusBar(HWND hwnd) {
	m_hStatusBar = ::CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, L"", hwnd, IDW_STATUSBAR);
	if (!m_hStatusBar)
		return FALSE;

	UpdateStatusBar();
	ShowStatusBar(m_bShowStatusBar);
	return TRUE;
}

// ステータスバーを破棄。
void MMainWnd::DestroyStatusBar() {
	if (m_hStatusBar) {
		::DestroyWindow(m_hStatusBar);
		m_hStatusBar = nullptr;
	}
}

// ステータスバーを更新する。
void MMainWnd::UpdateStatusBar() {
	// クライアント領域を取得する。
	RECT rc;
	GetClientRect(m_hwnd, &rc);

	// パーツのサイズを決定する。
	const INT anWidth[] = { rc.left + 150, rc.right - 170, rc.right - 85, rc.right };

	// ステータスバーをパーツに分ける。
	SendMessageW(m_hStatusBar, SB_SETPARTS, _countof(anWidth), LPARAM(anWidth));

	// タテ入力かヨコ入力かどうか表示する。
	MStringW str = XgLoadString(m_bVertInput ? IDS_VINPUT3 : IDS_HINPUT3);
	str += L" - ";

	// 入力モードを表示。
	switch (m_imode) {
	case xg_im_ABC: str += XgLoadString(IDS_ABC); break;
	case xg_im_KANA: str += XgLoadString(IDS_KANA); break;
	case xg_im_KANJI: str += XgLoadString(IDS_KANJI); break;
	case xg_im_RUSSIA: str += XgLoadString(IDS_RUSSIA); break;
	case xg_im_GREEK: str += XgLoadString(IDS_GREEK); break;
	case xg_im_DIGITS: str += XgLoadString(IDS_DIGITS); break;
	case xg_im_ANY: str += XgLoadString(IDS_ANYINPUT); break;
	default:
		break;
	}

	// 文字送りを表示。
	if (m_bCharFeed) {
		str += L" - ";
		str += XgLoadString(IDS_CHARFEED);
	}

	// ルール値を表示。
	str += L" - ";
	{
		WCHAR szRule[64];
		StringCchPrintfW(szRule, _countof(szRule), L"0x%04X", xg_nRules);
		str += szRule;
	}

	// 辞書名を表示。
	str += L" - ";
	if (m_dict_name.size()) {
		MStringW str2;
		if (XgIsUserJapanese()) {
			str2 += L'『'; // U+300E
			str2 += m_dict.m_japanese_name;
			str2 += L'』'; // U+300F
		} else {
			str2 += L'"';
			str2 += m_dict.m_english_name;
			str2 += L'"';
		}
		if (str2.size() > 13) {
			str2.resize(13);
			str2 += L"...";
		}
		str += str2;
	} else {
		str += XgLoadString(IDS_NONE);
	}

	// テーマタグありorなし？
	str += L" - ";
	if (m_dict.m_theme_tags == m_dict.m_default_theme_tags) {
		str += XgLoadString(IDS_NOTHEMETAGS);
	} else {
		str += XgLoadString(IDS_HASTHEMETAGS);
	}

	// 状態を表示。
	SendMessageW(m_hStatusBar, SB_SETTEXT, 1, LPARAM(str.c_str()));

	// キャレット位置。
	WCHAR szText[64];
	StringCchPrintfW(szText, _countof(szText), L"(%d, %d)", m_caret_pos.m_j + 1, m_caret_pos.m_i + 1);
	::SendMessageW(m_hStatusBar, SB_SETTEXT, 2, LPARAM(szText));

	// 盤のサイズ。
	const INT nColumns = m_board.Columns(), nRows = m_board.Rows();
	StringCchPrintfW(szText, _countof(szText), L"%d x %d = %d", nColumns, nRows, nColumns * nRows);
	::SendMessageW(m_hStatusBar, SB_SETTEXT, 3, LPARAM(szText));
}

// ステータスバーの表示切替。
void MMainWnd::ShowStatusBar(BOOL bShow) {
	::ShowWindow(m_hStatusBar, (bShow ? SW_SHOWNOACTIVATE : SW_HIDE));
	m_bShowStatusBar = bShow;
	PostMessageW(m_hwnd, WM_SIZE, 0, 0);
}

// タイトルバーテキストを更新。
void MMainWnd::UpdateTitleBarText() {
	MStringW str = XgLoadString(IDS_APPINFO);

	// ファイル名があるか？
	if (m_strFileName.size()) {
		str += L" - ";
		str += PathFindFileNameW(m_strFileName.c_str());
	} else {
		str += L" - ";
		str += XgLoadString(IDS_UNTITLED);
	}

	if (m_bFileModified) {
		str += L" * ";
	}

	MStringW word;
	if (XgGetMarkWord(m_display, word) && m_bShowAnswer) {
		str += L" (";
		str += word;
		str += L")";
	}

	::SetWindowTextW(m_hwnd, str.c_str());
}

void MMainWnd::MyPlaySound(SoundType type) {
	assert(MY_SOUND_SUCCESS <= type && type <= MY_SOUND_CANCELED);
	if (m_aszSoundFiles[type][0]) {
		PlaySoundW(m_aszSoundFiles[type], nullptr, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
	}
}

//////////////////////////////////////////////////////////////////////

// WM_CREATE
BOOL MMainWnd::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) {
	xg_hMainWnd = hwnd;

	// ここで環境変数の情報を更新。
	RefreshEnvironment(TRUE);

	m_pTaskbarProgress = new TaskbarProgress(hwnd);

	if (!CreateCanvasWnd(hwnd))
		return FALSE;
	if (!CreateToolBar(hwnd))
		return FALSE;
	if (!CreateStatusBar(hwnd))
		return FALSE;

	if (m_bShowInputPalette)
		ReCreateInputPalette(hwnd, m_imode, FALSE);

	XgLoadDictList(m_dict_list);
	ChooseDict();

	m_hAccel = LoadAcceleratorsW(xg_hInstance, MAKEINTRESOURCEW(1));

	if (m_argc >= 2) {
		LoadFile(hwnd, m_wargv[1]);
	} else {
		m_strFileName = XgLoadString(IDS_UNTITLED);
	}

	PostMessageW(hwnd, WM_SIZE, 0, 0);
	PostMessageW(hwnd, WM_COMMAND, ID_SWCHECK, 0);
	UpdateTitleBarText();

	return TRUE;
}

// WM_DESTROY
void MMainWnd::OnDestroy(HWND hwnd) {
	DestroyStatusBar();
	DestroyCanvasWnd();
	DestroyToolBar();
	DestroyCluesWnd();
	DestroyMarksDlg();
	m_ai_helper.CloseGUI();

	ClearUndoBuffer();

	if (m_hAccel) {
		DestroyAcceleratorTable(m_hAccel);
		m_hAccel = nullptr;
	}
	delete m_pTaskbarProgress;
	m_pTaskbarProgress = nullptr;

	PostQuitMessage(0);
	xg_hMainWnd = nullptr;
}

// WM_MOVE
void MMainWnd::OnMove(HWND hwnd, INT x, INT y) {
	if (!xg_hMainWnd)
		return;
	if (!::IsIconic(hwnd) && !::IsZoomed(hwnd)) {
		RECT rc;
		::GetWindowRect(hwnd, &rc);
		m_nWindowX = rc.left;
		m_nWindowY = rc.top;
	}
}

// WM_SIZE
void MMainWnd::OnSize(HWND hwnd, UINT state, INT cx, INT cy) {
	if (!xg_hMainWnd)
		return;

	MRect rcClient;
	GetClientRect(hwnd, &rcClient);

	// ツールバーの高さを考慮する。
	INT cyToolBar = 0;
	if (m_bShowToolBar) {
		::SendMessageW(m_hToolBar, WM_SIZE, 0, 0);

		MRect rcToolBar;
		GetWindowRect(m_hToolBar, &rcToolBar);
		cyToolBar = rcToolBar.Height();
	}
	rcClient.top += cyToolBar;

	// ステータスバーの高さを考慮する。
	INT cyStatus = 0;
	if (m_bShowStatusBar) {
		::SendMessageW(m_hStatusBar, WM_SIZE, 0, 0);

		MRect rcStatus;
		::GetWindowRect(m_hStatusBar, &rcStatus);
		cyStatus = rcStatus.Height();
	}
	rcClient.bottom -= cyStatus;

	// キャンバスを動かす。
	::MoveWindow(m_hCanvasWnd, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), TRUE);
	// 位置とサイズを覚えておく。
	m_rcCanvas = rcClient;
	// サムネイルを更新。
	if (m_pTaskbarProgress && state != SIZE_MINIMIZED)
		m_pTaskbarProgress->SetThumbnail();

	m_bMaximized = ::IsZoomed(hwnd);
	if (!m_bMaximized && !::IsIconic(hwnd)) {
		MRect rc;
		::GetWindowRect(hwnd, &rc);
		m_nWindowCX = rc.Width();
		m_nWindowCY = rc.Height();
	}
}

// WM_CLOSE
void MMainWnd::OnClose(HWND hwnd) {
	if (DoConfirmSave(hwnd))
		DestroyWindow(hwnd);
}

#include "MShareware.h"
static MShareware s_sw(
	/* application registry path */ TEXT("SOFTWARE\\Katayama Hirofumi MZ\\XWordGiver"),
	/* password hash */			"2005ff6c1765795538e52bde5478bc341f7633eb3bc12eab5014c92bcdf6b452",
	/* trial days */			15,
	/* salt string */			"{23D4569C-0DE8-4619-B8E9-52DF9840729F}",
	/* version string */		XWORDGIVER_VERSION_A);

BOOL XgIsRegdSW(void) {
	return !!s_sw.IsRegistered();
}

// WM_COMMAND
void MMainWnd::OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) {
	// NOTE: この関数内では return文を使わないこと。
	if (m_nCommandLock++ == 0) {
		// コマンド開始時に、「コマンド実行中...」と表示する。
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0 | 0, (LPARAM)XgLoadString(IDS_RUNNINGCOMMAND).c_str());
	}

	switch (id) {
	case ID_NEW: // 新規作成。
		OnNew(hwnd);
		break;
	case ID_GENERATE: // 生成。
		OnGenerate(hwnd);
		break;
	case ID_OPEN: // 開く。
		OnOpen(hwnd);
		break;
	case ID_SAVEAS: // 名前を付けて保存。
		OnSaveAs(hwnd);
		break;
	case ID_SAVEPROBASIMAGE: // 画像として問題を保存。
		OnSaveProbAsImage(hwnd);
		break;
	case ID_EXIT: // 終了。
		PostMessageW(hwnd, WM_CLOSE, 0, 0);
		break;
	case ID_UNDO: // 元に戻す。
		Undo();
		break;
	case ID_REDO: // やり直す。
		Redo();
		break;
	case ID_SOLVE: // 解を求める。
		OnSolve(hwnd);
		break;
	case ID_OPENREADME: // READMEを開く。
		XgOpenLocalFile(hwnd, XgLoadString(IDS_README).c_str());
		break;
	case ID_ABOUT: // バージョン情報。
		OnAbout(hwnd);
		break;
	case ID_COPY: // コピー。
		OnCopy(hwnd);
		break;
	case ID_PASTE: // 貼り付け。
		OnPaste(hwnd);
		break;
	case ID_SAVEANSASIMAGE: // 画像として答えを保存。
		OnSaveAnsAsImage(hwnd);
		break;
	case ID_KANAINPUT: // カナ入力
		SetInputMode(hwnd, xg_im_KANA, TRUE);
		break;
	case ID_ABCINPUT: // 英字入力
		SetInputMode(hwnd, xg_im_ABC, TRUE);
		break;
	case ID_KANJIINPUT: // 漢字入力
		SetInputMode(hwnd, xg_im_KANJI, TRUE);
		break;
	case ID_RUSSIAINPUT: // ロシア入力
		SetInputMode(hwnd, xg_im_RUSSIA, TRUE);
		break;
	case ID_GREEKINPUT: // ギリシャ入力
		SetInputMode(hwnd, xg_im_GREEK, TRUE);
		break;
	case ID_DIGITINPUT: // 数字入力
		SetInputMode(hwnd, xg_im_DIGITS, TRUE);
		break;
	case ID_ANYINPUT: // 自由入力
		SetInputMode(hwnd, xg_im_ANY, TRUE);
		break;
	case ID_SHOWSOLUTION: // 答えを表示する。
		if (m_display.m_bSolved) {
			m_display.m_bShowAnswer = TRUE;
			InvalidateImage();
		}
		break;
	case ID_NOSOLUTION: // 答えを表示しない。
		if (m_display.m_bSolved) {
			m_display.m_bShowAnswer = FALSE;
			InvalidateImage();
		}
		break;
	case ID_SHOWHIDEHINTS: // ヒント（カギ）
		OnShowHideHints(hwnd);
		break;
	case ID_KILLMARKS: // 二重マスをすべて解除する。
		OnKillMarks(hwnd);
		break;
	case ID_MARKSNEXT: // 二重マス単語の候補と配置
	case ID_MARKSPREV:
		if (m_bSolved)
			CreateMarksDlg();
		break;
	case ID_SHOWHIDESOLUTION: // 答えの表示切替。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_bShowAnswer = !m_bShowAnswer;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_PRINTPROBLEM: // 問題のみを印刷
		OnPrint(hwnd, FALSE);
		break;
	case ID_PRINTANSWER: // 問題と解答を印刷
		OnPrint(hwnd, TRUE);
		break;
	case ID_SOLVENOADDBLACK: // 解を求める(黒マス追加なし)
		OnSolveNoAddBlack(hwnd);
		break;
	case ID_LINESYMMETRYCHECK: // 黒マス線対称チェック
		OnLineSymmetryCheck(hwnd);
		break;
	case ID_POINTSYMMETRYCHECK: // 黒マス点対称チェック
		OnPointSymmetryCheck(hwnd);
		break;
	case ID_OPENHOMEPAGE: // ホームページを開く。
		::ShellExecuteW(hwnd, nullptr, XgLoadString(IDS_HOMEPAGE).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		break;
	case ID_SETTINGS: // 設定
		OnSettings(hwnd);
		break;
	case ID_ERASESETTINGS: // アプリの初期化
		OnEraseSettings(hwnd);
		break;
	case ID_OPENBBS: // 掲示板を開く。
		OnOpenBSS(hwnd);
		break;
	case ID_OPENLICENSE: // License.txtを開く。
		XgOpenLocalFile(hwnd, XgLoadString(IDS_LICENSE).c_str());
		break;
	case ID_CUT: // 切り取り。
		OnCut(hwnd);
		break;
	case ID_PANENEXT: // 次のペイン
		OnPaneNext(hwnd, FALSE);
		break;
	case ID_PANEPREV: // 前のペイン
		OnPaneNext(hwnd, TRUE);
		break;
	case ID_COPYASIMAGE: // 盤面を画像としてコピー
		XgCopyBoard(hwnd, m_display, FALSE, TRUE, TRUE);
		break;
	case ID_OPENPATTERNS: // 黒マスパターンから生成。
		OnOpenPatterns(hwnd);
		break;
	case ID_COPYHINTSSTYLE0: // すべてのカギをコピー (スタイル0)
		OnCopyHintsStyle0(hwnd, 2);
		break;
	case ID_COPYHINTSSTYLE1: // すべてのカギをコピー (スタイル1)
		OnCopyHintsStyle1(hwnd, 2);
		break;
	case ID_HEADERANDNOTES: // ヘッダーと備考欄
		OnHeaderAndNotes(hwnd);
		break;
	case ID_COPYMARKWORD: // 二重マス単語をコピー。
		OnCopyMarkWord(hwnd);
		break;
	case ID_COPYVHINTSSTYLE0: // タテのカギをコピー (スタイル0)
		OnCopyHintsStyle0(hwnd, 0);
		break;
	case ID_COPYHHINTSSTYLE0: // ヨコのカギをコピー (スタイル0)
		OnCopyHintsStyle0(hwnd, 1);
		break;
	case ID_COPYVHINTSSTYLE1: // タテのカギをコピー (スタイル1)
		OnCopyHintsStyle1(hwnd, 0);
		break;
	case ID_COPYHHINTSSTYLE1: // ヨコのカギをコピー (スタイル1)
		OnCopyHintsStyle1(hwnd, 1);
		break;
	case ID_GENERATEBLACKS: // 黒マスパターンを生成する
		OnGenerateBlacks(hwnd);
		break;
	case ID_FLIPVH: // タテとヨコを入れ替える
		OnFlipVH(hwnd);
		break;
	case ID_OPENCANDSWNDHORZ: // ヨコの入力候補
		OnOpenCandsWnd(hwnd, FALSE);
		break;
	case ID_OPENCANDSWNDVERT: // タテの入力候補
		OnOpenCandsWnd(hwnd, TRUE);
		break;
	case ID_MOSTLEFT: // Ctrl+←
		OnMostLeft(hwnd);
		break;
	case ID_MOSTRIGHT: // Ctrl+→。
		OnMostRight(hwnd);
		break;
	case ID_MOSTUPPER: // Ctrl+↑。
		OnMostUpper(hwnd);
		break;
	case ID_MOSTLOWER: // Ctrl+↓。
		OnMostLower(hwnd);
		break;
	case ID_LEFT: // キャレットを左へ移動。
		OnLeft(hwnd);
		break;
	case ID_RIGHT: // キャレットを右へ移動。
		OnRight(hwnd);
		break;
	case ID_UP: // キャレットを上へ移動。
		OnUp(hwnd);
		break;
	case ID_DOWN: // キャレットを下へ移動。
		OnDown(hwnd);
		break;
	case ID_CHARFEED: // 文字送りの切り替え。
		SetCharFeed(hwnd, -1);
		break;
	case ID_INPUTH: // ヨコ入力
		InputDirection(hwnd, 0);
		break;
	case ID_INPUTV: // タテ入力
		InputDirection(hwnd, 1);
		break;
	case ID_STATUS: // ステータスバーの表示切替。
		ShowStatusBar(!m_bShowStatusBar);
		break;
	case ID_PALETTE: // 入力パレット。
	case ID_PALETTE2: // 入力パレット。
		if (m_hInputPalette)
			DestroyInputPalette();
		else
			ReCreateInputPalette(hwnd, m_imode, FALSE);
		m_bShowInputPalette = !!m_hInputPalette;
		break;
	case ID_BACK: // BackSpace
		OnBack(hwnd);
		break;
	case ID_INPUTHV: // 入力方向の切り替え。
		InputDirection(hwnd, -1);
		break;
	case ID_RETURN: // Enterキー
		OnReturn(hwnd);
		break;
	case ID_BLOCK: // 黒マス。
		OnBlock(hwnd);
		break;
	case ID_SPACE: // 空白マス。
		OnSpace(hwnd);
		break;
	case ID_CLEARNONBLOCKS: // 文字をすべてクリア
		OnClearNonBlocks(hwnd);
		break;
	case ID_ONLINEDICT: // オンライン辞書。
		OnOnlineDict(hwnd, m_bVertInput);
		break;
	case ID_ONLINEDICTV: // オンライン辞書（タテ）。
		OnOnlineDict(hwnd, TRUE);
		break;
	case ID_ONLINEDICTH: // オンライン辞書（ヨコ）。
		OnOnlineDict(hwnd, FALSE);
		break;
	case ID_TOGGLEMARK: // 二重マス切り替え
		OnTottleMark(hwnd);
		break;
	case ID_TOOLBAR: // ツールバーの表示切替。
		ShowToolBar(!m_bShowToolBar);
		break;
	case ID_HELPDICTSITE:
		ShellExecuteW(hwnd, nullptr, XgLoadString(IDS_MATRIXSEARCH).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
		break;
	case ID_BLOCKNOFEED: // 黒マスを置く。
		SetChar(hwnd, ZEN_BLACK);
		break;
	case ID_SPACENOFEED: // 空白マスを置く。
		SetChar(hwnd, ZEN_SPACE);
		break;
	case ID_ZOOMIN: // ズームイン
		m_hCanvasWnd.ZoomInOut(FALSE);
		break;
	case ID_ZOOMOUT: // ズームアウト
		m_hCanvasWnd.ZoomInOut(TRUE);
		break;
	case ID_ZOOM100: // ズーム100%
		m_hCanvasWnd.SetZoom(100);
		break;
	case ID_COPYWORDHORZ: // ヨコの単語をコピー
		OnCopyWord(hwnd, FALSE);
		break;
	case ID_COPYWORDVERT: // タテの単語をコピー
		OnCopyWord(hwnd, TRUE);
		break;
	case ID_LOWERCASE: // 英字小文字。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_display.m_bLowercase = !m_display.m_bLowercase;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_HIRAGANA: // ひらがな。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_bHiragana = !m_bHiragana;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_RESETRULES: // ルールのリセット。
		SetRules(XG_STANDARD_RULES);
		break;
	case ID_OPENRULESTXT: // 黒マスルールの説明を開く。
		XgOpenLocalFile(hwnd, XgLoadString(IDS_RULESTXT).c_str());
		break;
	case ID_RULE_DONTDOUBLEBLACK: // 「連黒禁」切り替え。
		ToggleRules(RULE_DONTDOUBLEBLACK, RULE_DONTDOUBLEBLACK);
		break;
	case ID_RULE_DONTCORNERBLACK: // 「四隅黒禁」切り替え。
		ToggleRules(RULE_DONTCORNERBLACK, RULE_DONTCORNERBLACK);
		break;
	case ID_RULE_DONTTRIDIRECTIONS: // 「三方黒禁」切り替え。
		ToggleRules(RULE_DONTTRIDIRECTIONS, RULE_DONTTRIDIRECTIONS);
		break;
	case ID_RULE_DONTDIVIDE: // 「分断禁」切り替え。
		// 変更禁止。
		break;
	case ID_RULE_DONTFOURDIAGONALS: // 「黒斜四連禁」切り替え。
		ToggleRules(RULE_DONTFOURDIAGONALS, RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
		break;
	case ID_RULE_POINTSYMMETRY: // 「黒マス点対称」切り替え。
		ToggleRules(RULE_POINTSYMMETRY, RULE_POINTSYMMETRY | RULE_LINESYMMETRYH | RULE_LINESYMMETRYV);
		break;
	case ID_RULECHECK: // ルール適合性チェック
		RuleCheck(hwnd, TRUE);
		break;
	case ID_THEMETAGS: // テーマ タグ。
		OnThemeTags(hwnd);
		break;
	case ID_ZOOM30: // ズーム30%。
		m_hCanvasWnd.SetZoom(30);
		break;
	case ID_ZOOM50: // ズーム50%。
		m_hCanvasWnd.SetZoom(50);
		break;
	case ID_ZOOM65: // ズーム65%。
		m_hCanvasWnd.SetZoom(65);
		break;
	case ID_ZOOM80: // ズーム80%。
		m_hCanvasWnd.SetZoom(80);
		break;
	case ID_ZOOM90: // ズーム90%。
		m_hCanvasWnd.SetZoom(90);
		break;
	case ID_RULE_DONTTHREEDIAGONALS: // 「黒斜三連禁」切り替え。
		ToggleRules(RULE_DONTTHREEDIAGONALS, RULE_DONTTHREEDIAGONALS | RULE_DONTFOURDIAGONALS);
		break;
	case ID_SHOWHIDECARET: // キャレットの表示/非表示を切り替える。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_bShowCaret = !m_bShowCaret;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_ERASESOLUTIONANDUNLOCKEDIT: // ヒントを削除して盤面のロックを解除
		OnEraseSolutionAndUnlockEdit(hwnd);
		break;
	case ID_RELOADDICTS: // 辞書リストを再読み込み
		XgLoadDictList(m_dict_list);
		break;
	case ID_RULE_LINESYMMETRYV: // 「黒マス上下対称」切り替え。
		ToggleRules(RULE_LINESYMMETRYV, RULE_POINTSYMMETRY | RULE_LINESYMMETRYH | RULE_LINESYMMETRYV);
		break;
	case ID_RULE_LINESYMMETRYH: // 「黒マス左右対称」切り替え。
		ToggleRules(RULE_LINESYMMETRYH, RULE_POINTSYMMETRY | RULE_LINESYMMETRYH | RULE_LINESYMMETRYV);
		break;
	case ID_VIEW_SKELETON_VIEW: // スケルトン ビュー切り替え。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_nViewMode = (m_nViewMode == XG_VIEW_SKELETON) ? XG_VIEW_NORMAL : XG_VIEW_SKELETON;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_COPY_WORD_LIST: // 単語リストのコピー。
		OnCopyWordList(hwnd);
		break;
	case ID_VIEW_DOUBLEFRAME_LETTERS: // 二重マス文字を表示するか？
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_bShowDoubleFrameLetters = !m_bShowDoubleFrameLetters;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_INSERT_ROW_ABOVE: // 上に行を追加。
		OnInsertRow(hwnd, FALSE);
		break;
	case ID_INSERT_ROW_BELOW: // 下に行を追加。
		OnInsertRow(hwnd, TRUE);
		break;
	case ID_LEFT_INSERT_COLUMN: // 左に列を追加。
		OnInsertColumn(hwnd, FALSE);
		break;
	case ID_RIGHT_INSERT_COLUMN: // 右に列を追加。
		OnInsertColumn(hwnd, TRUE);
		break;
	case ID_DELETE_ROW: // 行を削除。
		OnDeleteRow(hwnd);
		break;
	case ID_DELETE_COLUMN: // 列を削除。
		OnDeleteColumn(hwnd);
		break;
	case ID_GENERATEFROMWORDLIST: // 単語群から自動生成
		OnGenerateFromWordList(hwnd);
		break;
	case ID_FITZOOM: // 全体に合わせる
		m_hCanvasWnd.FitZoom();
		break;
	case ID_COPYCHARSET: // 文字群をコピー。
		OnCopyCharSet(hwnd);
		break;
	case ID_RULEPRESET: // ルールプリセット。
		OnRulePreset(hwnd);
		break;
	case ID_SAVE: // 上書き保存
		OnSave(hwnd);
		break;
	case ID_UILANGID: // UI Language Select
		OnUILangID(hwnd);
		break;
	case ID_REFLECTBLOCKS: // 黒マスパターンを反射する
		OnReflectBlocks(hwnd);
		break;
	case ID_NUMCROMODE: // ナンクロモードの切り替え。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_display.m_bNumCroMode = !m_display.m_bNumCroMode;
		if (m_display.m_bNumCroMode) {
			XgMakeItNumCro(m_display);
		}
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_OPENPATTXT: // PAT.txtを開く。
		XgOpenLocalFile(hwnd, L"PAT.txt");
		break;
	case ID_OPENTHEMETXT: // THEME.txtを開く。
		XgOpenLocalFile(hwnd, L"THEME.txt");
		break;
	case ID_CLEARUNDOBUFFER: // 「元に戻す」情報を消す
		ClearUndoBuffer();
		break;
	case ID_DOWNLOADDICT: // 辞書のダウンロード
		ShellExecuteW(hwnd, nullptr, L"https://katahiromz.web.fc2.com/xword/dict", nullptr, nullptr, SW_SHOWNORMAL);
		break;
	case ID_OPENPOLICYTXT: // POLICY.txtを開く。
		XgOpenLocalFile(hwnd, L"POLICY.txt");
		break;
	case ID_SWAP_LEFT_RIGHT: // 左右を入れ替える
		OnSwapLeftRight(hwnd);
		break;
	case ID_SWAP_TOP_BOTTOM: // 上下を入れ替える
		OnSwapTopBottom(hwnd);
		break;
	case ID_CHECKANSWER: // 答え合わせ
		OnCheckAnswer(hwnd);
		break;
	case ID_JUMPNUMBER: // ジャンプ
		OnJumpNumber(hwnd);
		break;
	case ID_NUMBERINGSAVE: // 連番で保存
		NumberingSave(hwnd, FALSE, FALSE);
		break;
	case ID_VIEW_DOUBLEFRAME: // 二重マスを描画するか？
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_bShowDoubleFrame = !m_bShowDoubleFrame;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		break;
	case ID_JUMP: // ジャンプ
		OnJump(hwnd);
		break;
	case ID_OPENAIHELPER: // AI ヘルパーを開く
		OnOpenAIHelper(hwnd);
		break;
	case ID_GENERATEHINT: // AI: このカギを再生成
		OnGenerateClue();
		break;
	case ID_CLEARALLCLUES: // カギをすべてクリア
		OnClearAllClues(hwnd);
		break;
	case ID_GENERATEHINTSALL: // AI: すべてのカギを再生成
		m_ai_helper.RegenerateCluesAll();
		break;
	case ID_CLEARTHISCLUE: // このカギをクリア
		OnClearThisClue(hwnd);
		break;
	case ID_OPENAIHELPERTXT: // AIHelper.txtを開く。
		XgOpenLocalFile(hwnd, L"AIHelper.txt");
		break;
	case ID_EXPLAINTHISCLUE: // AI: このカギを解説
		OnExplainThisClue();
		break;
	case ID_CHECKQUALITYOFALLCLUES: // AI: すべてのカギの品質チェック
		m_ai_helper.CheckQualityOfAllClues();
		break;
	case ID_JUDGESEASON: // AI: このクロスワードの季節を分析
		m_ai_helper.JudgeSeason();
		break;
	case ID_JUDGETHEME: // AI: このクロスワードのテーマを分析
		m_ai_helper.JudgeTheme();
		break;
	case ID_AI_CUSTOM_PROMPT_1: // AI: カスタム プロンプト 1
		OnCustomPrompt(1);
		break;
	case ID_AI_CUSTOM_PROMPT_2: // AI: カスタム プロンプト 2
		OnCustomPrompt(2);
		break;
	case ID_DEBUGACTION1: // デバッグ アクション 1
		OnDebugAction1(hwnd);
		break;
	case ID_DEBUGACTION2: // デバッグ アクション 2
		OnDebugAction2(hwnd);
		break;
	case ID_SWCHECK: // シェアウェアチェック
		if (!s_sw.Start(hwnd)) {
			DestroyWindow(hwnd);
		}
		break;
	default: // その他
		if (ID_DICTIONARY00 <= id && id <= ID_DICTIONARY63) { // 辞書選択？
			size_t index = id - ID_DICTIONARY00;
			if (index < m_dict_list.size()) {
				if (m_dict_name != m_dict_list[index].m_pathname) {
					m_dict_name = m_dict_list[index].m_pathname;
					m_dict.load_file(m_dict_name.c_str());
				} else {
					m_dict.refresh();
				}
			}
		} else if (ID_RECENT_00 <= id && id <= ID_RECENT_09) { // 最近使ったファイル？
			OnRecent(hwnd, id - ID_RECENT_00);
		} else if (id >= 10000) {
			OnCommandExtra(hwnd, id);
		}
		break;
	}

	// 状態チェック。
	assert(m_board.Columns() == m_solution.Columns());
	assert(m_board.Rows() == m_solution.Rows());
	assert(m_board.Rules() == m_solution.Rules());
	assert(xg_nRules == m_board.Rules());
	assert(xg_nRules & RULE_DONTDIVIDE); // 分断禁を強制。

	// コマンドロックがゼロになったら、準備完了。
	if (--m_nCommandLock == 0) {
		UpdateToolBarUI();
		UpdateStatusBar();
		SendMessageW(m_hStatusBar, SB_SETTEXTW, 0 | 0, (LPARAM)XgLoadString(IDS_READY).c_str());
	}
}

// WM_NOTIFY
LRESULT MMainWnd::OnNotify(HWND hwnd, INT idFrom, LPNMHDR pnmhdr) {
	if (pnmhdr->code == NM_DBLCLK && idFrom == IDW_STATUSBAR) {
		OnJump(hwnd);
		return 0;
	}

	if (pnmhdr->code == NM_RCLICK && (idFrom == IDW_TOOLBAR || idFrom == IDW_STATUSBAR)) {
		// ツールバーかステータスバーが右クリックされた。右クリックメニューを表示する。

		// メニューを読み込む。
		HMENU hMenu = ::LoadMenuW(xg_hInstance, MAKEINTRESOURCEW(2));
		HMENU hSubMenu = ::GetSubMenu(hMenu, 2);

		// カーソル位置を取得。
		POINT pt;
		::GetCursorPos(&pt);

		// TrackPopupMenuの準備。
		::SetForegroundWindow(hwnd);

		// メニューを表示する。
		::TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0,
						 hwnd, nullptr);

		// TrackPopupMenuの後片づけ。
		::DestroyMenu(hMenu);
		::PostMessageW(hwnd, WM_NULL, 0, 0);
		return 0;
	}

	if (pnmhdr->code == TTN_NEEDTEXT) { // ツールバーのツールチップを表示する準備。
		auto pttt = reinterpret_cast<LPTOOLTIPTEXT>(pnmhdr);
		// idFrom --> ids
		INT ids;
		switch (pttt->hdr.idFrom) {
		case ID_NEW: ids = IDS_TT_NEW; break;
		case ID_GENERATE: ids = IDS_TT_GENERATE; break;
		case ID_OPEN: ids = IDS_TT_OPEN; break;
		case ID_SAVEAS: ids = IDS_TT_SAVEAS; break;
		case ID_UNDO: ids = IDS_TT_UNDO; break;
		case ID_REDO: ids = IDS_TT_REDO; break;
		case ID_SOLVE: ids = IDS_TT_SOLVE; break;
		case ID_COPY: ids = IDS_TT_COPY; break;
		case ID_PASTE: ids = IDS_TT_PASTE; break;
		case ID_PRINTPROBLEM: ids = IDS_TT_PRINTPROBLEM; break;
		case ID_PRINTANSWER: ids = IDS_TT_PRINTANSWER; break;
		case ID_SOLVENOADDBLACK: ids = IDS_TT_SOLVENOADDBLACK; break;
		default: return 0;
		}
		// ツールチップの情報をセットする。
		pttt->hinst = xg_hInstance;
		pttt->lpszText = MAKEINTRESOURCE(ids);
		return 0;
	}

	return 0;
}

// 行または列の文字列を取得する。
MStringW MMainWnd::GetRowOrColumnText(BOOL bRow, INT iRowOrCol) {
	auto& board = m_bSolved ? m_solution : m_board;

	MStringW str;
	if (bRow) {
		for (INT iCol = 0; iCol < board.Columns(); ++iCol) {
			str += board.GetCell(iCol, iRowOrCol);
		}
	} else {
		for (INT iRow = 0; iRow < board.Rows(); ++iRow) {
			str += board.GetCell(iRowOrCol, iRow);
		}
	}

	return str;
}

// カギ番号を指定してヒント文章をセットする（GUIと内部データの両方に反映する）。
BOOL __fastcall MMainWnd::SetHintText(INT number, BOOL bDown, const MStringW& text) {
	// まだ解かれていない場合は失敗。
	if (!m_bSolved)
		return FALSE;

	// タテかヨコかで対象の配列を選ぶ。
	auto& info_vec = bDown ? m_vVertInfo : m_vHorzInfo;

	// 対象番号のカギを探す。
	for (size_t i = 0; i < info_vec.size(); ++i) {
		if (info_vec[i].m_number != number)
			continue;

		// 内部データを更新する。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_CLUES, 0);
		m_word2clue[info_vec[i].m_word] = text;

		// GUI(MCluesWnd)を更新する。
		if (m_hCluesWnd) {
			m_hCluesWnd.SetHintsData();
		}
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_CLUES, 0);

		return TRUE;
	}

	return FALSE; // 対象番号のカギが見つからなかった。
}

MStringW __fastcall MMainWnd::GetHintWord(INT number, BOOL bDown) {
	// タテかヨコかで対象の配列を選ぶ。
	auto& info_vec = bDown ? m_vVertInfo : m_vHorzInfo;

	// 対象番号の単語を探す。
	for (size_t i = 0; i < info_vec.size(); ++i) {
		if (info_vec[i].m_number != number)
			continue;
		return info_vec[i].m_word;
	}

	return MStringW();
}

// 行または列を書き換える。
BOOL MMainWnd::SetBoardRowOrColumn(BOOL bRow, INT nNumber, const MStringW& text) {
	if (m_bSolved)
		return FALSE;
	if (nNumber <= 0)
		return FALSE;
	nNumber--;

	MStringW str = XgNormalizeString(text);

	if (bRow) {
		if (nNumber >= m_board.Rows())
			return FALSE;
		if (INT(str.size()) != m_board.Columns())
			return FALSE;

		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
		for (size_t ich = 0; ich < str.size(); ++ich) {
			m_board.SetCell(INT(ich), nNumber, text[ich]);
		}
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
	} else {
		if (nNumber >= m_board.Columns())
			return FALSE;
		if (INT(str.size()) != m_board.Rows())
			return FALSE;

		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
		for (size_t ich = 0; ich < str.size(); ++ich) {
			m_board.SetCell(nNumber, INT(ich), text[ich]);
		}
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
	}

	return TRUE;
}

// 生成AIで生成する。
BOOL MMainWnd::GenerateByAI(INT nNumber, const MStringW& text) {
	MStringListW strs;
	XgStrSplit(strs, text, L",");

	switch (nNumber) {
	case 0: // 新規作成。
		if (strs.size() == 2) {
			if (!DoConfirmSave(m_hwnd))
				return FALSE;
			New(_wtoi(strs[0].c_str()), _wtoi(strs[1].c_str()), xg_nRules, rad1, TRUE);
			return TRUE;
		}
		break;
	case 1: // 問題の生成。
		if (strs.size() == 2) {
			Generate(m_hwnd, _wtoi(strs[0].c_str()), _wtoi(strs[1].c_str()), xg_nRules, TRUE, m_bShowAnswerOnGenerate, 6, TRUE);
			return TRUE;
		}
		break;
	case 2: // 解を求める（黒マス追加あり）。
		if (strs.size() == 1 && strs[0] == L"-") {
			Solve(m_hwnd, TRUE);
			return TRUE;
		}
		break;
	case 3: // 解を求める（黒マス追加なし）。
		if (strs.size() == 1 && strs[0] == L"-") {
			SolveNoAddBlack(m_hwnd, TRUE);
			return TRUE;
		}
		break;
	case 4: // 単語群から生成する。
		if (strs.size() >= 2) {
			GenerateFromWordList(strs, FALSE, TRUE);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

// プロパティシートのインデックス。
enum IPAGE : INT {
	IPAGE_FILE = 0,
	IPAGE_LOOKS = 1,
	IPAGE_RULES = 2,
	IPAGE_GENERATIVE = 3,
	IPAGE_DICTS = 4,
	IPAGE_AIHELPER = 5,
#ifndef NDEBUG
	IPAGE_ADVANCED = 6,
#endif
};

// 設定プロパティシートを開く。
void MMainWnd::OpenConfigPropSheet(HWND hwnd, INT nIndex) {
	// 「ファイル」タブ。
	MFileSettingsDialog page0;
	page0.m_bShowAnswerOnOpen = m_bShowAnswerOnOpen;
	page0.m_bAutoSave = m_bAutoSave;
	page0.m_bShowAnswerOnGenerate = m_bShowAnswerOnGenerate;
	page0.m_bNoReadLooks = m_bNoReadLooks;
	page0.m_bNoWriteLooks = m_bNoWriteLooks;
	StringCchCopyW(page0.m_szNumberingFileName1, _countof(page0.m_szNumberingFileName1), m_szNumberingFileName1);
	StringCchCopyW(page0.m_szNumberingFileName2, _countof(page0.m_szNumberingFileName2), m_szNumberingFileName2);
	page0.m_dirs_save_to = m_dirs_save_to;
	page0.m_fnApply = [&]() {
		m_bShowAnswerOnOpen = page0.m_bShowAnswerOnOpen;
		m_bAutoSave = page0.m_bAutoSave;
		m_bShowAnswerOnGenerate = page0.m_bShowAnswerOnGenerate;
		m_bNoReadLooks = page0.m_bNoReadLooks;
		m_bNoWriteLooks = page0.m_bNoWriteLooks;
		StringCchCopyW(m_szNumberingFileName1, _countof(m_szNumberingFileName1), page0.m_szNumberingFileName1);
		StringCchCopyW(m_szNumberingFileName2, _countof(m_szNumberingFileName2), page0.m_szNumberingFileName2);
		m_dirs_save_to = page0.m_dirs_save_to;
	};
	// 「見た目」タブ。
	MSettingsDialog page1(m_display, m_strFileName);
	page1.m_fnApply = [&]() {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		// ここではルールは変更しない。ルールは「ルール」タブで変更する。
		INT nRules = xg_nRules;
		m_display.CopyFrom(page1.m_display);
		SetRules(nRules);
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		InvalidateImage();
	};
	// 「ルール」タブ。
	MRulePresetDialog page2(xg_nRules);
	page2.m_fnApply = [this](INT nRules) {
		SetRules(nRules);
	};
	// 「生成」タブ。
	MGenerativeDialog page3;
	page3.m_bNoGeneratedMsg = m_bNoGeneratedMsg;
	page3.m_bNoCanceledMsg = m_bNoCanceledMsg;
	StringCchCopyW(page3.m_aszSoundFiles[0], _countof(page3.m_aszSoundFiles[0]), m_aszSoundFiles[0]);
	StringCchCopyW(page3.m_aszSoundFiles[1], _countof(page3.m_aszSoundFiles[1]), m_aszSoundFiles[1]);
	StringCchCopyW(page3.m_aszSoundFiles[2], _countof(page3.m_aszSoundFiles[2]), m_aszSoundFiles[2]);
	page3.m_fnApply = [&]() {
		m_bNoGeneratedMsg = page3.m_bNoGeneratedMsg;
		m_bNoCanceledMsg = page3.m_bNoCanceledMsg;
		StringCchCopyW(m_aszSoundFiles[0], _countof(m_aszSoundFiles[0]), page3.m_aszSoundFiles[0]);
		StringCchCopyW(m_aszSoundFiles[1], _countof(m_aszSoundFiles[1]), page3.m_aszSoundFiles[1]);
		StringCchCopyW(m_aszSoundFiles[2], _countof(m_aszSoundFiles[2]), page3.m_aszSoundFiles[2]);
	};
	// 「辞書」タブ。
	MDictsDialog page4(m_dict_name);
	page4.m_fnApply = [&]() {
		if (m_dict_name != page4.m_dict_name) {
			m_dict_name = page4.m_dict_name;
			m_dict.load_file(m_dict_name.c_str());
			UpdateStatusBar();
		}
	};
	// 「AI ヘルパー」タブ。
	MAIHelperDialog page5(m_ai_helper);
	page5.m_fnApply = [&]() {
		m_ai_helper.m_provider = page5.m_ai_provider;
		m_ai_helper.m_model = page5.m_ai_model;
		m_ai_helper.m_promptMapping = page5.m_promptMapping;
	};
#ifndef NDEBUG
	// 「上級者向け」タブ。
	MHiddenDialog page6(m_display);
	page6.m_fnApply = [&]() {
		;
	};
#endif

	// ページを合わせてプロパティシートを表示する。
	MPropSheet sheet(hwnd, XgLoadString(IDS_GENERALSETTINGS).c_str());
	sheet.AddPage(page0);
	sheet.AddPage(page1);
	sheet.AddPage(page2);
	sheet.AddPage(page3);
	sheet.AddPage(page4);
	sheet.AddPage(page5);
#ifndef NDEBUG
	sheet.AddPage(page6);
#endif
	sheet.SetCurSelDx(nIndex);
	sheet.DoSheetDx();
}

// 印刷する。
void MMainWnd::DoPrint(HDC hdc, PRINTDLGW* ppd, BOOL bPrintAnswer) {
	LOGFONTW lf;
	HFONT hFont;
	HGDIOBJ hFontOld;
	RECT rc;

	const INT nVert = ::GetDeviceCaps(hdc, VERTSIZE);
	const INT nHorz = ::GetDeviceCaps(hdc, HORZSIZE);
	if (nVert < nHorz) {
		// 印刷用紙が横長。メッセージを表示して終了。
		ErrorBoxDx(IDS_SETPORTRAIT);
		::DeleteDC(hdc);
		::GlobalFree(ppd->hDevMode);
		::GlobalFree(ppd->hDevNames);
		return;
	}

	DOCINFOW di;
	ZeroMemory(&di, sizeof(di));
	di.cbSize = sizeof(di);
	auto doc_name = XgLoadString(IDS_CROSSWORD);
	di.lpszDocName = &doc_name[0];

	// 指定された部数を印刷する。
	for (INT i = 0; i < ppd->nCopies; i++) {
		// 文書を開始する。
		if (::StartDocW(hdc, &di) <= 0)
			continue;

		size_t ich, ichOld;
		MStringW str, strMarkWord, strHints;
		INT yLast = 0;

		// 論理ピクセルのアスペクト比を取得する。
		const INT nLogPixelX = ::GetDeviceCaps(hdc, LOGPIXELSX);
		const INT nLogPixelY = ::GetDeviceCaps(hdc, LOGPIXELSY);

		// 用紙のピクセルサイズを取得する。
		const INT cxPaper = ::GetDeviceCaps(hdc, HORZRES);
		const INT cyPaper = ::GetDeviceCaps(hdc, VERTRES);

		// ページ開始。
		if (::StartPage(hdc) > 0) {
			// 二重マス単語を描画する。
			if (bPrintAnswer && XgGetMarkWord(m_display, strMarkWord)) {
				// 既定のフォント情報を取得する。
				hFont = HFONT(::GetStockObject(DEFAULT_GUI_FONT));
				::GetObjectW(hFont, sizeof(LOGFONTW), &lf);

				// フォント情報を設定する。
				lf = m_display.m_lfCellLogFont;
				lf.lfHeight = cyPaper / 2 / 15;
				lf.lfWidth = 0;
				lf.lfWeight = FW_NORMAL;
				lf.lfQuality = ANTIALIASED_QUALITY;

				// 二重マス単語を描画する。
				hFont = ::CreateFontIndirectW(&lf);
				hFontOld = ::SelectObject(hdc, hFont);
				rc = { cxPaper / 8, cyPaper / 16, cxPaper * 7 / 8, cyPaper / 8 };
				str = XgLoadString(IDS_ANSWER);
				str += strMarkWord;
				::DrawTextW(hdc, str.data(), INT(str.size()), &rc, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX);
				::SelectObject(hdc, hFontOld);
				::DeleteFont(hFont);
			}

			// サイズを取得する。
			SIZE siz = XgGetBoardExtent(m_display);

			// EMFオブジェクトを作成する。
			HDC hdcEMF = ::CreateEnhMetaFileW(hdc, nullptr, nullptr, XgLoadString(IDS_APPNAME).c_str());
			if (hdcEMF != nullptr) {
				if (bPrintAnswer)
					XgDrawBoard(m_display, m_solution, hdcEMF, &siz, DRAW_MODE_EMF);
				else
					XgDrawBoard(m_display, m_board, hdcEMF, &siz, DRAW_MODE_EMF);

				// EMFオブジェクトを閉じる。
				HENHMETAFILE hEMF = ::CloseEnhMetaFile(hdcEMF);
				if (hEMF) {
					// EMFを描画する領域を計算する。
					INT x, y, cx, cy;
					if (m_board.Columns() >= 10) {
						x = cxPaper / 6;
						y = cyPaper / 8;
						cx = cxPaper * 2 / 3;
						cy = cxPaper * nLogPixelY * siz.cy / nLogPixelX / siz.cx * 2 / 3;
					} else {
						x = cxPaper / 4;
						y = cyPaper / 8;
						cx = cxPaper / 2;
						cy = cxPaper * nLogPixelY * siz.cy / nLogPixelX / siz.cx / 2;
					}
					rc = { x, y, x + cx, y + cy };

					// EMFを描画する。
					::PlayEnhMetaFile(hdc, hEMF, &rc);
					::DeleteEnhMetaFile(hEMF);
					yLast = y + cy;
				}
			}

			// ヒントを取得する。
			if (m_bSolved) {
				MStringW str;
				str += XgTextFromDownCluesStyle0(m_vVertInfo, m_word2clue, bPrintAnswer);
				str += XgTextFromAcrossCluesStyle0(m_vHorzInfo, m_word2clue, bPrintAnswer);
				strHints = std::move(str);
			} else {
				strHints.clear();
			}

			// 文字色をセットする。
			::SetTextColor(hdc, RGB(0, 0, 0));

			// 既定のフォント情報を取得する。
			hFont = HFONT(::GetStockObject(DEFAULT_GUI_FONT));
			::GetObjectW(hFont, sizeof(LOGFONTW), &lf);

			// フォント情報を設定する。
			lf = m_display.m_lfCellLogFont;
			lf.lfHeight = cyPaper / 2 / 45;
			lf.lfWidth = 0;
			lf.lfWeight = FW_NORMAL;
			lf.lfQuality = ANTIALIASED_QUALITY;
			hFont = ::CreateFontIndirectW(&lf);
			hFontOld = ::SelectObject(hdc, hFont);

			// ヒントを一行ずつ描画する。
			rc = { cxPaper / 8, cyPaper / 2, cxPaper * 7 / 8, cyPaper * 8 / 9 };
			for (ichOld = ich = 0; rc.bottom <= cyPaper * 8 / 9; ichOld = ich + 1) {
				ich = strHints.find(L"\n", ichOld);
				if (ich == strHints.npos)
					break;
				str = strHints.substr(0, ich);  // 一行取り出す。
				rc = { cxPaper / 8, yLast, cxPaper * 7 / 8, cyPaper * 8 / 9 };
				::DrawTextW(hdc, str.data(), INT(str.size()), &rc,
					DT_LEFT | DT_TOP | DT_CALCRECT | DT_NOPREFIX | DT_WORDBREAK);
			}
			// 最後の行を描画する。
			rc = { cxPaper / 8, yLast, cxPaper * 7 / 8, cyPaper * 8 / 9 };
			if (ich == MStringW::npos) {
				str = strHints;
				::DrawTextW(hdc, str.data(), INT(str.size()), &rc, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK);
				strHints.clear();
			} else {
				str = strHints.substr(0, ichOld);
				::DrawTextW(hdc, str.data(), INT(str.size()), &rc, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK);
				strHints = strHints.substr(ichOld);
			}

			// 文字のフォントの選択を解除して削除する。
			::SelectObject(hdc, hFontOld);
			::DeleteFont(hFont);

			// ページ終了。
			::EndPage(hdc);
		}

		// ヒントの残りを描画する。
		if (!strHints.empty()) {
			// ページ開始。
			if (::StartPage(hdc) > 0) {
				// 文字のフォントを作成する。
				hFont = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
				::GetObjectW(hFont, sizeof(LOGFONTW), &lf);
				lf.lfHeight = cyPaper / 2 / 45;
				lf.lfWidth = 0;
				lf.lfWeight = FW_NORMAL;
				lf.lfQuality = ANTIALIASED_QUALITY;

				// ヒントの残りを描画する。
				hFont = ::CreateFontIndirectW(&lf);
				hFontOld = ::SelectObject(hdc, hFont);
				str = strHints;
				rc = { cxPaper / 8, cyPaper / 9, cxPaper * 7 / 8, cyPaper * 8 / 9 };
				::DrawTextW(hdc, str.data(), INT(str.size()), &rc, DT_LEFT | DT_TOP | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK);
				::SelectObject(hdc, hFontOld);
				::DeleteFont(hFont);

				// ページ終了。
				::EndPage(hdc);
			}
		}
		// 文書終了。
		::EndDoc(hdc);
	}

	::DeleteDC(hdc);
	::GlobalFree(ppd->hDevMode);
	::GlobalFree(ppd->hDevNames);
}

// 単語群から生成する。
void MMainWnd::GenerateFromWordList(const MStringListW& word_list, BOOL bUseAllWords, BOOL bNoGUI) {
	std::unordered_set<MStringW> words(word_list.begin(), word_list.end());

	MBoard board(XG_MIN_SIZE, XG_MIN_SIZE, RULE_DONTDIVIDE);
	std::vector<MBoard> solutions;
	MStringW strError;

	MCancelDialog dialog1;
	auto fnQuit = [&](HRESULT hr, std::vector<MBoard>& sols, size_t max_sols, MStringW& error) -> void {
		solutions = std::move(sols);
		strError = std::move(error);
	};
	dialog1.m_fnRun = [&]() -> HRESULT {
		return XgAlgo5(fnQuit, board, words, bUseAllWords, 1);
	};
	dialog1.m_fnCancel = []() -> void {
		XgAlgo5Cancel();
	};
	dialog1.m_fnEnd = [&](HRESULT hr) -> void {
		if (FAILED(hr)) {
			EndDialog(dialog1, IDCANCEL);
		} else {
			EndDialog(dialog1, IDOK);
		}
	};
	BeginTaskbarProgress();
	auto id1 = dialog1.DoModal(m_hwnd);
	EndTaskbarProgress();
	if (id1 == IDOK && solutions.size()) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		m_board = solutions[0];
		m_solution = solutions[0];
		xg_nRules = m_board.Rules();
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		FinalizeBoard();
		UpdateScrollInfo(0, 0);
		InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		if (m_bAutoSave)
			NumberingSave(m_hwnd, FALSE, bNoGUI);
		MyPlaySound(MY_SOUND_SUCCESS);
		if (!bNoGUI && !m_bNoGeneratedMsg)
			ShowResult(IDS_MADEPROBLEM, GetTickCount64() - dialog1.m_nStartTick, 1);
	}
	else {
		if (dialog1.m_hr == E_ABORT) {
			MyPlaySound(MY_SOUND_CANCELED);
			if (!bNoGUI && !m_bNoCanceledMsg)
				ShowResult(IDS_CANCELLED, GetTickCount64() - dialog1.m_nStartTick);
		}
		else if (dialog1.m_hr == S_FALSE) {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowResult(IDS_CANTMAKEPROBLEM, GetTickCount64() - dialog1.m_nStartTick);
			EndTaskbarProgress();
		}
		else {
			ErrorTaskbarProgress();
			MyPlaySound(MY_SOUND_FAILED);
			if (!bNoGUI)
				ShowError(dialog1.m_hr, strError);
			EndTaskbarProgress();
		}
	}
}

// WM_GETMINMAXINFO
void MMainWnd::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) {
	// メインウィンドウの最小サイズを指定。
	lpMinMaxInfo->ptMinTrackSize.x = 300;
	lpMinMaxInfo->ptMinTrackSize.y = 300;
}

//////////////////////////////////////////////////////////////////////

// ID_NEW
void MMainWnd::OnNew(HWND hwnd) {
	DestroyCluesWnd();
	DestroyCandsWnd();

	if (!DoConfirmSave(hwnd))
		return;

	auto& board = XgGetDisplayBoard(m_display);
	const INT cx = board.Columns(), cy = board.Rows();
	const INT nRules = board.Rules();

	MNewDialog dialog(cx, cy);
	auto id = dialog.DoModal(hwnd);
	switch (id) {
	case rad1:
	case rad2:
	case rad3:
		New(dialog.m_nColumns, dialog.m_nRows, nRules, (INT)id);
		break;
	case IDCANCEL:
		break;
	default:
		assert(0);
		break;
	}
}

// ID_GENERATE
void MMainWnd::OnGenerate(HWND hwnd) {
	DestroyCluesWnd();

	MGenDialog dialog0(m_dict, m_board.Columns(), m_board.Rows(), m_bChoosePAT, m_bShowAnswerOnGenerate);
	if (dialog0.DoModal(hwnd) != IDOK)
		return;

	// ここでいったん、盤面のサイズ変更を見せた方がいいらしい。
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
	m_bSolved = FALSE;
	m_board = MBoard(dialog0.m_nColumns, dialog0.m_nRows, xg_nRules);
	m_solution = m_board;
	m_caret_pos = { 0, 0 };
	m_vHorzInfo.clear();
	m_vVertInfo.clear();
	m_vMarks.clear();
	m_word2clue.clear();
	UpdateScrollInfo(0, 0);
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
	UpdateStatusBar(); // ステータスバーの更新も忘れずに。

	m_bChoosePAT = dialog0.m_bChoosePAT;
	m_bShowAnswerOnGenerate = dialog0.m_bShowAnswerOnGenerate;
	INT nMaxWordLen = dialog0.m_nMaxWordLen;

	// 実際の動作はパラメータを明示してこちらの関数で。
	Generate(hwnd, dialog0.m_nColumns, dialog0.m_nRows, xg_nRules, m_bChoosePAT, m_bShowAnswerOnGenerate, nMaxWordLen);
}

// ID_OPEN
BOOL MMainWnd::OnOpen(HWND hwnd) {
	if (!DoConfirmSave(hwnd))
		return FALSE;

	// ユーザーにファイルの場所を問い合わせる。
	WCHAR filename[MAX_PATH] = L"";
	auto filter = XgLoadString(IDS_CROSSFILTER);
	OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
	ofn.lpstrFilter = XgMakeFilterString(filter.data());
	ofn.lpstrFile = filename;
	ofn.nMaxFile = DWORD(_countof(filename));
	auto title = XgLoadString(IDS_OPENCROSSDATA);
	ofn.lpstrTitle = title.c_str();
	ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST |
				OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"xd";
	if (!::GetOpenFileNameW(&ofn))
		return FALSE;

	return LoadFile(hwnd, filename);
}

// ID_SAVE
BOOL MMainWnd::OnSave(HWND hwnd) {
	// ファイルが存在しなければ保存場所を問い合わせる。
	if (m_strFileName.empty() || !PathFileExistsW(m_strFileName.c_str()))
		return OnSaveAs(hwnd);

	// 上書き保存。
	if (!SaveFile(hwnd, m_strFileName.c_str()))
		return FALSE;

	return TRUE;
}

// ID_SAVEAS
BOOL MMainWnd::OnSaveAs(HWND hwnd) {
	// ユーザーにファイルの場所を問い合わせる準備。
	OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
	WCHAR filename[MAX_PATH] = L"";
	StringCchCopyW(filename, _countof(filename), m_strFileName.c_str());
	auto filter = XgLoadString(IDS_SAVEFILTER);
	ofn.lpstrFilter = XgMakeFilterString(filter.data());
	ofn.lpstrFile = filename;
	ofn.nMaxFile = DWORD(_countof(filename));
	auto title = XgLoadString(IDS_SAVECROSSDATA);
	ofn.lpstrTitle = title.c_str();
	ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_OVERWRITEPROMPT |
				OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = L"xd";
	ofn.nFilterIndex = 3;

	// ファイルの種類を決定する。
	if (lstrcmpiW(PathFindExtensionW(filename), L".xwj") == 0 ||
		lstrcmpiW(PathFindExtensionW(filename), L".json") == 0 ||
		lstrcmpiW(PathFindExtensionW(filename), L".jso") == 0)
	{
		PathRemoveExtensionW(filename);
		ofn.nFilterIndex = 1;
	}
	else if (lstrcmpiW(PathFindExtensionW(filename), L".crp") == 0)
	{
		PathRemoveExtensionW(filename);
		ofn.nFilterIndex = 2;
	}
	else if (lstrcmpiW(PathFindExtensionW(filename), L".xd") == 0)
	{
		PathRemoveExtensionW(filename);
		ofn.nFilterIndex = 3;
	}

	// ユーザーにファイルの場所を問い合わせる。
	if (!::GetSaveFileNameW(&ofn))
		return FALSE;

	// 保存する。
	if (!SaveFile(hwnd, filename))
		return FALSE;

	return TRUE;
}

// ID_SAVEPROBASIMAGE
void MMainWnd::OnSaveProbAsImage(HWND hwnd) {
	// 「解答を画像ファイルとして保存」ダイアログを表示。
	WCHAR szFileName[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
	auto filter = XgLoadString(IDS_IMGFILTER);
	ofn.lpstrFilter = XgMakeFilterString(&filter[0]);
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = _countof(szFileName);
	auto title = XgLoadString(IDS_SAVEPROBASIMG);
	ofn.lpstrTitle = &title[0];
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"png";
	ofn.nFilterIndex = 2;
	if (!::GetSaveFileNameW(&ofn))
		return;

	// 拡張子を判定。
	if (lstrcmpiW(PathFindExtensionW(szFileName), L".emf") == 0) {
		// EMFを保存する。
		HDC hdcRef = ::GetDC(hwnd);
		HDC hdc = ::CreateEnhMetaFileW(hdcRef, szFileName, nullptr, XgLoadString(IDS_APPNAME).c_str());
		if (hdc) {
			XgDrawBoard(m_display, m_board, hdc, nullptr, DRAW_MODE_EMF);
			::DeleteEnhMetaFile(::CloseEnhMetaFile(hdc));
		} else {
			ErrorBoxDx(IDS_CANTSAVE2);
		}
		::ReleaseDC(hwnd, hdcRef);
	} else {
		// ビットマップを保存する。
		HBITMAP hbm = XgBitmapFromBoard(m_display, m_display.m_board, nullptr, DRAW_MODE_PRINT);
		if (hbm != nullptr) {
			if (!XgSavePictureToFile(szFileName, hbm)) {
				ErrorBoxDx(IDS_CANTSAVE2);
			}
			::DeleteObject(hbm);
		} else {
			ErrorBoxDx(IDS_CANTSAVE2);
		}
	}
}

// ID_SOLVE
void MMainWnd::OnSolve(HWND hwnd) {
	// ゆるいルールチェック。
	if (!RuleCheck(hwnd, FALSE, TRUE))
		return;

	// 重複する単語がないか？
	MStringSetW used_word;
	if (!XgScanBoardPats(m_board, nullptr, &used_word)) {
		ErrorBoxDx(IDS_WORDDUPLICATE);
		return;
	}

	// 辞書にない単語がないか？
	for (auto& word : used_word) {
		auto it = m_dict.m_word2clue.find(word);
		if (it == m_dict.m_word2clue.end()) {
			WCHAR text[256];
			StringCchPrintfW(text, _countof(text), XgLoadString(IDS_NOTREGDWORD).c_str(),
							 word.c_str());
			if (MsgBoxDx(text, MB_ICONWARNING | MB_YESNOCANCEL) != IDYES) {
				return;
			}
			break;
		}
	}

	Solve(hwnd);
}

// ID_ABOUT
void MMainWnd::OnAbout(HWND hwnd) {
	MAboutDialog dialog;
	dialog.DoModal(hwnd);
}

// ID_SWAP_LEFT_RIGHT
void MMainWnd::OnSwapLeftRight(HWND hwnd) {
	if (m_display.m_bSolved)
		return;
	auto new_board = XgFlipBoardH(m_display.m_board);
	m_display.m_board = new_board;
	m_display.m_solution = std::move(new_board);
	m_display.m_bSolved = FALSE;
	m_display.m_caret_pos = { 0, 0 };
	UpdateCaretPos();
	InvalidateImage();
}

// ID_SWAP_TOP_BOTTOM
void MMainWnd::OnSwapTopBottom(HWND hwnd) {
	if (m_display.m_bSolved)
		return;
	auto new_board = XgFlipBoardV(m_display.m_board);
	m_display.m_board = new_board;
	m_display.m_solution = std::move(new_board);
	m_display.m_bSolved = FALSE;
	m_display.m_caret_pos = { 0, 0 };
	UpdateCaretPos();
	InvalidateImage();
}

// 盤面を塗りつぶす。
void MMainWnd::OnFill(HWND hwnd, BOOL bBlacks) {
	INT cx = m_display.m_board.Columns(), cy = m_display.m_board.Rows();
	INT nRules = m_display.m_board.Rules();
	auto new_board = MBoard(cx, cy, nRules);
	if (bBlacks) {
		new_board.clear(ZEN_BLACK);
	} else {
		new_board.clear(ZEN_SPACE);
	}
	m_display.m_board = new_board;
	m_display.m_solution = std::move(new_board);
	m_display.m_bSolved = FALSE;
	m_display.m_caret_pos = { 0, 0 };
	UpdateCaretPos();
	InvalidateImage();
}

// ID_OPENPATTERNS
void MMainWnd::OnOpenPatterns(HWND hwnd) {
	auto board = m_display.m_board;
	MPatternDialog dialog0(board);
	dialog0.m_bShowAnswerOnGenerate = m_bShowAnswerOnGenerate;
	auto id0 = dialog0.DoModal(hwnd);
	if (id0 == psh1) { // コピー。
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		m_display.m_board = dialog0.m_selected;
		m_display.m_solution = std::move(dialog0.m_selected);
		m_display.m_bSolved = FALSE;
		m_display.m_caret_pos = { 0, 0 };
		m_display.m_vMarks.clear();
		m_display.m_vHorzInfo.clear();
		m_display.m_vVertInfo.clear();
		m_display.m_mapNumCro1.clear();
		m_display.m_mapNumCro2.clear();
		m_display.m_bNumCroMode = FALSE;
		UpdateScrollInfo(0, 0);
		UpdateCaretPos();
		InvalidateImage();
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		if (m_bAutoSave)
			NumberingSave(hwnd, TRUE, FALSE);
		return;
	}
	if (id0 == IDCANCEL)
		return;
	board = std::move(dialog0.m_selected);
	if (id0 == IDOK) { // 解を求める？
		std::vector<MBoard> solutions;
		MStringW strError;

		MCancelDialog dialog1;
		dialog1.m_fnRun = [&]() -> HRESULT {
			return XgAlgo2(m_dict, board, solutions, strError, 1);
		};
		dialog1.m_fnCancel = []() -> void {
			XgAlgo2Cancel();
		};
		dialog1.m_fnEnd = [&](HRESULT hr) -> void {
			if (FAILED(hr)) {
				EndDialog(dialog1, IDCANCEL);
			} else {
				EndDialog(dialog1, IDOK);
			}
		};
		auto id1 = dialog1.DoModal(hwnd);
		if (id1 == IDOK && solutions.size()) {
			SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
			m_bShowAnswer = m_bShowAnswerOnGenerate = dialog0.m_bShowAnswerOnGenerate;
			m_board = solutions[0];
			m_solution = solutions[0];
			m_caret_pos = { 0, 0 };
			m_bCheckingAnswer = FALSE;
			FinalizeBoard();
			UpdateScrollInfo(0, 0);
			InvalidateImage();
			SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
			MyPlaySound(MY_SOUND_SUCCESS);
			if (!m_bNoGeneratedMsg)
				ShowResult(IDS_MADEPROBLEM, GetTickCount64() - dialog1.m_nStartTick, 1);
		} else {
			if (dialog1.m_hr == E_ABORT) {
				MyPlaySound(MY_SOUND_CANCELED);
				if (!m_bNoCanceledMsg)
					ShowResult(IDS_CANCELLED, GetTickCount64() - dialog1.m_nStartTick);
			} else if (dialog1.m_hr == S_FALSE) {
				MyPlaySound(MY_SOUND_FAILED);
				ShowResult(IDS_CANTMAKEPROBLEM, GetTickCount64() - dialog1.m_nStartTick);
			} else {
				MyPlaySound(MY_SOUND_FAILED);
				ShowError(dialog1.m_hr, strError);
			}
		}
	}
}

// ID_HEADERANDNOTES
void MMainWnd::OnHeaderAndNotes(HWND hwnd) {
	MNotesDialog dialog(m_header, m_notes);
	if (dialog.DoModal(hwnd) == IDOK) {
		m_header = dialog.m_header;
		m_notes = dialog.m_notes;
	}
}

// ID_COPYMARKWORD
void MMainWnd::OnCopyMarkWord(HWND hwnd) {
	// マークがなければ、実行を拒否する。
	if (m_vMarks.empty())
		return;

	// 描画サイズを取得する。
	const INT nCount = INT(m_vMarks.size());
	SIZE siz = XgGetMarkWordExtent(m_display, nCount);

	// EMFを作成する。
	HENHMETAFILE hEMF = nullptr;
	HDC hdcRef = ::GetDC(hwnd);
	HDC hdc = ::CreateEnhMetaFileW(hdcRef, nullptr, nullptr, XgLoadString(IDS_APPNAME).c_str());
	if (hdc) {
		// EMFに描画する。
		XgDrawMarkWord(m_display, hdc, &siz);
		hEMF = ::CloseEnhMetaFile(hdc);
	}
	::ReleaseDC(hwnd, hdcRef);

	auto& board = m_bSolved ? m_solution : m_board;

	// 二重マス単語のテキストを取得。
	HGLOBAL hText = nullptr;
	MStringW strMarkWord;
	if (XgGetMarkWord(m_display, strMarkWord)) {
		for (auto& wch : strMarkWord) {
			if (L'Ａ' <= wch && wch <= L'Ｚ')
				wch = L'A' + (wch - L'Ａ');
			else if (L'ａ' <= wch && wch <= L'ｚ')
				wch = L'A' + (wch - L'ａ');
		}

		// CF_UNICODETEXTのデータを用意。
		hText = XgCreateTextDataW(strMarkWord);
	}

	// クリップボードを開く。
	if (::OpenClipboard(hwnd)) {
		// クリップボードを空にする。
		if (::EmptyClipboard()) {
			// テキストを設定。
			if (hText)
				::SetClipboardData(CF_UNICODETEXT, hText);
			// EMFを設定。
			::SetClipboardData(CF_ENHMETAFILE, hEMF);
		}
		// クリップボードを閉じる。
		::CloseClipboard();
	}
}

// ID_COPYHHINTSSTYLE0, ID_COPYVHINTSSTYLE0
void MMainWnd::OnCopyHintsStyle0(HWND hwnd, INT nType) {
	assert(0 <= nType && nType <= 2);
	MStringW str;
	if (nType == 0 || nType == 2) { // タテのカギ
		str += XgTextFromDownCluesStyle0(m_vVertInfo, m_word2clue, m_bShowAnswer);
	}
	if (nType == 1 || nType == 2) { // ヨコのカギ
		str += XgTextFromAcrossCluesStyle0(m_vHorzInfo, m_word2clue, m_bShowAnswer);
	}
	XgSetClipboardUnicodeText(hwnd, str);
}

// ID_COPYHHINTSSTYLE1, ID_COPYVHINTSSTYLE1
void MMainWnd::OnCopyHintsStyle1(HWND hwnd, INT nType) {
	assert(0 <= nType && nType <= 2);

	// クロスワードの文字列を取得する。
	MStringW str;
	if (nType == 0 || nType == 2) { // タテのカギ
		str += XgTextFromDownCluesStyle1(m_vVertInfo, m_word2clue, m_bShowAnswer);
	}
	if (nType == 1 || nType == 2) { // ヨコのカギ
		str += XgTextFromAcrossCluesStyle1(m_vHorzInfo, m_word2clue, m_bShowAnswer);
	}

	// HTMLデータを用意する。
	MStringW htmlW;
	if (nType == 0 || nType == 2) { // タテのカギ
		htmlW += XgTextFromDownCluesHTML(m_vVertInfo, m_word2clue, m_bShowAnswer);
	}
	if (nType == 1 || nType == 2) { // ヨコのカギ
		htmlW += XgTextFromAcrossCluesHTML(m_vHorzInfo, m_word2clue, m_bShowAnswer);
	}
	XgStrTrim(htmlW);
	auto style = L"p, ol, li { margin-top: 0px; margin-bottom: 0px; }\r\n";
	MStringA htmlA = XgMakeClipHtmlData(htmlW, style);

	// クリップボードのHTML形式を登録する。
	const auto CF_HTML = ::RegisterClipboardFormatW(L"HTML Format");

	HGLOBAL hText = XgCreateTextDataW(str);
	HGLOBAL hHtml = XgCreateTextDataA(htmlA);
	// クリップボードを開く。
	if (::OpenClipboard(hwnd)) {
		// クリップボードを空にする。
		if (::EmptyClipboard()) {
			if (hText)
				::SetClipboardData(CF_UNICODETEXT, hText);
			if (hHtml)
				::SetClipboardData(CF_HTML, hHtml);
		}
		// クリップボードを閉じる。
		::CloseClipboard();
	}
}

// ID_COPY:
void MMainWnd::OnCopy(HWND hwnd) {
	XgCopyBoard(hwnd, m_display, TRUE, TRUE, TRUE);
}

// ID_PASTE
void MMainWnd::OnPaste(HWND hwnd) {
	auto text = XgGetClipboardUnicodeText(hwnd);
	if (text.find(ZEN_ULEFT) != text.npos && text.find(ZEN_LRIGHT) != text.npos) {
		MBoard board(3, 3, xg_nRules);
		if (!XgBoardFromText(board, text))
			return;

		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		DestroyCandsWnd();
		DestroyCluesWnd();
		m_vHorzInfo.clear();
		m_vVertInfo.clear();
		m_board = board;
		m_solution = board;
		m_bSolved = FALSE;
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		m_header.clear();
		m_notes.clear();
		m_word2clue.clear();
		m_mapNumCro1.clear();
		m_vMarks.clear();
		m_nViewMode = XG_VIEW_NORMAL;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		InvalidateImage();
	} else if (text.find(L'\n') != text.npos) {
		// エクセルからの貼り付けと仮定。タブ区切りのテキストと見なす。
		// "a\tb\tc"
		// "e\tf\tg"
		// "h\ti\tj"
		MBoard board(3, 3, xg_nRules);
		if (!XgBoardFromText2(board, text))
			return;

		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		DestroyCandsWnd();
		DestroyCluesWnd();
		m_vHorzInfo.clear();
		m_vVertInfo.clear();
		m_board = board;
		m_solution = board;
		m_bSolved = FALSE;
		m_caret_pos = { 0, 0 };
		m_bCheckingAnswer = FALSE;
		m_header.clear();
		m_notes.clear();
		m_word2clue.clear();
		m_mapNumCro1.clear();
		m_vMarks.clear();
		m_nViewMode = XG_VIEW_NORMAL;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		InvalidateImage();
	} else {
		// 単語の貼り付け。
		for (auto& ch : text) {
			OnImeChar(hwnd, ch, 0);
		}
	}
}

// ID_SAVEANSASIMAGE
void MMainWnd::OnSaveAnsAsImage(HWND hwnd) {
	if (!m_bSolved)
		return;

	// 「解答を画像ファイルとして保存」ダイアログを表示。
	WCHAR szFileName[MAX_PATH] = L"";
	OPENFILENAMEW ofn = { sizeof(ofn), hwnd };
	auto filter = XgLoadString(IDS_IMGFILTER);
	ofn.lpstrFilter = XgMakeFilterString(&filter[0]);
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = _countof(szFileName);
	auto title = XgLoadString(IDS_SAVEANSASIMG);
	ofn.lpstrTitle = &title[0];
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = L"png";
	ofn.nFilterIndex = 2;
	if (!::GetSaveFileNameW(&ofn))
		return;

	if (lstrcmpiW(PathFindExtensionW(szFileName), L".emf") == 0) {
		// EMFを保存する。
		HDC hdcRef = ::GetDC(hwnd);
		HDC hdc = ::CreateEnhMetaFileW(hdcRef, szFileName, nullptr, XgLoadString(IDS_APPNAME).c_str());
		if (hdc) {
			XgDrawBoard(m_display, m_solution, hdc, nullptr, DRAW_MODE_EMF);
			::DeleteEnhMetaFile(::CloseEnhMetaFile(hdc));
		} else {
			ErrorBoxDx(IDS_CANTSAVE2);
		}
		::ReleaseDC(hwnd, hdcRef);
	} else {
		// ビットマップを保存する。
		HBITMAP hbm = XgBitmapFromBoard(m_display, m_display.m_solution, nullptr, DRAW_MODE_PRINT);
		if (hbm != nullptr) {
			if (!XgSavePictureToFile(szFileName, hbm)) {
				ErrorBoxDx(IDS_CANTSAVE2);
			}
			::DeleteObject(hbm);
		} else {
			ErrorBoxDx(IDS_CANTSAVE2);
		}
	}
}

// ID_PRINTANSWER, ID_PRINTPROBLEM
void MMainWnd::OnPrint(HWND hwnd, BOOL bAnswer) {
	PRINTDLGW pd;
	ZeroMemory(&pd, sizeof(pd));
	pd.lStructSize = sizeof(pd);
	pd.hwndOwner = hwnd;
	pd.Flags = PD_ALLPAGES | PD_HIDEPRINTTOFILE | PD_NOPAGENUMS | PD_NOSELECTION | PD_RETURNDC;
	pd.nCopies = 1;
	if (::PrintDlgW(&pd)) {
		DoPrint(pd.hDC, &pd, bAnswer);
	} else {
		// キャンセルなら0。エラーなら共通ダイアログのエラーコードを表示する。
		const DWORD dwError = ::CommDlgExtendedError();
		if (dwError != 0) {
			WCHAR szText[128];
			::wsprintfW(szText, L"PrintDlgW failed: CommDlgExtendedError = 0x%04lX", dwError);
			::MessageBoxW(hwnd, szText, nullptr, MB_ICONERROR);
		}
		// 念のため、PrintDlgWが確保していたら解放する。
		if (pd.hDevMode)
			::GlobalFree(pd.hDevMode);
		if (pd.hDevNames)
			::GlobalFree(pd.hDevNames);
	}
}

// ID_CUT
void MMainWnd::OnCut(HWND hwnd) {
	::SendMessageW(::GetFocus(), WM_CUT, 0, 0);
}

// ID_PANENEXT, ID_PANEPREV
void MMainWnd::OnPaneNext(HWND hwnd, BOOL bPrev) {
	const HWND ahwnd[] = {
		m_hwnd,
		m_hCluesWnd,
		m_hCandsWnd,
		m_hInputPalette,
		m_hMarksDlg,
		m_hJumpDialog,
		m_ai_helper.GetWindow(),
	};
	const INT count = (INT)_countof(ahwnd);

	// 現在のフォアグラウンドウィンドウの位置を探す
	const HWND hwndFore = ::GetForegroundWindow();
	INT i = -1;
	for (INT j = 0; j < count; ++j) {
		if (ahwnd[j] == hwndFore) {
			i = j;
			break;
		}
	}

	// リストにないウィンドウがアクティブなら、メインウィンドウに戻す
	if (i < 0) {
		if (::IsWindowVisible(m_hwnd) && ::IsWindowEnabled(m_hwnd))
			::SetForegroundWindow(m_hwnd);
		return;
	}

	// 次: +1、前: -1 (count - 1 を足して剰余を取ることで負数を避ける)
	const INT step = bPrev ? (count - 1) : 1;
	for (INT k = 1; k < count; ++k) {
		const INT m = (i + k * step) % count;
		HWND hTarget = ahwnd[m];
		if (hTarget && ::IsWindowVisible(hTarget) && ::IsWindowEnabled(hTarget)) {
			::SetForegroundWindow(hTarget);
			break;
		}
	}
}

// ID_UILANGID
void MMainWnd::OnUILangID(HWND hwnd) {
	MUILanguageDialog dialog(m_wUILangID);
	if (dialog.DoModal(hwnd) == IDOK) {
		m_wUILangID = dialog.m_LangID;
		// 終了後にレジストリに保存され、再起動後に有効となる。
	}
}

// ID_KILLMARKS
void MMainWnd::OnKillMarks(HWND hwnd) {
	m_display.m_vMarks.clear();
	InvalidateImage();
}

// ID_ONLINEDICT, ID_ONLINEDICTH, ID_ONLINEDICTV
void MMainWnd::OnOnlineDict(HWND hwnd, BOOL bVertInput) {
	// パターンを取得する。
	auto& board = XgGetDisplayBoard(m_display);
	auto pattern = XgGetPatFromBoard(board, m_caret_pos.m_j, m_caret_pos.m_i, bVertInput);

	// 空白を含んでいたら、無視。
	if (pattern.find(ZEN_SPACE) != pattern.npos)
		return;

	// 「辞書」を追加。
	pattern += L' ';
	pattern += XgLoadString(IDS_DICTIONARY);

	// ウェブ検索。
	XgDoWebSearch(hwnd, pattern.c_str());
}

// WM_DROPFILES
void MMainWnd::OnDropFiles(HWND hwnd, HDROP hdrop) {
	WCHAR file[MAX_PATH];
	DragQueryFileW(hdrop, 0, file, _countof(file));

	if (lstrcmpiW(PathFindExtensionW(file), L".LOOKS") == 0) { // LOOKSファイルか？
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
		XgImportLooksFile(m_display, file);
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
		InvalidateImage();
		return;
	}

	if (DoConfirmSave(hwnd))
		LoadFile(hwnd, file);
}

// WM_INITMENUPOPUP
void MMainWnd::OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu) {
	if (fSystemMenu)
		return;

	// 辞書メニューを更新。
	InitDictMenu(hMenu);
	// ファイルメニューを更新。
	InitFileMenu(hMenu);

	// キャレットを表示するか？
	CheckMenuItem(hMenu, ID_SHOWHIDECARET, m_bShowCaret ? MF_CHECKED : MF_UNCHECKED);

	// 連黒禁。
	if (xg_nRules & RULE_DONTDOUBLEBLACK)
		::CheckMenuItem(hMenu, ID_RULE_DONTDOUBLEBLACK, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_DONTDOUBLEBLACK, MF_UNCHECKED);
	// 四隅黒禁。
	if (xg_nRules & RULE_DONTCORNERBLACK)
		::CheckMenuItem(hMenu, ID_RULE_DONTCORNERBLACK, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_DONTCORNERBLACK, MF_UNCHECKED);
	// 三方黒禁。
	if (xg_nRules & RULE_DONTTRIDIRECTIONS)
		::CheckMenuItem(hMenu, ID_RULE_DONTTRIDIRECTIONS, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_DONTTRIDIRECTIONS, MF_UNCHECKED);
	// 分断禁。
	if (xg_nRules & RULE_DONTDIVIDE)
		::CheckMenuItem(hMenu, ID_RULE_DONTDIVIDE, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_DONTDIVIDE, MF_UNCHECKED);
	// 黒斜三連禁。
	if (xg_nRules & RULE_DONTTHREEDIAGONALS) {
		::CheckMenuItem(hMenu, ID_RULE_DONTTHREEDIAGONALS, MF_CHECKED);
		::CheckMenuItem(hMenu, ID_RULE_DONTFOURDIAGONALS, MF_UNCHECKED);
	} else {
		::CheckMenuItem(hMenu, ID_RULE_DONTTHREEDIAGONALS, MF_UNCHECKED);
		// 黒斜四連禁。
		if (xg_nRules & RULE_DONTFOURDIAGONALS)
			::CheckMenuItem(hMenu, ID_RULE_DONTFOURDIAGONALS, MF_CHECKED);
		else
			::CheckMenuItem(hMenu, ID_RULE_DONTFOURDIAGONALS, MF_UNCHECKED);
	}
	// 黒マス点対称。
	if (xg_nRules & RULE_POINTSYMMETRY)
		::CheckMenuItem(hMenu, ID_RULE_POINTSYMMETRY, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_POINTSYMMETRY, MF_UNCHECKED);
	// 黒マス線対称。
	if (xg_nRules & RULE_LINESYMMETRYV)
		::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYV, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYV, MF_UNCHECKED);
	if (xg_nRules & RULE_LINESYMMETRYH)
		::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYH, MF_CHECKED);
	else
		::CheckMenuItem(hMenu, ID_RULE_LINESYMMETRYH, MF_UNCHECKED);

	// 入力モード。
	switch (m_imode) {
	case xg_im_KANA:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_KANAINPUT, MF_BYCOMMAND);
		break;
	case xg_im_ABC:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_ABCINPUT, MF_BYCOMMAND);
		break;
	case xg_im_KANJI:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_KANJIINPUT, MF_BYCOMMAND);
		break;
	case xg_im_RUSSIA:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_RUSSIAINPUT, MF_BYCOMMAND);
		break;
	case xg_im_GREEK:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_GREEKINPUT, MF_BYCOMMAND);
		break;
	case xg_im_DIGITS:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_DIGITINPUT, MF_BYCOMMAND);
		break;
	case xg_im_ANY:
		::CheckMenuRadioItem(hMenu, ID_KANAINPUT, ID_ANYINPUT, ID_ANYINPUT, MF_BYCOMMAND);
		break;
	}

	// 「元に戻す」「やり直し」メニュー更新。
	::EnableMenuItem(hMenu, ID_UNDO, CanUndo() ? MF_ENABLED : MF_GRAYED);
	::EnableMenuItem(hMenu, ID_REDO, CanRedo() ? MF_ENABLED : MF_GRAYED);

	if (m_bSolved) {
		::EnableMenuItem(hMenu, ID_SHOWHIDESOLUTION, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_SOLVE, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_SOLVENOADDBLACK, MF_GRAYED);
		if (m_bShowAnswer) {
			::EnableMenuItem(hMenu, ID_SHOWSOLUTION, MF_GRAYED);
			::EnableMenuItem(hMenu, ID_NOSOLUTION, MF_ENABLED);
			::CheckMenuRadioItem(hMenu, ID_SHOWSOLUTION, ID_NOSOLUTION, ID_SHOWSOLUTION, MF_BYCOMMAND);
			::CheckMenuItem(hMenu, ID_SHOWHIDESOLUTION, MF_CHECKED);
		} else {
			::EnableMenuItem(hMenu, ID_SHOWSOLUTION, MF_ENABLED);
			::EnableMenuItem(hMenu, ID_NOSOLUTION, MF_GRAYED);
			::CheckMenuRadioItem(hMenu, ID_SHOWSOLUTION, ID_NOSOLUTION, ID_NOSOLUTION, MF_BYCOMMAND);
			::CheckMenuItem(hMenu, ID_SHOWHIDESOLUTION, MF_UNCHECKED);
		}
		::EnableMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_MARKSNEXT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_SAVEANSASIMAGE, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_PRINTANSWER, MF_ENABLED);

		::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE0, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE1, MF_ENABLED);

		::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE0, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE0, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE1, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE1, MF_ENABLED);

		::EnableMenuItem(hMenu, ID_OPENCANDSWNDHORZ, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_OPENCANDSWNDVERT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_MOSTLEFT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_MOSTRIGHT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_MOSTUPPER, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_MOSTLOWER, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_LEFT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_RIGHT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_UP, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_DOWN, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_SWAP_LEFT_RIGHT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_SWAP_TOP_BOTTOM, MF_GRAYED);
	} else {
		::EnableMenuItem(hMenu, ID_SHOWHIDESOLUTION, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_SOLVE, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_SOLVENOADDBLACK, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_SHOWSOLUTION, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_NOSOLUTION, MF_GRAYED);
		::CheckMenuRadioItem(hMenu, ID_SHOWSOLUTION, ID_NOSOLUTION, ID_NOSOLUTION, MF_BYCOMMAND);
		::EnableMenuItem(hMenu, ID_SHOWHIDEHINTS, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_MARKSNEXT, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_SAVEANSASIMAGE, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_PRINTANSWER, MF_GRAYED);

		::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE0, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_COPYHINTSSTYLE1, MF_GRAYED);

		::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE0, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE0, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_COPYHHINTSSTYLE1, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_COPYVHINTSSTYLE1, MF_GRAYED);

		::EnableMenuItem(hMenu, ID_OPENCANDSWNDHORZ, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_OPENCANDSWNDVERT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_MOSTLEFT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_MOSTRIGHT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_MOSTUPPER, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_MOSTLOWER, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_LEFT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_RIGHT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_UP, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_DOWN, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_SWAP_LEFT_RIGHT, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_SWAP_TOP_BOTTOM, MF_ENABLED);
	}

	// 答え合わせ。
	::EnableMenuItem(hMenu, ID_CHECKANSWER, m_bSolved ? MF_ENABLED : MF_GRAYED);
	::CheckMenuItem(hMenu, ID_CHECKANSWER, m_bCheckingAnswer ? MF_CHECKED : MF_UNCHECKED);

	// 黒マスパターンを反射する。
	if (m_bSolved) {
		::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, MF_GRAYED);
	} else {
		if (xg_nRules & (RULE_LINESYMMETRYH | RULE_LINESYMMETRYV | RULE_POINTSYMMETRY)) {
			::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, MF_ENABLED);
		} else {
			::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, MF_GRAYED);
		}
	}

	// 二重マスメニュー更新。
	if (m_display.m_vMarks.empty()) {
		::EnableMenuItem(hMenu, ID_KILLMARKS, MF_GRAYED);
		::EnableMenuItem(hMenu, ID_COPYMARKWORD, MF_GRAYED);
	} else {
		::EnableMenuItem(hMenu, ID_KILLMARKS, MF_ENABLED);
		::EnableMenuItem(hMenu, ID_COPYMARKWORD, MF_ENABLED);
	}

	// 「解を削除して盤のロックを解除」
	::EnableMenuItem(hMenu, ID_ERASESOLUTIONANDUNLOCKEDIT, m_bSolved ? MF_ENABLED : MF_GRAYED);

	// ステータスバーのメニュー更新。
	::CheckMenuItem(hMenu, ID_STATUS, m_bShowStatusBar ? MF_CHECKED : MF_UNCHECKED);

	// 入力パレットのメニュー更新。
	if (m_hInputPalette) {
		::CheckMenuItem(hMenu, ID_PALETTE, MF_CHECKED);
		::CheckMenuItem(hMenu, ID_PALETTE2, MF_CHECKED);
	} else {
		::CheckMenuItem(hMenu, ID_PALETTE, MF_UNCHECKED);
		::CheckMenuItem(hMenu, ID_PALETTE2, MF_UNCHECKED);
	}

	// ツールバーのメニュー更新。
	::CheckMenuItem(hMenu, ID_TOOLBAR, m_bShowToolBar ? MF_CHECKED : MF_UNCHECKED);

	// ヒントウィンドウのメニュー更新。
	::CheckMenuItem(hMenu, ID_SHOWHIDEHINTS, m_hCluesWnd ? MF_CHECKED : MF_UNCHECKED);

	// ひらがなウィンドウのメニュー更新。
	::CheckMenuItem(hMenu, ID_HIRAGANA, m_bHiragana ? MF_CHECKED : MF_UNCHECKED);

	// Lowercaseウィンドウのメニュー更新。
	::CheckMenuItem(hMenu, ID_LOWERCASE, m_bLowercase ? MF_CHECKED : MF_UNCHECKED);

	// タテヨコ入力のメニュー更新。
	::CheckMenuRadioItem(hMenu, ID_INPUTH, ID_INPUTV, m_bVertInput ? ID_INPUTV : ID_INPUTH, MF_BYCOMMAND);

	// 文字送りのメニュー更新。
	::CheckMenuItem(hMenu, ID_CHARFEED, m_bCharFeed ? MF_CHECKED : MF_UNCHECKED);

	// 二重マス文字表示のメニュー更新。
	::CheckMenuItem(hMenu, ID_VIEW_DOUBLEFRAME_LETTERS, m_bShowDoubleFrameLetters ? MF_CHECKED : MF_UNCHECKED);

	// 二重マス表示のメニュー更新。
	::CheckMenuItem(hMenu, ID_VIEW_DOUBLEFRAME, m_bShowDoubleFrame ? MF_CHECKED : MF_UNCHECKED);

	// ビューモード。
	::CheckMenuItem(hMenu, ID_VIEW_SKELETON_VIEW, (m_nViewMode == XG_VIEW_SKELETON) ? MF_CHECKED : MF_UNCHECKED);

	// ナンクロモード。
	if (!m_bSolved) {
		::EnableMenuItem(hMenu, ID_NUMCROMODE, MF_GRAYED);
	} else {
		::EnableMenuItem(hMenu, ID_NUMCROMODE, MF_ENABLED);
		if (m_bNumCroMode) {
			::CheckMenuItem(hMenu, ID_NUMCROMODE, MF_CHECKED);
		} else {
			::CheckMenuItem(hMenu, ID_NUMCROMODE, MF_UNCHECKED);
		}
	}

	// 行と列の削除と追加。
	if (m_bSolved || m_board.Rows() - 1 < XG_MIN_SIZE) {
		::DeleteMenu(hMenu, ID_DELETE_ROW, MF_BYCOMMAND);
	}
	if (m_bSolved || m_board.Columns() - 1 < XG_MIN_SIZE) {
		::DeleteMenu(hMenu, ID_DELETE_COLUMN, MF_BYCOMMAND);
	}
	if (m_bSolved || m_board.Rows() + 1 > XG_MAX_SIZE) {
		::DeleteMenu(hMenu, ID_INSERT_ROW_ABOVE, MF_BYCOMMAND);
		::DeleteMenu(hMenu, ID_INSERT_ROW_BELOW, MF_BYCOMMAND);
	}
	if (m_bSolved || m_board.Columns() + 1 > XG_MAX_SIZE) {
		::DeleteMenu(hMenu, ID_LEFT_INSERT_COLUMN, MF_BYCOMMAND);
		::DeleteMenu(hMenu, ID_RIGHT_INSERT_COLUMN, MF_BYCOMMAND);
	}

	// ズーム倍率。
	const INT nZoom = m_hCanvasWnd.GetZoom();
	::CheckMenuItem(hMenu, ID_ZOOM100, (nZoom == 100) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_ZOOM30, (nZoom == 30) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_ZOOM50, (nZoom == 50) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_ZOOM65, (nZoom == 65) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_ZOOM80, (nZoom == 80) ? MF_CHECKED : MF_UNCHECKED);
	::CheckMenuItem(hMenu, ID_ZOOM90, (nZoom == 90) ? MF_CHECKED : MF_UNCHECKED);

	// 黒マスヘルプ機能
	::EnableMenuItem(hMenu, ID_SWAP_LEFT_RIGHT, m_bSolved ? MF_GRAYED : MF_ENABLED);
	::EnableMenuItem(hMenu, ID_SWAP_TOP_BOTTOM, m_bSolved ? MF_GRAYED : MF_ENABLED);
	::EnableMenuItem(hMenu, ID_REFLECTBLOCKS, m_bSolved ? MF_GRAYED : MF_ENABLED);
}

// ID_COPYWORDHORZ, ID_COPYWORDVERT
void MMainWnd::OnCopyWord(HWND hwnd, BOOL bVert) {
	// 現在の位置から文字列を取得する。
	auto& board = XgGetDisplayBoard(m_display);
	auto pat = XgGetPatFromBoard(board, m_caret_pos.m_j, m_caret_pos.m_i, bVert);

	// 自然な文字列に変換する。
	pat = XgNaturalizeString(pat);

	XgSetClipboardUnicodeText(hwnd, pat);
}

// ID_INSERT_ROW_ABOVE, ID_INSERT_ROW_BELOW
void MMainWnd::OnInsertRow(HWND hwnd, BOOL bBelow) {
	if (m_bSolved)
		return;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
	if (bBelow) {
		XgInsertRows(m_board, m_caret_pos.m_i + 1);
	} else {
		XgInsertRows(m_board, m_caret_pos.m_i);
	}
	m_solution = m_board;
	UpdateScrollInfo();
	UpdateCaretPos();
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
}

// ID_LEFT_INSERT_COLUMN, ID_RIGHT_INSERT_COLUMN
void MMainWnd::OnInsertColumn(HWND hwnd, BOOL bRight) {
	if (m_bSolved)
		return;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
	if (bRight) {
		XgInsertColumns(m_board, m_caret_pos.m_j + 1);
	} else {
		XgInsertColumns(m_board, m_caret_pos.m_j);
	}
	m_solution = m_board;
	UpdateScrollInfo();
	UpdateCaretPos();
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
}

// ID_DELETE_COLUMN
void MMainWnd::OnDeleteRow(HWND hwnd) {
	if (m_bSolved)
		return;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
	XgDeleteRow(m_board, m_caret_pos.m_i);
	m_solution = m_board;
	UpdateScrollInfo();
	UpdateCaretPos();
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
}

// ID_DELETE_COLUMN
void MMainWnd::OnDeleteColumn(HWND hwnd) {
	if (m_bSolved)
		return;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
	XgDeleteColumn(m_board, m_caret_pos.m_j);
	m_solution = m_board;
	UpdateScrollInfo();
	UpdateCaretPos();
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, 0);
}

// ID_COPYCHARSET
void MMainWnd::OnCopyCharSet(HWND hwnd) {
	auto& board = m_bSolved ? m_solution : m_board;

	std::multiset<WCHAR> multiset; // ソート済みになる。
	auto data = board.Data();

	// 文字マスだけを追加。
	for (auto ch : data) {
		if (XgIsLetter(ch))
			multiset.insert(ch);
	}

	// 空白区切りに変換。
	MStringW str;
	for (auto ch : multiset) {
		if (str.size())
			str += L' ';
		str += ch;
	}

	// 自然な文字列に変換する。
	str = XgNaturalizeString(str);

	// クリップボードにテキストをコピーする。
	XgSetClipboardUnicodeText(hwnd, str);
}

// ID_COPY_WORD_LIST
void MMainWnd::OnCopyWordList(HWND hwnd) {
	auto& board = m_bSolved ? m_solution : m_board;

	// 盤面にある全単語を取得。空白を含むものや、2文字未満は無視。
	MStringSetW words = XgWordsFromBoard(board);

	// ソート。
	MStringListW word_list(words.begin(), words.end());
	std::sort(word_list.begin(), word_list.end());

	// 改行区切りにする。
	auto str = XgStrJoin(word_list, L"\r\n");
	str += L"\r\n";

	// 自然な文字列に変換する。
	str = XgNaturalizeString(str);

	// クリップボードにテキストをコピーする。
	XgSetClipboardUnicodeText(hwnd, str);
}

// ID_CLEARNONBLOCKS
void MMainWnd::OnClearNonBlocks(HWND hwnd) {
	if (m_bSolved) {
		m_bShowAnswer = FALSE;
	}

	for (INT i = 0; i < m_board.Rows(); ++i) {
		for (INT j = 0; j < m_board.Columns(); ++j) {
			WCHAR oldch = m_board.GetCell0(j, i);
			if (XgIsLetter(oldch)) {
				m_board.SetCell0(j, i, ZEN_SPACE);
			}
		}
	}

	InvalidateImage();
	m_prev_vk = 0;
}

// ID_FLIPVH
void MMainWnd::OnFlipVH(HWND hwnd) {
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
	std::swap(m_caret_pos.m_j, m_caret_pos.m_i);

	if (m_bSolved) {
		auto word2clue = m_word2clue;
		DestroyCluesWnd();

		m_board = XgTransposeBoard(m_board);
		m_solution = XgTransposeBoard(m_solution);
		assert(m_solution.IsFull());

		XgDoNumbering(m_solution, m_vHorzInfo, m_vVertInfo);
		m_word2clue = std::move(word2clue);
		m_bSolved = TRUE;

		CreateCluesWnd(hwnd);
	} else {
		m_board = XgTransposeBoard(m_board);
		m_solution = XgTransposeBoard(m_solution);
	}

	UpdateScrollInfo();
	UpdateCaretPos();
	InvalidateImage();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);
	m_prev_vk = 0;
}

// ID_OPENBBS
void MMainWnd::OnOpenBSS(HWND hwnd) {
	static const LPCWSTR bss = L"http://katahiromz.bbs.fc2.com/";
	::ShellExecuteW(hwnd, nullptr, bss, nullptr, nullptr, SW_SHOWNORMAL);
}

// ID_RULEPRESET
void MMainWnd::OnRulePreset(HWND hwnd) {
	OpenConfigPropSheet(hwnd, IPAGE_RULES);
}

// ID_LINESYMMETRYCHECK
void MMainWnd::OnLineSymmetryCheck(HWND hwnd) {
	auto& board = XgGetDisplayBoard(m_display);
	if (board.IsLineSymmetryH() || board.IsLineSymmetryV()) {
		MsgBoxDx(IDS_LINESYMMETRY, IDS_APPNAME, MB_ICONINFORMATION);
	} else {
		MsgBoxDx(IDS_NOTLINESYMMETRY, IDS_APPNAME, MB_ICONINFORMATION);
	}
}

// ID_POINTSYMMETRYCHECK
void MMainWnd::OnPointSymmetryCheck(HWND hwnd) {
	auto& board = XgGetDisplayBoard(m_display);
	if (board.IsPointSymmetry()) {
		MsgBoxDx(IDS_POINTSYMMETRY, IDS_APPNAME, MB_ICONINFORMATION);
	} else {
		MsgBoxDx(IDS_NOTPOINTSYMMETRY, IDS_APPNAME, MB_ICONINFORMATION);
	}
}

// ID_GENERATEBLACKS
void MMainWnd::OnGenerateBlacks(HWND hwnd) {
	const INT nColumns = m_board.Columns(), nRows = m_board.Rows();

	// 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
	if (!(nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
		ErrorBoxDx(IDS_EVENROWLINESYMV);
		return;
	}

	// 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
	if (!(nColumns & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
		ErrorBoxDx(IDS_EVENCOLLINESYMH);
		return;
	}

	const INT nMaxWordLen = XgGetPreferredMaxLength(m_dict);
	MPatGenDialog dialog(nMaxWordLen);
	if (dialog.DoModal(hwnd) != IDOK)
		return;

	GenerateBlacks(hwnd, dialog.m_nMaxWordLen);
}

// ID_SOLVENOADDBLACK
void MMainWnd::OnSolveNoAddBlack(HWND hwnd) {
	// 厳しいルールチェック。
	if (!RuleCheck(hwnd, FALSE, FALSE))
		return;

	// 重複する単語がないか？
	MStringSetW used_word;
	if (!XgScanBoardPats(m_board, nullptr, &used_word)) {
		ErrorBoxDx(IDS_WORDDUPLICATE);
		return;
	}

	// 辞書にない単語がないか？
	for (auto& word : used_word) {
		auto it = m_dict.m_word2clue.find(word);
		if (it == m_dict.m_word2clue.end()) {
			WCHAR text[256];
			StringCchPrintfW(text, _countof(text), XgLoadString(IDS_NOTREGDWORD).c_str(),
							 word.c_str());
			if (MsgBoxDx(text, MB_ICONWARNING | MB_YESNOCANCEL) != IDYES) {
				return;
			}
			break;
		}
	}

	SolveNoAddBlack(hwnd);
}

// ID_REFLECTBLOCKS
void MMainWnd::OnReflectBlocks(HWND hwnd) {
	if (m_bSolved)
		return;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_BOARD, 0);
	BOOL bChanged = FALSE;
	for (INT iRow = 0; iRow < m_board.Rows(); ++iRow) {
		for (INT jColumn = 0; jColumn < m_board.Columns(); ++jColumn) {
			if (m_board.GetCell0(jColumn, iRow) == ZEN_BLACK) {
				m_board.MirroredSetCell(jColumn, iRow, ZEN_BLACK);
				bChanged = TRUE;
			}
		}
	}
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_BOARD, !bChanged);
	InvalidateImage();
	m_prev_vk = 0;
}

// ID_GENERATEFROMWORDLIST
void MMainWnd::OnGenerateFromWordList(HWND hwnd) {
	MWordListDialog dialog0;
	auto id0 = dialog0.DoModal(hwnd);
	if (id0 != IDOK)
		return;

	GenerateFromWordList(MWordListDialog::s_word_list, dialog0.m_bUseAllWords, FALSE);
}

// ID_SHOWHIDEHINTS
void MMainWnd::OnShowHideHints(HWND hwnd) {
	if (!m_bSolved)
		return;

	if (m_hCluesWnd)
		DestroyCluesWnd();
	else
		CreateCluesWnd(hwnd);
}

// ID_ERASESETTINGS
void MMainWnd::OnEraseSettings(HWND hwnd) {
	// 消去するのか確認。
	if (MsgBoxDx(IDS_QUERYERASESETTINGS, MB_ICONWARNING | MB_YESNO) != IDYES)
		return;

	// ウィンドウを破棄する。
	DestroyCandsWnd();
	DestroyCluesWnd();
	DestroyInputPalette();
	m_ai_helper.CloseGUI();

	// 実際に設定を消す。
	ResetSettings();
	RegDeleteKeyW(HKEY_CURRENT_USER, XG_REGKEY_APP);

	// 表示を設定に合わせる。
	ShowStatusBar(m_bShowStatusBar);
	ShowToolBar(m_bShowToolBar);
	UpdateCaretPos();
	UpdateScrollInfo(0, 0);
	InvalidateImage();
}

// ID_JUMP
void MMainWnd::OnJump(HWND hwnd) {
	if (m_hJumpDialog) {
		PostMessageW(m_hJumpDialog, DM_REPOSITION, 0, 0);
		::SetForegroundWindow(m_hJumpDialog);
		return;
	}
	m_hJumpDialog.SetData(m_caret_pos.m_j, m_caret_pos.m_i);
	m_hJumpDialog.CreateDx(hwnd);
}

// ID_JUMPNUMBER
void MMainWnd::OnJumpNumber(HWND hwnd) {
	JumpNumber(m_hCluesWnd.m_nNumber, m_hCluesWnd.m_bVert);
}

// ID_CLEARTHISCLUE
void MMainWnd::OnClearThisClue(HWND hwnd) {
	ClearThisClue(m_hCluesWnd.m_nNumber, m_hCluesWnd.m_bVert);
}

// ID_CLEARALLCLUES
void MMainWnd::OnClearAllClues(HWND hwnd) {
	ClearAllClues();
}

// XGWM_HIGHLIGHT1
LRESULT MMainWnd::OnHighlight1(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	if (wParam) {
		SHORT number = (SHORT)LOWORD(lParam);
		BOOL vertical = !!HIWORD(lParam);
		if (xg_highlight.m_number != number || xg_highlight.m_vertical != vertical) {
			xg_highlight.m_number = number;
			xg_highlight.m_vertical = vertical;
			InvalidateImage();
		}
	}
	return TRUE;
}

// XGWM_HIGHLIGHT2
LRESULT MMainWnd::OnHighlight2(HWND hwnd, WPARAM wParam, LPARAM lParam) {
	// ヒントウィンドウのハイライトを設定する。
	INT nHorz = -1, nVert = -1;
	for (auto& info : m_vHorzInfo) {
		if (info.m_jCol == m_caret_pos.m_j && info.m_iRow == m_caret_pos.m_i) {
			nHorz = info.m_number;
			break;
		}
	}
	for (auto& info : m_vVertInfo) {
		if (info.m_jCol == m_caret_pos.m_j && info.m_iRow == m_caret_pos.m_i) {
			nVert = info.m_number;
			break;
		}
	}
	m_hCluesWnd.SetHighlight(nHorz, nVert);
	UpdateStatusBar();
	return TRUE;
}

// ID_OPENCANDSWNDHORZ, ID_OPENCANDSWNDVERT
void MMainWnd::OnOpenCandsWnd(HWND hwnd, BOOL bVert) {
	m_hCandsWnd.Open(hwnd, bVert);
}

// ID_RECENT_00 ... ID_RECENT_09
void MMainWnd::OnRecent(HWND hwnd, size_t index) {
	if (index < m_recently_used_files.size()) {
		LoadFile(hwnd, m_recently_used_files[index].c_str());
	}
}

// ID_TOGGLEMARK
void MMainWnd::OnTottleMark(HWND hwnd) {
	if (!m_bSolved)
		return;

	const INT j = m_caret_pos.m_j, i = m_caret_pos.m_i;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_MARKS, 0);
	BOOL bChanged = FALSE;
	if (XgGetMarked(m_display, j, i) == -1) { // マークされていないか？
		if (m_solution.GetCell(j, i) != ZEN_BLACK) {
			XgSetMark(m_display, j, i);
			bChanged = TRUE;
		}
	} else { // マークがセットされている。マークを解除する。
		XgDeleteMark(m_display, j, i);
		bChanged = TRUE;
	}
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_MARKS, !bChanged);
	SendMessageW(xg_hMainWnd, XGWM_MARKSUPDATE, 0, 0);

	InvalidateImage();
	UpdateTitleBarText();
}

// ID_ERASESOLUTIONANDUNLOCKEDIT
void MMainWnd::OnEraseSolutionAndUnlockEdit(HWND hwnd) {
	if (!m_bSolved)
		return;

	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);
	DestroyCandsWnd();
	DestroyCluesWnd();
	m_bSolved = FALSE;
	m_board = m_solution;
	m_vHorzInfo.clear();
	m_vVertInfo.clear();
	m_vMarks.clear();
	m_word2clue.clear();
	m_bNumCroMode = FALSE;
	m_mapNumCro1.clear();
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, 0, 0);

	InvalidateImage();
	UpdateTitleBarText();
}

// ID_CHECKANSWER
void MMainWnd::OnCheckAnswer(HWND hwnd) {
	if (m_bSolved) {
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, XG_UNDO_VIEW, 0);
		m_bCheckingAnswer = !m_bCheckingAnswer;
		SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, XG_UNDO_VIEW, 0);
		InvalidateImage();
		CheckAnswer();
	}
}

// ID_EXPLAINTHISCLUE
void MMainWnd::OnExplainThisClue() {
	INT nNumber = m_hCluesWnd.m_nNumber;
	BOOL bVert = m_hCluesWnd.m_bVert;
	m_ai_helper.ExplainClue(nNumber, bVert);
}

// ID_GENERATEHINT
void MMainWnd::OnGenerateClue() {
	INT nNumber = m_hCluesWnd.m_nNumber;
	BOOL bVert = m_hCluesWnd.m_bVert;
	m_ai_helper.GenerateClue(nNumber, bVert);
}

// ID_AI_CUSTOM_PROMPT_1, ID_AI_CUSTOM_PROMPT_2
void MMainWnd::OnCustomPrompt(INT type) {
	INT nNumber = m_hCluesWnd.m_nNumber;
	BOOL bVert = m_hCluesWnd.m_bVert;
	MStringW clue_name = bVert ? L"D" : L"A";
	clue_name += to_mstrW(nNumber);
	m_ai_helper.CustomPrompt(type, clue_name);
}

// ID_SETTINGS
void MMainWnd::OnSettings(HWND hwnd) {
	OpenConfigPropSheet(hwnd, IPAGE_LOOKS);
}

// ID_THEMETAGS
void MMainWnd::OnThemeTags(HWND hwnd) {
	if (m_dict.m_tag_histgram.empty()) { // タグがない？
		ErrorBoxDx(IDS_NOTAGS);
		return;
	}

	MThemeDialog dialog(m_dict);
	if (dialog.DoModal(hwnd) != IDOK)
		return;

	m_dict.load_file(m_dict_name.c_str());
	m_dict.set_theme_tags(dialog.m_theme_tags);
	m_dict.m_priority_tags = dialog.m_priority_tags;
	m_dict.m_forbidden_tags = dialog.m_forbidden_tags;
	m_dict.update_theme_words();
	UpdateStatusBar();
}

// ID_DEBUGACTION1
void MMainWnd::OnDebugAction1(HWND hwnd) {
	assert(0);
	s_sw.Unregister(hwnd, TRUE);
}

// ID_DEBUGACTION2
void MMainWnd::OnDebugAction2(HWND hwnd) {
	assert(0);
}

// ID_OPENAIHELPER
void MMainWnd::OnOpenAIHelper(HWND hwnd) {
	if (m_bServerAccessConfirmed || MsgBoxDx(IDS_SERVERACCESSCONFIRM, MB_YESNOCANCEL) == IDYES) {
		m_bServerAccessConfirmed = TRUE;
		m_ai_helper.OpenGUI(hwnd);
	}
}

//////////////////////////////////////////////////////////////////////

static UINT uTaskbarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

LRESULT CALLBACK
MMainWnd::WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hwnd, WM_MOVE, OnMove);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
		HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
		HANDLE_MSG(hwnd, WM_KEYUP, OnKey);
		HANDLE_MSG(hwnd, WM_CHAR, OnChar);
		HANDLE_MSG(hwnd, WM_CLOSE, OnClose);
		HANDLE_MSG(hwnd, WM_INITMENUPOPUP, OnInitMenuPopup);
		HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
		HANDLE_MSG(hwnd, WM_GETMINMAXINFO, OnGetMinMaxInfo);
	case WM_IME_CHAR:
		OnImeChar(hwnd, WCHAR(wParam), lParam);
		break;
	case XGWM_RECREATEINPUTPALETTE:
		ReCreateInputPalette(hwnd, m_imode);
		break;
	case XGWM_SETINPUTMODE:
		m_imode = (MInputMode)wParam;
		ReCreateInputPalette(hwnd, m_imode);
		break;
	case XGWM_FILECHANGED:
		m_bFileModified = TRUE;
		UpdateTitleBarText();
		UpdateStatusBar();
		break;
	case XGWM_HIGHLIGHT1: // ハイライト1。
		return OnHighlight1(hwnd, wParam, lParam);
	case XGWM_HIGHLIGHT2: // ハイライト2。
		return OnHighlight2(hwnd, wParam, lParam);
	case XGWM_UNDO_UPDATING: // 「元に戻す」情報の更新開始。
		return OnUndoUpdate(hwnd, uMsg, wParam, lParam);
	case XGWM_UNDO_UPDATED: // 「元に戻す」情報の更新完了。
		return OnUndoUpdate(hwnd, uMsg, wParam, lParam);
	case XGWM_MARKSUPDATE: // 二重マスが更新された。
		if (m_bSolved && m_hMarksDlg)
			m_hMarksDlg.RefreshMarks();
		break;
	case XGWM_GETTHUMBNAILEXTENT: // サムネイルに表示する範囲(メイン ウィンドウのクライアント座標)。
		if (wParam && lParam) {
			RECT rc = {};
			if (m_hCanvasWnd && ::GetClientRect(m_hCanvasWnd, &rc)) {
				// キャンバスのクライアント領域(スクロール バーを除く)を、メイン ウィンドウの座標へ。
				::MapWindowPoints(m_hCanvasWnd, hwnd, (LPPOINT)&rc, 2);
				// 盤面がキャンバスより小さいときは、盤面の範囲に絞る。
				// (大きいときはスクロールできるので、キャンバス全体が盤面で埋まっている)
				const SIZE siz = XgGetBoardExtent(m_display, DRAW_MODE_SCREEN);
				if (rc.right - rc.left > siz.cx)
					rc.right = rc.left + siz.cx;
				if (rc.bottom - rc.top > siz.cy)
					rc.bottom = rc.top + siz.cy;
			}
			*(LPRECT)lParam = rc;
			*(LPSIZE)wParam = { rc.right - rc.left, rc.bottom - rc.top };
			return TRUE;
		}
		break;
	case WM_SETTINGCHANGE:
		// 環境変数を同期する。
		RefreshEnvironment(FALSE);
		break;
	default:
		if (uMsg == uTaskbarButtonCreated) {
			if (m_pTaskbarProgress)
				m_pTaskbarProgress->SetThumbnail();
		}
		return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////

BOOL MMainWnd::InitInstance(HINSTANCE hInst) {
	xg_hInstance = hInst;
	InitCommonControls();

	LoadSettings();

	// 環境変数の同期を開始する。
	InitEnvironmentTracking();

	if (!CreateMainWnd()) {
		MessageBoxA(nullptr, "CreateWindow failed.", nullptr, MB_ICONERROR);
		XgUninitialize();
		return FALSE;
	}

	ClearEnvironment();

	return TRUE;
}

BOOL MMainWnd::CreateMainWnd() {
	DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
	DWORD exstyle = WS_EX_ACCEPTFILES;
	HWND hwnd = CreateWindowDx(nullptr, LoadStringDx(IDS_APPINFO), style, exstyle,
							   m_nWindowX, m_nWindowY, m_nWindowCX, m_nWindowCY);
	if (!hwnd)
		return FALSE;

	ShowWindow(hwnd, (m_bMaximized ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL));
	UpdateWindow(hwnd);
	return TRUE;
}

INT MMainWnd::Run() {
	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0)) {
		// F6 / Shift+F6 はモードレスダイアログ上でも有効にする
		// (IsDialogMessageW がキーを消費する前に処理する)
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_F6 &&
			::GetKeyState(VK_CONTROL) >= 0 && ::GetKeyState(VK_MENU) >= 0)
		{
			OnPaneNext(m_hwnd, ::GetKeyState(VK_SHIFT) < 0);
			continue;
		}
		if (m_hInputPalette && IsDialogMessageW(m_hInputPalette, &msg))
			continue;
		if (m_hJumpDialog && IsDialogMessageW(m_hJumpDialog, &msg))
			continue;
		if (m_hMarksDlg && IsDialogMessageW(m_hMarksDlg, &msg))
			continue;
		if (m_ai_helper.GetWindow() && IsDialogMessageW(m_ai_helper.GetWindow(), &msg))
			continue;
		if (TranslateAcceleratorW(m_hwnd, m_hAccel, &msg))
			continue;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	SaveSettings();
	return INT(msg.wParam);
}
