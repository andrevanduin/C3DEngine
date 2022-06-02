
#include "vulkan_image.h"

#include "vulkan_device.h"
#include "vulkan_types.h"

#include "core/memory.h"
#include "core/logger.h"
#include "services/services.h"

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
			Logger::Error("[IMAGE] - Required memory type not found. Image not valid.");
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

	void VulkanImage::TransitionLayout(const VulkanContext* context, const VulkanCommandBuffer* commandBuffer,
		VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout) const
	{
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = context->device.graphicsQueueIndex;
		barrier.dstQueueFamilyIndex = context->device.graphicsQueueIndex;
		barrier.image = handle;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage, destStage;

		// Don't care about the old layout - transfer to optimal layout for the GPU's underlying implementation
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			// Don't care what stage the pipeline is in at the start
			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			// Used for copying
			destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		// Transition from a transfer destination to a shader-readonly layout
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			// From a copying stage to
			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			// The fragment stage
			destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			Logger::Fatal("[VULKAN_IMAGE] - Unsupported layout transition");
			return;
		}

		vkCmdPipelineBarrier(commandBuffer->handle, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	void VulkanImage::CopyFromBuffer(const VulkanContext* context, VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const
	{
		VkBufferImageCopy region;
		Memory.Zero(&region, sizeof(VkBufferImageCopy));
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageExtent.width = m_width;
		region.imageExtent.height = m_height;
		region.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(commandBuffer->handle, buffer, handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
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
