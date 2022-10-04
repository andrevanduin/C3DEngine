
#pragma once
#include "../core/defines.h"

namespace C3D
{
	class Platform
	{
	public:
		static void* Allocate(u64 size, bool aligned);
		static void Free(void* block, bool aligned);
		static void* ZeroOutMemory(void* block, u64 size);
		static void* CopyOverMemory(void* dest, const void* source, u64 size);
		static void* SetMemory(void* dest, i32 value, u64 size);

		static f64 GetAbsoluteTime();
		static void SleepMs(u64 ms);

		/* @brief Obtains the number of logical processor cores. */
		static i32 GetProcessorCount();

		/* @brief Obtains the current thread's id. */
		static u64 GetThreadId();
	};
}
