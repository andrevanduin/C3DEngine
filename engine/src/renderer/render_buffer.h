
#pragma once
#include "core/defines.h"
#include "memory/free_list.h"

namespace C3D
{
	enum class RenderBufferType
	{
		/* @brief Buffer with unknown use. Default but will almost always be invalid. */
		Unknown,
		/* @brief Buffer used for vertex data. */
		Vertex,
		/* @brief Buffer used for index data. */
		Index,
		/* @brief Buffer used for uniform data. */
		Uniform,
		/* @brief Buffer used for staging  (i.e. host-visible to device-local memory). */
		Staging,
		/* @brief Buffer used for reading only. */
		Read,
		/* @brief Buffer used for data storage. */
		Storage,
	};

	class RenderBuffer
	{
	public:
		RenderBuffer(RenderBufferType type, u64 totalSize);
		RenderBuffer(const RenderBuffer& other) = delete;
		RenderBuffer(RenderBuffer&& other) = delete;

		RenderBuffer& operator=(const RenderBuffer& other) = delete;
		RenderBuffer& operator=(RenderBuffer&& other) = delete;

		virtual ~RenderBuffer() = default;

		virtual bool Create(bool useFreelist);
		virtual void Destroy();

		virtual bool Bind(u64 offset) = 0;
		virtual bool Unbind() = 0;

		virtual void* MapMemory(u64 offset, u64 size) = 0;
		virtual void UnmapMemory(u64 offset, u64 size) = 0;

		virtual bool Flush(u64 offset, u64 size) = 0;
		virtual bool Resize(u64 newTotalSize) = 0;
		
		virtual bool Read(u64 offset, u64 size, void** outMemory) = 0;
		virtual bool LoadRange(u64 offset, u64 size, const void* data) = 0;
		virtual bool CopyRange(u64 srcOffset, RenderBuffer* dest, u64 dstOffset, u64 size) = 0;

		virtual bool Draw(u64 offset, u32 elementCount, bool bindOnly) = 0;

		RenderBufferType type;
		u64 totalSize;

	protected:
		u64 m_freeListMemoryRequirement;
		NewFreeList m_freeList;
		void* m_freeListBlock;
	};
}