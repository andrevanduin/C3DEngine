
#include "linear_allocator.h"

#include "core/memory.h"
#include "core/logger.h"
#include "services/services.h"

namespace C3D
{
	void LinearAllocator::Create(const u64 totalSize, void* memory)
	{
		m_totalSize = totalSize;
		m_allocated = 0;
		m_ownsMemory = memory == nullptr;

		// The memory already exists is thus owned by someone else
		if (memory) m_memory = memory;
		else
		{
			// We need to allocate the memory ourselves
			m_memory = Memory::Allocate(totalSize, MemoryType::LinearAllocator);
		}
	}

	void LinearAllocator::Destroy()
	{
		m_allocated = 0;
		if (m_ownsMemory && m_memory)
		{
			// We own the memory so let's free it
			Memory::Free(m_memory, m_totalSize, MemoryType::LinearAllocator);
		}

		m_memory = nullptr;
		m_totalSize = 0;
		m_ownsMemory = false;
	}

	void* LinearAllocator::Allocate(const u64 size)
	{
		if (m_memory)
		{
			if (m_allocated + size > m_totalSize)
			{
				u64 remaining = m_totalSize - m_allocated;
				Logger::PrefixError("LINEAR_ALLOCATOR", "Tried to allocate {}B, but there are only {}B remaining", size, remaining);
				return nullptr;
			}

			void* block = static_cast<u8*>(m_memory) + m_allocated;
			m_allocated += size;
			return block;
		}

		Logger::PrefixError("LINEAR_ALLOCATOR", "Not initialized");
		return nullptr;
	}

	void LinearAllocator::FreeAll()
	{
		if (m_memory)
		{
			m_allocated = 0;
			Memory::Zero(m_memory, m_totalSize);
		}
	}
}
