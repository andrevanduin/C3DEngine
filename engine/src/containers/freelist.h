
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

	class C3D_API FreeList
	{
	public:
		FreeList();

		bool Create(u64 totalSize, void* memory);

		bool Resize(void* newMemory, u64 newSize, void** outOldMemory);

		void Destroy();

		bool AllocateBlock(u64 size, u64* outOffset);

		bool FreeBlock(u64 size, u64 offset);

		void Clear() const;

		[[nodiscard]] u64 FreeSpace() const;

		static u64 GetMemoryRequirements(u64 totalSize);
	private:
		[[nodiscard]] FreeListNode* GetNode() const;

		static void ReturnNode(FreeListNode* node);

		// The total size of this structure in bytes
		u64 m_totalSize;
		// The max number of entries that can be contained in this structure
		u64 m_maxEntries;

		// The first free node
		FreeListNode* m_head;
		// List of all the nodes
		FreeListNode* m_nodes;
	};
}