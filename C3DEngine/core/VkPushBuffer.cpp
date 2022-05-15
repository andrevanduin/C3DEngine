
#include "VkPushBuffer.h"
#include "Logger.h"

#include <cstring>

namespace C3D
{
	uint32_t PushBuffer::Push(const void* data, const size_t size)
	{
		const uint32_t offset = currentOffset;

		auto target = static_cast<char*>(mapped);
		target += currentOffset;

		memcpy(target, data, size);

		currentOffset += static_cast<uint32_t>(size);
		currentOffset = PadUniformBufferSize(currentOffset);

		return offset;
	}

	void PushBuffer::Init(const VmaAllocator& allocator, const AllocatedBufferUntyped sourceBuffer, const uint32_t alignment)
	{
		source = sourceBuffer;
		align = alignment;
		currentOffset = 0;
		if (vmaMapMemory(allocator, sourceBuffer.allocation, &mapped) != VK_SUCCESS)
		{
			Logger::Error("PushBuffer::Init() failed");
		}
	}

	void PushBuffer::Reset()
	{
		currentOffset = 0;
	}

	// Method from: https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
	uint32_t PushBuffer::PadUniformBufferSize(const uint32_t originalSize) const
	{
		const size_t minUboAlignment = align;
		size_t alignedSize = originalSize;
		if (minUboAlignment > 0)
		{
			alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
		}
		return static_cast<uint32_t>(alignedSize);
	}
}
