
#pragma once

#include "VkTypes.h"
#include "VkEngine.h"

namespace C3D
{
	struct MipmapInfo
	{
		size_t dataSize;
		size_t dataOffset;
	};

	bool LoadImageFromFile(VulkanEngine& engine, const std::string& file, AllocatedImage& outImage);

	bool LoadImageFromAsset(VulkanEngine& engine, const std::string& path, AllocatedImage& outImage);

	AllocatedImage UploadImage(int texWidth, int texHeight, VkFormat imageFormat, VulkanEngine& engine, const AllocatedBufferUntyped& stagingBuffer);

	AllocatedImage UploadImageMipMapped(uint32_t texWidth, uint32_t texHeight, VkFormat imageFormat, VulkanEngine& engine,
	                                    const AllocatedBufferUntyped& stagingBuffer, const std::vector<MipmapInfo>& mips);
}
