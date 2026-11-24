// EnvVars.cpp --- 環境変数を同期する
// Author: katahiromz
// License: MIT

#pragma once

BOOL InitEnvironmentTracking();
BOOL RefreshEnvironment(BOOL overwriteAppChanges = FALSE);
void ClearEnvironment();
