
#include "global_memory_system.h"

#include "platform/platform.h"

namespace C3D
{
	void* GlobalMemorySystem::m_memoryBlock;

	DynamicAllocator* GlobalMemorySystem::s_globalAllocator;
	// We keep this allocator immediately statically loaded so we can use it for the creation
	// of other statically initialized systems in our engine since the global allocator
	// will not yet have been initialized at that point.
	MallocAllocator* GlobalMemorySystem::s_mallocAllocator;

	// Default linear stack allocator
	LinearAllocator* GlobalMemorySystem::s_linearAllocator;

	// Default Stack allocator that should never really be used but it is required to
	// ensure that the compiler will always find a GetDefaultAllocator function
	StackAllocator<KibiBytes(8)>* GlobalMemorySystem::s_stackAllocator;

	void GlobalMemorySystem::Init(const MemorySystemConfig& config)
	{
		if (s_globalAllocator)
		{
			Logger::Fatal("[GLOBAL_MEMORY_SYSTEM] - Init() has already been called.");
		}

		const u64 memoryRequirement = DynamicAllocator::GetMemoryRequirements(config.totalAllocSize);

		m_memoryBlock = Platform::Allocate(memoryRequirement, false);
		if (!m_memoryBlock)
		{
			Logger::Fatal("[GLOBAL_MEMORY_SYSTEM] - Allocating memory pool failed");
		}

		s_globalAllocator = new DynamicAllocator(AllocatorType::GlobalDynamic);
		s_globalAllocator->Create(m_memoryBlock, memoryRequirement, config.totalAllocSize);

		s_linearAllocator = new LinearAllocator();
		s_linearAllocator->Create("DefaultLinearAllocator", KibiBytes(8));

		s_stackAllocator = new StackAllocator<KibiBytes(8)>();
		s_stackAllocator->Create("DefaultStackAllocator");

		Logger::Info("[GLOBAL_MEMORY_SYSTEM] - Initialized successfully");
	}

	void GlobalMemorySystem::Destroy()
	{
		if (s_stackAllocator)
		{
			s_stackAllocator->Destroy();
			delete s_stackAllocator;
		}

		if (s_linearAllocator)
		{
			s_linearAllocator->Destroy();
			delete s_linearAllocator;
		}

		if (s_globalAllocator)
		{
			s_globalAllocator->Destroy();
			Platform::Free(m_memoryBlock, false);
		}
	}

	DynamicAllocator& GlobalMemorySystem::GetAllocator()
	{
		return *s_globalAllocator;
	}
}
