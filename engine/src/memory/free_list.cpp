
#include "free_list.h"

#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	NewFreeList::NewFreeList()
		: m_logger("FREE_LIST"), m_nodes(nullptr), m_head(nullptr), m_totalNodes(0), m_nodesSize(0), m_totalManagedSize(0)
	{}

	bool NewFreeList::Create(void* memory, const u64 memorySizeForNodes, const u64 smallestPossibleAllocation, const u64 managedSize)
	{
		m_totalNodes = memorySizeForNodes / smallestPossibleAllocation;
		m_nodesSize = memorySizeForNodes;
		m_totalManagedSize = managedSize;

		m_nodes = static_cast<Node*>(memory);
		Memory.Zero(m_nodes, m_nodesSize);

		// Set all our nodes to invalid
		for (u64 i = 0; i < m_totalNodes; i++)
		{
			m_nodes[i].size = 0;
			m_nodes[i].next = nullptr;
			m_nodes[i].offset = INVALID_ID_U64;
		}

		// Set our head as the first node with all the memory available
		m_head = &m_nodes[0];
		m_head->size = m_totalManagedSize;
		m_head->offset = 0;

		m_logger.Trace("Create() - Created FreeList with {} nodes to manage {} bytes of memory.", m_totalNodes, m_totalManagedSize);
		return true;
	}

	void NewFreeList::Destroy()
	{
		// Zero out our nodes
		Memory.Zero(m_nodes, m_nodesSize);
		m_nodes = nullptr;
		// NOTE: We can't free our memory since the user of this class is responsible for the memory we are using.
	}

	bool NewFreeList::AllocateBlock(const u64 size, u64* outOffset)
	{
		// Keep track of our current and previous node
		Node* prev = nullptr;
		// We start at the head of our free_list
		Node* current = m_head;
		
		while (current)
		{
			if (current->size == size)
			{
				// We have an exact size match so our allocation fits nicely into this node
				*outOffset = current->offset;
				
				if (prev)
				{
					// We have a previous so we can simply point it to the next node 
					prev->next = current->next;
				}
				else if (current->next)
				{
					// There is no previous node so our next node needs to become the head
					m_head = current->next;
				}
				else
				{
					// There is no next node either. This shouldn't happen
					m_logger.Fatal("AllocateBlock() - No previous or next node!");
				}

				// Invalidate this node since it is no longer needed
				current->Invalidate();
				return true;
			}

			if (current->size > size)
			{
				// We have more space than is required for our allocation
				*outOffset = current->offset;

				current->size -= size;
				current->offset += size;
				return true;
			}

			prev = current;
			current = current->next;
		}

		m_logger.Error("AllocateBlock() - Failed to find a node with enough space for the allocation.");
		throw std::bad_alloc();
	}

	bool NewFreeList::FreeBlock(const u64 size, const u64 offset)
	{
#ifdef _DEBUG
		if (size == 0)
		{
			m_logger.Error("FreeBlock() - Called with size = 0.");
			return false;
		}
#endif

		Node* prev = nullptr;
		Node* current = m_head;

		// Check the case where the entire freelist is allocated (which means we have no head node)
		if (!current)
		{
			// In this case a new node is needed at the head
			const auto newHead = GetNode();
			newHead->offset = offset;
			newHead->size = size;
			newHead->next = nullptr;
			m_head = newHead;
			return true;
		}

		while (current)
		{
			if (current->offset > offset)
			{
				// Our current node's offset is further into the memory block than where we want to free
				const auto newNode = GetNode();
				newNode->next = current;
				newNode->offset = offset;
				newNode->size = size;

				if (prev)
				{
					// We have a previous node so we insert our new node in between prev and current
					prev->next = newNode;
				}
				else
				{
					// We have no previous node so our new node becomes the new head
					m_head = newNode;
				}

				// If our new node is exactly adjacent to the next
				if (newNode->next && AreExactlyAdjacent(newNode, newNode->next))
				{
					const auto next = newNode->next;

					// We add the size of the next node to our new node
					newNode->size += next->size;
					// And connect our new node to the next node's next node
					newNode->next = next->next;
					// Then we invalidate the next node
					next->Invalidate();
				}

				// If our previous node is exactly adjacent to our new node
				if (prev && AreExactlyAdjacent(prev, newNode))
				{
					// We add the size of our new node to our prev node
					prev->size += newNode->size;
					// And connect our prev node to our new node's next node
					prev->next = newNode->next;
					// Then we invalidate our new node
					newNode->Invalidate();
				}

				return true;
			}

			// If our current node's offset is smaller then the provided offset we simply move to the next node
			prev = current;
			current = current->next;
		}

		m_logger.Fatal("FreeBlock() - Failed. No node was found");
		return false;
	}

	u64 NewFreeList::FreeSpace() const
	{
		u64 free = 0;
		const Node* current = m_head;
		while (current)
		{
			free += current->size;
			current = current->next;
		}
		return free;
	}

	bool NewFreeList::AreExactlyAdjacent(const Node* first, const Node* second)
	{
		return first->offset + first->size == second->offset;
	}

	NewFreeList::Node* NewFreeList::GetNode() const
	{
		for (u64 i = 0; i < m_totalNodes; i++)
		{
			if (m_nodes[i].offset == INVALID_ID_U64)
			{
				return &m_nodes[i];
			}
		}
		m_logger.Fatal("GetNode() - Unable to get a valid node");
		return nullptr;
	}
}
