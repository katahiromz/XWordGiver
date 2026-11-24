// pat-file-io.cpp --- Pattern File Input/Output
// Author: katahiromz
// License: MIT

#include "xwg.h"

void __fastcall XgSortAndUniqueBoards(std::vector<MBoard>& boards, BOOL bExpand);
HRESULT __fastcall XgLoadPatFile(LPCWSTR pszFileName, std::vector<MBoard>& boards, INT nRules, MStringListW& comments);
HRESULT __fastcall XgSavePatFile(LPCWSTR pszFileName, std::vector<MBoard>& boards, MStringListW& comments);
BOOL __fastcall XgPatEdit(const MBoard& board, BOOL bAdd);
void __fastcall XgPatFileUnitTest(void);
