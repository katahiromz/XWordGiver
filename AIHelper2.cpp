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
#include <cstdio>
#include "AIHelper.h"
#include "AIModelsDat.h"
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
static CRITICAL_SECTION g_csHistory;
static BOOL   g_bCsInitialized = FALSE;

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

// HTTPステータスコードを一言で説明する
static std::wstring HttpStatusDescription(DWORD status)
{
	bool jp = XgIsUserJapanese();
	switch (status) {
	case 400: return jp ? L"リクエスト内容が不正です。" : L"Bad request. ";
	case 401: return jp ? L"APIキーが無効か認証に失敗しました。" : L"Invalid API key or authentication failed. ";
	case 403: return jp ? L"アクセスが拒否されました（権限不足の可能性）。" : L"Access denied (possibly insufficient permissions). ";
	case 404: return jp ? L"モデルまたはエンドポイントが見つかりません。" : L"Model or endpoint not found. ";
	case 408: return jp ? L"リクエストがタイムアウトしました。" : L"Request timed out. ";
	case 429: return jp ? L"レート制限またはクォータを超過しました。" : L"Rate limit or quota exceeded. ";
	case 500: case 502: case 503: case 504:
		return jp ? L"サーバー側で一時的なエラーが発生しています。" : L"Temporary server-side error. ";
	default:  return L"";
	}
}

// "retryDelay":"5.5s" のような値を拾う（Gemini等）
static bool ExtractRetryDelay(const std::string& json, std::wstring& delayOut)
{
	std::string val;
	if (!ExtractJsonString(json, "retryDelay", val)) return false;
	delayOut = Utf8ToWide(val);
	return true;
}

// APIキー等の機微情報が混じらないよう、メッセージ中のURLや改行以降を軽く整理する
static std::wstring TrimApiMessage(const std::wstring& raw)
{
	std::wstring msg = raw;
	// 1行目だけを採用（改行以降は詳細情報が続くことが多いため）
	size_t nl = msg.find_first_of(L"\r\n");
	if (nl != std::wstring::npos)
		msg = msg.substr(0, nl);
	// 文中に埋め込まれたURL（"For more information..." 等）以降は冗長なので削る
	size_t urlPos = msg.find(L"http://");
	size_t urlPos2 = msg.find(L"https://");
	size_t cut = std::wstring::npos;
	if (urlPos != std::wstring::npos) cut = urlPos;
	if (urlPos2 != std::wstring::npos && (cut == std::wstring::npos || urlPos2 < cut)) cut = urlPos2;
	if (cut != std::wstring::npos) {
		// URL直前にある "head to:" 等の接続句も一緒に削る
		size_t trimTo = msg.find_last_of(L".", cut);
		msg = (trimTo != std::wstring::npos) ? msg.substr(0, trimTo + 1) : msg.substr(0, cut);
	}
	// 前後の空白を除去
	size_t s = msg.find_first_not_of(L" \t");
	size_t e = msg.find_last_not_of(L" \t");
	if (s == std::wstring::npos) return L"";
	return msg.substr(s, e - s + 1);
}

// HTTPエラー応答全体を、ユーザーに見せやすい形に整形する
static std::wstring FormatApiError(const std::wstring& provider, DWORD status, const std::string& resp)
{
	bool jp = XgIsUserJapanese();
	std::wstring out = L"[" + provider + L"] " + (jp ? L"エラー" : L"Error") +
						L" (HTTP " + std::to_wstring(status) + L")";

	std::wstring desc = HttpStatusDescription(status);
	if (!desc.empty())
		out += L" — " + desc;

	std::string msg;
	if (ExtractJsonString(resp, "message", msg) && !msg.empty()) {
		std::wstring wmsg = TrimApiMessage(Utf8ToWide(msg));
		if (!wmsg.empty())
			out += L"\n" + wmsg;
	}

	std::wstring retryDelay;
	if (ExtractRetryDelay(resp, retryDelay)) {
		out += jp ? (L"\n→ " + retryDelay + L" 後に再試行してください。")
				  : (L"\n→ Please retry after " + retryDelay + L".");
	} else if (status == 429) {
		out += jp ? L"\n→ しばらく時間をおいてから再試行してください。"
				  : L"\n→ Please wait a while and try again.";
	} else if (status == 401 || status == 403) {
		out += jp ? L"\n→ APIキーや利用権限を確認してください。"
				  : L"\n→ Please check your API key and permissions.";
	}

	return out;
}

// ---------------------------------------------------------------------------
// プロバイダ設定
// ---------------------------------------------------------------------------
struct ProviderInfo {
	std::wstring envKey;      // 空文字列ならAPIキー不要（ローカルAI等） / empty = no API key needed (local AI, etc.)
	std::wstring host;		  // 例: api.openai.com
	INTERNET_PORT port = INTERNET_DEFAULT_HTTPS_PORT;
	std::wstring path;		  // 例: /v1/chat/completions
	bool isOpenAICompat = false;
	bool isClaude = false;
	bool isGemini = false;
	bool useHttps = true;    // false ならプレーンHTTP接続（ローカルAI等）/ false = plain HTTP (local AI, etc.)
};

// プロバイダー接続設定。以前はここに直接書いていたが、外部ファイル AIModels.dat の
// [PROVIDER_INFO] セクションから LoadAIModelsData() で読み込むようになった。
// Provider connection settings. This used to be a hardcoded literal here; it is now
// loaded by LoadAIModelsData() from the [PROVIDER_INFO] section of the external
// file AIModels.dat.
static std::map<std::wstring, ProviderInfo> g_providers;

// ---------------------------------------------------------------------------
// AIModels.dat 読み込み
// Load AIModels.dat
// ---------------------------------------------------------------------------
// ファイル探索・読み込みと (セクション名, 項目行) へのパースは AIModelsDat.h/.cpp に
// 共通化されている（AIHelper.cpp の LoadKnownAIModels() / LoadKnownAIProviders() と共有）。
// File lookup/reading and parsing into (section, line) pairs are shared via
// AIModelsDat.h/.cpp (used together with LoadKnownAIModels() / LoadKnownAIProviders()
// in AIHelper.cpp).

// AIModels.dat の [PROVIDER_INFO] セクションから g_providers を構築する。
// 書式: provider=APIキー環境変数名,ホスト,ポート,パス,OpenAI互換か,Claude形式か,Gemini形式か,HTTPSか
// （APIキー環境変数名を空にすると、そのプロバイダーはAPIキー不要として扱われる＝ローカルAI等向け）
// プロバイダー一覧の表示順もこのセクションの出現順から取る。
// 他のセクション（MODELS:*）は Python版や AIHelper.cpp 側で使うものなので、
// ここでは読み飛ばす。なお Python版も [PROVIDER_INFO] を読み、isOpenAICompat=1 の
// 行から OpenAI 互換の base_url を組み立てる。
//
// Build g_providers from the [PROVIDER_INFO] section of AIModels.dat.
// Format: provider=api_key_env,host,port,path,isOpenAICompat,isClaude,isGemini,useHttps
// (an empty api_key_env means no API key is required -- for local AI, etc.)
// Provider display order is also taken from the appearance order in this
// section. The other sections // (MODELS:*) are used by the Python build or by
// AIHelper.cpp, so they are skipped here. The Python build also reads
// [PROVIDER_INFO] and derives OpenAI-compatible base_url from rows with
// isOpenAICompat=1.
BOOL LoadAIModelsData()
{
	g_providers.clear();

	for (const auto& entry : GetAIModelsDatLines())
	{
		const std::wstring& section = entry.first;
		const std::wstring& line = entry.second;

		if (section != L"PROVIDER_INFO")
			continue;

		size_t eq = line.find(L'=');
		if (eq == std::wstring::npos)
			continue;

		// '=' 前後の空白は許容する（provider 名・値側の先頭空白を除去）
		// Allow whitespace around '=' (trim provider name and the value side).
		std::wstring provider = TrimW(line.substr(0, eq));
		if (provider.empty())
			continue;

		auto fields = SplitCsvLine(line.substr(eq + 1));
		if (fields.size() < 8)
			continue;

		ProviderInfo info;
		info.envKey         = fields[0]; // 空文字列ならAPIキー不要
		info.host           = fields[1];
		info.port           = (INTERNET_PORT)_wtoi(fields[2].c_str());
		info.path           = fields[3];
		info.isOpenAICompat = (fields[4] == L"1");
		info.isClaude       = (fields[5] == L"1");
		info.isGemini       = (fields[6] == L"1");
		info.useHttps       = (fields[7] == L"1");
		g_providers[provider] = info;
	}

	return !g_providers.empty();
}

// ---------------------------------------------------------------------------
// WinHTTP 共通送信
// ---------------------------------------------------------------------------
static bool HttpPost(const std::wstring& host, INTERNET_PORT port, const std::wstring& path,
					 const std::wstring& headers, const std::string& body,
					 std::string& response, DWORD& statusCode, std::wstring& errMsg,
					 bool useHttps = true)
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
								  useHttps ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest) {
		errMsg = L"WinHttpOpenRequest failed (" + std::to_wstring(GetLastError()) + L")";
		goto cleanup;
	}

	// セキュリティプロトコルを明示（古い環境対策）。HTTPS接続の場合のみ設定する
	// （ローカルAI等のプレーンHTTP接続には不要かつ無意味なため）。
	if (useHttps) {
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
	// APIキー環境変数名が空の場合は、ローカルAI等キー不要のプロバイダーとして扱う。
	// If the API key env var name is empty, treat this as a key-less provider
	// (local AI, etc.) and skip the Authorization header.
	std::wstring apiKey;
	if (!info.envKey.empty()) {
		apiKey = GetEnv(info.envKey.c_str());
		if (apiKey.empty()) {
			if (XgIsUserJapanese())
				err = L"[" + xg_ai_provider + L"] 環境変数 " + info.envKey + L" がセットされていません。";
			else
				err = L"[" + xg_ai_provider + L"] Environment variable " + info.envKey + L" is not set.";
			return false;
		}
	}

	std::wstring body = L"{\"model\":\"" + EscapeJson(model) + L"\",\"messages\":" + BuildOpenAIMessagesJson(hist) + L"}";
	std::string bodyUtf8 = WideToUtf8(body);

	std::wstring headers = L"Content-Type: application/json\r\n";
	if (!apiKey.empty())
		headers += L"Authorization: Bearer " + apiKey + L"\r\n";

	std::string resp;
	DWORD status = 0;
	if (!HttpPost(info.host, info.port, info.path, headers, bodyUtf8, resp, status, err, info.useHttps)) {
		err = FormatApiError(xg_ai_provider, status, resp);
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
	std::wstring apiKey = GetEnv(info.envKey.c_str());
	if (apiKey.empty()) {
		if (XgIsUserJapanese())
			err = L"[claude] 環境変数 ANTHROPIC_API_KEY がセットされていません。";
		else
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
	if (!HttpPost(info.host, info.port, info.path, headers, bodyUtf8, resp, status, err, info.useHttps)) {
		err = FormatApiError(L"anthropic", status, resp);
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
	std::wstring apiKey = GetEnv(info.envKey.c_str());
	if (apiKey.empty()) {
		if (XgIsUserJapanese())
			err = L"[gemini] 環境変数 GOOGLE_API_KEY がセットされていません。";
		else
			err = L"[gemini] Environment variable GOOGLE_API_KEY is not set.";
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
	if (!HttpPost(info.host, info.port, path, headers, bodyUtf8, resp, status, err, info.useHttps)) {
		err = FormatApiError(L"google", status, resp);
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
	if (g_bStop)
		return false;

	EnterCriticalSection(&g_csHistory);
	// ロック中に再度チェック（Stop が先に走った場合）
	if (g_bStop) {
		LeaveCriticalSection(&g_csHistory);
		return false;
	}

	// 念のため、まだ読み込まれていなければここでも読み込む。
	// Load it here too, just in case it hasn't been loaded yet.
	LoadAIModelsData();

	auto it = g_providers.find(provider);
	if (it == g_providers.end()) {
		err = L"Unknown provider: " + provider;
		LeaveCriticalSection(&g_csHistory);
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
	LeaveCriticalSection(&g_csHistory);
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
	if (!g_bCsInitialized) {
		InitializeCriticalSection(&g_csHistory);
		g_bCsInitialized = TRUE;
	}

	// プロバイダー接続設定を AIModels.dat から読み込む（未読み込みの場合のみ）。
	// Load provider connection settings from AIModels.dat (only if not loaded yet).
	if (g_providers.empty())
		LoadAIModelsData();

	g_hwndNotify = hwnd;
	g_bStop = FALSE;

	EnterCriticalSection(&g_csHistory);
	g_histories.clear();
	LeaveCriticalSection(&g_csHistory);

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

	if (g_bCsInitialized) {
		EnterCriticalSection(&g_csHistory);
		g_histories.clear();
		LeaveCriticalSection(&g_csHistory);
		// 注: DeleteCriticalSection はプロセス終了時に任せる（多重 Start/Stop に耐えるため）
	}
}

void Helper2_Ask(HWND hwnd, const std::wstring& text)
{
	if (text.empty()) return;

	auto* p = new AskParam{ hwnd, xg_ai_provider, xg_ai_model, text };
	HANDLE h = CreateThread(nullptr, 0, AskWorkerProc, p, 0, nullptr);
	if (h) CloseHandle(h);
	else {
		if (XgIsUserJapanese())
			PostLine(hwnd, L"[エラー] ワーカースレッドの作成に失敗しました。");
		else
			PostLine(hwnd, L"[Error] Failed to create worker thread.");
		delete p;
	}
}

void Helper2_ResetHistory()
{
	if (!g_bCsInitialized)
		return;
	EnterCriticalSection(&g_csHistory);
	g_histories[xg_ai_provider].clear();
	LeaveCriticalSection(&g_csHistory);
}
