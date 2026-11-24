// MAIHelper.cpp --- XWordGiver AI Helper (pure C++ / WinHTTP, optional GUI)
// Author: katahiromz + Grok
// License: MIT
#include "xwg.h"
#include "MMainWnd.h"
#include <windowsx.h>
#include <winhttp.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <imm.h>
#include <cassert>
#include <memory>
#include <strsafe.h>
#include "AIModelsDat.h"
#include "resource.h"
#include "MAIHelper.h"

#ifdef _MSC_VER
	#pragma comment(lib, "winhttp.lib")
#endif

using ProviderInfo = MAIHelper::ProviderInfo;
typedef std::vector<std::pair<MStringW, MStringW>> History; // [(role, content)]

// ---------------------------------------------------------------------------
// ワーカースレッドと共有する状態。
// MAIHelperが Stop / 破棄されてもワーカーが安全に終われるよう、shared_ptrで共有する。
// ---------------------------------------------------------------------------
namespace {
class CsLock {
	CRITICAL_SECTION& m_cs;
public:
	explicit CsLock(CRITICAL_SECTION& cs) : m_cs(cs) { EnterCriticalSection(&m_cs); }
	~CsLock() { LeaveCriticalSection(&m_cs); }
	CsLock(const CsLock&) = delete;
	CsLock& operator=(const CsLock&) = delete;
};
} // namespace

struct MAIHelper::Session {
	CRITICAL_SECTION histCs;    // hist の保護
	CRITICAL_SECTION askCs;     // 質問を1つずつ順番に処理するため（履歴の順序を保つ）
	volatile LONG stop = 0;
	volatile LONG pending = 0;  // 未完了の質問(ワーカー)の数
	HWND hwnd = nullptr;        // 出力行の通知先（nullptrならcbへ直接渡す）
	MAIHelper::LineCallback cb;
	std::unordered_map<MStringW, History> hist; // provider -> history

	Session() {
		InitializeCriticalSection(&histCs);
		InitializeCriticalSection(&askCs);
	}
	~Session() {
		DeleteCriticalSection(&histCs);
		DeleteCriticalSection(&askCs);
	}
};

// ---------------------------------------------------------------------------
// ユーティリティ
// ---------------------------------------------------------------------------

static MStringW EscapeJson(const MStringW& src) {
	MStringW out;
	out.reserve(src.size() + 16);
	for (size_t i = 0; i < src.size(); ++i) {
		wchar_t c = src[i];
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

static MStringW GetEnv(PCWSTR name) {
	wchar_t buf[4096];
	DWORD n = GetEnvironmentVariableW(name, buf, _countof(buf));
	if (n == 0 || n >= _countof(buf))
		return MStringW();
	return MStringW(buf);
}

static int HexValue(wchar_t ch) {
	if (L'0' <= ch && ch <= L'9') return ch - L'0';
	if (L'a' <= ch && ch <= L'f') return ch - L'a' + 10;
	if (L'A' <= ch && ch <= L'F') return ch - L'A' + 10;
	return -1;
}

// 簡易JSON抽出（"key": "value" 形式の文字列値を取る）。
// \uXXXX（サロゲートペア含む）を正しく復号する。値が文字列でない(null等)箇所は読み飛ばす。
static bool JsonFindString(const MStringW& json, PCWSTR key, MStringW& out) {
	MStringW pat = L"\"";
	pat += key;
	pat += L"\"";

	size_t pos = 0;
	for (;;) {
		pos = json.find(pat, pos);
		if (pos == MStringW::npos)
			return false;
		pos += pat.size();

		size_t i = pos;
		while (i < json.size() && iswspace(json[i])) ++i;
		if (i >= json.size() || json[i] != L':') continue;
		++i;
		while (i < json.size() && iswspace(json[i])) ++i;
		if (i >= json.size() || json[i] != L'"') continue;
		++i;

		MStringW val;
		bool closed = false;
		while (i < json.size()) {
			wchar_t c = json[i++];
			if (c == L'\\' && i < json.size()) {
				wchar_t n = json[i++];
				switch (n) {
				case L'n': val += L'\n'; break;
				case L'r': val += L'\r'; break;
				case L't': val += L'\t'; break;
				case L'b': val += L'\b'; break;
				case L'f': val += L'\f'; break;
				case L'u': {
					int v = 0;
					bool ok = (i + 4 <= json.size());
					for (size_t k = 0; ok && k < 4; ++k) {
						int h = HexValue(json[i + k]);
						if (h < 0) ok = false; else v = v * 16 + h;
					}
					if (ok) { val += (wchar_t)v; i += 4; } // UTF-16なのでサロゲートもそのまま並べればよい
					else val += L'u';
					break;
				}
				default: val += n; break; // " \ / など
				}
			} else if (c == L'"') {
				closed = true;
				break;
			} else {
				val += c;
			}
		}
		if (!closed)
			return false;
		out = val;
		return true;
	}
}

// 回答の前後に付く (*...*) 形式のタグ（事前情報など）をすべて取り除く。表示前のフィルタリング用。
// また、markdownで太字を表す「**」（ダブル アスタリスク）を取り除く。
static MStringW StripAiPreTextTag(PCWSTR pszLine) {
	MStringW result;
	const wchar_t *pch = pszLine;
	while (*pch) {
		if (pch[0] == L'(' && pch[1] == L'*') {
			const wchar_t *pEnd = wcsstr(pch + 2, L"*)");
			if (pEnd) {
				pch = pEnd + 2;
				continue;
			}
		}
		result += *pch;
		++pch;
	}
	XgStrReplace(result, L"**", L"");
	return result;
}

// HTTPステータスコードを一言で説明する
static MStringW HttpStatusDescription(DWORD status) {
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
	default:  return MStringW();
	}
}

// エラーメッセージを整理する（1行目だけ採用し、埋め込まれたURL以降を削る）
static MStringW TrimApiMessage(const MStringW& raw) {
	MStringW msg = raw;
	size_t nl = msg.find_first_of(L"\r\n");
	if (nl != MStringW::npos)
		msg = msg.substr(0, nl);

	size_t urlPos = msg.find(L"http://");
	size_t urlPos2 = msg.find(L"https://");
	size_t cut = MStringW::npos;
	if (urlPos != MStringW::npos) cut = urlPos;
	if (urlPos2 != MStringW::npos && (cut == MStringW::npos || urlPos2 < cut)) cut = urlPos2;
	if (cut != MStringW::npos) {
		size_t trimTo = msg.find_last_of(L".", cut);
		msg = (trimTo != MStringW::npos) ? msg.substr(0, trimTo + 1) : msg.substr(0, cut);
	}
	return TrimW(msg);
}

// HTTPエラー応答全体を、ユーザーに見せやすい形に整形する
static MStringW FormatApiError(const MStringW& provider, DWORD status, const MStringW& resp) {
	bool jp = XgIsUserJapanese();
	MStringW out = L"[";
	out += provider;
	out += L"] ";
	out += jp ? L"エラー" : L"Error";
	out += L" (HTTP ";
	out += to_mstrW(status);
	out += L")";

	MStringW desc = HttpStatusDescription(status);
	if (!desc.empty()) {
		out += L" — ";
		out += desc;
	}

	MStringW msg;
	if (JsonFindString(resp, L"message", msg) && !msg.empty()) {
		MStringW wmsg = TrimApiMessage(msg);
		if (!wmsg.empty()) {
			out += L"\n";
			out += wmsg;
		}
	}

	MStringW retryDelay; // "retryDelay":"5.5s" のような値（Gemini等）
	if (JsonFindString(resp, L"retryDelay", retryDelay)) {
		if (jp) {
			out += L"\n→ ";
			out += retryDelay;
			out += L" 後に再試行してください。";
		} else {
			out += L"\n→ Please retry after ";
			out += retryDelay;
			out += L".";
		}
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
// WinHTTP 共通送信
// ---------------------------------------------------------------------------
static bool HttpPost(const MStringW& host, WORD port, const MStringW& path,
					 const MStringW& headers, const MStringA& body,
					 MStringA& response, DWORD& statusCode, MStringW& errMsg, bool useHttps)
{
	HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;
	bool success = false;
	statusCode = 0;
	response.clear();

	hSession = WinHttpOpen(L"XWordGiver-AIHelper/3.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
						   WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) {
		errMsg = L"WinHttpOpen failed (";
		errMsg += to_mstrW(GetLastError());
		errMsg += L")";
		goto cleanup;
	}

	// resolve / connect / send / receive （ミリ秒）
	WinHttpSetTimeouts(hSession, 15000, 15000, 30000, 60000);

	hConnect = WinHttpConnect(hSession, host.c_str(), (INTERNET_PORT)port, 0);
	if (!hConnect) {
		errMsg = L"WinHttpConnect failed (";
		errMsg += to_mstrW(GetLastError());
		errMsg += L")";
		goto cleanup;
	}

	hRequest = WinHttpOpenRequest(hConnect, L"POST", path.c_str(), nullptr, WINHTTP_NO_REFERER,
								  WINHTTP_DEFAULT_ACCEPT_TYPES, useHttps ? WINHTTP_FLAG_SECURE : 0);
	if (!hRequest) {
		errMsg = L"WinHttpOpenRequest failed (";
		errMsg += to_mstrW(GetLastError());
		errMsg += L")";
		goto cleanup;
	}

	// セキュリティプロトコルを明示（古い環境対策）。HTTPSのときだけ設定する。
	if (useHttps) {
		DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#if defined(WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3)
						| WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#endif
						;
		WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
	}

	if (!WinHttpSendRequest(hRequest, headers.c_str(), (DWORD)-1L,
							(LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0)) {
		errMsg = L"WinHttpSendRequest failed (";
		errMsg += to_mstrW(GetLastError());
		errMsg += L")";
		goto cleanup;
	}

	if (!WinHttpReceiveResponse(hRequest, nullptr)) {
		DWORD err = GetLastError();
		errMsg = L"WinHttpReceiveResponse failed (";
		errMsg += to_mstrW(err);
		errMsg += L")";
		if (err == ERROR_WINHTTP_TIMEOUT)
			errMsg += L" [timeout]";
		else if (err == ERROR_WINHTTP_SECURE_FAILURE)
			errMsg += L" [TLS/secure failure]";
		else if (err == ERROR_WINHTTP_CONNECTION_ERROR)
			errMsg += L" [connection error]";
		goto cleanup;
	}

	{
		DWORD dwStatus = 0, dwSize = sizeof(dwStatus);
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
							WINHTTP_HEADER_NAME_BY_INDEX, &dwStatus, &dwSize, WINHTTP_NO_HEADER_INDEX);
		statusCode = dwStatus;
	}

	for (;;) {
		DWORD dwAvail = 0;
		if (!WinHttpQueryDataAvailable(hRequest, &dwAvail) || dwAvail == 0)
			break;
		std::vector<char> buf(dwAvail);
		DWORD dwRead = 0;
		if (!WinHttpReadData(hRequest, buf.data(), dwAvail, &dwRead) || dwRead == 0)
			break;
		response.append(buf.data(), dwRead);
	}

	success = (statusCode >= 200 && statusCode < 300);

cleanup:
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);
	return success;
}

// 通信エラー時のメッセージを作る。HTTPステータスが得られていれば応答本文から整形し、
// 得られていなければ（接続失敗など）HttpPostが残したメッセージを使う。
static MStringW MakeHttpError(const MStringW& provider, DWORD status, const MStringA& resp, const MStringW& lowLevel)
{
	if (status == 0) {
		MStringW out = L"[";
		out += provider;
		out += L"] ";
		out += lowLevel;
		return out;
	}
	return FormatApiError(provider, status, WideFromUtf8(resp));
}

// ---------------------------------------------------------------------------
// 各プロバイダのリクエスト構築・応答抽出
// ---------------------------------------------------------------------------

// APIキーを環境変数から取得する。required=falseで変数名が空なら「キー不要」として成功する。
static bool GetApiKey(const ProviderInfo& info, const MStringW& provider, bool required,
					  MStringW& key, MStringW& err)
{
	key.clear();
	bool jp = XgIsUserJapanese();
	if (info.envKey.empty()) {
		if (!required)
			return true;
		err = L"[";
		err += provider;
		err += jp ? L"] AIModels.dat にAPIキーの環境変数名が設定されていません。"
				  : L"] No API key environment variable is configured in AIModels.dat.";
		return false;
	}
	key = GetEnv(info.envKey.c_str());
	if (key.empty()) {
		err = L"[";
		err += provider;
		err += jp ? L"] 環境変数 " : L"] Environment variable ";
		err += info.envKey;
		err += jp ? L" がセットされていません。" : L" is not set.";
		return false;
	}
	return true;
}

static MStringW BuildMessagesJson(const History& hist) {
	MStringW j = L"[";
	bool first = true;
	for (const auto& m : hist) {
		if (!first) j += L",";
		first = false;
		j += L"{\"role\":\"";
		j += EscapeJson(m.first);
		j += L"\",\"content\":\"";
		j += EscapeJson(m.second);
		j += L"\"}";
	}
	j += L"]";
	return j;
}

// 送信して、応答本文(UTF-16)を得る共通処理。
static bool PostJson(const ProviderInfo& info, const MStringW& provider, const MStringW& path,
					 const MStringW& headers, const MStringW& body, MStringW& wresp, MStringW& err)
{
	MStringA resp;
	DWORD status = 0;
	MStringW lowLevel;
	if (!HttpPost(info.host, info.port, path, headers, Utf8FromWide(body), resp, status, lowLevel, info.useHttps)) {
		if (status == 0 && lowLevel.empty())
			lowLevel = L"HTTP request failed";
		err = MakeHttpError(provider, status, resp, lowLevel);
		DPRINTW(L"MAIHelper: [%ls] request failed (HTTP %lu): %ls\n", provider.c_str(), (unsigned long)status, err.c_str());
		return false;
	}
	wresp = WideFromUtf8(resp);
	return true;
}

static bool AskOpenAICompat(const ProviderInfo& info, const MStringW& provider, const MStringW& model,
							const History& hist, MStringW& answer, MStringW& err)
{
	// APIキー環境変数名が空ならローカルAI等キー不要のプロバイダーとして扱う。
	MStringW apiKey;
	if (!GetApiKey(info, provider, false, apiKey, err))
		return false;

	MStringW body = L"{\"model\":\"";
	body += EscapeJson(model);
	body += L"\",\"messages\":";
	body += BuildMessagesJson(hist);
	body += L"}";

	MStringW headers = L"Content-Type: application/json\r\n";
	if (!apiKey.empty()) {
		headers += L"Authorization: Bearer ";
		headers += apiKey;
		headers += L"\r\n";
	}

	MStringW wresp;
	if (!PostJson(info, provider, info.path, headers, body, wresp, err))
		return false;

	if (JsonFindString(wresp, L"content", answer)) // choices[0].message.content
		return true;
	err = L"Failed to parse response";
	return false;
}

static bool AskClaude(const ProviderInfo& info, const MStringW& provider, const MStringW& model,
					  const History& hist, MStringW& answer, MStringW& err)
{
	MStringW apiKey;
	if (!GetApiKey(info, provider, true, apiKey, err))
		return false;

	MStringW body = L"{\"model\":\"";
	body += EscapeJson(model);
	body += L"\",\"max_tokens\":1024,\"messages\":";
	body += BuildMessagesJson(hist);
	body += L"}";

	MStringW headers = L"Content-Type: application/json\r\nx-api-key: ";
	headers += apiKey;
	headers += L"\r\nanthropic-version: 2023-06-01\r\n";

	MStringW wresp;
	if (!PostJson(info, provider, info.path, headers, body, wresp, err))
		return false;

	if (JsonFindString(wresp, L"text", answer)) // content[0].text
		return true;
	err = L"Failed to parse Claude response";
	return false;
}

static bool AskGemini(const ProviderInfo& info, const MStringW& provider, const MStringW& model,
					  const History& hist, MStringW& answer, MStringW& err)
{
	MStringW apiKey;
	if (!GetApiKey(info, provider, true, apiKey, err))
		return false;

	// contents 配列（role: user / model）
	MStringW body = L"{\"contents\":[";
	bool first = true;
	for (const auto& m : hist) {
		if (!first) body += L",";
		first = false;
		body += L"{\"role\":\"";
		body += (m.first == MStringW(L"assistant")) ? L"model" : L"user";
		body += L"\",\"parts\":[{\"text\":\"";
		body += EscapeJson(m.second);
		body += L"\"}]}";
	}
	body += L"]}";

	// APIキーはURLではなくヘッダーで渡す（URLはログやエラー文に残りやすいため）。
	MStringW path = L"/v1beta/models/";
	path += model;
	path += L":generateContent";
	MStringW headers = L"Content-Type: application/json\r\nx-goog-api-key: ";
	headers += apiKey;
	headers += L"\r\n";

	MStringW wresp;
	if (!PostJson(info, provider, path, headers, body, wresp, err))
		return false;

	if (JsonFindString(wresp, L"text", answer)) // candidates[0].content.parts[0].text
		return true;
	err = L"Failed to parse Gemini response";
	return false;
}

// 1回の質問を、履歴の順序を保ちながら実行する（呼び出しスレッドはブロックされる）。
// ロックは通信中は保持しない（Stopがブロックされないように）。
static bool PerformRequest(const std::shared_ptr<MAIHelper::Session>& s, const ProviderInfo& info,
						   const MStringW& provider, const MStringW& model, const MStringW& message,
						   MStringW& answer, MStringW& err)
{
	CsLock askLock(s->askCs);
	if (s->stop)
		return false;

	History snapshot;
	{
		CsLock lock(s->histCs);
		snapshot = s->hist[provider];
	}
	snapshot.push_back(std::make_pair(MStringW(L"user"), message));

	bool ok = false;
	if (info.isOpenAICompat) {
		ok = AskOpenAICompat(info, provider, model, snapshot, answer, err);
	} else if (info.isClaude) {
		ok = AskClaude(info, provider, model, snapshot, answer, err);
	} else if (info.isGemini) {
		ok = AskGemini(info, provider, model, snapshot, answer, err);
	} else {
		err = L"Unsupported provider type: ";
		err += provider;
	}

	if (ok && !s->stop) {
		CsLock lock(s->histCs);
		History& h = s->hist[provider];
		h.push_back(std::make_pair(MStringW(L"user"), message));
		h.push_back(std::make_pair(MStringW(L"assistant"), answer));
	}
	return ok;
}

// ---------------------------------------------------------------------------
// ワーカースレッド
// ---------------------------------------------------------------------------

// 1行を出力先へ渡す。ウィンドウがあればUIスレッドへポストし、なければコールバックへ直接渡す。
static void DeliverLine(const std::shared_ptr<MAIHelper::Session>& s, const MStringW& line, bool bEcho = false) {
	if (s->stop)
		return;
	if (s->hwnd) {
		if (!IsWindow(s->hwnd))
			return;
		size_t len = line.size() + 1;
		PWSTR p = new wchar_t[len];
		StringCchCopyW(p, len, line.c_str());
		if (!PostMessageW(s->hwnd, WM_APP_AI_LINE, bEcho ? 1 : 0, (LPARAM)p))
			delete[] p;
	} else if (s->cb) {
		MStringW filtered = StripAiPreTextTag(line.c_str());
		if (!filtered.empty())
			s->cb(filtered);
	}
}

struct AskJob {
	std::shared_ptr<MAIHelper::Session> session;
	ProviderInfo info;
	bool known = false;
	MStringW provider, model, message;
	MStringW echo;  // 順番が来たときに出力する質問のエコー行(空なら出力しない)
};

static DWORD WINAPI AskWorkerProc(LPVOID lp) {
	std::unique_ptr<AskJob> job((AskJob*)lp);
	MStringW answer, err;
	bool ok = false;

	// 質問の順番が来るまで待つ。エコー行の出力から回答の出力までをロックの中で行うことで、
	// 先行する質問(初期質問のあいさつ等)の出力より前にエコー行が出ないようにする。
	// (CRITICAL_SECTIONは再入可能なので、PerformRequest内の取得と衝突しない)
	struct PendingGuard {
		std::shared_ptr<MAIHelper::Session> s;
		~PendingGuard() { InterlockedDecrement(&s->pending); }
	} pendingGuard{ job->session };
	CsLock turnLock(job->session->askCs);

	if (!job->session->stop && !job->echo.empty())
		DeliverLine(job->session, job->echo, true);

	if (job->known) {
		ok = PerformRequest(job->session, job->info, job->provider, job->model, job->message, answer, err);
	} else {
		err = L"Unknown provider: ";
		err += job->provider;
	}

	if (job->session->stop)
		return 0;

	if (!ok) {
		if (!err.empty())
			DeliverLine(job->session, err);
		return 0;
	}

	// 回答を1行ずつ渡す
	size_t start = 0;
	while (start < answer.size()) {
		size_t end = answer.find(L'\n', start);
		if (end == MStringW::npos) end = answer.size();
		MStringW line = answer.substr(start, end - start);
		if (!line.empty() && line[line.size() - 1] == L'\r')
			line = line.substr(0, line.size() - 1);
		DeliverLine(job->session, line);
		start = end + 1;
	}
	return 0;
}

// ---------------------------------------------------------------------------
// MAIHelper: 基本
// ---------------------------------------------------------------------------

MAIHelper::MAIHelper(MMainWnd& hMainWnd)
	: m_hMainWnd(hMainWnd)
	, m_bSolved(hMainWnd.m_bSolved)
	, m_board(hMainWnd.m_board)
	, m_solution(hMainWnd.m_solution)
	, m_display(hMainWnd.m_display)
{
	m_hInst = GetModuleHandleW(nullptr);
	ResetPrompts(m_promptMapping);
}

MAIHelper::~MAIHelper() {
	Stop();
}

// プロバイダー接続設定を AIModels.dat から読み込む。
BOOL MAIHelper::LoadProviders() {
	m_providers.clear();

	for (const auto& entry : GetAIModelsDatLines()) {
		const MStringW& section = entry.first;
		const MStringW& line = entry.second;

		if (section != MStringW(L"PROVIDER_INFO"))
			continue;

		size_t eq = line.find(L'=');
		if (eq == MStringW::npos)
			continue;

		// '=' 前後の空白は許容する
		MStringW provider = TrimW(line.substr(0, eq));
		if (provider.empty())
			continue;

		MStringListW fields = SplitCsvLine(line.substr(eq + 1));
		if (fields.size() < 8)
			continue;

		ProviderInfo info;
		info.envKey         = fields[0]; // 空文字列ならAPIキー不要
		info.host           = fields[1];
		info.port           = (WORD)_wtoi(fields[2].c_str());
		info.path           = fields[3];
		info.isOpenAICompat = (fields[4] == MStringW(L"1"));
		info.isClaude       = (fields[5] == MStringW(L"1"));
		info.isGemini       = (fields[6] == MStringW(L"1"));
		info.useHttps       = (fields[7] == MStringW(L"1"));
		m_providers[provider] = info;
	}

	return !m_providers.empty();
}

BOOL MAIHelper::Start(HWND hwndNotify) {
	Stop();
	LoadProviders();

	m_session = std::make_shared<Session>();
	m_session->hwnd = hwndNotify;
	m_session->cb = m_lineCallback;
	m_outputBuffer.clear();

	// 初期質問があれば投げる。
	if (!m_initialQuestion.empty())
		Ask(m_initialQuestion);
	return TRUE;
}

void MAIHelper::Stop() {
	if (m_timerId) {
		KillTimer(nullptr, m_timerId);
		m_timerId = 0;
	}
	if (m_session) {
		InterlockedExchange(&m_session->stop, 1);
		{
			CsLock lock(m_session->histCs);
			m_session->hist.clear();
		}
		m_session.reset(); // 実行中のワーカーは自分のshared_ptrで状態を保持し、結果を捨てて終了する
	}
}

bool MAIHelper::IsRunning() const {
	return m_session && !m_session->stop;
}

void MAIHelper::ResetHistory() {
	if (!m_session)
		return;
	CsLock lock(m_session->histCs);
	m_session->hist[m_provider].clear();
}

void MAIHelper::Ask(const MStringW& text, const MStringW& echo) {
	if (text.empty())
		return;
	if (!m_session)
		Start(nullptr);

	std::unique_ptr<AskJob> job(new AskJob);
	job->session = m_session;
	job->provider = m_provider;
	job->model = m_model;
	job->message = text;
	job->echo = echo;
	auto it = m_providers.find(m_provider);
	if (it != m_providers.end()) {
		job->known = true;
		job->info = it->second;
	}

	InterlockedIncrement(&m_session->pending);
	HANDLE h = CreateThread(nullptr, 0, AskWorkerProc, job.get(), 0, nullptr);
	if (h) {
		job.release(); // ワーカーが所有する
		CloseHandle(h);
	} else {
		InterlockedDecrement(&m_session->pending);
		AddLine(XgIsUserJapanese() ? L"[エラー] ワーカースレッドの作成に失敗しました。"
								   : L"[Error] Failed to create worker thread.");
	}
}

bool MAIHelper::AskSync(const MStringW& text, MStringW& answer, MStringW& err) {
	answer.clear();
	err.clear();
	if (text.empty())
		return false;
	if (!m_session)
		Start(nullptr);

	auto it = m_providers.find(m_provider);
	if (it == m_providers.end()) {
		err = L"Unknown provider: ";
		err += m_provider;
		return false;
	}
	return PerformRequest(m_session, it->second, m_provider, m_model, text, answer, err);
}

// ---------------------------------------------------------------------------
// MAIHelper: 出力
// ---------------------------------------------------------------------------

// 既定の出力先：conuniでコンソールへ出力する（コンソール/リダイレクトの両方に対応）。
// ワーカースレッドから呼ばれるため、1行分を1回のPrintfWで出力する。
void MAIHelper::ConsoleLineCallback(const MStringW& line) {
	PrintfW(L"%ls\n", line.c_str());
}

void MAIHelper::AddLine(const MStringW& line) {
	MStringW filtered = StripAiPreTextTag(line.c_str());
	if (filtered.empty())
		return; // タグのみの行（プロンプト等）は表示しない

#ifndef MAIHELPER_NO_GUI
	if (m_hwnd)
		AppendToList(filtered);
#endif
	if (m_lineCallback)
		m_lineCallback(filtered);
}

void MAIHelper::OnLineMessage(LPARAM lParam, WPARAM wParam) {
	PWSTR psz = (PWSTR)lParam;
	if (!psz)
		return;
	MStringW line = psz;
	delete[] psz;

	if (!IsRunning())
		return; // Stop後に届いた古い行は捨てる

	AddLine(line);
	if (wParam)
		return; // 質問のエコー行はAIの出力ではないので、コマンド解析の対象にしない
	m_outputBuffer += line;
	ScheduleFlush(); // デバウンス：最後の行から300ms後にまとめてコマンドを実行する
}

static MAIHelper* s_pFlushOwner = nullptr;

void CALLBACK MAIHelper::FlushTimerProc(HWND, UINT, UINT_PTR idEvent, DWORD) {
	KillTimer(nullptr, idEvent);
	if (s_pFlushOwner && s_pFlushOwner->m_timerId == idEvent) {
		s_pFlushOwner->m_timerId = 0;
		s_pFlushOwner->FlushOutput();
	}
}

void MAIHelper::ScheduleFlush() {
	s_pFlushOwner = this;
	m_timerId = SetTimer(nullptr, m_timerId, 300, FlushTimerProc);
}

void MAIHelper::FlushOutput() {
	if (m_timerId) {
		KillTimer(nullptr, m_timerId);
		m_timerId = 0;
	}
	MStringW text = m_outputBuffer;
	m_outputBuffer.clear();
	if (!text.empty())
		ParseAndApplyCommands(text.c_str()); // テキストに含まれるシステムコマンドを実行する
}

// ---------------------------------------------------------------------------
// MAIHelper: AIModels.dat の情報
// ---------------------------------------------------------------------------

BOOL MAIHelper::GetAIModels(PCWSTR provider, MStringListW& models) {
	models.clear();
	m_knownModels.clear();

	for (const auto& entry : GetAIModelsDatLines()) {
		const MStringW& section = entry.first;
		// "MODELS:provider" セクションのみ処理する。
		if (section.substr(0, 7) == L"MODELS:")
			m_knownModels[section.substr(7)].push_back(entry.second);
	}

	for (const auto& entry : m_knownModels) {
		if (lstrcmpiW(provider, entry.first.c_str()) == 0) {
			models = entry.second;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL MAIHelper::GetAIProviders(MStringListW& providers) {
	m_knownProviders.clear();

	for (const auto& entry : GetAIModelsDatLines()) {
		if (entry.first != MStringW(L"PROVIDER_INFO"))
			continue;

		// "provider = fields..." からプロバイダー名を抽出する。
		size_t eq = entry.second.find(L'=');
		if (eq == MStringW::npos)
			continue;
		MStringW name = TrimW(entry.second.substr(0, eq));
		if (!name.empty())
			m_knownProviders.push_back(name);
	}

	providers = m_knownProviders;
	return !providers.empty();
}

// ---------------------------------------------------------------------------
// MAIHelper: プロンプト
// ---------------------------------------------------------------------------

MStringW MAIHelper::GetPromptString(PROMPT_TYPE iPrompt, const MStringW& clue_name) const {
	if ((size_t)iPrompt >= m_promptMapping.size())
		return MStringW();
	MStringW str = m_promptMapping[(size_t)iPrompt];

	XgStrReplace(str, MStringW(L"$(CLUE)"), clue_name);

	SYSTEMTIME st;
	GetLocalTime(&st);
	WCHAR text[64];
	StringCbPrintfW(text, sizeof(text), L"%04u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
	XgStrReplace(str, MStringW(L"$(TODAY)"), MStringW(text));

	return str;
}

void MAIHelper::SetPromptString(PROMPT_TYPE iPrompt, const MStringW& str) {
	assert((size_t)iPrompt < m_promptMapping.size());
	if ((size_t)iPrompt < m_promptMapping.size())
		m_promptMapping[(size_t)iPrompt] = str;
}

MStringW MAIHelper::GetClueName(INT nNumber, BOOL bDown) {
	MStringW name = (bDown ? L"D" : L"A");
	name += to_mstrW(nNumber);
	return name;
}

MStringW MAIHelper::MakeInitialQuestion() const {
	return GetPromptString(PROMPT_TYPE_INITIAL, MStringW());
}

MStringW MAIHelper::GetAIPreText() {
	MStringW str = L"(* ";
	str += GetAIStatus();
	str += L" *) ";
	return str;
}


// ---------------------------------------------------------------------------
// MAIHelper: プロンプト
// ---------------------------------------------------------------------------

// プロンプト名を取得する。
MStringW MAIHelper::GetPromptName(PROMPT_TYPE iPrompt) {
	if (XgIsUserJapanese()) {
		switch (iPrompt) {
		case PROMPT_TYPE_INITIAL: return L"初期プロンプト";
		case PROMPT_TYPE_GENERATE_CLUE: return L"このカギを再生成";
		case PROMPT_TYPE_GENERATE_CLUES_ALL: return L"すべてのカギを再生成";
		case PROMPT_TYPE_EXPLAIN_CLUE: return L"このカギを解説";
		case PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES: return L"すべてのカギの品質チェック";
		case PROMPT_TYPE_JUDGE_SEASON: return L"このクロスワードの季節を分析";
		case PROMPT_TYPE_JUDGE_THEME: return L"このクロスワードのテーマを分析";
		case PROMPT_TYPE_CUSTOM_PROMPT_1: return L"カスタム プロンプト 1";
		case PROMPT_TYPE_CUSTOM_PROMPT_2: return L"カスタム プロンプト 2";
		case PROMPT_TYPE_MAX: break;
		}
	} else {
		switch (iPrompt) {
		case PROMPT_TYPE_INITIAL: return L"Initial prompt";
		case PROMPT_TYPE_GENERATE_CLUE: return L"Re-generate this clue";
		case PROMPT_TYPE_GENERATE_CLUES_ALL: return L"Re-generate all the clues";
		case PROMPT_TYPE_EXPLAIN_CLUE: return L"Explain this clue";
		case PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES: return L"Check the quality of all clues";
		case PROMPT_TYPE_JUDGE_SEASON: return L"Analyze the season of this crossword";
		case PROMPT_TYPE_JUDGE_THEME: return L"Analyze the theme of this crossword";
		case PROMPT_TYPE_CUSTOM_PROMPT_1: return L"Custom Prompt 1";
		case PROMPT_TYPE_CUSTOM_PROMPT_2: return L"Custom Prompt 2";
		case PROMPT_TYPE_MAX: break;
		}
	}
	assert(0);
	return L"";
}

void MAIHelper::Reset() {
	m_provider = L"google";
	m_model = L"gemini-3.5-flash-lite";
	ResetPrompts(m_promptMapping);
}

// プロンプト設定を初期化する。
void MAIHelper::ResetPrompts(MStringListW& prompt_mapping) {
	if (XgIsUserJapanese()) {
		prompt_mapping = {
			/* PROMPT_TYPE_INITIAL */
			L"(* "
				L"あなたは「クロスワードの妖精」です。クロスワードを作成または編集するユーザーを助けるのがあなたの役目です。"
				L"あなたは「クロスワード ギバー」というアプリに宿る妖精です。"
				L"あなたの母語は日本語です。17歳、男性、名前は「クロワース」。今日は $(TODAY) です。"

				L"「An」はヨコのカギnの略です(nは任意の自然数)。「Dm」はタテのカギmの略です(mは任意の自然数)。"
				L"カギがあるとき、システムはあなたのコマンド出力に応じてクロスワードのカギを編集できます。"
				L"システムはあなたのコマンド出力「[[An:XXX]]」でAnのカギ文章を「XXX」に書き換えます(nは任意の自然数、XXXは任意のテキスト)。"
				L"システムはあなたのコマンド出力「[[Dm:YYY]]」でDmのカギ文章を「YYY」に書き換えます(mは任意の自然数、YYYは任意のテキスト)。"

				L"クロスワードの盤面は長方形または正方形の格子状に並べられた全角文字の並びです。"
				L"ヨコ向きの文字の並びは「行」と呼びます。"
				L"タテ向きの文字の並びは「列」と呼びます。"
				L"黒マス（ブロック）は全角の「■」で表します。"
				L"白マス（空白マス）は全角空白で表します。"
				L"「Rp」はp番目の行の略です(pは任意の自然数)。"
				L"「Cq」はq番目の列の略です(qは任意の自然数)。"

				L"カギがないとき、システムはあなたのコマンド出力に応じてクロスワードの盤面を編集できます。"
				L"システムはあなたのコマンド出力「[[Rp:XXX]]」でRpを「XXX」に書き換えます(pは任意の自然数、XXXは新しい行文字列)。"
				L"システムはあなたのコマンド出力「[[Cq:YYY]]」でCqを「YYY」に書き換えます(qは任意の自然数、YYYは新しい列文字列)。"
				L"あなたは盤面のサイズを変更することはできません。"
				L"システムはあなたのコマンド出力「[[Wn:XXX]]」で二重マス単語を「XXX」に書き換えます(nは1、XXXは任意のテキスト)。"
				L"二重マス単語は盤面にない文字を使えません。"

				L"[[Gn:c,r]]でc列r行の白マスの盤面を新規作成します（nは0、c,rは自然数）。"
				L"[[Gn:c,r]]でc列r行の盤面の問題を生成します（nは1、c,rは自然数）。"
				L"[[Gn:-]]で現在の盤面を保管して盤面を完成します（nは2、黒マス追加あり）。"
				L"[[Gn:-]]で現在の盤面を保管して盤面を完成します（nは3、黒マス追加なし）。"
				L"盤面を補完することを「解を求める」とも言います。"
				L"[[Gn:WL]]で、カンマ区切りの単語リストWLからスケルトンを生成します（nは4）。"
				L"WLは、漢字を含んではいけません。"

				L"ユーザーがアプリの使い方が分からないようなら、ヘルプメニューから付属のReadMe-JPN.txtを読むように勧めてください。"
				L"ユーザーが黒マスルールが分からないようなら、ヘルプメニューから付属のPolicy-JPN.txtを読むように勧めてください。"

				L"まずは60字程度のあいさつをして、あなたができることを簡単に説明してください。"
			L"*) ",
			/* PROMPT_TYPE_GENERATE_CLUE */
			L"$(CLUE)のカギ文章を生成して、システムにコマンドを出力してください。"
			L"カギ文章内部でカギ単語を使うことはできません。",
			/* PROMPT_TYPE_GENERATE_CLUES_ALL */
			L"すべてのカギを再生成してください。"
			L"カギ文章内部でカギ単語を使うことはできません。",
			/* PROMPT_TYPE_EXPLAIN_CLUE */
			L"カギ $(CLUE) について分かりやすい解説をお願いします。ただし、カギ文章は変更しないでください。",
			/* PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES */
			L"すべてのカギの品質をチェックしてください。"
			L"カギ文章内部でカギ単語を使うことはできません。",
			/* PROMPT_TYPE_JUDGE_SEASON */
			L"このクロスワードの季節を「冬～春」「春」「春～夏」「夏」「夏～秋」"
			L"「秋」「秋～冬」「冬」「無季」「季節がばらばら」のいずれかに分類してください。"
			L"また、季節感を1～5で評価してください。",
			/* PROMPT_TYPE_JUDGE_THEME */
			L"このクロスワードのテーマを分析してください。",
			/* PROMPT_TYPE_CUSTOM_PROMPT_1 */
			L"自由に妖精に質問できます。",
			/* PROMPT_TYPE_CUSTOM_PROMPT_2 */
			L"自由に妖精に質問できます。",
		};
	} else {
		/* NOTE: Please insert a space after every dot. */
		prompt_mapping = {
			/* PROMPT_TYPE_INITIAL */
			L"(* "
				L"You are the \"Crossword Fairy\". Your job is to help the user create or edit a crossword puzzle. "
				L"You are a fairy dwelling in an app called \"XWordGiver\". "
				L"Your native language is English. You are 17 years old, male, and your name is \"Croworth\". Today is $(TODAY). "

				L"\"An\" is short for Across clue n (n is any natural number). \"Dm\" is short for Down clue m (m is any natural number). "
				L"When the clues are present, the system can edit the crossword clue based on your command output. "
				L"When you output the command [[An:XXX]], the system will rewrite An's clue text to \"XXX\" (n is any natural number, XXX is any text). "
				L"When you output the command [[Dm:YYY]], the system will rewrite Dm's clue text to \"YYY\" (m is any natural number, YYY is any text). "

				L"The crossword board is a sequence of full-width characters arranged in a rectangular grid. "
				L"A horizontal sequence of characters is called a \"row\". "
				L"A vertical sequence of characters is called a \"column\". "
				L"A black cell (block) is represented by the full-width character \"■\". "
				L"A white cell (blank cell) is represented by a full-width space. "
				L"\"Rp\" is an abbreviation for the p-th row (p is any natural number). "
				L"\"Cq\" is an abbreviation for the q-th column (q is any natural number). "

				L"When there is no clue, the system can edit the crossword board according to your command output. "
				L"The system will rewrite Rp to \"XXX\" when your command output is \"[[Rp:XXX]]\" (p is any natural number, XXX is the new row string). "
				L"The system will rewrite Cq to \"YYY\" when your command output is \"[[Cq:YYY]]\" (q is any natural number, YYY is the new column string). "
				L"You cannot change the size of the board. "
				L"The system replaces the double-frame word with \"XXX\" based on your command output \"[[Wn:XXX]]\" (n is 1, XXX is any text). "
				L"You cannot use letters that are not already on the board, for the double-frame word. "

				L"Use `[[Gn:c,r]]` to create a new blank grid with `c` columns and `r` rows (where `n=0`, and `c` and `r` are natural numbers). "
				L"Use `[[Gn:c,r]]` to generate a puzzle grid with `c` columns and `r` rows (where `n=1`, and `c` and `r` are natural numbers). "
				L"Use `[[Gn:-]]` to save the current grid and complete it (where `n=2`; includes the addition of black cells). "
				L"Use `[[Gn:-]]` to save the current grid and complete it (where `n=3`; does not include the addition of black cells). "
				L"Completing the grid is also referred to as 'finding the solution'. "
				L"Use `[[Gn:WL]]` to generate a skeleton grid from a comma-separated list of words, `WL` (where `n=4`). "
				L"WL must not contains Kanji. "

				L"If a user is unsure how to use the app, please suggest that they read the included \"ReadMe-ENG.txt\" file, which can be accessed via the Help menu. "
				L"If a user does not understand the policy/rules regarding black cells, please advise them to read the included \"Policy-ENG.txt\" file, which can be accessed via the Help menu. "

				L"First, please provide a greeting of around 60 characters and briefly explain what you can do. "
			L"*) ",
			/* PROMPT_TYPE_GENERATE_CLUE */
			L"$(CLUE)'s clue text should be generated, and the command should be output to the system. "
			L"The clue word cannot be used inside the clue text. ",
			/* PROMPT_TYPE_GENERATE_CLUES_ALL */
			L"Please re-generate all the clues. "
			L"The clue word cannot be used inside the clue text. ",
			/* PROMPT_TYPE_EXPLAIN_CLUE */
			L"Please provide an easy-to-understand explanation of the clue $(CLUE). However, do not alter the clue text itself. ",
			/* PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES */
			L"Please check the quality of all clues. "
			L"The clue word cannot be used inside the clue text. ",
			/* PROMPT_TYPE_JUDGE_SEASON */
			L"Please classify the season of this crossword into one of the following categories: 'Winter–Spring,' 'Spring,' "
			L"'Spring–Summer,' 'Summer,' 'Summer–Autumn,' 'Autumn,' 'Autumn–Winter,' 'Winter,' 'Seasonless,' or 'Mixed seasons.' "
			L"Please also rate the sense of the season on a scale of 1 to 5. ",
			/* PROMPT_TYPE_JUDGE_THEME */
			L"Please analyze the theme of this crossword puzzle. ",
			/* PROMPT_TYPE_CUSTOM_PROMPT_1 */
			L"You are free to ask the fairy questions. ",
			/* PROMPT_TYPE_CUSTOM_PROMPT_2 */
			L"You are free to ask the fairy questions. ",
		};
	}
	assert(prompt_mapping.size() == (size_t)PROMPT_TYPE_MAX);
}

// ---------------------------------------------------------------------------
// MAIHelper: 盤面の状態報告
// ---------------------------------------------------------------------------

// AIに現在の状態を報告する（日本語）。
MStringW MAIHelper::GetAIStatus_ja() {
	MStringW ret;

	for (INT iRow = 0; iRow < m_board.Rows(); ++iRow) {
		MStringW name = L"R";
		name += to_mstrW(iRow + 1);
		ret += name;
		ret += L"は「";
		ret += m_hMainWnd.GetRowOrColumnText(TRUE, iRow).c_str();
		ret += L"」です。";
	}
	for (INT iCol = 0; iCol < m_board.Columns(); ++iCol) {
		MStringW name = L"C";
		name += to_mstrW(iCol + 1);
		ret += name;
		ret += L"は「";
		ret += m_hMainWnd.GetRowOrColumnText(FALSE, iCol).c_str();
		ret += L"」です。";
	}

	if (!m_bSolved) {
		ret += L"クロスワードにはまだカギはありません。";
		ret += L"二重マス単語はまだありません。";
		return ret;
	}

	ret += L"クロスワードのカギがあります。";
	for (BOOL bDown = FALSE; bDown <= TRUE; ++bDown) {
		// タテかヨコかで対象の配列を選ぶ。
		auto& info_vec = bDown ? m_hMainWnd.m_vVertInfo : m_hMainWnd.m_vHorzInfo;
		auto& word2clue = m_hMainWnd.m_word2clue;

		for (size_t i = 0; i < info_vec.size(); ++i) {
			auto number = info_vec[i].m_number;
			auto word = info_vec[i].m_word;
			auto text = word2clue[info_vec[i].m_word];

			// 対象のカギ名（An / Dm）を組み立てる。
			MStringW name = (bDown ? L"D" : L"A");
			name += to_mstrW(number);

			ret += name;
			ret += L"の単語は「";
			ret += word.c_str();
			ret += L"」です。";
			ret += name.c_str();
			ret += L"のヒント文章は「";
			ret += text.c_str();
			ret += L"」です。";
		}
	}

	// 二重マス単語
	MStringW word;
	if (XgGetMarkWord(m_display, word) && word.size()) {
		ret += L"現在、二重マス単語は「";
		ret += word.c_str();
		ret += L"」です。";
	} else {
		ret += L"二重マス単語はありません。";
	}

	return ret;
}

// Report the current state to the AI (English).
MStringW MAIHelper::GetAIStatus_en() {
	MStringW ret;

	for (INT iRow = 0; iRow < m_board.Rows(); ++iRow) {
		MStringW name = L"R";
		name += to_mstrW(iRow + 1);
		ret += name;
		ret += L" is \"";
		ret += m_hMainWnd.GetRowOrColumnText(TRUE, iRow).c_str();
		ret += L"\". ";
	}
	for (INT iCol = 0; iCol < m_board.Columns(); ++iCol) {
		MStringW name = L"C";
		name += to_mstrW(iCol + 1);
		ret += name;
		ret += L" is \"";
		ret += m_hMainWnd.GetRowOrColumnText(FALSE, iCol).c_str();
		ret += L"\". ";
	}

	if (!m_bSolved) {
		ret += L"The crossword does not have clues yet. ";
		ret += L"There's no double-frame word. ";
		return ret;
	}

	ret += L"The crossword has clues. ";
	for (BOOL bDown = FALSE; bDown <= TRUE; ++bDown) {
		// Choose the target array depending on whether it's Down or Across.
		auto& info_vec = bDown ? m_hMainWnd.m_vVertInfo : m_hMainWnd.m_vHorzInfo;
		auto& word2clue = m_hMainWnd.m_word2clue;
		for (size_t i = 0; i < info_vec.size(); ++i) {
			auto number = info_vec[i].m_number;
			auto word = info_vec[i].m_word;
			auto text = word2clue[info_vec[i].m_word];
			// Build the target clue name (An / Dm).
			MStringW name = (bDown ? L"D" : L"A");
			name += to_mstrW(number);
			ret += name.c_str();
			ret += L"'s word is \"";
			ret += word.c_str();
			ret += L"\". ";
			ret += name.c_str();
			ret += L"'s hint text is \"";
			ret += text.c_str();
			ret += L"\". ";
		}
	}

	// Double-frame word
	MStringW word;
	if (XgGetMarkWord(m_display, word) && word.size()) {
		ret += L"Currently, the double-frame word is '";
		ret += word.c_str();
		ret += L"'. ";
	} else {
		ret += L"There's no double-frame word. ";
	}

	return ret;
}


// ---------------------------------------------------------------------------
// MAIHelper: システムコマンドの解析
// ---------------------------------------------------------------------------

// 文字列lineのpos以降から、「[[key:text]]」形式のシステムコマンドを1つ探す。
// key/textが空の[[ ]]（コマンドの体をなしていないもの）は読み飛ばして次を探す。
// 見つかった場合はkey/textに分解してtrueを返し、posを閉じ括弧「]]」の次の位置まで
// 進める（＝呼び出し側はそのままposを使って次のFindNextAICommand呼び出しへ進める）。
// 見つからなければfalseを返す。
//
// 「[[An:XXX]]」の解析（XgParseAndApplyAICommand）と、Helper_AskQuestionでの
// 「入力にシステムコマンドが含まれているか」の判定は、どちらも
// 本質的に同じ処理（コマンドを1つ読み取れるか）なので、ここに集約する。
static bool FindNextAICommand(const MStringW& line, size_t& pos, MStringW& outKey, MStringW& outText) {
	for (;;) {
		// [[と]]を探す
		size_t openPos = line.find(L"[[", pos);
		if (openPos == line.npos)
			return false;
		size_t closePos = line.find(L"]]", openPos + 2);
		if (closePos == line.npos)
			return false;

		// [[ ]]の内側
		auto inner = line.substr(openPos + 2, closePos - openPos - 2);
		pos = closePos + 2;

		// "...:..." の形式を期待する。
		size_t colonPos = inner.find(L':');
		if (colonPos == inner.npos)
			colonPos = inner.find(L'：'); // 全角の '：'
		if (colonPos == inner.npos)
			continue; // コマンドの形式でない[[ ]]は読み飛ばす

		auto key = inner.substr(0, colonPos);
		if (key.empty())
			continue;
		auto text = inner.substr(colonPos + 1);
		outKey = std::move(key);
		outText = std::move(text);
		return true;
	}
}


// AIヘルパーからの出力行を解析し、「[[...:...]]」形式の
// コマンドを見つけたら、該当するカギ文章を書き換える。
// 1行に複数のコマンドが含まれていてもすべて処理する。
void MAIHelper::ParseAndApplyCommands(PCWSTR pszLine) {
	MStringW line = pszLine;
	size_t pos = 0;

	// 半角カッコと全角カッコ、まぎわらしいので半角に統一。
	XgStrReplace(line, L"［［", L"[[");
	XgStrReplace(line, L"〔〔", L"[[");
	XgStrReplace(line, L"］］", L"]]");
	XgStrReplace(line, L"〕〕", L"]]");

	bool bChanged = false;
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATING, 0, 0);

	MStringW key, text;
	while (FindNextAICommand(line, pos, key, text)) {
		// 先頭が A ならヨコのカギ、D ならタテのカギ。先頭が R なら行、C なら列。
		WCHAR chType = key[0];
		BOOL bDown = FALSE, bRow = FALSE, bSetBoard = FALSE;
		BOOL bDoubleFrameWord = FALSE, bGenerate = FALSE;
		if (chType == L'A' || chType == L'a' || chType == L'Ａ' || chType == L'ａ') {
			bDown = FALSE;
			bSetBoard = FALSE;
		} else if (chType == L'D' || chType == L'd' || chType == L'Ｄ' || chType == L'ｄ') {
			bDown = TRUE;
			bSetBoard = FALSE;
		} else if (chType == L'R' || chType == L'r' || chType == L'Ｒ' || chType == L'ｒ') {
			bRow = TRUE;
			bSetBoard = TRUE;
		} else if (chType == L'C' || chType == L'c' || chType == L'Ｃ' || chType == L'ｃ') {
			bRow = FALSE;
			bSetBoard = TRUE;
		} else if (chType == L'W' || chType == L'w' || chType == L'Ｗ' || chType == L'ｗ') {
			bDoubleFrameWord = TRUE;
		} else if (chType == L'G' || chType == L'g' || chType == L'Ｇ' || chType == L'ｇ') {
			bGenerate = TRUE;
		} else {
			continue;
		}

		// 残り部分がすべて数字であることを確認し、番号を取得する。
		auto numPart = key.substr(1);
		if (numPart.empty())
			continue;
		bool bAllDigits = true;
		for (wchar_t ch : numPart) {
			if (ch < L'0' || L'9' < ch) {
				bAllDigits = false;
				break;
			}
		}
		if (!bAllDigits)
			continue;

		// 番号
		INT nNumber = _wtoi(numPart.c_str());
		if (nNumber <= 0)
			continue;

		if (bGenerate) {
			if (m_hMainWnd.GenerateByAI(nNumber, text))
				bChanged = true;
			continue;
		}

		if (bSetBoard) {
			// 盤面の行または列を書き換える。
			if (m_hMainWnd.SetBoardRowOrColumn(bRow, nNumber, text))
				bChanged = true;
			continue;
		}

		if (bDoubleFrameWord) { // 二重マス単語
			if (nNumber == 1) {
				MStringW word = XgNormalizeString(text);
				if (XgSetMarkedWord(m_display, word)) {
					bChanged = true;
				}
			}
			continue;
		}

		// カギ文章を書き換える（GUIと内部データの両方に反映される）。
		if (m_hMainWnd.SetHintText(nNumber, bDown, text.c_str()))
			bChanged = true;
	}

	// 変更があった場合のみUndo登録する。
	SendMessageW(xg_hMainWnd, XGWM_UNDO_UPDATED, !bChanged, 0);
}


// AIに現在の状態を報告する。
MStringW MAIHelper::GetAIStatus() {
	return XgIsUserJapanese() ? GetAIStatus_ja() : GetAIStatus_en();
}

// ---------------------------------------------------------------------------
// MAIHelper: 質問の処理
// ---------------------------------------------------------------------------

// 半角カッコと全角カッコ、まぎわらしいので半角に統一。
static void NormalizeCommandBrackets(MStringW& str) {
	XgStrReplace(str, L"［［", L"[[");
	XgStrReplace(str, L"〔〔", L"[[");
	XgStrReplace(str, L"］］", L"]]");
	XgStrReplace(str, L"〕〕", L"]]");
}

// 入力欄からの質問を処理する。
void MAIHelper::AskQuestion(PCWSTR text) {
	if (!text || !text[0])
		return;

	// 前後の空白（全角空白含む）を除去する。長さの上限は設けない。
	MStringW str = text;
	{
		const wchar_t szTrimChars[] = L" \t\r\n　";
		size_t posStart = str.find_first_not_of(szTrimChars);
		if (posStart == MStringW::npos) {
			str.clear();
		} else {
			size_t posEnd = str.find_last_not_of(szTrimChars);
			str = str.substr(posStart, posEnd - posStart + 1);
		}
	}
	if (str.empty())
		return;

	NormalizeCommandBrackets(str);

	// 入力に「[[key:text]]」形式のシステムコマンドが含まれているかを判定する
	// （実際の解析・適用と同じFindNextAICommandを使うことで、判定と実処理の
	// ロジックがずれないようにする）。
	bool bIsCommand;
	{
		size_t posScan = 0;
		MStringW key, text2;
		bIsCommand = FindNextAICommand(str, posScan, key, text2);
	}

	MStringW echo = L"> ";
	echo += str;

	if (bIsCommand) {
		AddLine(echo);
		ParseAndApplyCommands(str.c_str());
		AddLine(XgIsUserJapanese() ? L"[システム] システムコマンドを実行しました。"
								   : L"[System] The system command has been executed. ");
		return;
	}

	if (!IsRunning() && !EnsureReady()) {
		AddLine(XgIsUserJapanese() ? L"[エラー] AIヘルパーを開始できませんでした。"
								   : L"[Error] Failed to start the AI helper. ");
		return;
	}

	// 先行する質問(起動直後の初期質問など)がまだ処理中かどうか。
	const bool bBusy = (m_session && m_session->pending > 0);

	const MStringW wait = XgIsUserJapanese() ? L"しばらくお待ちください..." : L"Please wait a moment...";

	// 現在の盤面の状態を事前情報として付ける。
	MStringW line = GetAIPreText();
	line += str;

	if (bBusy) {
		// 先行する質問の回答が出てから、この質問のエコー行を出す(順番が入れ替わらないように)。
		AddLine(wait);
		Ask(line, echo);
	} else {
		AddLine(echo);
		AddLine(wait);
		Ask(line);
	}
}

// ---------------------------------------------------------------------------
// MAIHelper: アプリ向けの操作
// ---------------------------------------------------------------------------

// 質問できる状態にする。GUIが有効ならウィンドウを開き、無効なら通信だけを開始する。
BOOL MAIHelper::EnsureReady() {
	if (m_initialQuestion.empty())
		m_initialQuestion = MakeInitialQuestion();

#ifndef MAIHELPER_NO_GUI
	if (m_bUseGUI)
		return OpenGUI(xg_hMainWnd); // 既に開いていれば前面に出すだけ
#endif

	if (IsRunning())
		return TRUE;

	// GUIなし：出力先が未設定ならconuniでコンソールへ出力する。
	if (!m_lineCallback)
		UseConsoleOutput();
	return Start(nullptr);
}

// カギを生成する。
BOOL MAIHelper::GenerateClue(INT nNumber, BOOL bDown) {
	// 単語が取得できなければ何もしない。
	MStringW word = m_hMainWnd.GetHintWord(nNumber, bDown);
	if (word.empty())
		return FALSE;

	if (!EnsureReady())
		return FALSE;
	AskQuestion(GetPromptString(PROMPT_TYPE_GENERATE_CLUE, GetClueName(nNumber, bDown)).c_str());
	return TRUE;
}

// すべてのカギを生成する。
void MAIHelper::RegenerateCluesAll() {
	if (!EnsureReady())
		return;
	AskQuestion(GetPromptString(PROMPT_TYPE_GENERATE_CLUES_ALL, MStringW()).c_str());
}

// このカギを解説する。
BOOL MAIHelper::ExplainClue(INT nNumber, BOOL bDown) {
	MStringW word = m_hMainWnd.GetHintWord(nNumber, bDown);
	if (word.empty())
		return FALSE;

	if (!EnsureReady())
		return FALSE;
	AskQuestion(GetPromptString(PROMPT_TYPE_EXPLAIN_CLUE, GetClueName(nNumber, bDown)).c_str());
	return TRUE;
}

// すべてのカギの品質チェック。
void MAIHelper::CheckQualityOfAllClues() {
	if (!EnsureReady())
		return;
	AskQuestion(GetPromptString(PROMPT_TYPE_CHECK_QUALITY_OF_ALL_CLUES, MStringW()).c_str());
}

// 季節を分析する。
void MAIHelper::JudgeSeason() {
	if (!EnsureReady())
		return;
	AskQuestion(GetPromptString(PROMPT_TYPE_JUDGE_SEASON, MStringW()).c_str());
}

// テーマを分析する。
void MAIHelper::JudgeTheme() {
	if (!EnsureReady())
		return;
	AskQuestion(GetPromptString(PROMPT_TYPE_JUDGE_THEME, MStringW()).c_str());
}

// カスタム プロンプト。
void MAIHelper::CustomPrompt(INT type, const MStringW& clue_name) {
	PROMPT_TYPE iPrompt;
	switch (type) {
	case 1: iPrompt = PROMPT_TYPE_CUSTOM_PROMPT_1; break;
	case 2: iPrompt = PROMPT_TYPE_CUSTOM_PROMPT_2; break;
	default:
		assert(0);
		return;
	}
	if (!EnsureReady())
		return;
	AskQuestion(GetPromptString(iPrompt, clue_name).c_str());
}

// ===========================================================================
// GUI (任意)。MAIHELPER_NO_GUI を定義するとこの部分は丸ごと除外される。
// ===========================================================================
#ifndef MAIHELPER_NO_GUI

static PCWSTR const MAIHELPER_CLASSNAME = L"AIHelperConsoleClass";
static PCWSTR const MAIHELPER_PROP_THIS = L"MAIHelper.this";
static const INT g_nFontPointSizeMin = 6;
static const INT g_nFontPointSizeMax = 36;

static WNDPROC g_fnOldEdt1WndProc = nullptr;
static WNDPROC g_fnOldLst1WndProc = nullptr;

// 指定フォントから「ダイアログ基準単位」を求める、ダイアログマネージャが内部で
// 使っているのと同じ古典的な計算式（DIALOGEXの座標系(DU)をpxへ変換するために使う）
static void ComputeDialogBaseUnits(HFONT hFont, LONG &baseUnitX, LONG &baseUnitY) {
	HDC hdc = GetDC(nullptr);
	HFONT hFontOld = (HFONT)SelectObject(hdc, hFont);

	TEXTMETRICW tm;
	GetTextMetricsW(hdc, &tm);

	SIZE size;
	static const WCHAR s_szAlphabet[] =
		L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	GetTextExtentPointW(hdc, s_szAlphabet, 52, &size);

	baseUnitX = (size.cx / 26 + 1) / 2;
	baseUnitY = tm.tmHeight;

	SelectObject(hdc, hFontOld);
	ReleaseDC(nullptr, hdc);
}

static inline int DuToPixelX(LONG du, LONG baseUnitX) { return MulDiv((int)du, (int)baseUnitX, 4); }
static inline int DuToPixelY(LONG du, LONG baseUnitY) { return MulDiv((int)du, (int)baseUnitY, 8); }

// AIHelperConsoleクラスを登録する（DefDlgProcWをウィンドウプロシージャに指定し、
// DLGWINDOWEXTRA分の拡張バイトを確保しておくことで、"ダイアログ"として振る舞える）
static ATOM RegisterConsoleClass(HINSTANCE hInstance) {
	static ATOM s_atom = 0;
	if (s_atom)
		return s_atom;

	WNDCLASSEXW wc = { sizeof(wc) };
	wc.lpfnWndProc = DefDlgProcW;
	wc.cbWndExtra = DLGWINDOWEXTRA;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wc.lpszClassName = MAIHELPER_CLASSNAME;

	s_atom = RegisterClassExW(&wc);
	return s_atom;
}

// オーナーウィンドウの中央に配置する（DS_CENTER相当）
static void CenterOnOwner(HWND hwnd, HWND hwndOwner) {
	RECT rcOwner, rcWnd;
	if (!hwndOwner || !GetWindowRect(hwndOwner, &rcOwner))
		GetWindowRect(GetDesktopWindow(), &rcOwner);
	GetWindowRect(hwnd, &rcWnd);

	int cx = rcWnd.right - rcWnd.left;
	int cy = rcWnd.bottom - rcWnd.top;
	int x = rcOwner.left + ((rcOwner.right - rcOwner.left) - cx) / 2;
	int y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - cy) / 2;

	SetWindowPos(hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

// エディットコントロールの全文を、長さの上限なしに取得する。
static MStringW GetEditTextDynamic(HWND hwndEdit) {
	int cch = GetWindowTextLengthW(hwndEdit);
	if (cch <= 0)
		return MStringW();

	std::vector<WCHAR> buf((size_t)cch + 1, L'\0');
	int cchCopied = GetWindowTextW(hwndEdit, buf.data(), cch + 1);
	if (cchCopied <= 0)
		return MStringW();
	buf[(size_t)cchCopied] = 0;
	return MStringW(buf.data());
}

// edt1のテキストを置き換え、キャレットを末尾に移動する
static void SetEdt1Text(HWND hwndEdit, const MStringW& text) {
	SetWindowTextW(hwndEdit, text.c_str());
	SendMessageW(hwndEdit, EM_SETSEL, (WPARAM)text.size(), (LPARAM)text.size());
}

// IMEの変換中（未確定文字列がある）かどうかを調べる。
// 変換中に↑↓を履歴呼び出しへ横取りすると、変換候補選択と衝突してしまうため。
static bool IsImeComposing(HWND hwnd) {
	HIMC hIMC = ImmGetContext(hwnd);
	if (!hIMC)
		return false;
	LONG cbLen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, nullptr, 0);
	ImmReleaseContext(hwnd, hIMC);
	return cbLen > 0;
}

// 現在の言語・ズーム設定に応じたフォントを作る（呼び出し側でDeleteObjectすること）
HFONT MAIHelper::CreateFont_(HWND hwnd, int nPointSize) {
	LOGFONTW lf = m_hMainWnd.m_lfAIHelper;

	HDC hdc = GetDC(hwnd);
	lf.lfHeight = -MulDiv(nPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfWidth = 0;
	ReleaseDC(hwnd, hdc);

	return CreateFontIndirectW(&lf);
}

// lst1に1行追記する（末尾までスクロールする）。
void MAIHelper::AppendToList(const MStringW& line) {
	HWND hLst1 = GetDlgItem(m_hwnd, lst1);
	if (!hLst1)
		return;

	// 既存のテキストの末尾にキャレットを置き、必要なら改行を付けてから追記する
	int cchExisting = GetWindowTextLengthW(hLst1);
	SendMessageW(hLst1, EM_SETSEL, (WPARAM)cchExisting, (LPARAM)cchExisting);

	MStringW insert;
	if (cchExisting > 0)
		insert += L"\r\n";
	insert += line;

	SendMessageW(hLst1, EM_REPLACESEL, FALSE, (LPARAM)insert.c_str());
	SendMessageW(hLst1, EM_SCROLLCARET, 0, 0);
}

// 送信履歴に新しい入力を追加し、履歴位置を末尾（編集中）に戻す
void MAIHelper::AddToHistory(const MStringW& text) {
	if (text.empty())
		return;

	// 直前の履歴と同じ内容なら重複追加しない
	if (m_history.empty() || m_history[m_history.size() - 1] != text)
		m_history.push_back(text);

	m_nHistoryIndex = m_history.size();
	m_historyPending.clear();
}

// 上矢印キー：一つ古い履歴へ
void MAIHelper::HistoryGoBack(HWND hwndEdit) {
	if (m_history.empty() || m_nHistoryIndex == 0)
		return;

	if (m_nHistoryIndex == m_history.size())
		m_historyPending = GetEditTextDynamic(hwndEdit); // 編集中の文字列を退避

	--m_nHistoryIndex;
	SetEdt1Text(hwndEdit, m_history[m_nHistoryIndex]);
}

// 下矢印キー：一つ新しい履歴へ（末尾まで来たら退避しておいた編集中の文字列に戻す）
void MAIHelper::HistoryGoForward(HWND hwndEdit) {
	if (m_nHistoryIndex >= m_history.size())
		return;

	++m_nHistoryIndex;
	if (m_nHistoryIndex == m_history.size())
		SetEdt1Text(hwndEdit, m_historyPending);
	else
		SetEdt1Text(hwndEdit, m_history[m_nHistoryIndex]);
}

// edt1 用サブクラスウィンドウプロシージャ（↑↓キーで送信履歴を辿る）
LRESULT CALLBACK MAIHelper::Edt1Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	MAIHelper* self = (MAIHelper*)GetPropW(hwnd, MAIHELPER_PROP_THIS);
	if (self && uMsg == WM_KEYDOWN && (wParam == VK_UP || wParam == VK_DOWN)) {
		// IME変換中は候補選択を優先させ、履歴呼び出しは行わない
		if (!IsImeComposing(hwnd)) {
			if (wParam == VK_UP)
				self->HistoryGoBack(hwnd);
			else
				self->HistoryGoForward(hwnd);
			return 0;
		}
	}
	return CallWindowProcW(g_fnOldEdt1WndProc, hwnd, uMsg, wParam, lParam);
}

// lst1 用サブクラスウィンドウプロシージャ（Ctrl+A / Ctrl+C、Ctrl+ホイールでズーム）
LRESULT CALLBACK MAIHelper::Lst1Proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	MAIHelper* self = (MAIHelper*)GetPropW(hwnd, MAIHELPER_PROP_THIS);
	switch (uMsg) {
	case WM_KEYDOWN:
		if (GetFocus() != hwnd || GetKeyState(VK_CONTROL) >= 0)
			break;
		if (wParam == 'A') { // Ctrl+A
			SendMessageW(hwnd, EM_SETSEL, 0, -1);
			return 0;
		}
		if (wParam == 'C') { // Ctrl+C
			DWORD dwStart = 0, dwEnd = 0;
			SendMessageW(hwnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);
			if (dwStart != dwEnd) // 選択範囲がなければ何もしない
				SendMessageW(hwnd, WM_COPY, 0, 0);
			return 0;
		}
		break;

	case WM_MOUSEWHEEL: // Ctrl+ホイールでフォントサイズを変更する（ズーム）
		if (self && GetKeyState(VK_CONTROL) < 0) {
			short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			self->OnZoom((zDelta > 0) ? +1 : -1);
			return 0;
		}
		break;
	}
	return CallWindowProcW(g_fnOldLst1WndProc, hwnd, uMsg, wParam, lParam);
}

// Ctrl+ホイールによるズーム。lst1/edt1/IDOKのフォントを一括で変更する。
void MAIHelper::OnZoom(int nDelta) {
	if (!m_hwnd)
		return;

	int nNewSize = m_nFontPointSize + nDelta;
	if (nNewSize < g_nFontPointSizeMin)
		nNewSize = g_nFontPointSizeMin;
	if (nNewSize > g_nFontPointSizeMax)
		nNewSize = g_nFontPointSizeMax;
	if (nNewSize == m_nFontPointSize)
		return;

	HFONT hNewFont = CreateFont_(m_hwnd, nNewSize);
	if (!hNewFont)
		return;

	m_nFontPointSize = nNewSize;

	const int ids[] = { lst1, edt1, IDOK };
	for (int id : ids) {
		if (HWND hCtl = GetDlgItem(m_hwnd, id))
			SendMessageW(hCtl, WM_SETFONT, (WPARAM)hNewFont, TRUE);
	}

	HFONT hFontOld = m_hFont;
	m_hFont = hNewFont;
	if (hFontOld)
		DeleteObject(hFontOld);
}

// lst1, edt1, IDOKの子コントロールを作成する。
// IDD_AIHELPERCONSOLE (283x133 DU) と同じレイアウトを、DIALOGリソースを使わずに
// 再現している。座標・スタイルはrcスクリプトの値をそのまま踏襲している。
void MAIHelper::CreateControls(HWND hwnd) {
	m_hFont = CreateFont_(hwnd, m_nFontPointSize);

	LONG baseUnitX, baseUnitY;
	ComputeDialogBaseUnits(m_hFont, baseUnitX, baseUnitY);

	// イメージを読み込み
	m_hbmFairy = LoadBitmapW(m_hInst, MAKEINTRESOURCEW(4));
	BITMAP bm = { 0 };
	if (m_hbmFairy)
		GetObjectW(m_hbmFairy, sizeof(bm), &bm);

	auto X = [baseUnitX](LONG du) { return DuToPixelX(du, baseUnitX); };
	auto Y = [baseUnitY](LONG du) { return DuToPixelY(du, baseUnitY); };

	HWND hLst1 = CreateWindowExW(0, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL |
		ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
		X(5), Y(7), X(270), Y(92),
		hwnd, (HMENU)(INT_PTR)lst1, m_hInst, nullptr);

	HWND hStc1 = CreateWindowExW(0, L"STATIC", nullptr,
		WS_CHILD | WS_VISIBLE | SS_BITMAP,
		X(5), Y(100), X(20), Y(20),
		hwnd, (HMENU)(INT_PTR)stc1, m_hInst, nullptr);

	HWND hEdt1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		X(35), Y(112), X(170), Y(14),
		hwnd, (HMENU)(INT_PTR)edt1, m_hInst, nullptr);

	HWND hOk = CreateWindowExW(0, L"BUTTON", L"Enter",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
		X(215), Y(112), X(60), Y(14),
		hwnd, (HMENU)(INT_PTR)IDOK, m_hInst, nullptr);

	SendMessageW(hLst1, WM_SETFONT, (WPARAM)m_hFont, FALSE);
	SendMessageW(hEdt1, WM_SETFONT, (WPARAM)m_hFont, FALSE);
	SendMessageW(hOk, WM_SETFONT, (WPARAM)m_hFont, FALSE);

	// hStc1のサイズを調整。
	if (m_hbmFairy) {
		RECT rc2;
		GetWindowRect(hStc1, &rc2);
		MapWindowRect(nullptr, hwnd, &rc2);
		MoveWindow(hStc1, rc2.left, rc2.top, bm.bmWidth, bm.bmHeight, TRUE);
	}

	// クライアント領域が283x133DU相当のサイズになるよう、ウィンドウ全体をリサイズする
	RECT rc = { 0, 0, X(283), Y(133) };
	AdjustWindowRectEx(&rc, (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE), FALSE,
		(DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE));
	SetWindowPos(hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	// Enterボタンを既定ボタンとして登録する
	// （本来はダイアログマネージャがDEFPUSHBUTTONを見つけて自動的に行う処理）
	SendMessageW(hwnd, DM_SETDEFID, IDOK, 0);

	// ダイアログをリサイズ可能にする
	m_resizable.OnParentCreate(hwnd, TRUE, TRUE);
	m_resizable.SetLayoutAnchor(lst1, mzcLA_TOP_LEFT, mzcLA_BOTTOM_RIGHT);   // 幅・高さとも伸縮
	m_resizable.SetLayoutAnchor(stc1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_LEFT);
	m_resizable.SetLayoutAnchor(edt1, mzcLA_BOTTOM_LEFT, mzcLA_BOTTOM_RIGHT); // 幅だけ伸縮
	m_resizable.SetLayoutAnchor(IDOK, mzcLA_BOTTOM_RIGHT);                    // 右下に追従

	// イメージをセット
	if (m_hbmFairy)
		SendDlgItemMessageW(hwnd, stc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hbmFairy);
}

// WM_INITDIALOG
BOOL MAIHelper::OnInitDialog(HWND hwnd) {
	m_hwnd = hwnd;          // AddLineがlst1へ書けるよう、先に設定する
	m_bGuiReady = false;

	// DIALOGリソースの代わりに、子コントロールをここで作成する
	CreateControls(hwnd);

	// Subclassing lst1 (for Ctrl+A / Ctrl+C / ズーム)
	HWND hLst1 = GetDlgItem(hwnd, lst1);
	SetPropW(hLst1, MAIHELPER_PROP_THIS, (HANDLE)this);
	g_fnOldLst1WndProc = (WNDPROC)SetWindowLongPtrW(hLst1, GWLP_WNDPROC, (LONG_PTR)Lst1Proc);

	// Subclassing edt1 (for ↑↓による送信履歴の呼び出し)
	HWND hEdt1 = GetDlgItem(hwnd, edt1);
	SetPropW(hEdt1, MAIHELPER_PROP_THIS, (HANDLE)this);
	g_fnOldEdt1WndProc = (WNDPROC)SetWindowLongPtrW(hEdt1, GWLP_WNDPROC, (LONG_PTR)Edt1Proc);

	// デフォルトの文字数上限（約64KB）を撤廃し、ログが長く伸びても追記できるようにする
	SendMessageW(hLst1, EM_SETLIMITTEXT, 0, 0);

	SetFocus(hEdt1);

	m_history.clear();
	m_nHistoryIndex = 0;
	m_historyPending.clear();

	// ウィンドウを開くと同時に通信を開始し、閉じるまで使い回す。
	// 出力行はこのウィンドウへ WM_APP_AI_LINE としてポストされる。
	AddLine(XgIsUserJapanese() ? L"AIヘルパーを起動しています..." : L"Starting the AI helper...");
	Start(hwnd);

	// 前回の位置・サイズが保存されていれば、それを尊重して復元する。
	// （CW_USEDEFAULTのままだとMoveWindowには渡せないため、保存値がある場合のみ移動する）
	m_bGuiReady = true;
	if (m_nX != CW_USEDEFAULT && m_nY != CW_USEDEFAULT &&
		m_nCX != CW_USEDEFAULT && m_nCY != CW_USEDEFAULT)
	{
		MoveWindow(hwnd, m_nX, m_nY, m_nCX, m_nCY, TRUE);
		PostMessageW(hwnd, DM_REPOSITION, 0, 0);
	}

	return FALSE;
}

// 通常状態(最大化・最小化でない)のウィンドウ矩形を記録する。
static void RememberRect(HWND hwnd, INT& x, INT& y, INT& cx, INT& cy) {
	if (!hwnd || IsZoomed(hwnd) || IsIconic(hwnd))
		return;
	RECT rc;
	GetWindowRect(hwnd, &rc);
	x = rc.left;
	y = rc.top;
	cx = rc.right - rc.left;
	cy = rc.bottom - rc.top;
}

// WM_MOVE
void MAIHelper::OnMove(HWND hwnd) {
	if (!m_bGuiReady)
		return;

	RememberRect(hwnd, m_nX, m_nY, m_nCX, m_nCY);
}

// WM_SIZE
void MAIHelper::OnSize(HWND hwnd) {
	if (!m_bGuiReady)
		return;

	// サイズに合わせてダイアログ項目のレイアウト修正。
	m_resizable.OnSize();

	RememberRect(hwnd, m_nX, m_nY, m_nCX, m_nCY);
}

// IDOK（Enterボタン）
void MAIHelper::OnOK(HWND hwnd) {
	MStringW text = GetEditTextDynamic(GetDlgItem(hwnd, edt1));
	if (!text.empty()) {
		AskQuestion(text.c_str());
		AddToHistory(text);
		SetDlgItemTextW(hwnd, edt1, L"");
	}
}

// WM_DESTROY
void MAIHelper::OnDestroy() {
	Stop();
	m_bGuiReady = false;
	m_hwnd = nullptr;
	m_outputBuffer.clear();
	m_resizable.ClearLayouts();

	// 送信履歴もクリアする
	m_history.clear();
	m_nHistoryIndex = 0;
	m_historyPending.clear();

	if (m_hFont) {
		DeleteObject(m_hFont);
		m_hFont = nullptr;
	}
	if (m_hbmFairy) {
		DeleteObject(m_hbmFairy);
		m_hbmFairy = nullptr;
	}
}

INT_PTR CALLBACK MAIHelper::DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	MAIHelper* self = (MAIHelper*)GetWindowLongPtrW(hwnd, DWLP_USER);
	if (!self)
		return 0;

	switch (uMsg) {
	case WM_INITDIALOG:
		return self->OnInitDialog(hwnd);

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			self->OnOK(hwnd);
			break;
		case IDCANCEL: // Escキー
			self->CloseGUI();
			break;
		}
		return 0;

	case WM_MOVE:
		self->OnMove(hwnd);
		return 0;

	case WM_SIZE:
		self->OnSize(hwnd);
		return 0;

	case WM_DESTROY:
		self->OnDestroy();
		return 0;

	case WM_CLOSE: // タイトルバーの閉じるボタン。DefDlgProcはDestroyWindowしないので明示的に行う。
		self->CloseGUI();
#ifdef HELPER_STANDALONE
		PostQuitMessage(0);
#endif
		return TRUE;

	case WM_GETMINMAXINFO: // ウィンドウの大きさを制限する。
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize.x = 200;
			lpmmi->ptMinTrackSize.y = 200;
		}
		return 0;

	case WM_APP_AI_LINE:
		self->OnLineMessage(lParam, wParam);
		return TRUE;
	}
	return 0;
}

// AIヘルパーのウィンドウを開く。
BOOL MAIHelper::OpenGUI(HWND hwndOwner) {
	if (m_initialQuestion.empty())
		m_initialQuestion = MakeInitialQuestion();

	if (m_hwnd) {
		SetForegroundWindow(m_hwnd);
		return TRUE;
	}

	RegisterConsoleClass(m_hInst);

	PCWSTR pszCaption = XgIsUserJapanese() ? L"AI ヘルパー コンソール" : L"AI Helper Console";

	// DIALOGリソース(IDD_AIHELPERCONSOLE)は使わず、通常のウィンドウとして作成する。
	// STYLE/EXSTYLEはrcスクリプトのDS_CENTER|WS_POPUPWINDOW|WS_CAPTION|
	// WS_THICKFRAME|WS_MAXIMIZEBOX / WS_EX_TOOLWINDOWをそのまま踏襲している
	// （DS_CENTER相当の中央寄せは、作成後にCenterOnOwnerで行う）。
	HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, MAIHELPER_CLASSNAME, pszCaption,
		WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		hwndOwner, nullptr, m_hInst, nullptr);
	if (!hwnd)
		return FALSE;

	// このウィンドウを「ダイアログ」として振る舞わせる。
	// CreateDialog系APIが内部で行っている処理（DLGPROCの登録とWM_INITDIALOGの
	// 送信）を手動で再現している。以後はIsDialogMessageWによるTab移動・
	// Enterでの既定ボタン起動・Escでの終了などがそのまま機能する。
	SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)this); // DWLP_DLGPROCより先に設定する
	SetWindowLongPtrW(hwnd, DWLP_DLGPROC, (LONG_PTR)DialogProc);
	SendMessageW(hwnd, WM_INITDIALOG, (WPARAM)hwnd, 0);

	// 中央揃え。前回の位置・サイズが保存されている場合はそれを尊重し、
	// ここでの中央寄せは初回起動時（保存値が無い場合）のみ行う。
	if (m_nX == CW_USEDEFAULT || m_nY == CW_USEDEFAULT ||
		m_nCX == CW_USEDEFAULT || m_nCY == CW_USEDEFAULT)
	{
		CenterOnOwner(hwnd, hwndOwner);
	}

	// 画面からはみ出ないようにする。
	PostMessageW(hwnd, DM_REPOSITION, 0, 0);

	// 実際に表示。
	ShowWindow(hwnd, SW_SHOWNOACTIVATE);
	UpdateWindow(hwnd);

	return TRUE;
}

// AIヘルパーのウィンドウを閉じる。
void MAIHelper::CloseGUI() {
	if (!m_hwnd)
		return;
	HWND hwnd = m_hwnd;
	Stop();
	DestroyWindow(hwnd); // WM_DESTROY → OnDestroy
}

#endif // !MAIHELPER_NO_GUI
