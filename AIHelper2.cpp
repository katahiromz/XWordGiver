// AIHelper2.cpp --- XWordGiver AI Helper (pure C++ / WinHTTP, no Python)
// Author: katahiromz + Grok
// License: MIT
#include "DetectLeaks.h"
#include "XWordGiver.hpp"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <strsafe.h>
#include "AIHelper.h"
#include "resource.h"

#ifdef _MSC_VER
    #pragma comment(lib, "winhttp.lib")
#endif

// ---------------------------------------------------------------------------
// 内部状態
// ---------------------------------------------------------------------------
std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>> g_histories; // provider -> [(role, content)]
static volatile BOOL g_bStop = FALSE;
static HWND   g_hwndNotify = nullptr;

// ---------------------------------------------------------------------------
// ユーティリティ
// ---------------------------------------------------------------------------
static std::string WideToUtf8(const std::wstring& w)
{
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(len, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], len, nullptr, nullptr);
    return s;
}

static std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], len);
    return w;
}

static std::wstring EscapeJson(const std::wstring& src)
{
    std::wstring out;
    out.reserve(src.size() + 16);
    for (wchar_t c : src) {
        switch (c) {
        case L'"':  out += L"\\\""; break;
        case L'\\': out += L"\\\\"; break;
        case L'\b': out += L"\\b";  break;
        case L'\f': out += L"\\f";  break;
        case L'\n': out += L"\\n";  break;
        case L'\r': out += L"\\r";  break;
        case L'\t': out += L"\\t";  break;
        default:
            if (c < 0x20) {
                wchar_t buf[8];
                StringCchPrintfW(buf, _countof(buf), L"\\u%04x", (unsigned)c);
                out += buf;
            } else {
                out += c;
            }
            break;
        }
    }
    return out;
}

static std::wstring GetEnv(const wchar_t* name)
{
    wchar_t buf[4096];
    DWORD n = GetEnvironmentVariableW(name, buf, _countof(buf));
    if (n == 0 || n >= _countof(buf)) return {};
    return buf;
}

// 簡易JSON抽出（"key": "value" 形式の文字列値を取る）
static bool ExtractJsonString(const std::string& json, const char* key, std::string& out)
{
    std::string pat = std::string("\"") + key + "\"";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return false;
    pos = json.find(':', pos);
    if (pos == std::string::npos) return false;
    pos = json.find('"', pos);
    if (pos == std::string::npos) return false;
    ++pos;
    std::string val;
    while (pos < json.size()) {
        char c = json[pos++];
        if (c == '\\' && pos < json.size()) {
            char n = json[pos++];
            switch (n) {
            case '"': case '\\': case '/': val += n; break;
            case 'n': val += '\n'; break;
            case 'r': val += '\r'; break;
            case 't': val += '\t'; break;
            case 'u': // 簡易（4桁hexは無視してスキップ）
                if (pos + 4 <= json.size()) pos += 4;
                break;
            default: val += n; break;
            }
        } else if (c == '"') {
            break;
        } else {
            val += c;
        }
    }
    out = std::move(val);
    return true;
}

// ---------------------------------------------------------------------------
// プロバイダ設定
// ---------------------------------------------------------------------------
struct ProviderInfo {
    const wchar_t* envKey;
    const wchar_t* host;          // 例: api.openai.com
    const wchar_t* path;          // 例: /v1/chat/completions
    bool isOpenAICompat;
    bool isClaude;
    bool isGemini;
};

static const std::map<std::wstring, ProviderInfo> g_providers = {
    {L"chatgpt",  {L"OPENAI_API_KEY",    L"api.openai.com",               L"/v1/chat/completions", true,  false, false}},
    {L"grok",     {L"XAI_API_KEY",       L"api.x.ai",                     L"/v1/chat/completions", true,  false, false}},
    {L"deepseek", {L"DEEPSEEK_API_KEY",  L"api.deepseek.com",             L"/v1/chat/completions", true,  false, false}},
    {L"sakana",   {L"SAKANA_API_KEY",    L"api.sakana.ai",                L"/v1/chat/completions", true,  false, false}},
    {L"qwen",     {L"DASHSCOPE_API_KEY", L"dashscope-intl.aliyuncs.com",  L"/compatible-mode/v1/chat/completions", true, false, false}},
    {L"kimi",     {L"MOONSHOT_API_KEY",  L"api.moonshot.ai",              L"/v1/chat/completions", true,  false, false}},
    {L"mistral",  {L"MISTRAL_API_KEY",   L"api.mistral.ai",               L"/v1/chat/completions", true,  false, false}},
    {L"llama",    {L"LLAMA_API_KEY",     L"api.llama.com",                L"/compat/v1/chat/completions", true, false, false}},
    {L"claude",   {L"ANTHROPIC_API_KEY", L"api.anthropic.com",            L"/v1/messages",         false, true,  false}},
    {L"gemini",   {L"GOOGLE_API_KEY",    L"generativelanguage.googleapis.com", L"", false, false, true}},
};

// ---------------------------------------------------------------------------
// WinHTTP 共通送信
// ---------------------------------------------------------------------------
static bool HttpPost(const std::wstring& host, INTERNET_PORT port, const std::wstring& path,
                     const std::wstring& headers, const std::string& body,
                     std::string& response, DWORD& statusCode, std::wstring& errMsg)
{
    HINTERNET hSession = nullptr;
    HINTERNET hConnect = nullptr;
    HINTERNET hRequest = nullptr;
    bool success = false;
    statusCode = 0;

    hSession = WinHttpOpen(L"XWordGiver-AIHelper/2.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        errMsg = L"WinHttpOpen failed (" + std::to_wstring(GetLastError()) + L")";
        goto cleanup;
    }

    // タイムアウトを長めに設定（ミリ秒）
    // resolve / connect / send / receive
    WinHttpSetTimeouts(hSession, 15000, 15000, 30000, 60000);

    hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) {
        errMsg = L"WinHttpConnect failed (" + std::to_wstring(GetLastError()) + L")";
        goto cleanup;
    }

    hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(),
                                  nullptr, WINHTTP_NO_REFERER,
                                  WINHTTP_DEFAULT_ACCEPT_TYPES,
                                  WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        errMsg = L"WinHttpOpenRequest failed (" + std::to_wstring(GetLastError()) + L")";
        goto cleanup;
    }

    // セキュリティプロトコルを明示（古い環境対策）
    {
        DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#if defined(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3)
                        | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#endif
                        ;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
    }

    if (!WinHttpSendRequest(hRequest,
                            headers.c_str(), (DWORD)-1L,
                            (LPVOID)body.data(), (DWORD)body.size(),
                            (DWORD)body.size(), 0)) {
        errMsg = L"WinHttpSendRequest failed (" + std::to_wstring(GetLastError()) + L")";
        goto cleanup;
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        DWORD err = GetLastError();
        errMsg = L"WinHttpReceiveResponse failed (" + std::to_wstring(err) + L")";
        // よくあるエラーコードの補足
        if (err == ERROR_WINHTTP_TIMEOUT)
            errMsg += L" [timeout]";
        else if (err == ERROR_WINHTTP_SECURE_FAILURE)
            errMsg += L" [TLS/secure failure]";
        else if (err == ERROR_WINHTTP_CONNECTION_ERROR)
            errMsg += L" [connection error]";
        goto cleanup;
    }

    {
        DWORD dwStatus = 0;
        DWORD dwSize = sizeof(dwStatus);
        WinHttpQueryHeaders(hRequest,
                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &dwStatus, &dwSize,
                            WINHTTP_NO_HEADER_INDEX);
        statusCode = dwStatus;
    }

    response.clear();
    for (;;) {
        DWORD dwAvail = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwAvail)) {
            break;
        }
        if (dwAvail == 0)
            break;

        std::vector<char> buf(dwAvail + 1);
        DWORD dwRead = 0;
        if (!WinHttpReadData(hRequest, buf.data(), dwAvail, &dwRead) || dwRead == 0)
            break;

        response.append(buf.data(), dwRead);
    }

    success = (statusCode >= 200 && statusCode < 300);
    if (!success && errMsg.empty()) {
        errMsg = L"HTTP " + std::to_wstring(statusCode);
    }

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    return success;
}

// ---------------------------------------------------------------------------
// 各プロバイダのリクエスト構築・応答抽出
// ---------------------------------------------------------------------------
static std::wstring BuildOpenAIMessagesJson(const std::vector<std::pair<std::wstring, std::wstring>>& hist)
{
    std::wstring j = L"[";
    bool first = true;
    for (const auto& m : hist) {
        if (!first) j += L",";
        first = false;
        j += L"{\"role\":\"" + EscapeJson(m.first) + L"\",\"content\":\"" + EscapeJson(m.second) + L"\"}";
    }
    j += L"]";
    return j;
}

static bool AskOpenAICompat(const ProviderInfo& info, const std::wstring& model,
                            const std::vector<std::pair<std::wstring, std::wstring>>& hist,
                            std::wstring& answer, std::wstring& err)
{
    std::wstring apiKey = GetEnv(info.envKey);
    if (apiKey.empty()) {
        err = L"[" + xg_ai_provider + L"] Environment variable " + info.envKey + L" is not set.";
        return false;
    }

    std::wstring body = L"{\"model\":\"" + EscapeJson(model) + L"\",\"messages\":" + BuildOpenAIMessagesJson(hist) + L"}";
    std::string bodyUtf8 = WideToUtf8(body);

    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer " + apiKey + L"\r\n";

    std::string resp;
    DWORD status = 0;
    if (!HttpPost(info.host, INTERNET_DEFAULT_HTTPS_PORT, info.path, headers, bodyUtf8, resp, status, err)) {
        if (err.empty()) err = L"HTTP " + std::to_wstring(status);
        // エラー本文からメッセージを取れるだけ取る
        std::string msg;
        if (ExtractJsonString(resp, "message", msg) || ExtractJsonString(resp, "error", msg))
            err += L" / " + Utf8ToWide(msg);
        return false;
    }

    std::string content;
    // choices[0].message.content
    size_t p = resp.find("\"content\"");
    if (p != std::string::npos && ExtractJsonString(resp.substr(p), "content", content)) {
        answer = Utf8ToWide(content);
        return true;
    }
    err = L"Failed to parse response";
    return false;
}

static bool AskClaude(const ProviderInfo& info, const std::wstring& model,
                      const std::vector<std::pair<std::wstring, std::wstring>>& hist,
                      std::wstring& answer, std::wstring& err)
{
    std::wstring apiKey = GetEnv(info.envKey);
    if (apiKey.empty()) {
        err = L"[claude] Environment variable ANTHROPIC_API_KEY is not set.";
        return false;
    }

    std::wstring body = L"{\"model\":\"" + EscapeJson(model) + L"\",\"max_tokens\":1024,\"messages\":" + BuildOpenAIMessagesJson(hist) + L"}";
    std::string bodyUtf8 = WideToUtf8(body);

    std::wstring headers = L"Content-Type: application/json\r\n"
                           L"x-api-key: " + apiKey + L"\r\n"
                           L"anthropic-version: 2023-06-01\r\n";

    std::string resp;
    DWORD status = 0;
    if (!HttpPost(info.host, INTERNET_DEFAULT_HTTPS_PORT, info.path, headers, bodyUtf8, resp, status, err)) {
        if (err.empty()) err = L"HTTP " + std::to_wstring(status);
        std::string msg;
        if (ExtractJsonString(resp, "message", msg) || ExtractJsonString(resp, "error", msg))
            err += L" / " + Utf8ToWide(msg);
        return false;
    }

    // content[0].text
    size_t p = resp.find("\"text\"");
    if (p != std::string::npos) {
        std::string text;
        if (ExtractJsonString(resp.substr(p), "text", text)) {
            answer = Utf8ToWide(text);
            return true;
        }
    }
    err = L"Failed to parse Claude response";
    return false;
}

static bool AskGemini(const ProviderInfo& info, const std::wstring& model,
                      const std::vector<std::pair<std::wstring, std::wstring>>& hist,
                      std::wstring& answer, std::wstring& err)
{
    std::wstring apiKey = GetEnv(info.envKey);
    if (apiKey.empty()) {
        // フォールバック
        apiKey = GetEnv(L"GEMINI_API_KEY");
    }
    if (apiKey.empty()) {
        err = L"[gemini] Environment variable GOOGLE_API_KEY (or GEMINI_API_KEY) is not set.";
        return false;
    }

    // contents 配列を組み立て（role: user / model）
    std::wstring contents = L"[";
    bool first = true;
    for (const auto& m : hist) {
        if (!first) contents += L",";
        first = false;
        std::wstring role = (m.first == L"assistant") ? L"model" : L"user";
        contents += L"{\"role\":\"" + role + L"\",\"parts\":[{\"text\":\"" + EscapeJson(m.second) + L"\"}]}";
    }
    contents += L"]";

    std::wstring body = L"{\"contents\":" + contents + L"}";
    std::string bodyUtf8 = WideToUtf8(body);

    std::wstring path = L"/v1beta/models/" + model + L":generateContent?key=" + apiKey;
    std::wstring headers = L"Content-Type: application/json\r\n";

    std::string resp;
    DWORD status = 0;
    if (!HttpPost(info.host, INTERNET_DEFAULT_HTTPS_PORT, path, headers, bodyUtf8, resp, status, err)) {
        if (err.empty()) err = L"HTTP " + std::to_wstring(status);
        std::string msg;
        if (ExtractJsonString(resp, "message", msg) || ExtractJsonString(resp, "error", msg))
            err += L" / " + Utf8ToWide(msg);
        return false;
    }

    // candidates[0].content.parts[0].text
    size_t p = resp.find("\"text\"");
    if (p != std::string::npos) {
        std::string text;
        if (ExtractJsonString(resp.substr(p), "text", text)) {
            answer = Utf8ToWide(text);
            return true;
        }
    }
    err = L"Failed to parse Gemini response";
    return false;
}

// ---------------------------------------------------------------------------
// 共通 Ask
// ---------------------------------------------------------------------------
static bool AskProvider(const std::wstring& provider, const std::wstring& model,
                        const std::wstring& userMessage, std::wstring& answer, std::wstring& err)
{
    auto it = g_providers.find(provider);
    if (it == g_providers.end()) {
        err = L"Unknown provider: " + provider;
        return false;
    }
    const ProviderInfo& info = it->second;

    auto& hist = g_histories[provider];
    hist.emplace_back(L"user", userMessage);

    bool ok = false;
    if (info.isOpenAICompat)
        ok = AskOpenAICompat(info, model, hist, answer, err);
    else if (info.isClaude)
        ok = AskClaude(info, model, hist, answer, err);
    else if (info.isGemini)
        ok = AskGemini(info, model, hist, answer, err);

    if (ok) {
        hist.emplace_back(L"assistant", answer);
    } else {
        // 失敗したら直前のuserを戻す
        if (!hist.empty() && hist.back().first == L"user")
            hist.pop_back();
    }
    return ok;
}

// ---------------------------------------------------------------------------
// UI通知（既存のWM_APP_AI_LINEを利用）
// ---------------------------------------------------------------------------
static void PostLine(HWND hwnd, const std::wstring& line)
{
    if (!hwnd || !IsWindow(hwnd)) return;
    size_t len = line.size() + 1;
    PWSTR p = new wchar_t[len];
    StringCchCopyW(p, len, line.c_str());
    if (!PostMessageW(hwnd, WM_APP_AI_LINE, 0, (LPARAM)p)) {
        delete[] p;
    }
}

// ---------------------------------------------------------------------------
// ワーカースレッド
// ---------------------------------------------------------------------------
struct AskParam {
    HWND hwnd;
    std::wstring provider;
    std::wstring model;
    std::wstring message;
};

static DWORD WINAPI AskWorkerProc(LPVOID lp)
{
    std::unique_ptr<AskParam> param((AskParam*)lp);
    std::wstring answer, err;

    bool ok = AskProvider(param->provider, param->model, param->message, answer, err);
    if (ok) {
        // 回答を1行ずつ投げる（既存の表示ロジックに合わせる）
        size_t start = 0;
        while (start < answer.size()) {
            size_t end = answer.find(L'\n', start);
            if (end == std::wstring::npos) end = answer.size();
            std::wstring line = answer.substr(start, end - start);
            // 末尾の\rを除去
            if (!line.empty() && line.back() == L'\r') line.pop_back();
            PostLine(param->hwnd, line);
            start = end + 1;
        }
    } else {
        PostLine(param->hwnd, err);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 公開インターフェース（AIHelper.cpp から呼ばれる）
// ---------------------------------------------------------------------------

// プロセスの代わりに内部状態を初期化する
BOOL Helper2_Start(HWND hwnd)
{
    g_hwndNotify = hwnd;
    g_bStop = FALSE;
    g_histories.clear();

    // ※ xg_hReadyEvent の SetEvent は呼び出し側（Helper_StartAIProcess）で行う

    // 初期質問があれば投げる
    if (!xg_initial_question.empty()) {
        auto* p = new AskParam{ hwnd, xg_ai_provider, xg_ai_model, xg_initial_question };
        HANDLE h = CreateThread(nullptr, 0, AskWorkerProc, p, 0, nullptr);
        if (h) CloseHandle(h);
    }
    return TRUE;
}

void Helper2_Stop()
{
    g_bStop = TRUE;
    g_hwndNotify = nullptr;
    // 履歴は残しても良いが、明示的にクリア
    g_histories.clear();
}

void Helper2_Ask(HWND hwnd, const std::wstring& text)
{
    if (text.empty()) return;

    auto* p = new AskParam{ hwnd, xg_ai_provider, xg_ai_model, text };
    HANDLE h = CreateThread(nullptr, 0, AskWorkerProc, p, 0, nullptr);
    if (h) CloseHandle(h);
    else {
        PostLine(hwnd, L"[Error] Failed to create worker thread.");
        delete p;
    }
}

void Helper2_ResetHistory()
{
    g_histories[xg_ai_provider].clear();
}
