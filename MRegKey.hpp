// MRegKey.hpp -- Win32API registry key manipulator             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREGKEY_HPP_
#define MZC4_MREGKEY_HPP_       101 /* Version 101 */

#ifndef HKCR
    #define HKCR    HKEY_CLASSES_ROOT
    #define HKCU    HKEY_CURRENT_USER
    #define HKLM    HKEY_LOCAL_MACHINE
    #define HKU     HKEY_USERS
    #define HKPD    HKEY_PERFORMANCE_DATA
    #define HKCC    HKEY_CURRENT_CONFIG
    #define HKDD    HKEY_DYN_DATA
#endif

class MRegKey;

////////////////////////////////////////////////////////////////////////////

#ifndef _INC_WINDOWS
    #include <windows.h>    // Win32API
#endif
#include <cassert>          // assert
#include <malloc.h>         // _malloca/_freea

////////////////////////////////////////////////////////////////////////////

// NOTE: RegDeleteTreeDx deletes all value entries if pszSubKey == nullptr.
// NOTE: RegDeleteTreeDx cannot delete opening keys.
LONG RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey/* = nullptr*/) noexcept;

////////////////////////////////////////////////////////////////////////////

class MRegKey
{
public:
    MRegKey() noexcept;
    MRegKey(HKEY hKey) noexcept;
    explicit MRegKey(MRegKey& key) noexcept;
    MRegKey(HKEY hBaseKey, LPCTSTR pszSubKey, BOOL bCreate = FALSE) noexcept;
    virtual ~MRegKey() noexcept;

    operator HKEY() const noexcept;
    bool operator!() const noexcept;
    bool operator==(HKEY hKey) const noexcept;
    bool operator!=(HKEY hKey) const noexcept;
    MRegKey& operator=(HKEY hKey) noexcept;
    MRegKey& operator=(MRegKey& key) noexcept;

    BOOL Attach(HKEY hKey) noexcept;
    HKEY Detach() noexcept;
    HKEY Handle() const noexcept;

    LONG RegCreateKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey, DWORD dwReserved = 0,
                        LPTSTR lpClass = nullptr, DWORD dwOptions = 0,
                        REGSAM samDesired = KEY_ALL_ACCESS,
                        LPSECURITY_ATTRIBUTES lpsa = nullptr,
                        LPDWORD lpdwDisposition = nullptr) noexcept;
    LONG RegOpenKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey, DWORD dwOptions = 0,
                      REGSAM samDesired = KEY_READ) noexcept;

    LONG RegConnectRegistry(LPCTSTR lpMachineName, HKEY hBaseKey) noexcept;

    LONG RegCloseKey() noexcept;

    LONG RegQueryValueEx(LPCTSTR pszValueName = nullptr,
                         LPDWORD lpReserved = nullptr, LPDWORD lpType = nullptr,
                         LPBYTE lpData = nullptr, LPDWORD lpcbData = nullptr) noexcept;

    LONG QueryBinary(LPCTSTR pszValueName, LPVOID pvValue, DWORD cb) noexcept;
    LONG QueryDword(LPCTSTR pszValueName, DWORD& dw) noexcept;
    LONG QueryDwordLE(LPCTSTR pszValueName, DWORD& dw) noexcept;
    LONG QueryDwordBE(LPCTSTR pszValueName, DWORD& dw) noexcept;
    LONG QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue) noexcept;
    LONG QueryExpandSz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue) noexcept;
    LONG QueryMultiSz(LPCTSTR pszValueName, LPTSTR pszzValues, DWORD cchValues) noexcept;
    template <typename T_CONTAINER>
    LONG QueryMultiSz(LPCTSTR pszValueName, T_CONTAINER& container) noexcept;
    template <typename T_STRUCT>
    LONG QueryStruct(LPCTSTR pszValueName, T_STRUCT& data) noexcept;

    template <typename T_STRING>
    LONG QuerySz(LPCTSTR pszValueName, T_STRING& strValue) noexcept;
    template <typename T_STRING>
    LONG QueryExpandSz(LPCTSTR pszValueName, T_STRING& strValue) noexcept;

    LONG RegSetValueEx(LPCTSTR pszValueName, DWORD dwReserved,
        DWORD dwType, CONST BYTE *lpData, DWORD cbData) noexcept;

    LONG SetBinary(LPCTSTR pszValueName, LPCVOID pvValue, DWORD cb) noexcept;
    LONG SetDword(LPCTSTR pszValueName, DWORD dw) noexcept;
    LONG SetDwordLE(LPCTSTR pszValueName, DWORD dw) noexcept;
    LONG SetDwordBE(LPCTSTR pszValueName, DWORD dw) noexcept;
    LONG SetSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue) noexcept;
    LONG SetSz(LPCTSTR pszValueName, LPCTSTR pszValue) noexcept;
    LONG SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue) noexcept;
    LONG SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue) noexcept;
    LONG SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues) noexcept;
    LONG SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues, DWORD cchValues) noexcept;
    template <typename T_CONTAINER>
    LONG SetMultiSz(LPCTSTR pszValueName, const T_CONTAINER& container) noexcept;
    template <typename T_STRUCT>
    LONG SetStruct(LPCTSTR pszValueName, const T_STRUCT& data) noexcept;

    LONG RegDeleteValue(LPCTSTR pszValueName) noexcept;
    LONG RegDeleteTreeDx(LPCTSTR pszSubKey) noexcept;
    LONG RegEnumKeyEx(DWORD dwIndex, LPTSTR lpName, LPDWORD lpcchName,
                      LPDWORD lpReserved = nullptr, LPTSTR lpClass = nullptr,
                      LPDWORD lpcchClass = nullptr,
                      PFILETIME lpftLastWriteTime = nullptr) noexcept;
    LONG RegEnumValue(DWORD dwIndex, LPTSTR lpName, LPDWORD lpcchName,
                      LPDWORD lpReserved = nullptr, LPDWORD lpType = nullptr,
                      LPBYTE lpData = nullptr, LPDWORD lpcbData = nullptr) noexcept;

    LONG RegFlushKey() noexcept;
    LONG RegGetKeySecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD, LPDWORD pcbSD) noexcept;

    LONG RegNotifyChangeKeyValue(BOOL bWatchSubTree = TRUE,
        DWORD dwFilter = REG_LEGAL_CHANGE_FILTER,
        HANDLE hEvent = nullptr, BOOL bAsyncronous = FALSE) noexcept;

    LONG RegQueryInfoKey(LPTSTR lpClass = nullptr,
        LPDWORD lpcchClass = nullptr,
        LPDWORD lpReserved = nullptr,
        LPDWORD lpcSubKeys = nullptr,
        LPDWORD lpcchMaxSubKeyLen = nullptr,
        LPDWORD lpcchMaxClassLen = nullptr,
        LPDWORD lpcValues = nullptr,
        LPDWORD lpcchMaxValueNameLen = nullptr,
        LPDWORD lpcbMaxValueLen = nullptr,
        LPDWORD lpcbSecurityDescriptor = nullptr,
        PFILETIME lpftLastWriteTime = nullptr) noexcept;

    LONG RegQueryMultipleValues(PVALENT val_list, DWORD num_vals,
                                LPTSTR lpValueBuf, LPDWORD lpdwTotsize) noexcept;
    LONG RegSetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) noexcept;
    
    static LONG RegLoadKey(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszFile) noexcept;
    static LONG RegUnLoadKey(HKEY hKey, LPCTSTR pszSubKey) noexcept;
    static LONG RegSaveKey(HKEY hKey, LPCTSTR pszFile,
                           LPSECURITY_ATTRIBUTES lpsa = nullptr) noexcept;
    static LONG RegRestoreKey(HKEY hKey, LPCTSTR pszFile, DWORD dwFlags) noexcept;
    static LONG RegReplaceKey(HKEY hKey, LPCTSTR pszSubKey,
                              LPCTSTR pszNewFile, LPCTSTR pszOldFile) noexcept;
    static LONG RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey) noexcept;
    static size_t MultiSzSizeDx(LPCTSTR pszz) noexcept;

    static HKEY CloneHandleDx(HKEY hKey) noexcept;

protected:
    HKEY m_hKey;
};

////////////////////////////////////////////////////////////////////////////

template <typename T_STRUCT>
inline LONG MRegKey::QueryStruct(LPCTSTR pszValueName, T_STRUCT& data) noexcept
{
    assert(m_hKey);
    DWORD cbData = static_cast<DWORD>(sizeof(data));
    LONG result = ::RegQueryValueEx(m_hKey, pszValueName, nullptr, nullptr,
        reinterpret_cast<LPBYTE>(&data), &cbData);
    if (result == ERROR_SUCCESS && cbData != sizeof(data))
        result = ERROR_INVALID_DATA;
    return result;
}

template <typename T_STRUCT>
inline LONG MRegKey::SetStruct(LPCTSTR pszValueName, const T_STRUCT& data) noexcept
{
    assert(m_hKey);
    const DWORD cbData = static_cast<DWORD>(sizeof(data));
    return ::RegSetValueEx(m_hKey, pszValueName, 0, REG_BINARY,
        reinterpret_cast<const BYTE *>(&data), cbData);
}

template <typename T_CONTAINER>
LONG MRegKey::QueryMultiSz(LPCTSTR pszValueName, T_CONTAINER& container) noexcept
{
    container.clear();

    LONG result;
    #ifndef NDEBUG
        DWORD dwType;
        result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_MULTI_SZ);
    #endif

    DWORD cbData;
    result = RegQueryValueEx(pszValueName, nullptr, nullptr, nullptr, &cbData);
    if (result != ERROR_SUCCESS)
        return result;

    const DWORD cch = static_cast<DWORD>(cbData / sizeof(TCHAR) + 1);
    LPTSTR pszz = new(std::nothrow) TCHAR[cch];
    if (pszz)
    {
        result = QueryMultiSz(pszValueName, pszz, cch);
        if (result == ERROR_SUCCESS)
        {
            for (LPTSTR pch = pszz; *pch; pch += lstrlen(pch) + 1)
            {
                #if (__cplusplus >= 201103L)
                    container.emplace_back(pch);
                #else
                    container.push_back(pch);
                #endif
            }
        }
        delete[] pszz;
    }
    else
        result = ERROR_OUTOFMEMORY;

    return result;
}

template <typename T_CONTAINER>
inline LONG MRegKey::SetMultiSz(
    LPCTSTR pszValueName, const T_CONTAINER& container) noexcept
{
    typename T_CONTAINER::value_type         str;
    typename T_CONTAINER::const_iterator     it, end;

    it = container.begin();
    end = container.end();
    if (it != end)
    {
        for (; it != end; ++it)
        {
            str += *it;
            str += TEXT('\0');
        }
    }
    else
    {
        str += TEXT('\0');
    }

    const DWORD cchValues = static_cast<DWORD>(str.size() + 1);
    return SetMultiSz(pszValueName, str.c_str(), cchValues);
}

template <typename T_STRING>
LONG MRegKey::QuerySz(LPCTSTR pszValueName, T_STRING& strValue) noexcept
{
    LONG result;
    strValue.clear();

    #ifndef NDEBUG
        DWORD dwType;
        result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_SZ);
    #endif

    DWORD cbData;
    result = RegQueryValueEx(pszValueName, nullptr, nullptr, nullptr, &cbData);
    if (result != ERROR_SUCCESS)
        return result;

    LPTSTR psz = new(std::nothrow) TCHAR[cbData / sizeof(TCHAR) + 1];
    assert(psz);
    if (psz)
    {
        result = RegQueryValueEx(pszValueName, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(psz), &cbData);
        if (result == ERROR_SUCCESS)
        {
            strValue = psz;
        }
        delete[] psz;
    }
    return result;
}

template <typename T_STRING>
LONG MRegKey::QueryExpandSz(LPCTSTR pszValueName, T_STRING& strValue) noexcept
{
    LONG result;
    strValue.clear();

    #ifndef NDEBUG
        DWORD dwType;
        result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_EXPAND_SZ);
    #endif

    DWORD cbData;
    result = RegQueryValueEx(pszValueName, nullptr, nullptr, nullptr, &cbData);
    if (result != ERROR_SUCCESS)
        return result;

    LPTSTR psz = new(std::nothrow) TCHAR[cbData / sizeof(TCHAR) + 1];
    assert(psz);
    if (psz)
    {
        result = RegQueryValueEx(pszValueName, nullptr, nullptr,
                                 reinterpret_cast<LPBYTE>(psz), &cbData);
        if (result == ERROR_SUCCESS)
        {
            strValue = psz;
        }
        delete[] psz;
    }
    return result;
}

inline MRegKey::MRegKey() noexcept : m_hKey(nullptr)
{
}

inline MRegKey::MRegKey(HKEY hKey) noexcept : m_hKey(hKey)
{
}

inline MRegKey::MRegKey(
    HKEY hBaseKey, LPCTSTR pszSubKey,
    BOOL bCreate/* = FALSE*/) noexcept : m_hKey(nullptr)
{
    if (bCreate)
        RegCreateKeyEx(hBaseKey, pszSubKey);
    else
        RegOpenKeyEx(hBaseKey, pszSubKey);
}

inline MRegKey::MRegKey(MRegKey& key) noexcept : m_hKey(CloneHandleDx(key))
{
}

inline /*virtual*/ MRegKey::~MRegKey() noexcept
{
    RegCloseKey();
}

inline HKEY MRegKey::Handle() const noexcept
{
    return m_hKey;
}

inline MRegKey::operator HKEY() const noexcept
{
    return Handle();
}

inline bool MRegKey::operator!() const noexcept
{
    return Handle() == nullptr;
}

inline bool MRegKey::operator==(HKEY hKey) const noexcept
{
    return Handle() == hKey;
}

inline bool MRegKey::operator!=(HKEY hKey) const noexcept
{
    return Handle() != hKey;
}

inline MRegKey& MRegKey::operator=(HKEY hKey) noexcept
{
    if (Handle() != hKey)
    {
        Attach(hKey);
    }
    return *this;
}

inline MRegKey& MRegKey::operator=(MRegKey& key) noexcept
{
    if (Handle() != key.m_hKey)
    {
        HKEY hKey = CloneHandleDx(key);
        Attach(hKey);
    }
    return *this;
}

inline BOOL MRegKey::Attach(HKEY hKey) noexcept
{
    RegCloseKey();
    m_hKey = hKey;
    return m_hKey != nullptr;
}

inline HKEY MRegKey::Detach() noexcept
{
    HKEY hKey = m_hKey;
    m_hKey = nullptr;
    return hKey;
}

inline LONG MRegKey::RegCreateKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey,
    DWORD dwReserved/* = 0*/, LPTSTR lpClass/* = nullptr*/,
    DWORD dwOptions/* = 0*/, REGSAM samDesired/* = KEY_ALL_ACCESS*/,
    LPSECURITY_ATTRIBUTES lpsa/* = nullptr*/,
    LPDWORD lpdwDisposition/* = nullptr*/) noexcept
{
    UNREFERENCED_PARAMETER(dwReserved);
    assert(m_hKey == nullptr);
    return ::RegCreateKeyEx(hBaseKey, pszSubKey, 0,
        lpClass, dwOptions, samDesired, lpsa, &m_hKey, lpdwDisposition);
}

inline LONG MRegKey::RegOpenKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey,
    DWORD dwOptions/* = 0*/, REGSAM samDesired/* = KEY_READ*/) noexcept
{
    assert(m_hKey == nullptr);
    return ::RegOpenKeyEx(hBaseKey, pszSubKey, dwOptions, samDesired,
                          &m_hKey);
}

inline LONG
MRegKey::RegConnectRegistry(LPCTSTR lpMachineName, HKEY hBaseKey) noexcept
{
    assert(m_hKey == nullptr);
    return ::RegConnectRegistry(lpMachineName, hBaseKey, &m_hKey);
}

inline LONG MRegKey::RegCloseKey() noexcept
{
    if (Handle())
    {
        return ::RegCloseKey(Detach());
    }
    return ERROR_INVALID_HANDLE;
}

inline LONG MRegKey::RegQueryValueEx(LPCTSTR pszValueName/* = nullptr*/,
    LPDWORD lpReserved/* = nullptr*/, LPDWORD lpType/* = nullptr*/,
    LPBYTE lpData/* = nullptr*/, LPDWORD lpcbData/* = nullptr*/) noexcept
{
    assert(m_hKey);
    return ::RegQueryValueEx(m_hKey, pszValueName, lpReserved,
        lpType, lpData, lpcbData);
}


inline LONG MRegKey::QueryBinary(
    LPCTSTR pszValueName, LPVOID pvValue, DWORD cb) noexcept
{
    #ifndef NDEBUG
        DWORD dwType;
        const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                            nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_BINARY);
    #endif
    DWORD cbData = cb;
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           static_cast<LPBYTE>(pvValue),
                           &cbData);
}

inline LONG MRegKey::QueryDword(LPCTSTR pszValueName, DWORD& dw) noexcept
{
    #ifndef NDEBUG
        DWORD dwType;
        const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                            nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_DWORD);
    #endif
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG MRegKey::QueryDwordLE(LPCTSTR pszValueName, DWORD& dw) noexcept
{
    #ifndef NDEBUG
        DWORD dwType;
        const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                            nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_DWORD_LITTLE_ENDIAN);
    #endif
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG MRegKey::QueryDwordBE(LPCTSTR pszValueName, DWORD& dw) noexcept
{
    #ifndef NDEBUG
        DWORD dwType;
        const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                            nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_DWORD_BIG_ENDIAN);
    #endif
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG
MRegKey::QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue) noexcept
{
    #ifndef NDEBUG
        DWORD dwType;
        const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                            nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_SZ);
    #endif
    DWORD cbData = cchValue * sizeof(TCHAR);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pszValue), &cbData);
}

inline LONG MRegKey::QueryExpandSz(
    LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue) noexcept
{
#ifndef NDEBUG
    DWORD dwType;
    const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
    if (result == ERROR_SUCCESS)
        assert(dwType == REG_EXPAND_SZ);
#endif
    DWORD cbData = cchValue * sizeof(TCHAR);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pszValue), &cbData);
}

inline LONG MRegKey::QueryMultiSz(
    LPCTSTR pszValueName, LPTSTR pszzValues, DWORD cchValues) noexcept
{
#ifndef NDEBUG
    DWORD dwType;
    const LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
    if (result == ERROR_SUCCESS)
        assert(dwType == REG_MULTI_SZ);
#endif
    DWORD cbData = sizeof(TCHAR) * cchValues;
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pszzValues), &cbData);
}

inline LONG MRegKey::RegSetValueEx(LPCTSTR pszValueName, DWORD dwReserved,
    DWORD dwType, CONST BYTE *lpData, DWORD cbData) noexcept
{
    UNREFERENCED_PARAMETER(dwReserved);
    assert(m_hKey);
    return ::RegSetValueEx(m_hKey, pszValueName, 0, dwType,
        lpData, cbData);
}

inline LONG MRegKey::SetBinary(LPCTSTR pszValueName, LPCVOID pvValue, DWORD cb) noexcept
{
    return RegSetValueEx(pszValueName, 0, REG_BINARY,
        reinterpret_cast<const BYTE *>(pvValue), cb);
}

inline LONG MRegKey::SetDword(LPCTSTR pszValueName, DWORD dw) noexcept
{
    const DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG MRegKey::SetDwordLE(LPCTSTR pszValueName, DWORD dw) noexcept
{
    const DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD_LITTLE_ENDIAN,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG MRegKey::SetDwordBE(LPCTSTR pszValueName, DWORD dw) noexcept
{
    const DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD_BIG_ENDIAN,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG
MRegKey::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue) noexcept
{
    return RegSetValueEx(pszValueName, 0, REG_SZ,
        reinterpret_cast<const BYTE *>(pszValue), cchValue * sizeof(TCHAR));
}

inline LONG
MRegKey::SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue) noexcept
{
    return RegSetValueEx(pszValueName, 0, REG_EXPAND_SZ,
        reinterpret_cast<const BYTE *>(pszValue), cchValue * sizeof(TCHAR));
}

inline LONG
MRegKey::SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues) noexcept
{
    return RegSetValueEx(pszValueName, 0, REG_MULTI_SZ,
        reinterpret_cast<const BYTE *>(pszzValues),
        static_cast<DWORD>(MRegKey::MultiSzSizeDx(pszzValues)));
}

inline LONG
MRegKey::SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues, DWORD cchValues) noexcept
{
    return RegSetValueEx(pszValueName, 0, REG_MULTI_SZ,
        reinterpret_cast<const BYTE *>(pszzValues),
        static_cast<DWORD>(sizeof(TCHAR) * cchValues));
}

inline LONG MRegKey::RegDeleteValue(LPCTSTR pszValueName) noexcept
{
    assert(m_hKey);
    return ::RegDeleteValue(m_hKey, pszValueName);
}

inline LONG MRegKey::RegEnumKeyEx(DWORD dwIndex, LPTSTR lpName,
    LPDWORD lpcchName, LPDWORD lpReserved/* = nullptr*/,
    LPTSTR lpClass/* = nullptr*/, LPDWORD lpcchClass/* = nullptr*/,
    PFILETIME lpftLastWriteTime/* = nullptr*/) noexcept
{
    assert(m_hKey);
    return ::RegEnumKeyEx(m_hKey, dwIndex, lpName, lpcchName,
        lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
}

inline LONG MRegKey::RegEnumValue(DWORD dwIndex, LPTSTR lpName,
    LPDWORD lpcchName, LPDWORD lpReserved/* = nullptr*/, LPDWORD lpType/* = nullptr*/,
    LPBYTE lpData/* = nullptr*/, LPDWORD lpcbData/* = nullptr*/) noexcept
{
    assert(m_hKey);
    return ::RegEnumValue(m_hKey, dwIndex, lpName, lpcchName, lpReserved,
        lpType, lpData, lpcbData);
}

inline LONG MRegKey::RegFlushKey() noexcept
{
    assert(m_hKey);
    return ::RegFlushKey(m_hKey);
}

inline LONG MRegKey::RegGetKeySecurity(SECURITY_INFORMATION si,
    PSECURITY_DESCRIPTOR pSD, LPDWORD pcbSD) noexcept
{
    assert(m_hKey);
    return ::RegGetKeySecurity(m_hKey, si, pSD, pcbSD);
}

inline LONG MRegKey::RegNotifyChangeKeyValue(BOOL bWatchSubTree/* = TRUE*/,
    DWORD dwFilter/* = REG_LEGAL_CHANGE_FILTER*/,
    HANDLE hEvent/* = nullptr*/, BOOL bAsyncronous/* = FALSE*/) noexcept
{
    assert(m_hKey);
    return ::RegNotifyChangeKeyValue(m_hKey, bWatchSubTree, dwFilter,
        hEvent, bAsyncronous);
}

inline LONG MRegKey::RegQueryInfoKey(LPTSTR lpClass/* = nullptr*/,
    LPDWORD lpcchClass/* = nullptr*/,
    LPDWORD lpReserved/* = nullptr*/,
    LPDWORD lpcSubKeys/* = nullptr*/,
    LPDWORD lpcchMaxSubKeyLen/* = nullptr*/,
    LPDWORD lpcchMaxClassLen/* = nullptr*/,
    LPDWORD lpcValues/* = nullptr*/,
    LPDWORD lpcchMaxValueNameLen/* = nullptr*/,
    LPDWORD lpcbMaxValueLen/* = nullptr*/,
    LPDWORD lpcbSecurityDescriptor/* = nullptr*/,
    PFILETIME lpftLastWriteTime/* = nullptr*/) noexcept
{
    assert(m_hKey);
    return ::RegQueryInfoKey(m_hKey, lpClass, lpcchClass,
        lpReserved, lpcSubKeys, lpcchMaxSubKeyLen, lpcchMaxClassLen,
        lpcValues, lpcchMaxValueNameLen, lpcbMaxValueLen,
        lpcbSecurityDescriptor, lpftLastWriteTime);
}

inline LONG MRegKey::RegQueryMultipleValues(PVALENT val_list, DWORD num_vals,
                            LPTSTR lpValueBuf, LPDWORD lpdwTotsize) noexcept
{
    assert(m_hKey);
    return ::RegQueryMultipleValues(
        m_hKey, val_list, num_vals, lpValueBuf, lpdwTotsize);
}

inline LONG
MRegKey::RegSetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd) noexcept
{
    assert(m_hKey);
    return ::RegSetKeySecurity(m_hKey, si, psd);
}

inline /*static*/ LONG
MRegKey::RegLoadKey(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszFile) noexcept
{
    assert(pszSubKey);
    assert(pszFile);
    return ::RegLoadKey(hKey, pszSubKey, pszFile);
}

inline /*static*/ LONG MRegKey::RegUnLoadKey(HKEY hKey, LPCTSTR pszSubKey) noexcept
{
    assert(pszSubKey);
    return ::RegUnLoadKey(hKey, pszSubKey);
}

inline /*static*/ LONG
MRegKey::RegSaveKey(HKEY hKey, LPCTSTR pszFile,
                    LPSECURITY_ATTRIBUTES lpsa/* = nullptr*/) noexcept
{
    assert(pszFile);
    return ::RegSaveKey(hKey, pszFile, lpsa);
}

inline /*static*/
LONG MRegKey::RegRestoreKey(HKEY hKey, LPCTSTR pszFile, DWORD dwFlags) noexcept
{
    assert(pszFile);
    return ::RegRestoreKey(hKey, pszFile, dwFlags);
}

inline /*static*/ LONG MRegKey::RegReplaceKey(
    HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszNewFile, LPCTSTR pszOldFile) noexcept
{
    assert(pszNewFile);
    assert(pszOldFile);
    return ::RegReplaceKey(hKey, pszSubKey, pszNewFile, pszOldFile);
}

inline /*static*/ LONG MRegKey::RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey) noexcept
{
    return ::RegDeleteTreeDx(hKey, pszSubKey);
}

inline LONG MRegKey::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue) noexcept
{
    return SetSz(pszValueName, pszValue, lstrlen(pszValue) + 1);
}

inline LONG MRegKey::SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue) noexcept
{
    return SetExpandSz(pszValueName, pszValue, lstrlen(pszValue) + 1);
}

inline LONG MRegKey::RegDeleteTreeDx(LPCTSTR pszSubKey) noexcept
{
    assert(m_hKey);
    return RegDeleteTreeDx(m_hKey, pszSubKey);
}

inline LONG RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey/* = nullptr*/) noexcept
{
    DWORD cchSubKeyMax, cchValueMax;
    DWORD cchMax, cch;
    TCHAR szNameBuf[MAX_PATH], *pszName = szNameBuf;
    HKEY hSubKey = hKey;

    if (pszSubKey != nullptr)
    {
        const LONG ret = ::RegOpenKeyEx(hKey, pszSubKey, 0, KEY_READ, &hSubKey);
        if (ret)
            return ret;
    }

    LONG ret = ::RegQueryInfoKey(hSubKey, nullptr, nullptr, nullptr, nullptr,
        &cchSubKeyMax, nullptr, nullptr, &cchValueMax, nullptr, nullptr, nullptr);
    if (ret)
        goto cleanup;

    cchSubKeyMax++;
    cchValueMax++;
    if (cchSubKeyMax < cchValueMax)
        cchMax = cchValueMax;
    else
        cchMax = cchSubKeyMax;
    if (cchMax > sizeof(szNameBuf) / sizeof(TCHAR))
    {
        pszName = (TCHAR *)_malloca(cchMax * sizeof(TCHAR));
        if (pszName == nullptr)
            goto cleanup;
    }

    for(;;)
    {
        cch = cchMax;
        if (::RegEnumKeyEx(hSubKey, 0, pszName, &cch, nullptr,
                           nullptr, nullptr, nullptr))
        {
            break;
        }

        ret = RegDeleteTreeDx(hSubKey, pszName);
        if (ret)
            goto cleanup;
    }

    if (pszSubKey != nullptr)
    {
        ret = ::RegDeleteKey(hKey, pszSubKey);
    }
    else
    {
        // NOTE: if pszSubKey was nullptr, then delete value entries.
        for (;;)
        {
            cch = cchMax;
            if (::RegEnumValue(hKey, 0, pszName, &cch,
                               nullptr, nullptr, nullptr, nullptr))
            {
                break;
            }

            ret = ::RegDeleteValue(hKey, pszName);
            if (ret)
                goto cleanup;
        }
    }

cleanup:
    if (pszSubKey != nullptr)
        ::RegCloseKey(hSubKey);
    if (pszName != szNameBuf)
        _freea(pszName);
    return ret;
}

inline /*static*/ size_t MRegKey::MultiSzSizeDx(LPCTSTR pszz) noexcept
{
    size_t siz = 0;
    if (*pszz)
    {
        do
        {
            const size_t len = lstrlen(pszz);
            siz += len + 1;
            pszz += len + 1;
        }
        while (*pszz);
    }
    else
    {
        ++siz;
    }
    ++siz;
    siz *= sizeof(TCHAR);
    return siz;
}

inline /*static*/ HKEY MRegKey::CloneHandleDx(HKEY hKey) noexcept
{
    if (hKey == nullptr)
        return nullptr;

    HANDLE hProcess = ::GetCurrentProcess();
    HANDLE hDup = nullptr;
    ::DuplicateHandle(hProcess, hKey, hProcess, &hDup, 0,
                      FALSE, DUPLICATE_SAME_ACCESS);
    return reinterpret_cast<HKEY>(hDup);
}

////////////////////////////////////////////////////////////////////////////

#endif  // ndef MZC4_MREGKEY_HPP_
