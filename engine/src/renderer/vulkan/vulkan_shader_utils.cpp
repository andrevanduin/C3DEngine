
#include "vulkan_shader_utils.h"

#include "core/c3d_string.h"
#include "core/logger.h"

#include "resources/resource_types.h"
#include "services/services.h"

namespace C3D
{
	bool CreateShaderModule(const VulkanContext* context, const std::string& name, const std::string& type,
	                        const VkShaderStageFlagBits shaderStageFlag, const u32 stageIndex, VulkanShaderStage* shaderStages)
	{
		// Build the file name which will also be the resource name
		char fileName[512];
		StringFormat(fileName, "shaders/%s.%s.spv", name.data(), type.data());

		// Load the resource
		Resource binaryResource{};
		if (!Resources.Load(fileName, ResourceType::Binary, &binaryResource))
		{
			Logger::Error("[CREATE_SHADER_MODULE] - Unable to read shader module '{}'", fileName);
			return false;
		}

		Memory.Zero(&shaderStages[stageIndex].createInfo, sizeof(VkShaderModuleCreateInfo));
		shaderStages[stageIndex].createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		// Use the resource's size and data directly
		shaderStages[stageIndex].createInfo.codeSize = binaryResource.dataSize;
		shaderStages[stageIndex].createInfo.pCode = binaryResource.GetData<u32*>();

		VK_CHECK(vkCreateShaderModule(context->device.logicalDevice, &shaderStages[stageIndex].createInfo, context->allocator, &shaderStages[stageIndex].module));

		Resources.Unload(&binaryResource);

		Memory.Zero(&shaderStages[stageIndex].shaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
		shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[stageIndex].shaderStageCreateInfo.stage = shaderStageFlag;
		shaderStages[stageIndex].shaderStageCreateInfo.module = shaderStages[stageIndex].module;
		shaderStages[stageIndex].shaderStageCreateInfo.pName = "main"; // TODO: possibly make this configurable in the future

		return true;
	}
}
