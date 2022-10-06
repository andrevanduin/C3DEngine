
#pragma once
#include "free_list.h"
#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	typedef u32 AllocSizeMarker;

	struct AllocHeader
	{
		void* start;
		u16 alignment;
	};

	constexpr auto MAX_SINGLE_ALLOC_SIZE = GibiBytes(4);
	constexpr auto SMALLEST_POSSIBLE_ALLOCATION = sizeof(AllocHeader) + sizeof(AllocSizeMarker) + 1 + 1;
		
	class C3D_API DynamicAllocator
	{
	public:
		DynamicAllocator();

		bool Create(void* memory, u64 totalMemory, u64 usableMemory);

		bool Destroy();

		void* Allocate(u64 size);
		void* AllocateAligned(u64 size, u16 alignment);

		bool Free(void* block, u64 size);
		bool FreeAligned(void* block);

		/* @brief Obtains the size and alignment of a given block of memory.
		 * Will fail if the provided block of memory is invalid.
		 */
		static bool GetSizeAlignment(void* block, u64* outSize, u16* outAlignment);

		/* @brief Obtains the alignment of a given block of memory.
		 * Will fail if the provided block of memory is invalid.
		 */
		static bool GetAlignment(void* block, u16* outAlignment);
		static bool GetAlignment(const void* block, u16* outAlignment);

		[[nodiscard]] u64 FreeSpace() const;
		[[nodiscard]] u64 GetTotalUsableSize() const;

		static constexpr u64 GetMemoryRequirements(u64 usableSize);
	private:
		LoggerInstance m_logger;

		bool m_initialized;
		// The total size including our freelist
		u64 m_totalSize;
		// The size of usable memory
		u64 m_memorySize;
		// The freelist to keep track of all the free blocks of memory
		NewFreeList m_freeList;
		// A pointer to the actual block of memory that this allocator manages
		char* m_memory;
	};

	constexpr u64 DynamicAllocator::GetMemoryRequirements(const u64 usableSize)
	{
		return NewFreeList::GetMemoryRequirement(usableSize, SMALLEST_POSSIBLE_ALLOCATION) + usableSize;
	}
}
