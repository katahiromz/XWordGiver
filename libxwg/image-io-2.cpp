// image-io-2.cpp --- 画像ファイルの読み書き。
// Author: katahiromz
// License: MIT

#include "image-io-2.h"
#include <gdiplus.h>

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <cwchar>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// 指定MIMEタイプに対応するエンコーダのCLSIDを取得
static BOOL GetEncoderClsid(const WCHAR* mimeType, CLSID* pClsid) {
	UINT num = 0, size = 0;
	if (GetImageEncodersSize(&num, &size) != Ok || size == 0)
		return FALSE;

	std::vector<BYTE> buffer(size);
	auto* pInfo = reinterpret_cast<ImageCodecInfo*>(buffer.data());
	if (GetImageEncoders(num, size, pInfo) != Ok)
		return FALSE;

	for (UINT i = 0; i < num; ++i) {
		if (wcscmp(pInfo[i].MimeType, mimeType) == 0) {
			*pClsid = pInfo[i].Clsid;
			return TRUE;
		}
	}
	return FALSE;
}

// 拡張子からMIMEタイプを決定
static const WCHAR* MimeFromExtension(const MStringW& path) {
	size_t pos = path.find_last_of(L'.');
	if (pos == MStringW::npos)
		return nullptr;

	MStringW ext = path.substr(pos + 1);
	for (auto& c : ext) c = static_cast<WCHAR>(towlower(c));

	if (ext == L"png")				  return L"image/png";
	if (ext == L"jpg" || ext == L"jpeg") return L"image/jpeg";
	if (ext == L"bmp")				  return L"image/bmp";
	if (ext == L"gif")				  return L"image/gif";
	if (ext == L"tif" || ext == L"tiff") return L"image/tiff";
	return nullptr;
}

/// HBITMAP を画像ファイルとして保存する
/// @param hBitmap  保存するビットマップ(DDB/DIB どちらも可。他のDCに選択されていないこと)
/// @param filePath 保存先パス(拡張子で形式を判定)
/// @param jpegQuality JPEG品質 0-100(JPEG以外では無視)
/// @return 成功なら TRUE
/// @note 事前に GdiplusStartup() を呼んでおく必要があります
BOOL XgSavePictureToFile(const MStringW& filePath, HBITMAP hBitmap, ULONG jpegQuality) {
	if (!hBitmap)
		return FALSE;

	const WCHAR* mime = MimeFromExtension(filePath);
	if (!mime)
		return FALSE;

	CLSID clsid;
	if (!GetEncoderClsid(mime, &clsid))
		return FALSE;

	// HBITMAP -> Gdiplus::Bitmap (パレットはNULL=既定)
	Bitmap bmp(hBitmap, nullptr);
	if (bmp.GetLastStatus() != Ok)
		return FALSE;

	Status status;
	if (wcscmp(mime, L"image/jpeg") == 0) {
		if (jpegQuality > 100) jpegQuality = 100;
		EncoderParameters params;
		params.Count = 1;
		params.Parameter[0].Guid = EncoderQuality;
		params.Parameter[0].Type = EncoderParameterValueTypeLong;
		params.Parameter[0].NumberOfValues = 1;
		params.Parameter[0].Value = &jpegQuality;
		status = bmp.Save(filePath.c_str(), &clsid, &params);
	} else {
		status = bmp.Save(filePath.c_str(), &clsid, nullptr);
	}

	return status == Ok;
}
