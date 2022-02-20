#pragma once

#include "XG_Dialog.hpp"

// UIフォント。
extern WCHAR xg_szUIFont[LF_FACESIZE];

// ツールバーを表示するか？
extern bool xg_bShowToolBar;

// マスのフォント。
extern WCHAR xg_szCellFont[LF_FACESIZE];

// 小さな文字のフォント。
extern WCHAR xg_szSmallFont[LF_FACESIZE];

// UIフォント。
extern WCHAR xg_szUIFont[LF_FACESIZE];

// 「見た目の設定」ダイアログ。
class XG_SettingsDialog : public XG_Dialog
{
public:
    inline static COLORREF s_rgbColorTable[] = {
        RGB(0, 0, 0),
        RGB(0x33, 0x33, 0x33),
        RGB(0x66, 0x66, 0x66),
        RGB(0x99, 0x99, 0x99),
        RGB(0xCC, 0xCC, 0xCC),
        RGB(0xFF, 0xFF, 0xFF),
        RGB(0xFF, 0xFF, 0xCC),
        RGB(0xFF, 0xCC, 0xFF),
        RGB(0xFF, 0xCC, 0xCC),
        RGB(0xCC, 0xFF, 0xFF),
        RGB(0xCC, 0xFF, 0xCC),
        RGB(0xCC, 0xCC, 0xFF),
        RGB(0xCC, 0xCC, 0xCC),
        RGB(0, 0, 0xCC),
        RGB(0, 0xCC, 0),
        RGB(0xCC, 0, 0),
    };
    // 一時的に保存する色のデータ。
    inline static COLORREF s_rgbColors[3];

    LPCWSTR m_pszAutoFile = NULL;
    BOOL m_bImport = FALSE;
    BOOL m_bUpdating = FALSE;

    XG_SettingsDialog()
    {
    }

    // [設定]ダイアログの初期化。
    BOOL OnInitDialog(HWND hwnd);

    // [設定]ダイアログで[OK]ボタンを押された。
    void OnOK(HWND hwnd);

    // LOOKSファイルのインポート。
    BOOL DoImportLooks(HWND hwnd, LPCWSTR pszFileName);

    // LOOKSファイルのエクスポート。
    BOOL DoExportLooks(HWND hwnd, LPCWSTR pszFileName);

    // 設定のインポート。
    BOOL OnImportLooks(HWND hwnd);

    // 設定のエクスポート。
    BOOL OnExportLooks(HWND hwnd);

    // 設定のリセット。
    void OnResetLooks(HWND hwnd);

    // UIフォントの論理オブジェクトを設定する。
    void SetUIFont(HWND hwnd, const LOGFONTW *plf);

    // [設定]ダイアログで[変更...]ボタンを押された。
    void OnChange(HWND hwnd, int i);

    // [設定]ダイアログで[リセット]ボタンを押された。
    void OnReset(HWND hwnd, int i)
    {
        switch (i) {
        case 0:
            ::SetDlgItemTextW(hwnd, edt1, L"");
            break;

        case 1:
            ::SetDlgItemTextW(hwnd, edt2, L"");
            break;

        case 2:
            ::SetDlgItemTextW(hwnd, edt3, L"");
            break;
        }
    }

    // [設定]ダイアログのオーナードロー。
    void OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam);

    // 色を指定する。
    void OnSetColor(HWND hwnd, int nIndex);

    // ファイルがドロップされた？
    void OnDropFiles(HWND hwnd, HDROP hdrop);

    // [設定]ダイアログのダイアログ プロシージャー。
    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // BLOCKのプレビュー。
    void UpdateBlockPreview(HWND hwnd);

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_CONFIG);
    }
};
