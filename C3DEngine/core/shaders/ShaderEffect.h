
#pragma once
#include "../VkTypes.h"
#include "ShaderModule.h"

#include <string>
#include <unordered_map>
#include <array>

namespace C3D
{
	class ShaderEffect
	{
	public:
		struct ReflectionOverrides
		{
			std::string name;
			VkDescriptorType type;
		};

		struct ReflectedBinding
		{
			uint32_t set;
			uint32_t binding;
			VkDescriptorType type;
		};

		void AddStage(ShaderModule* shaderModule, VkShaderStageFlagBits stage);

		bool ReflectLayout(VkDevice device, const std::vector<ReflectionOverrides>& overrides);

		void FillStages(std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages) const;

		VkPipelineLayout builtLayout;

		std::unordered_map<std::string, ReflectedBinding> bindings;
		std::array<VkDescriptorSetLayout, 4> setLayouts;
		std::array<uint32_t, 4> setHashes;
	private:
		struct ShaderStage
		{
			ShaderModule* module;
			VkShaderStageFlagBits stage;

			ShaderStage(ShaderModule* module, VkShaderStageFlagBits stage);
		};

		std::vector<ShaderStage> m_stages;
	};
}