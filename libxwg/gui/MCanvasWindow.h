// MCanvasWindow.h --- キャンバスウィンドウ。
// Author: katahiromz
// License: MIT

#pragma once

#include "MWindowBase.h"
#include "MPointSizeRect.h"
#include "board-image.h"
#include "marks.h"
#include "gui-utils.h"
#include <functional>

#define XGWM_HIGHLIGHT2 (WM_APP + 1000)

// マス位置からイメージ座標を取得する。
inline void __fastcall XgCellToImage(MBoardDisplay& display, RECT& rc) {
	const INT nCellSize = (display.m_nCellSize * display.m_nZoomRate / 100);

	auto& caret_pos = display.m_caret_pos;
	auto margin = display.m_nMargin;
	rc = {
		margin + caret_pos.m_j * nCellSize,
		margin + caret_pos.m_i * nCellSize,
		margin + (caret_pos.m_j + 1) * nCellSize,
		margin + (caret_pos.m_i + 1) * nCellSize
	};
}

// マス位置からキャンバス座標を取得する。
inline void __fastcall XgCellToCanvas(HWND hwnd, MBoardDisplay& display, RECT& rc) {
	XgCellToImage(display, rc);
	INT x = ::GetScrollPos(hwnd, SB_HORZ);
	INT y = ::GetScrollPos(hwnd, SB_VERT);
	::OffsetRect(&rc, -x, -y);
}

// キャンバスウィンドウ。盤面を描画し、スクロール・ズーム・マウス操作を受け持つ。
class MCanvasWindow : public MWindowBase
{
public:
	MBoardDisplay& m_display;
	MInputMode& m_imode;
	HWND m_hwndParent;
	POINT m_ptMButtonDragging = { -1, -1 };
	BOOL m_bMButtonDragging = FALSE;
	HBITMAP m_hbmImage = nullptr;
	BOOL m_bUpdatingScroll = FALSE; // UpdateScrollInfo の再入防止。
	std::function<void()> m_fnLayoutChanged; // スクロール情報(サイズやズーム)が更新されたときの通知。

	MCanvasWindow(MBoardDisplay& display, MInputMode& imode) noexcept
		: m_display(display)
		, m_imode(imode)
		, m_hwndParent(nullptr)
	{
	}

	~MCanvasWindow() {
		ReleaseImage();
		m_display.DeleteFonts();
	}

	//////////////////////////////////////////////////////////////////////////
	// ウィンドウ クラス

	LPCTSTR GetWndClassNameDx() const override {
		return TEXT("XG_CanvasWindow");
	}

	void ModifyWndClassDx(WNDCLASSEX& wcx) override {
		wcx.hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
	}

	//////////////////////////////////////////////////////////////////////////
	// イメージと再描画

	// イメージとフォントを破棄して、再描画を要求する。
	void InvalidateImage() {
		ReleaseImage();
		m_display.DeleteFonts();
		// NOTE: m_hwndがNULLだと、InvalidateRectは全ウィンドウを無効化してしまう。
		if (m_hwnd && ::IsWindow(m_hwnd))
			::InvalidateRect(m_hwnd, nullptr, TRUE);
	}

	// キャレット位置のマスを無効化する。
	void InvalidateCell() {
		RECT rcCell;
		XgCellToCanvas(m_hwnd, m_display, rcCell);
		::InvalidateRect(m_hwnd, &rcCell, FALSE);
	}

	//////////////////////////////////////////////////////////////////////////
	// スクロール

	// 表示用のスクロール位置(display側)を、実際のスクロールバーの位置に同期する。
	// XgDrawCaret や XgGetCellPosition/XgSetCellPosition は display 側の値を使う。
	void SyncScrollPos() noexcept {
		m_display.m_xScrollPos = ::GetScrollPos(m_hwnd, SB_HORZ);
		m_display.m_yScrollPos = ::GetScrollPos(m_hwnd, SB_VERT);
	}

	// スクロール位置を範囲内に収めて設定し、実際にスクロールした量(旧位置-新位置)を返す。
	INT SetScrollPosClamped(INT nBar, INT nNewPos) noexcept {
		SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS };
		::GetScrollInfo(m_hwnd, nBar, &si);
		const INT nOldPos = si.nPos;

		// スクロール可能な最大位置は nMax - (nPage - 1)。
		const INT nPage = (si.nPage > 0) ? (INT)si.nPage : 1;
		INT nMaxPos = si.nMax - (nPage - 1);
		if (nMaxPos < si.nMin)
			nMaxPos = si.nMin;
		if (nNewPos > nMaxPos)
			nNewPos = nMaxPos;
		if (nNewPos < si.nMin)
			nNewPos = si.nMin;
		if (nNewPos == nOldPos)
			return 0;

		si.fMask = SIF_POS;
		si.nPos = nNewPos;
		::SetScrollInfo(m_hwnd, nBar, &si, TRUE);

		// 実際の位置を再取得する。
		si.fMask = SIF_POS;
		::GetScrollInfo(m_hwnd, nBar, &si);
		return nOldPos - si.nPos;
	}

	// 指定した位置(イメージ座標)へスクロールして、画面を更新する。
	void ScrollToPos(INT x, INT y) noexcept {
		const INT dx = SetScrollPosClamped(SB_HORZ, x);
		const INT dy = SetScrollPosClamped(SB_VERT, y);
		SyncScrollPos();
		if (dx == 0 && dy == 0)
			return;

		// 高速スクロールする。
		::ScrollWindowEx(m_hwnd, dx, dy, nullptr, nullptr, nullptr, nullptr,
		                 SW_INVALIDATE | SW_ERASE);

		// キャレット位置更新。
		UpdateCaretPos();
	}

	// キャレット位置のマスが全体表示されるようにスクロールする。
	// スクロール対象は常にこのキャンバス(m_hwnd)である。
	void EnsureCaretVisible() noexcept {
		// キャレット位置のマスのイメージ座標(スクロール前の座標)。
		RECT rcCell;
		XgCellToImage(m_display, rcCell);

		// 表示領域のサイズ。
		MRect rcClient;
		GetClientRect(m_hwnd, &rcClient);
		const INT cx = rcClient.Width(), cy = rcClient.Height();

		// 現在のスクロール位置。
		const INT x = ::GetScrollPos(m_hwnd, SB_HORZ);
		const INT y = ::GetScrollPos(m_hwnd, SB_VERT);
		INT nx = x, ny = y;

		// 右/下にはみ出していれば、マスの右端/下端が見える位置へ。
		// その後、左/上にはみ出していれば、マスの左端/上端を優先する。
		if (rcCell.right > x + cx)
			nx = rcCell.right - cx;
		if (rcCell.left < nx)
			nx = rcCell.left;
		if (rcCell.bottom > y + cy)
			ny = rcCell.bottom - cy;
		if (rcCell.top < ny)
			ny = rcCell.top;

		// 範囲内に収めてスクロールする(画面更新・ボックス移動も行う)。
		ScrollToPos(nx, ny);
	}

	// スクロール情報を設定する。
	void UpdateScrollInfo() noexcept {
		UpdateScrollInfo(m_display.m_xScrollPos, m_display.m_yScrollPos);
	}

	// スクロール情報を設定する。
	void UpdateScrollInfo(INT x, INT y) noexcept {
		if (m_bUpdatingScroll)
			return;
		m_bUpdatingScroll = TRUE;

		// 描画サイズを取得。ズームあり(実際に描画されるサイズ)。
		SIZE siz = XgGetBoardExtent(m_display, DRAW_MODE_SCREEN);

		// クライアント領域を計算する。
		MRect rcClient;
		GetClientRect(m_hwnd, &rcClient);

		// 範囲は [nMin, nMax](両端を含む)なので、サイズ-1 を nMax とする。
		SCROLLINFO si = { sizeof(si) };
		si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS | SIF_DISABLENOSCROLL;

		// 横スクロール情報を設定する。
		si.nMin = 0;
		si.nMax = (siz.cx > 0) ? (siz.cx - 1) : 0;
		si.nPage = (rcClient.Width() > 0) ? (UINT)rcClient.Width() : 0;
		si.nPos = x;
		::SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);

		// 縦スクロール情報を設定する。
		si.nMin = 0;
		si.nMax = (siz.cy > 0) ? (siz.cy - 1) : 0;
		si.nPage = (rcClient.Height() > 0) ? (UINT)rcClient.Height() : 0;
		si.nPos = y;
		::SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);

		m_bUpdatingScroll = FALSE;
		SyncScrollPos();

		// 位置が補正された可能性があるので再描画する。
		::InvalidateRect(m_hwnd, nullptr, FALSE);

		// 表示サイズが変わったことを通知する(タスクバーのサムネイル更新など)。
		if (m_fnLayoutChanged)
			m_fnLayoutChanged();
	}

	//////////////////////////////////////////////////////////////////////////
	// キャレット(IMEの未確定文字列の表示位置)

	// キャレット位置を更新する(IMEの未確定文字列の表示位置を設定する)。
	void __fastcall UpdateCaretPos(void) {
		// IMEの入力コンテキストはメインウィンドウ側のもの。その座標系で位置を指定する。
		// (hWndTo が NULL だとスクリーン座標になってしまうので必ず確認する)
		HWND hwndTarget = xg_hMainWnd ? xg_hMainWnd : m_hwndParent;
		if (!m_hwnd || !hwndTarget)
			return;

		// キャレット位置を修正。
		m_display.FixCaretPos();

		// キャレット位置のマスの、キャンバスのクライアント座標。
		// XgCellToImage はスクロール前のイメージ座標なので、スクロール位置を引く。
		MRect rcCell;
		XgCellToImage(m_display, rcCell);
		POINT pt = {
			rcCell.left - ::GetScrollPos(m_hwnd, SB_HORZ),
			rcCell.top - ::GetScrollPos(m_hwnd, SB_VERT)
		};

		LOGFONTW lf = {};
		m_display.m_nZoomedCellSize = (m_display.m_nCellSize * m_display.m_nZoomRate / 100);
		::GetObjectW(XgGetNormalFont(m_display), sizeof(lf), &lf);

		// キャンバスのクライアント座標をメインウィンドウのクライアント座標へ変換する。
		pt.y += 4;
		::MapWindowPoints(m_hwnd, hwndTarget, &pt, 1);

		COMPOSITIONFORM CompForm = { CFS_POINT };
		CompForm.ptCurrentPos = pt; // 未確定文字列の表示位置。

		// IMEの制御。
		HIMC hIMC = ::ImmGetContext(hwndTarget);
		if (hIMC) {
			::ImmSetCompositionWindow(hIMC, &CompForm); // 未確定文字列の表示位置を設定。
			::ImmSetCompositionFont(hIMC, &lf); // 未確定文字列のフォントを設定。
			::ImmReleaseContext(hwndTarget, hIMC);
		}

		// ヒントウィンドウのハイライトを設定する。
		PostMessageW(xg_hMainWnd, XGWM_HIGHLIGHT2, 0, 0);
	}

	//////////////////////////////////////////////////////////////////////////
	// ズーム

	INT GetZoom() const noexcept { return m_display.m_nZoomRate; }

	// ズーム倍率を変更する。表示の中央が動かないようにスクロール位置を調整する。
	void SetZoom(INT nZoomRate) {
		MRect rcClient;
		GetClientRect(m_hwnd, &rcClient);
		const INT w = rcClient.Width(), h = rcClient.Height();

		// ズーム前のサイズと、スクロール位置。
		const SIZE sizOld = XgGetBoardExtent(m_display, DRAW_MODE_SCREEN);
		INT x = ::GetScrollPos(m_hwnd, SB_HORZ), y = ::GetScrollPos(m_hwnd, SB_VERT);

		m_display.SetZoom(nZoomRate);

		// ズーム後も同じ場所が中央に来るようにスクロール位置を調整する。
		const SIZE sizNew = XgGetBoardExtent(m_display, DRAW_MODE_SCREEN);
		if (sizOld.cx > 0)
			x = (INT)((LONGLONG)(x + w / 2) * sizNew.cx / sizOld.cx) - w / 2;
		if (sizOld.cy > 0)
			y = (INT)((LONGLONG)(y + h / 2) * sizNew.cy / sizOld.cy) - h / 2;
		if (x < 0) x = 0;
		if (y < 0) y = 0;

		UpdateScrollInfo(x, y);

		// フォントを作り直してから、IMEの位置とフォントを更新する。
		InvalidateImage();
		UpdateCaretPos();
	}

	// ズームイン/ズームアウト。
	void ZoomInOut(BOOL bOut) {
		INT nZoomRate = m_display.m_nZoomRate;
		if (bOut) {
			if (nZoomRate > 200) {
				nZoomRate -= 30;
			} else if (nZoomRate > 100) {
				nZoomRate -= 15;
			} else if (nZoomRate > 50) {
				nZoomRate -= 7;
			} else if (nZoomRate > 11) {
				nZoomRate -= 3;
			} else {
				return;
			}
		} else {
			if (nZoomRate < 50) {
				nZoomRate += 7;
			} else if (nZoomRate < 100) {
				nZoomRate += 15;
			} else if (nZoomRate < 200) {
				nZoomRate += 30;
			} else {
				return;
			}
		}

		SetZoom(nZoomRate);
	}

	// ズームを全体に合わせる。
	void FitZoom() {
		// クライアント領域のサイズを取得する。
		RECT rc;
		GetClientRect(m_hwnd, &rc);
		const SIZE sizClient = { rc.right - rc.left, rc.bottom - rc.top };

		// 描画サイズを取得。ズームなし。
		const SIZE siz = XgGetBoardExtent(m_display, DRAW_MODE_PRINT);
		if (siz.cx <= 0 || siz.cy <= 0)
			return;

		// サイズをクライアント領域にフィットさせる倍率を求める。
		INT nZoomRate;
		if (sizClient.cx * siz.cy > siz.cx * sizClient.cy) {
			nZoomRate = sizClient.cy * 100 / siz.cy;
		} else {
			nZoomRate = sizClient.cx * 100 / siz.cx;
		}

		SetZoom(nZoomRate);
	}

	//////////////////////////////////////////////////////////////////////////
	// コンテキストメニュー

	// コンテキストメニューを表示する。ptはキャンバスのクライアント座標。
	void ShowContextMenu(HWND hwnd, POINT pt) {
		HMENU hMenu = XgLoadPopupMenu(hwnd, 0);
		if (!hMenu)
			return;
		HMENU hSubMenu = GetSubMenu(hMenu, 0);
		if (!hSubMenu) {
			::DestroyMenu(hMenu);
			return;
		}

		// 座標が(-1, -1)ならキーボード操作。キャレット位置のマスの中央に出す。
		if (pt.x == -1 && pt.y == -1) {
			MRect rcCell;
			XgCellToImage(m_display, rcCell);
			pt = rcCell.CenterPoint();
			pt.x -= m_display.m_xScrollPos;
			pt.y -= m_display.m_yScrollPos;
		}
		// スクリーン座標へ変換する。
		::ClientToScreen(hwnd, &pt);

		// 入力モードに合わない入力パレットのメニュー項目を削除する。
		// 位置 5:カナ、6:英字、7:ロシア、8:数字、9:ギリシャ。
		// 位置がずれないよう、大きいインデックスから削除する。
		INT iKeep = -1; // 残す位置。-1なら全部削除。
		BOOL bKeepAll = FALSE;
		switch (m_imode) {
		case xg_im_ABC:    iKeep = 6; break;
		case xg_im_KANA:   iKeep = 5; break;
		case xg_im_KANJI:  break;
		case xg_im_RUSSIA: iKeep = 7; break;
		case xg_im_GREEK:  iKeep = 9; break;
		case xg_im_DIGITS: iKeep = 8; break;
		case xg_im_ANY:
		default:
			bKeepAll = TRUE;
			break;
		}
		if (!bKeepAll) {
			for (INT i = 9; i >= 5; --i) {
				if (i != iKeep)
					::DeleteMenu(hSubMenu, i, MF_BYPOSITION);
			}
		}

		if (m_display.m_bSolved) { // 解があり、ロックされている場合。
			if (m_display.m_bShowAnswer) { // かつ、解が表示されている場合。
				// 白マス・黒マスのメニュー項目を無効化する。
				::EnableMenuItem(hSubMenu, ID_BLOCKNOFEED, MF_GRAYED);
				::EnableMenuItem(hSubMenu, ID_SPACENOFEED, MF_GRAYED);
			}
			// 行・列関係のメニュー項目を無効化する。
			::EnableMenuItem(hSubMenu, ID_INSERT_ROW_ABOVE, MF_GRAYED);
			::EnableMenuItem(hSubMenu, ID_INSERT_ROW_BELOW, MF_GRAYED);
			::EnableMenuItem(hSubMenu, ID_LEFT_INSERT_COLUMN, MF_GRAYED);
			::EnableMenuItem(hSubMenu, ID_RIGHT_INSERT_COLUMN, MF_GRAYED);
			::EnableMenuItem(hSubMenu, ID_DELETE_ROW, MF_GRAYED);
			::EnableMenuItem(hSubMenu, ID_DELETE_COLUMN, MF_GRAYED);
		}

		// 右クリックメニューを表示する。コマンドは親ウィンドウへ送られる。
		::SetForegroundWindow(m_hwndParent);
		::TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x, pt.y, 0,
		                 m_hwndParent, nullptr);
		::PostMessageW(m_hwndParent, WM_NULL, 0, 0);

		::DestroyMenu(hMenu);
	}

	//////////////////////////////////////////////////////////////////////////
	// ウィンドウ プロシージャ

	LRESULT CALLBACK
	WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
			HANDLE_MSG(hwnd, WM_SIZE, OnSize);
			HANDLE_MSG(hwnd, WM_MOVE, OnMove);
			HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
			HANDLE_MSG(hwnd, WM_LBUTTONDBLCLK, OnLButtonDown);
			HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
			HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
			HANDLE_MSG(hwnd, WM_MBUTTONDOWN, OnMButtonDown);
			HANDLE_MSG(hwnd, WM_MBUTTONDBLCLK, OnMButtonDown);
			HANDLE_MSG(hwnd, WM_MBUTTONUP, OnMButtonUp);
			HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
			HANDLE_MSG(hwnd, WM_RBUTTONDBLCLK, OnRButtonDown);
			HANDLE_MSG(hwnd, WM_MOUSEWHEEL, OnMouseWheel);
			HANDLE_MSG(hwnd, WM_HSCROLL, OnHScroll);
			HANDLE_MSG(hwnd, WM_VSCROLL, OnVScroll);
			HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
			HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
		default:
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	}

protected:
	// 1行スクロールの量(ピクセル)。
	static constexpr INT LINE_SCROLL = 25;

	// イメージ(ビットマップ)を破棄する。
	void ReleaseImage() noexcept {
		if (m_hbmImage) {
			::DeleteObject(m_hbmImage);
			m_hbmImage = nullptr;
		}
	}

	// クライアント座標(x, y)にあるマスを取得する。盤面の外ならFALSE。
	BOOL HitTestCell(HWND hwnd, INT x, INT y, INT& j, INT& i) {
		const INT nCellSize = m_display.m_nCellSize * m_display.m_nZoomRate / 100;
		if (nCellSize <= 0)
			return FALSE;

		// イメージ座標(スクロール位置とマージンを考慮)。
		const INT px = x + ::GetScrollPos(hwnd, SB_HORZ) - m_display.m_nMargin;
		const INT py = y + ::GetScrollPos(hwnd, SB_VERT) - m_display.m_nMargin;
		if (px < 0 || py < 0)
			return FALSE;

		auto& board = XgGetDisplayBoard(m_display);
		j = px / nCellSize;
		i = py / nCellSize;
		return (j < board.Columns() && i < board.Rows());
	}

	// キャレットをマスへ移動して、見える位置までスクロールする。
	void MoveCaretTo(INT j, INT i) {
		m_display.m_caret_pos = { j, i };
		EnsureCaretVisible();
		UpdateCaretPos();
	}

	// ドラッグ用の手のカーソルを設定する。
	static void SetHandCursor() noexcept {
		::SetCursor(::LoadCursor(xg_hInstance, MAKEINTRESOURCEW(IDC_MYHAND)));
	}

	//////////////////////////////////////////////////////////////////////////
	// メッセージ ハンドラ

	// WM_CREATE
	BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) noexcept {
		m_hwndParent = GetParent(hwnd);
		return TRUE;
	}

	// WM_DESTROY
	void OnDestroy(HWND hwnd) noexcept {
		ReleaseImage();
	}

	// WM_SIZE
	// クライアント領域が変わったので、スクロール情報(ページサイズ)を更新する。
	void OnSize(HWND hwnd, UINT state, INT cx, INT cy) noexcept {
		if (!xg_hMainWnd || state == SIZE_MINIMIZED)
			return;
		UpdateScrollInfo(::GetScrollPos(hwnd, SB_HORZ), ::GetScrollPos(hwnd, SB_VERT));
		UpdateCaretPos();
	}

	// WM_MOVE
	// キャンバスの位置が変わると、メインウィンドウ基準のキャレット位置も変わる。
	void OnMove(HWND hwnd, INT x, INT y) noexcept {
		if (!xg_hMainWnd)
			return;
		UpdateCaretPos();
	}

	// WM_LBUTTONDOWN / WM_LBUTTONDBLCLK
	// シングルクリックは無視する(キャレット移動は OnLButtonUp で行う)。
	// ダブルクリックでマークをトグルする。
	void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags) {
		if (!fDoubleClick)
			return;

		INT j, i;
		if (!HitTestCell(hwnd, x, y, j, i))
			return;

		// キャレットを移動する。
		MoveCaretTo(j, i);

		// あとはメインウィンドウにまかせる。
		SendMessageW(xg_hMainWnd, WM_COMMAND, ID_TOGGLEMARK, 0);
	}

	// WM_LBUTTONUP
	void OnLButtonUp(HWND hwnd, INT x, INT y, UINT keyFlags) {
		INT j, i;
		if (!HitTestCell(hwnd, x, y, j, i))
			return;

		// キャレットを移動して、イメージを更新する。
		MoveCaretTo(j, i);
		InvalidateImage();
	}

	// WM_MBUTTONDOWN / WM_MBUTTONDBLCLK
	// マウスの中央ボタンが押された。
	void OnMButtonDown(HWND hwnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags) noexcept {
		if (!m_bMButtonDragging) {
			m_bMButtonDragging = TRUE;
			m_ptMButtonDragging = { x, y };
			::SetCapture(hwnd);
			SetHandCursor();
		}
	}

	// WM_MOUSEMOVE
	// ウィンドウ上でマウスが動いた。
	void OnMouseMove(HWND hwnd, INT x, INT y, UINT keyFlags) {
		if (!m_bMButtonDragging)
			return;
		if (::GetCapture() == hwnd) {
			OnMouseScroll(hwnd, x, y);
		} else {
			m_bMButtonDragging = FALSE; // キャプチャーを失っていた。
		}
	}

	// WM_MBUTTONUP
	// マウスの中央ボタンが離された。
	void OnMButtonUp(HWND hwnd, INT x, INT y, UINT flags) {
		if (m_bMButtonDragging && ::GetCapture() == hwnd) {
			OnMouseScroll(hwnd, x, y);
			m_bMButtonDragging = FALSE;
			::ReleaseCapture();
			::SetCursor(::LoadCursor(nullptr, IDC_ARROW));
		}
	}

	// WM_RBUTTONDOWN / WM_RBUTTONDBLCLK
	// 右クリックされた。キャレットを移動してからメニューを表示する。
	void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, INT x, INT y, UINT keyFlags) {
		OnLButtonUp(hwnd, x, y, keyFlags);
		POINT pt = { x, y };
		ShowContextMenu(hwnd, pt);
	}

	// WM_MOUSEWHEEL
	// マウスホイールが回転した。
	void OnMouseWheel(HWND hwnd, INT x, INT y, INT zDelta, UINT fwKeys) noexcept {
		if (zDelta == 0)
			return;

		if (fwKeys & MK_CONTROL) {
			// Ctrlが押されていればズーム。
			::SendMessageW(hwnd, WM_COMMAND, (zDelta < 0) ? ID_ZOOMOUT : ID_ZOOMIN, 0);
		} else if (fwKeys & MK_SHIFT) {
			// Shiftが押されていれば横スクロール。
			::SendMessageW(hwnd, WM_HSCROLL,
				MAKEWPARAM((zDelta < 0) ? SB_LINERIGHT : SB_LINELEFT, 0), 0);
		} else {
			// さもなければ縦スクロール。
			::SendMessageW(hwnd, WM_VSCROLL,
				MAKEWPARAM((zDelta < 0) ? SB_LINEDOWN : SB_LINEUP, 0), 0);
		}
	}

	// WM_ERASEBKGND
	BOOL OnEraseBkgnd(HWND hwnd, HDC hdc) noexcept {
		return TRUE; // 描画しない(WM_PAINTで背景も描く)。
	}

	// WM_PAINT
	// ウィンドウを描画する。
	void OnPaint(HWND hwnd) {
		// 必ず BeginPaint/EndPaint を呼ぶ(呼ばないと WM_PAINT が無限に届く)。
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hwnd, &ps);
		assert(hdc);
		if (!hdc)
			return;

		if (xg_hMainWnd) {
			// クロスワードの描画サイズを取得する。ズームあり。
			SIZE siz = XgGetBoardExtent(m_display, DRAW_MODE_SCREEN);

			// クライアント領域を得る。
			MRect rcClient;
			GetClientRect(hwnd, &rcClient);

			// スクロール位置を取得する。
			const INT x = ::GetScrollPos(hwnd, SB_HORZ), y = ::GetScrollPos(hwnd, SB_VERT);

			// 盤面の外側の背景を描画する。
			const INT iSaveDC = ::SaveDC(hdc);
			::ExcludeClipRect(hdc, rcClient.left - x, rcClient.top - y,
			                  rcClient.left - x + siz.cx,
			                  rcClient.top - y + siz.cy);
			::FillRect(hdc, &rcClient, ::GetSysColorBrush(COLOR_3DFACE));
			::RestoreDC(hdc, iSaveDC);

			// イメージがない場合は、イメージを取得する。
			if (m_hbmImage == nullptr)
				m_hbmImage = XgBitmapFromBoard(m_display, &siz, DRAW_MODE_SCREEN);

			// ビットマップ イメージをウィンドウに転送する。
			if (m_hbmImage) {
				if (HDC hdcMem = ::CreateCompatibleDC(hdc)) {
					HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_hbmImage);
					::BitBlt(hdc, rcClient.left - x, rcClient.top - y, siz.cx, siz.cy,
					         hdcMem, 0, 0, SRCCOPY);
					::SelectObject(hdcMem, hbmOld);
					::DeleteDC(hdcMem);
				}
			}

			// キャレットを描画する。display側のスクロール位置を最新にしてから描く。
			SyncScrollPos();
			XgDrawCaret(m_display, hdc, TRUE);
		}

		// 描画を終了する。
		::EndPaint(hwnd, &ps);
	}

	// WM_HSCROLL / WM_VSCROLL 共通のスクロール処理。
	void OnScrollCode(HWND hwnd, INT nBar, UINT code) {
		// SB_THUMBTRACK の位置は 16bit に切り詰められるので nTrackPos を使う。
		SCROLLINFO si = { sizeof(si), SIF_ALL };
		::GetScrollInfo(hwnd, nBar, &si);

		const INT nPage = (si.nPage > 0) ? (INT)si.nPage : 1;
		INT nMaxPos = si.nMax - (nPage - 1);
		if (nMaxPos < si.nMin)
			nMaxPos = si.nMin;

		// コードに応じて位置情報を設定する。
		// (SB_LEFT==SB_TOP, SB_LINELEFT==SB_LINEUP などは同じ値)
		INT nPos = si.nPos;
		switch (code) {
		case SB_LEFT:
			nPos = si.nMin;
			break;
		case SB_RIGHT:
			nPos = nMaxPos;
			break;
		case SB_LINELEFT:
			nPos -= LINE_SCROLL;
			break;
		case SB_LINERIGHT:
			nPos += LINE_SCROLL;
			break;
		case SB_PAGELEFT:
			nPos -= nPage;
			break;
		case SB_PAGERIGHT:
			nPos += nPage;
			break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			nPos = si.nTrackPos;
			break;
		default:
			return; // SB_ENDSCROLL など。
		}

		// 範囲内に収めてスクロールする。
		if (nBar == SB_HORZ)
			ScrollToPos(nPos, ::GetScrollPos(hwnd, SB_VERT));
		else
			ScrollToPos(::GetScrollPos(hwnd, SB_HORZ), nPos);
	}

	// WM_HSCROLL
	void OnHScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, INT /*pos*/) {
		OnScrollCode(hwnd, SB_HORZ, code);
	}

	// WM_VSCROLL
	void OnVScroll(HWND hwnd, HWND /*hwndCtl*/, UINT code, INT /*pos*/) {
		OnScrollCode(hwnd, SB_VERT, code);
	}

	// WM_COMMAND
	// コマンドは親ウィンドウ(メインウィンドウ)へ転送する。
	void OnCommand(HWND hwnd, INT id, HWND hwndCtl, UINT codeNotify) noexcept {
		// NOTE: 親がNULLだとスレッド メッセージになり、誰にも届かない。
		if (!m_hwndParent)
			return;
		FORWARD_WM_COMMAND(m_hwndParent, id, hwndCtl, codeNotify, PostMessageW);
	}

	// マウスの中央ボタンでドラッグされた。
	void OnMouseScroll(HWND hwnd, INT x, INT y) {
		const INT dx = m_ptMButtonDragging.x - x;
		const INT dy = m_ptMButtonDragging.y - y;

		// 範囲内に収めてスクロールする(画面更新・ボックス移動も行う)。
		ScrollToPos(::GetScrollPos(hwnd, SB_HORZ) + dx, ::GetScrollPos(hwnd, SB_VERT) + dy);

		m_ptMButtonDragging.x = x;
		m_ptMButtonDragging.y = y;

		SetHandCursor();
	}
};
