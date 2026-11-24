# SaveBitmapToFile

[![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/eubpjk9esdbqhyh9?svg=true)](https://ci.appveyor.com/project/katahiromz/savebitmaptofile)

`LoadBitmapFromFile` function loads a bitmap from file in Win32 C/C++.

`SaveBitmapToFile` function saves a bitmap to file in Win32 C/C++.

```c
#include "SaveBitmapToFile.h"
HBITMAP WINAPI LoadBitmapFromFile(LPCTSTR bmp_file);
BOOL WINAPI SaveBitmapToFile(LPCTSTR bmp_file, HBITMAP hbm);
```

Don't forget to append the filename extension.

License: MIT
