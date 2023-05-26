﻿#pragma once

#include "XG_Window.hpp"
#include "XG_ColorBox.hpp"
#include "XG_Settings.hpp"

// 「見た目の設定」ダイアログ。
class XG_SettingsDialog : public XG_Dialog
{
public:
    XG_ColorBox m_hwndWhite;
    XG_ColorBox m_hwndBlack;
    XG_ColorBox m_hwndMarked;

    std::wstring m_strAutoFile;
    BOOL m_bImport = FALSE;
    BOOL m_bUpdating = FALSE;

    XG_SettingsDialog() noexcept
    {
        m_hwndWhite.SetColor(xg_rgbWhiteCellColor);
        m_hwndBlack.SetColor(xg_rgbBlackCellColor);
        m_hwndMarked.SetColor(xg_rgbMarkedCellColor);
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

    // [設定]ダイアログのオーナードロー。
    void OnDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam) noexcept;

    // ファイルがドロップされた？
    void OnDropFiles(HWND hwnd, HDROP hdrop);

    // [設定]ダイアログのダイアログ プロシージャー。
    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // BLOCKのプレビュー。
    void UpdateBlockPreview(HWND hwnd);

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_CONFIG);
    }
};
