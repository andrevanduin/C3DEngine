
#pragma once
#ifdef C3D_PLATFORM_LINUX

#include "containers/string.h"

namespace C3D
{
	struct LinuxHandleInfo
	{
		HINSTANCE hInstance;
		HWND hwnd;
	};

	struct LinuxFileWatch
	{
		u32 id;
		String filePath;
		FILETIME lastWriteTime;
	};
}
#endif