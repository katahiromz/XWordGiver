#pragma once

extern HWND g_hwndAIHelper;
extern HINSTANCE g_hAIHelperInst;
extern std::wstring g_privider;
extern std::wstring g_model;
extern std::wstring g_python_exe;
extern std::wstring g_additional_instruction;

void AskAIQuestion(HWND hwnd, PCWSTR text);
BOOL OpenAIHelper(HWND hwndOwner, BOOL bOpen);

// AIプロセスから届いた出力行を、表示とは別に呼び出し側へ通知するためのコールバック。
// (*...*) タグの除去やフィルタリングは行わない、生の1行がそのまま渡される。
typedef void (CALLBACK *AIHELPER_LINE_CALLBACK)(LPCWSTR pszLine);
void AIHelper_SetLineCallback(AIHELPER_LINE_CALLBACK callback);
