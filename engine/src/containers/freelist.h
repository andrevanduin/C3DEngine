
#pragma once
#include "core/defines.h"

namespace C3D
{
	struct FreeListNode
	{
		u64 offset;
		u64 size;
		FreeListNode* next;
	};

	// The smallest allocation someone would make is of the size of a single pointer (should be 8 bytes on most platforms)
	constexpr auto FREELIST_SIZE_OF_SMALLEST_ALLOCATION = sizeof(void*);
	// The size of one of our FreeListNodes
	constexpr auto FREELIST_SIZE_OF_NODE = sizeof(FreeListNode);

	class C3D_API FreeList
	{
	public:
		FreeList();

		bool Create(u64 useableMemory, void* memory);

		bool Resize(void* newMemory, u64 newSize, void** outOldMemory);

		void Destroy();

		bool AllocateBlock(u64 size, u64* outOffset);

		bool FreeBlock(u64 size, u64 offset);

		void Clear() const;

		[[nodiscard]] u64 FreeSpace() const;

		static constexpr u64 GetMemoryRequirements(u64 totalSize);
	private:
		[[nodiscard]] FreeListNode* GetNode() const;

		static void ReturnNode(FreeListNode* node);

		// The size of this structure in bytes
		u64 m_size;
		// The max number of entries that can be contained in this structure
		u64 m_maxEntries;

		// The first free node
		FreeListNode* m_head;
		// List of all the nodes
		FreeListNode* m_nodes;
	};

	constexpr u64 FreeList::GetMemoryRequirements(const u64 totalSize)
	{
		auto elementCount = totalSize / (FREELIST_SIZE_OF_SMALLEST_ALLOCATION * FREELIST_SIZE_OF_NODE);
		if (elementCount < 20) elementCount = 20;
		return elementCount * FREELIST_SIZE_OF_NODE;
	}
}
