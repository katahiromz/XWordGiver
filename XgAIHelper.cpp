//////////////////////////////////////////////////////////////////////////////
// XgAIHelper.cpp --- XWordGiver (Japanese Crossword Generator)
// Copyright (C) 2026 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

extern std::wstring xg_ai_provider;
extern std::wstring xg_ai_model;
extern std::wstring xg_additional_instruction;

INT_PTR CALLBACK
XgAIHelperDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static HBITMAP s_hbm = nullptr;
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            xg_ahSyncedDialogs[I_SYNCED_AIHELPER] = hwnd;

            // Add providers
            static const PCWSTR providers[] =
            {
                L"chatgpt", L"gemini", L"claude", L"grok", L"deepseek", L"sakana",
                L"qwen", L"kimi", L"mistral", L"llama",
            };
            for (auto provider : providers)
                SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)provider);

            // Add models
            static const PCWSTR models[] =
            {
                L"gpt-4o-mini",
                L"gemini-3.6-flash",
                L"claude-haiku-4-5-20251001",
                L"grok-4.6",
                L"deepseek-v4-flash",
                L"sakana-namazu",
                L"qwen3-max",
                L"kimi-k3",
                L"mistral-large-latest",
                L"llama-4-maverick",
            };
            for (auto model : models)
                SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)model);

            SetDlgItemTextW(hwnd, cmb1, xg_ai_provider.c_str());
            SetDlgItemTextW(hwnd, cmb2, xg_ai_model.c_str());
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
            case CBN_EDITCHANGE:
            case CBN_SELCHANGE:
            case CBN_SELENDOK:
                {
                    WCHAR text[512];
                    GetDlgItemTextW(hwnd, cmb1, text, _countof(text));

                    if (lstrcmpW(text, L"chatgpt") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"gpt-4o-mini");
                    else if (lstrcmpW(text, L"gemini") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"gemini-3.6-flash");
                    else if (lstrcmpW(text, L"claude") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"claude-haiku-4-5-20251001");
                    else if (lstrcmpW(text, L"grok") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"grok-4.6");
                    else if (lstrcmpW(text, L"deepseek") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"deepseek-v4-flash");
                    else if (lstrcmpW(text, L"sakana") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"sakana-namazu");
                    else if (lstrcmpW(text, L"qwen") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"qwen3-max");
                    else if (lstrcmpW(text, L"kimi") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"kimi-k3");
                    else if (lstrcmpW(text, L"mistral") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"mistral-large-latest");
                    else if (lstrcmpW(text, L"llama") == 0)
                        SetDlgItemTextW(hwnd, cmb2, L"llama-4-maverick");
                    else
                        SetDlgItemTextW(hwnd, cmb2, L"");

                    PropSheet_Changed(GetParent(hwnd), hwnd);
                    return 0;
                }
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
