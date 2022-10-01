
#pragma once
#include <vulkan/vulkan.h>

#include "core/defines.h"

#include "vulkan_command_buffer.h"

namespace C3D
{
	struct VulkanContext;

	class VulkanImage
	{
	public:
		VulkanImage();

		void Create(const VulkanContext* context, VkImageType imageType, u32 _width, u32 _height, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView,
			VkImageAspectFlags viewAspectFlags);

		void CreateView(const VulkanContext* context, VkFormat format, VkImageAspectFlags aspectFlags);

		void TransitionLayout(const VulkanContext* context, const VulkanCommandBuffer* commandBuffer, VkFormat format, 
			VkImageLayout oldLayout, VkImageLayout newLayout) const;

		void CopyFromBuffer(const VulkanContext* context, VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const;

		void Destroy(const VulkanContext* context);

		VkImage handle;
		VkImageView view;

		u32 width, height;
	private:
		VkDeviceMemory m_memory;
	};
}