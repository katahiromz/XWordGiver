// MCluesWnd.h --- [ヒント]ウィンドウ
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MScrollView.h"

#define XGWM_HIGHLIGHT1 (WM_APP + 200)

class MCluesWnd : public MWindowBase {
public:
	std::vector<MPlaceInfo>& m_vHorzInfo;
	std::vector<MPlaceInfo>& m_vVertInfo;
	MStringMapW& m_word2clue;
	LOGFONT& m_lfCluesLogFont;
	BOOL m_bTest = FALSE;

	// ヒントウィンドウのスクロールビュー。
	MScrollView			m_svHintsScrollView;
	// ヒントウィンドウのUIフォント。
	HFONT				m_hCluesFont = nullptr;

	// 縦のカギのコントロール群。
	HWND				m_hwndVertCaptionStatic = nullptr; // 所有権あり。
	std::vector<HWND>   m_ahwndVertStatics; // 所有権あり。
	std::vector<HWND>   m_ahwndVertEdits; // 所有権あり。

	// 横のカギのコントロール群。
	HWND				m_hwndHorzCaptionStatic = nullptr; // 所有権あり。
	std::vector<HWND>   m_ahwndHorzStatics; // 所有権あり。
	std::vector<HWND>   m_ahwndHorzEdits; // 所有権あり。

	// ヒントウィンドウの位置とサイズ。
	INT m_nWindowX = CW_USEDEFAULT;
    INT m_nWindowY = CW_USEDEFAULT;
	INT m_nWindowCX = 420;
    INT m_nWindowCY = 250;

	// ハイライト。
	HWND m_hwndHighlightVertEdit = nullptr; // m_ahwndVertEdits内部を指す。所有権はない。
	HWND m_hwndHighlightHorzEdit = nullptr; // m_ahwndHorzEdits内部を指す。所有権はない。

	// ジャンプ先番号。
	INT m_nNumber = 0;
	BOOL m_bVert = FALSE;
	HWND m_hwndTrackMenuOn = nullptr;

	// 配置の基準にするキャンバスウィンドウ。所有権はない。
	HWND m_hwndCanvas = nullptr;

	MCluesWnd(std::vector<MPlaceInfo>& vHorzInfo, std::vector<MPlaceInfo>& vVertInfo,
			  MStringMapW& word2clue, LOGFONTW& lfCluesLogFont)
		: m_vHorzInfo(vHorzInfo)
		, m_vVertInfo(vVertInfo)
		, m_word2clue(word2clue)
		, m_lfCluesLogFont(lfCluesLogFont)
	{
	}

	LPCTSTR GetWndClassNameDx() const override { return TEXT("MCluesWnd"); }

	BOOL CreateDx(HWND hwndOwner);

	// ハイライトを更新する。
	void SetHighlight(INT nHorz, INT nVert) noexcept;

	// ヒントが変更されたか？
	BOOL IsModified() noexcept;

	// ヒントデータを設定する。
	void SetHintsData() noexcept;

	// ヒントデータを更新する。
	BOOL UpdateHintData();

	struct MClueEditData {
		WNDPROC m_fnOldWndProc;
		bool	m_fVert;
		MCluesWnd* m_pParent;
	};

	LRESULT CALLBACK WindowProcDx(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

protected:
	BOOL CreateControls(HWND hwnd);
	void DestroyControls();
	void DestroyCtrl(HWND& hwndCtrl);
	void DestroyArray(std::vector<HWND>& ahwndCtrls);
    void AdjustPosInfo();
	void LayoutAcross(HDC hdc, INT& y, MRect& rcClient, MSize& size1, MSize& size2);
	void LayoutDown(HDC hdc, INT& y, MRect& rcClient, MSize& size1, MSize& size2);

	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT /*lpCreateStruct*/);
	void OnDestroy(HWND hwnd);
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept;
	void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) noexcept;
	void OnZoom(HWND hwnd, BOOL bZoomIn);
	void __fastcall OnMouseWheel(HWND hwnd, int xPos, int yPos, int zDelta, UINT fwKeys);
	void OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos) noexcept;
	HBRUSH OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type) noexcept;
	void OnSize(HWND hwnd, UINT /*state*/, int /*cx*/, int /*cy*/);

	// WM_VSCROLL
	void OnVScroll(HWND /*hwnd*/, HWND /*hwndCtl*/, UINT code, int pos) noexcept {
		m_svHintsScrollView.Scroll(SB_VERT, code, pos);
	}

	// WM_GETMINMAXINFO
	void OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) noexcept {
		lpMinMaxInfo->ptMinTrackSize.x = 256;
		lpMinMaxInfo->ptMinTrackSize.y = 128;
	}

	static LRESULT CALLBACK XgHintEditWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	// 単体テスト。
	static void UnitTest();
};
