// AIHelper2.h --- XWordGiver AI Helper (pure C++ / WinHTTP, no Python)
// Author: katahiromz + Grok
// License: MIT

#pragma once

#include <map>
#include <string>

// AIHelper2.cpp で実装
BOOL Helper2_Start(HWND hwnd);
void Helper2_Stop();
void Helper2_Ask(HWND hwnd, const std::wstring& text);
void Helper2_ResetHistory();

extern std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> g_histories; // provider -> [(role, content)];
