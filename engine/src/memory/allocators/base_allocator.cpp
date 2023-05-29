
#include "base_allocator.h"
#include "core/metrics/metrics.h"

namespace C3D
{
	BaseAllocator& BaseAllocator::SetStacktraceRef()
	{
		Metrics.SetStacktrace();
		return *this;
	}

	BaseAllocator* BaseAllocator::SetStacktrace()
	{
		Metrics.SetStacktrace();
		return this;
	}
}