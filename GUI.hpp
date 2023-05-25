#pragma once

#ifdef _MSC_VER
    #define XG_NOINLINE __declspec(noinline)
#else
    #define XG_NOINLINE __attribute__((noinline))
#endif

// ヒントウィンドウを作成する。
BOOL XgCreateHintsWnd(HWND hwnd);
// ヒントウィンドウを破棄する。
void XgDestroyHintsWnd(void);
// ヒントの内容をヒントウィンドウで開く。
bool XgOpenHintsByWindow(HWND /*hwnd*/);
// ヒントを表示する。
void XgShowHints(HWND hwnd);
// ヒントの内容をメモ帳で開く。
bool XgOpenHintsByNotepad(HWND /*hwnd*/, bool bShowAnswer);

// ヒントを表示するかどうか。
extern BOOL xg_bShowClues;

//////////////////////////////////////////////////////////////////////////////

// ポップアップメニューを読み込む。
HMENU XgLoadPopupMenu(HWND hwnd, INT nPos);

// ツールバーのUIを更新する。
void XgUpdateToolBarUI(HWND hwnd);

// 候補ウィンドウを破棄する。
void XgDestroyCandsWnd(void);

// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd, INT x, INT y);
// 描画イメージを更新する。
void __fastcall XgUpdateImage(HWND hwnd);

// UIフォントの論理オブジェクトを取得する。
LOGFONTW *XgGetUIFont(void);

// ローカルファイルを見つける。
BOOL __fastcall XgFindLocalFile(LPWSTR pszPath, UINT cchPath, LPCWSTR pszFileName);

// 計算時間測定用。
extern DWORDLONG xg_dwlTick0;    // 開始時間。
extern DWORDLONG xg_dwlTick1;    // 再計算時間。
extern DWORDLONG xg_dwlTick2;    // 終了時間。
extern DWORDLONG xg_dwlWait;     // 待ち時間。

// 再計算の回数。
extern LONG xg_nRetryCount;

// [二重マス単語の候補と配置]ダイアログの位置。
extern INT xg_nMarkingX;
extern INT xg_nMarkingY;

//////////////////////////////////////////////////////////////////////////////

// 入力パレットを表示するか？
extern bool xg_bShowInputPalette;

// 入力パレットの位置。
extern INT xg_nInputPaletteWndX;
extern INT xg_nInputPaletteWndY;

// 「入力パレット」縦置き？
extern bool xg_bTateOki;

// 入力パレット。
extern HWND xg_hwndInputPalette;

// タテ入力？
extern bool xg_bTateInput;

// 入力パレットを作成する。
BOOL XgCreateInputPalette(HWND hwndOwner);
// 入力パレットを作成する。
BOOL XgCreateInputPalette(HWND hwndOwner, XG_InputMode imode);
// 入力パレットを破棄する。
BOOL XgDestroyInputPalette(void);
// 入力モードを切り替える。
void __fastcall XgSetInputMode(HWND hwnd, XG_InputMode mode, BOOL bForce = FALSE);
// 文字が入力された。
void __fastcall XgOnChar(HWND hwnd, TCHAR ch, int cRepeat);
void __fastcall XgOnImeChar(HWND hwnd, WCHAR ch, LPARAM /*lKeyData*/);
// BackSpaceを実行する。
void __fastcall XgCharBack(HWND hwnd);
// 入力方向を切り替える。
void __fastcall XgInputDirection(HWND hwnd, INT nDirection);
// 文字送りを切り替える。
void __fastcall XgSetCharFeed(HWND hwnd, INT nMode);
// 改行する。
void __fastcall XgReturn(HWND hwnd);
// 二重マス切り替え。
void __fastcall XgToggleMark(HWND hwnd);
// キー入力のハンドラ。
void __fastcall XgOnChar(HWND hwnd, TCHAR ch, int cRepeat);
void __fastcall XgOnKey(HWND hwnd, UINT vk, bool fDown, int /*cRepeat*/, UINT /*flags*/);
void __fastcall XgOnImeChar(HWND hwnd, WCHAR ch, LPARAM /*lKeyData*/);
// その他のコマンド。
bool __fastcall XgOnCommandExtra(HWND hwnd, INT id);

//////////////////////////////////////////////////////////////////////////////

// クリップボードから貼り付け。
void XgPasteBoard(HWND hwnd, const std::wstring& str);
// クリップボードにクロスワードをコピー。
void XgCopyBoard(HWND hwnd);
// クリップボードにクロスワードを画像としてコピー。
void XgCopyBoardAsImage(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////
// スクロール。

extern HWND xg_hCanvasWnd;

// 水平スクロールの位置を取得する。
inline INT __fastcall XgGetHScrollPos(void)
{
    return ::GetScrollPos(xg_hCanvasWnd, SB_HORZ);
}

// 垂直スクロールの位置を取得する。
inline INT __fastcall XgGetVScrollPos(void)
{
    return ::GetScrollPos(xg_hCanvasWnd, SB_VERT);
}

// 水平スクロールの情報を取得する。
inline BOOL __fastcall XgGetHScrollInfo(LPSCROLLINFO psi)
{
    return ::GetScrollInfo(xg_hCanvasWnd, SB_HORZ, psi);
}

// 垂直スクロールの情報を取得する。
inline BOOL __fastcall XgGetVScrollInfo(LPSCROLLINFO psi)
{
    return ::GetScrollInfo(xg_hCanvasWnd, SB_VERT, psi);
}

// 水平スクロールの位置を設定する。
inline INT __fastcall XgSetHScrollPos(int nPos, BOOL bRedraw)
{
    return ::SetScrollPos(xg_hCanvasWnd, SB_HORZ, nPos, bRedraw);
}

// 垂直スクロールの位置を設定する。
inline INT __fastcall XgSetVScrollPos(int nPos, BOOL bRedraw)
{
    return ::SetScrollPos(xg_hCanvasWnd, SB_VERT, nPos, bRedraw);
}

// 水平スクロールの情報を設定する。
inline BOOL __fastcall XgSetHScrollInfo(LPSCROLLINFO psi, BOOL bRedraw)
{
    return ::SetScrollInfo(xg_hCanvasWnd, SB_HORZ, psi, bRedraw);
}

// 垂直スクロールの情報を設定する。
inline BOOL __fastcall XgSetVScrollInfo(LPSCROLLINFO psi, BOOL bRedraw)
{
    return ::SetScrollInfo(xg_hCanvasWnd, SB_VERT, psi, bRedraw);
}

// スクロール情報を設定する。
void __fastcall XgUpdateScrollInfo(HWND hwnd, int x, int y);
// キャレットが見えるように、必要ならばスクロールする。
void __fastcall XgEnsureCaretVisible(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////

// 現在の状態で好ましいと思われる単語の最大長を取得する。
INT __fastcall XgGetPreferredMaxLength(void);

// マス位置を取得する。
VOID XgGetCellPosition(RECT& rc, INT i1, INT j1, INT i2, INT j2, BOOL bScroll);
// マス位置を設定する。
VOID XgSetCellPosition(LONG& x, LONG& y, INT& i, INT& j, BOOL bEnd);

// マウスの中央ボタンの処理に使う変数。
extern BOOL xg_bMButtonDragging;
extern POINT xg_ptMButtonDragging;

// 本当のクライアント領域を計算する。
void __fastcall XgGetRealClientRect(HWND hwnd, LPRECT prcClient);
// ズームを実際のウィンドウに合わせる。
void __fastcall XgFitZoom(HWND hwnd);
// テーマが変更された。
void XgUpdateTheme(HWND hwnd);
// ルールが変更された。
void XgUpdateRules(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////
