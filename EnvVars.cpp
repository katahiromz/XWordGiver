// EnvVars.cpp --- 環境変数を同期する
// Author: katahiromz
// License: MIT

#include "DetectLeaks.h"
#include <windows.h>
#include <userenv.h>
#include <map>
#include <string>
#include <cstdlib>
#include <cwchar>
#pragma comment(lib, "userenv.lib")

// 環境変数名は大文字小文字を区別しない
struct CiLess {
    bool operator()(const std::wstring& a, const std::wstring& b) const {
        return _wcsicmp(a.c_str(), b.c_str()) < 0;
    }
};
using EnvMap = std::map<std::wstring, std::wstring, CiLess>;

static EnvMap g_snapshot;          // 前回反映した(=システム由来の)環境
static bool   g_snapshotReady = false;

// システム + ユーザーの最新環境を取得
static bool LoadFreshEnvironment(EnvMap& out)
{
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &token))
        return false;

    LPVOID block = nullptr;
    BOOL ok = CreateEnvironmentBlock(&block, token, FALSE);
    CloseHandle(token);
    if (!ok) return false;

    for (const wchar_t* p = static_cast<const wchar_t*>(block); *p; p += wcslen(p) + 1)
    {
        if (*p == L'=') continue;                 // "=C:" 等の特殊エントリ
        const wchar_t* eq = wcschr(p, L'=');
        if (!eq) continue;
        out[std::wstring(p, eq)] = eq + 1;
    }
    DestroyEnvironmentBlock(block);
    return true;
}

// 現在のプロセスの環境変数を取得(存在しなければ false)
static bool GetProcessVar(const std::wstring& name, std::wstring& value)
{
    DWORD len = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
    if (len == 0) {
        // 値が空文字の変数か、存在しないかを区別
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) return false;
        value.clear();
        return true;
    }
    value.resize(len);                            // len は終端NUL込み
    DWORD n = GetEnvironmentVariableW(name.c_str(), &value[0], len);
    value.resize(n);
    return true;
}

// 変数の設定/削除(CRTの環境コピーも同期)
static void SetVar(const std::wstring& name, const wchar_t* valueOrNull)
{
    SetEnvironmentVariableW(name.c_str(), valueOrNull);   // nullptr で削除
    _wputenv_s(name.c_str(), valueOrNull ? valueOrNull : L""); // 空文字でCRT側から削除
}

// アプリ起動時に1回呼ぶ: 基準スナップショットを作る(環境は変更しない)
bool InitEnvironmentTracking()
{
    EnvMap fresh;
    if (!LoadFreshEnvironment(fresh)) return false;
    g_snapshot = std::move(fresh);
    g_snapshotReady = true;
    return true;
}

// WM_SETTINGCHANGE("Environment") 受信時に呼ぶ
// overwriteAppChanges: true ならアプリが独自に変更した値も上書きする
bool RefreshEnvironment(bool overwriteAppChanges/* = false*/)
{
    if (!g_snapshotReady && !InitEnvironmentTracking()) return false;

    EnvMap fresh;
    if (!LoadFreshEnvironment(fresh)) return false;

    // 1) 削除された変数: 旧スナップショットにあって新ブロックにないもの
    for (const auto& [name, oldValue] : g_snapshot)
    {
        if (fresh.find(name) != fresh.end()) continue;

        std::wstring cur;
        if (!GetProcessVar(name, cur)) continue;              // すでに無い
        if (overwriteAppChanges || cur == oldValue)           // アプリが変更していなければ削除
            SetVar(name, nullptr);
    }

    // 2) 追加・変更された変数
    for (const auto& [name, newValue] : fresh)
    {
        auto it = g_snapshot.find(name);
        std::wstring cur;
        bool exists = GetProcessVar(name, cur);

        if (it == g_snapshot.end())
        {
            // 新規追加。プロセス側に同名があってもアプリ設定の可能性があるので配慮
            if (!exists || overwriteAppChanges || cur == newValue)
                SetVar(name, newValue.c_str());
        }
        else if (it->second != newValue)
        {
            // 変更された。アプリが手を加えていなければ更新
            if (!exists || overwriteAppChanges || cur == it->second)
                SetVar(name, newValue.c_str());
        }
    }

    g_snapshot = std::move(fresh);
    return true;
}

void ClearEnvironment()
{
    g_snapshot.clear();
}
