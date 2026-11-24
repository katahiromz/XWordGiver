// MAIHelperDialog.h --- 「AI ヘルパー」設定ダイアログ
// Copyright (C) 2026 Katayama Hirofumi MZ. All Rights Reserved.
// Author: katahiromz
// License: MIT

#include "MWindowBase.h"
#include "MPropSheet.h"
#include "MAIHelper.h"
#include "dict.h"
#include <functional>

class MAIHelperDialog : public MPropSheetPage {
public:
	INT m_bUpdating = 0;
	MAIHelper& m_ai_helper;
	MStringW m_ai_provider;
	MStringW m_ai_model;
    MStringListW m_promptMapping;
    HBITMAP m_hbm = nullptr;
	// 適用(PSN_APPLY)が成功したときに呼ばれる。
	std::function<void()> m_fnApply;

	MAIHelperDialog(MAIHelper& ai_helper)
		: MPropSheetPage(IDD_AIHELPER)
		, m_ai_helper(ai_helper)
    	, m_ai_provider(ai_helper.m_provider)
    	, m_ai_model(ai_helper.m_model)
    	, m_promptMapping(ai_helper.m_promptMapping)
	{
	}

	// cmb2（モデル一覧コンボ）を、指定プロバイダーの実際のモデル一覧で埋め直す。
	void XgFillModelCombo(HWND hwnd, PCWSTR provider, PCWSTR preferredModel = nullptr) {
		SendDlgItemMessageW(hwnd, cmb2, CB_RESETCONTENT, 0, 0);

		// AIモデル群を取得。
		MStringListW models;
		if (m_ai_helper.GetAIModels(provider, models)) {
			for (auto model : models)
				SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)model.c_str());
		}

		// 優先モデル（保存済みの設定値など）が指定されていればそれを選択状態にする。
		// 一覧に無ければ追加してから選択する。
		if (preferredModel && *preferredModel) {
			if (SendDlgItemMessageW(hwnd, cmb2, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)preferredModel) == CB_ERR)
				SendDlgItemMessageW(hwnd, cmb2, CB_ADDSTRING, 0, (LPARAM)preferredModel);
			SetDlgItemTextW(hwnd, cmb2, preferredModel);
		} else {
			SetDlgItemTextW(hwnd, cmb2, models.empty() ? L"" : models[0].c_str());
		}
	}

	// WM_INITDIALOG
	BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) {
		// Add providers（AIModels.dat の [PROVIDERS] セクションから取得する）
		MStringListW providers;
		m_ai_helper.GetAIProviders(providers);
		for (auto& provider : providers)
			SendDlgItemMessageW(hwnd, cmb1, CB_ADDSTRING, 0, (LPARAM)provider.c_str());

		// Add models（選択中のプロバイダーの実際のモデル一覧を取得してセットする）
		SetDlgItemTextW(hwnd, cmb1, m_ai_provider.c_str());
		XgFillModelCombo(hwnd, m_ai_provider.c_str(), m_ai_model.c_str());

		// ビットマップをstc1にセットする。
		HWND hStc1 = GetDlgItem(hwnd, stc1);
		m_hbm = LoadBitmapW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(5));
		SendMessageW(hStc1, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)m_hbm);

		// stc1をビットマップのサイズに合わせる。
		BITMAP bm;
		GetObjectW(m_hbm, sizeof(bm), &bm);
		RECT rc;
		GetWindowRect(hStc1, &rc);
		MapWindowRect(nullptr, hwnd, &rc);
		MoveWindow(hStc1, rc.left, rc.top, bm.bmWidth, bm.bmHeight, TRUE);

		// 妖精の名前をセットする。
		WCHAR name[MAX_PATH];
		LoadStringW(xg_hInstance, IDS_FAIRY_NAME00, name, _countof(name));
		SendDlgItemMessageW(hwnd, cmb4, CB_ADDSTRING, 0, (LPARAM)name);
		SendDlgItemMessageW(hwnd, cmb4, CB_SETCURSEL, 0, 0);

		// プロンプトの種類群を追加。
		HWND hCmb3 = GetDlgItem(hwnd, cmb3);
		for (size_t type = PROMPT_TYPE_INITIAL; type < PROMPT_TYPE_MAX; ++type)
			ComboBox_AddString(hCmb3, m_ai_helper.GetPromptName((PROMPT_TYPE)type).c_str());

	    // プロンプトの種類をセット。
		ComboBox_SetCurSel(hCmb3, 0);
		// プロンプトのテキストの最大長をセット。
		SendDlgItemMessageW(hwnd, edt2, EM_LIMITTEXT, MAX_PROMPT_TEXT - 1, 0);
		// プロンプトのテキストをセット。
		assert(m_promptMapping.size() == (size_t)PROMPT_TYPE_MAX);
		SetDlgItemTextW(hwnd, edt2, m_promptMapping[0].c_str());

		return TRUE;
	}

	// WM_COMMAND
	void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
		switch (id) {
		case psh1:
			{
				// ファイル「AIHelper.txt」を開く。
			    auto path = XgFindLocalFile(L"AIHelper.txt");
				ShellExecuteW(hwnd, nullptr, path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
			}
			break;
		case psh2:
			// プロンプトをリセット。
		    m_ai_helper.ResetPrompts(m_promptMapping);
			// プロンプトの種類をセット。
			SendDlgItemMessageW(hwnd, cmb3, CB_SETCURSEL, 0, 0);
			// プロンプトのテキストをセット。
			SetDlgItemTextW(hwnd, edt2, m_promptMapping[0].c_str());
			Changed(hwnd);
			break;
		case edt1:
			if (codeNotify == EN_CHANGE)
    			Changed(hwnd);
			break;
		case edt2:
			if (codeNotify == EN_CHANGE) {
				INT iItem = (INT)SendDlgItemMessageW(hwnd, cmb3, CB_GETCURSEL, 0, 0);

				WCHAR text[MAX_PROMPT_TEXT];
				GetDlgItemTextW(hwnd, edt2, text, _countof(text));
				assert(iItem < (INT)m_promptMapping.size());
				m_promptMapping[iItem] = text;

    			Changed(hwnd);
			}
			break;
		case cmb1:
			switch (codeNotify) {
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

					Changed(hwnd);
				}
				break;
			case CBN_KILLFOCUS:
				{
					// 直接入力（手入力）で確定した場合にも一覧を更新する。
					// ※ CBN_SELCHANGE は矢印キーでの一時的なハイライト変化でも発生するため
					//	ここでは使わない（未確定の項目でモデルを取得してしまうのを防ぐ）。
					WCHAR text[512];
					GetDlgItemTextW(hwnd, cmb1, text, _countof(text));
					XgFillModelCombo(hwnd, text);
				}
				break;
			case CBN_EDITCHANGE:
				// 入力中はモデル一覧を再取得せず、変更フラグだけ立てる
				Changed(hwnd);
				break;
			}
			break;
		case cmb2:
			switch (codeNotify) {
			case CBN_EDITCHANGE:
			case CBN_SELCHANGE:
			case CBN_SELENDOK:
			    Changed(hwnd);
				break;
			}
			break;
		case cmb3: // プロンプトの種類
			switch (codeNotify) {
			case CBN_SELCHANGE:
			case CBN_SELENDOK:
				{
					INT iItem = (INT)SendDlgItemMessageW(hwnd, cmb3, CB_GETCURSEL, 0, 0);
					assert(iItem < (INT)m_promptMapping.size());
					SetDlgItemTextW(hwnd, edt2, m_promptMapping[iItem].c_str());
				}
				break;
			}
			break;
		}
	}

	// WM_NOTIFY
	LRESULT OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr) {
		HWND hwndLst1 = GetDlgItem(hwnd, lst1);
		auto pListView = (NM_LISTVIEW*)pnmhdr;
		switch (pnmhdr->code) {
		case PSN_APPLY: // 適用
			{
				WCHAR text[512];
				GetDlgItemTextW(hwnd, cmb1, text, _countof(text));
				m_ai_provider = text;
				GetDlgItemTextW(hwnd, cmb2, text, _countof(text));
				m_ai_model = text;
				if (m_fnApply)
					m_fnApply();
			}
			return SetDlgMsgResult(hwnd, WM_NOTIFY, PSNRET_NOERROR);

		}
		return 0;
	}

    // WM_DESTROY
    void OnDestroy(HWND hwnd) {
		if (m_hbm) {
			DeleteObject(m_hbm);
			m_hbm = nullptr;
		}
    }

	INT_PTR CALLBACK
	DialogProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override {
		switch (uMsg) {
			HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
			HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
			HANDLE_MSG(hwnd, WM_NOTIFY, OnNotify);
			HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
		}
		return 0;
	}

	INT_PTR DoModal(HWND hwnd) {
		m_bSheet = FALSE;
		return DialogBoxDx(hwnd, IDD_AIHELPER);
	}

	void Changed(HWND hwnd) {
		if (m_bSheet)
			PropSheet_Changed(GetParent(hwnd), hwnd);
	}

protected:
	BOOL m_bSheet = TRUE;
};
