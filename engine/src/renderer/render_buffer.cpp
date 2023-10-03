
#include "render_buffer.h"

#include "memory/global_memory_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    // TODO: Setting smallestPossibleAllocation here to 8 could cause problems potentially.
    constexpr static auto SMALLEST_POSSIBLE_FREELIST_ALLOCATION = 8;

    RenderBuffer::RenderBuffer(const String& name) : m_logger(name.Data()), m_name(name) {}

    bool RenderBuffer::Create(const RenderBufferType bufferType, const u64 size, const bool useFreelist)
    {
        type      = bufferType;
        totalSize = size;

        if (useFreelist)
        {
            // Get the memory requirements for our freelist
            m_freeListMemoryRequirement =
                FreeList::GetMemoryRequirement(totalSize, SMALLEST_POSSIBLE_FREELIST_ALLOCATION);
            // Allocate enough space for our freelist
            m_freeListBlock = Memory.AllocateBlock(MemoryType::RenderSystem, m_freeListMemoryRequirement);
            // Create the freelist
            m_freeList.Create(m_freeListBlock, m_freeListMemoryRequirement, SMALLEST_POSSIBLE_FREELIST_ALLOCATION,
                              totalSize);
        }
        return true;
    }

    void RenderBuffer::Destroy()
    {
        if (m_freeListMemoryRequirement > 0)
        {
            // We are using a freelist
            m_freeList.Destroy();
            Memory.Free(MemoryType::RenderSystem, m_freeListBlock);
            m_freeListMemoryRequirement = 0;
        }
    }

    bool RenderBuffer::Bind(u64 offset) { return true; }

    bool RenderBuffer::Unbind() { return true; }

    void* RenderBuffer::MapMemory(u64 offset, u64 size) { return nullptr; }

    void RenderBuffer::UnMapMemory(u64 offset, u64 size) {}

    bool RenderBuffer::Flush(u64 offset, u64 size) { return true; }

    bool RenderBuffer::Resize(const u64 newTotalSize)
    {
        if (newTotalSize <= totalSize)
        {
            m_logger.Error(
                "Resize() - Requires that a new size is larger than the old. No doing this could lead to possible data "
                "loss.");
            return false;
        }

        if (m_freeListMemoryRequirement > 0)
        {
            // We are using a freelist so we should resize it first.
            const u64 newMemoryRequirement =
                FreeList::GetMemoryRequirement(newTotalSize, SMALLEST_POSSIBLE_FREELIST_ALLOCATION);
            // Allocate enough space for our freelist
            void* newMemory = Memory.AllocateBlock(MemoryType::RenderSystem, newMemoryRequirement);
            // A pointer to our old memory block (which will get populated by the resize method)
            void* oldMemory = nullptr;
            // Resize our freelist
            if (!m_freeList.Resize(newMemory, newMemoryRequirement, &oldMemory))
            {
                // Our resize failed
                m_logger.Error("Resize() - Failed to resize internal freelist.");
                Memory.Free(MemoryType::RenderSystem, newMemory);
                return false;
            }

            // Free our old memory and store our new info
            Memory.Free(MemoryType::RenderSystem, oldMemory);
            m_freeListMemoryRequirement = newMemoryRequirement;
            m_freeListBlock             = newMemory;
        }

        return true;
    }

    bool RenderBuffer::Allocate(const u64 size, u64* outOffset)
    {
        if (size == 0 || !outOffset)
        {
            m_logger.Error("Allocate() - Requires a nonzero size and a valid pointer to hold the offset.");
            return false;
        }

        if (m_freeListMemoryRequirement == 0)
        {
            m_logger.Warn(
                "Allocate() - Called on a buffer that is not using Freelists. Offset will not be valid! Call "
                "LoadRange() instead.");
            *outOffset = 0;
            return true;
        }

        return m_freeList.AllocateBlock(size, outOffset);
    }

    bool RenderBuffer::Free(const u64 size, const u64 offset)
    {
        if (size == 0)
        {
            m_logger.Error("Free() - Requires a nonzero size.");
            return false;
        }

        if (m_freeListMemoryRequirement == 0)
        {
            m_logger.Warn("Free() - Called on a buffer that is not using Freelists. Nothing was done");
            return true;
        }

        return m_freeList.FreeBlock(size, offset);
    }

    bool RenderBuffer::Read(u64 offset, u64 size, void** outMemory) { return true; }

    bool RenderBuffer::LoadRange(u64 offset, u64 size, const void* data) { return true; }

    bool RenderBuffer::CopyRange(u64 srcOffset, RenderBuffer* dest, u64 dstOffset, u64 size) { return true; }

    bool RenderBuffer::Draw(u64 offset, u32 elementCount, bool bindOnly) { return true; }
}  // namespace C3D
