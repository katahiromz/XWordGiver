#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <map>
using namespace std;

#ifndef ARRAYSIZE
    #define ARRAYSIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

LPSTR LoadStringDx(INT nID)
{
    static char s_buf[128];
    LoadStringA(NULL, nID, s_buf, nID);
    return s_buf;
}

void mstr_trim(std::wstring& str, const WCHAR *spaces)
{
    size_t i = str.find_first_not_of(spaces);
    size_t j = str.find_last_not_of(spaces);
    if ((i == std::wstring::npos) || (j == std::wstring::npos))
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

template <size_t siz>
void mstr_trim(WCHAR (&str)[siz], const WCHAR *spaces)
{
    std::wstring s = str;
    mstr_trim(s, spaces);
    lstrcpyW(str, s.c_str());
}

LPWSTR fgetws_be(LPWSTR psz, size_t cch, FILE *fp)
{
    WCHAR ch, *pch = psz;
    size_t ich = 0;
    BOOL bHasRead = FALSE;
    if (cch == 0)
        return NULL;
    while (fread(&ch, sizeof(WCHAR), 1, fp))
    {
        bHasRead = TRUE;
        BYTE lo = LOBYTE(ch);
        BYTE hi = HIBYTE(ch);
        ch = MAKEWORD(hi, lo);
        *pch++ = ch;
        ++ich;
        if (ch == L'\n' || ich == cch)
        {
            *pch = 0;
            break;
        }
        ++ich;
    }
    return bHasRead ? psz : NULL;
}

enum CODE
{
    CODE_ANSI,
    CODE_UTF8,
    CODE_UTF16,
    CODE_UTF16BE,
};

typedef std::map<size_t, int> map_t;

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf(LoadStringDx(100));
        return EXIT_SUCCESS;
    }

    if (FILE *fp = fopen(argv[1], "rb"))
    {
        char szTextA[256];
        WCHAR szTextW[256];
        CODE type;

        fgets(szTextA, sizeof(szTextA), fp);
        if (memcmp(szTextA, "\xEF\xBB\xBF", 3) == 0)
        {
            type = CODE_UTF8;
            fseek(fp, 3, SEEK_SET);
        }
        else if (memcmp(szTextA, "\xFF\xFE", 2) == 0)
        {
            type = CODE_UTF16;
            fseek(fp, 2, SEEK_SET);
        }
        else if (memcmp(szTextA, "\xFE\xFF", 2) == 0)
        {
            type = CODE_UTF16BE;
            fseek(fp, 2, SEEK_SET);
        }
        else
        {
            if (szTextA[0] == 0)
            {
                type = CODE_UTF16BE;
            }
            else if (szTextA[1] == 0)
            {
                type = CODE_UTF16;
            }
            else
            {
                type = CODE_ANSI;
            }
            fseek(fp, 0, SEEK_SET);
        }

        map_t the_map;
        switch (type)
        {
        case CODE_ANSI:
            printf("CODE_ANSI\n");
            while (fgets(szTextA, sizeof(szTextA), fp))
            {
                if (szTextA[0] == '#')
                    continue;
                MultiByteToWideChar(CP_ACP, 0, szTextA, -1, szTextW, ARRAYSIZE(szTextW));
                mstr_trim(szTextW, L" \t\r\n");
                if (WCHAR *pch = wcschr(szTextW, L'\t'))
                {
                    *pch = 0;
                }
                size_t len = wcslen(szTextW);
                ++the_map[len];
            }
            break;

        case CODE_UTF8:
            printf("CODE_UTF8\n");
            while (fgets(szTextA, sizeof(szTextA), fp))
            {
                if (szTextA[0] == '#')
                    continue;
                MultiByteToWideChar(CP_UTF8, 0, szTextA, -1, szTextW, ARRAYSIZE(szTextW));
                mstr_trim(szTextW, L" \t\r\n");
                if (WCHAR *pch = wcschr(szTextW, L'\t'))
                {
                    *pch = 0;
                }
                size_t len = wcslen(szTextW);
                ++the_map[len];
            }
            break;

        case CODE_UTF16:
            printf("CODE_UTF16\n");
            while (fgetws(szTextW, ARRAYSIZE(szTextW), fp))
            {
                if (szTextW[0] == L'#')
                    continue;
                mstr_trim(szTextW, L" \t\r\n");
                if (WCHAR *pch = wcschr(szTextW, L'\t'))
                {
                    *pch = 0;
                }
                size_t len = wcslen(szTextW);
                ++the_map[len];
            }
            break;

        case CODE_UTF16BE:
            printf("CODE_UTF16BE\n");
            while (fgetws_be(szTextW, ARRAYSIZE(szTextW), fp))
            {
                if (szTextW[0] == L'#')
                    continue;
                mstr_trim(szTextW, L" \t\r\n");
                if (WCHAR *pch = wcschr(szTextW, L'\t'))
                {
                    *pch = 0;
                }
                size_t len = wcslen(szTextW);
                ++the_map[len];
            }
            break;
        }
        fclose(fp);

        map_t::iterator it;
        for (it = the_map.begin(); it != the_map.end(); ++it)
        {
            printf(LoadStringDx(101), (int)it->first, it->second);
        }

        return EXIT_SUCCESS;
    }

    printf(LoadStringDx(102), argv[1]);
    return EXIT_FAILURE;
}
