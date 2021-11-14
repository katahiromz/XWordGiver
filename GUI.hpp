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

// ポップアップメニューを読み込む。
HMENU XgLoadPopupMenu(HWND hwnd, INT nPos);

// ツールバーのUIを更新する。
void XgUpdateToolBarUI(HWND hwnd);

// クリップボードから貼り付け。
void XgPasteBoard(HWND hwnd, const std::wstring& str);
// クリップボードにクロスワードをコピー。
void XgCopyBoard(HWND hwnd);
// クリップボードにクロスワードを画像としてコピー。
void XgCopyBoardAsImage(HWND hwnd);
