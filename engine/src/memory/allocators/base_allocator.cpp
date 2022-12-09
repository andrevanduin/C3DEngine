
#include "base_allocator.h"
#include "core/metrics/metrics.h"

namespace C3D
{
	BaseAllocator& BaseAllocator::SetFileAndLineRef(const char* file, const int line)
	{
		Metrics.SetFileAndLine(file, line);
		return *this;
	}

	BaseAllocator* BaseAllocator::SetFileAndLine(const char* file, const int line)
	{
		Metrics.SetFileAndLine(file, line);
		return this;
	}
}