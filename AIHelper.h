// AIHelper.h --- XWordGiver AI Helper
// Author: katahiromz
// License: MIT

#pragma once

#include <vector>

extern HWND xg_hwndAIHelper;
extern HINSTANCE xg_hAIHelperInst;
extern std::wstring xg_ai_provider;
extern std::wstring xg_ai_model;
extern std::wstring xg_initial_question;
extern INT xg_nHelperFontPointSize;
extern INT xg_nHelperX;
extern INT xg_nHelperY;
extern INT xg_nHelperCX;
extern INT xg_nHelperCY;
extern std::map<std::wstring, std::vector<std::wstring>> xg_knownAIModels;
extern std::vector<std::wstring> xg_knownAIProviders;
extern std::vector<std::wstring> xg_prompt_mapping;

// プロンプトの種類。
enum PROMPT_TYPE {
    PROMPT_TYPE_INITIAL = 0,
    PROMPT_TYPE_GENERATE_CLUE,
    PROMPT_TYPE_GENERATE_CLUES_ALL,
    PROMPT_TYPE_EXPLAIN_CLUE,
    PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES,
    PROMPT_TYPE_JUDGE_SEASON,
    PROMPT_TYPE_JUDGE_THEME,
    PROMPT_TYPE_MAX,
};

// 子プロセスの出力の1行をUIスレッドへ渡すためのカスタムメッセージ
// (WPARAMは未使用、LPARAMはnewしたPWSTR。受け取った側でdelete[]すること)
#define WM_APP_AI_LINE  (WM_APP + 1)

BOOL Helper_Open(HWND hwndOwner);
void Helper_AskQuestion(HWND hwnd, PCWSTR text);
void Helper_AddLine(HWND hwnd, PCWSTR pszLine);
BOOL XgIsUserJapanese(VOID) noexcept;

BOOL Helper_GetAIModels(PCWSTR provider, std::vector<std::wstring>& models);
BOOL Helper_GetAIProviders(std::vector<std::wstring>& providers);
std::wstring Helper_GetPromptName(PROMPT_TYPE iPrompt);
std::wstring Helper_GetPromptString(PROMPT_TYPE iPrompt, const std::wstring& clue_name);
void Helper_SetPromptString(PROMPT_TYPE iPrompt, const std::wstring& str);
void Helper_ResetPrompts(std::vector<std::wstring>& prompt_mapping);
