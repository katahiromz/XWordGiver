// EnvVars.cpp --- 環境変数を同期する
// Author: katahiromz
// License: MIT

#pragma once

bool InitEnvironmentTracking();
bool RefreshEnvironment(bool overwriteAppChanges = false);
void ClearEnvironment();
