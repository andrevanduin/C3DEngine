
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
		explicit RenderBuffer(const char* name);
		RenderBuffer(const RenderBuffer& other) = delete;
		RenderBuffer(RenderBuffer&& other) = delete;

		RenderBuffer& operator=(const RenderBuffer& other) = delete;
		RenderBuffer& operator=(RenderBuffer&& other) = delete;

		virtual ~RenderBuffer() = default;

		virtual bool Create(RenderBufferType bufferType, u64 size, bool useFreelist);
		virtual void Destroy();

		virtual bool Bind(u64 offset);
		virtual bool Unbind();

		virtual void* MapMemory(u64 offset, u64 size);
		virtual void UnMapMemory(u64 offset, u64 size);

		virtual bool Flush(u64 offset, u64 size);
		virtual bool Resize(u64 newTotalSize);

		virtual bool Allocate(u64 size, u64* outOffset);
		virtual bool Free(u64 size, u64 offset);

		virtual bool Read(u64 offset, u64 size, void** outMemory);
		virtual bool LoadRange(u64 offset, u64 size, const void* data);
		virtual bool CopyRange(u64 srcOffset, RenderBuffer* dest, u64 dstOffset, u64 size);

		virtual bool Draw(u64 offset, u32 elementCount, bool bindOnly);

		RenderBufferType type;
		u64 totalSize;

	protected:
		LoggerInstance m_logger;

		u64 m_freeListMemoryRequirement;
		FreeList m_freeList;
		void* m_freeListBlock;
	};
}