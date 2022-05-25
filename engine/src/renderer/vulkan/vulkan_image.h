
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanImageManager
{
	void CreateImage(const VulkanContext* context, VkImageType imageType, u32 width, u32 height, VkFormat format,
	                 VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView,
	                 VkImageAspectFlags viewAspectFlags, VulkanImage* outImage);

	void CreateView(const VulkanContext* context, VkFormat format, VulkanImage* image, VkImageAspectFlags aspectFlags);

	void Destroy(const VulkanContext* context, VulkanImage* image);
}