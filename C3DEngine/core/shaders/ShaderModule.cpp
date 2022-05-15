
#include "ShaderModule.h"
#include "../Logger.h"

#include <sstream>
#include <fstream>

namespace C3D
{
	bool LoadShaderModule(VkDevice device, const std::string& path, ShaderModule* outShaderModule)
	{
		// Open the file with the cursor at the end
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file.is_open())
		{
			Logger::Error("Failed to open SPIR-V file: {}", path);
			return false;
		}

		// Get the cursor position (which is the file size in bytes since it's at the end)s
		const auto fileSize = static_cast<size_t>(file.tellg());

		std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
		file.close();

		VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		createInfo.pNext = nullptr;

		createInfo.codeSize = buffer.size() * sizeof(uint32_t);
		createInfo.pCode = buffer.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			Logger::Error("Failed to create ShaderModule for file: {}", path);
			return false;
		}

		outShaderModule->code = std::move(buffer);
		outShaderModule->module = shaderModule;
		return true;
	}

	constexpr uint32_t Fnv1A32(const char* s, const std::size_t count)
	{
		return ((count ? Fnv1A32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

	uint32_t HashDescriptorLayoutInfo(VkDescriptorSetLayoutCreateInfo* info)
	{
		std::stringstream ss;

		ss << info->flags;
		ss << info->bindingCount;

		for (uint32_t i = 0; i < info->bindingCount; i++)
		{
			const auto& binding = info->pBindings[i];

			ss << binding.binding;
			ss << binding.descriptorCount;
			ss << binding.descriptorType;
			ss << binding.stageFlags;
		}

		const auto str = ss.str();
		return Fnv1A32(str.c_str(), str.length());
	}
}
