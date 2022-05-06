
#pragma once

#include "VkTypes.h"
#include "VkEngine.h"

namespace C3D::VkUtil
{
	bool LoadImageFromFile(VulkanEngine& engine, const char* file, AllocatedImage& outImage);
}
