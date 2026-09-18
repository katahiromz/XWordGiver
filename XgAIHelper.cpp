//////////////////////////////////////////////////////////////////////////////
// XgAIHelper.cpp --- XWordGiver (Japanese Crossword Generator)
// Copyright (C) 2026 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

extern std::wstring xg_ai_provider;
extern std::wstring xg_ai_model;
extern std::wstring xg_additional_instruction;

// プロバイダーごとの既知のAIモデル名一覧。
struct KnownAIModelList
{
    PCWSTR provider;
    const PCWSTR* models;
    size_t count;
};

// TODO: １ヵ月ごとに以下の既知のAIモデル名一覧を更新せよ。可能ならば先頭を無料モデルにしておくこと。
static const PCWSTR s_chatgptModels[] = {
    L"gpt-5.6-luna", L"gpt-5.6-sol", L"gpt-5.6-terra", L"gpt-6-astra", L"gpt-4.1", L"gpt-4o-mini",
};
static const PCWSTR s_geminiModels[] = {
    L"gemini-3.6-flash", L"gemini-3.8-flash", L"gemini-3.5-flash-lite", L"gemini-3.1-pro", L"gemini-3-deep-think",
};
static const PCWSTR s_claudeModels[] = {
    L"claude-haiku-4-5-20251001", L"claude-sonnet-5", L"claude-opus-5", L"claude-fable-5-1",
};
static const PCWSTR s_grokModels[] = {
    L"grok-4", L"grok-4.1", L"grok-4.3", L"grok-4.5",
};
static const PCWSTR s_deepseekModels[] = {
    L"deepseek-v4-flash", L"deepseek-v4-pro", L"deepseek-v3.2",
};
static const PCWSTR s_sakanaModels[] = {
    L"sakana-namazu", L"sakana-fugu",
};
static const PCWSTR s_qwenModels[] = {
    L"qwen3-turbo", L"qwen-long", L"qwen3.7-plus", L"qwen3.7-max", L"qwen3.8-max",
};
static const PCWSTR s_kimiModels[] = {
    L"moonshot-v1-auto", L"kimi-k2.6", L"kimi-k3",
};
static const PCWSTR s_mistralModels[] = {
    L"mistral-small-latest", L"ministral-3-8b", L"mistral-medium-latest", L"mistral-large-latest",
};
static const PCWSTR s_llamaModels[] = {
    L"llama-4-scout", L"llama-4-maverick", L"llama-3.3-70b",
};
static const PCWSTR s_pepaboModels[] = {
    L"gpt-4o-mini", L"gpt-5-6-luna",
};

#define MODELS_ENTRY(name, arr) { (name), (arr), _countof(arr) }
static const KnownAIModelList s_knownAIModels[] = {
    MODELS_ENTRY(L"chatgpt",  s_chatgptModels),
    MODELS_ENTRY(L"gemini",   s_geminiModels),
    MODELS_ENTRY(L"claude",   s_claudeModels),
    MODELS_ENTRY(L"grok",     s_grokModels),
    MODELS_ENTRY(L"deepseek", s_deepseekModels),
    MODELS_ENTRY(L"sakana",   s_sakanaModels),
    MODELS_ENTRY(L"qwen",     s_qwenModels),
    MODELS_ENTRY(L"kimi",     s_kimiModels),
    MODELS_ENTRY(L"mistral",  s_mistralModels),
    MODELS_ENTRY(L"llama",    s_llamaModels),
    MODELS_ENTRY(L"pepabo",   s_pepaboModels),
};
#undef MODELS_ENTRY

// cmb2（モデル一覧コンボ）を、指定プロバイダーの実際のモデル一覧で埋め直す。
static void XgFillModelCombo(HWND hwnd, PCWSTR provider, PCWSTR preferredModel = nullptr)
{
    SendDlgItemMessageW(hwnd, cmb2, CB_RESETCONTENT, 0, 0);

    // 既知モデル名一覧
    for (const auto& entry : s_knownAIModels)
    {
        if (lstrcmpW(provider, entry.provider) == 0)
        {
            for (size_t i = 0; i < entry.count; ++i)
                SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)entry.models[i]);
            break;
        }
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
        WCHAR first[512] = L"";
        if (SendDlgItemMessageW(hwnd, cmb2, CB_GETLBTEXT, 0, (LPARAM)first) != CB_ERR)
            SetDlgItemTextW(hwnd, cmb2, first);
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

            // Add providers
            static const PCWSTR providers[] =
            {
                L"chatgpt", L"gemini", L"claude", L"grok", L"deepseek", L"sakana",
                L"qwen", L"kimi", L"mistral", L"llama", L"pepabo",
            };
            for (auto provider : providers)
                SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)provider);

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
