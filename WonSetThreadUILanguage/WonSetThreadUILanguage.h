#pragma once

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

typedef LANGID (WINAPI *FN_GetThreadUILanguage)(VOID);
typedef LANGID (WINAPI *FN_SetThreadUILanguage)(LANGID);

//#include <versionhelpers.h>
static inline BOOL IsWindowsVistaOrGreater(void)
{
    OSVERSIONINFOW info = { sizeof(info) };
    if (!GetVersionExW(&info))
        return FALSE;
    return info.dwMajorVersion >= 6;
}

static inline LANGID WonGetThreadUILanguage()
{
    static FN_GetThreadUILanguage s_fn = NULL;

    if (IsWindowsVistaOrGreater())
    {
        if (!s_fn)
            s_fn = reinterpret_cast<FN_GetThreadUILanguage>(
                GetProcAddress(GetModuleHandleA("kernel32"), "GetThreadUILanguage"));
        if (s_fn)
            return (*s_fn)();
    }

    return LANGIDFROMLCID(GetThreadLocale());
}

static inline LANGID WonSetThreadUILanguage(LANGID LangID)
{
    static FN_SetThreadUILanguage s_fn = NULL;

    if (IsWindowsVistaOrGreater())
    {
        if (!s_fn)
            s_fn = reinterpret_cast<FN_SetThreadUILanguage>(
                GetProcAddress(GetModuleHandleA("kernel32"), "SetThreadUILanguage"));
        if (s_fn)
            return (*s_fn)(LangID);
    }

    if (SetThreadLocale(MAKELCID(LangID, SORT_DEFAULT)))
        return LangID;
    return 0;
}

#define GetThreadUILanguage WonGetThreadUILanguage
#define SetThreadUILanguage WonSetThreadUILanguage
