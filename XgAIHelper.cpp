//////////////////////////////////////////////////////////////////////////////
// XgAIHelper.cpp --- XWordGiver (Japanese Crossword Generator)
// Copyright (C) 2026 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

extern std::wstring g_privider;
extern std::wstring g_model;

INT_PTR CALLBACK
XgAIHelperDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            xg_ahSyncedDialogs[I_SYNCED_AIHELPER] = hwnd;

            // Add providers
            static const PCWSTR providers[] =
            {
                L"chatgpt", L"gemini", L"claude", L"grok", L"deepseek", L"sakana",
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
            };
            for (auto model : models)
                SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)model);

            SetDlgItemTextW(hwnd, cmb1, g_privider.c_str());
            SetDlgItemTextW(hwnd, cmb2, g_model.c_str());
        }
        return TRUE;
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
                    else
                        SetDlgItemTextW(hwnd, cmb2, L"");

                    PropSheet_Changed(GetParent(hwnd), hwnd);
                    return 0;
                }
            }
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
                g_privider = text;
                GetDlgItemTextW(hwnd, cmb2, text, _countof(text));
                g_model = text;
                return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
            }
        }
        break;
    }
    return 0;
}
