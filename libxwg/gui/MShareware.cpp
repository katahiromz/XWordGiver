// MShareware.cpp -- The shareware maker for Win32
////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <sddl.h>
#include <tchar.h>
#include <strsafe.h>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "MSha256.h"
#include <string>
#include "MShareware.h"

#define sw_shareware_max_password 256

////////////////////////////////////////////////////////////////////////////
// Security helpers (private to this file)
//
// How the trial / license state is protected:
//  * The trial state (start time, last-use time, version) and the license
//    are stored as binary records that carry an HMAC-SHA256 tag. The HMAC
//    key is derived from the current user's SID, so records cannot simply be
//    edited by hand or copied to another account / PC.
//  * Each record is stored in THREE independent places (application registry
//    key, a second registry key, and a hidden file in %LOCALAPPDATA%).
//    On load, the earliest start time and the latest last-use time among the
//    valid copies win, so restoring only one old copy does not help.
//  * Any problem reading/writing the state fails CLOSED (no free trial).
//
// NOTE: This is deterrence against casual tampering. Because everything runs
// on the user's PC, a determined attacker who patches the binary can still
// defeat it. See GetMacSecret() to add a build-specific secret.

namespace {

const DWORDLONG SW_100NS_PER_SEC   = 10000000ull;
const DWORDLONG SW_100NS_PER_MIN   = 60ull * SW_100NS_PER_SEC;
// How far the clock may be behind the last observed time (NTP corrections etc.)
const DWORDLONG SW_CLOCK_TOLERANCE = 5ull * SW_100NS_PER_MIN;
const DWORD     SW_MAX_TRIAL_DAYS  = 36500;
const DWORD     SW_MAX_BLOB        = 4096;
const size_t    SW_MAX_VERSION     = 31;

const DWORD SW_TRIAL_MAGIC   = 0x31545753; // "SWT1"
const DWORD SW_LICENSE_MAGIC = 0x314C5753; // "SWL1"

#pragma pack(push, 4)
struct SW_TRIAL_RECORD {
	DWORD    dwMagic;
	DWORD    dwReserved;
	FILETIME ftStart;
	FILETIME ftLastUse;
	char     szVersion[SW_MAX_VERSION + 1];
	BYTE     abMac[32];
};
struct SW_LICENSE_RECORD {
	DWORD    dwMagic;
	DWORD    dwReserved;
	FILETIME ftRegistered;
	BYTE     abMac[32];
};
#pragma pack(pop)

inline DWORDLONG SwFtToQ(const FILETIME& ft) {
	ULARGE_INTEGER u;
	u.LowPart = ft.dwLowDateTime;
	u.HighPart = ft.dwHighDateTime;
	return u.QuadPart;
}

inline FILETIME SwQToFt(DWORDLONG q) {
	FILETIME ft;
	ft.dwLowDateTime = static_cast<DWORD>(q & 0xFFFFFFFF);
	ft.dwHighDateTime = static_cast<DWORD>(q >> 32);
	return ft;
}

// Constant-time comparison (do not leak where the first difference is).
bool SwSecureEquals(const void *a, const void *b, size_t n) {
	const BYTE *pa = static_cast<const BYTE *>(a);
	const BYTE *pb = static_cast<const BYTE *>(b);
	BYTE diff = 0;
	for (size_t i = 0; i < n; ++i)
		diff |= static_cast<BYTE>(pa[i] ^ pb[i]);
	return diff == 0;
}

bool SwSecureEquals(const std::string& a, const std::string& b) {
	if (a.size() != b.size())
		return false;
	return SwSecureEquals(a.data(), b.data(), a.size());
}

std::string SwToUtf8(const MShareware::tstring& str) {
#ifdef UNICODE
	if (str.empty())
		return std::string();
	int n = ::WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
								  nullptr, 0, nullptr, nullptr);
	if (n <= 0)
		return std::string();
	std::string s(static_cast<size_t>(n), '\0');
	::WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()),
						  &s[0], n, nullptr, nullptr);
	return s;
#else
	return str;
#endif
}

std::string SwToLowerAscii(const std::string& str) {
	std::string s = str;
	for (size_t i = 0; i < s.size(); ++i) {
		if ('A' <= s[i] && s[i] <= 'Z')
			s[i] = static_cast<char>(s[i] - 'A' + 'a');
	}
	return s;
}

// HMAC-SHA256 (RFC 2104) built on MSha256.
void SwHmacSha256(const std::string& key, const void *data, size_t len, BYTE out[32]) {
	BYTE k[64];
	memset(k, 0, sizeof(k));
	if (key.size() > 64) {
		MSha256 h;
		h.add_data(key.data(), key.size());
		h.generate_binary(k);
	} else if (!key.empty()) {
		memcpy(k, key.data(), key.size());
	}

	BYTE ipad[64], opad[64], inner[32];
	for (int i = 0; i < 64; ++i) {
		ipad[i] = static_cast<BYTE>(k[i] ^ 0x36);
		opad[i] = static_cast<BYTE>(k[i] ^ 0x5C);
	}

	MSha256 a;
	a.add_data(ipad, 64);
	a.add_data(data, len);
	a.generate_binary(inner);

	MSha256 b;
	b.add_data(opad, 64);
	b.add_data(inner, 32);
	b.generate_binary(out);

	SecureZeroMemory(k, sizeof(k));
	SecureZeroMemory(ipad, sizeof(ipad));
	SecureZeroMemory(opad, sizeof(opad));
}

// The SID string of the current user, e.g. "S-1-5-21-...".
std::string SwGetUserSidString() {
	std::string ret;
	HANDLE hToken = nullptr;
	if (::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
		DWORD cb = 0;
		::GetTokenInformation(hToken, TokenUser, nullptr, 0, &cb);
		if (cb > 0 && cb <= 4096) {
			std::vector<BYTE> buf(cb);
			if (::GetTokenInformation(hToken, TokenUser, buf.data(), cb, &cb)) {
				LPSTR psz = nullptr;
				TOKEN_USER *ptu = reinterpret_cast<TOKEN_USER *>(buf.data());
				if (::ConvertSidToStringSidA(ptu->User.Sid, &psz)) {
					ret = psz;
					::LocalFree(psz);
				}
			}
		}
		::CloseHandle(hToken);
	}
	if (ret.empty()) { // fallback
		char szUser[257];
		DWORD cch = 257;
		if (::GetUserNameA(szUser, &cch))
			ret = szUser;
	}
	return ret;
}

std::string SwSha256Hex(const std::string& str) {
	MSha256 h;
	h.add_data(str.data(), str.size());
	std::string hex;
	h.generate_hash_string(hex);
	return hex;
}

// Seal / verify the trailing HMAC of a record.
template <typename T_RECORD>
void SwSealRecord(T_RECORD& rec, const std::string& key) {
	SwHmacSha256(key, &rec, offsetof(T_RECORD, abMac), rec.abMac);
}

template <typename T_RECORD>
bool SwLoadRecord(const std::vector<BYTE>& blob, const std::string& key,
				  DWORD dwMagic, T_RECORD& rec) {
	if (blob.size() != sizeof(T_RECORD))
		return false;
	memcpy(&rec, blob.data(), sizeof(T_RECORD));
	if (rec.dwMagic != dwMagic)
		return false;
	BYTE mac[32];
	SwHmacSha256(key, &rec, offsetof(T_RECORD, abMac), mac);
	return SwSecureEquals(mac, rec.abMac, sizeof(mac));
}

// Redundant storage: registry (application key), registry (second key) and a
// hidden file. A "kind" ('T' = trial, 'L' = license) selects separate names.
class SwBlobStore {
public:
	enum { COUNT = 3 };

	SwBlobStore(const MShareware::tstring& appKey, const std::string& seed, char kind)
		: m_appKey(appKey)
	{
		const std::string tag = SwSha256Hex("MShareware|loc|" + seed + "|" + kind).substr(0, 16);
		const MShareware::tstring wtag(tag.begin(), tag.end());

		m_valueName0 = (kind == 'T' ? TEXT("SW_Trial") : TEXT("SW_License"));
		m_subKey1 = TEXT("Software\\");
		m_subKey1 += wtag;

		TCHAR szPath[MAX_PATH];
		szPath[0] = 0;
		if (SUCCEEDED(::SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE,
									   nullptr, SHGFP_TYPE_CURRENT, szPath)) && szPath[0]) {
			m_filePath = szPath;
			m_filePath += TEXT("\\");
			m_filePath += wtag;
			m_filePath += TEXT(".dat");
		}
	}

	// Returns true if something readable is stored there (it may be garbage).
	bool Read(int loc, std::vector<BYTE>& blob) const {
		blob.clear();
		if (loc == 0 || loc == 1)
			return ReadRegistry(loc, blob);
		return ReadFile_(blob);
	}

	bool Write(int loc, const void *p, DWORD cb) const {
		if (loc == 0 || loc == 1)
			return WriteRegistry(loc, p, cb);
		return WriteFile_(p, cb);
	}

	// Deletes the stored copy. Returns true if it is gone (or never existed).
	bool Remove(int loc) const {
		if (loc == 0 || loc == 1) {
			HKEY hkey;
			LONG error = ::RegOpenKeyEx(HKEY_CURRENT_USER, SubKey(loc), 0, KEY_SET_VALUE, &hkey);
			if (error == ERROR_FILE_NOT_FOUND)
				return true;
			if (error != ERROR_SUCCESS)
				return false;
			error = ::RegDeleteValue(hkey, ValueName(loc));
			::RegCloseKey(hkey);
			if (loc == 1)
				::RegDeleteKey(HKEY_CURRENT_USER, SubKey(loc)); // best effort (only if empty)
			return error == ERROR_SUCCESS || error == ERROR_FILE_NOT_FOUND;
		}
		if (m_filePath.empty())
			return true;
		::SetFileAttributes(m_filePath.c_str(), FILE_ATTRIBUTE_NORMAL);
		if (::DeleteFile(m_filePath.c_str()))
			return true;
		DWORD dwError = ::GetLastError();
		return dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND;
	}

private:
	MShareware::tstring m_appKey;
	MShareware::tstring m_valueName0;
	MShareware::tstring m_subKey1;
	MShareware::tstring m_filePath;

	const TCHAR *SubKey(int loc) const {
		return (loc == 0 ? m_appKey.c_str() : m_subKey1.c_str());
	}
	const TCHAR *ValueName(int loc) const {
		return (loc == 0 ? m_valueName0.c_str() : TEXT("v"));
	}

	bool ReadRegistry(int loc, std::vector<BYTE>& blob) const {
		HKEY hkey;
		if (::RegOpenKeyEx(HKEY_CURRENT_USER, SubKey(loc), 0, KEY_QUERY_VALUE, &hkey) != ERROR_SUCCESS)
			return false;

		bool ok = false;
		DWORD type = 0, cb = 0;
		if (::RegQueryValueEx(hkey, ValueName(loc), nullptr, &type, nullptr, &cb) == ERROR_SUCCESS) {
			if (type != REG_BINARY || cb == 0 || cb > SW_MAX_BLOB) {
				blob.assign(1, 0); // present but invalid
				ok = true;
			} else {
				blob.resize(cb);
				if (::RegQueryValueEx(hkey, ValueName(loc), nullptr, &type, blob.data(), &cb) == ERROR_SUCCESS) {
					blob.resize(cb);
					ok = true;
				} else {
					blob.clear();
				}
			}
		}
		::RegCloseKey(hkey);
		return ok;
	}

	bool WriteRegistry(int loc, const void *p, DWORD cb) const {
		HKEY hkey;
		DWORD dwDisp;
		if (::RegCreateKeyEx(HKEY_CURRENT_USER, SubKey(loc), 0, nullptr, 0,
							 KEY_SET_VALUE, nullptr, &hkey, &dwDisp) != ERROR_SUCCESS)
			return false;
		LONG error = ::RegSetValueEx(hkey, ValueName(loc), 0, REG_BINARY,
									 static_cast<const BYTE *>(p), cb);
		::RegCloseKey(hkey);
		return error == ERROR_SUCCESS;
	}

	bool ReadFile_(std::vector<BYTE>& blob) const {
		if (m_filePath.empty())
			return false;
		HANDLE hFile = ::CreateFile(m_filePath.c_str(), GENERIC_READ,
									FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
									OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;

		bool ok = false;
		blob.resize(SW_MAX_BLOB + 1);
		DWORD cbRead = 0;
		if (::ReadFile(hFile, blob.data(), SW_MAX_BLOB + 1, &cbRead, nullptr)) {
			if (cbRead == 0 || cbRead > SW_MAX_BLOB) {
				blob.assign(1, 0); // present but invalid
			} else {
				blob.resize(cbRead);
			}
			ok = true;
		} else {
			blob.clear();
		}
		::CloseHandle(hFile);
		return ok;
	}

	bool WriteFile_(const void *p, DWORD cb) const {
		if (m_filePath.empty())
			return false;
		HANDLE hFile = ::CreateFile(m_filePath.c_str(), GENERIC_WRITE, 0, nullptr,
									CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			return false;
		DWORD cbWritten = 0;
		BOOL ok = ::WriteFile(hFile, p, cb, &cbWritten, nullptr);
		ok = (ok && cbWritten == cb);
		::CloseHandle(hFile);
		return !!ok;
	}
};

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////
// Load resource string

LPTSTR SwLoadStringDx1(HINSTANCE hInstance, UINT uID) {
	static TCHAR s_sz[2048];
	s_sz[0] = 0;
	::LoadString(hInstance, uID, s_sz, 2048);
	assert(::lstrlen(s_sz) < 2048);
	return s_sz;
}

LPTSTR SwLoadStringDx2(HINSTANCE hInstance, UINT uID) {
	static TCHAR s_sz[2048];
	s_sz[0] = 0;
	::LoadString(hInstance, uID, s_sz, 2048);
	assert(::lstrlen(s_sz) < 2048);
	return s_sz;
}

////////////////////////////////////////////////////////////////////////////
// Centering dialog

void SwCenterDialog(HWND hwnd) {
	assert(hwnd);
	assert(::IsWindow(hwnd));
	BOOL bChild = !!(::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD);

	HWND hwndOwner = (bChild ? ::GetParent(hwnd) : ::GetWindow(hwnd, GW_OWNER));

	RECT rc, rcOwner;
	if (hwndOwner)
		::GetWindowRect(hwndOwner, &rcOwner);
	else
		::SystemParametersInfo(SPI_GETWORKAREA, 0, &rcOwner, 0);

	::GetWindowRect(hwnd, &rc);

	POINT pt;
	pt.x = rcOwner.left + ((rcOwner.right - rcOwner.left) - (rc.right - rc.left)) / 2;
	pt.y = rcOwner.top + ((rcOwner.bottom - rcOwner.top) - (rc.bottom - rc.top)) / 2;

	if (bChild && hwndOwner != nullptr)
		::ScreenToClient(hwndOwner, &pt);

	::SetWindowPos(hwnd, nullptr, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

	::SendMessage(hwnd, DM_REPOSITION, 0, 0);
}

////////////////////////////////////////////////////////////////////////////
// Centering message box

static LRESULT CALLBACK
SwMsgBoxCbtProc_(int nCode, WPARAM wParam, LPARAM /*lParam*/) {
	if (nCode == HCBT_ACTIVATE) {
		HWND hwnd = (HWND)wParam;
		TCHAR szClassName[MAX_PATH];
		::GetClassName(hwnd, szClassName, MAX_PATH);
		if (lstrcmpi(szClassName, TEXT("#32770")) == 0) {
			SwCenterDialog(hwnd);
		}
	}
	return 0;
}

int SwCenterMessageBox(HWND hwndParent, LPCTSTR pszText, LPCTSTR pszCaption, UINT uMB_) {
	static HHOOK s_hMsgBoxHook = nullptr;

	if (s_hMsgBoxHook != nullptr && ::UnhookWindowsHookEx(s_hMsgBoxHook))
		s_hMsgBoxHook = nullptr;

	HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
		::GetWindowLongPtr(hwndParent, GWLP_HINSTANCE));

	DWORD dwThreadID = ::GetCurrentThreadId();
	s_hMsgBoxHook = ::SetWindowsHookEx(WH_CBT, SwMsgBoxCbtProc_, hInst, dwThreadID);

	int nID = ::MessageBox(hwndParent, pszText, pszCaption, uMB_);

	if (s_hMsgBoxHook != nullptr && ::UnhookWindowsHookEx(s_hMsgBoxHook))
		s_hMsgBoxHook = nullptr;

	return nID;
}

////////////////////////////////////////////////////////////////////////////
// MShareware class

DWORD MShareware::GetTrialDays() const {
	return m_dwTrialDays;
}

bool MShareware::IsRegistered() const {
	return m_status == MShareware::REGD;
}

bool MShareware::IsInTrial() const {
	return m_status == MShareware::IN_TRIAL ||
		   m_status == MShareware::IN_TRIAL_FIRST_TIME;
}

bool MShareware::IsOutOfTrial() const {
	return m_status == MShareware::OUT_OF_TRIAL;
}

void MShareware::OnTrialFirstTime(HWND hwndParent) {
}

void MShareware::OnTrial(HWND hwndParent) {
	UrgeRegister(hwndParent);
}

bool MShareware::OnOutOfTrial(HWND hwndParent) {
	return UrgeRegister(hwndParent);
}

void MShareware::ThisCommandRequiresRegistering(HWND hwndParent) {
	ShowErrorMessage(hwndParent, 32737);
}

void MShareware::ShowErrorMessage(HWND hwndParent, UINT uStringID) {
	SwCenterMessageBox(hwndParent, SwLoadStringDx2(m_hInstance, uStringID), nullptr, MB_ICONERROR);
}

MShareware::MShareware(
	LPCTSTR pszAppKey,
	const char *pszSha256HashHexString,
	DWORD dwTrialDays/* = 15*/,
	const char *salt/* = ""*/,
	const char *new_version/* = ""*/)
	: m_hInstance(::GetModuleHandle(nullptr))
	, m_dwTrialDays(dwTrialDays)
	, m_status(MShareware::IN_TRIAL)
	, m_strAppKey(pszAppKey ? pszAppKey : TEXT(""))
	, m_strSha256HashHexString(SwToLowerAscii(pszSha256HashHexString ? pszSha256HashHexString : ""))
	, m_strSalt(salt ? salt : "")
	, m_strNewVersion(new_version ? new_version : "")
	, m_strOldVersion()
{
}

MShareware::MShareware(
	LPCTSTR pszAppKey,
	const BYTE *pbHash32Bytes,
	DWORD dwTrialDays/* = 15*/,
	const char *salt/* = ""*/,
	const char *new_version/* = ""*/)
	: m_hInstance(::GetModuleHandle(nullptr))
	, m_dwTrialDays(dwTrialDays)
	, m_status(MShareware::IN_TRIAL)
	, m_strAppKey(pszAppKey ? pszAppKey : TEXT(""))
	, m_strSha256HashHexString()
	, m_strSalt(salt ? salt : "")
	, m_strNewVersion(new_version ? new_version : "")
	, m_strOldVersion()
{
	if (pbHash32Bytes)
		mzc_hex_string_from_bytes(m_strSha256HashHexString, &pbHash32Bytes[0], &pbHash32Bytes[32]);
}

MShareware::~MShareware() {
}

bool MShareware::Start(HWND hwndParent/* = nullptr*/) {
	CheckRegistry(hwndParent);

	switch (m_status) {
	case MShareware::IN_TRIAL_FIRST_TIME:
		OnTrialFirstTime(hwndParent);
		return true;

	case MShareware::IN_TRIAL:
		OnTrial(hwndParent);
		return true;

	case MShareware::REGD:
		return true;

	case MShareware::OUT_OF_TRIAL:
		return OnOutOfTrial(hwndParent);

	default:
		return false;
	}
}

void SwUrgeRegisterDlg_OnInit(HWND hwnd, MShareware *pShareware) {
	LPTSTR pszMsg;
	TCHAR szMsg[MAX_PATH * 3];
	pShareware->CheckDate();

	switch (pShareware->m_status) {
	case MShareware::IN_TRIAL_FIRST_TIME:
		pszMsg = SwLoadStringDx1(pShareware->m_hInstance, 32732);
		::StringCchPrintf(szMsg, _countof(szMsg), pszMsg, pShareware->m_dwTrialDays, 0);
		::SetDlgItemText(hwnd, edt1, szMsg);
		break;

	case MShareware::IN_TRIAL:
		if (pShareware->m_dwlTotalMinutesRemains >= 60 * 24) {
			pszMsg = SwLoadStringDx1(pShareware->m_hInstance, 32732);
			::StringCchPrintf(szMsg, _countof(szMsg), pszMsg,
							  static_cast<DWORD>(pShareware->m_dwlTotalMinutesRemains / (60 * 24)),
							  static_cast<DWORD>((pShareware->m_dwlTotalMinutesRemains / 60) % 24));
			::SetDlgItemText(hwnd, edt1, szMsg);
		} else {
			pszMsg = SwLoadStringDx1(pShareware->m_hInstance, 32733);
			::StringCchPrintf(szMsg, _countof(szMsg), pszMsg,
							  static_cast<DWORD>(pShareware->m_dwlTotalMinutesRemains / 60),
							  static_cast<DWORD>(pShareware->m_dwlTotalMinutesRemains % 60));
			::SetDlgItemText(hwnd, edt1, szMsg);
		}
		break;

	case MShareware::OUT_OF_TRIAL:
		// 32738: the system clock was set back
		pszMsg = SwLoadStringDx1(pShareware->m_hInstance,
								 pShareware->m_bClockRollback ? 32738 : 32734);
		::SetDlgItemText(hwnd, edt1, pszMsg);
		break;

	case MShareware::REGD:
		::EnableWindow(::GetDlgItem(hwnd, edt2), FALSE);
		::EnableWindow(::GetDlgItem(hwnd, IDOK), FALSE);
		pszMsg = SwLoadStringDx1(pShareware->m_hInstance, 32735);
		::SetDlgItemText(hwnd, edt1, pszMsg);
		::SetFocus(::GetDlgItem(hwnd, IDCANCEL));
		break;
	}
}

static INT_PTR CALLBACK
SwUrgeRegisterDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	static MShareware *s_pShareware = nullptr;
	char szPassword[sw_shareware_max_password];

	switch (uMsg) {
	case WM_INITDIALOG:
		SwCenterDialog(hwnd);
		s_pShareware = reinterpret_cast<MShareware *>(lParam);
		SwUrgeRegisterDlg_OnInit(hwnd, s_pShareware);
		return TRUE;

	case WM_COMMAND:
		if (!s_pShareware)
			break;
		switch (LOWORD(wParam)) {
		case IDOK:
			szPassword[0] = 0;
			::GetDlgItemTextA(hwnd, edt2, szPassword, sw_shareware_max_password);
			if (s_pShareware->RegisterPassword(hwnd, szPassword)) {
				SwUrgeRegisterDlg_OnInit(hwnd, s_pShareware);
				TCHAR szTitle[256];
				::GetWindowText(hwnd, szTitle, 256);
				SwCenterMessageBox(hwnd, SwLoadStringDx1(s_pShareware->m_hInstance, 32735),
								   szTitle, MB_ICONINFORMATION);
			} else if (!s_pShareware->IsPasswordValid(szPassword)) {
				// (If the password is valid, RegisterPassword already reported the storage error.)
				s_pShareware->ShowErrorMessage(hwnd, 32731);
			}
			SecureZeroMemory(szPassword, sizeof(szPassword));
			break;

		case IDCANCEL:
			::EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR *pnmhdr = reinterpret_cast<NMHDR *>(lParam);
			switch (pnmhdr->code) {
			case NM_CLICK:
			case NM_RETURN:
				if (pnmhdr->idFrom == stc1) {
					// Extract URL from stc1 text
					char szText[MAX_PATH];
					szText[0] = 0;
					::GetDlgItemTextA(hwnd, stc1, szText, _countof(szText));
					char *ptr0 = strstr(szText, "<a>");
					if (ptr0) {
						ptr0 += strlen("<a>");
						char *ptr1 = strstr(ptr0, "</a>");
						if (ptr1) {
							*ptr1 = 0;
							// Open the URL (only http / https for safety)
							if (_strnicmp(ptr0, "http://", 7) == 0 ||
								_strnicmp(ptr0, "https://", 8) == 0)
							{
								::ShellExecuteA(hwnd, nullptr, ptr0, nullptr, nullptr, SW_SHOWNORMAL);
							}
						}
					}
				}
				break;
			}
		}
	}
	return 0;
}

bool MShareware::UrgeRegister(HWND hwndParent/* = nullptr*/) {
	::DialogBoxParam(m_hInstance, MAKEINTRESOURCE(32731), hwndParent, SwUrgeRegisterDlgProc,
					 reinterpret_cast<LPARAM>(this));
	return IsRegistered();
}

////////////////////////////////////////////////////////////////////////////
// Keys

// Override this to add a build-specific secret (e.g. a random constant of
// your own) to the HMAC key. It makes records forged from the public
// information (salt / hash) invalid.
void MShareware::GetMacSecret(std::string& secret) const {
	secret = "MShareware/1|";
	secret += m_strSalt;
	secret += '|';
	secret += m_strSha256HashHexString;
}

std::string MShareware::MakeMacKey(char kind) const {
	std::string key;
	GetMacSecret(key);
	key += '|';
	key += SwGetUserSidString(); // bind records to the current user
	key += '|';
	key += SwToUtf8(m_strAppKey);
	key += '|';
	key += kind;
	return key;
}

static std::string SwMakeSeed(const MShareware::tstring& appKey, const std::string& salt) {
	return SwToUtf8(appKey) + "|" + salt;
}

////////////////////////////////////////////////////////////////////////////
// Registry / start-up check

bool MShareware::CheckRegistry(HWND hwndParent) {
	m_bClockRollback = false;

	// 1) licensed?
	if (LoadLicense() || MigrateLegacyLicense()) {
		m_status = MShareware::REGD;
		return true;
	}

	// 2) trial
	return CheckTrial(hwndParent);
}

// Delete values written by older versions of this class.
// Returns true if the values are gone (or never existed).
bool MShareware::DeleteLegacyValues(bool bTrial, bool bLicense) {
	HKEY hkeyApp;
	LONG error = ::RegOpenKeyEx(HKEY_CURRENT_USER, m_strAppKey.c_str(), 0, KEY_SET_VALUE, &hkeyApp);
	if (error == ERROR_FILE_NOT_FOUND)
		return true;
	if (error != ERROR_SUCCESS)
		return false;

	bool ok = true;
	const TCHAR *names[5];
	int n = 0;
	if (bTrial) {
		names[n++] = TEXT("SW_StartUse");
		names[n++] = TEXT("SW_LastUse");
		names[n++] = TEXT("SW_OldVersion");
	}
	if (bLicense) {
		names[n++] = TEXT("SW_EncodedPassword");
		names[n++] = TEXT("SW_CheckSum");
	}
	for (int i = 0; i < n; ++i) {
		error = ::RegDeleteValue(hkeyApp, names[i]);
		if (error != ERROR_SUCCESS && error != ERROR_FILE_NOT_FOUND)
			ok = false;
	}
	::RegCloseKey(hkeyApp);
	return ok;
}

////////////////////////////////////////////////////////////////////////////
// License

bool MShareware::LoadLicense() {
	SwBlobStore store(m_strAppKey, SwMakeSeed(m_strAppKey, m_strSalt), 'L');
	const std::string key = MakeMacKey('L');

	for (int loc = 0; loc < SwBlobStore::COUNT; ++loc) {
		std::vector<BYTE> blob;
		if (!store.Read(loc, blob))
			continue;
		SW_LICENSE_RECORD rec;
		if (SwLoadRecord(blob, key, SW_LICENSE_MAGIC, rec))
			return true;
	}
	return false;
}

bool MShareware::SaveLicense() {
	SW_LICENSE_RECORD rec;
	memset(&rec, 0, sizeof(rec));
	rec.dwMagic = SW_LICENSE_MAGIC;
	::GetSystemTimeAsFileTime(&rec.ftRegistered);
	SwSealRecord(rec, MakeMacKey('L'));

	SwBlobStore store(m_strAppKey, SwMakeSeed(m_strAppKey, m_strSalt), 'L');
	int nSaved = 0;
	for (int loc = 0; loc < SwBlobStore::COUNT; ++loc) {
		if (store.Write(loc, &rec, static_cast<DWORD>(sizeof(rec))))
			++nSaved;
	}
	return nSaved > 0;
}

// Accept a license written by older versions (XOR-encoded password + user
// check sum) and convert it to the new format.
bool MShareware::MigrateLegacyLicense() {
	HKEY hkeyApp;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, m_strAppKey.c_str(), 0, KEY_QUERY_VALUE, &hkeyApp) != ERROR_SUCCESS)
		return false;

	bool bValid = false;
	char szPassword[sw_shareware_max_password];
	DWORD type = 0, cb = sw_shareware_max_password - 1;
	if (::RegQueryValueExA(hkeyApp, "SW_EncodedPassword", nullptr, &type,
						   reinterpret_cast<BYTE *>(szPassword), &cb) == ERROR_SUCCESS &&
		type == REG_BINARY && cb < sw_shareware_max_password)
	{
		szPassword[cb] = 0;
		DecodePassword(szPassword, cb);
		if (IsPasswordValid(szPassword)) {
			DWORD dwValue = 0;
			cb = static_cast<DWORD>(sizeof(dwValue));
			if (::RegQueryValueEx(hkeyApp, TEXT("SW_CheckSum"), nullptr, &type,
								  reinterpret_cast<BYTE *>(&dwValue), &cb) == ERROR_SUCCESS &&
				type == REG_DWORD && dwValue == GetUserCheckSum())
			{
				bValid = true;
			}
		}
	}
	SecureZeroMemory(szPassword, sizeof(szPassword));
	::RegCloseKey(hkeyApp);

	if (bValid && SaveLicense())
		DeleteLegacyValues(false, true); // the decodable password is no longer kept
	return bValid;
}

bool MShareware::RegisterPassword(HWND hwndParent, const char *pszPassword) {
	// check password
	if (!IsPasswordValid(pszPassword)) {
		// slow down brute-force attempts via the dialog
		++m_dwFailedAttempts;
		DWORD dwWait = 750 * m_dwFailedAttempts;
		::Sleep(dwWait > 5000 ? 5000 : dwWait);
		return false;
	}
	m_dwFailedAttempts = 0;

	// store the license (the password itself is NOT stored)
	if (!SaveLicense()) {
		ShowErrorMessage(hwndParent, 32736);
		return false;
	}
	DeleteLegacyValues(false, true);

	m_status = MShareware::REGD;
	return true;
}

// Removes every copy of the license (and any old-format values).
bool MShareware::RemoveLicense() {
	SwBlobStore store(m_strAppKey, SwMakeSeed(m_strAppKey, m_strSalt), 'L');
	bool ok = true;
	for (int loc = 0; loc < SwBlobStore::COUNT; ++loc) {
		if (!store.Remove(loc))
			ok = false;
	}
	if (!DeleteLegacyValues(false, true))
		ok = false;
	return ok && !LoadLicense();
}

bool MShareware::Unregister(HWND hwndParent/* = nullptr*/, BOOL bForce/* = FALSE*/) {
	if (!RemoveLicense()) {
		// Some copy could not be removed: it would come back at the next start.
		ShowErrorMessage(hwndParent, 32736);
		return false;
	}

	// Decide the status from the EXISTING trial data only (no new trial).
	m_bClockRollback = false;
	m_dwlTotalMinutesRemains = 0;
	m_status = MShareware::OUT_OF_TRIAL;
	if (LoadTrialState() == LOAD_OK && CheckDateInternal() == DATE_OK)
		m_status = MShareware::IN_TRIAL;
	if (bForce)
		CheckRegistry(hwndParent);
	return true;
}

////////////////////////////////////////////////////////////////////////////
// Trial

int MShareware::LoadTrialState() {
	SwBlobStore store(m_strAppKey, SwMakeSeed(m_strAppKey, m_strSalt), 'T');
	const std::string key = MakeMacKey('T');

	int nPresent = 0, nValid = 0;
	SW_TRIAL_RECORD best;
	memset(&best, 0, sizeof(best));

	for (int loc = 0; loc < SwBlobStore::COUNT; ++loc) {
		std::vector<BYTE> blob;
		if (!store.Read(loc, blob))
			continue;
		++nPresent;

		SW_TRIAL_RECORD rec;
		if (!SwLoadRecord(blob, key, SW_TRIAL_MAGIC, rec))
			continue; // tampered or broken copy: ignore it
		rec.szVersion[SW_MAX_VERSION] = 0;
		if (SwFtToQ(rec.ftLastUse) < SwFtToQ(rec.ftStart))
			continue;

		if (++nValid == 1) {
			best = rec;
			continue;
		}
		// earliest start, latest use
		if (SwFtToQ(rec.ftStart) < SwFtToQ(best.ftStart))
			best.ftStart = rec.ftStart;
		if (SwFtToQ(rec.ftLastUse) > SwFtToQ(best.ftLastUse)) {
			best.ftLastUse = rec.ftLastUse;
			memcpy(best.szVersion, rec.szVersion, sizeof(best.szVersion));
		}
	}

	if (nPresent == 0)
		return LOAD_NONE;
	if (nValid == 0)
		return LOAD_TAMPERED; // data exists but none of it is trustworthy

	m_ftStart = best.ftStart;
	m_ftLastUse = best.ftLastUse;
	m_strOldVersion = best.szVersion;
	return LOAD_OK;
}

// Returns true if at least one location was written.
bool MShareware::SaveTrialState() {
	SW_TRIAL_RECORD rec;
	memset(&rec, 0, sizeof(rec));
	rec.dwMagic = SW_TRIAL_MAGIC;
	rec.ftStart = m_ftStart;
	rec.ftLastUse = m_ftLastUse;
	size_t cchVer = m_strOldVersion.size();
	if (cchVer > SW_MAX_VERSION)
		cchVer = SW_MAX_VERSION;
	memcpy(rec.szVersion, m_strOldVersion.data(), cchVer);
	SwSealRecord(rec, MakeMacKey('T'));

	SwBlobStore store(m_strAppKey, SwMakeSeed(m_strAppKey, m_strSalt), 'T');
	int nSaved = 0;
	for (int loc = 0; loc < SwBlobStore::COUNT; ++loc) {
		if (store.Write(loc, &rec, static_cast<DWORD>(sizeof(rec))))
			++nSaved;
	}
	return nSaved > 0;
}

// Import the (unprotected) trial values written by older versions.
bool MShareware::ImportLegacyTrial(const FILETIME& ftNow) {
	HKEY hkeyApp;
	if (::RegOpenKeyEx(HKEY_CURRENT_USER, m_strAppKey.c_str(), 0, KEY_QUERY_VALUE, &hkeyApp) != ERROR_SUCCESS)
		return false;

	bool bFound = false;
	const DWORDLONG qNow = SwFtToQ(ftNow);

	FILETIME ft;
	DWORD type = 0, cb = static_cast<DWORD>(sizeof(ft));
	if (::RegQueryValueEx(hkeyApp, TEXT("SW_StartUse"), nullptr, &type,
						  reinterpret_cast<BYTE *>(&ft), &cb) == ERROR_SUCCESS &&
		type == REG_BINARY && cb == sizeof(ft))
	{
		DWORDLONG qStart = SwFtToQ(ft);
		if (qStart > qNow)
			qStart = qNow; // a start time in the future is not credible
		DWORDLONG qLast = qStart;

		cb = static_cast<DWORD>(sizeof(ft));
		if (::RegQueryValueEx(hkeyApp, TEXT("SW_LastUse"), nullptr, &type,
							  reinterpret_cast<BYTE *>(&ft), &cb) == ERROR_SUCCESS &&
			type == REG_BINARY && cb == sizeof(ft))
		{
			DWORDLONG q = SwFtToQ(ft);
			if (q > qNow)
				q = qNow;
			if (q > qLast)
				qLast = q;
		}

		char szVersion[SW_MAX_VERSION + 1];
		cb = static_cast<DWORD>(SW_MAX_VERSION);
		if (::RegQueryValueExA(hkeyApp, "SW_OldVersion", nullptr, &type,
							   reinterpret_cast<BYTE *>(szVersion), &cb) == ERROR_SUCCESS &&
			type == REG_SZ && cb <= SW_MAX_VERSION)
		{
			szVersion[cb] = 0;
			m_strOldVersion = szVersion;
		} else {
			m_strOldVersion = m_strNewVersion.substr(0, SW_MAX_VERSION);
		}

		m_ftStart = SwQToFt(qStart);
		m_ftLastUse = SwQToFt(qLast);
		bFound = true;
	}

	::RegCloseKey(hkeyApp);
	return bFound;
}

bool MShareware::CheckTrial(HWND hwndParent) {
	const std::string strNewVersion = m_strNewVersion.substr(0, SW_MAX_VERSION);
	FILETIME ftNow;
	::GetSystemTimeAsFileTime(&ftNow);

	m_dwlTotalMinutesRemains = 0;

	const int nLoad = LoadTrialState();
	if (nLoad == LOAD_TAMPERED) {
		// The trial data was modified or damaged. Fail closed.
		m_status = MShareware::OUT_OF_TRIAL;
		return false;
	}

	bool bFirstTime = false;
	if (nLoad == LOAD_NONE) {
		if (ImportLegacyTrial(ftNow)) {
			if (SaveTrialState())
				DeleteLegacyValues(true, false);
		} else {
			// first run
			m_ftStart = m_ftLastUse = ftNow;
			m_strOldVersion = strNewVersion;
			bFirstTime = true;
		}
		if (bFirstTime && !SaveTrialState()) {
			// Cannot remember the start time: do NOT grant a free trial.
			ShowErrorMessage(hwndParent, 32736);
			m_status = MShareware::OUT_OF_TRIAL;
			return false;
		}
	}

	// A new version restarts the trial.
	if (!bFirstTime && CompareVersion(m_strOldVersion.c_str(), strNewVersion.c_str()) < 0) {
		const FILETIME ftStartOld = m_ftStart, ftLastOld = m_ftLastUse;
		const std::string strVersionOld = m_strOldVersion;

		m_ftStart = m_ftLastUse = ftNow;
		m_strOldVersion = strNewVersion;
		if (SaveTrialState()) {
			CheckDateInternal(); // refresh the remaining time
			m_status = MShareware::IN_TRIAL;
			return true;
		}
		// not persisted: keep the old trial
		m_ftStart = ftStartOld;
		m_ftLastUse = ftLastOld;
		m_strOldVersion = strVersionOld;
		ShowErrorMessage(hwndParent, 32736);
	}

	switch (CheckDateInternal()) {
	case DATE_OK:
		m_status = (bFirstTime ? MShareware::IN_TRIAL_FIRST_TIME : MShareware::IN_TRIAL);
		return true;

	case DATE_ROLLBACK:
		ShowErrorMessage(hwndParent, 32738);
		m_status = MShareware::OUT_OF_TRIAL;
		return false;

	default:
		m_status = MShareware::OUT_OF_TRIAL;
		return false;
	}
}

bool MShareware::IsPasswordValid(const char *pszPassword) const {
	if (pszPassword == nullptr || m_strSha256HashHexString.empty())
		return false;
	if (strlen(pszPassword) >= sw_shareware_max_password)
		return false;

	std::string str;
	MSha256::hex_from_passwd_and_salt(str, pszPassword, m_strSalt.data());
	return SwSecureEquals(str, m_strSha256HashHexString);
}

// Public: refresh the remaining time. Returns false if the trial is over.
bool MShareware::CheckDate() {
	if (m_status == MShareware::REGD)
		return true;
	if (m_status == MShareware::OUT_OF_TRIAL) {
		m_dwlTotalMinutesRemains = 0;
		return false;
	}

	if (CheckDateInternal() == DATE_OK)
		return true;
	m_status = MShareware::OUT_OF_TRIAL;
	return false;
}

// Time never goes backwards for the trial:
//  * latest observed time (m_ftLastUse) is stored in the protected record;
//  * if the clock is earlier than that by more than the tolerance, the trial
//    is suspended (DATE_ROLLBACK) until the clock is corrected;
//  * a small difference (NTP correction) is tolerated but earns no extra time.
int MShareware::CheckDateInternal() {
	FILETIME ftNow;
	::GetSystemTimeAsFileTime(&ftNow);

	const DWORDLONG qNow = SwFtToQ(ftNow);
	const DWORDLONG qStart = SwFtToQ(m_ftStart);
	DWORDLONG qLast = SwFtToQ(m_ftLastUse);
	if (qLast < qStart)
		qLast = qStart;

	m_dwlTotalMinutesRemains = 0;

	if (qNow + SW_CLOCK_TOLERANCE < qLast) {
		m_bClockRollback = true;
		return DATE_ROLLBACK;
	}
	m_bClockRollback = false;

	DWORDLONG qEffective = qLast; // never earlier than the last observed time
	if (qNow > qLast) {
		qEffective = qNow;
		if (qNow - qLast >= SW_100NS_PER_MIN) { // limit registry / file writes
			m_ftLastUse = ftNow;
			SaveTrialState();
		}
	}

	DWORD dwDays = GetTrialDays();
	if (dwDays > SW_MAX_TRIAL_DAYS)
		dwDays = SW_MAX_TRIAL_DAYS;
	const DWORDLONG qTrial = static_cast<DWORDLONG>(dwDays) * 24 * 60 * 60 * SW_100NS_PER_SEC;
	const DWORDLONG qElapsed = qEffective - qStart; // qEffective >= qLast >= qStart

	if (qElapsed < qTrial) {
		m_dwlTotalMinutesRemains = (qTrial - qElapsed) / SW_100NS_PER_MIN;
		return DATE_OK;
	}
	return DATE_EXPIRED;
}

// NOTE: Kept as is for compatibility with licenses issued by older versions.
DWORD MShareware::GetUserCheckSum() const {
	DWORD dwCheckSum = 0xDeadFace;
	TCHAR szUser[64];
	DWORD dwSize = 64;
	if (::GetUserName(szUser, &dwSize)) {
		LPTSTR pch = szUser;
		while (*pch) {
			dwCheckSum += *pch;
			++pch;
		}
	}
	return dwCheckSum;
}

// NOTE: Only used to read licenses written by older versions.
void MShareware::EncodePassword(void *pass, DWORD size) const {
	BYTE *pb = reinterpret_cast<BYTE *>(pass);
	while (size--) {
		*pb ^= 0xFF;
		pb++;
	}
}

void MShareware::DecodePassword(void *pass, DWORD size) const {
	BYTE *pb = reinterpret_cast<BYTE *>(pass);
	while (size--) {
		*pb ^= 0xFF;
		pb++;
	}
}

static void sw_explode(std::vector<std::string>& v, const std::string& separators, const std::string& s) {
	size_t i = s.find_first_not_of(separators);
	size_t n = s.size();

	v.clear();
	while (i < n) {
		size_t stop = s.find_first_of(separators, i);
		if (stop > n) stop = n;
		v.push_back(s.substr(i, stop - i));
		i = s.find_first_not_of(separators, stop + 1);
	}
}

int MShareware::CompareVersion(const char *old_ver, const char *new_ver) {
	using namespace std;
	if (old_ver[0] == 0 && new_ver[0] == 0)
		return 0;

	std::vector<string> o, n;
	sw_explode(o, " .", old_ver);
	sw_explode(n, " .", new_ver);

	size_t osiz = o.size(), nsiz = n.size();
	size_t m = (osiz > nsiz ? nsiz : osiz);
	for (size_t i = 0; i < m; ++i) {
		if (o[i].size() == 0 && n[i].size() == 0)
	        continue;
		if (o[i].size() == 0)
	        return -1;
		if (n[i].size() == 0)
	        return -1;

		if (isdigit(o[i][0]) && isdigit(n[i][0])) {
			char *op, *np;
			int on = strtol(o[i].data(), &op, 10);
			int nn = strtol(n[i].data(), &np, 10);
			if (on < nn)
		        return -1;
			if (on > nn)
		        return 1;

			int cmp = strcmp(op, np);
			if (cmp < 0)
		        return -1;
			if (cmp > 0)
		        return 1;
		} else {
			int cmp = strcmp(o[i].data(), n[i].data());
			if (cmp < 0)
		        return -1;
			if (cmp > 0)
		        return 1;
		}
	}
	if (osiz < nsiz)
        return -1;
	if (osiz > nsiz)
        return 1;
	return 0;
}


////////////////////////////////////////////////////////////////////////////

#ifdef SHAREWARE_UNITTEST
	extern "C"
	int main(void) {
		MShareware s(
			TEXT("Katayama Hirofumi MZ"),
			"9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08");
		assert(s.IsPasswordValid("test"));

		assert(s.CompareVersion("", "") == 0);
		assert(s.CompareVersion("1", "1") == 0);
		assert(s.CompareVersion("2.1", "2.1") == 0);

		assert(s.CompareVersion("1", "2") < 0);
		assert(s.CompareVersion("1.1", "2.1") < 0);
		assert(s.CompareVersion("1.1", "1.2") < 0);
		assert(s.CompareVersion("1.1", "1.1.1") < 0);
		assert(s.CompareVersion("1.1", "1.2.1") < 0);
		assert(s.CompareVersion("1.1", "1.1a") < 0);
		assert(s.CompareVersion("1.1a", "1.1b") < 0);
		assert(s.CompareVersion("1.1", "1.1 b") < 0);
		assert(s.CompareVersion("1.1 a", "1.1 b") < 0);

		assert(s.CompareVersion("2", "1") > 0);
		assert(s.CompareVersion("2.1", "1.1") > 0);
		assert(s.CompareVersion("1.2", "1.1") > 0);
		assert(s.CompareVersion("1.1.1", "1.1") > 0);
		assert(s.CompareVersion("1.2.1", "1.1") > 0);
		assert(s.CompareVersion("1.1a", "1.1") > 0);
		assert(s.CompareVersion("1.1b", "1.1a") > 0);
		assert(s.CompareVersion("1.1 b", "1.1") > 0);
		assert(s.CompareVersion("1.1 b", "1.1 a") > 0);
		return 0;
	}
#endif  // def SHAREWARE_UNITTEST

////////////////////////////////////////////////////////////////////////////
