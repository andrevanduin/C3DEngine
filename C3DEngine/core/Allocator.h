
#pragma once
#include "VkTypes.h"

namespace C3D
{
	class Allocator
	{
	public:
		void Init(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance);

		void CreateImage(VkImageCreateInfo info, VmaMemoryUsage usage, VkMemoryPropertyFlags flags, AllocatedImage& outImage) const;

		void DestroyImage(const AllocatedImage& image) const;

		void Cleanup() const;
	private:
		VmaAllocator m_allocator{};
	};
}