
#pragma once

#include "VkTypes.h"
#include "VkEngine.h"

namespace VkUtil
{
	bool LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
}