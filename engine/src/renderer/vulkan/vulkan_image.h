
#pragma once
#include <vulkan/vulkan.h>
#include "core/defines.h"

namespace C3D
{
	struct VulkanContext;

	class VulkanImage
	{
	public:
		VulkanImage();

		void Create(const VulkanContext* context, VkImageType imageType, u32 width, u32 height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView,
			VkImageAspectFlags viewAspectFlags);

		void CreateView(const VulkanContext* context, VkFormat format, VkImageAspectFlags aspectFlags);

		void Destroy(const VulkanContext* context);

		VkImage handle;
		VkImageView view;

	private:
		VkDeviceMemory m_memory;
		u32 m_width, m_height;
	};
}