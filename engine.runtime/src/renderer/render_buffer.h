
#pragma once
#include "defines.h"
#include "memory/free_list.h"
#include "string/string.h"

namespace C3D
{
    enum class RenderBufferType
    {
        /** @brief Buffer with unknown use. Default but will almost always be invalid. */
        Unknown,
        /** @brief Buffer used for vertex data. */
        Vertex,
        /** @brief Buffer used for index data. */
        Index,
        /** @brief Buffer used for uniform data. */
        Uniform,
        /** @brief Buffer used for staging  (i.e. host-visible to device-local memory). */
        Staging,
        /** @brief Buffer used for reading only. */
        Read,
        /** @brief Buffer used for data storage. */
        Storage,
    };

    enum class RenderBufferTrackType
    {
        None,
        FreeList,
        Linear,
    };

    class C3D_API RenderBuffer
    {
    public:
        explicit RenderBuffer(const String& name);
        RenderBuffer(const RenderBuffer& other) = delete;
        RenderBuffer(RenderBuffer&& other)      = delete;

        RenderBuffer& operator=(const RenderBuffer& other) = delete;
        RenderBuffer& operator=(RenderBuffer&& other)      = delete;

        virtual ~RenderBuffer() = default;

        virtual bool Create(RenderBufferType bufferType, u64 size, RenderBufferTrackType trackType);
        virtual void Destroy();

        virtual bool Bind(u64 offset);
        virtual bool Unbind();

        virtual void* MapMemory(u64 offset, u64 size);
        virtual void UnMapMemory(u64 offset, u64 size);

        virtual bool Flush(u64 offset, u64 size);
        virtual bool Resize(u64 newTotalSize);

        virtual bool Allocate(u64 size, u64& outOffset);
        virtual bool Free(u64 size, u64 offset);
        virtual bool Clear(bool zeroMemory);

        virtual bool Read(u64 offset, u64 size, void** outMemory);
        virtual bool LoadRange(u64 offset, u64 size, const void* data, bool includeInFrameWorkload);
        virtual bool CopyRange(u64 srcOffset, RenderBuffer* dest, u64 dstOffset, u64 size, bool includeInFrameWorkload);

        virtual bool Draw(u64 offset, u32 elementCount, bool bindOnly);

        RenderBufferType type;
        u64 totalSize = 0;

    protected:
        String m_name;

        /** @brief The type of memory tracking this renderbuffer uses. */
        RenderBufferTrackType m_trackType = RenderBufferTrackType::None;

        // Linear allocation
        u64 m_offset = 0;

        // Freelist allocation
        FreeList m_freeList;
        void* m_freeListBlock = nullptr;
    };
}  // namespace C3D