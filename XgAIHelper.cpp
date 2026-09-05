//////////////////////////////////////////////////////////////////////////////
// XgAIHelper.cpp --- XWordGiver (Japanese Crossword Generator)
// Copyright (C) 2026 Katayama Hirofumi MZ. All Rights Reserved.
// (Japanese, UTF-8)

INT_PTR CALLBACK
XgAIHelperDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        xg_ahSyncedDialogs[I_SYNCED_AIHELPER] = hwnd;
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR)lParam;
            switch (pnmhdr->code) {
            case PSN_APPLY: // 適用
                return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);
            }
        }
        break;
    }
    return 0;
}
