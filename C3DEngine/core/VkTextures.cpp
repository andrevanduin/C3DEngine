
#include <iostream>

#include "VkTextures.h"
#include "VkInitializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool VkUtil::LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage)
{
	int texWidth, texHeight, texChannels;

	const auto pixels = stbi_load(file, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels)
	{
		std::cout << "Failed to load texture file: " << file << std::endl;
		return false;
	}

	const void* pixelPtr = pixels;
	const auto imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;

	constexpr auto imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	const auto stagingBuffer = engine.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(engine.allocator, stagingBuffer.allocation, &data);

	memcpy(data, pixelPtr, imageSize);

	vmaUnmapMemory(engine.allocator, stagingBuffer.allocation);

	stbi_image_free(pixels);

	const VkExtent3D imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

	const auto createInfo = VkInit::ImageCreateInfo(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

	AllocatedImage image;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	vmaCreateImage(engine.allocator, &createInfo, &allocInfo, &image.image, &image.allocation, nullptr);

	engine.ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkImageSubresourceRange range{};
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;

		VkImageMemoryBarrier imageBarrierToTransfer = {};
		imageBarrierToTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

		imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrierToTransfer.image = image.image;
		imageBarrierToTransfer.subresourceRange = range;

		imageBarrierToTransfer.srcAccessMask = 0;
		imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageExtent;

		vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
		imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
		});

	engine.deletionQueue.Push([=] { vmaDestroyImage(engine.allocator, image.image, image.allocation); });

	vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	std::cout << "Texture loaded successfully: " << file << std::endl;

	outImage = image;
	return true;
}