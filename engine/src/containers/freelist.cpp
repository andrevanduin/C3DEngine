
#include "freelist.h"

#include "core/logger.h"
#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	// The smallest allocation someone would make is of the size of a single pointer (should be 8 bytes on most platforms)
	constexpr auto FREELIST_SIZE_OF_SMALLEST_ALLOCATION = sizeof(void*);
	// The size of one of our FreeListNodes
	constexpr auto FREELIST_SIZE_OF_NODE = sizeof(FreeListNode);

	FreeList::FreeList()
		: m_totalSize(0), m_maxEntries(0), m_head(nullptr), m_nodes(nullptr)
	{}

	void FreeList::Create(const u64 totalSize, void* memory)
	{
		if (constexpr auto minSize = sizeof(FreeList) + FREELIST_SIZE_OF_NODE * 8; totalSize < minSize)
		{
			Logger::Warn("[FREELIST] - Using a freelist for very small amounts of memory is very inefficient; Consider using a different structure in this case");
		}

		
		// The maximum entries we could possibly have (assuming the smallest allocation would always be at least an 8 byte pointer)
		// NOTE: This is quite overkill
		m_maxEntries = totalSize / (FREELIST_SIZE_OF_SMALLEST_ALLOCATION * FREELIST_SIZE_OF_NODE);
		m_totalSize = totalSize;

		m_nodes = static_cast<FreeListNode*>(memory);

		m_head = &m_nodes[0];
		m_head->offset = 0;
		m_head->size = totalSize;
		m_head->next = nullptr;

		// Invalidate the size and offset for all nodes (except for the head) 
		for (u64 i = 1; i < m_maxEntries; i++)
		{
			m_nodes[i].offset = INVALID_ID;
			m_nodes[i].size = INVALID_ID;
		}
	}

	void FreeList::Destroy()
	{
		Memory.Zero(m_nodes, FREELIST_SIZE_OF_NODE * m_maxEntries);
		m_nodes = nullptr;
	}

	bool FreeList::AllocateBlock(const u64 size, u64* outOffset)
	{
		if (!outOffset) return false;

		FreeListNode* node = m_head;
		FreeListNode* prev = nullptr;
		while (node)
		{
			if (node->size == size)
			{
				// Exact match. Just return this node.
				*outOffset = node->offset;
				FreeListNode* nodeToReturn;
				if (prev)
				{
					// This node has a previous node
					prev->next = node->next;
					nodeToReturn = node;
				}
				else
				{
					// This node is the head of the list. Reassign the head and return previous
					nodeToReturn = m_head;
					m_head = node->next;
				}

				ReturnNode(nodeToReturn);
				return true;
			}
			if (node->size > size)
			{
				// Then node is larger than required. Deduct the memory from it and move the offset
				*outOffset = node->offset;
				node->size -= size;
				node->offset += size;
				return true;
			}

			prev = node;
			node = node->next;
		}

		Logger::Warn("[FREELIST] - FindBlock() failed, no block with enough free space found (requested {}B, available {}B)", size, FreeSpace());
		return false;
	}

	bool FreeList::FreeBlock(const u64 size, const u64 offset)
	{
		if (size == 0) return false;

		FreeListNode* node = m_head;
		FreeListNode* prev = nullptr;

		if (!node)
		{
			// Check the case where the entire freelist is allocated (which means we have no head node)
			// In this case a new node is needed at the head
			FreeListNode* newNode = GetNode();
			newNode->offset = offset;
			newNode->size = size;
			newNode->next = nullptr;
			m_head = newNode;
			return true;
		}

		while (node)
		{
			if (node->offset == offset)
			{
				// Current node can just be appended with the new free space
				node->size += size;

				// Check if this then connects the range between this node and the next node
				// if so, combine them and return the second node
				if (node->next && node->next->offset == node->offset + node->size)
				{
					node->size += node->next->size;
					FreeListNode* next = node->next;
					node->next = node->next->next;
					ReturnNode(next);
				}
				return true;
			}
			if (node->offset > offset)
			{
				// We iterated beyond the space to be freed. Thus we need a new node.
				FreeListNode* newNode = GetNode();
				newNode->offset = offset;
				newNode->size = size;

				if (prev)
				{
					// There is a previous node. The new node should be inserted between this node and the previous one
					prev->next = newNode;
					newNode->next = node;
				}
				else
				{
					// There is no previous node. The new node becomes the head
					newNode->next = node;
					m_head = newNode;
				}

				if (newNode->next && newNode->offset + newNode->size == newNode->next->offset)
				{
					// The new node connects to the next node
					newNode->size += newNode->next->size;
					FreeListNode* removeNode = newNode->next;
					newNode->next = removeNode->next;
					ReturnNode(removeNode);
				}

				if (prev && prev->offset + prev->size == newNode->offset)
				{
					// The previous node connects to the new node
					prev->size += newNode->size;
					FreeListNode* removeNode = newNode;
					prev->next = removeNode->next;
					ReturnNode(removeNode);
				}

				return true;
			}

			prev = node;
			node = node->next;
		}

		Logger::Warn("[FREELIST] - Unable to find block to be freed. Possibly corrupted memory?");
		return false;
	}

	void FreeList::Clear() const
	{
		// Invalidate all nodes except the head
		for (u64 i = 1; i < m_maxEntries; i++)
		{
			m_nodes[i].offset = INVALID_ID;
			m_nodes[i].size = INVALID_ID;
		}

		m_head->offset = 0;
		m_head->size = m_totalSize;
		m_head->next = nullptr;
	}

	u64 FreeList::FreeSpace() const
	{
		u64 total = 0;
		const FreeListNode* node = m_head;
		while (node)
		{
			total += node->size;
			node = node->next;
		}
		return total;
	}

	u64 FreeList::GetMemoryRequirements(const u64 totalSize)
	{
		const auto elementCount = totalSize / (FREELIST_SIZE_OF_SMALLEST_ALLOCATION * FREELIST_SIZE_OF_NODE);
		return elementCount * FREELIST_SIZE_OF_NODE;
	}

	FreeListNode* FreeList::GetNode() const
	{
		for (u64 i = 1; i < m_maxEntries; i++)
		{
			if (m_nodes[i].offset == INVALID_ID)
			{
				return &m_nodes[i];
			}
		}

		// No more nodes available
		Logger::Warn("No nodes available. Returning nullptr");
		return nullptr;
	}

	void FreeList::ReturnNode(FreeListNode* node)
	{
		node->offset = INVALID_ID;
		node->size = INVALID_ID;
		node->next = nullptr;
	}
}
