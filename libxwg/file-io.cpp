// file-io.cpp --- File Input/Output
// Author: katahiromz
// License: MIT

#include "file-io.h"
#include <array>
#include <shellapi.h>
#include <shlobj.h>

// 画像ファイルか？
BOOL __fastcall XgIsImageFile(LPCWSTR pszFileName) noexcept {
	LPCWSTR pchDotExt = PathFindExtensionW(pszFileName);
	if (lstrcmpiW(pchDotExt, L".bmp") == 0 ||
		lstrcmpiW(pchDotExt, L".dib") == 0 ||
		lstrcmpiW(pchDotExt, L".emf") == 0 ||
		lstrcmpiW(pchDotExt, L".gif") == 0 ||
		lstrcmpiW(pchDotExt, L".png") == 0 ||
		lstrcmpiW(pchDotExt, L".jpg") == 0 ||
		lstrcmpiW(pchDotExt, L".jpeg") == 0 ||
		lstrcmpiW(pchDotExt, L".jpe") == 0 ||
		lstrcmpiW(pchDotExt, L".jfif") == 0 ||
		lstrcmpiW(pchDotExt, L".tif") == 0 ||
		lstrcmpiW(pchDotExt, L".tiff") == 0)
	{
		return TRUE;
	}
	return FALSE;
}

// テキストファイルか？
BOOL __fastcall XgIsTextFile(LPCWSTR pszFileName) noexcept {
	LPCWSTR pchDotExt = PathFindExtensionW(pszFileName);
	return lstrcmpiW(pchDotExt, L".txt") == 0;
}

// ファイルを全部読み込む。
BOOL __fastcall XgReadFileAll(LPCWSTR file, MStringA& strBinary) {
	strBinary.clear();
	if (FILE *fin = _wfopen(file, L"rb")) {
		CHAR buf[1024];
		for (;;) {
			size_t count = fread(buf, 1, 1024, fin);
			if (!count)
				break;
			strBinary.append(&buf[0], &buf[count]);
		}

		fclose(fin);
		return TRUE;
	}
	return FALSE;
}

// ファイルを全部書き込む。
BOOL __fastcall XgWriteFileAll(LPCWSTR file, const MStringA& strBinary) noexcept {
	if (FILE *fout = _wfopen(file, L"wb")) {
		BOOL ret = TRUE;
		if (strBinary.size())
			ret = !!fwrite(strBinary.c_str(), 1, strBinary.size(), fout);
		fclose(fout);
		return ret;
	}
	return FALSE;
}

// ファイルが書き込み可能か？
BOOL __fastcall XgCanWriteFile(HWND hwnd, LPCWSTR pszFile) noexcept {
	// 書き込みをすべきでない特殊フォルダのID。
	static const INT c_anFolders[] = {
		CSIDL_PROGRAM_FILES,
		CSIDL_PROGRAM_FILES_COMMON,
		CSIDL_PROGRAM_FILES_COMMONX86,
		CSIDL_PROGRAM_FILESX86,
		CSIDL_SYSTEM,
		CSIDL_SYSTEMX86,
		CSIDL_WINDOWS
	};

	// 与えられたパスファイル名。
	MStringW str(pszFile);

	for (auto folder : c_anFolders) {
		// 特殊フォルダの位置の取得。
		LPITEMIDLIST pidl = nullptr;
		HRESULT hr = ::SHGetSpecialFolderLocation(hwnd, folder, &pidl);
		if (SUCCEEDED(hr)) {
			// 特殊フォルダのパスを得る。
			WCHAR szPath[MAX_PATH];
			::SHGetPathFromIDListW(pidl, szPath);
			::CoTaskMemFree(pidl);

			// パスが一致するか？
			if (str.find(szPath) == 0)
				return FALSE;   // ここには保存すべきでない。
		}
	}

	// 書き込みできるか？
	return _waccess(pszFile, 02) == 0;
}

// パスを作る。
BOOL __fastcall XgMakePathW(LPCWSTR pszPath) noexcept {
	WCHAR szPath[MAX_PATH];
	StringCchCopyW(szPath, _countof(szPath), pszPath);

	const DWORD attrs = ::GetFileAttributesW(szPath);
	if (attrs != 0xFFFFFFFF) {
		return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	LPWSTR pch = wcsrchr(szPath, L'\\');
	if (pch == nullptr) {
		return TRUE;
	}
	*pch = 0;

	if (XgMakePathW(szPath)) {
		return CreateDirectoryW(pszPath, nullptr);
	}
	return FALSE;
}

// エンディアン変換。
void __fastcall XgSwab(LPBYTE pbFile, size_t cbFile) noexcept {
	LPWORD pw = reinterpret_cast<LPWORD>(pbFile);
	size_t cw = (cbFile >> 1);
	while (cw--) {
		const WORD w = *pw;
		const BYTE lo = LOBYTE(w);
		const BYTE hi = HIBYTE(w);
		*pw = MAKEWORD(hi, lo);
		++pw;
	}
}

// For EclipseCrossword
static INT __fastcall XgCharSetToCodePage(INT charset) {
	switch (charset) {
	case ANSI_CHARSET:		  return 1252;
	case DEFAULT_CHARSET:	   return CP_ACP;
	case SYMBOL_CHARSET:		return CP_SYMBOL;
	case SHIFTJIS_CHARSET:	  return 932;
	case HANGEUL_CHARSET:	   return 949;
	case GB2312_CHARSET:		return 936;
	case CHINESEBIG5_CHARSET:   return 950;
	case OEM_CHARSET:		   return CP_OEMCP;
	case JOHAB_CHARSET:		 return 1255;
	case HEBREW_CHARSET:		return 1255;
	case ARABIC_CHARSET:		return 1256;
	case GREEK_CHARSET:		 return 1253;
	case TURKISH_CHARSET:	   return 1254;
	case VIETNAMESE_CHARSET:	return 1258;
	case THAI_CHARSET:		  return 874;
	case EASTEUROPE_CHARSET:	return 1250;
	case RUSSIAN_CHARSET:	   return 1251;
	case MAC_CHARSET:		   return CP_MACCP;
	case BALTIC_CHARSET:		return 1257;
	default:					return CP_ACP;
	}
}

// For EclipseCrossword
static INT __fastcall XgDetectCodePageFromEcw(const MStringA& strBinary) {
	auto index = strBinary.find("\nCodePage:");
	if (index == strBinary.npos)
		return CP_ACP;

	// The name is "CodePage" but codepage
	index += std::strlen("\nCodePage:");
	INT charset = atoi(&strBinary.c_str()[index]);

	return XgCharSetToCodePage(charset);
}

// テキストファイルを読み込む。
BOOL __fastcall XgReadTextFileAll(LPCWSTR file, MStringW& strText, BOOL ecw) {
	strText.clear();

	MStringA strBinary;
	if (!XgReadFileAll(file, strBinary))
		return FALSE;

	if (strBinary.empty())
		return TRUE;

	if (ecw) { // EclipseCrossword
		strText = WideFromAnsi(strBinary, XgDetectCodePageFromEcw(strBinary));
		return TRUE;
	}

	if (strBinary.size() >= 3) {
		if (memcmp(strBinary.c_str(), u8"\xEF\xBB\xBF", 3) == 0) { // UTF-8 BOM
			strText = WideFromUtf8(&strBinary[3]);
			return TRUE;
		}
		if (memcmp(strBinary.c_str(), "\xFF\xFE", 2) == 0) { // UTF-16 LE
			auto ptr = reinterpret_cast<LPWSTR>(&strBinary[2]);
			const size_t len = (strBinary.size() - 1) / sizeof(WCHAR);
			strText.assign(ptr, len);
			return TRUE;
		}
		if (memcmp(strBinary.c_str(), "\xFE\xFF", 2) == 0) { // UTF-16 BE
			auto ptr = reinterpret_cast<LPWSTR>(&strBinary[2]);
			const size_t len = (strBinary.size() - 1) / sizeof(WCHAR);
			strText.assign(ptr, len);
			XgSwab(reinterpret_cast<LPBYTE>(&strText[0]), len * sizeof(WCHAR));
			return TRUE;
		}
	}

	size_t index = 0;
	BOOL bUTF16LE = FALSE, bUTF16BE = FALSE;
	for (auto ch : strBinary) {
		if (ch == 0) {
			if (index & 1) {
				bUTF16LE = TRUE;
				if (bUTF16BE)
					break;
			} else {
				bUTF16BE = TRUE;
				if (bUTF16LE)
					break;
			}
		}

		++index;
		if (index >= strBinary.size())
			break;
	}

	if (bUTF16BE && bUTF16LE) {
		// binary
		strText = WideFromUtf8(strBinary);
		return TRUE;
	}

	if (bUTF16BE || bUTF16LE) {
		// UTF-16 BE/LE
		auto ptr = reinterpret_cast<LPCWSTR>(strBinary.c_str());
		const size_t len = strBinary.size() / sizeof(WCHAR);
		strText.assign(ptr, len);
		if (bUTF16BE)
			XgSwab(reinterpret_cast<LPBYTE>(&strText[0]), len * sizeof(WCHAR));
		return TRUE;
	}

	const BOOL bNotUTF8 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, strBinary.c_str(), -1,
											  nullptr, 0) == 0;
	const BOOL bNotAnsi = MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, strBinary.c_str(), -1,
											  nullptr, 0) == 0;
	if (!bNotUTF8 && bNotAnsi) {
		strText = WideFromUtf8(strBinary);
		return TRUE;
	}

	if (bNotUTF8 && !bNotAnsi) {
		strText = WideFromAnsi(strBinary);
		return TRUE;
	}

	MStringW strFromUtf8 = WideFromUtf8(strBinary);
	if (Utf8FromWide(strFromUtf8) == strBinary) {
		strText = std::move(strFromUtf8);
		return TRUE;
	}

	MStringW strFromAnsi = WideFromAnsi(strBinary);
	if (AnsiFromWide(strFromAnsi) == strBinary) {
		strText = std::move(strFromAnsi);
		return TRUE;
	}

	strText = WideFromUtf8(strBinary);
	return TRUE;
}
