/* SaveBitmapToFile.h --- loading/saving a bitmap in Win32 C/C++ */
/* Copyright (C) 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>. */
/* This file is public domain software. */
#ifndef SAVEBITMAPTOFILE_H_
#define SAVEBITMAPTOFILE_H_ 10  /* Version 10 */

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif

#ifdef UNICODE
    #define LoadBitmapFromFile LoadBitmapFromFileW
    #define SaveBitmapToFile SaveBitmapToFileW
#else
    #define LoadBitmapFromFile LoadBitmapFromFileA
    #define SaveBitmapToFile SaveBitmapToFileA
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* load a bitmap as DIB from a BMP/DIB file */
HBITMAP WINAPI LoadBitmapFromFileA(LPCSTR bmp_file);
HBITMAP WINAPI LoadBitmapFromFileW(LPCWSTR bmp_file);

/* save the bitmap to a BMP/DIB file */
BOOL WINAPI SaveBitmapToFileA(LPCSTR bmp_file, HBITMAP hbm);
BOOL WINAPI SaveBitmapToFileW(LPCWSTR bmp_file, HBITMAP hbm);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* ndef SAVEBITMAPTOFILE_H_ */
