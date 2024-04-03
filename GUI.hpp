#pragma once

#ifdef _MSC_VER
    #define XG_NOINLINE __declspec(noinline)
#else
    #define XG_NOINLINE __attribute__((noinline))
#endif

// ヒントウィンドウを作成する。
BOOL XgCreateHintsWnd(HWND hwnd);
// ヒントウィンドウを破棄する。
void XgDestroyHintsWnd(void) noexcept;
// ヒントの内容をヒントウィンドウで開く。
bool XgOpenHintsByWindow(HWND /*hwnd*/);
// ヒントを表示する。
void XgShowHints(HWND hwnd);

// ヒントを表示するかどうか。
extern BOOL xg_bShowClues;

// クロスワードをチェックする。
bool __fastcall XgCheckCrossWord(HWND hwnd, bool check_words = true, bool loose = false);

//////////////////////////////////////////////////////////////////////////////

// UIフォントの論理オブジェクトを取得する。
LOGFONTW *XgGetUIFont(void);

// 「黒マスルールの説明.txt」を開く。
void __fastcall XgOpenRulesTxt(HWND hwnd);

// 計算時間測定用。
extern DWORDLONG xg_dwlTick0;    // 開始時間。
extern DWORDLONG xg_dwlTick1;    // 再計算時間。
extern DWORDLONG xg_dwlTick2;    // 終了時間。
extern DWORDLONG xg_dwlWait;     // 待ち時間。

// 再計算の回数。
extern LONG xg_nRetryCount;

// [二重マス単語の候補と配置]ダイアログの位置。
extern int xg_nMarkingX;
extern int xg_nMarkingY;

// 文字が入力された。
void __fastcall XgOnChar(HWND hwnd, TCHAR ch, int cRepeat);
void __fastcall XgOnImeChar(HWND hwnd, WCHAR ch, LPARAM /*lKeyData*/);
// BackSpaceを実行する。
void __fastcall XgCharBack(HWND hwnd);
// 入力方向を切り替える。
void __fastcall XgInputDirection(HWND hwnd, int nDirection);
// 文字送りを切り替える。
void __fastcall XgSetCharFeed(HWND hwnd, int nMode) noexcept;
// 改行する。
void __fastcall XgReturn(HWND hwnd);
// 二重マス切り替え。
void __fastcall XgToggleMark(HWND hwnd);
// その他のコマンド。
bool __fastcall XgOnCommandExtra(HWND hwnd, int id);

//////////////////////////////////////////////////////////////////////////////

// クリップボードから貼り付け。
bool XgPasteBoard(HWND hwnd, const XGStringW& str);
bool XgPasteBoard2(HWND hwnd, const XGStringW& str);
// クリップボードにクロスワードをコピー。
void XgCopyBoard(HWND hwnd);
// クリップボードにクロスワードを画像としてコピー。
void XgCopyBoardAsImage(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////

// 現在の状態で好ましいと思われる単語の最大長を取得する。
int __fastcall XgGetPreferredMaxLength(void) noexcept;

// マス位置を取得する。
VOID XgGetCellPosition(RECT& rc, int i1, int j1, int i2, int j2, BOOL bScroll) noexcept;
// マス位置を設定する。
VOID XgSetCellPosition(LONG& x, LONG& y, int& i, int& j, BOOL bEnd) noexcept;

// マウスの中央ボタンの処理に使う変数。
extern BOOL xg_bMButtonDragging;
extern POINT xg_ptMButtonDragging;

// 本当のクライアント領域を計算する。
void __fastcall XgGetRealClientRect(HWND hwnd, LPRECT prcClient) noexcept;
// ズームを実際のウィンドウに合わせる。
void __fastcall XgFitZoom(HWND hwnd);
// ズーム倍率を設定する。
void XgSetZoomRate(HWND hwnd, INT nZoomRate);
// テーマが変更された。
void XgUpdateTheme(HWND hwnd);
// ルールが変更された。
void XgUpdateRules(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////

// 問題を生成したときに答えを表示するか？
extern BOOL xg_bShowAnswerOnGenerate;

// LOOKS情報を読み込まない、書き込まない。
extern BOOL xg_bNoReadLooks;
extern BOOL xg_bNoWriteLooks;

// 「テーマ」ダイアログを表示する。
void __fastcall XgTheme(HWND hwnd);

//////////////////////////////////////////////////////////////////////////////
