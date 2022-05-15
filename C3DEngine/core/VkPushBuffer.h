
#include "VkTypes.h"

namespace C3D
{
	struct PushBuffer
	{
		template<typename T>
		uint32_t Push(T& data);

		uint32_t Push(const void* data, size_t size);

		void Init(const VmaAllocator& allocator, AllocatedBufferUntyped sourceBuffer, uint32_t alignment);

		void Reset();

		[[nodiscard]] uint32_t PadUniformBufferSize(uint32_t originalSize) const;

		AllocatedBufferUntyped source;
		uint32_t align = 0, currentOffset = 0;

		void* mapped{ nullptr };
	};

	template<typename T>
	uint32_t PushBuffer::Push(T& data)
	{
		return Push(&data, sizeof(T));
	}
}