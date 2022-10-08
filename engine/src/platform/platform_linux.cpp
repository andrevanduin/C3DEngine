
#include "platform.h"

#if C3D_PLATFORM_LINUX

#if _POSIX_C_SOURCE >= 199309L
#include <time.h>  // nanosleep
#else
#include <unistd.h>  // usleep
#endif

#include <pthread.h>
#include <errno.h>        // For error reporting
#include <sys/sysinfo.h>  // Processor info

namespace C3D
{
	static f64 CLOCK_FREQUENCY = 0.0;
	static LARGE_INTEGER START_TIME;

	void* Platform::Allocate(const u64 size, bool aligned)
	{
		return std::malloc(size);
	}

	void Platform::Free(void* block, bool aligned)
	{
		std::free(block);
	}

	void* Platform::ZeroOutMemory(void* block, const u64 size)
	{
		return std::memset(block, 0, size);
	}

	void* Platform::CopyOverMemory(void* dest, const void* source, const u64 size)
	{
		return std::memcpy(dest, source, size);
	}

	void* Platform::SetMemory(void* dest, const i32 value, const u64 size)
	{
		return std::memset(dest, value, size);
	}

	f64 Platform::GetAbsoluteTime()
	{
		timespec now;
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);
		return now.tv_sec + now.tv_nsec * 0.000000001;
	}

	void Platform::SleepMs(const u64 ms)
	{
#if _POSIX_C_SOURCE >= 199309L
		timespec ts;
		ts.tv_sec = ms / 1000;
		ts.tv_nsec = (ms % 1000) * 1000 * 1000;
		nanosleep(&ts, 0);
#else
		if (ms >= 1000)
		{
			sleep(ms / 1000);
		}
		usleep((ms % 1000) * 1000);
#endif
	}

	i32 Platform::GetProcessorCount()
	{
		auto processorCount = get_nprocs_conf();
		auto processorsAvailable = get_nprocs();
		return processorsAvailable;
	}

	u64 Platform::GetThreadId()
	{
		return static_cast<u64>(pthread_self());
	}
}

#endif