
#pragma once
#include "containers/freelist.h"
#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	class C3D_API DynamicAllocator
	{
	public:
		DynamicAllocator();

		bool Create(u64 usableSize, void* memory);

		bool Destroy();

		void* Allocate(u64 size);

		bool Free(void* block, u64 size);

		[[nodiscard]] u64 FreeSpace() const;

		static constexpr u64 GetMemoryRequirements(u64 totalSize);
	private:
		LoggerInstance m_logger;

		bool m_initialized;
		// The total size that can be allocated
		u64 m_totalSize;
		// The freelist to keep track of all the free blocks of memory
		FreeList m_freeList;
		// A pointer to the actual block of memory that this allocator manages
		void* m_memory;
	};

	constexpr u64 DynamicAllocator::GetMemoryRequirements(const u64 totalSize)
	{
		return FreeList::GetMemoryRequirements(totalSize) + totalSize;
	}
}
