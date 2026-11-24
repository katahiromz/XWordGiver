// SaveBitmapToFileTest.c --- Test for SaveBitmapToFile
// Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>.
// This file is public domain software.
#include "SaveBitmapToFile.h"
#include <stdio.h>

#ifdef __cplusplus
    #include <cassert>
#else
    #include <assert.h>
#endif

#ifndef ARRAYSIZE
    #define ARRAYSIZE(array) (sizeof(array) / sizeof((array)[0]))
#endif

int main(void)
{
    static const LPCTSTR array1[] =
    {
        TEXT("1bpp.bmp"),
        TEXT("4bpp.bmp"),
        TEXT("8bpp.bmp"),
        TEXT("24bpp.bmp"),
        TEXT("32bpp.bmp")
    };
    static const INT array2[] =
    {
        1, 4, 8, 24, 32
    };
    static const LPCTSTR array3[] =
    {
        TEXT("1bpp-saved.bmp"),
        TEXT("4bpp-saved.bmp"),
        TEXT("8bpp-saved.bmp"),
        TEXT("24bpp-saved.bmp"),
        TEXT("32bpp-saved.bmp")
    };
    HBITMAP hbm;
    BITMAP bm;
    size_t i, count;

    count = ARRAYSIZE(array1);
    for (i = 0; i < count; ++i)
    {
        hbm = LoadBitmapFromFile(array1[i]);
        assert(hbm != NULL);

        if (!GetObject(hbm, sizeof(bm), &bm))
            assert(0);

        assert(bm.bmBitsPixel == array2[i]);

        if (!SaveBitmapToFile(array3[i], hbm))
            assert(0);

        if (!DeleteObject(hbm))
            assert(0);
    }

    puts("success");
    return 0;
}
