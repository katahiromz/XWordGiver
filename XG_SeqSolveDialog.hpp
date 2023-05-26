#pragma once

#include "XG_Window.hpp"

// [解の連続作成]ダイアログ。
class XG_SeqSolveDialog : public XG_Dialog
{
public:
    XG_SeqSolveDialog() noexcept
    {
    }

    BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
        // ダイアログを中央へ移動する。
        XgCenterDialog(hwnd);
        // 保存先を設定する。
        WCHAR szFile[MAX_PATH];
        COMBOBOXEXITEMW item;
        for (const auto& dir : xg_dirs_save_to) {
            item.mask = CBEIF_TEXT;
            item.iItem = -1;
            StringCbCopy(szFile, sizeof(szFile), dir.data());
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                  reinterpret_cast<LPARAM>(&item));
        }
        // コンボボックスの最初の項目を選択する。
        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        // 自動で再計算をするか？
        if (xg_bAutoRetry) {
            ::CheckDlgButton(hwnd, chx1, BST_CHECKED);
        }
        // 無制限か？
        // 生成する数を設定する。
        ::SetDlgItemInt(hwnd, edt1, xg_nNumberToGenerate, FALSE);
        // ファイルドロップを有効にする。
        ::DragAcceptFiles(hwnd, TRUE);
        SendDlgItemMessageW(hwnd, scr1, UDM_SETRANGE, 0, MAKELPARAM(100, 2));
        return TRUE;
    }

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        WCHAR szFile[MAX_PATH];
        switch (id)
        {
        case IDOK:
            // 保存先のパス名を取得する。
            ::GetDlgItemTextW(hwnd, cmb1, szFile, _countof(szFile));
            {
                const auto attrs = ::GetFileAttributesW(szFile);
                if (attrs == 0xFFFFFFFF || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                    // パスがなければ作成する。
                    if (!XgMakePathW(szFile)) {
                        // 作成に失敗。
                        ::SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, -1);
                        XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_STORAGEINVALID), nullptr, MB_ICONERROR);
                        ::SetFocus(::GetDlgItem(hwnd, cmb1));
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
            // 自動で再計算をするか？
            xg_bAutoRetry = (::IsDlgButtonChecked(hwnd, chx1) == BST_CHECKED);
            // 問題の数を取得する。
            {
                BOOL bTranslated;
                xg_nNumberToGenerate = ::GetDlgItemInt(hwnd, edt1, &bTranslated, FALSE);
                if (!bTranslated || xg_nNumberToGenerate == 0) {
                    ::SendDlgItemMessageW(hwnd, edt1, EM_SETSEL, 0, -1);
                    XgCenterMessageBoxW(hwnd, XgLoadStringDx1(IDS_ENTERPOSITIVE), nullptr, MB_ICONERROR);
                    ::SetFocus(::GetDlgItem(hwnd, edt1));
                    return;
                }
            }
            // JSON形式で保存するか？
            xg_bSaveAsJsonFile = true;
            // 初期化する。
            xg_bSolved = false;
            xg_bShowAnswer = false;
            xg_vTateInfo.clear();
            xg_vYokoInfo.clear();
            xg_vMarks.clear();
            xg_vMarkedCands.clear();
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDOK);
            break;

        case IDCANCEL:
            // ダイアログを閉じる。
            ::EndDialog(hwnd, IDCANCEL);
            break;

        case psh1:
            // ユーザーに保存先を問い合わせる。
            {
                BROWSEINFOW bi;
                ZeroMemory(&bi, sizeof(bi));
                bi.hwndOwner = hwnd;
                bi.lpszTitle = XgLoadStringDx1(IDS_CROSSSTORAGE);
                bi.ulFlags = BIF_RETURNONLYFSDIRS;
                bi.lpfn = XgBrowseCallbackProc;
                ::GetDlgItemTextW(hwnd, cmb1, xg_szDir, _countof(xg_szDir));
                {
                    LPITEMIDLIST pidl = ::SHBrowseForFolderW(&bi);
                    if (pidl) {
                        // コンボボックスにパスを設定する。
                        ::SHGetPathFromIDListW(pidl, szFile);
                        ::SetDlgItemTextW(hwnd, cmb1, szFile);
                        ::CoTaskMemFree(pidl);
                    }
                }
            }
            break;

        default:
            break;
        }
    }

    void OnDropFiles(HWND hwnd, HDROP hDrop)
    {
        // ドロップされたファイルのパス名を取得する。
        WCHAR szFile[MAX_PATH];
        ::DragQueryFileW(hDrop, 0, szFile, _countof(szFile));
        ::DragFinish(hDrop);

        // ショートカットだった場合は、ターゲットのパスを取得する。
        if (::lstrcmpiW(PathFindExtensionW(szFile), L".LNK") == 0) {
            WCHAR szTarget[MAX_PATH];
            if (!XgGetPathOfShortcutW(szFile, szTarget)) {
                ::MessageBeep(0xFFFFFFFF);
                return;
            }
            StringCbCopy(szFile, sizeof(szFile), szTarget);
        }

        // ファイルの属性を確認する。
        const auto attrs = ::GetFileAttributesW(szFile);
        if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
            // ディレクトリーだった。
            // 同じ項目がすでにあれば、削除する。
            const int i = static_cast<int>(::SendDlgItemMessageW(
                hwnd, cmb1, CB_FINDSTRINGEXACT, 0,
                reinterpret_cast<LPARAM>(szFile)));
            if (i != CB_ERR) {
                ::SendDlgItemMessageW(hwnd, cmb1, CB_DELETESTRING, i, 0);
            }
            // コンボボックスの最初に挿入する。
            COMBOBOXEXITEMW item;
            item.mask = CBEIF_TEXT;
            item.iItem = 0;
            item.pszText = szFile;
            item.cchTextMax = -1;
            ::SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEMW, 0,
                                reinterpret_cast<LPARAM>(&item));
            // コンボボックスの最初の項目を選択する。
            ::SendDlgItemMessageW(hwnd, cmb1, CB_SETCURSEL, 0, 0);
        } else {
            // ディレクトリーではなかった。
            ::MessageBeep(0xFFFFFFFF);
        }
    }

    virtual INT_PTR CALLBACK
    DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
        return DialogBoxDx(hwnd, IDD_SEQSOLVE);
    }
};
