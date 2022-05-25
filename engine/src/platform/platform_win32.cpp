
#include "platform.h"

#if C3D_PLATFORM_WINDOWS
#include <Windows.h>
#include <cstdlib>

namespace C3D
{
	static f64 CLOCK_FREQUENCY = 0;
	static LARGE_INTEGER START_TIME;

	void ClockSetup() {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		CLOCK_FREQUENCY = 1.0 / static_cast<f64>(frequency.QuadPart);
		QueryPerformanceCounter(&START_TIME);
	}

	void* Platform::Allocate(const u64 size, bool aligned)
	{
		return malloc(size);
	}

	void Platform::Free(void* block, bool aligned)
	{
		free(block);
	}

	void* Platform::ZeroOutMemory(void* block, const u64 size)
	{
		return memset(block, 0, size);
	}

	void* Platform::CopyOverMemory(void* dest, const void* source, const u64 size)
	{
		return memcpy(dest, source, size);
	}

	void* Platform::SetMemory(void* dest, const i32 value, const u64 size)
	{
		return memset(dest, value, size);
	}

	f64 Platform::GetAbsoluteTime()
	{
		if (!CLOCK_FREQUENCY)
		{
			ClockSetup();
		}

		LARGE_INTEGER nowTime;
		QueryPerformanceCounter(&nowTime);
		return nowTime.QuadPart * CLOCK_FREQUENCY;
	}

	void Platform::Sleep(const u64 ms)
	{
		Sleep(ms);
	}
}

#endif