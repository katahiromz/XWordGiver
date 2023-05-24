#pragma once

#include "XG_Window.hpp"

// [黒マスパターンの連続作成]ダイアログ。
class XG_SeqPatGenDialog : public XG_Dialog
{
public:
    XG_SeqPatGenDialog()
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // ドラッグ＆ドロップを有効にする。
        DragAcceptFiles(hwnd, TRUE);
        // 現在の状態で好ましいと思われる単語の最大長を取得する。
        INT n3 = XgGetPreferredMaxLength();
        ::SetDlgItemInt(hwnd, edt3, n3, FALSE);
        // 保存先を設定する。
        COMBOBOXEXITEMW item;
        WCHAR szFile[MAX_PATH];
        for (const auto& dir : xg_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCbCopy(szFile, sizeof(szFile), dir.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb2, CB_SETCURSEL, 0, 0);
        // 生成する数を設定する。
        ::SetDlgItemInt(hwnd, edt4, xg_nNumberToGenerate, FALSE);
        // IMEをOFFにする。
        {
            HWND hwndCtrl = ::GetDlgItem(hwnd, edt3);
            ::ImmAssociateContext(hwndCtrl, nullptr);
            hwndCtrl = ::GetDlgItem(hwnd, edt4);
            ::ImmAssociateContext(hwndCtrl, nullptr);
        }
        SendDlgItemMessageW(hwnd, scr3, UDM_SETRANGE, 0, MAKELPARAM(XG_MAX_WORD_LEN, XG_MIN_WORD_LEN));
        SendDlgItemMessageW(hwnd, scr4, UDM_SETRANGE, 0, MAKELPARAM(100, 1));
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        INT n3;
        WCHAR szFile[MAX_PATH];
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
            // 保存先のパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb2, szFile, _countof(szFile));
            {
                DWORD attrs = ::GetFileAttributesW(szFile);
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
                xg_xword.clear();
                xg_vTateInfo.clear();
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

        case psh2:
            // ユーザーに保存先の場所を問い合わせる。
            {
                BROWSEINFOW bi;
                ZeroMemory(&bi, sizeof(bi));
                bi.hwndOwner = hwnd;
                bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
                bi.ulFlags = BIF_RETURNONLYFSDIRS;
                bi.lpfn = XgBrowseCallbackProc;
                ::GetDlgItemTextW(hwnd, cmb2, xg_szDir, _countof(xg_szDir));
                {
                    LPITEMIDLIST pidl = ::SHBrowseForFolderW(&bi);
                    if (pidl) {
                        // パスをコンボボックスに設定。
                        ::SHGetPathFromIDListW(pidl, szFile);
                        ::SetDlgItemTextW(hwnd, cmb2, szFile);
                        ::CoTaskMemFree(pidl);
                    }
                }
            }
            break;
        }
    }

    // 何かがドロップされた。
    void OnDropFiles(HWND hwnd, HDROP hdrop)
    {
        WCHAR szDir[MAX_PATH];
        DragQueryFile(hdrop, 0, szDir, _countof(szDir));
        if (PathIsDirectoryW(szDir)) // フォルダだった。
        {
            ::SetDlgItemTextW(hwnd, cmb2, szDir);
        }
        DragFinish(hdrop);
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        switch (uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
            HANDLE_MSG(hwnd, WM_DROPFILES, OnDropFiles);
        }
        return 0;
    }

    INT_PTR DoModal(HWND hwnd)
    {
        return DialogBoxDx(hwnd, IDD_SEQPATGEN);
    }
};
