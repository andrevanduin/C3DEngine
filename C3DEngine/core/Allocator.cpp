
#include "Allocator.h"

namespace C3D
{
	void Allocator::Init(VkDevice device, VkPhysicalDevice physicalDevice, VkInstance instance)
	{
		VmaAllocatorCreateInfo createInfo;
		createInfo.device = device;
		createInfo.physicalDevice = physicalDevice;
		createInfo.instance = instance;
		vmaCreateAllocator(&createInfo, &m_allocator);
	}

	void Allocator::CreateImage(const VkImageCreateInfo info, const VmaMemoryUsage usage, const VkMemoryPropertyFlags flags, AllocatedImage& outImage) const
	{
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = usage;
		allocInfo.requiredFlags = flags;

		vmaCreateImage(m_allocator, &info, &allocInfo, &outImage.image, &outImage.allocation, nullptr);
	}

	void Allocator::DestroyImage(const AllocatedImage& image) const
	{
		vmaDestroyImage(m_allocator, image.image, image.allocation);
	}

	void Allocator::Cleanup() const
	{
		vmaDestroyAllocator(m_allocator);
	}
}
