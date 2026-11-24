// utils.cpp --- Utility functions
// Author: katahiromz
// License: MIT

#include "utils.h"
#include <shellapi.h>
#include <shlobj.h>

// ローカルファイルを開く。
void __fastcall XgOpenLocalFile(HWND hwnd, LPCWSTR pszFileName) noexcept {
	auto strPathName = XgFindLocalFile(pszFileName);
	if (strPathName.size())
		ShellExecuteW(hwnd, nullptr, strPathName.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// 文字列をエスケープする。
MStringW __fastcall xg_str_escape(const MStringW& str) {
	MStringW ret;
	ret.reserve(str.size());
	for (auto ch : str) {
		switch (ch)
		{
		case L'\a': ret += L"\\a"; break;
		case L'\b': ret += L"\\b"; break;
		case L'\t': ret += L"\\t"; break;
		case L'\n': ret += L"\\n"; break;
		case L'\r': ret += L"\\r"; break;
		case L'\f': ret += L"\\f"; break;
		case L'\v': ret += L"\\v"; break;
		case L'\\': ret += L"\\\\"; break;
		default:
			if (ch < 0x20 || ch == 0x7F) {
				WCHAR sz[8];
				StringCchPrintfW(sz, _countof(sz), L"\\%03o", ch);
				ret += sz;
			} else {
				ret += ch;
			}
		}
	}
	return ret;
}

// 文字列をアンエスケープする。
MStringW __fastcall xg_str_unescape(const MStringW& str) {
	MStringW ret;
	ret.reserve(str.size());
	for (size_t i = 0; i < str.size(); ++i) {
		auto ch = str[i];
		if (ch != L'\\') {
			ret += ch;
			continue;
		}
		ch = str[++i];
		switch (ch) {
		case L'a': ret += L"\a"; break;
		case L'b': ret += L"\b"; break;
		case L't': ret += L"\t"; break;
		case L'n': ret += L"\n"; break;
		case L'r': ret += L"\r"; break;
		case L'f': ret += L"\f"; break;
		case L'v': ret += L"\v"; break;
		case L'\\': ret += L"\\"; break;
		default:
			if (L'0' <= ch && ch <= L'7') { // octal
				int k, octal = 0;
				for (k = 0; k < 3; ++k) {
					ch = str[i + k];
					if (!(L'0' <= ch && ch <= L'7'))
						break;
					octal *= 8;
					octal += ch - L'0';
				}
				ret += static_cast<wchar_t>(octal);
				i += k - 1;
			}
			if (ch == L'x' || ch == L'X') {
				++i;
				int k, hexi = 0;
				for (k = 0; k < 4; ++k) {
					ch = str[i + k];
					if (!iswxdigit(ch))
						break;
					hexi *= 16;
					if (iswdigit(ch))
						hexi += ch - L'0';
					else if (iswlower(ch))
						hexi += ch - L'a' + 10;
					else if (iswupper(ch))
						hexi += ch - L'A' + 10;
				}
				ret += static_cast<wchar_t>(hexi);
				i += k - 1;
			}
		}
	}
	return ret;
}

// 文字列を引用する。
MStringW __fastcall xg_str_quote(const MStringW& str) {
	MStringW ret = L"\"";
	ret += xg_str_escape(str);
	ret += L"\"";
	return ret;
}

// 文字列を逆引用する。
MStringW __fastcall xg_str_unquote(const MStringW& str) {
	MStringW ret;
	ret = str;
	XgStrTrim(ret);
	if (ret.empty())
		return ret;
	if (ret[0] == L'"')
		ret = ret.substr(1);
	if (ret.size() && ret[ret.size() - 1] == L'"')
		ret = ret.substr(0, ret.size() - 1);
	return xg_str_unescape(ret);
}

// フィルター文字列を作る。
LPWSTR __fastcall XgMakeFilterString(LPWSTR psz) noexcept {
	// 文字列中の '|' を '\0' に変換する。
	for (LPWSTR pch = psz; *pch; ++pch) {
		if (*pch == L'|')
			*pch = UNICODE_NULL;
	}
	return psz;
}

// ショートカットのターゲットのパスを取得する。
BOOL __fastcall XgGetPathOfShortcutW(LPCWSTR pszLnkFile, LPWSTR pszPath) {
	IShellLinkW* pShellLink;
	IPersistFile* pPersistFile;
	WIN32_FIND_DATAW find;
	BOOL ret = FALSE;

	pszPath[0] = UNICODE_NULL;
	HRESULT hr = CoInitialize(nullptr);
	if (SUCCEEDED(hr)) {
		if (SUCCEEDED(hr = CoCreateInstance(CLSID_ShellLink, nullptr,
			CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *)&pShellLink)))
		{
			if (SUCCEEDED(hr = pShellLink->QueryInterface(IID_IPersistFile,
				(void **)&pPersistFile)))
			{
				hr = pPersistFile->Load(pszLnkFile, STGM_READ);
				if (SUCCEEDED(hr)) {
					if (SUCCEEDED(hr = pShellLink->GetPath(pszPath, MAX_PATH, &find, 0)))
					{
						ret = PathFileExistsW(pszPath);
					}
				}
				pPersistFile->Release();
			}
			pShellLink->Release();
		}
		CoUninitialize();
	}
	return ret;
}

// HTML形式のクリップボードデータを作成する。
MStringA XgMakeClipHtmlData(const MStringA& html_utf8,
							const MStringA& style_utf8/* = ""*/)
{
	MStringA str(
		"Version:0.9\r\n"
		"StartHTML:00000000\r\n"
		"EndHTML:00000000\r\n"
		"StartFragment:00000000\r\n"
		"EndFragment:00000000\r\n"
		"<html><head>\r\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\r\n"
		"<style type=\"text/css\"><!--\r\n");
	str += style_utf8;
	str += 
		"--></style></head>"
		"<body>\r\n"
		"<!-- StartFragment -->\r\n";
	str += html_utf8;
	str += "\r\n<!-- EndFragment -->\r\n";
	str += "</body></html>\r\n";

	const size_t iHtmlStart = str.find("<html>");
	const size_t iHtmlEnd = str.size();
	const size_t iFragmentStart = str.find("<!-- StartFragment -->");
	const size_t iFragmentEnd = str.find("<!-- EndFragment -->");

	char buf[9];
	size_t i;
	i = str.find("StartHTML:");
	i += 10;
	StringCchPrintfA(buf, _countof(buf), "%08u", UINT(iHtmlStart));
	str.replace(i, 8, buf);

	i = str.find("EndHTML:");
	i += 8;
	StringCchPrintfA(buf, _countof(buf), "%08u", UINT(iHtmlEnd));
	str.replace(i, 8, buf);

	i = str.find("StartFragment:");
	i += 14;
	StringCchPrintfA(buf, _countof(buf), "%08u", UINT(iFragmentStart));
	str.replace(i, 8, buf);

	i = str.find("EndFragment:");
	i += 12;
	StringCchPrintfA(buf, _countof(buf), "%08u", UINT(iFragmentEnd));
	str.replace(i, 8, buf);

	return str;
}

// HTML形式のクリップボードデータを作成する。
MStringA XgMakeClipHtmlData(const MStringW& html_wide,
							const MStringW& style_wide/* = L""*/)
{
	return XgMakeClipHtmlData(Utf8FromWide(html_wide), Utf8FromWide(style_wide));
}

// クリップボード用のテキストデータを用意する。
HGLOBAL XgCreateTextDataA(const MStringA& str) {
	// ヒープからメモリを確保する。
	const auto cb = (str.size() + 1) * sizeof(CHAR);
	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
	if (hGlobal) {
		// メモリをロックする。
		auto psz = static_cast<LPSTR>(::GlobalLock(hGlobal));
		if (psz) {
			// メモリに文字列をコピーする。
			::CopyMemory(psz, str.data(), cb);
			// メモリのロックを解除する。
			::GlobalUnlock(hGlobal);
		} else {
			::GlobalFree(hGlobal);
			hGlobal = nullptr;
		}
	}
	return hGlobal;
}

// クリップボード用のテキストデータを用意する。
HGLOBAL XgCreateTextDataW(const MStringW& str) {
	// ヒープからメモリを確保する。
	const auto cb = (str.size() + 1) * sizeof(WCHAR);
	HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cb);
	if (hGlobal) {
		// メモリをロックする。
		auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal));
		if (psz) {
			// メモリに文字列をコピーする。
			::CopyMemory(psz, str.data(), cb);
			// メモリのロックを解除する。
			::GlobalUnlock(hGlobal);
		} else {
			::GlobalFree(hGlobal);
			hGlobal = nullptr;
		}
	}
	return hGlobal;
}

// クリップボードからテキストを取得する。
MStringW XgGetClipboardUnicodeText(HWND hwnd) {
	MStringW str;

	// クリップボードを開く。
	if (::OpenClipboard(hwnd)) {
		// Unicode文字列を取得。
		HGLOBAL hGlobal = ::GetClipboardData(CF_UNICODETEXT);
		auto psz = static_cast<LPWSTR>(::GlobalLock(hGlobal));
		if (psz) {
			str = psz;
			::GlobalUnlock(hGlobal);
		}
		// クリップボードを閉じる。
		::CloseClipboard();
	}

	return str;
}

// クリップボードにテキストをコピーする。
BOOL XgSetClipboardUnicodeText(HWND hwnd, const MStringW& str) noexcept {
	// ヒープからメモリを確保する。
	HGLOBAL hGlobal = XgCreateTextDataW(str);
	if (hGlobal) {
		// クリップボードを開く。
		if (::OpenClipboard(hwnd)) {
			// クリップボードを空にする。
			if (::EmptyClipboard()) {
				// Unicodeテキストを設定。
				::SetClipboardData(CF_UNICODETEXT, hGlobal);
			}
			// クリップボードを閉じる。
			::CloseClipboard();
			return TRUE;
		}
		// 確保したメモリを解放する。
		::GlobalFree(hGlobal);
	}

	return FALSE;
}
