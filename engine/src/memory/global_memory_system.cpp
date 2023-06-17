
#include "global_memory_system.h"

#include "platform/platform.h"

#include "allocators/linear_allocator.h"
#include "allocators/stack_allocator.h"

namespace C3D
{
	void GlobalMemorySystem::Init(const MemorySystemConfig& config)
	{
		const u64 memoryRequirement = DynamicAllocator::GetMemoryRequirements(config.totalAllocSize);
		const auto memoryBlock = GetMemoryBlock(memoryRequirement);
		if (!memoryBlock)
		{
			Logger::Fatal("[GLOBAL_MEMORY_SYSTEM] - Allocating memory pool failed");
		}

		const auto globalAllocator = BaseAllocator<DynamicAllocator>::GetDefault();
		globalAllocator->Create(memoryBlock, memoryRequirement, config.totalAllocSize);

		const auto linearAllocator = BaseAllocator<LinearAllocator>::GetDefault();
		linearAllocator->Create("DefaultLinearAllocator", KibiBytes(8));

		const auto stackAllocator = BaseAllocator<StackAllocator<KibiBytes(8)>>::GetDefault();
		stackAllocator->Create("DefaultStackAllocator");

		Logger::Info("[GLOBAL_MEMORY_SYSTEM] - Initialized successfully");
	}

	void GlobalMemorySystem::Destroy()
	{
		if (const auto stackAllocator = BaseAllocator<StackAllocator<KibiBytes(8)>>::GetDefault())
		{
			stackAllocator->Destroy();
			delete stackAllocator;
		}

		if (const auto linearAllocator = BaseAllocator<LinearAllocator>::GetDefault())
		{
			linearAllocator->Destroy();
			delete linearAllocator;
		}

		if (const auto globalAllocator = BaseAllocator<DynamicAllocator>::GetDefault())
		{
			globalAllocator->Destroy();
			delete globalAllocator;
		}
	}

	void* GlobalMemorySystem::Zero(void* block, const u64 size)
	{
		return std::memset(block, 0, size);
	}

	void* GlobalMemorySystem::MemCopy(void* dest, const void* source, const u64 size)
	{
		return std::memcpy(dest, source, size);
	}

	void* GlobalMemorySystem::SetMemory(void* dest, const i32 value, const u64 size)
	{
		return std::memset(dest, value, size);
	}

	DynamicAllocator& GlobalMemorySystem::GetAllocator()
	{
		return *BaseAllocator<DynamicAllocator>::GetDefault();
	}

	void* GlobalMemorySystem::GetMemoryBlock(const u64 memoryRequirement)
	{
		static void* memoryBlock = std::malloc(memoryRequirement);
		return memoryBlock;
	}
}
