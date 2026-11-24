/* SaveBitmapToFile.c --- loading/saving a bitmap in Win32 C/C++ */
/* Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>. */
/* This file is public domain software. */
#include "SaveBitmapToFile.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BM_MAGIC 0x4D42  /* 'BM' */

typedef struct tagKHMZ_BITMAPINFOEX
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD          bmiColors[256];
} KHMZ_BITMAPINFOEX, FAR *LPKHMZ_BITMAPINFOEX;

HBITMAP WINAPI LoadBitmapFromFile(LPCTSTR bmp_file)
{
    HANDLE hFile;
    BITMAPFILEHEADER bf;
    KHMZ_BITMAPINFOEX bmi;
    DWORD cb, cbImage;
    LPVOID pvBits1, pvBits2;
    HDC hDC;
    HBITMAP hbm;

    /* LoadImage can load a bottom-up bitmap */
    hbm = (HBITMAP)LoadImage(NULL, bmp_file, IMAGE_BITMAP, 0, 0,
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (hbm)
        return hbm;

    /* Let's load a top-down bitmap */
    hFile = CreateFile(bmp_file, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        /* failed to open the file */
        return NULL;
    }

    if (!ReadFile(hFile, &bf, sizeof(BITMAPFILEHEADER), &cb, NULL))
    {
        /* failed to read the file */
        CloseHandle(hFile);
        return NULL;
    }

    /* allocate the bits and read from file */
    pvBits1 = NULL;
    if (bf.bfType == BM_MAGIC && bf.bfReserved1 == 0 && bf.bfReserved2 == 0 &&
        bf.bfSize > bf.bfOffBits && bf.bfOffBits > sizeof(BITMAPFILEHEADER) &&
        bf.bfOffBits <= sizeof(BITMAPFILEHEADER) + sizeof(KHMZ_BITMAPINFOEX))
    {
        cbImage = bf.bfSize - bf.bfOffBits;
        pvBits1 = HeapAlloc(GetProcessHeap(), 0, cbImage);
        if (pvBits1)
        {
            if (!ReadFile(hFile, &bmi, bf.bfOffBits -
                          sizeof(BITMAPFILEHEADER), &cb, NULL) ||
                !ReadFile(hFile, pvBits1, cbImage, &cb, NULL))
            {
                /* failed to read the file */
                HeapFree(GetProcessHeap(), 0, pvBits1);
                pvBits1 = NULL;
            }
        }
    }

    /* close the file */
    CloseHandle(hFile);

    if (pvBits1 == NULL)
    {
        return NULL;    /* allocation failure */
    }

    /* create the DIB object and get the handle */
    hbm = CreateDIBSection(NULL, (BITMAPINFO*)&bmi, DIB_RGB_COLORS,
                           &pvBits2, NULL, 0);
    if (hbm)
    {
        hDC = CreateCompatibleDC(NULL);
        if (!SetDIBits(hDC, hbm, 0, (UINT)labs(bmi.bmiHeader.biHeight),
                       pvBits1, (BITMAPINFO*)&bmi, DIB_RGB_COLORS))
        {
            /* failed to get the bits */
            DeleteObject(hbm);
            hbm = NULL;
        }
        DeleteDC(hDC);
    }

    /* clean up */
    HeapFree(GetProcessHeap(), 0, pvBits1);

    return hbm;
}

/* save the bitmap to a BMP/DIB file */
BOOL WINAPI SaveBitmapToFile(LPCTSTR bmp_file, HBITMAP hbm)
{
    BOOL fOK;
    BITMAPFILEHEADER bf;
    KHMZ_BITMAPINFOEX bmi;
    BITMAPINFOHEADER *pbmih;
    DWORD cb, cbColors;
    HDC hDC;
    HANDLE hFile;
    LPVOID pBits;
    BITMAP bm;

    /* verify the bitmap */
    if (!GetObject(hbm, sizeof(BITMAP), &bm))
    {
        return FALSE;
    }

    pbmih = &bmi.bmiHeader;
    ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
    pbmih->biSize             = sizeof(BITMAPINFOHEADER);
    pbmih->biWidth            = bm.bmWidth;
    pbmih->biHeight           = bm.bmHeight;
    pbmih->biPlanes           = 1;
    pbmih->biBitCount         = bm.bmBitsPixel;
    pbmih->biCompression      = BI_RGB;
    pbmih->biSizeImage        = (DWORD)(bm.bmWidthBytes * bm.bmHeight);

    /* size of color table */
    if (bm.bmBitsPixel <= 8)
        cbColors = (DWORD)((1ULL << bm.bmBitsPixel) * sizeof(RGBQUAD));
    else
        cbColors = 0;

    bf.bfType = BM_MAGIC;
    bf.bfReserved1 = 0;
    bf.bfReserved2 = 0;
    cb = sizeof(BITMAPFILEHEADER) + pbmih->biSize + cbColors;
    bf.bfOffBits = cb;
    bf.bfSize = cb + pbmih->biSizeImage;

    /* allocate the bits */
    pBits = HeapAlloc(GetProcessHeap(), 0, pbmih->biSizeImage);
    if (pBits == NULL)
    {
        return FALSE;   /* allocation failure */
    }

    /* create a DC */
    hDC = CreateCompatibleDC(NULL);

    /* get the bits */
    fOK = FALSE;
    if (GetDIBits(hDC, hbm, 0, (UINT)bm.bmHeight, pBits, (BITMAPINFO *)&bmi,
                  DIB_RGB_COLORS))
    {
        /* create the file */
        hFile = CreateFile(bmp_file, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL |
                           FILE_FLAG_WRITE_THROUGH, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            /* write to file */
            fOK = WriteFile(hFile, &bf, sizeof(bf), &cb, NULL) &&
                  WriteFile(hFile, &bmi, sizeof(BITMAPINFOHEADER) + cbColors, &cb, NULL) &&
                  WriteFile(hFile, pBits, pbmih->biSizeImage, &cb, NULL);

            /* close the file */
            CloseHandle(hFile);

            /* if writing failed, delete the file */
            if (!fOK)
            {
                DeleteFile(bmp_file);
            }
        }
    }

    /* delete the DC */
    DeleteDC(hDC);

    /* clean up */
    HeapFree(GetProcessHeap(), 0, pBits);

    return fOK;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
