
#pragma once

#include <vk_types.h>
#include <vk_engine.h>

namespace VkUtil
{
	bool LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
}