// MRegKey.hpp -- Win32API registry key manipulator             -*- C++ -*-
// This file is part of MZC4.  See file "ReadMe.txt" and "License.txt".
////////////////////////////////////////////////////////////////////////////

#ifndef MZC4_MREGKEY_HPP_
#define MZC4_MREGKEY_HPP_       100 /* Version 100 */

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
#include <new>              // std::nothrow

////////////////////////////////////////////////////////////////////////////

// NOTE: RegDeleteTreeDx deletes all value entries if pszSubKey == nullptr.
// NOTE: RegDeleteTreeDx cannot delete opening keys.
LONG RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey/* = nullptr*/);

////////////////////////////////////////////////////////////////////////////

class MRegKey
{
public:
    MRegKey();
    MRegKey(HKEY hKey);
    MRegKey(MRegKey& key);
    MRegKey(HKEY hBaseKey, LPCTSTR pszSubKey, BOOL bCreate = FALSE);
    virtual ~MRegKey();

    operator HKEY() const;
    bool operator!() const;
    bool operator==(HKEY hKey) const;
    bool operator!=(HKEY hKey) const;
    MRegKey& operator=(HKEY hKey);
    MRegKey& operator=(MRegKey& key);

    BOOL Attach(HKEY hKey);
    HKEY Detach();
    HKEY Handle() const;

    LONG RegCreateKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey, DWORD dwReserved = 0,
                        LPTSTR lpClass = nullptr, DWORD dwOptions = 0,
                        REGSAM samDesired = KEY_ALL_ACCESS,
                        LPSECURITY_ATTRIBUTES lpsa = nullptr,
                        LPDWORD lpdwDisposition = nullptr);
    LONG RegOpenKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey, DWORD dwOptions = 0,
                      REGSAM samDesired = KEY_READ);

    LONG RegConnectRegistry(LPCTSTR lpMachineName, HKEY hBaseKey);

    LONG RegCloseKey();

    LONG RegQueryValueEx(LPCTSTR pszValueName = nullptr,
                         LPDWORD lpReserved = nullptr, LPDWORD lpType = nullptr,
                         LPBYTE lpData = nullptr, LPDWORD lpcbData = nullptr);

    LONG QueryBinary(LPCTSTR pszValueName, LPVOID pvValue, DWORD cb);
    LONG QueryDword(LPCTSTR pszValueName, DWORD& dw);
    LONG QueryDwordLE(LPCTSTR pszValueName, DWORD& dw);
    LONG QueryDwordBE(LPCTSTR pszValueName, DWORD& dw);
    LONG QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue);
    LONG QueryExpandSz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue);
    LONG QueryMultiSz(LPCTSTR pszValueName, LPTSTR pszzValues, DWORD cchValues);
    template <typename T_CONTAINER>
    LONG QueryMultiSz(LPCTSTR pszValueName, T_CONTAINER& container);
    template <typename T_STRUCT>
    LONG QueryStruct(LPCTSTR pszValueName, T_STRUCT& data);

    template <typename T_STRING>
    LONG QuerySz(LPCTSTR pszValueName, T_STRING& strValue);
    template <typename T_STRING>
    LONG QueryExpandSz(LPCTSTR pszValueName, T_STRING& strValue);

    LONG RegSetValueEx(LPCTSTR pszValueName, DWORD dwReserved,
        DWORD dwType, CONST BYTE *lpData, DWORD cbData);

    LONG SetBinary(LPCTSTR pszValueName, LPCVOID pvValue, DWORD cb);
    LONG SetDword(LPCTSTR pszValueName, DWORD dw);
    LONG SetDwordLE(LPCTSTR pszValueName, DWORD dw);
    LONG SetDwordBE(LPCTSTR pszValueName, DWORD dw);
    LONG SetSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue);
    LONG SetSz(LPCTSTR pszValueName, LPCTSTR pszValue);
    LONG SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue);
    LONG SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue);
    LONG SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues);
    LONG SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues, DWORD cchValues);
    template <typename T_CONTAINER>
    LONG SetMultiSz(LPCTSTR pszValueName, const T_CONTAINER& container);
    template <typename T_STRUCT>
    LONG SetStruct(LPCTSTR pszValueName, const T_STRUCT& data);

    LONG RegDeleteValue(LPCTSTR pszValueName);
    LONG RegDeleteTreeDx(LPCTSTR pszSubKey);
    LONG RegEnumKeyEx(DWORD dwIndex, LPTSTR lpName, LPDWORD lpcchName,
                      LPDWORD lpReserved = nullptr, LPTSTR lpClass = nullptr,
                      LPDWORD lpcchClass = nullptr,
                      PFILETIME lpftLastWriteTime = nullptr);
    LONG RegEnumValue(DWORD dwIndex, LPTSTR lpName, LPDWORD lpcchName,
                      LPDWORD lpReserved = nullptr, LPDWORD lpType = nullptr,
                      LPBYTE lpData = nullptr, LPDWORD lpcbData = nullptr);

    LONG RegFlushKey();
    LONG RegGetKeySecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD, LPDWORD pcbSD);

    LONG RegNotifyChangeKeyValue(BOOL bWatchSubTree = TRUE,
        DWORD dwFilter = REG_LEGAL_CHANGE_FILTER,
        HANDLE hEvent = nullptr, BOOL bAsyncronous = FALSE);

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
        PFILETIME lpftLastWriteTime = nullptr);

    LONG RegQueryMultipleValues(PVALENT val_list, DWORD num_vals,
                                LPTSTR lpValueBuf, LPDWORD lpdwTotsize);
    LONG RegSetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd);
    
    static LONG RegLoadKey(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszFile);
    static LONG RegUnLoadKey(HKEY hKey, LPCTSTR pszSubKey);
    static LONG RegSaveKey(HKEY hKey, LPCTSTR pszFile,
                           LPSECURITY_ATTRIBUTES lpsa = nullptr);
    static LONG RegRestoreKey(HKEY hKey, LPCTSTR pszFile, DWORD dwFlags);
    static LONG RegReplaceKey(HKEY hKey, LPCTSTR pszSubKey,
                              LPCTSTR pszNewFile, LPCTSTR pszOldFile);
    static LONG RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey);
    static size_t MultiSzSizeDx(LPCTSTR pszz);

    static HKEY CloneHandleDx(HKEY hKey);

protected:
    HKEY m_hKey;
};

////////////////////////////////////////////////////////////////////////////

template <typename T_STRUCT>
inline LONG MRegKey::QueryStruct(LPCTSTR pszValueName, T_STRUCT& data)
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
inline LONG MRegKey::SetStruct(LPCTSTR pszValueName, const T_STRUCT& data)
{
    assert(m_hKey);
    const DWORD cbData = static_cast<DWORD>(sizeof(data));
    return ::RegSetValueEx(m_hKey, pszValueName, 0, REG_BINARY,
        reinterpret_cast<const BYTE *>(&data), cbData);
}

template <typename T_CONTAINER>
LONG MRegKey::QueryMultiSz(LPCTSTR pszValueName, T_CONTAINER& container)
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
    LPCTSTR pszValueName, const T_CONTAINER& container)
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
LONG MRegKey::QuerySz(LPCTSTR pszValueName, T_STRING& strValue)
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
LONG MRegKey::QueryExpandSz(LPCTSTR pszValueName, T_STRING& strValue)
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

inline MRegKey::MRegKey() : m_hKey(nullptr)
{
}

inline MRegKey::MRegKey(HKEY hKey) : m_hKey(hKey)
{
}

inline MRegKey::MRegKey(
    HKEY hBaseKey, LPCTSTR pszSubKey,
    BOOL bCreate/* = FALSE*/) : m_hKey(nullptr)
{
    if (bCreate)
        RegCreateKeyEx(hBaseKey, pszSubKey);
    else
        RegOpenKeyEx(hBaseKey, pszSubKey);
}

inline MRegKey::MRegKey(MRegKey& key) : m_hKey(CloneHandleDx(key))
{
}

inline /*virtual*/ MRegKey::~MRegKey()
{
    RegCloseKey();
}

inline HKEY MRegKey::Handle() const
{
    return m_hKey;
}

inline MRegKey::operator HKEY() const
{
    return Handle();
}

inline bool MRegKey::operator!() const
{
    return Handle() == nullptr;
}

inline bool MRegKey::operator==(HKEY hKey) const
{
    return Handle() == hKey;
}

inline bool MRegKey::operator!=(HKEY hKey) const
{
    return Handle() != hKey;
}

inline MRegKey& MRegKey::operator=(HKEY hKey)
{
    if (Handle() != hKey)
    {
        Attach(hKey);
    }
    return *this;
}

inline MRegKey& MRegKey::operator=(MRegKey& key)
{
    if (Handle() != key.m_hKey)
    {
        HKEY hKey = CloneHandleDx(key);
        Attach(hKey);
    }
    return *this;
}

inline BOOL MRegKey::Attach(HKEY hKey)
{
    RegCloseKey();
    m_hKey = hKey;
    return m_hKey != nullptr;
}

inline HKEY MRegKey::Detach()
{
    HKEY hKey = m_hKey;
    m_hKey = nullptr;
    return hKey;
}

inline LONG MRegKey::RegCreateKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey,
    DWORD dwReserved/* = 0*/, LPTSTR lpClass/* = nullptr*/,
    DWORD dwOptions/* = 0*/, REGSAM samDesired/* = KEY_ALL_ACCESS*/,
    LPSECURITY_ATTRIBUTES lpsa/* = nullptr*/,
    LPDWORD lpdwDisposition/* = nullptr*/)
{
    UNREFERENCED_PARAMETER(dwReserved);
    assert(m_hKey == nullptr);
    return ::RegCreateKeyEx(hBaseKey, pszSubKey, 0,
        lpClass, dwOptions, samDesired, lpsa, &m_hKey, lpdwDisposition);
}

inline LONG MRegKey::RegOpenKeyEx(HKEY hBaseKey, LPCTSTR pszSubKey,
    DWORD dwOptions/* = 0*/, REGSAM samDesired/* = KEY_READ*/)
{
    assert(m_hKey == nullptr);
    return ::RegOpenKeyEx(hBaseKey, pszSubKey, dwOptions, samDesired,
                          &m_hKey);
}

inline LONG
MRegKey::RegConnectRegistry(LPCTSTR lpMachineName, HKEY hBaseKey)
{
    assert(m_hKey == nullptr);
    return ::RegConnectRegistry(lpMachineName, hBaseKey, &m_hKey);
}

inline LONG MRegKey::RegCloseKey()
{
    if (Handle())
    {
        LONG result = ::RegCloseKey(Detach());
        return result;
    }
    return ERROR_INVALID_HANDLE;
}

inline LONG MRegKey::RegQueryValueEx(LPCTSTR pszValueName/* = nullptr*/,
    LPDWORD lpReserved/* = nullptr*/, LPDWORD lpType/* = nullptr*/,
    LPBYTE lpData/* = nullptr*/, LPDWORD lpcbData/* = nullptr*/)
{
    assert(m_hKey);
    return ::RegQueryValueEx(m_hKey, pszValueName, lpReserved,
        lpType, lpData, lpcbData);
}


inline LONG MRegKey::QueryBinary(
    LPCTSTR pszValueName, LPVOID pvValue, DWORD cb)
{
    #ifndef NDEBUG
        DWORD dwType;
        LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                      nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_BINARY);
    #endif
    DWORD cbData = cb;
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pvValue),
                           &cbData);
}

inline LONG MRegKey::QueryDword(LPCTSTR pszValueName, DWORD& dw)
{
    #ifndef NDEBUG
        DWORD dwType;
        LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                      nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_DWORD);
    #endif
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG MRegKey::QueryDwordLE(LPCTSTR pszValueName, DWORD& dw)
{
    #ifndef NDEBUG
        DWORD dwType;
        LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                      nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_DWORD_LITTLE_ENDIAN);
    #endif
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG MRegKey::QueryDwordBE(LPCTSTR pszValueName, DWORD& dw)
{
    #ifndef NDEBUG
        DWORD dwType;
        LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                      nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_DWORD_BIG_ENDIAN);
    #endif
    DWORD cbData = sizeof(DWORD);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(&dw), &cbData);
}

inline LONG
MRegKey::QuerySz(LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue)
{
    #ifndef NDEBUG
        DWORD dwType;
        LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType,
                                      nullptr, nullptr);
        if (result == ERROR_SUCCESS)
            assert(dwType == REG_SZ);
    #endif
    DWORD cbData = cchValue * sizeof(TCHAR);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pszValue), &cbData);
}

inline LONG MRegKey::QueryExpandSz(
    LPCTSTR pszValueName, LPTSTR pszValue, DWORD cchValue)
{
#ifndef NDEBUG
    DWORD dwType;
    LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
    if (result == ERROR_SUCCESS)
        assert(dwType == REG_EXPAND_SZ);
#endif
    DWORD cbData = cchValue * sizeof(TCHAR);
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pszValue), &cbData);
}

inline LONG MRegKey::QueryMultiSz(
    LPCTSTR pszValueName, LPTSTR pszzValues, DWORD cchValues)
{
#ifndef NDEBUG
    DWORD dwType;
    LONG result = RegQueryValueEx(pszValueName, nullptr, &dwType, nullptr, nullptr);
    if (result == ERROR_SUCCESS)
        assert(dwType == REG_MULTI_SZ);
#endif
    DWORD cbData = sizeof(TCHAR) * cchValues;
    return RegQueryValueEx(pszValueName, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(pszzValues), &cbData);
}

inline LONG MRegKey::RegSetValueEx(LPCTSTR pszValueName, DWORD dwReserved,
    DWORD dwType, CONST BYTE *lpData, DWORD cbData)
{
    UNREFERENCED_PARAMETER(dwReserved);
    assert(m_hKey);
    return ::RegSetValueEx(m_hKey, pszValueName, 0, dwType,
        lpData, cbData);
}

inline LONG MRegKey::SetBinary(LPCTSTR pszValueName, LPCVOID pvValue, DWORD cb)
{
    return RegSetValueEx(pszValueName, 0, REG_BINARY,
        reinterpret_cast<const BYTE *>(pvValue), cb);
}

inline LONG MRegKey::SetDword(LPCTSTR pszValueName, DWORD dw)
{
    DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG MRegKey::SetDwordLE(LPCTSTR pszValueName, DWORD dw)
{
    DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD_LITTLE_ENDIAN,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG MRegKey::SetDwordBE(LPCTSTR pszValueName, DWORD dw)
{
    DWORD dwValue = dw;
    return RegSetValueEx(pszValueName, 0, REG_DWORD_BIG_ENDIAN,
        reinterpret_cast<const BYTE *>(&dwValue), sizeof(DWORD));
}

inline LONG
MRegKey::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue)
{
    return RegSetValueEx(pszValueName, 0, REG_SZ,
        reinterpret_cast<const BYTE *>(pszValue), cchValue * sizeof(TCHAR));
}

inline LONG
MRegKey::SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD cchValue)
{
    return RegSetValueEx(pszValueName, 0, REG_EXPAND_SZ,
        reinterpret_cast<const BYTE *>(pszValue), cchValue * sizeof(TCHAR));
}

inline LONG
MRegKey::SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues)
{
    return RegSetValueEx(pszValueName, 0, REG_MULTI_SZ,
        reinterpret_cast<const BYTE *>(pszzValues),
        DWORD(MRegKey::MultiSzSizeDx(pszzValues)));
}

inline LONG
MRegKey::SetMultiSz(LPCTSTR pszValueName, LPCTSTR pszzValues, DWORD cchValues)
{
    DWORD cb = static_cast<DWORD>(sizeof(TCHAR) * cchValues);
    return RegSetValueEx(pszValueName, 0, REG_MULTI_SZ,
        reinterpret_cast<const BYTE *>(pszzValues), cb);
}

inline LONG MRegKey::RegDeleteValue(LPCTSTR pszValueName)
{
    assert(m_hKey);
    return ::RegDeleteValue(m_hKey, pszValueName);
}

inline LONG MRegKey::RegEnumKeyEx(DWORD dwIndex, LPTSTR lpName,
    LPDWORD lpcchName, LPDWORD lpReserved/* = nullptr*/,
    LPTSTR lpClass/* = nullptr*/, LPDWORD lpcchClass/* = nullptr*/,
    PFILETIME lpftLastWriteTime/* = nullptr*/)
{
    assert(m_hKey);
    return ::RegEnumKeyEx(m_hKey, dwIndex, lpName, lpcchName,
        lpReserved, lpClass, lpcchClass, lpftLastWriteTime);
}

inline LONG MRegKey::RegEnumValue(DWORD dwIndex, LPTSTR lpName,
    LPDWORD lpcchName, LPDWORD lpReserved/* = nullptr*/, LPDWORD lpType/* = nullptr*/,
    LPBYTE lpData/* = nullptr*/, LPDWORD lpcbData/* = nullptr*/)
{
    assert(m_hKey);
    return ::RegEnumValue(m_hKey, dwIndex, lpName, lpcchName, lpReserved,
        lpType, lpData, lpcbData);
}

inline LONG MRegKey::RegFlushKey()
{
    assert(m_hKey);
    return ::RegFlushKey(m_hKey);
}

inline LONG MRegKey::RegGetKeySecurity(SECURITY_INFORMATION si,
    PSECURITY_DESCRIPTOR pSD, LPDWORD pcbSD)
{
    assert(m_hKey);
    return ::RegGetKeySecurity(m_hKey, si, pSD, pcbSD);
}

inline LONG MRegKey::RegNotifyChangeKeyValue(BOOL bWatchSubTree/* = TRUE*/,
    DWORD dwFilter/* = REG_LEGAL_CHANGE_FILTER*/,
    HANDLE hEvent/* = nullptr*/, BOOL bAsyncronous/* = FALSE*/)
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
    PFILETIME lpftLastWriteTime/* = nullptr*/)
{
    assert(m_hKey);
    return ::RegQueryInfoKey(m_hKey, lpClass, lpcchClass,
        lpReserved, lpcSubKeys, lpcchMaxSubKeyLen, lpcchMaxClassLen,
        lpcValues, lpcchMaxValueNameLen, lpcbMaxValueLen,
        lpcbSecurityDescriptor, lpftLastWriteTime);
}

inline LONG MRegKey::RegQueryMultipleValues(PVALENT val_list, DWORD num_vals,
                            LPTSTR lpValueBuf, LPDWORD lpdwTotsize)
{
    assert(m_hKey);
    return ::RegQueryMultipleValues(
        m_hKey, val_list, num_vals, lpValueBuf, lpdwTotsize);
}

inline LONG
MRegKey::RegSetKeySecurity(SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR psd)
{
    assert(m_hKey);
    return ::RegSetKeySecurity(m_hKey, si, psd);
}

inline /*static*/ LONG
MRegKey::RegLoadKey(HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszFile)
{
    assert(pszSubKey);
    assert(pszFile);
    return ::RegLoadKey(hKey, pszSubKey, pszFile);
}

inline /*static*/ LONG MRegKey::RegUnLoadKey(HKEY hKey, LPCTSTR pszSubKey)
{
    assert(pszSubKey);
    return ::RegUnLoadKey(hKey, pszSubKey);
}

inline /*static*/ LONG
MRegKey::RegSaveKey(HKEY hKey, LPCTSTR pszFile,
                    LPSECURITY_ATTRIBUTES lpsa/* = nullptr*/)
{
    assert(pszFile);
    return ::RegSaveKey(hKey, pszFile, lpsa);
}

inline /*static*/
LONG MRegKey::RegRestoreKey(HKEY hKey, LPCTSTR pszFile, DWORD dwFlags)
{
    assert(pszFile);
    return ::RegRestoreKey(hKey, pszFile, dwFlags);
}

inline /*static*/ LONG MRegKey::RegReplaceKey(
    HKEY hKey, LPCTSTR pszSubKey, LPCTSTR pszNewFile, LPCTSTR pszOldFile)
{
    assert(pszNewFile);
    assert(pszOldFile);
    return ::RegReplaceKey(hKey, pszSubKey, pszNewFile, pszOldFile);
}

inline /*static*/ LONG MRegKey::RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey)
{
    return ::RegDeleteTreeDx(hKey, pszSubKey);
}

inline LONG MRegKey::SetSz(LPCTSTR pszValueName, LPCTSTR pszValue)
{
    return SetSz(pszValueName, pszValue, lstrlen(pszValue) + 1);
}

inline LONG MRegKey::SetExpandSz(LPCTSTR pszValueName, LPCTSTR pszValue)
{
    return SetExpandSz(pszValueName, pszValue, lstrlen(pszValue) + 1);
}

inline LONG MRegKey::RegDeleteTreeDx(LPCTSTR pszSubKey)
{
    assert(m_hKey);
    return RegDeleteTreeDx(m_hKey, pszSubKey);
}

inline LONG RegDeleteTreeDx(HKEY hKey, LPCTSTR pszSubKey/* = nullptr*/)
{
    LONG ret;
    DWORD cchSubKeyMax, cchValueMax;
    DWORD cchMax, cch;
    TCHAR szNameBuf[MAX_PATH], *pszName = szNameBuf;
    HKEY hSubKey = hKey;

    if (pszSubKey != nullptr)
    {
        ret = ::RegOpenKeyEx(hKey, pszSubKey, 0, KEY_READ, &hSubKey);
        if (ret)
            return ret;
    }

    ret = ::RegQueryInfoKey(hSubKey, nullptr, nullptr, nullptr, nullptr,
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
        pszName = new(std::nothrow) TCHAR[cchMax * sizeof(TCHAR)];
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
        delete[] pszName;
    return ret;
}

inline /*static*/ size_t MRegKey::MultiSzSizeDx(LPCTSTR pszz)
{
    size_t siz = 0;
    if (*pszz)
    {
        do
        {
            size_t len = lstrlen(pszz);
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

inline /*static*/ HKEY MRegKey::CloneHandleDx(HKEY hKey)
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
