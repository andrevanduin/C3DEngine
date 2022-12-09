
#pragma once
#include "core/defines.h"
#include "allocators/dynamic_allocator.h"
#include "allocators/linear_allocator.h"
#include "allocators/malloc_allocator.h"
#include "allocators/stack_allocator.h"

namespace C3D
{
#define Memory C3D::GlobalMemorySystem::GetAllocator().SetFileAndLineRef(__FILE__, __LINE__)

#ifdef C3D_MEMORY_METRICS_FILE_LINE
	#define Allocator(pAlloc) pAlloc->SetFileAndLine(__FILE__, __LINE__)
#else
	#define Allocator(pAlloc) pAlloc
#endif



	struct MemorySystemConfig
	{
		u64 totalAllocSize = 0;
		bool excludeFromStats = false;
	};

	class C3D_API GlobalMemorySystem
	{
		static DynamicAllocator* s_globalAllocator;
		static MallocAllocator* s_mallocAllocator;
		static LinearAllocator* s_linearAllocator;
		static StackAllocator<KibiBytes(8)>* s_stackAllocator;

	public:

		static void Init(const MemorySystemConfig& config);
		static void Destroy();

		template <class Allocator>
		static Allocator* GetDefaultAllocator();

		static DynamicAllocator& GetAllocator();
	private:
		static void* m_memoryBlock;
	};

	template <class Allocator>
	Allocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		C3D_ASSERT_MSG(false, "No default allocator assigned for this type");
		return nullptr;
	}

	template <>
	inline DynamicAllocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		return s_globalAllocator;
	}

	template <>
	inline MallocAllocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		if (!s_mallocAllocator)
		{
			s_mallocAllocator = new MallocAllocator();
		}
		return s_mallocAllocator;
	}

	template <>
	inline StackAllocator<KibiBytes(8)>* GlobalMemorySystem::GetDefaultAllocator()
	{
		return s_stackAllocator;
	}

	template<>
	inline LinearAllocator* GlobalMemorySystem::GetDefaultAllocator()
	{
		return s_linearAllocator;
	}
}
