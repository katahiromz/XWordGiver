// AIHelper.h --- XWordGiver AI Helper
// Author: katahiromz
// License: MIT

#pragma once

extern HWND g_hwndAIHelper;
extern HINSTANCE g_hAIHelperInst;
extern std::wstring g_provider;
extern std::wstring g_model;
extern std::wstring g_python_exe;
extern std::wstring g_additional_instruction;
extern std::wstring g_initial_question;
extern INT g_nHelperFontPointSize;

void AskAIQuestion(HWND hwnd, PCWSTR text);
