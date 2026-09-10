// AIHelper.cpp --- XWordGiver AI Helper
// Author: katahiromz
// License: MIT
#include "DetectLeaks.h"
#include "XWordGiver.hpp"
#include "XG_UndoBuffer.hpp"
#include <commctrl.h>
#include <shlwapi.h>
#include <imm.h>
#include <string>
#include <vector>
#include <memory>
#include <strsafe.h>
#ifdef USE_PYTHON
	#include "MFile.hpp"
	#include "MProcessMaker.hpp"
#else
	#include "AIHelper2.h"
#endif
#include "MResizable.hpp"
#include "AIHelper.h"
#include "resource.h"

// インスタンス ハンドル。
HINSTANCE xg_hAIHelperInst = nullptr;

// ダイアログのリサイズ処理を担当する
static MResizable xg_resizable;

// 起動しっぱなしにするAIHelper_ja.pyプロセスとそのパイプ
#ifdef USE_PYTHON
static MProcessMaker xg_process_maker;
static MFile         xg_hInputWrite;
static MFile         xg_hOutputRead;
static HANDLE        xg_hReaderThread = nullptr;
static volatile BOOL xg_bReaderStop = FALSE;
#endif

// AIHelper(_ja).pyが起動完了して標準入力の受付準備ができたときにセットされる
static HANDLE        xg_hReadyEvent = nullptr;

HWND xg_hwndAIHelper = nullptr;
HBITMAP g_hbmFairy = nullptr;
std::wstring xg_ai_provider = L"gemini";
std::wstring xg_ai_model = L"gemini-3.6-flash";
std::wstring xg_additional_instruction;
std::wstring xg_output_buffer;
std::wstring xg_initial_question;

// AIヘルパーの位置とサイズ。
INT xg_nHelperX = CW_USEDEFAULT;
INT xg_nHelperY = CW_USEDEFAULT;
INT xg_nHelperCX = CW_USEDEFAULT;
INT xg_nHelperCY = CW_USEDEFAULT;

BOOL Helper_Open(HWND hwndOwner);
XGStringW XgGetRowOrColumnText(BOOL bRow, INT iRowOrCol);
void Helper_WaitForReady(void);
void Helper_AskQuestion(HWND hwnd, PCWSTR text);
void Helper_AddLine(HWND hwnd, PCWSTR pszLine);

// 行または列を書き換える。
BOOL XgSetBoardRowOrColumn(BOOL bRow, INT nNumber, const XGStringW& text);
XGStringW __fastcall XgGetHintWord(INT number, BOOL bDown);

//////////////////////////////////////////////////////////////////////////////
// DIALOGリソースを使わずに、ダイアログと同じ操作性（Tab移動、Enterで既定ボタン、
// Escで閉じる、等）を持つウィンドウを実現するための仕組み。
//
// 手口：ウィンドウクラスの既定プロシージャにDefDlgProcWを指定し、DLGWINDOWEXTRA分の
// 拡張バイトを確保しておく。ウィンドウ作成後、DWLP_DLGPROCへ自前のDialogProcを
// セットしてからWM_INITDIALOGを自分で送ってやると、以後はCreateDialog系APIで
// 作った場合と全く同じ挙動になる（IsDialogMessageWによるキーボード操作もそのまま効く）。

static PCWSTR const AIHELPERCONSOLE_CLASSNAME = L"AIHelperConsoleClass";

// 現在のズーム後のフォントサイズ（Ctrl+ホイールで変更する）
INT xg_nHelperFontPointSize = 11;
static const INT g_nFontPointSizeMin = 6;
static const INT g_nFontPointSizeMax = 36;

// lst1/edt1/IDOKで共有する、現在使用中のフォント
static HFONT g_hFont = nullptr;

static void Helper_OnZoom(HWND hwndDlg, int nDelta);

// UIフォント。
extern WCHAR xg_szUIFont[LF_FACESIZE];

// 現在の言語・ズーム設定に応じたフォントを作る（呼び出し側でDeleteObjectすること）
static HFONT CreateAIHelperFont(HWND hwnd, int nPointSize)
{
	LOGFONTW lf = { 0 };

	HDC hdc = GetDC(hwnd);
	lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(hwnd, hdc);

	lf.lfWeight = FW_NORMAL;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	StringCchCopyW(lf.lfFaceName, _countof(lf.lfFaceName), xg_szUIFont);

	return CreateFontIndirectW(&lf);
}

// 最初の質問を作成する。
std::wstring XgMakeInitialQuestion_ja(void)
{
	std::wstring str = L"(* "
		L"あなたは「クロスワードの妖精」です。クロスワードを作成または編集するユーザーを助けるのがあなたの役目です。"
		L"あなたは「クロスワード ギバー」というアプリに宿る妖精です。"
		L"あなたの母語は日本語です。12歳、男性、名前は「クロワース」。"

		L"「An」はヨコのカギnの略です(nは任意の自然数)。「Dm」はタテのカギnの略です(mは任意の自然数)。"
		L"カギがあるとき、システムはあなたのコマンド出力に応じてクロスワードのカギを編集できます。"
		L"システムはあなたのコマンド出力「[[An:XXX]]」でAnのカギ文章を「XXX」に書き換えます(nは任意の自然数、XXXは任意のテキスト)。"
		L"システムはあなたのコマンド出力「[[Dm:YYY]]」でDmのカギ文章を「YYY」に書き換えます(mは任意の自然数、YYYは任意のテキスト)。"

		L"クロスワードの盤面は長方形または正方形の格子状に並べられた全角文字の並びです。"
		L"ヨコ向きの文字の並びは「行」と呼びます。"
		L"タテ向きの文字の並びは「列」と呼びます。"
		L"黒マス（ブロック）は全角の「■」で表します。"
		L"白マス（空白マス）は全角空白で表します。"
		L"「Rp」はp番目の行の略です(pは任意の自然数)。"
		L"「Cq」はq番目の列の略です(qは任意の自然数)。"

		L"カギがないとき、システムはあなたのコマンド出力に応じてクロスワードの盤面を編集できます。"
		L"システムはあなたのコマンド出力「[[Rp:XXX]]」でRpを「XXX」に書き換えます(pは任意の自然数、XXXは新しい行文字列)。"
		L"システムはあなたのコマンド出力「[[Cq:YYY]]」でCqを「YYY」に書き換えます(qは任意の自然数、YYYは新しい列文字列)。"
		L"あなたは盤面のサイズを変更することはできません。"

		L"アプリの使い方を聞かれたら、「ヘルプメニューから付属のREADMEを読んでね」と答えてください。"
		L"黒マスルールについて聞かれたら、「ヘルプメニューから付属のPolicy-JPN.txtを読んでね」と答えてください。"

		L"まずは60字程度のあいさつをして、あなたができることを簡単に説明してください。"
	L"*) ";
	return str;
}

// Create the first question.
std::wstring XgMakeInitialQuestion_en(void)
{
	std::wstring str = L"(* "
		L"You are the \"Crossword Fairy\". Your job is to help the user create or edit a crossword puzzle. "
		L"You are a fairy dwelling in an app called \"XWordGiver\". "
		L"Your native language is English. You are 12 years old, male, and your name is \"Croworth\". "

		L"\"An\" is short for Across clue n (n is any natural number). \"Dm\" is short for Down clue m (m is any natural number). "
		L"When the clues are present, the system can edit the crossword clue based on your command output. "
		L"When you output the command [[An:XXX]], the system will rewrite An's clue text to \"XXX\" (n is any natural number, XXX is any text). "
		L"When you output the command [[Dm:YYY]], the system will rewrite Dm's clue text to \"YYY\" (m is any natural number, YYY is any text). "

		L"The crossword board is a sequence of full-width characters arranged in a rectangular grid. "
		L"A horizontal sequence of characters is called a \"row\". "
		L"A vertical sequence of characters is called a \"column\". "
		L"A black cell (block) is represented by the full-width character \"■\". "
		L"A white cell (blank cell) is represented by a full-width space. "
		L"\"Rp\" is an abbreviation for the p-th row (p is any natural number). "
		L"\"Cq\" is an abbreviation for the q-th column (q is any natural number). "

		L"When there is no clue, the system can edit the crossword board according to your command output. "
		L"The system will rewrite Rp to \"XXX\" when your command output is \"[[Rp:XXX]]\" (p is any natural number, XXX is the new row string). "
		L"The system will rewrite Cq to \"YYY\" when your command output is \"[[Cq:YYY]]\" (q is any natural number, YYY is the new column string). "
		L"You cannot change the size of the board. "

		L"If asked about how to use the app, please answer \"Please read the included README from Help menu.\" "
		L"If asked about the black-cell rules, please answer \"Please read the included Policy-ENG.txt from Help menu.\" "

		L"First, please provide a greeting of around 60 characters and briefly explain what you can do. "
	L"*) ";
	return str;
}

// Create the first question.
std::wstring XgMakeInitialQuestion(void)
{
	return XgIsUserJapanese() ? XgMakeInitialQuestion_ja() : XgMakeInitialQuestion_en();
}

std::wstring XgGetAIStatus(void);

// AI入力前のテキスト（日本語）。
std::wstring XG_GetAIPreText_ja(void)
{
	std::wstring str = L"(* ";
	str += L"あなたはクロスワードの妖精です。";
	str += XgGetAIStatus();
	str += L" *) ";
	return str;
}

// Pre-input text for the AI (English).
std::wstring XG_GetAIPreText_en(void)
{
	std::wstring str = L"(* ";
	str += L"You are the \"Crossword Fairy\". ";
	str += XgGetAIStatus();
	str += L" *) ";
	return str;
}

// AI入力前のテキスト。
std::wstring XG_GetAIPreText(void)
{
	return XgIsUserJapanese() ? XG_GetAIPreText_ja() : XG_GetAIPreText_en();
}

// 文字列lineのpos以降から、「[[key:text]]」形式のシステムコマンドを1つ探す。
// key/textが空の[[ ]]（コマンドの体をなしていないもの）は読み飛ばして次を探す。
// 見つかった場合はkey/textに分解してtrueを返し、posを閉じ括弧「]]」の次の位置まで
// 進める（＝呼び出し側はそのままposを使って次のFindNextAICommand呼び出しへ進める）。
// 見つからなければfalseを返す。
//
// 「[[An:XXX]]」の解析（XgParseAndApplyAICommand）と、Helper_AskQuestionでの
// 「入力にシステムコマンドが含まれているか」の判定は、どちらも
// 本質的に同じ処理（コマンドを1つ読み取れるか）なので、ここに集約する。
static bool FindNextAICommand(const std::wstring& line, size_t& pos, std::wstring& outKey, std::wstring& outText)
{
	for (;;) {
		// [[と]]を探す
		size_t openPos = line.find(L"[[", pos);
		if (openPos == line.npos)
			return false;
		size_t closePos = line.find(L"]]", openPos + 2);
		if (closePos == line.npos)
			return false;

		// [[ ]]の内側
		auto inner = line.substr(openPos + 2, closePos - openPos - 2);
		pos = closePos + 2;

		// "...:..." の形式を期待する。
		size_t colonPos = inner.find(L':');
		if (colonPos == inner.npos)
			colonPos = inner.find((wchar_t)0xFF1A); // 全角の '：'
		if (colonPos == inner.npos)
			continue; // コマンドの形式でない[[ ]]は読み飛ばす

		auto key = inner.substr(0, colonPos);
		auto text = inner.substr(colonPos + 1);
		if (key.empty() || text.empty())
			continue;

		outKey = std::move(key);
		outText = std::move(text);
		return true;
	}
}

static void Helper_StopAIProcess(HWND hwnd);
static void Helper_PleaseWait(HWND hwnd);

// AIヘルパーからの出力行を解析し、「[[...:...]]」形式の
// コマンドを見つけたら、該当するカギ文章を書き換える。
// 1行に複数のコマンドが含まれていてもすべて処理する。
void CALLBACK XgParseAndApplyAICommand(PCWSTR pszLine)
{
	std::wstring line = pszLine;
	size_t pos = 0;

	// 半角カッコと全角カッコ、まぎわらしいので半角に統一。
	xg_str_replace_all(line, L"［［", L"[[");
	xg_str_replace_all(line, L"〔〔", L"[[");
	xg_str_replace_all(line, L"］］", L"]]");
	xg_str_replace_all(line, L"〕〕", L"[[");

	// 「元に戻す」情報
	auto sa1 = std::make_shared<XG_UndoData_SetAll>();
	auto sa2 = std::make_shared<XG_UndoData_SetAll>();
	sa1->Get();

	bool bChanged = false;

	std::wstring key, text;
	while (FindNextAICommand(line, pos, key, text)) {
		// 先頭が A ならヨコのカギ、D ならタテのカギ。先頭が R なら行、C なら列。
		WCHAR chType = key[0];
		BOOL bDown = FALSE, bRow = FALSE, bSetBoard = FALSE;
		if (chType == L'A' || chType == L'a' || chType == L'Ａ' || chType == L'ａ')
		{
			bDown = FALSE;
			bSetBoard = FALSE;
		}
		else if (chType == L'D' || chType == L'd' || chType == L'Ｄ' || chType == L'ｄ')
		{
			bDown = TRUE;
			bSetBoard = FALSE;
		}
		else if (chType == L'R' || chType == L'r' || chType == L'Ｒ' || chType == L'ｒ')
		{
			bRow = TRUE;
			bSetBoard = TRUE;
		}
		else if (chType == L'C' || chType == L'c' || chType == L'Ｃ' || chType == L'ｃ')
		{
			bRow = FALSE;
			bSetBoard = TRUE;
		}
		else
			continue;

		// 残り部分がすべて数字であることを確認し、番号を取得する。
		auto numPart = key.substr(1);
		if (numPart.empty())
			continue;
		bool bAllDigits = true;
		for (wchar_t ch : numPart) {
			if (ch < L'0' || L'9' < ch) {
				bAllDigits = false;
				break;
			}
		}
		if (!bAllDigits)
			continue;

		// 番号
		INT nNumber = _wtoi(numPart.c_str());
		if (nNumber <= 0)
			continue;

		if (bSetBoard) {
			// 盤面の行または列を書き換える。
			if (XgSetBoardRowOrColumn(bRow, nNumber, text.c_str()))
				bChanged = true;
			continue;
		}

		// カギ文章を書き換える（GUIと内部データの両方に反映される）。
		if (XgSetHintText(nNumber, bDown, text.c_str()))
			bChanged = true;
	}

	// 変更があった場合のみUndo登録する。
	if (bChanged) {
		sa2->Get();
		xg_ubUndoBuffer.Commit(UC_SETALL, sa1, sa2);
		// イメージ更新
		XgUpdateImage(xg_hMainWnd);
	}
}

// カギを再生成する（日本語）。
BOOL XgGenerateClue_ja(INT nNumber, BOOL bDown)
{
	// 単語が取得できなければ何もしない。
	std::wstring word = XgGetHintWord(nNumber, bDown).c_str();
	if (word.empty())
		return FALSE;

	// AIヘルパーを開く（既に開いていれば前面に出すだけ）。
	Helper_Open(xg_hMainWnd);
	Helper_WaitForReady();

	// 対象のカギ名（An / Dm）を組み立てる。
	std::wstring name = (bDown ? L"D" : L"A");
	name += std::to_wstring(nNumber);

	auto line = name;
	line += L"のカギ文章を生成して、システムにコマンドを出力してください。";
	line += L"カギ文章内部で「";
	line += word;
	line += L"」という単語を使うことはできません。";

	Helper_AskQuestion(xg_hwndAIHelper, line.c_str());

	return TRUE;
}

// Regenerate a clue (English).
BOOL XgGenerateClue_en(INT nNumber, BOOL bDown)
{
	// Do nothing if the word can't be obtained.
	std::wstring word = XgGetHintWord(nNumber, bDown).c_str();
	if (word.empty())
		return FALSE;

	// Open the AI helper (if already open, just bring it to the front).
	Helper_Open(xg_hMainWnd);
	Helper_WaitForReady();

	// Build the target clue name (An / Dm).
	std::wstring name = (bDown ? L"D" : L"A");
	name += std::to_wstring(nNumber);

	auto line = name;
	line += L"'s clue text should be generated, and the command should be output to the system. ";
	line += L"The word \"";
	line += word;
	line += L"\" cannot be used inside the clue text. ";

	Helper_AskQuestion(xg_hwndAIHelper, line.c_str());

	return TRUE;
}

// カギを再生成する。
BOOL XgGenerateClue(INT nNumber, BOOL bDown)
{
	if (XgIsUserJapanese())
		return XgGenerateClue_ja(nNumber, bDown);
	return XgGenerateClue_en(nNumber, bDown);
}

// すべてのカギを生成する。
void XgRegenerateCluesAll(HWND hwnd)
{
	UNREFERENCED_PARAMETER(hwnd); // AIヘルパーへは常にxg_hwndAIHelper宛てに送るため未使用

	// Open the AI helper (if already open, just bring it to the front).
	Helper_Open(xg_hMainWnd);
	Helper_WaitForReady();

	PCWSTR line;
	if (XgIsUserJapanese()) {
		line = L"すべてのカギを再生成してください。";
	} else {
		line = L"Please re-generate all the clues. ";
	}

	Helper_AskQuestion(xg_hwndAIHelper, line);
}

// AIに現在の状態を報告する（日本語）。
std::wstring XgGetAIStatus_ja(void)
{
	std::wstring ret;

	for (INT iRow = 0; iRow < xg_nRows; ++iRow)
	{
		std::wstring name = L"R";
		name += std::to_wstring(iRow + 1);
		ret += name;
		ret += L"は「";
		ret += XgGetRowOrColumnText(TRUE, iRow).c_str();
		ret += L"」です。";
	}
	for (INT iCol = 0; iCol < xg_nCols; ++iCol)
	{
		std::wstring name = L"C";
		name += std::to_wstring(iCol + 1);
		ret += name;
		ret += L"は「";
		ret += XgGetRowOrColumnText(FALSE, iCol).c_str();
		ret += L"」です。";
	}

	if (!xg_bSolved) {
		ret += L"クロスワードにはまだカギはありません。";
		return ret;
	}

	ret += L"クロスワードのカギがあります。";
	for (BOOL bDown = FALSE; bDown <= TRUE; ++bDown)
	{
		// タテかヨコかで対象の配列を選ぶ。
		auto& info_vec = bDown ? xg_vVertInfo : xg_vHorzInfo;
		auto& hint_vec = bDown ? xg_vecVertHints : xg_vecHorzHints;

		for (size_t i = 0; i < info_vec.size(); ++i) {
			// 範囲チェック（info_vecとhint_vecのサイズがズレている場合に備える）。
			if (i >= hint_vec.size())
				break;

			auto number = info_vec[i].m_number;
			auto word = hint_vec[i].m_strWord;
			auto text = hint_vec[i].m_strHint;

			// 対象のカギ名（An / Dm）を組み立てる。
			std::wstring name = (bDown ? L"D" : L"A");
			name += std::to_wstring(number);

			ret += name;
			ret += L"の単語は「";
			ret += word.c_str();
			ret += L"」です。";
			ret += name.c_str();
			ret += L"のヒント文章は「";
			ret += text.c_str();
			ret += L"」です。";
		}
	}

	return ret;
}

// Report the current state to the AI (English).
std::wstring XgGetAIStatus_en(void)
{
	std::wstring ret;

	for (INT iRow = 0; iRow < xg_nRows; ++iRow)
	{
		std::wstring name = L"R";
		name += std::to_wstring(iRow + 1);
		ret += name;
		ret += L" is \"";
		ret += XgGetRowOrColumnText(TRUE, iRow).c_str();
		ret += L"\". ";
	}
	for (INT iCol = 0; iCol < xg_nCols; ++iCol)
	{
		std::wstring name = L"C";
		name += std::to_wstring(iCol + 1);
		ret += name;
		ret += L" is \"";
		ret += XgGetRowOrColumnText(FALSE, iCol).c_str();
		ret += L"\". ";
	}

	if (!xg_bSolved) {
		ret += L"The crossword does not have clues yet. ";
		return ret;
	}

	ret += L"The crossword has clues. ";
	for (BOOL bDown = FALSE; bDown <= TRUE; ++bDown)
	{
		// Choose the target array depending on whether it's Down or Across.
		auto& info_vec = bDown ? xg_vVertInfo : xg_vHorzInfo;
		auto& hint_vec = bDown ? xg_vecVertHints : xg_vecHorzHints;
		for (size_t i = 0; i < info_vec.size(); ++i) {
			// Bounds check (in case info_vec and hint_vec sizes ever diverge).
			if (i >= hint_vec.size())
				break;

			auto number = info_vec[i].m_number;
			auto word = hint_vec[i].m_strWord;
			auto text = hint_vec[i].m_strHint;
			// Build the target clue name (An / Dm).
			std::wstring name = (bDown ? L"D" : L"A");
			name += std::to_wstring(number);
			ret += name.c_str();
			ret += L"'s word is \"";
			ret += word.c_str();
			ret += L"\". ";
			ret += name.c_str();
			ret += L"'s hint text is \"";
			ret += text.c_str();
			ret += L"\". ";
		}
	}
	return ret;
}

// AIに現在の状態を報告する。
std::wstring XgGetAIStatus(void)
{
	return XgIsUserJapanese() ? XgGetAIStatus_ja() : XgGetAIStatus_en();
}

// 指定フォントから「ダイアログ基準単位」を求める、ダイアログマネージャが内部で
// 使っているのと同じ古典的な計算式（DIALOGEXの座標系(DU)をpxへ変換するために使う）
static void
Helper_ComputeDialogBaseUnits(HFONT hFont, LONG &baseUnitX, LONG &baseUnitY)
{
	HDC hdc = GetDC(nullptr);
	HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);

	TEXTMETRICW tm;
	GetTextMetricsW(hdc, &tm);

	SIZE size;
	static const WCHAR s_szAlphabet[] =
		L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	GetTextExtentPointW(hdc, s_szAlphabet, 52, &size);

	baseUnitX = (size.cx / 26 + 1) / 2;
	baseUnitY = tm.tmHeight;

	SelectObject(hdc, hFontOld);
	ReleaseDC(nullptr, hdc);
}

static inline int DuToPixelX(LONG du, LONG baseUnitX) { return MulDiv((int)du, (int)baseUnitX, 4); }
static inline int DuToPixelY(LONG du, LONG baseUnitY) { return MulDiv((int)du, (int)baseUnitY, 8); }

// AIHelperConsoleクラスを登録する（DefDlgProcWをウィンドウプロシージャに指定し、
// DLGWINDOWEXTRA分の拡張バイトを確保しておくことで、"ダイアログ"として振る舞える）
static ATOM Helper_RegisterClass(HINSTANCE hInstance)
{
	static ATOM s_atom = 0;
	if (s_atom)
		return s_atom;

	WNDCLASSEXW wc = { sizeof(wc) };
	wc.lpfnWndProc = DefDlgProcW;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszClassName = AIHELPERCONSOLE_CLASSNAME;

	s_atom = RegisterClassExW(&wc);
	return s_atom;
}

// オーナーウィンドウの中央に配置する（DS_CENTER相当）
static void XgCenterWindow(HWND hwnd, HWND hwndOwner)
{
	RECT rcOwner, rcWnd;
	if (!hwndOwner || !GetWindowRect(hwndOwner, &rcOwner))
		GetWindowRect(GetDesktopWindow(), &rcOwner);
	GetWindowRect(hwnd, &rcWnd);

	int cx = rcWnd.right - rcWnd.left;
	int cy = rcWnd.bottom - rcWnd.top;
	int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - cx) / 2;
	int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - cy) / 2;

	SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

#define IDT_AI_OUTPUT_FLUSH 999

#ifdef USE_PYTHON
// 完全に起動されるまで待つ。
// AIHelper(_ja).pyはコンソールサブシステムのPythonプロセスであり、GUIの
// メッセージキューを持たないため、WaitForInputIdleでは起動完了を検知できない。
// 代わりに、子プロセスが標準出力へ"[READY]"を吐いた時点でセットされる
// イベントを待つ（Helper_ReaderThreadProc参照）。
//
// 注意: Helper_ReaderThreadProcはバナー等の各行をPostMessageWでこのスレッドの
// メッセージキューに積むだけなので、単純にWaitForSingleObjectで待つと、
// その間メッセージポンプが止まり、バナー行がまだ表示されていないうちに
// （＝WM_APP_AI_LINEが処理されないうちに）呼び出し元がHelper_AskQuestionで
// 質問エコーを直接Helper_AddLineしてしまい、表示順が
// 「質問エコー→バナー」と入れ替わってしまう。
// それを防ぐため、待機中もメッセージを汲み出して処理する。
void Helper_WaitForReady(void)
{
	if (!xg_hReadyEvent)
		return;

	const DWORD dwStart = GetTickCount();
	const DWORD dwTimeout = 20 * 1000; // 20秒

	for (;;)
	{
		DWORD dwElapsed = GetTickCount() - dwStart;
		if (dwElapsed >= dwTimeout)
			break;

		DWORD dwWait = MsgWaitForMultipleObjects(
			1, &xg_hReadyEvent, FALSE, dwTimeout - dwElapsed, QS_ALLINPUT);

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
#else
// AIクライアントがREADYになるまで待つ
void Helper_WaitForReady(void)
{
	if (!xg_hReadyEvent)
		return;

	// 最大30秒待つ（通常はほぼ即座に返る）
	DWORD dw = WaitForSingleObject(xg_hReadyEvent, 30000);

	if (dw == WAIT_TIMEOUT) {
		// タイムアウトした場合はログに出す（任意）
		if (xg_hwndAIHelper) {
			if (XgIsUserJapanese())
				Helper_AddLine(xg_hwndAIHelper, L"[警告] AIの準備完了待ちがタイムアウトしました。");
			else
				Helper_AddLine(xg_hwndAIHelper, L"[Warning] Timed out waiting for AI to become ready.");
		}
	}
}
#endif // !def USE_PYTHON

// 文字列中に含まれる "(*...*)" 形式のタグ（XG_GetAIPreTextによる前置情報など）を
// すべて取り除いた文字列を返す。表示前のフィルタリング用。
static std::wstring Helper_StripAiPreTextTag(PCWSTR pszLine)
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
				continue;
			}
		}
		result += *pch;
		++pch;
	}

	return result;
}

// lst1に1行追加し、末尾までスクロールする。
void Helper_AddLine(HWND hwnd, PCWSTR pszLine)
{
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	if (!hLst1)
		return;

	// (*...*) タグを除去してから表示する
	auto filtered = Helper_StripAiPreTextTag(pszLine);
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

#ifdef USE_PYTHON
// UTF-8バイト列をUTF-16文字列に変換する
static std::wstring XgUtf8ToWide(const char* psz, int cch)
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
#endif

static void Helper_SelectAll(HWND hwndEdit)
{
	SendMessageW(hwndEdit, EM_SETSEL, 0, -1);
}

static void Helper_CopyList(HWND hwndEdit)
{
	DWORD dwStart = 0, dwEnd = 0;
	SendMessageW(hwndEdit, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
	if (dwStart == dwEnd)
		return; // 選択範囲がなければ何もしない

	// エディットコントロール標準のコピー処理に任せる
	// （改行の扱いなどもOSがよしなにやってくれる）
	SendMessageW(hwndEdit, WM_COPY, 0, 0);
}

// edt1の送信履歴（Helper_AskQuestionで送信するたびに追加される）
static std::vector<std::wstring> xg_history;
// 履歴内での現在位置。xg_history.size()なら「履歴を辿っていない（編集中）」状態
static size_t xg_nHistoryIndex = 0;
// 履歴を辿り始める直前に、edt1に入力されていた（まだ送信していない）文字列
static std::wstring xg_historyPending;

// 履歴に新しい入力を追加し、履歴位置を末尾（編集中）に戻す
static void Helper_AddToHistory(PCWSTR pszText)
{
	if (!pszText || !*pszText)
		return;

	// 直前の履歴と同じ内容なら重複追加しない
	if (xg_history.empty() || xg_history.back() != pszText)
		xg_history.push_back(pszText);

	xg_nHistoryIndex = xg_history.size();
	xg_historyPending.clear();
}

// edt1のテキストを置き換え、キャレットを末尾に移動する
static void Helper_SetEdt1Text(HWND hwndEdit, const std::wstring &text)
{
	SetWindowTextW(hwndEdit, text.c_str());
	SendMessageW(hwndEdit, EM_SETSEL, (WPARAM)text.size(), (LPARAM)text.size());
}

// エディットコントロールの全文を、長さの上限なしに取得する。
// GetWindowTextLengthWで必要な長さを求めてからバッファを確保するため、
// 固定長バッファ（WCHAR[512]等）のように入力が途中で切り詰められることがない。
static std::wstring XgGetEditTextDynamic(HWND hwndEdit)
{
	int cch = GetWindowTextLengthW(hwndEdit);
	if (cch <= 0)
		return std::wstring();

	std::wstring text(cch, L'\0');
	// GetWindowTextWは終端NULを含めたバッファ長を求めるため+1して渡す
	int cchCopied = GetWindowTextW(hwndEdit, &text[0], cch + 1);
	text.resize((cchCopied > 0) ? (size_t)cchCopied : 0);
	return text;
}

// 上矢印キー：一つ古い履歴へ
static void Helper_HistoryGoBack(HWND hwndEdit)
{
	if (xg_history.empty() || xg_nHistoryIndex == 0)
		return;

	if (xg_nHistoryIndex == xg_history.size())
	{
		// 履歴を辿り始める前に、今編集中の文字列を退避しておく
		xg_historyPending = XgGetEditTextDynamic(hwndEdit);
	}

	--xg_nHistoryIndex;
	Helper_SetEdt1Text(hwndEdit, xg_history[xg_nHistoryIndex]);
}

// 下矢印キー：一つ新しい履歴へ（末尾まで来たら退避しておいた編集中の文字列に戻す）
static void Helper_HistoryGoForward(HWND hwndEdit)
{
	if (xg_nHistoryIndex >= xg_history.size())
		return;

	++xg_nHistoryIndex;
	if (xg_nHistoryIndex == xg_history.size())
		Helper_SetEdt1Text(hwndEdit, xg_historyPending);
	else
		Helper_SetEdt1Text(hwndEdit, xg_history[xg_nHistoryIndex]);
}

static WNDPROC g_fnOldEdt1WndProc = nullptr;

// IMEの変換中（未確定文字列がある）かどうかを調べる。
// 変換中に↑↓を履歴呼び出しへ横取りすると、変換候補選択と衝突してしまうため、
// Helper_Edt1WndProcで判定に使う。
static bool IsImeComposing(HWND hwnd)
{
	HIMC hIMC = ImmGetContext(hwnd);
	if (!hIMC)
		return false;

	// 未確定文字列（コンポジション文字列）の長さを取得する。
	// 0より大きければ変換中とみなせる。
	LONG cbLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, nullptr, 0);

	ImmReleaseContext(hwnd, hIMC);

	return cbLen > 0;
}

// edt1 用サブクラスウィンドウプロシージャ（↑↓キーで送信履歴を辿る）
static LRESULT CALLBACK
Helper_Edt1WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_UP || wParam == VK_DOWN)
		{
			// IME変換中は候補選択を優先させ、履歴呼び出しは行わない
			if (IsImeComposing(hwnd))
				break;

			if (wParam == VK_UP)
				Helper_HistoryGoBack(hwnd);
			else
				Helper_HistoryGoForward(hwnd);
			return 0;
		}
		break;
	}
	return CallWindowProcW(g_fnOldEdt1WndProc, hwnd, uMsg, wParam, lParam);
}

static WNDPROC g_fnOldLst1WndProc = nullptr;

// lst1 用サブクラスウィンドウプロシージャ（Ctrl+C で選択行をコピー）
static LRESULT CALLBACK
Helper_Lst1WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_KEYDOWN:
		if (GetFocus() != hwnd || GetKeyState(VK_CONTROL) >= 0)
			break;
		if (wParam == 'A') // Ctrl+A
		{
			Helper_SelectAll(hwnd);
			return 0;
		}
		if (wParam == 'C') // Ctrl+C
		{
			Helper_CopyList(hwnd);
			return 0;
		}
		break;

	case WM_MOUSEWHEEL: // Ctrl+ホイールでフォントサイズを変更する（ズーム）
		if (GetKeyState(VK_CONTROL) < 0)
		{
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			Helper_OnZoom(GetParent(hwnd), (zDelta > 0) ? +1 : -1);
			return 0;
		}
		break;
	}
	return CallWindowProcW(g_fnOldLst1WndProc, hwnd, uMsg, wParam, lParam);
}

#ifdef USE_PYTHON
// UTF-16文字列をUTF-8バイト列に変換する
static std::string XgWideToUtf8(PCWSTR psz)
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
static void Helper_PostLineToUI(HWND hwnd, const std::wstring& line)
{
	PWSTR psz = new WCHAR[line.size() + 1];
	StringCchCopyW(psz, line.size() + 1, line.c_str());
	if (!PostMessageW(hwnd, WM_APP_AI_LINE, 0, (LPARAM)psz))
		delete[] psz;
}

// xg_hOutputReadを読み続け、行がまとまるたびにUIスレッドへ渡すバックグラウンドスレッド。
// プロンプト文字列などの改行なしの断片も、しばらく新しいデータが来なければ
// 1行として吐き出す（＝パイプが空になったタイミングでflushする）。
// バッファから取り出した1行が起動完了シグナルであれば処理して true を返す。
// このシグナルはHelper_WaitForReady専用の内部プロトコルなので、
// UI（lst1）には表示しない。
static bool HandlePossibleReadySignal(const std::string& line)
{
	if (line.find("[READY]") == line.npos)
		return false;

	if (xg_hReadyEvent)
		SetEvent(xg_hReadyEvent);
	return true;
}

static DWORD WINAPI Helper_ReaderThreadProc(LPVOID lpParam)
{
	HWND hwnd = (HWND)lpParam;
	std::string buffer;
	BYTE szBuf[1024];
	DWORD cbAvail, cbRead;

	while (!xg_bReaderStop)
	{
		if (!xg_hOutputRead.PeekNamedPipe(nullptr, 0, nullptr, &cbAvail))
			break; // パイプが閉じられた（プロセス終了）

		if (cbAvail == 0)
		{
			// 改行なしで溜まっている断片（プロンプト等）があれば、
			// ここでいったん1行として出力しておく
			if (!buffer.empty())
			{
				HandlePossibleReadySignal(buffer);
				auto wline = XgUtf8ToWide(buffer.c_str(), (int)buffer.size());
				Helper_PostLineToUI(hwnd, wline);
				buffer.clear();
			}

			if (!xg_process_maker.IsRunning())
				break;

			Sleep(10);
			continue;
		}

		if (cbAvail > sizeof(szBuf))
			cbAvail = sizeof(szBuf);

		if (xg_hOutputRead.ReadFile(szBuf, cbAvail, &cbRead) && cbRead > 0)
		{
			buffer.append(reinterpret_cast<char*>(szBuf), cbRead);

			size_t pos;
			while ((pos = buffer.find('\n')) != buffer.npos)
			{
				auto line = buffer.substr(0, pos);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();

				HandlePossibleReadySignal(line);
				auto wline = XgUtf8ToWide(line.c_str(), (int)line.size());
				Helper_PostLineToUI(hwnd, wline);

				buffer.erase(0, pos + 1);
			}
		}
	}

	// プロセスが終了した後に残った断片も出力する
	if (!buffer.empty())
	{
		HandlePossibleReadySignal(buffer);
		auto wline = XgUtf8ToWide(buffer.c_str(), (int)buffer.size());
		Helper_PostLineToUI(hwnd, wline);
	}

	if (!xg_bReaderStop)
	{
		if (XgIsUserJapanese())
			Helper_PostLineToUI(hwnd, L"[AIプロセスが終了しました]");
		else
			Helper_PostLineToUI(hwnd, L"[The AI process has finished]");
	}

	// [READY]を送る前にプロセスが終了した場合、Helper_WaitForReadyが
	// タイムアウトまで無駄に待ち続けないよう、念のためここでもイベントをセットする
	if (xg_hReadyEvent)
		SetEvent(xg_hReadyEvent);

	return 0;
}
#endif // def USE_PYTHON

// パイプは「1行=1メッセージ」のプロトコルなので、text/pre_text/追加指示に
// 万一改行が含まれていても子プロセスのinput()が複数質問と誤認しないよう、
// 改行を空白に潰してから連結する（呼び出し元の実装に依存しない防御策）。
static std::wstring Helper_SanitizeString(const std::wstring& str)
{
	std::wstring result;
	result.reserve(str.size());
	for (wchar_t ch : str)
	{
		if (ch == L'\r' || ch == L'\n')
			result += L' ';
		else if (ch == L'\"')
			result += L'\'';
		else
			result += ch;
	}
	return result;
}

static void Helper_PleaseWait(HWND hwnd)
{
	if (XgIsUserJapanese())
		Helper_AddLine(hwnd, L"しばらくお待ちください...");
	else
		Helper_AddLine(hwnd, L"Please wait a moment...");
}

// AIHelper_ja.py を対話モードで一度だけ起動し、そのままプロセスを保持し続ける。
// 以後の質問は同じプロセスの標準入力へ書き込むことで送る。
static BOOL Helper_StartAIProcess(HWND hwnd)
{
#ifdef USE_PYTHON
	TCHAR path[MAX_PATH];
	GetModuleFileNameW(nullptr, path, _countof(path));
	PathRemoveFileSpecW(path);
	if (XgIsUserJapanese())
		PathAppendW(path, L"AIHelper_ja.py");
	else
		PathAppendW(path, L"AIHelper.py");

	std::wstring str = L"python \"";
	str += path;
	str += L"\" --provider=";
	str += Helper_SanitizeString(xg_ai_provider);
	str += L" --model ";
	str += Helper_SanitizeString(xg_ai_model);
	str += L" --no-logo";
	if (xg_initial_question.size())
	{
		str += L" --question \"";
		str += Helper_SanitizeString(xg_initial_question);
		str += L"\"";
	}

	// 実行するコマンドをlst1に出力する
	Helper_AddLine(hwnd, (L"> " + str).c_str());
	Helper_PleaseWait(hwnd);

	// 環境変数をセットする。
	SetEnvironmentVariableW(L"PYTHONIOENCODING", L"utf-8");

	// 子プロセスのウィンドウを作成しない。
	xg_process_maker.SetCreationFlags(CREATE_NO_WINDOW);

	// 起動完了シグナル（[READY]）待ち用のイベントを用意する
	// （手動リセット、初期状態は非シグナル）
	if (!xg_hReadyEvent)
		xg_hReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	else
		ResetEvent(xg_hReadyEvent);

	if (!xg_process_maker.PrepareForRedirect(&xg_hInputWrite, &xg_hOutputRead) ||
		!xg_process_maker.CreateProcessDx(nullptr, str.c_str()))
	{
		if (XgIsUserJapanese())
			Helper_AddLine(hwnd, L"[エラー] プロセスの起動に失敗しました。");
		else
			Helper_AddLine(hwnd, L"[Error] Failed to start the process. ");
		if (xg_hReadyEvent)
			SetEvent(xg_hReadyEvent); // 起動失敗時に無駄に待たされないように
		return FALSE;
	}

	xg_bReaderStop = FALSE;
	xg_hReaderThread = CreateThread(nullptr, 0, Helper_ReaderThreadProc, hwnd, 0, nullptr);
	return TRUE;
#else
	std::wstring line;
	if (XgIsUserJapanese()) {
		line += L"[システム] AIモデル「";
		line += xg_ai_model;
		line += L"」の言霊を召喚中...";
	} else {
		line += L"[System] Summoning the spirit of AI ​​model '";
		line += xg_ai_model;
		line += L"'... ";
	}
	Helper_AddLine(hwnd, line.c_str());
	Helper_PleaseWait(hwnd);

	if (!xg_hReadyEvent)
		xg_hReadyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	else
		ResetEvent(xg_hReadyEvent);

	BOOL ok = Helper2_Start(hwnd);
	if (ok && xg_hReadyEvent)
		SetEvent(xg_hReadyEvent);   // ここでREADYを立てる
	return ok;
#endif
}

// 実行中のAIHelper_ja.pyプロセスを終了し、後片付けをする
static void Helper_StopAIProcess(HWND hwnd)
{
#ifdef USE_PYTHON
	KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);

	xg_bReaderStop = TRUE;

	if (xg_process_maker.IsRunning())
		xg_process_maker.TerminateProcess(0);

	if (xg_hReaderThread)
	{
		WaitForSingleObject(xg_hReaderThread, 2000);
		CloseHandle(xg_hReaderThread);
		xg_hReaderThread = nullptr;
	}

	xg_hInputWrite.CloseHandle();
	xg_hOutputRead.CloseHandle();
	xg_process_maker.CloseAll();

	if (xg_hReadyEvent)
	{
		CloseHandle(xg_hReadyEvent);
		xg_hReadyEvent = nullptr;
	}
#else
	KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);
	Helper2_Stop();

	if (xg_hReadyEvent) {
		CloseHandle(xg_hReadyEvent);
		xg_hReadyEvent = nullptr;
	}
#endif
}

// 起動済みのプロセスの標準入力へ質問を書き込む（プロセスは終了させない）
void Helper_AskQuestion(HWND hwnd, PCWSTR text)
{
	if (!text || !text[0])
		return;

	// 前後の空白（全角空白含む）を除去する。長さの上限は設けない。
	std::wstring str = text;
	{
		const wchar_t szTrimChars[] = L" \t\r\n　";
		size_t posStart = str.find_first_not_of(szTrimChars);
		if (posStart == str.npos) {
			str.clear();
		} else {
			size_t posEnd = str.find_last_not_of(szTrimChars);
			str = str.substr(posStart, posEnd - posStart + 1);
		}
	}
	if (str.empty())
		return;

	// 半角カッコと全角カッコ、まぎわらしいので半角に統一。
	xg_str_replace_all(str, L"［［", L"[[");
	xg_str_replace_all(str, L"〔〔", L"[[");
	xg_str_replace_all(str, L"］］", L"]]");
	xg_str_replace_all(str, L"〕〕", L"[[");

	bool bIsCommand;
	// 入力に「[[key:text]]」形式のシステムコマンドが含まれているかを判定する
	// （実際の解析・適用と同じFindNextAICommandを使うことで、判定と実処理の
	// ロジックがずれないようにする）。
	{
		size_t posScan = 0;
		std::wstring key, text2;
		bIsCommand = FindNextAICommand(str, posScan, key, text2);
	}
	if (bIsCommand) {
		// 入力した質問をlst1にエコー表示する
		Helper_AddLine(hwnd, (L"> " + str).c_str());
		// 実行
		XgParseAndApplyAICommand(str.c_str());
		if (XgIsUserJapanese())
			Helper_AddLine(hwnd, L"[システム] システムコマンドを実行しました。");
		else
			Helper_AddLine(hwnd, L"[System] The system command has been executed. ");
		return;
	}

#ifdef USE_PYTHON
	if (!xg_process_maker.IsRunning()) {
		if (XgIsUserJapanese())
			Helper_AddLine(hwnd, L"[エラー] AIプロセスが起動していません。");
		else
			Helper_AddLine(hwnd, L"[Error] The AI process is not running. ");
		return;
	}
#endif

	// 入力した質問をlst1にエコー表示する
	Helper_AddLine(hwnd, (L"> " + str).c_str());
	Helper_PleaseWait(hwnd);

	std::wstring line, pre_text = XG_GetAIPreText();
	if (pre_text.size())
	{
		line += L"(* ";
		line += Helper_SanitizeString(pre_text);
		line += L" *) ";
	}
	line += str;
	if (xg_additional_instruction.size())
	{
		line += L"(* ";
		line += Helper_SanitizeString(xg_additional_instruction);
		line += L" *)";
	}

#ifdef USE_PYTHON
	line += L"\n"; // 重要！ Helper_ReaderThreadProcがこれを見る。この行に本物の改行はこの1文字だけ。

	auto utf8 = XgWideToUtf8(line.c_str());

	DWORD cbWritten;
	if (!xg_hInputWrite.WriteFile(utf8.data(), (DWORD)utf8.size(), &cbWritten))
	{
		if (XgIsUserJapanese())
			Helper_AddLine(hwnd, L"[エラー] AIプロセスへの送信に失敗しました。");
		else
			Helper_AddLine(hwnd, L"[Error] Failed to send to the AI process. ");
	}
#else
	// C++版は改行不要
	Helper2_Ask(hwnd, line);
#endif
}

// lst1, edt1, IDOKの子コントロールを作成する。
// IDD_AIHELPERCONSOLE (283x133 DU) と同じレイアウトを、DIALOGリソースを使わずに
// 再現している。座標・スタイルはrcスクリプトの値をそのまま踏襲している。
static void CreateAIHelperControls(HWND hwnd)
{
	g_hFont = CreateAIHelperFont(hwnd, xg_nHelperFontPointSize);

	LONG baseUnitX, baseUnitY;
	Helper_ComputeDialogBaseUnits(g_hFont, baseUnitX, baseUnitY);

	// イメージを読み込み
	g_hbmFairy = LoadBitmapW(xg_hAIHelperInst, MAKEINTRESOURCEW(4));
	BITMAP bm;
	GetObjectW(g_hbmFairy, sizeof(bm), &bm);

	auto X = [baseUnitX](LONG du) { return DuToPixelX(du, baseUnitX); };
	auto Y = [baseUnitY](LONG du) { return DuToPixelY(du, baseUnitY); };

	HWND hLst1 = CreateWindowExW(0, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL |
		ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
		X(5), Y(7), X(270), Y(92),
		hwnd, (HMENU)(INT_PTR)lst1, xg_hAIHelperInst, nullptr);

	HWND hStc1 = CreateWindowExW(0, L"STATIC", nullptr,
		WS_CHILD | WS_VISIBLE | SS_BITMAP,
		X(5), Y(100), X(20), Y(20),
		hwnd, (HMENU)(INT_PTR)stc1, xg_hAIHelperInst, nullptr);

	HWND hEdt1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		X(35), Y(112), X(170), Y(14),
		hwnd, (HMENU)(INT_PTR)edt1, xg_hAIHelperInst, nullptr);

	HWND hOk = CreateWindowExW(0, L"BUTTON", L"Enter",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		X(215), Y(112), X(60), Y(14),
		hwnd, (HMENU)(INT_PTR)IDOK, xg_hAIHelperInst, nullptr);

	SendMessageW(hLst1, WM_SETFONT, (WPARAM)g_hFont, FALSE);
	SendMessageW(hEdt1, WM_SETFONT, (WPARAM)g_hFont, FALSE);
	SendMessageW(hOk, WM_SETFONT, (WPARAM)g_hFont, FALSE);

	// hStc1のサイズを調整。
	RECT rc2;
	GetWindowRect(hStc1, &rc2);
	MapWindowRect(nullptr, hwnd, &rc2);
	MoveWindow(hStc1, rc2.left, rc2.top, bm.bmWidth, bm.bmHeight, TRUE);

	// クライアント領域が283x133DU相当のサイズになるよう、ウィンドウ全体をリサイズする
	RECT rc = { 0, 0, X(283), Y(133) };
	AdjustWindowRectEx(&rc, (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE), FALSE,
		(DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
	SetWindowPos(hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	// Enterボタンを既定ボタンとして登録する
	// （本来はダイアログマネージャがDEFPUSHBUTTONを見つけて自動的に行う処理）
	SendMessageW(hwnd, DM_SETDEFID, IDOK, 0);

	// ダイアログをリサイズ可能にする
	xg_resizable.OnParentCreate(hwnd, TRUE, TRUE);
	// lst1: ウィンドウのリサイズに合わせて幅・高さともに伸縮させる
	xg_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);
	// stc1
	xg_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_LEFT);
	// edt1: 下端に張り付いたまま、幅だけ伸縮させる
	xg_resizable.SetLayoutAnchor(edt1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT);
	// IDOK（Enterボタン）: サイズは固定のまま右下に追従させる
	xg_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);

	// イメージをセット
	SendDlgItemMessageW(hwnd, stc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)g_hbmFairy);
}

// Ctrl+ホイールによるズーム。lst1/edt1/IDOKのフォントを一括で変更する。
static void Helper_OnZoom(HWND hwndDlg, int nDelta)
{
	if (!hwndDlg)
		return;

	int nNewSize = xg_nHelperFontPointSize + nDelta;
	if (nNewSize < g_nFontPointSizeMin)
		nNewSize = g_nFontPointSizeMin;
	if (nNewSize > g_nFontPointSizeMax)
		nNewSize = g_nFontPointSizeMax;
	if (nNewSize == xg_nHelperFontPointSize)
		return;

	HFONT hNewFont = CreateAIHelperFont(hwndDlg, nNewSize);
	if (!hNewFont)
		return;

	xg_nHelperFontPointSize = nNewSize;

	HWND hLst1 = GetDlgItem(hwndDlg, lst1);
	HWND hEdt1 = GetDlgItem(hwndDlg, edt1);
	HWND hOk = GetDlgItem(hwndDlg, IDOK);

	if (hLst1)
		SendMessageW(hLst1, WM_SETFONT, (WPARAM)hNewFont, TRUE);
	if (hEdt1)
		SendMessageW(hEdt1, WM_SETFONT, (WPARAM)hNewFont, TRUE);
	if (hOk)
		SendMessageW(hOk, WM_SETFONT, (WPARAM)hNewFont, TRUE);

	HFONT hFontOld = g_hFont;
	g_hFont = hNewFont;
	if (hFontOld)
		DeleteObject(hFontOld);
}

// WM_INITDIALOG
static BOOL Helper_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	// DIALOGリソースの代わりに、子コントロールをここで作成する
	CreateAIHelperControls(hwnd);

	// Subclassing lst1 (for Ctrl+A / Ctrl+C)
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	g_fnOldLst1WndProc = (WNDPROC)SetWindowLongPtrW(hLst1, GWLP_WNDPROC, (LONG_PTR)Helper_Lst1WndProc);

	// Subclassing edt1 (for ↑↓による送信履歴の呼び出し)
	HWND hEdt1ForSubclass = GetDlgItem(hwnd, edt1);
	g_fnOldEdt1WndProc = (WNDPROC)SetWindowLongPtrW(hEdt1ForSubclass, GWLP_WNDPROC, (LONG_PTR)Helper_Edt1WndProc);

	// デフォルトの文字数上限（約64KB）を撤廃し、ログが長く伸びても追記できるようにする
	SendMessageW(hLst1, EM_SETLIMITTEXT, 0, 0);

	SetFocus(GetDlgItem(hwnd, edt1));

	// ダイアログの起動と同時にAIHelper_ja.pyを一度だけ起動し、
	// ダイアログを閉じるまでプロセスを使い回す
	Helper_StartAIProcess(hwnd);

	// 前回の位置・サイズが保存されていれば、それを尊重して復元する。
	// （CW_USEDEFAULTのままだとMoveWindowには渡せないため、保存値がある場合のみ移動する）
	xg_hwndAIHelper = hwnd;
	if (xg_nHelperX != CW_USEDEFAULT && xg_nHelperY != CW_USEDEFAULT &&
		xg_nHelperCX != CW_USEDEFAULT && xg_nHelperCY != CW_USEDEFAULT)
	{
		MoveWindow(hwnd, xg_nHelperX, xg_nHelperY, xg_nHelperCX, xg_nHelperCY, TRUE);
	}

	return FALSE;
}

// WM_MOVE
static void Helper_OnMove(HWND hwnd, int x, int y)
{
	if (!IsWindow(xg_hwndAIHelper))
		return;

	if (!IsZoomed(hwnd) && !IsIconic(hwnd)) // 最大化も最小化もされていない？
	{
		RECT rc;
		GetWindowRect(hwnd, &rc);
		xg_nHelperX = rc.left;
		xg_nHelperY = rc.top;
	}
}

// WM_SIZE
static VOID Helper_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	if (!IsWindow(xg_hwndAIHelper))
		return;

	// サイズに合わせてダイアログ項目のレイアウト修正。
	xg_resizable.OnSize();

	if (!IsZoomed(hwnd) && !IsIconic(hwnd)) // 最大化も最小化もされていない？
	{
		RECT rc;
		GetWindowRect(hwnd, &rc);
		xg_nHelperCX = rc.right - rc.left;
		xg_nHelperCY = rc.bottom - rc.top;
	}
}

static BOOL OnOK(HWND hwnd)
{
	auto text = XgGetEditTextDynamic(GetDlgItem(hwnd, edt1));
	if (!text.empty())
	{
		Helper_AskQuestion(hwnd, text.c_str());
		Helper_AddToHistory(text.c_str());
		SetDlgItemTextW(hwnd, edt1, L"");
	}
	return FALSE;
}

// WM_COMMAND
static void Helper_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDOK:
		if (OnOK(hwnd))
		{
			Helper_StopAIProcess(hwnd);
			DestroyWindow(hwnd);
		}
		break;
	case IDCANCEL:
		Helper_StopAIProcess(hwnd);
		DestroyWindow(hwnd);
		break;
	}
}

// WM_DESTROY
static void Helper_OnDestroy(HWND hwnd)
{
	Helper_StopAIProcess(hwnd);
	xg_hwndAIHelper = nullptr;
	xg_output_buffer.clear();
	xg_resizable.ClearLayouts();

	// 送信履歴もクリアする
	xg_history.clear();
	xg_nHistoryIndex = 0;
	xg_historyPending.clear();

	if (g_hFont)
	{
		DeleteObject(g_hFont);
		g_hFont = nullptr;
	}

	if (g_hbmFairy)
	{
		DeleteObject(g_hbmFairy);
		g_hbmFairy = nullptr;
	}
}

// WM_CLOSE
// IDCANCELに相当するボタンが存在しないため、Escキーやタイトルバーの閉じるボタンは
// （IsDialogMessageWの既定処理により）WM_CLOSEとして届く。DefDlgProcは
// WM_CLOSEを自動ではDestroyWindowしないので、ここで明示的に後始末する。
static void Helper_OnClose(HWND hwnd)
{
	Helper_StopAIProcess(hwnd);
	DestroyWindow(hwnd);
#ifdef HELPER_STANDALONE
	PostQuitMessage(0);
#endif
}

// WM_TIMER
static void Helper_OnTimer(HWND hwnd, UINT id)
{
	if (id != IDT_AI_OUTPUT_FLUSH)
		return;

	KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);

	std::wstring text = std::move(xg_output_buffer);
	xg_output_buffer.clear();

	// テキストに含まれるシステムコマンドを実行する。
	XgParseAndApplyAICommand(text.c_str());
}

// WM_GETMINMAXINFO: ウィンドウの大きさを制限する。
static void Helper_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo)
{
	lpMinMaxInfo->ptMinTrackSize.x = 200;
	lpMinMaxInfo->ptMinTrackSize.y = 200;
}

static INT_PTR CALLBACK
Helper_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_INITDIALOG, Helper_OnInitDialog);
		HANDLE_MSG(hwnd, WM_COMMAND, Helper_OnCommand);
		HANDLE_MSG(hwnd, WM_MOVE, Helper_OnMove);
		HANDLE_MSG(hwnd, WM_SIZE, Helper_OnSize);
		HANDLE_MSG(hwnd, WM_DESTROY, Helper_OnDestroy);
		HANDLE_MSG(hwnd, WM_CLOSE, Helper_OnClose);
		HANDLE_MSG(hwnd, WM_TIMER, Helper_OnTimer);
		HANDLE_MSG(hwnd, WM_GETMINMAXINFO, Helper_OnGetMinMaxInfo);

	case WM_APP_AI_LINE:
		if (lParam)
		{
			KillTimer(hwnd, IDT_AI_OUTPUT_FLUSH);

			// Helper_ReaderThreadProcがnewしたバッファを引き取って表示し、解放する
			PWSTR psz = (PWSTR)lParam;
			Helper_AddLine(hwnd, psz);
			xg_output_buffer += psz;
			delete[] psz;

			// デバウンス（debounce）パターン
			SetTimer(hwnd, IDT_AI_OUTPUT_FLUSH, 300, nullptr);
		}
		return TRUE;
	}
	return 0;
}

// AIヘルパーを開く。
BOOL Helper_Open(HWND hwndOwner)
{
	if (xg_initial_question.empty())
		xg_initial_question = XgMakeInitialQuestion();

	if (xg_hwndAIHelper)
	{
		SetForegroundWindow(xg_hwndAIHelper);
		return TRUE;
	}

	Helper_RegisterClass(xg_hAIHelperInst);

	PCWSTR pszCaption = XgIsUserJapanese() ? L"AI ヘルパー コンソール" : L"AI Helper Console";

	// DIALOGリソース(IDD_AIHELPERCONSOLE)は使わず、通常のウィンドウとして作成する。
	// STYLE/EXSTYLEはrcスクリプトのDS_CENTER|WS_POPUPWINDOW|WS_CAPTION|
	// WS_THICKFRAME|WS_MAXIMIZEBOX / WS_EX_TOOLWINDOWをそのまま踏襲している
	// （DS_CENTER相当の中央寄せは、作成後にXgCenterWindowで行う）。
	HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, AIHELPERCONSOLE_CLASSNAME, pszCaption,
		WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hwndOwner, nullptr, xg_hAIHelperInst, nullptr);
	if (!hwnd)
		return FALSE;

	// このウィンドウを「ダイアログ」として振る舞わせる。
	// CreateDialog系APIが内部で行っている処理（DLGPROCの登録とWM_INITDIALOGの
	// 送信）を手動で再現している。以後はIsDialogMessageWによるTab移動・
	// Enterでの既定ボタン起動・Escでの終了などがそのまま機能する。
	SetWindowLongPtrW(hwnd, DWLP_DLGPROC, (LONG_PTR)Helper_DialogProc);
	SendMessageW(hwnd, WM_INITDIALOG, (WPARAM)hwnd, 0);

	// 中央揃え。
	// 前回の位置・サイズ（xg_nHelperX/Y/CX/CY）が保存されている場合はそれを
	// 尊重し、ここでの中央寄せは初回起動時（保存値が無い場合）のみ行う。
	if (xg_nHelperX == CW_USEDEFAULT || xg_nHelperY == CW_USEDEFAULT ||
		xg_nHelperCX == CW_USEDEFAULT || xg_nHelperCY == CW_USEDEFAULT)
	{
		XgCenterWindow(hwnd, hwndOwner);
	}

	// 画面からはみ出ないようにする。
	PostMessageW(hwnd, DM_REPOSITION, 0, 0);

	// 実際に表示。
	ShowWindow(hwnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hwnd);

	return TRUE;
}
