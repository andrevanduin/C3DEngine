
#pragma once
#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	class NewFreeList
	{
		struct Node
		{
			u64 offset;
			u64 size;
			Node* next;

			void Invalidate()
			{
				offset = INVALID_ID_U64;
				size = 0;
				next = nullptr;
			}
		};

	public:
		NewFreeList();

		bool Create(void* memory, u64 memorySizeForNodes, u64 smallestPossibleAllocation, u64 managedSize);
		void Destroy();

		bool Resize(void* newMemory, u64 newSize, void** outOldMemory);

		bool AllocateBlock(u64 size, u64* outOffset);
		bool FreeBlock(u64 size, u64 offset);

		[[nodiscard]] u64 FreeSpace() const;

		/* @brief Checks if memory block of first is exactly adjacent to second. */
		static bool AreExactlyAdjacent(const Node* first, const Node* second);
		
		static constexpr u64 GetMemoryRequirement(u64 usableSize, u64 smallestPossibleAllocation);

	private:
		[[nodiscard]] Node* GetNode() const;

		LoggerInstance m_logger;

		Node* m_nodes;
		Node* m_head;

		/* @brief Amount of nodes this list holds. */
		u64 m_totalNodes;
		/* @brief The size of the memory block that holds the nodes. */
		u64 m_nodesSize;
		/* @brief The size of the smallest allocation a user could possibly make with this freelist. */
		u64 m_smallestPossibleAllocation;
		/* @brief The amount of memory that this freelist is used for. */
		u64 m_totalManagedSize;
	};

	constexpr u64 NewFreeList::GetMemoryRequirement(const u64 usableSize, const u64 smallestPossibleAllocation)
	{
		auto elementCount = usableSize / (smallestPossibleAllocation * sizeof(Node));
		if (elementCount < 20) elementCount = 20;
		return elementCount * sizeof(Node);
	}
}

