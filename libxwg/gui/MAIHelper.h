// MAIHelper.h --- XWordGiver AI Helper (pure C++ / WinHTTP, optional GUI)
// Author: katahiromz + Grok
// License: MIT
//
// AIHelper.cpp(GUI)と AIHelper2.cpp(WinHTTPクライアント)を1つにまとめたクラス。
//
//  * コア部分(プロバイダー設定・履歴・HTTP通信・プロンプト・コマンド解析)は
//    GUIに一切依存しない。
//  * GUI(AIヘルパー コンソール ウィンドウ)は任意。
//      - コンパイル時: MAIHELPER_NO_GUI を定義するとGUIコードがすべて除外される。
//      - 実行時    : SetUseGUI(FALSE) でGUIを使わずに動作する。
//  * GUIを使わない場合、AIの出力行は SetLineCallback() で登録した関数へ渡される
//    (ワーカースレッドから呼ばれる)。未設定ならconuni経由でコンソールへ出力する。
//    同期的に使いたいなら AskSync() を使う。
//  * 文字列はすべて MStringW (xwg.h) を使う。
#pragma once

#include "xwg.h"
#include "conuni.h"   // コンソール出力(PrintfW等)
#include <windows.h>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#ifndef MAIHELPER_NO_GUI
	#include "MResizable.h"
#endif

#define MAX_PROMPT_TEXT 5000 /* プロンプトの最大長+1 */

// プロンプトの種類。
enum PROMPT_TYPE {
	PROMPT_TYPE_INITIAL = 0,
	PROMPT_TYPE_GENERATE_CLUE,
	PROMPT_TYPE_GENERATE_CLUES_ALL,
	PROMPT_TYPE_EXPLAIN_CLUE,
	PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES,
	PROMPT_TYPE_JUDGE_SEASON,
	PROMPT_TYPE_JUDGE_THEME,
	PROMPT_TYPE_CUSTOM_PROMPT_1,
	PROMPT_TYPE_CUSTOM_PROMPT_2,
	PROMPT_TYPE_MAX,
};

// ワーカースレッドからウィンドウ(GUIまたは通知先)へ1行を渡すためのメッセージ。
// (LPARAMはnewしたPWSTR。WPARAMが1なら質問のエコー行で、AIの出力ではないので
// コマンド解析の対象にしない。受け取った側で OnLineMessage に両方を渡すこと)
#define WM_APP_AI_LINE  (WM_APP + 1)

class MMainWnd;

class MAIHelper {
public:
	// 1行の出力を受け取るコールバック。
	using LineCallback = std::function<void(const MStringW&)>;

	// プロバイダー接続設定(AIModels.datのPROVIDER_INFO)。
	struct ProviderInfo {
		 MStringW envKey;        // 空ならAPIキー不要(ローカルAI等)
		 MStringW host;
		 WORD     port = 443;
		 MStringW path;
		 bool isOpenAICompat = false;
		 bool isClaude = false;
		 bool isGemini = false;
		 bool useHttps = true;
	};

	struct Session; // 内部用(ワーカースレッドと共有する状態)

	// ---- 設定(旧グローバル変数) ----
	MStringW     m_provider = L"google";
	MStringW     m_model = L"gemini-3.5-flash-lite";
	MStringW     m_initialQuestion;
	MStringListW m_promptMapping;
	HINSTANCE    m_hInst = nullptr;     // GUIで使うリソースのインスタンス
	INT m_nFontPointSize = 11;          // GUIのフォントサイズ(Ctrl+ホイールで変更)
	INT m_nX = CW_USEDEFAULT;           // GUIの位置とサイズ
	INT m_nY = CW_USEDEFAULT;
	INT m_nCX = CW_USEDEFAULT;
	INT m_nCY = CW_USEDEFAULT;
	std::unordered_map<MStringW, MStringListW> m_knownModels;
	MStringListW m_knownProviders;

	MMainWnd& m_hMainWnd;
	BOOL& m_bSolved;
	MBoard& m_board;
	MBoard& m_solution;
	MBoardDisplay& m_display;

	MAIHelper(MMainWnd& hMainWnd);
	~MAIHelper();
	MAIHelper(const MAIHelper&) = delete;
	MAIHelper& operator=(const MAIHelper&) = delete;

	//////////////////////////////////////////////////////////////////////
	// コア(GUI非依存)

	// 通信セッションを開始する。AIModels.datを読み込み、履歴を空にする。
	// hwndNotify が指定されると、出力行は WM_APP_AI_LINE としてそのウィンドウへ
	// ポストされる(受け取り側は OnLineMessage を呼ぶ)。nullptr なら
	// SetLineCallback() のコールバックへ直接渡される。
	BOOL Start(HWND hwndNotify = nullptr);
	void Stop();
	bool IsRunning() const;

	// プロバイダー接続設定を AIModels.dat から読み込む。Start が自動的に呼ぶ。
	BOOL LoadProviders();

	void Reset();

	// 出力行を受け取るコールバックを登録する(Startの前に設定すること)。
	void SetLineCallback(LineCallback cb) { m_lineCallback = std::move(cb); }
	// 出力行を conuni (PrintfW) でコンソールへ出力する。
	// GUIなしで動かすとき、コールバック未設定ならこれが自動的に使われる。
	static void ConsoleLineCallback(const MStringW& line);
	void UseConsoleOutput() { SetLineCallback(ConsoleLineCallback); }

	// 質問を非同期に送る(加工なし)。
	// echo が空でなければ、この質問の順番が来た時点(先行する質問の回答が出力された後)に
	// その行を出力する。
	void Ask(const MStringW& text, const MStringW& echo = MStringW());
	// 質問を送って回答を待つ(履歴は共有される)。GUIなしのツール向け。
	bool AskSync(const MStringW& text, MStringW& answer, MStringW& err);
	// 現在のプロバイダーの会話履歴を消す。
	void ResetHistory();

	// 入力欄からの質問を処理する。空白除去、システムコマンドの判定と実行、
	// 事前情報(現在の盤面)の付加を行ってから非同期に送る。
	void AskQuestion(PCWSTR text);

	// 1行を出力する(GUIのリスト、およびコールバック)。(*...*)タグは除去される。
	void AddLine(const MStringW& line);
	// WM_APP_AI_LINE を受け取ったときの処理(lParamを引き取って解放する)。
	void OnLineMessage(LPARAM lParam, WPARAM wParam = 0);
	// 溜まった出力に含まれるシステムコマンドを今すぐ実行する。
	void FlushOutput();

	// AIModels.datの情報。
	BOOL GetAIModels(PCWSTR provider, MStringListW& models);
	BOOL GetAIProviders(MStringListW& providers);

	//////////////////////////////////////////////////////////////////////
	// プロンプト

	static MStringW GetPromptName(PROMPT_TYPE iPrompt);
	MStringW GetPromptString(PROMPT_TYPE iPrompt, const MStringW& clue_name) const;
	void SetPromptString(PROMPT_TYPE iPrompt, const MStringW& str);
	static void ResetPrompts(MStringListW& prompt_mapping);
	static MStringW GetClueName(INT nNumber, BOOL bDown);

	// 初期質問、盤面の状態報告。
	MStringW MakeInitialQuestion() const;
	MStringW GetAIStatus();
	MStringW GetAIPreText();

	// 「[[An:XXX]]」形式のシステムコマンドを解析して盤面・カギに反映する。
	void ParseAndApplyCommands(PCWSTR pszLine);

	//////////////////////////////////////////////////////////////////////
	// アプリ向けの操作(GUIが有効ならウィンドウを開き、無効なら直接送る)

	BOOL GenerateClue(INT nNumber, BOOL bDown);
	void RegenerateCluesAll();
	BOOL ExplainClue(INT nNumber, BOOL bDown);
	void CheckQualityOfAllClues();
	void JudgeSeason();
	void JudgeTheme();
	void CustomPrompt(INT type, const MStringW& clue_name);

	//////////////////////////////////////////////////////////////////////
	// GUI(任意)

	void SetUseGUI(BOOL bUse) { m_bUseGUI = bUse; }
	BOOL GetUseGUI() const { return m_bUseGUI; }
#ifndef MAIHELPER_NO_GUI
	BOOL OpenGUI(HWND hwndOwner);   // 開く(既に開いていれば前面に出す)
	void CloseGUI();
	HWND GetWindow() const { return m_hwnd; }
#else
	HWND GetWindow() const { return nullptr; }
#endif

private:
	BOOL EnsureReady();
	MStringW GetAIStatus_ja();
	MStringW GetAIStatus_en();
	void ScheduleFlush();
	static void CALLBACK FlushTimerProc(HWND, UINT, UINT_PTR, DWORD);

	BOOL                      m_bUseGUI = TRUE;
	std::shared_ptr<Session>  m_session;
	LineCallback              m_lineCallback;
	std::unordered_map<MStringW, ProviderInfo> m_providers;
	MStringW                  m_outputBuffer;
	UINT_PTR                  m_timerId = 0;

#ifndef MAIHELPER_NO_GUI
	HWND         m_hwnd = nullptr;
	bool         m_bGuiReady = false;   // OnInitDialog完了後にtrue
	HFONT        m_hFont = nullptr;
	HBITMAP      m_hbmFairy = nullptr;
	MResizable   m_resizable;
	MStringListW m_history;         // 送信履歴(↑↓キーで辿る)
	size_t       m_nHistoryIndex = 0;
	MStringW     m_historyPending;

	static LRESULT CALLBACK Edt1Proc(HWND, UINT, WPARAM, LPARAM);
	static LRESULT CALLBACK Lst1Proc(HWND, UINT, WPARAM, LPARAM);
	static INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);

	void CreateControls(HWND hwnd);
	HFONT CreateFont_(HWND hwnd, int nPointSize);
	void OnZoom(int nDelta);
	BOOL OnInitDialog(HWND hwnd);
	void OnMove(HWND hwnd);
	void OnSize(HWND hwnd);
	void OnOK(HWND hwnd);
	void OnDestroy();
	void AddToHistory(const MStringW& text);
	void HistoryGoBack(HWND hwndEdit);
	void HistoryGoForward(HWND hwndEdit);
	void AppendToList(const MStringW& line);
#endif
};
