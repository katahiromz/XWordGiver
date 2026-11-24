// image-io.cpp --- 画像ファイルの読み書き。
// Author: katahiromz
// License: MIT

#include "image-io.h"
#include <gdiplus.h>

// 24BPPビットマップを作成。
HBITMAP __fastcall XgCreate24BppBitmap(HDC hDC, LONG width, LONG height) noexcept {
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	return CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
}

// BITMAPINFOEX構造体。
typedef struct tagBITMAPINFOEX {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD		  bmiColors[256];
} BITMAPINFOEX, FAR * LPBITMAPINFOEX;

BOOL PackedDIB_CreateFromHandle(std::vector<BYTE>& vecData, HBITMAP hbm) {
	vecData.clear();

	BITMAP bm;
	if (!GetObject(hbm, sizeof(bm), &bm))
		return FALSE;

	BITMAPINFOEX bi;
	BITMAPINFOHEADER *pbmih;
	DWORD cColors, cbColors;

	pbmih = &bi.bmiHeader;
	ZeroMemory(pbmih, sizeof(BITMAPINFOHEADER));
	pbmih->biSize			 = sizeof(BITMAPINFOHEADER);
	pbmih->biWidth			= bm.bmWidth;
	pbmih->biHeight		   = bm.bmHeight;
	pbmih->biPlanes		   = 1;
	pbmih->biBitCount		 = bm.bmBitsPixel;
	pbmih->biSizeImage		= bm.bmWidthBytes * bm.bmHeight;

	if (bm.bmBitsPixel < 16)
		cColors = 1 << bm.bmBitsPixel;
	else
		cColors = 0;
	cbColors = cColors * sizeof(RGBQUAD);

	std::vector<BYTE> Bits(pbmih->biSizeImage);
	HDC hDC = CreateCompatibleDC(nullptr);
	if (hDC == nullptr)
		return FALSE;

	auto pbi = reinterpret_cast<LPBITMAPINFO>(&bi);
	if (!GetDIBits(hDC, hbm, 0, bm.bmHeight, &Bits[0], pbi, DIB_RGB_COLORS))
	{
		DeleteDC(hDC);
		return FALSE;
	}

	DeleteDC(hDC);

	MStringA stream;
	stream.append(reinterpret_cast<const char *>(pbmih), sizeof(*pbmih));
	stream.append(reinterpret_cast<const char *>(bi.bmiColors), cbColors);
	stream.append(reinterpret_cast<const char *>(&Bits[0]), Bits.size());
	vecData.assign(stream.begin(), stream.end());
	return TRUE;
}

// 画像を読み込む。
BOOL __fastcall XgLoadImage(const MStringW& filename, HBITMAP& hbm, HENHMETAFILE& hEMF) noexcept {
	hbm = nullptr;
	hEMF = nullptr;

	// パス名をセット。
	WCHAR szFullPath[MAX_PATH];
	if (!GetFullPathNameW(filename.c_str(), _countof(szFullPath), szFullPath, nullptr))
		return FALSE;

	LPCWSTR pchDotExt = PathFindExtensionW(szFullPath);
	if (lstrcmpiW(pchDotExt, L".bmp") == 0)
	{
		hbm = LoadBitmapFromFileW(szFullPath);
		return hbm != nullptr;
	}
	if (lstrcmpiW(pchDotExt, L".emf") == 0)
	{
		// ロックを防ぐためにメモリーに読み込む。
		HENHMETAFILE hGotEMF = ::GetEnhMetaFileW(szFullPath);
		hEMF = ::CopyEnhMetaFile(hGotEMF, nullptr);
		::DeleteEnhMetaFile(hGotEMF);
		return hEMF != nullptr;
	}

	// GDI+で読み込む。
	if (HINSTANCE hGdiPlus = LoadLibraryA("gdiplus"))
	{
		using namespace Gdiplus;
		typedef GpStatus (WINAPI *FN_GdiplusStartup)(ULONG_PTR*,GDIPCONST GdiplusStartupInput*,GdiplusStartupOutput*);
		typedef VOID (WINAPI *FN_GdiplusShutdown)(ULONG_PTR);
		typedef GpStatus (WINAPI *FN_GdipCreateBitmapFromFile)(GDIPCONST WCHAR*,GpBitmap**);
		typedef GpStatus (WINAPI *FN_GdipCreateHBITMAPFromBitmap)(GpBitmap*,HBITMAP*,ARGB);
		typedef GpStatus (WINAPI *FN_GdipDisposeImage)(GpImage*);

		auto GdiplusStartup = reinterpret_cast<FN_GdiplusStartup>(::GetProcAddress(hGdiPlus, "GdiplusStartup"));
		auto GdiplusShutdown = reinterpret_cast<FN_GdiplusShutdown>(::GetProcAddress(hGdiPlus, "GdiplusShutdown"));
		auto GdipCreateBitmapFromFile = reinterpret_cast<FN_GdipCreateBitmapFromFile>(::GetProcAddress(hGdiPlus, "GdipCreateBitmapFromFile"));
		auto GdipCreateHBITMAPFromBitmap = reinterpret_cast<FN_GdipCreateHBITMAPFromBitmap>(::GetProcAddress(hGdiPlus, "GdipCreateHBITMAPFromBitmap"));
		auto GdipDisposeImage = reinterpret_cast<FN_GdipDisposeImage>(::GetProcAddress(hGdiPlus, "GdipDisposeImage"));

		if (GdiplusStartup &&
			GdiplusShutdown &&
			GdipCreateBitmapFromFile &&
			GdipCreateHBITMAPFromBitmap &&
			GdipDisposeImage)
		{
			GdiplusStartupInput gdiplusStartupInput;
			ULONG_PTR gdiplusToken;

			// GDI+の初期化。
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

			Color c;
			c.SetFromCOLORREF(RGB(255, 255, 255));

			GpBitmap *pBitmap = nullptr;
			GdipCreateBitmapFromFile(szFullPath, &pBitmap);
			if (pBitmap)
			{
				GdipCreateHBITMAPFromBitmap(pBitmap, &hbm, c.ToCOLORREF());
				GdipDisposeImage(pBitmap);
			}

			// GDI+の後処理。
			GdiplusShutdown(gdiplusToken);
		}

		FreeLibrary(hGdiPlus);
	}

	return hbm != nullptr;
}

// 無効状態のビットマップを作成する。
HBITMAP __fastcall XgCreateGrayedBitmap(HBITMAP hbm, COLORREF crMask) noexcept {
	HDC hdc = ::GetDC(nullptr);
	if (::GetDeviceCaps(hdc, BITSPIXEL) < 24) {
		HPALETTE hPal = static_cast<HPALETTE>(::GetCurrentObject(hdc, OBJ_PAL));
		const UINT index = ::GetNearestPaletteIndex(hPal, crMask);
		if (index != CLR_INVALID)
			crMask = PALETTEINDEX(index);
	}
	::ReleaseDC(nullptr, hdc);

	BITMAP bm;
	::GetObject(hbm, sizeof(bm), &bm);

	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = bm.bmWidth;
	bi.bmiHeader.biHeight = bm.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;

	RECT rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = bm.bmWidth;
	rc.bottom = bm.bmHeight;

	HDC hdc1 = ::CreateCompatibleDC(nullptr);
	HDC hdc2 = ::CreateCompatibleDC(nullptr);

	LPVOID pvBits;
	HBITMAP hbmNew = ::CreateDIBSection(
		nullptr, &bi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
	assert(hbmNew);
	if (hbmNew) {
		HGDIOBJ hbm1Old = ::SelectObject(hdc1, hbm);
		HGDIOBJ hbm2Old = ::SelectObject(hdc2, hbmNew);
		if (crMask == CLR_INVALID) {
			HBRUSH hbr = ::CreateSolidBrush(crMask);
			::FillRect(hdc2, &rc, hbr);
			::DeleteObject(hbr);
		}

		BYTE by;
		COLORREF cr;
		if (crMask == CLR_INVALID) {
			for (int y = 0; y < bm.bmHeight; ++y) {
				for (int x = 0; x < bm.bmWidth; ++x) {
					cr = ::GetPixel(hdc1, x, y);
					by = static_cast<BYTE>(
						95 + (
							GetRValue(cr) * 3 +
							GetGValue(cr) * 6 +
							GetBValue(cr)
						) / 20
					);
					::SetPixelV(hdc2, x, y, RGB(by, by, by));
				}
			}
		} else {
			for (int y = 0; y < bm.bmHeight; ++y) {
				for (int x = 0; x < bm.bmWidth; ++x) {
					cr = ::GetPixel(hdc1, x, y);
					if (cr != crMask)
					{
						by = static_cast<BYTE>(
							95 + (
								GetRValue(cr) * 3 +
								GetGValue(cr) * 6 +
								GetBValue(cr)
							) / 20
						);
						::SetPixelV(hdc2, x, y, RGB(by, by, by));
					}
				}
			}
		}
		::SelectObject(hdc1, hbm1Old);
		::SelectObject(hdc2, hbm2Old);
	}

	::DeleteDC(hdc1);
	::DeleteDC(hdc2);

	return hbmNew;
}
