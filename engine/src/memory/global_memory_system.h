
#pragma once
#include "core/defines.h"
#include "allocators/dynamic_allocator.h"

namespace C3D
{
#ifdef C3D_MEMORY_METRICS_STACKTRACE
	#define Allocator(pAlloc) pAlloc->SetStacktrace()
	#define Memory C3D::GlobalMemorySystem::GetAllocator().SetStacktraceRef()
#else
	#define Allocator(pAlloc) pAlloc
	#define Memory C3D::GlobalMemorySystem::GetAllocator()
#endif
	
	struct MemorySystemConfig
	{
		u64 totalAllocSize = 0;
		bool excludeFromStats = false;
	};

	class C3D_API GlobalMemorySystem
	{
	public:
		template <class Allocator>
		static Allocator* GetDefaultAllocator();

		static void Init(const MemorySystemConfig& config);
		static void Destroy();

		static DynamicAllocator& GetAllocator();
	private:
		static void* GetMemoryBlock(u64 memoryRequirement = 0);
	};
}
