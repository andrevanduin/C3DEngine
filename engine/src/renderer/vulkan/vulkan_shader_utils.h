
#pragma once
#include "vulkan_types.h"

namespace C3D
{
	bool CreateShaderModule(const VulkanContext* context, const std::string& name, const std::string& type, VkShaderStageFlagBits shaderStageFlag, 
	                        u32 stageIndex, VulkanShaderStage* shaderStages);
}