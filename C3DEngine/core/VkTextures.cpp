
#include "VkTextures.h"
#include "VkInitializers.h"
#include "Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "AssetLoader.h"
#include "TextureAsset.h"

namespace C3D
{
	bool LoadImageFromFile(VulkanEngine& engine, const std::string& file, AllocatedImage& outImage)
	{
		int texWidth, texHeight, texChannels;

		const auto pixels = stbi_load(file.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		if (!pixels)
		{
			Logger::Error("Failed to load texture file {}", file);
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

		outImage = UploadImage(texWidth, texHeight, imageFormat, engine, stagingBuffer);

		vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		Logger::Info("Texture loaded successfully: {}", file);

		return true;
	}

	bool LoadImageFromAsset(VulkanEngine& engine, const std::string& path, AllocatedImage& outImage)
	{
		Assets::AssetFile file;
		if (!LoadBinary(path, file))
		{
			Logger::Error("Error when loading Texture {}", path);
			return false;
		}

		Assets::TextureInfo textureInfo = ReadTextureInfo(&file);

		VkDeviceSize imageSize = textureInfo.textureSize;
		VkFormat imageFormat;

		switch (textureInfo.textureFormat)
		{
			case Assets::TextureFormat::Rgba8:
				imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
				break;
			default:
				Logger::Error("Failed to load Texture. Unsupported TextureFormat {} in file {}", static_cast<uint32_t>(textureInfo.textureFormat), path);
				return false;
		}

		auto stagingBuffer = engine.CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_UNKNOWN, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

		std::vector<MipmapInfo> mips;

		void* data;
		vmaMapMemory(engine.allocator, stagingBuffer.allocation, &data);
		size_t offset = 0;
		for (int i = 0; i < textureInfo.pages.size(); i++) {
			MipmapInfo mip{};
			mip.dataOffset = offset;
			mip.dataSize = textureInfo.pages[i].originalSize;
			mips.push_back(mip);
			UnpackTexturePage(&textureInfo, i, file.binaryBlob.data(), static_cast<char*>(data) + offset);

			offset += mip.dataSize;
		}
		vmaUnmapMemory(engine.allocator, stagingBuffer.allocation);

		outImage = UploadImageMipMapped(textureInfo.pages[0].width, textureInfo.pages[0].height, imageFormat, engine, stagingBuffer, mips);

		vmaDestroyBuffer(engine.allocator, stagingBuffer.buffer, stagingBuffer.allocation);

		return true;
	}

	AllocatedImage UploadImage(const int texWidth, const int texHeight, const VkFormat imageFormat, VulkanEngine& engine, const AllocatedBufferUntyped& stagingBuffer)
	{
		const VkExtent3D imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

		const auto createInfo = VkInit::ImageCreateInfo(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);

		AllocatedImage image{};

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

			VkImageMemoryBarrier imageBarrierToTransfer = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
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

		const auto viewCreateInfo = VkInit::ImageViewCreateInfo(imageFormat, image.image, VK_IMAGE_ASPECT_COLOR_BIT);
		vkCreateImageView(engine.vkObjects.device, &viewCreateInfo, nullptr, &image.defaultView);

		engine.deletionQueue.Push([=, &engine]
		{
			vkDestroyImageView(engine.vkObjects.device, image.defaultView, nullptr);
			vmaDestroyImage(engine.allocator, image.image, image.allocation);
		});

		image.mipLevels = 1;
		return image;
	}

	AllocatedImage UploadImageMipMapped(const uint32_t texWidth, const uint32_t texHeight, const VkFormat imageFormat, VulkanEngine& engine, const AllocatedBufferUntyped& stagingBuffer, const std::vector<MipmapInfo>& mips)
	{
		VkExtent3D imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1 };

		auto createInfo = VkInit::ImageCreateInfo(imageFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, imageExtent);
		createInfo.mipLevels = static_cast<uint32_t>(mips.size());

		AllocatedImage image{};

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vmaCreateImage(engine.allocator, &createInfo, &allocInfo, &image.image, &image.allocation, nullptr);

		engine.ImmediateSubmit([&](VkCommandBuffer cmd) {
			VkImageSubresourceRange range{};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseMipLevel = 0;
			range.levelCount = static_cast<uint32_t>(mips.size());
			range.baseArrayLayer = 0;
			range.layerCount = 1;

			VkImageMemoryBarrier imageBarrierToTransfer = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			imageBarrierToTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrierToTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToTransfer.image = image.image;
			imageBarrierToTransfer.subresourceRange = range;

			imageBarrierToTransfer.srcAccessMask = 0;
			imageBarrierToTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToTransfer);

			for (int i = 0; i < mips.size(); i++)
			{
				VkBufferImageCopy copyRegion = {};
				copyRegion.bufferOffset = mips[i].dataOffset;
				copyRegion.bufferRowLength = 0;
				copyRegion.bufferImageHeight = 0;

				copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.imageSubresource.mipLevel = i;
				copyRegion.imageSubresource.baseArrayLayer = 0;
				copyRegion.imageSubresource.layerCount = 1;
				copyRegion.imageExtent = imageExtent;

				vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				imageExtent.width /= 2;
				imageExtent.height /= 2;
			}

			VkImageMemoryBarrier imageBarrierToReadable = imageBarrierToTransfer;
			imageBarrierToReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrierToReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			imageBarrierToReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrierToReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrierToReadable);
		});

		image.mipLevels = static_cast<uint32_t>(mips.size());

		auto viewCreateInfo = VkInit::ImageViewCreateInfo(imageFormat, image.image, VK_IMAGE_ASPECT_COLOR_BIT);
		viewCreateInfo.subresourceRange.layerCount = image.mipLevels;
		vkCreateImageView(engine.vkObjects.device, &viewCreateInfo, nullptr, &image.defaultView);

		engine.deletionQueue.Push([=, &engine]
		{
			vkDestroyImageView(engine.vkObjects.device, image.defaultView, nullptr);
			vmaDestroyImage(engine.allocator, image.image, image.allocation);
		});

		return image;
	}
}
