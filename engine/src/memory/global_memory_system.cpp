
#include "global_memory_system.h"

#include "platform/platform.h"

#include "allocators/linear_allocator.h"
#include "allocators/malloc_allocator.h"
#include "allocators/stack_allocator.h"

namespace C3D
{
	template <>
	C3D_API inline DynamicAllocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		static auto globalAllocator = new DynamicAllocator(AllocatorType::GlobalDynamic);
		return globalAllocator;
	}

	template <>
	C3D_API inline MallocAllocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		static auto mallocAllocator = new MallocAllocator();
		return mallocAllocator;
	}

	template <>
	C3D_API inline StackAllocator<KibiBytes(8)>* GlobalMemorySystem::GetDefaultAllocator()
	{
		static auto stackAllocator = new StackAllocator<KibiBytes(8)>();
		return stackAllocator;
	}

	template<>
	C3D_API inline LinearAllocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		static auto linearAllocator = new LinearAllocator();
		return linearAllocator;
	}

	void GlobalMemorySystem::Init(const MemorySystemConfig& config)
	{
		const u64 memoryRequirement = DynamicAllocator::GetMemoryRequirements(config.totalAllocSize);
		const auto memoryBlock = GetMemoryBlock(memoryRequirement);
		if (!memoryBlock)
		{
			Logger::Fatal("[GLOBAL_MEMORY_SYSTEM] - Allocating memory pool failed");
		}

		const auto globalAllocator = GetDefaultAllocator<DynamicAllocator>();
		globalAllocator->Create(memoryBlock, memoryRequirement, config.totalAllocSize);

		const auto linearAllocator = GetDefaultAllocator<LinearAllocator>();
		linearAllocator->Create("DefaultLinearAllocator", KibiBytes(8));

		const auto stackAllocator = GetDefaultAllocator<StackAllocator<KibiBytes(8)>>();
		stackAllocator->Create("DefaultStackAllocator");

		Logger::Info("[GLOBAL_MEMORY_SYSTEM] - Initialized successfully");
	}

	void GlobalMemorySystem::Destroy()
	{
		if (const auto stackAllocator = GetDefaultAllocator<StackAllocator<KibiBytes(8)>>())
		{
			stackAllocator->Destroy();
			delete stackAllocator;
		}

		if (const auto linearAllocator = GetDefaultAllocator<LinearAllocator>())
		{
			linearAllocator->Destroy();
			delete linearAllocator;
		}

		if (const auto globalAllocator = GetDefaultAllocator<DynamicAllocator>())
		{
			globalAllocator->Destroy();
			Platform::Free(GetMemoryBlock(), false);
		}
	}

	DynamicAllocator& GlobalMemorySystem::GetAllocator()
	{
		return *GetDefaultAllocator<DynamicAllocator>();
	}

	void* GlobalMemorySystem::GetMemoryBlock(const u64 memoryRequirement)
	{
		static void* memoryBlock = Platform::Allocate(memoryRequirement, false);
		return memoryBlock;
	}
}
