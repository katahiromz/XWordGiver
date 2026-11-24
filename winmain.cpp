// winmain.cpp --- クロスワード ギバー。

#include "xwg.h"
#include "gui/MMainWnd.h"

INT XWordGiver_Main(HINSTANCE hInstance, INT argc, LPWSTR* wargv) {
	HRESULT hr = XgInitialize();
	if (FAILED(hr)) {
		MessageBoxA(nullptr, "XgInitialize failed.", nullptr, MB_ICONERROR);
		return FALSE;
	}
	struct MAutoXgUninitialize {
		~MAutoXgUninitialize() {
			XgUninitialize();
		}
	};
	MAutoXgUninitialize auto_uninit;

	MMainWnd hwnd(argc, wargv);
	if (!hwnd.InitInstance(hInstance))
		return 1;

	return hwnd.Run();
}

INT WINAPI
WinMain(HINSTANCE   hInstance,
        HINSTANCE   hPrevInstance,
        LPSTR       lpCmdLine,
        INT         nCmdShow)
{
	INT argc;
	LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	INT ret = XWordGiver_Main(hInstance, argc, wargv);
	LocalFree(wargv);
	return ret;
}
