// AIHelper2.h --- XWordGiver AI Helper (pure C++ / WinHTTP)
// Author: katahiromz + Grok
// License: MIT

#pragma once

#include <map>
#include <string>

// 対応プロバイダー: openai, google, anthropic, xai, deepseek, sakana, qwen, moonshot,
//                   mistral, llama, pepabo
// AIHelper2.cpp で実装
BOOL Helper2_Start(HWND hwnd);
void Helper2_Stop();
void Helper2_Ask(HWND hwnd, const std::wstring& text);
void Helper2_ResetHistory();

// プロバイダー接続設定（g_providers）を外部ファイル AIModels.dat から読み込む。
// 成功したら TRUE を返す。Helper2_Start が自動的に呼び出すため、通常は呼ぶ必要はない。
// Load the provider connection settings (g_providers) from the external file
// AIModels.dat. Returns TRUE on success. Helper2_Start calls this automatically,
// so callers normally do not need to call it directly.
BOOL LoadAIModelsData(void);

extern std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> g_histories; // provider -> [(role, content)];
