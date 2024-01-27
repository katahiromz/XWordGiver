#pragma once

#include "XG_Window.hpp"

// [黒マスパターンの作成]ダイアログ。
class XG_PatGenDialog : public XG_Dialog
{
public:
    XG_PatGenDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 現在の状態で好ましいと思われる単語の最大長を取得する。
        const auto n3 = XgGetPreferredMaxLength();
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);
        // 単語長の範囲を指定する。
        ::SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_WORD_LEN, XG_MIN_WORD_LEN));
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        int n3;
        switch (id)
        {
        case IDOK:
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < XG_MIN_WORD_LEN || n3 > XG_MAX_WORD_LEN) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return;
            }
            xg_nMaxWordLen = n3;
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.clear();
                xg_vVertInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        default:
            break;
        }
    }

    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_PATGEN);
    }
};
