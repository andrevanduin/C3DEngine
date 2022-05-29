
#include "vulkan_image.h"

#include "vulkan_device.h"
#include "vulkan_types.h"

#include "core/memory.h"
#include "core/logger.h"

namespace C3D
{
	VulkanImage::VulkanImage()
		: handle(nullptr), view(nullptr), m_memory(nullptr), m_width(0), m_height(0)
	{
	}

	void VulkanImage::Create(const VulkanContext* context, VkImageType imageType, const u32 width, const u32 height, const VkFormat format,
	                         const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags memoryFlags, const bool createView,
							 const VkImageAspectFlags viewAspectFlags)
	{
		Logger::PushPrefix("IMAGE");

		m_width = width;
		m_height = height;

		VkImageCreateInfo imageCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;	// TODO: Support different depth.
		imageCreateInfo.mipLevels = 4;		// TODO: Support MipMapping.
		imageCreateInfo.arrayLayers = 1;	// TODO: Support number of layers in the image.
		imageCreateInfo.format = format;
		imageCreateInfo.tiling = tiling;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.usage = usage;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;			// TODO: Configurable sample count.
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;	// TODO: Configurable sharing mode.

		VK_CHECK(vkCreateImage(context->device.logicalDevice, &imageCreateInfo, context->allocator, &handle));

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(context->device.logicalDevice, handle, &memoryRequirements);

		const i32 memoryType = context->FindMemoryIndex(memoryRequirements.memoryTypeBits, memoryFlags);
		if (memoryType == -1)
		{
			Logger::Error("Required memory type not found. Image not valid.");
			return;
		}

		VkMemoryAllocateInfo memoryAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = memoryType;
		VK_CHECK(vkAllocateMemory(context->device.logicalDevice, &memoryAllocateInfo, context->allocator, &m_memory));

		VK_CHECK(vkBindImageMemory(context->device.logicalDevice, handle, m_memory, 0)); // TODO: Configurable memory offset.

		if (createView)
		{
			view = nullptr;
			CreateView(context, format, viewAspectFlags);
		}

		Logger::PopPrefix();
	}

	void VulkanImage::CreateView(const VulkanContext* context, const VkFormat format, const VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo viewCreateInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		viewCreateInfo.image = handle;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;	// TODO: Make configurable.
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

		// TODO: Make Configurable.
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(context->device.logicalDevice, &viewCreateInfo, context->allocator, &view));
	}

	void VulkanImage::Destroy(const VulkanContext* context)
	{
		if (view)
		{
			vkDestroyImageView(context->device.logicalDevice, view, context->allocator);
			view = nullptr;
		}
		if (m_memory)
		{
			vkFreeMemory(context->device.logicalDevice, m_memory, context->allocator);
			m_memory = nullptr;
		}
		if (handle)
		{
			vkDestroyImage(context->device.logicalDevice, handle, context->allocator);
			handle = nullptr;
		}
	}
}
