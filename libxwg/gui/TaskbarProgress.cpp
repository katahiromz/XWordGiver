// TaskbarProgress.cpp --- CLSID_TaskbarListを操作するためのラッパークラス
// Author: katahiromz
// License: MIT
#include <windows.h>
#include "xwg.h"
#include "TaskbarProgress.h"
//#pragma comment(lib, "ole32.lib")  // CoCreateInstance

TaskbarProgress::TaskbarProgress(HWND hwnd)
	: m_hWnd(hwnd)
	, m_pTaskbarList(nullptr)
	, m_hrCoInit(CoInitialize(nullptr))
	, m_hr(E_FAIL)
{
	m_hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
							IID_PPV_ARGS(&m_pTaskbarList));
	if (FAILED(m_hr))
		return;

	m_hr = m_pTaskbarList->HrInit();
}

TaskbarProgress::~TaskbarProgress() {
	if (m_pTaskbarList)
	{
		m_pTaskbarList->Release();
		m_pTaskbarList = nullptr;
	}

	if (SUCCEEDED(m_hrCoInit))
		CoUninitialize();
}

void TaskbarProgress::Set(INT percent) {
	if (!m_pTaskbarList || FAILED(m_hr))
		return;

	m_pTaskbarList->SetProgressState(m_hWnd, ((percent < 0) ? TBPF_INDETERMINATE : TBPF_NORMAL));

	if (percent >= 0)
		m_pTaskbarList->SetProgressValue(m_hWnd, percent, 100);
}

void TaskbarProgress::Error() {
	if (!m_pTaskbarList || FAILED(m_hr))
		return;

	m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
	m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
}

void TaskbarProgress::Clear() {
	if (!m_pTaskbarList || FAILED(m_hr))
		return;

	m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
	m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
}

// メニュー バーの高さを取得する。
INT TaskbarProgress::GetMenuHeight() {
	HMENU hMenu = ::GetMenu(m_hWnd);
	if (!hMenu)
		return 0;
	INT cItems = ::GetMenuItemCount(hMenu);
	RECT rc;
	::SetRectEmpty(&rc);
	for (INT iItem = 0; iItem < cItems; ++iItem)
	{
		RECT rcItem;
		GetMenuItemRect(m_hWnd, hMenu, iItem, &rcItem);
		::UnionRect(&rc, &rc, &rcItem);
	}
	return rc.bottom - rc.top;
}

void TaskbarProgress::GetThumbnailSize(PRECT prc) {
	// メイン ウィンドウ側が、クライアント座標でキャンバスの範囲を返す。
	RECT rc = {};
	SIZE siz = {};
	SendMessageW(xg_hMainWnd, XGWM_GETTHUMBNAILEXTENT, (WPARAM)&siz, (LPARAM)&rc);

	// NOTE: ドキュメントでは「クライアント領域の左上が基準」だが、実際にはメニュー バーが
	// ある場合、その高さ分だけ矩形が上にずれて解釈される(実機で確認済み)。そこで補正する。
	if (!IsRectEmpty(&rc))
		OffsetRect(&rc, 0, GetMenuHeight());

	*prc = rc;
}

void TaskbarProgress::SetThumbnail() {
	if (!m_pTaskbarList || FAILED(m_hr))
		return;

	RECT rc;
	GetThumbnailSize(&rc);
	m_pTaskbarList->SetThumbnailClip(m_hWnd, IsRectEmpty(&rc) ? nullptr : &rc);
}
