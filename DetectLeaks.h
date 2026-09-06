// DetectLeaks.h --- Detect memory leaks
// Author: katahiromz
// License: MIT

#pragma once

#if defined(_MSC_VER) && !defined(NDEBUG) && !defined(_CRTDBG_MAP_ALLOC)
	// for detecting memory leak (MSVC only)
	#define _CRTDBG_MAP_ALLOC
	#include <crtdbg.h>
#endif
