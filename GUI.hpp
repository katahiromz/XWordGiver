#pragma once

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

// ヒント文字列を取得する。
// hint_type 0: タテ。
// hint_type 1: ヨコ。
// hint_type 2: タテとヨコ。
// hint_type 3: HTMLのタテ。
// hint_type 4: HTMLのヨコ。
// hint_type 5: HTMLのタテとヨコ。
void __fastcall XgGetHintsStr(const XG_Board& board, std::wstring& str, int hint_type,
                              bool bShowAnswer = true);

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

//////////////////////////////////////////////////////////////////////////////

// 入力パレットを作成する。
BOOL XgCreateInputPalette(HWND hwndOwner);
// 入力パレットを破棄する。
BOOL XgDestroyInputPalette(void);
// 入力モードを切り替える。
void __fastcall XgSetInputMode(HWND hwnd, XG_InputMode mode);
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

//////////////////////////////////////////////////////////////////////////////

// クリップボードから貼り付け。
void XgPasteBoard(HWND hwnd, const std::wstring& str);
// クリップボードにクロスワードをコピー。
void XgCopyBoard(HWND hwnd);
// クリップボードにクロスワードを画像としてコピー。
void XgCopyBoardAsImage(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////
// スクロール。

// 水平スクロールの位置を取得する。
int __fastcall XgGetHScrollPos(void);
// 垂直スクロールの位置を取得する。
int __fastcall XgGetVScrollPos(void);
// 水平スクロールの情報を取得する。
BOOL __fastcall XgGetHScrollInfo(LPSCROLLINFO psi);
// 垂直スクロールの情報を取得する。
BOOL __fastcall XgGetVScrollInfo(LPSCROLLINFO psi);
// 水平スクロールの位置を設定する。
int __fastcall XgSetHScrollPos(int nPos, BOOL bRedraw);
// 垂直スクロールの位置を設定する。
int __fastcall XgSetVScrollPos(int nPos, BOOL bRedraw);
// スクロール情報を設定する。
void __fastcall XgUpdateScrollInfo(HWND hwnd, int x, int y);
// キャレットが見えるように、必要ならばスクロールする。
void __fastcall XgEnsureCaretVisible(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////
