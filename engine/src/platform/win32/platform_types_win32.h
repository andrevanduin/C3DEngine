
#pragma once
#ifdef C3D_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN

// Undef C3D Engine defines that cause issues with Windows.h
#undef Resources
#undef Event

#include <Windows.h>

// Undef Windows macros that cause issues with C3D Engine
#undef CopyFile
#undef max
#undef min

// Redefine the C3D Engine macros again for further use
#define Resources	m_engine->GetSystem<C3D::ResourceSystem>(C3D::SystemType::ResourceSystemType)
#define Event		m_engine->GetSystem<C3D::EventSystem>(C3D::SystemType::EventSystemType)

#include "containers/string.h"

namespace C3D
{
	struct Win32HandleInfo
	{
		HINSTANCE hInstance;
		HWND hwnd;
	};

	struct Win32FileWatch
	{
		u32 id;
		String filePath;
		FILETIME lastWriteTime;
	};
}
#endif