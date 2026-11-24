// main.cpp
// Author: katahiromz
// License: MIT

#include "xwg.h"
#include "algo1.h"
#include "algo2.h"
#include "algo3.h"
#include "algo4.h"
#include "algo5.h"
#include "clues.h"
#include "dict.h"
#include "histogram.h"
#include "board-text-io.h"
#include "pat-file-io.h"
#include "flip-board.h"
#include "utils.h"
#include "gui/MColorBox.h"
#include "gui/MJumpDialog.h"
#include "gui/MNewDialog.h"
#include "gui/MNotesDialog.h"
#include "gui/MPatGenDialog.h"
#include "gui/MRulePresetDialog.h"
#include "gui/MThemeDialog.h"
#include "gui/MUILanguageDialog.h"
#include "gui/MWordListDialog.h"
#include "gui/MCluesWnd.h"

int main(void) {
	try {
		HRESULT hr = XgInitialize();
		if (FAILED(hr)) {
			printf("XgInitialize failed: 0x%08lX\n", hr);
			return hr;
		}

		XgUnitTest();
		MDict::unittest();
		XgHistogramUnitTest();
		XgBoardTextIOUnitTest();
		//XgPatFileUnitTest(); // 時間がかかる
		XgFlipBoardUnitTest();
		//XgAlgo1UnitTest();
		//XgAlgo2UnitTest();
		//XgAlgo3UnitTest();
		//XgAlgo4UnitTest();

		//MUILanguageDialog::UnitTest();
		//MNewDialog::UnitTest();
		//MJumpDialog::UnitTest();
		//MNotesDialog::UnitTest();
		//MTextBoxDialog::UnitTest();

		XgAlgo1UnitTest();
		XgAlgo2UnitTest();
		XgAlgo3UnitTest();
		XgAlgo4UnitTest();
		XgAlgo5UnitTest();
		//MCluesWnd::UnitTest();

		//MPatternDialog::UnitTest();
		//MRulePresetDialog::UnitTest();
		//Clues_UnitTest();

		XgUninitialize();
	} catch (const std::bad_alloc&) {
		XgUninitialize();
		return E_OUTOFMEMORY;
	}

	return S_OK;
}
