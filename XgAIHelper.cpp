// XgAIHelper.cpp --- XWordGiver (Japanese Crossword Generator)
// Copyright (C) 2026 Katayama Hirofumi MZ. All Rights Reserved.
// Author: katahiromz
// License: MIT

extern std::wstring xg_ai_provider;
extern std::wstring xg_ai_model;
extern std::wstring xg_additional_instruction;

#include "AIHelper.h"

// cmb2（モデル一覧コンボ）を、指定プロバイダーの実際のモデル一覧で埋め直す。
static void XgFillModelCombo(HWND hwnd, PCWSTR provider, PCWSTR preferredModel = nullptr)
{
    SendDlgItemMessageW(hwnd, cmb2, CB_RESETCONTENT, 0, 0);

    // AIモデル群を取得。
    std::vector<std::wstring> models;
    if (Helper_GetAIModels(provider, models))
    {
        for (auto model : models)
            SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)model.c_str());
    }

    // 優先モデル（保存済みの設定値など）が指定されていればそれを選択状態にする。
    // 一覧に無ければ追加してから選択する。
    if (preferredModel && *preferredModel)
    {
        if (SendDlgItemMessageW(hwnd, cmb2, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)preferredModel) == CB_ERR)
            SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)preferredModel);
        SetDlgItemTextW(hwnd, cmb2, preferredModel);
    }
    else
    {
        SetDlgItemTextW(hwnd, cmb2, models.empty() ? L"" : models[0].c_str());
    }
}

INT_PTR CALLBACK
XgAIHelperDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP s_hbm = nullptr;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            xg_ahSyncedDialogs[I_SYNCED_AIHELPER] = hwnd;

            // Add providers（AIModels.dat の [PROVIDERS] セクションから取得する）
            std::vector<std::wstring> providers;
            Helper_GetAIProviders(providers);
            for (auto& provider : providers)
                SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)provider.c_str());

            // Add models（選択中のプロバイダーの実際のモデル一覧を取得してセットする）
            SetDlgItemTextW(hwnd, cmb1, xg_ai_provider.c_str());
            XgFillModelCombo(hwnd, xg_ai_provider.c_str(), xg_ai_model.c_str());

            SetDlgItemTextW(hwnd, edt2, xg_additional_instruction.c_str());

            HWND hStc1 = GetDlgItem(hwnd, stc1);

            s_hbm = LoadBitmapW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(5));
            SendMessageW(hStc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)s_hbm);

            BITMAP bm;
            GetObjectW(s_hbm, sizeof(bm), &bm);

            RECT rc;
            GetWindowRect(hStc1, &rc);
            MapWindowRect(nullptr, hwnd, &rc);
            MoveWindow(hStc1, rc.left, rc.top, bm.bmWidth, bm.bmHeight, TRUE);
        }
        return TRUE;
    case WM_DESTROY:
        if (s_hbm)
        {
            DeleteObject(s_hbm);
            s_hbm = nullptr;
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case psh1:
            {
                WCHAR path[MAX_PATH];
                GetModuleFileNameW(nullptr, path, _countof(path));
                PathRemoveFileSpecW(path);
                PathAppendW(path, L"AIHelper.txt");
                ShellExecuteW(hwnd, nullptr, path, nullptr, nullptr, SW_SHOWNORMAL);
            }
            break;
        case edt1:
        case edt2:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                PropSheet_Changed(GetParent(hwnd), hwnd);
            }
            break;
        case cmb1:
            switch (HIWORD(wParam))
            {
            case CBN_SELENDOK:
                {
                    // ドロップダウンから確定的に選択された項目を、編集中のテキストではなく
                    // リスト側のインデックスから取得する（矢印キー移動中の未確定の値を
                    // 拾ってしまわないようにするため）
                    LRESULT idx = SendDlgItemMessageW(hwnd, cmb1, CB_GETCURSEL, 0, 0);
                    WCHAR text[512] = L"";
                    if (idx != CB_ERR)
                        SendDlgItemMessageW(hwnd, cmb1, CB_GETLBTEXT, (WPARAM)idx, (LPARAM)text);
                    else
                        GetDlgItemTextW(hwnd, cmb1, text, _countof(text));

                    // プロバイダーが確定したら、実際のモデル一覧を取得してcmb2を更新する
                    XgFillModelCombo(hwnd, text);

                    PropSheet_Changed(GetParent(hwnd), hwnd);
                    return 0;
                }
            case CBN_KILLFOCUS:
                {
                    // 直接入力（手入力）で確定した場合にも一覧を更新する。
                    // ※ CBN_SELCHANGE は矢印キーでの一時的なハイライト変化でも発生するため
                    //    ここでは使わない（未確定の項目でモデルを取得してしまうのを防ぐ）。
                    WCHAR text[512];
                    GetDlgItemTextW(hwnd, cmb1, text, _countof(text));
                    XgFillModelCombo(hwnd, text);
                }
                break;
            case CBN_EDITCHANGE:
                // 入力中はモデル一覧を再取得せず、変更フラグだけ立てる
                PropSheet_Changed(GetParent(hwnd), hwnd);
                return 0;
            }
            break;
        case cmb2:
            switch (HIWORD(wParam))
            {
            case CBN_EDITCHANGE:
            case CBN_SELCHANGE:
            case CBN_SELENDOK:
                PropSheet_Changed(GetParent(hwnd), hwnd);
                return 0;
            }
        }
        break;
    case WM_NOTIFY:
        {
            WCHAR text[512];
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            switch (pnmhdr->code) {
            case PSN_APPLY: // Apply
                GetDlgItemTextW(hwnd, cmb1, text, _countof(text));
                xg_ai_provider = text;
                GetDlgItemTextW(hwnd, cmb2, text, _countof(text));
                xg_ai_model = text;
                GetDlgItemTextW(hwnd, edt2, text, _countof(text));
                xg_additional_instruction = text;
                return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
            }
        }
        break;
    }
    return 0;
}
