*** Begin Patch
*** Update File: XG_BoxWindow.hpp
@@
         case WM_CAPTURECHANGED:
         case WM_EXITSIZEMOVE:
             {
                 RECT rc;
                 GetWindowRect(hwnd, &rc);
@@
                 auto sa1 = std::make_shared<XG_UndoData_Boxes>();
                 sa1->Get();
                 int i1, j1, i2, j2;
                 XgSetCellPosition(rc.left, rc.top, i1, j1, FALSE);
                 XgSetCellPosition(rc.right, rc.bottom, i2, j2, TRUE);
                 if (SetPos(i1, j1, i2, j2)) {
                     auto sa2 = std::make_shared<XG_UndoData_Boxes>();
                     sa2->Get();
                     xg_ubUndoBuffer.Commit(UC_BOXES, sa1, sa2); // 元に戻す情報を設定。
                 }
                 // ボックスの位置を更新。
                 PostMessage(m_hwndParent, WM_COMMAND, ID_MOVEBOXES, 0);
                 // 表示を更新。
                 ::KillTimer(m_hWnd, 999);
                 ::SetTimer(m_hWnd, 999, 300, nullptr);
             }
             break;
*** End Patch
