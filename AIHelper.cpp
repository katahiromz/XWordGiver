// AIHelper.cpp --- AI Helper
// Author: katahiromz
// License: MIT
#include "DetectLeaks.h"
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include "MFile.hpp"
#include "MProcessMaker.hpp"
#include "MResizable.hpp"
#include "AIHelper.h"
#include "resource.h"

HINSTANCE g_hAIHelperInst = nullptr;

// ダイアログのリサイズ処理を担当する
static MResizable g_resizable;

// 起動しっぱなしにするAIHelper_ja.pyプロセスとそのパイプ
static MProcessMaker g_maker;
static MFile         g_hInputWrite;
static MFile         g_hOutputRead;
static HANDLE        g_hReaderThread = nullptr;
static volatile BOOL g_bReaderStop = FALSE;
// AIHelper(_ja).pyが起動完了して標準入力の受付準備ができたときにセットされる
static HANDLE        g_hReadyEvent = nullptr;

HWND g_hwndAIHelper = nullptr;
std::wstring g_provider = L"gemini";
std::wstring g_model = L"gemini-3.6-flash";
std::wstring g_python_exe;
std::wstring g_additional_instruction;
std::wstring g_buffer;

#ifdef __XWORDGIVER__
BOOL XgIsUserJapanese(VOID) noexcept;
#endif

#define IDT_AI_OUTPUT_FLUSH 999

// AIプロセスからの出力を呼び出し側へ通知するためのコールバック
static AIHELPER_OUTPUT_CALLBACK g_pfnOutputCallback = nullptr;

// コールバックを登録する（呼び出し側が解析したい場合に使う）
void AIHelper_SetOutputCallback(AIHELPER_OUTPUT_CALLBACK callback)
{
	g_pfnOutputCallback = callback;
}

// 完全に起動されるまで待つ。
// AIHelper(_ja).pyはコンソールサブシステムのPythonプロセスであり、GUIの
// メッセージキューを持たないため、WaitForInputIdleでは起動完了を検知できない。
// 代わりに、子プロセスが標準出力へ"[READY]"を吐いた時点でセットされる
// イベントを待つ（ReaderThreadProc参照）。
//
// 注意: ReaderThreadProcはバナー等の各行をPostMessageWでこのスレッドの
// メッセージキューに積むだけなので、単純にWaitForSingleObjectで待つと、
// その間メッセージポンプが止まり、バナー行がまだ表示されていないうちに
// （＝WM_APP_AI_LINEが処理されないうちに）呼び出し元がAskAIQuestionで
// 質問エコーを直接AddLineToListしてしまい、表示順が
// 「質問エコー→バナー」と入れ替わってしまう。
// それを防ぐため、待機中もメッセージを汲み出して処理する。
void AIHelper_WaitForReady(void)
{
	if (!g_hReadyEvent)
		return;

	const DWORD dwStart = GetTickCount();
	const DWORD dwTimeout = 5 * 1000;

	for (;;)
	{
		DWORD dwElapsed = GetTickCount() - dwStart;
		if (dwElapsed >= dwTimeout)
			break;

		DWORD dwWait = MsgWaitForMultipleObjects(
			1, &g_hReadyEvent, FALSE, dwTimeout - dwElapsed, QS_ALLINPUT);

		if (dwWait == WAIT_OBJECT_0)
			break; // [READY]を受信した

		if (dwWait != WAIT_OBJECT_0 + 1)
			break; // タイムアウトまたはエラー

		// キューにあるメッセージ（WM_APP_AI_LINEなど）を処理して、
		// バナー行などが先に表示されるようにする
		MSG msg;
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	// イベントがシグナル状態になった後にもう届いているかもしれない
	// メッセージ（バナーの残りなど）も、戻る前に処理しておく
	MSG msg;
	while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

// 子プロセスの出力の1行をUIスレッドへ渡すためのカスタムメッセージ
// (WPARAMは未使用、LPARAMはnewしたPWSTR。受け取った側でdelete[]すること)
#define WM_APP_AI_LINE   (WM_APP + 1)

// 文字列中に含まれる "(*...*)" 形式のタグ（XG_GetAIPreTextによる前置情報など）を
// すべて取り除いた文字列を返す。表示前のフィルタリング用。
static std::wstring StripAiPreTextTag(LPCWSTR pszLine)
{
	std::wstring result;
	const wchar_t *pch = pszLine;

	while (*pch)
	{
		if (pch[0] == L'(' && pch[1] == L'*')
		{
			const wchar_t *pEnd = wcsstr(pch + 2, L"*)");
			if (pEnd)
			{
				pch = pEnd + 2;
				// タグ直後の空白も1つ読み飛ばす（"(*...*) " のように付与されるため）
				if (*pch == L' ')
					++pch;
				continue;
			}
		}
		result += *pch;
		++pch;
	}

	return result;
}

// lst1に1行追加し、末尾までスクロールする。
static void AddLineToList(HWND hwnd, LPCWSTR pszLine)
{
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	if (!hLst1)
		return;

	// (*...*) タグを除去してから表示する
	std::wstring filtered = StripAiPreTextTag(pszLine);
	if (filtered.empty())
		return; // タグのみの行（プロンプト等）は表示しない

	// 既存のテキストの末尾にキャレットを置き、必要なら改行を付けてから追記する
	int cchExisting = GetWindowTextLengthW(hLst1);
	SendMessageW(hLst1, EM_SETSEL, (WPARAM)cchExisting, (LPARAM)cchExisting);

	std::wstring insert;
	if (cchExisting > 0)
		insert += L"\r\n";
	insert += filtered;

	SendMessageW(hLst1, EM_REPLACESEL, FALSE, (LPARAM)insert.c_str());

	// 末尾までスクロールする
	SendMessageW(hLst1, EM_SCROLLCARET, 0, 0);
}

// UTF-8バイト列をUTF-16文字列に変換する
static std::wstring Utf8ToWide(const char* psz, int cch)
{
	if (cch <= 0)
		return std::wstring();

	int cchWide = MultiByteToWideChar(CP_UTF8, 0, psz, cch, nullptr, 0);
	if (cchWide <= 0)
		return std::wstring();

	std::wstring wstr(cchWide, 0);
	MultiByteToWideChar(CP_UTF8, 0, psz, cch, &wstr[0], cchWide);
	return wstr;
}

static void DoSelectAll(HWND hwndEdit)
{
	SendMessageW(hwndEdit, EM_SETSEL, 0, -1);
}

static void DoCopyList(HWND hwndEdit)
{
	DWORD dwStart = 0, dwEnd = 0;
	SendMessageW(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
	if (dwStart == dwEnd)
		return; // 選択範囲がなければ何もしない

	// エディットコントロール標準のコピー処理に任せる
	// （改行の扱いなどもOSがよしなにやってくれる）
	SendMessageW(hwndEdit, WM_COPY, 0, 0);
}

static WNDPROC g_fnOldLst1WndProc = nullptr;

// lst1 用サブクラスウィンドウプロシージャ（Ctrl+C で選択行をコピー）
static LRESULT CALLBACK Lst1WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (GetFocus() != hwnd || GetKeyState(VK_CONTROL) >= 0)
			break;
		if (wParam == 'A') // Ctrl+A
		{
			DoSelectAll(hwnd);
			return 0;
		}
		if (wParam == 'C') // Ctrl+C
		{
			DoCopyList(hwnd);
			return 0;
		}
	}
	return CallWindowProcW(g_fnOldLst1WndProc, hwnd, uMsg, wParam, lParam);
}

// UTF-16文字列をUTF-8バイト列に変換する
static std::string WideToUtf8(LPCWSTR psz)
{
	if (!psz || !*psz)
		return std::string();

	int cch = (int)wcslen(psz);
	int cbUtf8 = WideCharToMultiByte(CP_UTF8, 0, psz, cch, nullptr, 0, nullptr, nullptr);
	if (cbUtf8 <= 0)
		return std::string();

	std::string str(cbUtf8, 0);
	WideCharToMultiByte(CP_UTF8, 0, psz, cch, &str[0], cbUtf8, nullptr, nullptr);
	return str;
}

// 子プロセスの出力を1行分、UIスレッドへ渡す（自スレッドから安全に呼べる）
static void PostLineToUI(HWND hwnd, const std::wstring& line)
{
	PWSTR psz = new WCHAR[line.size() + 1];
	StringCchCopyW(psz, line.size() + 1, line.c_str());
	PostMessageW(hwnd, WM_APP_AI_LINE, 0, (LPARAM)psz);
}

// g_hOutputReadを読み続け、行がまとまるたびにUIスレッドへ渡すバックグラウンドスレッド。
// プロンプト文字列などの改行なしの断片も、しばらく新しいデータが来なければ
// 1行として吐き出す（＝パイプが空になったタイミングでflushする）。
// バッファから取り出した1行が起動完了シグナルであれば処理して true を返す。
// このシグナルはAIHelper_WaitForReady専用の内部プロトコルなので、
// UI（lst1）には表示しない。
static bool HandlePossibleReadySignal(const std::string& line)
{
	if (line.find("[READY]") == line.npos)
		return false;

	if (g_hReadyEvent)
		SetEvent(g_hReadyEvent);
	return true;
}

static DWORD WINAPI ReaderThreadProc(LPVOID lpParam)
{
	HWND hwnd = (HWND)lpParam;
	std::string buffer;
	BYTE szBuf[1024];
	DWORD cbAvail, cbRead;

	while (!g_bReaderStop)
	{
		if (!g_hOutputRead.PeekNamedPipe(nullptr, 0, nullptr, &cbAvail))
			break; // パイプが閉じられた（プロセス終了）

		if (cbAvail == 0)
		{
			// 改行なしで溜まっている断片（プロンプト等）があれば、
			// ここでいったん1行として出力しておく
			if (!buffer.empty())
			{
				HandlePossibleReadySignal(buffer);
				std::wstring wline = Utf8ToWide(buffer.c_str(), (int)buffer.size());
				PostLineToUI(hwnd, wline);
				buffer.clear();
			}

			if (!g_maker.IsRunning())
				break;

			Sleep(10);
			continue;
		}

		if (cbAvail > sizeof(szBuf))
			cbAvail = sizeof(szBuf);

		if (g_hOutputRead.ReadFile(szBuf, cbAvail, &cbRead) && cbRead > 0)
		{
			buffer.append(reinterpret_cast<char*>(szBuf), cbRead);

			size_t pos;
			while ((pos = buffer.find('\n')) != std::string::npos)
			{
				std::string line = buffer.substr(0, pos);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();

				HandlePossibleReadySignal(line);
				std::wstring wline = Utf8ToWide(line.c_str(), (int)line.size());
				PostLineToUI(hwnd, wline);

				buffer.erase(0, pos + 1);
			}
		}
	}

	// プロセスが終了した後に残った断片も出力する
	if (!buffer.empty())
	{
		HandlePossibleReadySignal(buffer);
		std::wstring wline = Utf8ToWide(buffer.c_str(), (int)buffer.size());
		PostLineToUI(hwnd, wline);
	}

	if (!g_bReaderStop)
		PostLineToUI(hwnd, L"[AIプロセスが終了しました]");

	// [READY]を送る前にプロセスが終了した場合、AIHelper_WaitForReadyが
	// タイムアウトまで無駄に待ち続けないよう、念のためここでもイベントをセットする
	if (g_hReadyEvent)
		SetEvent(g_hReadyEvent);

	return 0;
}

// AIHelper_ja.py を対話モードで一度だけ起動し、そのままプロセスを保持し続ける。
// 以後の質問は同じプロセスの標準入力へ書き込むことで送る。
static BOOL StartAIProcess(HWND hwnd)
{
	TCHAR path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, _countof(path));
	PathRemoveFileSpecW(path);
#ifdef __XWORDGIVER__
	if (XgIsUserJapanese())
#else
	if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE)
#endif
		PathAppendW(path, L"AIHelper_ja.py");
	else
		PathAppendW(path, L"AIHelper.py");

	std::wstring str;
	str += L"\"";
	if (g_python_exe.size())
		str += g_python_exe;
	else
		str += L"python";
	str += L"\" \"";
	str += path;
	str += L"\" --provider=";
	str += g_provider;
	str += L" --model ";
	str += g_model;

	// 実行するコマンドをlst1に出力する
	AddLineToList(hwnd, (L"> " + str).c_str());

	// 環境変数をセットする。
	SetEnvironmentVariableW(L"PYTHONIOENCODING", L"utf-8");

	// 子プロセスのウィンドウを作成しない。
	g_maker.SetCreationFlags(CREATE_NO_WINDOW);

	// 起動完了シグナル（[READY]）待ち用のイベントを用意する
	// （手動リセット、初期状態は非シグナル）
	if (!g_hReadyEvent)
		g_hReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	else
		ResetEvent(g_hReadyEvent);

	if (!g_maker.PrepareForRedirect(&g_hInputWrite, &g_hOutputRead) ||
		!g_maker.CreateProcessDx(nullptr, str.c_str()))
	{
		AddLineToList(hwnd, L"[エラー] プロセスの起動に失敗しました。");
		if (g_hReadyEvent)
			SetEvent(g_hReadyEvent); // 起動失敗時に無駄に待たされないように
		return FALSE;
	}

	g_bReaderStop = FALSE;
	g_hReaderThread = CreateThread(nullptr, 0, ReaderThreadProc, hwnd, 0, nullptr);
	return TRUE;
}

// 実行中のAIHelper_ja.pyプロセスを終了し、後片付けをする
static void StopAIProcess(HWND hwnd)
{
	KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);

	g_bReaderStop = TRUE;

	if (g_maker.IsRunning())
		g_maker.TerminateProcess(0);

	if (g_hReaderThread)
	{
		WaitForSingleObject(g_hReaderThread, 2000);
		CloseHandle(g_hReaderThread);
		g_hReaderThread = nullptr;
	}

	g_hInputWrite.CloseHandle();
	g_hOutputRead.CloseHandle();
	g_maker.CloseAll();

	if (g_hReadyEvent)
	{
		CloseHandle(g_hReadyEvent);
		g_hReadyEvent = nullptr;
	}
}

// パイプは「1行=1メッセージ」のプロトコルなので、text/pre_text/追加指示に
// 万一改行が含まれていても子プロセスのinput()が複数質問と誤認しないよう、
// 改行を空白に潰してから連結する（呼び出し元の実装に依存しない防御策）。
static std::wstring SanitizeForPipeLine(const std::wstring& s)
{
	std::wstring result;
	result.reserve(s.size());
	for (wchar_t ch : s)
		result += (ch == L'\r' || ch == L'\n') ? L' ' : ch;
	return result;
}

// 起動済みのプロセスの標準入力へ質問を書き込む（プロセスは終了させない）
void AskAIQuestion(HWND hwnd, PCWSTR text)
{
	if (!g_maker.IsRunning())
	{
		AddLineToList(hwnd, L"[エラー] AIプロセスが起動していません。");
		return;
	}

	// 入力した質問をlst1にエコー表示する
	AddLineToList(hwnd, (L"> " + std::wstring(text)).c_str());
	if (XgIsUserJapanese())
		AddLineToList(hwnd, L"...しばらくお待ちください...");
	else
		AddLineToList(hwnd, L"...Please wait a moment...");

	std::wstring line;
#ifdef __XWORDGIVER__
	std::wstring XG_GetAIPreText(void);
	std::wstring pre_text = XG_GetAIPreText();
	if (pre_text.size())
	{
		line += L"(* ";
		line += SanitizeForPipeLine(pre_text);
		line += L" *) ";
	}
#endif
	line += text;
	if (g_additional_instruction.size())
	{
		line += L"(* ";
		line += SanitizeForPipeLine(g_additional_instruction);
		line += L" *)";
	}
	line += L"\n"; // 重要！ この行に本物の改行はこの1文字だけ。

	std::string utf8 = WideToUtf8(line.c_str());

	DWORD cbWritten;
	if (!g_hInputWrite.WriteFile(utf8.data(), (DWORD)utf8.size(), &cbWritten))
	{
		AddLineToList(hwnd, L"[エラー] AIプロセスへの送信に失敗しました。");
	}
}

// WM_INITDIALOG
static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	g_hwndAIHelper = hwnd;

	// Subclassing lst1 (for Ctrl+A / Ctrl+C)
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	g_fnOldLst1WndProc = (WNDPROC)SetWindowLongPtrW(hLst1, GWLP_WNDPROC, (LONG_PTR)Lst1WndProc);

	// デフォルトの文字数上限（約64KB）を撤廃し、ログが長く伸びても追記できるようにする
	SendMessageW(hLst1, EM_SETLIMITTEXT, 0, 0);

	SetFocus(GetDlgItem(hwnd, edt1));

	// ダイアログをリサイズ可能にする
	g_resizable.OnParentCreate(hwnd, TRUE, TRUE);
	// lst1: ウィンドウのリサイズに合わせて幅・高さともに伸縮させる
	g_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
	// edt1: 下端に張り付いたまま、幅だけ伸縮させる
	g_resizable.SetLayoutAnchor(edt1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
	// IDOK（Enterボタン）: サイズは固定のまま右下に追従させる
	g_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);

	// ダイアログの起動と同時にAIHelper_ja.pyを一度だけ起動し、
	// ダイアログを閉じるまでプロセスを使い回す
	StartAIProcess(hwnd);

	return FALSE;
}

// WM_SIZE
static VOID OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	g_resizable.OnSize();
}

static BOOL OnOK(HWND hwnd)
{
	WCHAR text[512];
	GetDlgItemTextW(hwnd, edt1, text, _countof(text));
	if (text[0])
	{
		AskAIQuestion(hwnd, text);
		SetDlgItemTextW(hwnd, edt1, L"");
	}
	return FALSE;
}

// WM_COMMAND
static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		if (OnOK(hwnd))
		{
			StopAIProcess(hwnd);
			DestroyWindow(hwnd);
		}
		break;
	case IDCANCEL:
		StopAIProcess(hwnd);
		DestroyWindow(hwnd);
		break;
	}
}

// WM_DESTROY
static void OnDestroy(HWND hwnd)
{
	StopAIProcess(hwnd);
	g_hwndAIHelper = nullptr;
	g_buffer.clear();
}

// WM_TIMER
static void OnTimer(HWND hwnd, UINT id)
{
	if (id != IDT_AI_OUTPUT_FLUSH)
		return;

	KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);

	std::wstring buffer = std::move(g_buffer);
	g_buffer.clear();

	// 登録されていれば、生の出力をそのままコールバックへ渡す
	// (表示用のタグ除去はAddLineToList内でのみ行われ、ここには影響しない)
	if (g_pfnOutputCallback)
		g_pfnOutputCallback(buffer.c_str());
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		HANDLE_MSG(hwnd, WM_SIZE, OnSize);
		HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		HANDLE_MSG(hwnd, WM_TIMER, OnTimer);

	case WM_APP_AI_LINE:
		if (lParam)
		{
			KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);

			// ReaderThreadProcがnewしたバッファを引き取って表示し、解放する
			PWSTR psz = (PWSTR)lParam;
			AddLineToList(hwnd, psz);
			g_buffer += psz;
			delete[] psz;

			// デバウンス（debounce）パターン
			SetTimer(hwnd, IDT_AI_OUTPUT_FLUSH, 300, nullptr);
		}
		return TRUE;
	}
	return 0;
}

BOOL OpenAIHelper(HWND hwndOwner, BOOL bOpen)
{
	if (bOpen)
	{
		if (g_hwndAIHelper)
		{
			SetForegroundWindow(g_hwndAIHelper);
			return TRUE;
		}

		HWND hwnd = CreateDialogW(g_hAIHelperInst, MAKEINTRESOURCE(IDD_AIHELPERCONSOLE), hwndOwner, DialogProc);
		if (!hwnd)
			return FALSE;
		ShowWindow(hwnd, SW_SHOWNOACTIVATE);
		UpdateWindow(hwnd);
		return TRUE;
	}
	else
	{
		if (g_hwndAIHelper)
		{
			DestroyWindow(g_hwndAIHelper);
			return TRUE;
		}

		return FALSE;
	}
}
