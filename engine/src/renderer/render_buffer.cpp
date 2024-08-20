
#include "render_buffer.h"

#include "memory/global_memory_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    // TODO: Setting SMALLEST_POSSIBLE_FREELIST_ALLOCATION here to 8 could cause problems potentially.
    constexpr static auto SMALLEST_POSSIBLE_FREELIST_ALLOCATION = 8;

    constexpr const char* INSTANCE_NAME = "RENDER_BUFFER";

    RenderBuffer::RenderBuffer(const String& name) : m_name(name) {}

    bool RenderBuffer::Create(const RenderBufferType bufferType, const u64 size, RenderBufferTrackType trackType)
    {
        type        = bufferType;
        m_trackType = trackType;
        totalSize   = size;

        if (m_trackType == RenderBufferTrackType::FreeList)
        {
            // Get the memory requirements for our freelist
            auto freeListMemoryRequirement = FreeList::GetMemoryRequirement(totalSize, SMALLEST_POSSIBLE_FREELIST_ALLOCATION);
            // Allocate enough space for our freelist
            m_freeListBlock = Memory.AllocateBlock(MemoryType::RenderSystem, freeListMemoryRequirement);
            // Create the freelist
            m_freeList.Create(m_freeListBlock, freeListMemoryRequirement, SMALLEST_POSSIBLE_FREELIST_ALLOCATION, totalSize);
        }
        else if (m_trackType == RenderBufferTrackType::Linear)
        {
            m_offset = 0;
        }
        else
        {
            ERROR_LOG("Invalid RenderBufferTrackType provided.");
            return false;
        }

        return true;
    }

    void RenderBuffer::Destroy()
    {
        m_name.Destroy();

        if (m_trackType == RenderBufferTrackType::FreeList)
        {
            // We are using a freelist
            m_freeList.Destroy();
            Memory.Free(m_freeListBlock);
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
            ERROR_LOG("Requires that a new size is larger than the old. Not doing this could lead to possible data loss.");
            return false;
        }

        if (m_trackType == RenderBufferTrackType::FreeList)
        {
            // We are using a freelist so we should resize it first.
            const u64 newMemoryRequirement = FreeList::GetMemoryRequirement(newTotalSize, SMALLEST_POSSIBLE_FREELIST_ALLOCATION);
            // Allocate enough space for our freelist
            void* newMemory = Memory.AllocateBlock(MemoryType::RenderSystem, newMemoryRequirement);
            // A pointer to our old memory block (which will get populated by the resize method)
            void* oldMemory = nullptr;
            // Resize our freelist
            if (!m_freeList.Resize(newMemory, newMemoryRequirement, &oldMemory))
            {
                // Our resize failed
                ERROR_LOG("Failed to resize internal freelist.");
                Memory.Free(newMemory);
                return false;
            }

            // Free our old memory and store our new info
            Memory.Free(oldMemory);
            m_freeListBlock = newMemory;
        }

        return true;
    }

    bool RenderBuffer::Allocate(const u64 size, u64& outOffset)
    {
        if (size == 0)
        {
            ERROR_LOG("Requires a nonzero size.");
            return false;
        }

        if (m_trackType == RenderBufferTrackType::None)
        {
            WARN_LOG("Called on a buffer that has TrackType == None. Offset will not be valid! Call LoadRange() instead.");
            outOffset = 0;
            return false;
        }
        else if (m_trackType == RenderBufferTrackType::Linear)
        {
            outOffset = m_offset;
            m_offset += size;
            return true;
        }
        else
        {
            return m_freeList.AllocateBlock(size, &outOffset);
        }
    }

    bool RenderBuffer::Free(const u64 size, const u64 offset)
    {
        if (size == 0)
        {
            ERROR_LOG("Requires a nonzero size.");
            return false;
        }

        if (m_trackType == RenderBufferTrackType::FreeList)
        {
            return m_freeList.FreeBlock(size, offset);
        }

        WARN_LOG("Called on a buffer that is not using Freelists. Nothing was done.");
        return true;
    }

    bool RenderBuffer::Clear(bool zeroMemory)
    {
        if (zeroMemory)
        {
            // TODO: Zero out memory
            FATAL_LOG("Unimplemented functionality.");
            return false;
        }

        if (m_trackType == RenderBufferTrackType::FreeList)
        {
            if (!m_freeList.Clear())
            {
                ERROR_LOG("Failed to Clear the freelist.");
                return false;
            }
            return true;
        }

        m_offset = 0;
        return true;
    }

    bool RenderBuffer::Read(u64 offset, u64 size, void** outMemory) { return true; }

    bool RenderBuffer::LoadRange(u64 offset, u64 size, const void* data, bool includeInFrameWorkload) { return true; }

    bool RenderBuffer::CopyRange(u64 srcOffset, RenderBuffer* dest, u64 dstOffset, u64 size, bool includeInFrameWorkload) { return true; }

    bool RenderBuffer::Draw(u64 offset, u32 elementCount, bool bindOnly) { return true; }
}  // namespace C3D
