
#pragma once
#include "../VkTypes.h"

#include <vector>
#include <string>

namespace C3D
{
	struct ShaderModule
	{
		std::vector<uint32_t> code;
		VkShaderModule module;
	};

	bool LoadShaderModule(VkDevice device, const std::string& path, ShaderModule* outShaderModule);

	uint32_t HashDescriptorLayoutInfo(VkDescriptorSetLayoutCreateInfo* info);
}