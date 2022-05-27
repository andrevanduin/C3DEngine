
#include "vulkan_shader_utils.h"

#include "core/logger.h"
#include "core/memory.h"

#include "platform/filesystem.h"

namespace C3D
{
	bool CreateShaderModule(const VulkanContext* context, const std::string& name, const std::string& type,
	                        const VkShaderStageFlagBits shaderStageFlag, const u32 stageIndex, VulkanShaderStage* shaderStages)
	{
		// Build the file path to the shader
		const auto fileName = "assets/shaders/" + name + "." + type + ".spv";

		Memory::Zero(&shaderStages[stageIndex].createInfo, sizeof(VkShaderModuleCreateInfo));
		shaderStages[stageIndex].createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

		File file;
		if (!file.Open(fileName, FileModeRead | FileModeBinary))
		{
			Logger::PrefixError("CREATE_SHADER_MODULE", "Unable to open shader module {}", fileName);
			return false;
		}

		u64 size = 0;
		char* fileBuffer = nullptr;
		if (!file.ReadAllBytes(&fileBuffer, &size))
		{
			Logger::PrefixError("CREATE_SHADER_MODULE", "Unable to binary read shader module {}", fileName);
			return false;
		}

		shaderStages[stageIndex].createInfo.codeSize = size;
		shaderStages[stageIndex].createInfo.pCode = reinterpret_cast<u32*>(fileBuffer);

		file.Close();

		VK_CHECK(vkCreateShaderModule(context->device.logicalDevice, &shaderStages[stageIndex].createInfo, context->allocator, &shaderStages[stageIndex].module));

		Memory::Zero(&shaderStages[stageIndex].shaderStageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
		shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[stageIndex].shaderStageCreateInfo.stage = shaderStageFlag;
		shaderStages[stageIndex].shaderStageCreateInfo.module = shaderStages[stageIndex].module;
		shaderStages[stageIndex].shaderStageCreateInfo.pName = "main"; // TODO: possibly make this configurable in the future

		if (fileBuffer)
		{
			Memory::Free(fileBuffer, sizeof(char) * size, MemoryType::String);
			fileBuffer = nullptr;
		}

		return true;
	}
}
