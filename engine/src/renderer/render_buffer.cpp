
#include "render_buffer.h"

#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	RenderBuffer::RenderBuffer(const RenderBufferType type, const u64 totalSize)
		: type(type), totalSize(totalSize), m_freeListMemoryRequirement(0), m_freeListBlock(nullptr)
	{}

	bool RenderBuffer::Create(const bool useFreelist)
	{
		if (useFreelist)
		{
			// Get the memory requirements for our freelist
			// TODO: Setting smallestPossibleAllocation here to 8 could cause problems potentially.
			m_freeListMemoryRequirement = NewFreeList::GetMemoryRequirement(totalSize, 8);
			// Allocate enough space for our freelist
			void* memory = Memory.Allocate(m_freeListMemoryRequirement, MemoryType::RenderSystem);
			// Create the freelist
			m_freeList.Create(memory, m_freeListMemoryRequirement, 8, totalSize);
		}
		return true;
	}

	void RenderBuffer::Destroy()
	{
		if (m_freeListMemoryRequirement > 0)
		{
			// We are using a freelist
			m_freeList.Destroy();
			Memory.Free(m_freeListBlock, m_freeListMemoryRequirement, MemoryType::RenderSystem);
			m_freeListMemoryRequirement = 0;
		}
	}
}
