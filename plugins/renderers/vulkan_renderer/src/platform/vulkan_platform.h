
#pragma once
#ifdef C3D_PLATFORM_WINDOWS
#include "windows/vulkan_platform_win32.h"
#elif defined(C3D_PLATFORM_LINUX)
#include "linux/vulkan_platform_linux.h"
#endif