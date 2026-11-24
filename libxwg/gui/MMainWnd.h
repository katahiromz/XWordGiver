// MMainWnd.h --- クロスワードギバー メインウィンドウ。
// Author: katahiromz
// License: MIT

#include "MWindowBase.h"
#include "MInputPalette.h"
#include "MCanvasWindow.h"
#include "MCandsWnd.h"
#include "MCluesWnd.h"
#include "MJumpDialog.h"
#include "MMarksDialog.h"
#include "MAIHelper.h"
#include "TaskbarProgress.h"

#define IDW_TOOLBAR 1
#define IDW_STATUSBAR 2

class MMainWnd : public MWindowBase {
public:
	MMainWnd(INT argc, LPWSTR* wargv);
	~MMainWnd();

	LPCTSTR GetWndClassNameDx() const override;
	VOID ModifyWndClassDx(WNDCLASSEX& wcx) override;

	BOOL InitInstance(HINSTANCE hInst);
	INT Run();

	LRESULT CALLBACK WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	//////////////////
	// アクション。

	void New(INT nColumns, INT nRows, INT nRules, INT id, BOOL bNoGUI = FALSE);
	void Generate(HWND hwnd, INT nColumns, INT nRows, INT nRules, BOOL bChoosePAT = TRUE, BOOL bShowAnswerOnGenerate = TRUE, INT nMaxWordLen = 5, BOOL bNoGUI = FALSE);
	void Solve(HWND hwnd, BOOL bNoGUI = FALSE);
	void SolveNoAddBlack(HWND hwnd, BOOL bNoGUI = FALSE);
	void GenerateBlacks(HWND hwnd, INT nMaxWordLen = 5, BOOL bNoGUI = FALSE);
	void GenerateFromWordList(const MStringListW& word_list, BOOL bUseAllWords, BOOL bNoGUI = FALSE);

public:
	INT m_argc;
	LPWSTR* m_wargv;
	LANGID m_wUILangID = 0;
	MStringW m_strFileName; // 現在のファイル名。
	MStringListW m_recently_used_files; // 最近使ったファイルのリスト。
	LOGFONTW m_lfCandsLogFont = {};
	LOGFONTW m_lfCluesLogFont = {};
	MBoardDisplay m_display;
	MBoard& m_board = m_display.m_board;
	MBoard& m_solution = m_display.m_solution;
	MBoardPos& m_caret_pos = m_display.m_caret_pos;
	std::unordered_map<WCHAR, INT>& m_mapNumCro1 = m_display.m_mapNumCro1;
	BOOL& m_bSolved = m_display.m_bSolved;
	BOOL& m_bShowAnswer = m_display.m_bShowAnswer;
	BOOL& m_bShowCaret = m_display.m_bShowCaret;
	BOOL& m_bCheckingAnswer = m_display.m_bCheckingAnswer;
	BOOL& m_bHiragana = m_display.m_bHiragana;
	BOOL& m_bLowercase = m_display.m_bLowercase;
	BOOL& m_bShowDoubleFrameLetters = m_display.m_bShowDoubleFrameLetters;
	BOOL& m_bShowDoubleFrame = m_display.m_bShowDoubleFrame;
	BOOL& m_bNumCroMode = m_display.m_bNumCroMode;
	MStringMapW& m_word2clue = m_display.m_word2clue;
	XG_VIEW_MODE& m_nViewMode = m_display.m_nViewMode;
	std::vector<MPlaceInfo>& m_vHorzInfo = m_display.m_vHorzInfo;
	std::vector<MPlaceInfo>& m_vVertInfo = m_display.m_vVertInfo;
	std::vector<MBoardPos>& m_vMarks = m_display.m_vMarks;
	MJumpDialog m_hJumpDialog; // ジャンプダイアログ。
	BOOL m_bFileModified = FALSE; // ファイル変更フラグ。
	BOOL m_bNoGeneratedMsg = FALSE; // 「生成した」メッセージを表示しない？
	BOOL m_bNoCanceledMsg = FALSE; // 「キャンセルした」メッセージを表示しない？
	BOOL m_bChoosePAT = TRUE; // PAT.txtから黒マスパターンを選ぶかどうか？
	BOOL m_bShowAnswerOnGenerate = TRUE; // 生成時に答えを表示するかどうか？
	BOOL m_bShowAnswerOnOpen = TRUE; // 開いたときに答えを表示するかどうか？
	BOOL m_bAutoSave = FALSE; // 自動保存するか？
	BOOL m_bNoReadLooks = FALSE; // LOOKS情報を読み込まない？
	BOOL m_bNoWriteLooks = FALSE; // LOOKS情報を書きこまない？
    WCHAR m_szNumberingFileName1[MAX_PATH] = L""; // 連番ファイル名1。
    WCHAR m_szNumberingFileName2[MAX_PATH] = L""; // 連番ファイル名2。
	MStringListW m_dirs_save_to; // 保存先フォルダ群。
	WCHAR m_aszSoundFiles[3][MAX_PATH] = {}; // 音声ファイル。
	RECT m_rcCanvas = {}; // キャンバスの位置とサイズ。

	//////////////////
	// タスク バー進捗

	TaskbarProgress* m_pTaskbarProgress = nullptr;

	// アルゴリズム実行中は不確定(マーキー)状態にする。
	void BeginTaskbarProgress() {
		if (m_pTaskbarProgress)
			m_pTaskbarProgress->Set(-1);
	}
	// タスク バー進捗をエラー状態(赤)にする。
	void ErrorTaskbarProgress() {
		if (m_pTaskbarProgress)
			m_pTaskbarProgress->Error();
	}
	// タスク バー進捗を消す。
	void EndTaskbarProgress() {
		if (m_pTaskbarProgress)
			m_pTaskbarProgress->Clear();
	}

	MStringW& m_header = m_display.m_header; // ヘッダ。
	MStringW& m_notes = m_display.m_notes; // 備考欄。
	INT m_nCommandLock = 0; // コマンド ロック。

	// ウィンドウの位置とサイズ。
	INT m_nWindowX = CW_USEDEFAULT, m_nWindowY = CW_USEDEFAULT;
	INT m_nWindowCX = CW_USEDEFAULT, m_nWindowCY = CW_USEDEFAULT;
	BOOL m_bMaximized = FALSE;

	BOOL LoadFile0(HWND hwnd, LPCWSTR file);
	BOOL LoadFile1(HWND hwnd, LPCWSTR file);
	BOOL LoadFile(HWND hwnd, LPCWSTR file);
	BOOL SaveFile0(HWND hwnd, LPCWSTR file);
	BOOL SaveFile1(HWND hwnd, LPCWSTR file, BOOL bUpdateMRU = TRUE);
	BOOL SaveFile(HWND hwnd, LPCWSTR file, BOOL bUpdateMRU = TRUE);

	void CheckAnswer();
	void InitFileMenu(HMENU hMenu);
	void UpdateTitleBarText();
	BOOL DoConfirmSave(HWND hwnd);
	BOOL CreateMainWnd();
	void ResetSettings();
	BOOL LoadSettings();
	BOOL SaveSettings();
	void SetRules(INT nRules);
	void SetRules0(INT nRules);
	void ToggleRules(INT flags, INT mask);
	void ShowResult(INT ids, ULONGLONG time);
	void ShowResult(INT ids, ULONGLONG time, size_t nCount);
	void ShowError(HRESULT hr, const MStringW& strError);
	HRESULT FinalizeBoard();
	BOOL RuleCheck(HWND hwnd, BOOL bMessageOnSuccess, BOOL bLoose = FALSE);
	void JumpWord(const MStringW& word);
	void JumpNumber(INT nNumber, BOOL bVert);
	void MarkSaved();
	void OpenConfigPropSheet(HWND hwnd, INT nIndex);
	void DoPrint(HDC hdc, PRINTDLGW* ppd, BOOL bPrintAnswer);
	BOOL NumberingSave(HWND hwnd, BOOL bPattern, BOOL bNoError = FALSE);
	enum SoundType : INT {
		MY_SOUND_SUCCESS = 0,
		MY_SOUND_FAILED = 1,
		MY_SOUND_CANCELED = 2,
	};
	void MyPlaySound(SoundType type);

	void InvalidateImage() {
		m_hCanvasWnd.InvalidateImage();
	}
	void UpdateScrollInfo() {
		m_hCanvasWnd.UpdateScrollInfo();
	}
	void UpdateScrollInfo(INT x, INT y) {
		m_hCanvasWnd.UpdateScrollInfo(x, y);
	}
	void UpdateCaretPos() {
		m_hCanvasWnd.UpdateCaretPos();
	}

	//////////////////
	// 候補ウィンドウ

	MCandsWnd m_hCandsWnd;

	void DestroyCandsWnd();

	//////////////////
	// 辞書

	MDictList m_dict_list; // 辞書リスト。
	MStringW m_dict_name; // 辞書ファイルのパスファイル名。
	MDict m_dict; // 現在の辞書。

	BOOL ChooseDict();
	BOOL ChooseDict0();
	void InitDictMenu(HMENU hMenu);

	//////////////////
	// 二重マス単語

	MMarksDialog m_hMarksDlg;

	BOOL CreateMarksDlg();
	void DestroyMarksDlg();

	//////////////////
	// Input

	HACCEL m_hAccel = nullptr;
	MInputMode m_imode = xg_im_ANY;
	BOOL m_bCharFeed = TRUE;
	BOOL m_bVertInput = FALSE;
	BOOL m_bVerticalLayout = FALSE;
	UINT m_prev_vk = 0; // 直前の仮想キー。
	MStringA m_romaji; // 入力途中のローマ字(カナ入力用)。m_prev_vk が 0 のときは無効。
	WCHAR m_chAccent = 0; // アクセント文字処理用。
	MInputPalette m_hInputPalette;
	BOOL m_bShowInputPalette = FALSE;

	void __fastcall CharFeed(HWND hwnd);
	MStringW FeedRomaji(WCHAR ch); // ローマ字を1文字入力し、確定したカナ(大きな文字)を返す。
	void __fastcall SetCharFeed(HWND hwnd, INT nMode = -1);
	void __fastcall SetChar(HWND hwnd, WCHAR ch);
	void DestroyInputPalette();
	BOOL ReCreateInputPalette(HWND hwnd, MInputMode imode, BOOL bForceKana = FALSE);
	void __fastcall InputDirection(HWND hwnd, INT nDirection);
	void SetInputMode(HWND hwnd, MInputMode imode, BOOL bForce = FALSE);

	//////////////////
	// Canvas Window

	MCanvasWindow m_hCanvasWnd;

	BOOL CreateCanvasWnd(HWND hwnd);
	void DestroyCanvasWnd();

	//////////////////
	// Clues Window

	MCluesWnd m_hCluesWnd;

	BOOL CreateCluesWnd(HWND hwnd);
	void DestroyCluesWnd();
	void ClearAllClues();
	void ClearThisClue(INT nNumber, BOOL bVert);

	//////////////////
	// Toolbar

	HWND m_hToolBar = nullptr; // ツールバーのウィンドウ ハンドル。
	BOOL m_bShowToolBar = TRUE; // ツールバーを表示するか？
	HIMAGELIST m_hImageList = nullptr; // ツールバーのイメージリスト。
	HIMAGELIST m_hGrayedImageList = nullptr; // ツールバーの灰色イメージリスト。

	void UpdateToolBarUI();
	BOOL CreateToolBar(HWND hwnd);
	void DestroyToolBar();
	void ShowToolBar(BOOL bShow = TRUE);

	//////////////////
	// Status Bar

	HWND m_hStatusBar  = nullptr;
	BOOL m_bShowStatusBar = TRUE;

	BOOL CreateStatusBar(HWND hwnd);
	void DestroyStatusBar();
	void UpdateStatusBar();
	void ShowStatusBar(BOOL bShow = TRUE);

	//////////////////
	// Undo / Redo

	BOOL CanUndo() const;
	BOOL CanRedo() const;
	BOOL Undo();
	BOOL Redo();
	void ClearUndoBuffer();

	//////////////////
	// AI Helper

	MAIHelper m_ai_helper;
	LOGFONTW m_lfAIHelper = {};
	BOOL m_bServerAccessConfirmed = FALSE; // サーバーへのアクセスを確認した？

	MStringW GetRowOrColumnText(BOOL bRow, INT iRowOrCol);
	BOOL __fastcall SetHintText(INT number, BOOL bDown, const MStringW& text);
	MStringW __fastcall GetHintWord(INT number, BOOL bDown);
	BOOL SetBoardRowOrColumn(BOOL bRow, INT nNumber, const MStringW& text);
	BOOL GenerateByAI(INT nNumber, const MStringW& text);

	//////////////////
	// Message handlers

	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
	void OnDestroy(HWND hwnd);
	void OnMove(HWND hwnd, int x, int y);
	void OnSize(HWND hwnd, UINT state, int cx, int cy);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr);
	void OnClose(HWND hwnd);
	void OnChar(HWND hwnd, TCHAR ch, int cRepeat);
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags);
	void OnImeChar(HWND hwnd, WCHAR ch, LPARAM lParam);
	void OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
	void OnDropFiles(HWND hwnd, HDROP hdrop);
	void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo);

	//////////////////
	// WM_COMMAND handlers

	void OnNew(HWND hwnd);
	void OnGenerate(HWND hwnd);
	BOOL OnOpen(HWND hwnd);
	BOOL OnSave(HWND hwnd);
	BOOL OnSaveAs(HWND hwnd);
	void OnSaveProbAsImage(HWND hwnd);
	void OnSolve(HWND hwnd);
	void OnAbout(HWND hwnd);
	void OnZoomInOut(HWND hwnd, BOOL bOut = FALSE);
	void OnSwapLeftRight(HWND hwnd);
	void OnSwapTopBottom(HWND hwnd);
	void OnFill(HWND hwnd, BOOL bBlacks);
	void OnBlock(HWND hwnd);
	void OnSpace(HWND hwnd);
	void OnOpenPatterns(HWND hwnd);
	void OnHeaderAndNotes(HWND hwnd);
	void OnCopyMarkWord(HWND hwnd);
	void OnCopyHintsStyle0(HWND hwnd, INT nType);
	void OnCopyHintsStyle1(HWND hwnd, INT nType);
	void OnCopy(HWND hwnd);
	void OnPaste(HWND hwnd);
	void OnSaveAnsAsImage(HWND hwnd);
	void OnPrint(HWND hwnd, BOOL bAnswer = FALSE);
	void OnCut(HWND hwnd);
	void OnPaneNext(HWND hwnd, BOOL bPrev = FALSE);
	void OnUILangID(HWND hwnd);
	void OnKillMarks(HWND hwnd);
	void OnBack(HWND hwnd);
	void OnReturn(HWND hwnd);
	void OnMostLeft(HWND hwnd);
	void OnMostRight(HWND hwnd);
	void OnMostUpper(HWND hwnd);
	void OnMostLower(HWND hwnd);
	void OnLeft(HWND hwnd);
	void OnRight(HWND hwnd);
	void OnUp(HWND hwnd);
	void OnDown(HWND hwnd);
	void OnOnlineDict(HWND hwnd, BOOL bVertInput = FALSE);
	BOOL OnCommandExtra(HWND hwnd, INT id);
	void OnCopyWord(HWND hwnd, BOOL bVert = FALSE);
	void OnInsertRow(HWND hwnd, BOOL bBelow = FALSE);
	void OnInsertColumn(HWND hwnd, BOOL bRight = FALSE);
	void OnDeleteRow(HWND hwnd);
	void OnDeleteColumn(HWND hwnd);
	void OnCopyCharSet(HWND hwnd);
	void OnCopyWordList(HWND hwnd);
	void OnClearNonBlocks(HWND hwnd);
	void OnFlipVH(HWND hwnd);
	void OnOpenBSS(HWND hwnd);
	void OnRulePreset(HWND hwnd);
	void OnLineSymmetryCheck(HWND hwnd);
	void OnPointSymmetryCheck(HWND hwnd);
	void OnGenerateBlacks(HWND hwnd);
	void OnSolveNoAddBlack(HWND hwnd);
	void OnReflectBlocks(HWND hwnd);
	void OnGenerateFromWordList(HWND hwnd);
	BOOL OnUndoUpdate(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void OnShowHideHints(HWND hwnd);
	void OnEraseSettings(HWND hwnd);
	void OnJump(HWND hwnd);
	void OnJumpNumber(HWND hwnd);
	void OnClearThisClue(HWND hwnd);
	void OnClearAllClues(HWND hwnd);
	LRESULT OnHighlight1(HWND hwnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnHighlight2(HWND hwnd, WPARAM wParam, LPARAM lParam);
	void OnOpenCandsWnd(HWND hwnd, BOOL bVert);
	void OnRecent(HWND hwnd, size_t index);
	void OnTottleMark(HWND hwnd);
	void OnEraseSolutionAndUnlockEdit(HWND hwnd);
	void OnCheckAnswer(HWND hwnd);
	void OnExplainThisClue();
	void OnGenerateClue();
	void OnCustomPrompt(INT type);
	void OnSettings(HWND hwnd);
	void OnThemeTags(HWND hwnd);
	void OnDebugAction1(HWND hwnd);
	void OnDebugAction2(HWND hwnd);
	void OnOpenAIHelper(HWND hwnd);
};
