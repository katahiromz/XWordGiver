// AIHelper.cpp --- AI Helper
// Author: katahiromz
// License: MIT
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

HWND g_hwndAIHelper = nullptr;
std::wstring g_privider = L"gemini";
std::wstring g_model = L"gemini-3.6-flash";
std::wstring g_python_exe;
std::wstring g_additional_instruction;

#ifdef __XWORDGIVER__
BOOL XgIsUserJapanese(VOID) noexcept;
#endif

// AIプロセスからの出力行を呼び出し側へ通知するためのコールバック
static AIHELPER_LINE_CALLBACK g_pfnLineCallback = nullptr;

// コールバックを登録する（呼び出し側が解析したい場合に使う）
void AIHelper_SetLineCallback(AIHELPER_LINE_CALLBACK callback)
{
	g_pfnLineCallback = callback;
}

// 子プロセスの出力の1行をUIスレッドへ渡すためのカスタムメッセージ
// (WPARAMは未使用、LPARAMはnewしたPWSTR。受け取った側でdelete[]すること)
#define WM_APP_AI_LINE   (WM_APP + 1)

// pszLineの表示幅（ピクセル）を、lst1で使われているフォントで計測する
static int MeasureLineWidth(HWND hLst1, LPCWSTR pszLine)
{
	int cxWidth = 0;

	HDC hdc = GetDC(hLst1);
	if (!hdc)
		return 0;

	HFONT hFont = (HFONT)SendMessageW(hLst1, WM_GETFONT, 0, 0);
	HFONT hFontOld = hFont ? (HFONT)SelectObject(hdc, hFont) : nullptr;

	SIZE size;
	if (GetTextExtentPoint32W(hdc, pszLine, (int)wcslen(pszLine), &size))
		cxWidth = size.cx;

	if (hFontOld)
		SelectObject(hdc, hFontOld);
	ReleaseDC(hLst1, hdc);

	return cxWidth;
}

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
// 横スクロールできるよう、必要に応じて水平スクロール範囲も広げる。
static void AddLineToList(HWND hwnd, LPCWSTR pszLine)
{
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	if (!hLst1)
		return;

	// (*...*) タグを除去してから表示する
	std::wstring filtered = StripAiPreTextTag(pszLine);
	if (filtered.empty())
		return; // タグのみの行（プロンプト等）は表示しない

	LPCWSTR pszDisplay = filtered.c_str();

	INT iIndex = (INT)SendMessageW(hLst1, LB_ADDSTRING, 0, (LPARAM)pszDisplay);
	SendMessageW(hLst1, LB_SETTOPINDEX, (WPARAM)iIndex, 0);

	int cxLine = MeasureLineWidth(hLst1, pszDisplay);
	int cxExtent = (int)SendMessageW(hLst1, LB_GETHORIZONTALEXTENT, 0, 0);
	if (cxLine + 10 > cxExtent)
		SendMessageW(hLst1, LB_SETHORIZONTALEXTENT, (WPARAM)(cxLine + 10), 0);
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

static void DoSelectAll(HWND hwndList)
{
	SendMessageW(hwndList, LB_SETSEL, TRUE, -1);
}

static void DoCopyList(HWND hwndList)
{
	INT nSelCount = (INT)SendMessageW(hwndList, LB_GETSELCOUNT, 0, 0);
	if (nSelCount <= 0)
		return;

	std::vector<int> selItems(nSelCount);
	SendMessageW(hwndList, LB_GETSELITEMS, nSelCount, (LPARAM)selItems.data());

	std::wstring text;
	for (INT i = 0; i < nSelCount; ++i)
	{
		INT idx = selItems[i];
		INT len = (INT)SendMessageW(hwndList, LB_GETTEXTLEN, idx, 0);
		if (len > 0)
		{
			std::wstring line;
			line.resize(len + 1);
			SendMessageW(hwndList, LB_GETTEXT, idx, (LPARAM)&line[0]);
			line.resize(len);
			text += line;
			text += L"\r\n";
		}
	}

	if (text.empty())
		return;

	if (OpenClipboard(hwndList))
	{
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(WCHAR));
		if (hMem)
		{
			PWSTR pMem = (PWSTR)GlobalLock(hMem);
			StringCchCopyW(pMem, text.size() + 1, text.c_str());
			GlobalUnlock(hMem);
			SetClipboardData(CF_UNICODETEXT, hMem);
			SendMessageW(hwndList, LB_SETSEL, FALSE, -1);
		}
		CloseClipboard();
	}
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

				std::wstring wline = Utf8ToWide(line.c_str(), (int)line.size());
				PostLineToUI(hwnd, wline);

				buffer.erase(0, pos + 1);
			}
		}
	}

	// プロセスが終了した後に残った断片も出力する
	if (!buffer.empty())
	{
		std::wstring wline = Utf8ToWide(buffer.c_str(), (int)buffer.size());
		PostLineToUI(hwnd, wline);
	}

	if (!g_bReaderStop)
		PostLineToUI(hwnd, L"[AIプロセスが終了しました]");

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
	str += g_privider;
	str += L" --model ";
	str += g_model;

	// 実行するコマンドをlst1に出力する
	AddLineToList(hwnd, (L"> " + str).c_str());

	// 環境変数をセットする。
	SetEnvironmentVariableW(L"PYTHONIOENCODING", L"utf-8");

	// 子プロセスのウィンドウを作成しない。
	g_maker.SetCreationFlags(CREATE_NO_WINDOW);

	if (!g_maker.PrepareForRedirect(&g_hInputWrite, &g_hOutputRead) ||
		!g_maker.CreateProcessDx(nullptr, str.c_str()))
	{
		AddLineToList(hwnd, L"[エラー] プロセスの起動に失敗しました。");
		return FALSE;
	}

	g_bReaderStop = FALSE;
	g_hReaderThread = CreateThread(nullptr, 0, ReaderThreadProc, hwnd, 0, nullptr);
	return TRUE;
}

// 実行中のAIHelper_ja.pyプロセスを終了し、後片付けをする
static void StopAIProcess()
{
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

	std::wstring line;
#ifdef __XWORDGIVER__
	std::wstring XG_GetAIPreText(void);
	std::wstring pre_text = XG_GetAIPreText();
	if (pre_text.size())
	{
		line += L"(*";
		line += pre_text;
		line += L"*) ";
	}
#endif
	line += text;
	if (g_additional_instruction.size())
	{
		line += L"\n---\n";
		line += g_additional_instruction;
	}
	line += L"\n"; // 重要！

	std::string utf8 = WideToUtf8(line.c_str());

	DWORD cbWritten;
	if (!g_hInputWrite.WriteFile(utf8.data(), (DWORD)utf8.size(), &cbWritten))
	{
		AddLineToList(hwnd, L"[エラー] AIプロセスへの送信に失敗しました。");
	}
}

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	g_hwndAIHelper = hwnd;

	// Subclassing lst1 (for Ctrl+C copy)
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	g_fnOldLst1WndProc = (WNDPROC)SetWindowLongPtrW(hLst1, GWLP_WNDPROC, (LONG_PTR)Lst1WndProc);

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

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		if (OnOK(hwnd))
		{
			StopAIProcess();
#ifdef AIHELPER_STANDALONE
			EndDialog(hwnd, id);
#else
			DestroyWindow(hwnd);
#endif
		}
		break;
	case IDCANCEL:
		StopAIProcess();
#ifdef AIHELPER_STANDALONE
		EndDialog(hwnd, id);
#else
		DestroyWindow(hwnd);
#endif
		break;
	}
}

static void OnDestroy(HWND hwnd)
{
	StopAIProcess();
	g_hwndAIHelper = nullptr;
#ifdef AIHELPER_STANDALONE
	PostQuitMessage(0);
#endif
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

	case WM_APP_AI_LINE:
		{
			// ReaderThreadProcがnewしたバッファを引き取って表示し、解放する
			PWSTR psz = (PWSTR)lParam;
			AddLineToList(hwnd, psz);
			// 登録されていれば、生の行をそのままコールバックへ渡す
			// (表示用のタグ除去はAddLineToList内でのみ行われ、ここには影響しない)
			if (g_pfnLineCallback)
				g_pfnLineCallback(psz);
			delete[] psz;
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

#ifdef AIHELPER_STANDALONE
INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
	g_hAIHelperInst = hInstance;
	InitCommonControls();
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_AIHELPERCONSOLE), nullptr, DialogProc);
	return 0;
}
#endif
