#pragma once

extern HWND g_hwndAIHelper;
extern HINSTANCE g_hAIHelperInst;

void AskAIQuestion(HWND hwnd, PWSTR text);
BOOL OpenAIHelper(HWND hwndOwner, BOOL bOpen);
