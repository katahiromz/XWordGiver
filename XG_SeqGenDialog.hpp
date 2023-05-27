#pragma once

#include "XG_Window.hpp"

// 保存先のパスのリスト。
extern std::deque<std::wstring> xg_dirs_save_to;

// 連続生成の場合、問題を生成する数。
extern int xg_nNumberToGenerate;

// 「保存先」参照。
int CALLBACK XgBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM /*lParam*/, LPARAM /*lpData*/) noexcept;

// 保存先。
extern WCHAR xg_szDir[MAX_PATH];

// [問題の連続作成]ダイアログ。
class XG_SeqGenDialog : public XG_Dialog
{
public:
    XG_SeqGenDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // ドロップを有効にする。
        DragAcceptFiles(hwnd, TRUE);
        // サイズの欄を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nRows, FALSE);
        ::SetDlgItemInt(hwnd, edt2, xg_nCols, FALSE);
        // 現在の状態で好ましいと思われる単語の最大長を取得する。
        const auto n3 = XgGetPreferredMaxLength();
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);
        // スマート解決か？
        if (xg_bSmartResolution) {
            ::CheckDlgButton(hwnd, chx2, BST_CHECKED);
        } else {
            ::CheckDlgButton(hwnd, chx2, BST_UNCHECKED);
        }
        // 保存先を設定する。
        WCHAR szFile[MAX_PATH];
        COMBOBOXEXITEMW item;
        for (const auto& dir : xg_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCchCopy(szFile, _countof(szFile), dir.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);
        // 自動で再計算をするか？
        if (xg_bAutoRetry)
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        // 生成する数を設定する。
        ::SetDlgItemInt(hwnd, edt4, xg_nNumberToGenerate, FALSE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl = ::GetDlgItem(hwnd, edt1);
            ::ImmAssociateContext(hwndCtrl, nullptr);
            hwndCtrl = ::GetDlgItem(hwnd, edt2);
            ::ImmAssociateContext(hwndCtrl, nullptr);
            hwndCtrl = ::GetDlgItem(hwnd, edt3);
            ::ImmAssociateContext(hwndCtrl, nullptr);
            hwndCtrl = ::GetDlgItem(hwnd, edt4);
            ::ImmAssociateContext(hwndCtrl, nullptr);
        }
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr2, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_SIZE, XG_MIN_SIZE));
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_WORD_LEN, XG_MIN_WORD_LEN));
        SendDlgItemMessageW(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(100, 1));
        if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
            EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
            EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
            EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
        } else {
            EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
            EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
            EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
        }
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        int n1, n2, n3;
        switch (id)
        {
        case IDOK:
            // サイズの欄をチェックする。
            n1 = static_cast<int>(::GetDlgItemInt(hwnd, edt1, nullptr, FALSE));
            if (n1 < XG_MIN_SIZE || n1 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt1));
                return;
            }
            n2 = static_cast<int>(::GetDlgItemInt(hwnd, edt2, nullptr, FALSE));
            if (n2 < XG_MIN_SIZE || n2 > XG_MAX_SIZE) {
                ::SendDlgItemMessageW(hwnd, edt2, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt2));
                return;
            }
            n3 = static_cast<int>(::GetDlgItemInt(hwnd, edt3, nullptr, FALSE));
            if (n3 < XG_MIN_WORD_LEN || n3 > XG_MAX_WORD_LEN) {
                ::SendDlgItemMessageW(hwnd, edt3, EM_SETSEL, 0, -1);
                XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERINT), nullptr, MB_ICONERROR);
                ::SetFocus(::GetDlgItem(hwnd, edt3));
                return;
            }
            xg_nMaxWordLen = n3;
            // 自動で再計算をするか？
            xg_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // スマート解決か？
            xg_bSmartResolution = (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED);
            // 保存先のパス名を取得する。
            WCHAR szFile[MAX_PATH];
            ::GetDlgItemTextW(hwnd, cmb2, szFile, _countof(szFile));
            {
                const auto attrs = ::GetFileAttributesW(szFile);
                if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    // パスがなければ作成する。
                    if (!XgMakePathW(szFile)) {
                        // 作成に失敗。
                        ::SendDlgItemMessageW(hwnd, cmb2, CB_SETEDITSEL, 0, -1);
                        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_STORAGEINVALID), nullptr, MB_ICONERROR);
                        ::SetFocus(::GetDlgItem(hwnd, cmb2));
                        return;
                    }
                }
            }
            // 保存先をセットする。
            {
                std::wstring strDir = szFile;
                xg_str_trim(strDir);
                {
                    auto end = xg_dirs_save_to.end();
                    for (auto it = xg_dirs_save_to.begin(); it != end; it++) {
                        if (_wcsicmp((*it).data(), strDir.data()) == 0) {
                            xg_dirs_save_to.erase(it);
                            break;
                        }
                    }
                    xg_dirs_save_to.emplace_front(strDir);
                }
            }
            // 問題の数を取得する。
            {
                // 無制限ではない。
                BOOL bTranslated;
                xg_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt4, &bTranslated, FALSE);
                if (!bTranslated || xg_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt4, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt4));
                    return;
                }
            }
            // JSON形式として保存するか？
            xg_bSaveAsJsonFile = true;
            // 初期化する。
            {
                xg_bSolved = false;
                xg_bShowAnswer = false;
                xg_xword.ResetAndSetSize(n1, n2);
                xg_nRows = n1;
                xg_nCols = n2;
                xg_vTateInfo.clear();
                xg_vYokoInfo.clear();
                xg_vMarks.clear();
                xg_vMarkedCands.clear();
                // 偶数行数で黒マス線対称（タテ）の場合は連黒禁は不可。
                if (!(xg_nRows & 1) && (xg_nRules & RULE_LINESYMMETRYV) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENROWLINESYMV), nullptr, MB_ICONERROR);
                    break;
                }
                // 偶数列数で黒マス線対称（ヨコ）の場合は連黒禁は不可。
                if (!(xg_nCols & 1) && (xg_nRules & RULE_LINESYMMETRYH) && (xg_nRules & RULE_DONTDOUBLEBLACK)) {
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_EVENCOLLINESYMH), nullptr, MB_ICONERROR);
                    break;
                }
            }
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case chx2:
            if (::IsDlgButtonChecked(hwnd, chx2) == BST_CHECKED) {
                EnableWindow(GetDlgItem(hwnd, stc1), TRUE);
                EnableWindow(GetDlgItem(hwnd, edt3), TRUE);
                EnableWindow(GetDlgItem(hwnd, scr3), TRUE);
            } else {
                EnableWindow(GetDlgItem(hwnd, stc1), FALSE);
                EnableWindow(GetDlgItem(hwnd, edt3), FALSE);
                EnableWindow(GetDlgItem(hwnd, scr3), FALSE);
            }
            break;

        case psh2:
            {
                // ユーザーに保存先の場所を問い合わせる。
                BROWSEINFOW bi;
                ZeroMemory(&bi, sizeof(bi));
                bi.hwndOwner = hwnd;
                bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
                bi.ulFlags = BIF_RETURNONLYFSDIRS;
                bi.lpfn = XgBrowseCallbackProc;
                ::GetDlgItemTextW(hwnd, cmb2, xg_szDir, _countof(xg_szDir));
                LPITEMIDLIST pidl = ::SHBrowseForFolderW(&bi);
                if (pidl) {
                    // パスをコンボボックスに設定。
                    ::SHGetPathFromIDListW(pidl, szFile);
                    ::SetDlgItemTextW(hwnd, cmb2, szFile);
                    ::CoTaskMemFree(pidl);
                }
            }
            break;

        default:
            break;
        }
    }

    // 何かがドロップされた。
    void OnDropFiles(HWND hwnd, HDROP hdrop) noexcept
    {
        WCHAR szDir[MAX_PATH];
        DragQueryFile(hdrop, 0, szDir, _countof(szDir));
        if (PathIsDirectoryW(szDir)) // フォルダだった。
        {
            ::SetDlgItemTextW(hwnd, cmb2, szDir);
        }
        DragFinish(hdrop);
    }

    INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        default:
            break;
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_SEQCREATE);
    }
};
