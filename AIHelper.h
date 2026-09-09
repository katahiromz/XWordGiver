// AIHelper.h --- XWordGiver AI Helper
// Author: katahiromz
// License: MIT

#pragma once

extern HWND xg_hwndAIHelper;
extern HINSTANCE xg_hAIHelperInst;
extern std::wstring xg_ai_provider;
extern std::wstring xg_ai_model;
extern std::wstring xg_python_exe;
extern std::wstring xg_additional_instruction;
extern std::wstring xg_initial_question;
extern INT xg_nHelperFontPointSize;
extern INT xg_nHelperX;
extern INT xg_nHelperY;
extern INT xg_nHelperCX;
extern INT xg_nHelperCY;

// 子プロセスの出力の1行をUIスレッドへ渡すためのカスタムメッセージ
// (WPARAMは未使用、LPARAMはnewしたPWSTR。受け取った側でdelete[]すること)
#define WM_APP_AI_LINE  (WM_APP + 1)

void Helper_AskQuestion(HWND hwnd, PCWSTR text);
