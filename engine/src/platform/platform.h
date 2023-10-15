
#pragma once
#ifdef C3D_PLATFORM_WINDOWS
    #include "windows/platform_win32.h"
#elif defined(C3D_PLATFORM_LINUX)
    #include "linux/platform_linux.h"
#endif