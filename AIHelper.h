#pragma once

extern HWND g_hwndAIHelper;
extern HINSTANCE g_hAIHelperInst;
extern std::wstring g_privider;
extern std::wstring g_model;

void AskAIQuestion(HWND hwnd, PWSTR text);
BOOL OpenAIHelper(HWND hwndOwner, BOOL bOpen);
