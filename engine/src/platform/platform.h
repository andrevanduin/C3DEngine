
#pragma once
#include "../core/defines.h"

#if C3D_PLATFORM_WINDOWS
#define SPRINTF(bufferSize, format, value)									\
	char buffer[bufferSize];												\
	if (sprintf_s(buffer, format, value) == -1)								\
	{																		\
		throw std::invalid_argument("Sprintf returned -1");					\
	}																		
#elif C3D_PLATFORM_LINUX
#define SPRINTF(bufferSize, format, value)									\
	char buffer[bufferSize];												\
	if (snprintf(buffer, format, value) == -1)								\
	{																		\
		throw std::invalid_argument("Sprintf returned -1");					\
	}
#endif

namespace C3D
{
	class C3D_API Platform
	{
	public:
		static void* Allocate(u64 size, bool aligned);

		static void Free(void* block, bool aligned);

		static void* Zero(void* block, u64 size);

		template <typename T>
		static T* Zero(T* pItem)
		{
			return static_cast<T*>(Zero(pItem, sizeof(T)));
		}

		static void* Copy(void* dest, const void* source, u64 size);

		static void* SetMemory(void* dest, i32 value, u64 size);

		static f64 GetAbsoluteTime();
		static void SleepMs(u64 ms);

		/* @brief Obtains the number of logical processor cores. */
		static i32 GetProcessorCount();

		/* @brief Obtains the current thread's id. */
		static u64 GetThreadId();
	};
}
