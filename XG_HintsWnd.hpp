*** Begin Patch
*** Update File: XG_HintsWnd.hpp
@@
     static bool UpdateHintData(void)
     {
         bool updated = false;
         if (::IsWindow(xg_hHintsWnd)) {
             WCHAR sz[512];
@@
             for (size_t i = 0; i < xg_vecHorzHints.size(); ++i) {
                 if (::SendMessageW(xg_ahwndHorzEdits[i], EM_GETMODIFY, 0, 0)) {
                     updated = true;
                     ::GetWindowTextW(xg_ahwndHorzEdits[i], sz, 
                                      static_cast<int>(_countof(sz)));
                     xg_vecHorzHints[i].m_strHint = sz;
-                    ::SendMessageW(xg_ahwndVertEdits[i], EM_SETMODIFY, FALSE, 0);
+                    ::SendMessageW(xg_ahwndHorzEdits[i], EM_SETMODIFY, FALSE, 0);
                 }
             }
         }
         if (updated)
             XG_FILE_MODIFIED(TRUE);
@@
         case WM_CHAR:
             if (wParam == L'\r' || wParam == L'\n') {
                 // 改行が押された。必要ならばデータを更新する。
                 if (AreHintsModified()) {
                     auto hu1 = std::make_shared<XG_UndoData_HintsUpdated>();
                     auto hu2 = std::make_shared<XG_UndoData_HintsUpdated>();
                     hu1->Get();
                     {
                         // ヒントを更新する。
                         UpdateHintData();
                     }
                     hu2->Get();
                     xg_ubUndoBuffer.Commit(UC_HINTS_UPDATED, hu1, hu2);
                 }
             }
+            if (wParam == L'\r' || wParam == L'\n') {
+                return 0;
+            }
             return ::CallWindowProc(data->m_fnOldWndProc, hwnd, uMsg, wParam, lParam);
*** End Patch
