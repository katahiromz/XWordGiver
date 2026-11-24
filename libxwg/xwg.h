// xwg.h --- Japanese Crossword Generator
// Author: katahiromz
// License: MIT

#pragma once

// Win32API headers
#ifndef _INC_WINDOWS
	#include <windows.h>	// Win32API
#endif
#ifndef _INC_WINDOWSX
	#include <windowsx.h>   // Win32API Macro APIs
#endif
#ifndef _INC_COMMCTRL
	#include <commctrl.h>   // common controls
#endif
#ifndef _INC_COMMDLG
	#include <commdlg.h>	// common dialogs
#endif
#ifndef _INC_SHLWAPI
	#include <shlwapi.h>	// Shell Light-Weight API
#endif
#ifndef _INC_TCHAR
	#include <tchar.h>	  // generic text mappings
#endif
#ifndef NO_STRSAFE
	#include <strsafe.h>	// StringCch* or StringCb*
#endif

#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cwchar>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>

#define XWG_CANCEL_EVENT L"xwg cancel event"
#define XWG_SHARED_NAME  L"xwg shared data"

extern HINSTANCE xg_hInstance; // アプリのインスタンス。
extern HWND xg_hMainWnd;
extern HANDLE xg_hHeap; // ヒープ。

// メッセージ。
#define XGWM_UNDO_UPDATING  (WM_APP + 1) // 「元に戻す」情報の更新開始。
#define XGWM_UNDO_UPDATED   (WM_APP + 2) // 「元に戻す」情報の更新完了。
#define XGWM_FILECHANGED	(WM_APP + 3) // ファイルが変更された。
#define XGWM_MARKSUPDATE	(WM_APP + 4) // 二重マスが更新された。
#define XGWM_GETTHUMBNAILEXTENT (WM_APP + 102) // サムネイルのサイズを取得。

// XGWM_UNDO_... wParam
#define XG_UNDO_ALL 0
#define XG_UNDO_BOARD 1
#define XG_UNDO_CLUES 2
#define XG_UNDO_VIEW 3
#define XG_UNDO_RULES 4
#define XG_UNDO_MARKS 5

// 日本語ロケール。
#define XG_JPN_LOCALE MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT), SORT_DEFAULT)

// ユーザーが日本語か？
BOOL __fastcall XgIsUserJapanese(void) noexcept;

// レジストリキー（会社名とアプリ名）。
#ifndef XG_REGKEY_APP
	#ifdef _WIN64
		#define XG_REGKEY_APP L"Software\\Katayama Hirofumi MZ\\XWord64"
	#else
		#define XG_REGKEY_APP L"Software\\Katayama Hirofumi MZ\\XWord32"
	#endif
#endif

///////////////////////////////////////////////////////////////////
// 文字列。

#include <string>
#include <cstring>
#include <strsafe.h>

#if 1
	#include "QString/QString.h"
	using MStringA = QStringA;
	using MStringW = QStringW;

	template <typename T>
	inline MStringA to_mstrA(const T& value) {
		return to_QStringA(value);
	}
	template <typename T>
	inline MStringW to_mstrW(const T& value) {
		return to_QStringW(value);
	}
#else
	using MStringA = std::string;
	using MStringW = std::wstring;

	template <typename T>
	inline MStringA to_mstrA(const T& value) {
		return std::to_string(value);
	}
	template <typename T>
	inline MStringW to_mstrW(const T& value) {
		return std::to_wstring(value);
	}
#endif

template <typename T_STR>
inline void XgStrTrimT(T_STR& str, const typename T_STR::value_type *spaces) {
	size_t i = str.find_first_not_of(spaces);
	size_t j = str.find_last_not_of(spaces);
	if ((i == T_STR::npos) || (j == T_STR::npos))
		str.clear();
	else
		str = str.substr(i, j - i + 1);
}
inline void XgStrTrimA(MStringA& str, LPCSTR spaces = " \t\r\n") {
	XgStrTrimT(str, spaces);
}
inline void XgStrTrimW(MStringW& str, LPCWSTR spaces = L" \t\r\n　") {
	XgStrTrimT(str, spaces);
}
inline void XgStrTrim(MStringA& str) {
	XgStrTrimA(str);
}
inline void XgStrTrim(MStringW& str) {
	XgStrTrimW(str);
}

template <typename T_STR>
inline void XgStrTrimRightT(T_STR& str, const typename T_STR::value_type *spaces) {
	size_t j = str.find_last_not_of(spaces);
	if (j == T_STR::npos)
		str.clear();
	else
		str = str.substr(0, j + 1);
}
inline void XgStrTrimRightA(MStringA& str, LPCSTR spaces = " \t\r\n") {
	XgStrTrimRightT(str, spaces);
}
inline void XgStrTrimRightW(MStringW& str, LPCWSTR spaces = L" \t\r\n　") {
	XgStrTrimRightT(str, spaces);
}
inline void XgStrTrimRight(MStringA& str) {
	XgStrTrimRightA(str);
}
inline void XgStrTrimRight(MStringW& str) {
	XgStrTrimRightW(str);
}

template <typename T_STR>
inline BOOL
XgStrReplace(T_STR& str, const T_STR& from, const T_STR& to) {
	BOOL ret = FALSE;
	size_t i = 0;
	for (;;) {
		i = str.find(from, i);
		if (i == T_STR::npos)
			break;
		ret = TRUE;
		str.replace(i, from.size(), to);
		i += to.size();
	}
	return ret;
}

template <typename T_STR>
inline BOOL
XgStrReplace(T_STR& str, const typename T_STR::value_type* from, const typename T_STR::value_type* to) {
	return XgStrReplace(str, T_STR(from), T_STR(to));
}

template <typename T_STR_CONTAINER>
inline void
XgStrSplit(T_STR_CONTAINER& container,
		   const typename T_STR_CONTAINER::value_type& str,
		   const typename T_STR_CONTAINER::value_type& chars)
{
	container.clear();
	size_t i = 0, k = str.find_first_of(chars);
	while (k != T_STR_CONTAINER::value_type::npos)
	{
		container.push_back(str.substr(i, k - i));
		i = k + 1;
		k = str.find_first_of(chars, i);
	}
	container.push_back(str.substr(i));
}

template <typename T_STR_CONTAINER>
inline typename T_STR_CONTAINER::value_type
XgStrJoin(const T_STR_CONTAINER& container,
		  const typename T_STR_CONTAINER::value_type& sep)
{
	typename T_STR_CONTAINER::value_type result;
	typename T_STR_CONTAINER::const_iterator it, end;
	it = container.begin();
	end = container.end();
	if (it != end)
	{
		result = *it;
		for (++it; it != end; ++it)
		{
			result += sep;
			result += *it;
		}
	}
	return result;
}

// Shift_JIS codepage
#define XG_SJIS_CODEPAGE 932

// 文字列リスト。
using MStringListA = std::vector<MStringA>;
using MStringListW = std::vector<MStringW>;

// 文字列集合。
using MStringSetA = std::unordered_set<MStringA>;
using MStringSetW = std::unordered_set<MStringW>;

// 文字列マップ。
using MStringMapA = std::unordered_map<MStringA, MStringA>;
using MStringMapW = std::unordered_map<MStringW, MStringW>;

#ifdef UNICODE
	using MString = MStringW;
	using MStringList = MStringListW;
	using MStringSet = MStringSetW;
	using MStringMap = MStringMapW;
#else
	using MString = MStringA;
	using MStringList = MStringListA;
	using MStringSet = MStringSetA;
	using MStringMap = MStringMapA;
#endif

// Console Unicode I/O
#define MStringA MStringA
#define MStringW MStringW
#include "conuni.h"

// リソース文字列を読み込む。
MStringW __fastcall XgLoadString(INT id);

// 文字種の変換。
MStringW __fastcall XgLCMapString(const MStringW& str, DWORD dwMapFlags);
MStringW __fastcall XgConvertString(const MStringW& str, BOOL bLowercase = FALSE, BOOL bHiragana = FALSE);

// クロスワードで扱いやすいように文字列を正規化する。
// カタカナ、全角、大文字になり、いくつかの文字が置き換わる。
MStringW __fastcall XgNormalizeString(const MStringW& str);

// 自然な文字列に変換する。
MStringW __fastcall XgNaturalizeString(const MStringW& str);

///////////////////////////////////////////////////////////////////
// 自動CloseHandle。

struct MAutoCloseHandle {
	HANDLE m_h = nullptr;
	explicit MAutoCloseHandle(HANDLE h = nullptr) noexcept : m_h(h) {}
	~MAutoCloseHandle() noexcept {
		Close();
	}
	MAutoCloseHandle(const MAutoCloseHandle&) = delete;
	MAutoCloseHandle& operator=(const MAutoCloseHandle&) = delete;
	void __fastcall Close() noexcept {
		if (m_h && m_h != INVALID_HANDLE_VALUE) {
			CloseHandle(m_h);
			m_h = nullptr;
		}
	}
	operator HANDLE() const noexcept {
		return m_h;
	}
	PHANDLE __fastcall operator&() noexcept {
		return &m_h;
	}
	void __fastcall operator=(HANDLE h) noexcept {
		Close();
		m_h = h;
	}
};

///////////////////////////////////////////////////////////////////
// 共有メモリー

template <typename T_BASE>
struct Shared : MAutoCloseHandle {
	typedef T_BASE value_type;
	static_assert(std::is_trivially_copyable<value_type>::value,
	              "Shared<T> requires a trivially copyable type");

	BOOL __fastcall Create(const value_type& initial = {}) noexcept {
		Close();
		m_h = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
		                         0, sizeof(value_type), XWG_SHARED_NAME);
		if (!m_h)
			return FALSE;
		return Write(initial);
	}

	BOOL __fastcall Open(BOOL bWrite = FALSE) noexcept {
		Close();
		m_h = OpenFileMappingW(bWrite ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ, TRUE, XWG_SHARED_NAME);
		return !!m_h;
	}

	BOOL __fastcall Read(value_type* value) noexcept {
		auto p = (const volatile value_type*)MapViewOfFile(m_h, FILE_MAP_READ, 0, 0, sizeof(value_type));
		if (!p)
			return FALSE;
		*value = *p;
		UnmapViewOfFile((LPCVOID)p);
		return TRUE;
	}

	BOOL __fastcall Write(const value_type& value) noexcept {
		auto p = (volatile value_type*)MapViewOfFile(m_h, FILE_MAP_WRITE, 0, 0, sizeof(value_type));
		if (!p)
			return FALSE;
		*p = value;
		UnmapViewOfFile((LPCVOID)p);
		return TRUE;
	}
};

typedef Shared<LONG> SharedLong;

///////////////////////////////////////////////////////////////////
// 自動fclose。

struct AutoFClose {
	FILE *m_fp;

	AutoFClose(FILE *fp = nullptr) noexcept : m_fp(fp) { }

	~AutoFClose() noexcept {
		Close();
	}

	AutoFClose(const AutoFClose&) = delete;
	AutoFClose& operator=(const AutoFClose&) = delete;

	void __fastcall Close() noexcept {
		if (m_fp) {
			fclose(m_fp);
			m_fp = nullptr;
		}
	}

	operator FILE*() const noexcept {
		return m_fp;
	}
	void __fastcall operator=(FILE *fp) noexcept {
		Close();
		m_fp = fp;
	}
};

///////////////////////////////////////////////////////////////////
// 文字の判定。

// 半角大文字英字か？
constexpr inline bool XgIsCharHankakuUpperW(WCHAR ch) noexcept {
	return (L'A' <= ch && ch <= L'Z');
}

// 半角小文字英字か？
constexpr inline bool XgIsCharHankakuLowerW(WCHAR ch) noexcept {
	return (L'a' <= ch && ch <= L'z');
}

// 半角英字か？
constexpr inline bool XgIsCharHankakuAlphaW(WCHAR ch) noexcept {
	return XgIsCharHankakuUpperW(ch) || XgIsCharHankakuLowerW(ch);
}

// 全角大文字英字か？
constexpr inline bool XgIsCharZenkakuUpperW(WCHAR ch) noexcept {
	return (L'Ａ' <= ch && ch <= L'Ｚ');
}

// 全角小文字英字か？
constexpr inline bool XgIsCharZenkakuLowerW(WCHAR ch) noexcept {
	return (L'ａ' <= ch && ch <= L'ｚ');
}

// 全角英字か？
constexpr inline bool XgIsCharZenkakuAlphaW(WCHAR ch) noexcept {
	return XgIsCharZenkakuUpperW(ch) || XgIsCharZenkakuLowerW(ch);
}

// ひらがなか？
constexpr inline bool XgIsCharHiraganaW(WCHAR ch) noexcept {
	return ((L'ぁ' <= ch && ch <= L'ん') || ch == L'ー' || ch == L'ゔ');
}

// カタカナか？
constexpr inline bool XgIsCharKatakanaW(WCHAR ch) noexcept {
	return ((L'ァ' <= ch && ch <= L'ン') || ch == L'ー' || ch == L'ヴ');
}

// かなか？
constexpr inline bool XgIsCharKanaW(WCHAR ch) noexcept {
	return XgIsCharHiraganaW(ch) || XgIsCharKatakanaW(ch);
}

// 漢字か？
constexpr inline bool XgIsCharKanjiW(WCHAR ch) noexcept {
	return ((0x3400 <= ch && ch <= 0x9FFF) || (0xF900 <= ch && ch <= 0xFAFF) || ch == 0x3007);
}

// ハングルか？
constexpr inline bool XgIsCharHangulW(WCHAR ch) noexcept {
	return ((0x1100 <= ch && ch <= 0x11FF) ||
			(0xAC00 <= ch && ch <= 0xD7A3) ||
			(0x3130 <= ch && ch <= 0x318F));
}

// キリル文字か？
constexpr inline bool XgIsCharZenkakuCyrillicW(WCHAR ch) noexcept {
	return 0x0400 <= ch && ch <= 0x04FF;
}

// 全角数字か？
constexpr inline bool XgIsCharZenkakuNumericW(WCHAR ch) noexcept {
	return 0xFF10 <= ch && ch <= 0xFF19;
}
// 半角数字か？
constexpr inline bool XgIsCharHankakuNumericW(WCHAR ch) noexcept {
	return L'0' <= ch && ch <= L'9';
}

// ギリシャ文字か？
constexpr inline bool XgIsCharGreekW(WCHAR ch) noexcept {
	return 0x0370 <= ch && ch <= 0x03FF;
}

///////////////////////////////////////////////////////////////////
// クロスワード。

// クロスワードの盤面サイズの制限。
#define XG_MIN_SIZE 2
#define XG_MAX_SIZE 256

// 盤面サイズのデフォルト値。
#define XG_DEFAULT_BOARD_SIZE 7

// ビューモード。
typedef enum XG_VIEW_MODE : INT {
	XG_VIEW_NORMAL, // 通常ビュー。
	XG_VIEW_SKELETON // スケルトンビュー。
} XG_VIEW_MODE;

// クロスワードでよく使う文字。
enum : WCHAR {
	ZEN_SPACE	  = L'　', // U+3000: 全角スペース、空白マスと見なす
	ZEN_UNDERLINE  = L'＿', // U+FF3F: 全角のアンダースコア、空白マスと見なす
	ZEN_SHARP1	 = L'♯', // U+266F: 黒マスと見なす
	ZEN_SHARP2	 = L'＃', // U+FF03: 黒マスと見なす
	ZEN_DOT		= L'．', // U+FF0E: 全角ドット(ピリオド)、立入禁止。クロスワード ギバーでは黒マスと見なす
	ZEN_BLACK	  = L'■', // U+25A0: 黒マス
	ZEN_PROLONG	= L'ー', // U+30FC: 伸ばす音
	ZEN_ULEFT	  = L'┏', // U+250F: 太い罫線
	ZEN_URIGHT	 = L'┓', // U+2513: 太い罫線
	ZEN_LLEFT	  = L'┗', // U+2517: 太い罫線
	ZEN_LRIGHT	 = L'┛', // U+251B: 太い罫線
	ZEN_VLINE	  = L'┃', // U+2503: 太い罫線
	ZEN_HLINE	  = L'━', // U+2501: 太い罫線
};

// 文字マスの文字か判定する。ただし、XgNormalizeStringで正規化された文字が対象。
inline BOOL __fastcall XgIsLetter(WCHAR ch) noexcept {
	//return ch != ZEN_BLACK && ch != ZEN_SPACE && ch != ZEN_DOT; // 立入禁止を考慮する場合
	return ch != ZEN_BLACK && ch != ZEN_SPACE;
}

// 黒マスルール。
enum : WORD {
	RULE_DONTDOUBLEBLACK = (1 << 0),	// 連黒禁。
	RULE_DONTCORNERBLACK = (1 << 1),	// 四隅黒禁。
	RULE_DONTTRIDIRECTIONS = (1 << 2),  // 三方黒禁。
	RULE_DONTDIVIDE = (1 << 3),		 // 分断禁。
	RULE_DONTFOURDIAGONALS = (1 << 4),  // 黒斜四連禁。
	RULE_POINTSYMMETRY = (1 << 5),	  // 黒マス点対称。
	RULE_DONTTHREEDIAGONALS = (1 << 6), // 黒斜三連禁。
	RULE_LINESYMMETRYV = (1 << 7),	  // 黒マス線対称（タテ）。
	RULE_LINESYMMETRYH = (1 << 8),	  // 黒マス線対称（ヨコ）。
	// 標準ルール。
	XG_STANDARD_RULES = (RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK | \
						 RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE |
						 RULE_DONTTHREEDIAGONALS | RULE_POINTSYMMETRY),
	// 正当なルール。
	XG_VALID_RULES = (RULE_DONTDOUBLEBLACK | RULE_DONTCORNERBLACK |
					  RULE_DONTTRIDIRECTIONS | RULE_DONTDIVIDE |
					  RULE_DONTFOURDIAGONALS | RULE_POINTSYMMETRY |
					  RULE_DONTTHREEDIAGONALS | RULE_LINESYMMETRYV |
					  RULE_LINESYMMETRYH),
	// RULE_DONTTHREEDIAGONALS と RULE_DONTFOURDIAGONALS は互いに排他的に扱う。
	// RULE_POINTSYMMETRY と RULE_LINESYMMETRYV と RULE_LINESYMMETRYH は互いに排他的に扱う。
};

extern INT xg_nRules;

HRESULT __fastcall XgInitialize(void);
void __fastcall XgUninitialize(void);
void __fastcall XgUnitTest(void);

// 単語の長さの制限。
#define XG_MIN_WORD_LEN 2
#define XG_MAX_WORD_LEN 100

///////////////////////////////////////////////////////////////////
// 盤面と位置。

// 盤面上の位置。
struct MBoardPos {
	INT m_j = 0;
	INT m_i = 0;
	MBoardPos() noexcept : m_j(0), m_i(0) { }
	MBoardPos(INT j, INT i) noexcept : m_j(j), m_i(i) { }
	bool operator==(const MBoardPos& another) const noexcept {
		return m_j == another.m_j && m_i == another.m_i;
	}
	bool operator!=(const MBoardPos& another) const noexcept {
		return !(*this == another);
	}
};

// 単語の位置。
struct MWordPos : MBoardPos {
	BOOL m_bDown = FALSE;
	MWordPos() noexcept : MBoardPos() { }
	MWordPos(INT j, INT i, BOOL bDown) noexcept : MBoardPos(j, i), m_bDown(bDown) { }
	bool operator==(const MWordPos& another) const noexcept {
		return m_j == another.m_j && m_i == another.m_i && m_bDown == another.m_bDown;
	}
	bool operator!=(const MWordPos& another) const noexcept {
		return !(*this == another);
	}
};

namespace std {
	template <>
	struct hash<MBoardPos> {
	public:
		size_t operator()(const MBoardPos& pos) const noexcept {
			return pos.m_j | (pos.m_i << 16);
		}
	};
	template <>
	struct hash<MWordPos> {
	public:
		size_t operator()(const MWordPos& pos) const noexcept {
			return pos.m_j | (pos.m_i << 16) | (pos.m_bDown ? 0x80000000 : 0);
		}
	};
}

///////////////////////////////////////////////////////////////////
// MBod: 盤面の内部データ。

enum : WORD { BOD_MAGIC = MAKEWORD(0xB0, 0xD0) };

struct MBod {
	// フラットなデータ構造で高速化。メモリのキャッシュに乗ることが期待できる。
	WORD m_magic; // BOD_MAGIC
	WORD m_rules; // ルール値
	SHORT m_columns; // 列数
	SHORT m_rows; // 行数
	DWORD m_nonspace_count; // 非スペース文字の個数
	WCHAR m_cells[0]; // 正規化された文字列。構造体 MBod のサイズは可変。

	// コンストラクタも代入も使わせない。
	MBod() = delete;
	MBod(const MBod&) = delete;
	MBod(MBod&&) = delete;
	void operator=(const MBod&) = delete;
	void operator=(MBod&&) = delete;

	static void operator delete(void* ptr) {
		HeapFree(xg_hHeap, 0, ptr);
	}
	static void operator delete[](void* ptr) {
		HeapFree(xg_hHeap, 0, ptr);
	}

	// MBodのサイズ。
	static size_t __fastcall size(INT columns, INT rows) {
		return sizeof(MBod) + (columns * rows) * sizeof(WCHAR);
	}
	size_t __fastcall size() const {
		return size(m_columns, m_rows);
	}

	// マスの個数。
	size_t __fastcall count() const {
		return m_columns * m_rows;
	}

	// 比較。
	bool __fastcall operator==(const MBod& another) const {
		size_t cb1 = size();
		size_t cb2 = another.size();
		return (cb1 == cb2 && memcmp(this, &another, cb1) == 0);
	}
	bool __fastcall operator!=(const MBod& another) const {
		return !(*this == another);
	}

	// MBodの作成。
	static MBod* __fastcall create(INT columns, INT rows, INT rules, WCHAR chFill = ZEN_SPACE) {
		size_t siz = size(columns, rows);
		auto pBod = (MBod*)HeapAlloc(xg_hHeap, 0, siz);
		if (!pBod)
			throw std::bad_alloc();
		pBod->m_magic = BOD_MAGIC;
		pBod->m_rules = SHORT(rules);
		pBod->m_columns = SHORT(columns);
		pBod->m_rows = SHORT(rows);
		size_t cch = columns * rows;
		pBod->m_nonspace_count = DWORD((chFill != ZEN_SPACE) ? cch : 0);
		LPWSTR pch = pBod->m_cells;
		while (cch--) {
			*pch++ = chFill;
		}
		return pBod;
	}

	// MBodの複製。
	MBod* __fastcall clone() const {
		size_t siz = size();
		auto pBod = (MBod*)HeapAlloc(xg_hHeap, 0, siz);
		if (!pBod)
			throw std::bad_alloc();
		memcpy(pBod, this, siz);
		return pBod;
	}

	// 盤面のマスを取得。リリース時チェックなし。
	WCHAR __fastcall get_cell0(INT jColumn, INT iRow) const noexcept {
		assert(0 <= jColumn && jColumn < m_columns);
		assert(0 <= iRow && iRow < m_rows);
		return m_cells[jColumn + iRow * m_columns];
	}

	// 盤面のマスを取得。チェックあり。
	WCHAR __fastcall get_cell(INT jColumn, INT iRow, WCHAR chDefault = ZEN_BLACK) const noexcept {
		if (jColumn < m_columns && iRow < m_rows) {
			if (jColumn >= 0 && iRow >= 0) {
				return m_cells[jColumn + iRow * m_columns];
			}
		}
		return chDefault;
	}

	// 盤面のマスをセット。リリース時チェックなし。
	void __fastcall set_cell_0(INT jColumn, INT iRow, WCHAR chNew) noexcept {
		assert(0 <= jColumn && jColumn < m_columns);
		assert(0 <= iRow && iRow < m_rows);
		m_cells[jColumn + iRow * m_columns] = chNew;
	}

	// 盤面のマスをセット。チェックあり。
	void __fastcall set_cell(INT jColumn, INT iRow, WCHAR chNew) noexcept {
		if (jColumn < m_columns && iRow < m_rows) {
			if (jColumn >= 0 && iRow >= 0) {
				m_cells[jColumn + iRow * m_columns] = chNew;
			}
		}
	}

	// 盤面のマスを取得＆セット。
	WCHAR __fastcall get_set_cell(INT jColumn, INT iRow, WCHAR chNew, WCHAR chDefault) noexcept {
		WCHAR chOld = chDefault;
		if (jColumn < m_columns && iRow < m_rows) {
			if (jColumn >= 0 && iRow >= 0) {
				const INT ich = jColumn + iRow * m_columns;
				chOld = m_cells[ich];
				m_cells[ich] = chNew;
			}
		}
		return chOld;
	}
};

// 盤面データ。
struct MBoard {
	MBod* m_ptr = nullptr;
	MBod* m_allocated = nullptr; // Allocated by MBod::create/clone

	///////////////////////////////////////////////////////////////////
	// コンストラクタとデストラクタ

	MBoard(INT columns, INT rows, INT rules, WCHAR chFill = ZEN_SPACE) {
		assert(XG_MIN_SIZE <= columns && columns <= XG_MAX_SIZE);
		assert(XG_MIN_SIZE <= rows && rows <= XG_MAX_SIZE);
		m_allocated = MBod::create(columns, rows, rules, chFill);
		m_ptr = m_allocated;
	}

	MBoard() : MBoard(XG_DEFAULT_BOARD_SIZE, XG_DEFAULT_BOARD_SIZE, xg_nRules) { }

	MBoard(const MBoard& another) {
		m_allocated = another.m_ptr->clone();
		m_ptr = m_allocated;
	}

	MBoard(MBoard&& another) noexcept {
		m_allocated = another.m_allocated;
		another.m_allocated = nullptr;
		m_ptr = another.m_ptr;
		another.m_ptr = nullptr;
	}

	MBoard(MBod* ptr, BOOL bOwnership = FALSE) noexcept {
		if (bOwnership)
			m_allocated = ptr;
		m_ptr = ptr;
	}

	~MBoard() noexcept {
		Close();
	}

	///////////////////////////////////////////////////////////////////
	// 比較。

	bool __fastcall operator==(const MBoard& another) const {
		return *m_ptr == *another.m_ptr;
	}
	bool __fastcall operator!=(const MBoard& another) const {
		return *m_ptr != *another.m_ptr;
	}

	///////////////////////////////////////////////////////////////////
	// 閉じる。

	void __fastcall Close() noexcept {
		if (m_allocated) {
			delete m_allocated;
			m_allocated = nullptr;
		}
		m_ptr = nullptr;
	}

	///////////////////////////////////////////////////////////////////
	// 代入。

	void __fastcall operator=(const MBoard& another) {
		if (this != &another) {
			Close();
			m_allocated = another.m_ptr->clone();
			m_ptr = m_allocated;
		}
	}
	void __fastcall operator=(MBoard&& another) noexcept {
		if (this != &another) {
			Close();
			m_allocated = another.m_allocated;
			another.m_allocated = nullptr;
			m_ptr = another.m_ptr;
			another.m_ptr = nullptr;
		}
	}

	///////////////////////////////////////////////////////////////////
	// アクセサ。

	// 列の個数。
	INT __fastcall Columns() const noexcept { return m_ptr->m_columns; }
	// 行の個数。
	INT __fastcall Rows() const noexcept { return m_ptr->m_rows; }
	// サイズをセット。
	void __fastcall SetSize(INT columns, INT rows, WCHAR chFill = ZEN_SPACE) {
		assert(XG_MIN_SIZE <= columns && columns <= XG_MAX_SIZE);
		assert(XG_MIN_SIZE <= rows && rows <= XG_MAX_SIZE);
		auto rules = Rules();
		Close();
		m_allocated = MBod::create(columns, rows, rules, chFill);
		m_ptr = m_allocated;
	}
	// マスの個数。
	INT __fastcall Count() const noexcept {
		return INT(m_ptr->count());
	}

	// ルール。
	WORD& __fastcall Rules() const noexcept {
		return m_ptr->m_rules;
	}

	// データ。
	MStringW Data() const {
		return MStringW(&m_ptr->m_cells[0], &m_ptr->m_cells[m_ptr->count()]);
	}

	///////////////////////////////////////////////////////////////////
	// 非空白マスの個数。

	// 非空白マスの個数。SetCellでカウントされる。
	DWORD& __fastcall NonSpaceCount() const noexcept {
		return m_ptr->m_nonspace_count;
	}
	// 非空白マスの個数を数え直し。
	void __fastcall RecountNonSpaces() noexcept {
		INT ich, cch = Count(), nNonSpaces = 0;
		for (ich = 0; ich < cch; ++ich) {
			WCHAR ch = GetAt0(ich);
			if (ch != ZEN_SPACE)
				++nNonSpaces;
		}
		NonSpaceCount() = nNonSpaces;
	}

	///////////////////////////////////////////////////////////////////
	// マスを取得。

	WCHAR __fastcall GetCell0(INT jColumn, INT iRow) const noexcept {
		return m_ptr->get_cell0(jColumn, iRow);
	}
	WCHAR __fastcall GetCell(INT jColumn, INT iRow, WCHAR chDefault = ZEN_BLACK) const noexcept {
		return m_ptr->get_cell(jColumn, iRow, chDefault);
	}
	WCHAR __fastcall GetCell(const MBoardPos& pos) const noexcept {
		return GetCell(pos.m_j, pos.m_i);
	}

	// マスを取得。インデックスアクセス。
	WCHAR __fastcall GetAt0(INT ich) const noexcept {
		assert(0 <= ich && ich < Count());
		return m_ptr->m_cells[ich];
	}
	WCHAR __fastcall GetAt(INT ich, WCHAR chDefault = ZEN_BLACK) const noexcept {
		INT cch = Count();
		if (ich < cch && ich >= 0)
			return m_ptr->m_cells[ich];
		return chDefault;
	}

	// 対称性ルールに従った対称位置（自分自身を含む、最大8個）を取得。個数を返す。
	INT __fastcall GetMirroredPositions(INT jColumn, INT iRow, MBoardPos out[8]) const noexcept;

	///////////////////////////////////////////////////////////////////
	// マスの書き換え。

	// クリアする。
	void __fastcall clear(WCHAR ch = ZEN_SPACE) noexcept {
		INT cch = Count();
		LPWSTR cells = m_ptr->m_cells;
		while (cch--) {
			*cells++ = ch;
		}
		NonSpaceCount() = (ch != ZEN_SPACE) ? Count() : 0;
	}

	// マスをセット。カウントなし。
	void __fastcall SetCell0(INT jColumn, INT iRow, WCHAR chNew) noexcept {
		m_ptr->set_cell(jColumn, iRow, chNew);
	}
	// マスをセット。カウントあり。
	void SetCell(INT j, INT i, WCHAR chNew) noexcept {
		if (j < 0 || j >= Columns() || i < 0 || i >= Rows()) return;
		WCHAR& cell = m_ptr->m_cells[j + i * Columns()];
		NonSpaceCount() += (cell == ZEN_SPACE) - (chNew == ZEN_SPACE);
		cell = chNew;
	}
	void __fastcall SetCell(const MBoardPos& pos, WCHAR chNew) noexcept {
		SetCell(pos.m_j, pos.m_i, chNew);
	}

	// 対称性ルールに従って、対称位置にも同じマスをセットする。カウントあり。
	void __fastcall MirroredSetCell(INT jColumn, INT iRow, WCHAR chNew) noexcept;

	// マスを設定。インデックスアクセス。
	void __fastcall SetAt0(INT ich, WCHAR chNew) noexcept {
		assert(0 <= ich && ich < Count());
		m_ptr->m_cells[ich] = chNew;
	}
	void __fastcall SetAt(INT ich, WCHAR chNew) noexcept {
		if (!(0 <= ich && ich < Count()))
			return;
		WCHAR chOld = m_ptr->m_cells[ich];
		m_ptr->m_cells[ich] = chNew;
		INT diff_count = 0;
		if (chOld == ZEN_SPACE)
			++diff_count;
		if (chNew == ZEN_SPACE)
			--diff_count;
		NonSpaceCount() += diff_count;
	}

	// 文字マスをクリアする。
	void ClearLetters();

	/////////////////////////////////////////////////////////////////////////
	// 判定。

	// 空か？
	BOOL __fastcall empty() noexcept {
		INT cch = Count();
		LPWSTR cells = m_ptr->m_cells;
		while (cch--) {
			if (*cells++ != ZEN_SPACE)
				return FALSE;
		}
		return TRUE;
	}
	// いっぱいか？
	BOOL __fastcall IsFull() const noexcept {
		return Count() == NonSpaceCount();
	}

	// 四隅に黒マスがあるかどうか。
	BOOL __fastcall CornerBlack() const noexcept {
		return (GetCell0(0, 0) == ZEN_BLACK ||
				GetCell0(0, Rows() - 1) == ZEN_BLACK ||
				GetCell0(Columns() - 1, Rows() - 1) == ZEN_BLACK ||
				GetCell0(Columns() - 1, 0) == ZEN_BLACK);
	}
	// 黒マスが隣り合っているか？
	BOOL __fastcall DoubleBlack() const noexcept;
	// 三方向が黒マスで囲まれたマスがあるかどうか？
	BOOL __fastcall TriBlackAround() const noexcept;
	// 黒マスで分断されているかどうか？
	BOOL __fastcall DividedByBlack() const;
	// 黒斜三連か？
	BOOL __fastcall ThreeDiagonals() const noexcept;
	// 黒斜四連か？
	BOOL __fastcall FourDiagonals() const noexcept;
	// 黒マスが点対称か？
	BOOL __fastcall IsPointSymmetry() const noexcept;
	// 黒マスが線対称（タテ）か？
	BOOL __fastcall IsLineSymmetryV() const noexcept;
	// 黒マスが線対称（ヨコ）か？
	BOOL __fastcall IsLineSymmetryH() const noexcept;
	// おそらく黒マスを置けるか？
	BOOL __fastcall MaybeCanPutBlack(INT jColumn, INT iRow) const noexcept;

	// ルールに適合するか？
	BOOL __fastcall AreBlacksOK(INT rules) const;
	BOOL __fastcall AreBlacksOK() const;
	// 違反するルールを返す。
	INT __fastcall GetViolatedRules(INT rules) const;
	INT __fastcall GetViolatedRules() const;

	// 多分大丈夫か？
	BOOL __fastcall MaybeOK() const;
};

// 文字列の配列から盤面を作る。
MBoard __fastcall XgMakeBoard(const std::vector<const wchar_t *>& rows, INT nRules = 0);

// 盤面のルールチェックを行う。
BOOL __fastcall XgRuleCheck(const MBoard& board, BOOL bLoose = TRUE);

#ifndef XW_LOCAL_DIR
	#define XW_LOCAL_DIR nullptr
#endif

// ローカルのファイルを探す。
MStringW __fastcall XgFindLocalFile(LPCWSTR pszFileName, LPCWSTR pszPathToAdd = XW_LOCAL_DIR);

// アルゴリズムの終了直前に1回だけ呼び出される関数。
using xg_algo_quit_fn_t = std::function<void(HRESULT hr, std::vector<MBoard>& solutions, size_t max_solutions, MStringW& strError)>;
