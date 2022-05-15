
#include "ShaderEffect.h"
#include "../Logger.h"
#include "../VkInitializers.h"

#include <spirv_reflect.h>
#include <algorithm>

namespace C3D
{
	ShaderEffect::ShaderStage::ShaderStage(ShaderModule* module, const VkShaderStageFlagBits stage)
		: module(module), stage(stage)
	{}

	void ShaderEffect::AddStage(ShaderModule* shaderModule, VkShaderStageFlagBits stage)
	{
		m_stages.emplace_back(shaderModule, stage);
	}

	struct DescriptorSetLayoutData
	{
		uint32_t setNumber;
		VkDescriptorSetLayoutCreateInfo createInfo;
		std::vector<VkDescriptorSetLayoutBinding> bindings;
	};

	bool ShaderEffect::ReflectLayout(VkDevice device, const std::vector<ReflectionOverrides>& overrides)
	{
		std::vector<DescriptorSetLayoutData> setLayoutData;
		std::vector<VkPushConstantRange> constantRanges;

		for (const auto& stage : m_stages)
		{
			SpvReflectShaderModule spvModule;
			auto result = spvReflectCreateShaderModule(stage.module->code.size() * sizeof(uint32_t), stage.module->code.data(), &spvModule);
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				Logger::Error("Failed to Reflect Shader Module");
				return false;
			}

			// First we reflect the amount Descriptor Sets
			uint32_t count = 0;
			result = spvReflectEnumerateDescriptorSets(&spvModule, &count, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				Logger::Error("Failed to Enumerate Descriptor Sets to get the Count");
				return false;
			}

			// The second time we actually get the descriptor sets now that we know how many there are in total
			std::vector<SpvReflectDescriptorSet*> sets(count);
			result = spvReflectEnumerateDescriptorSets(&spvModule, &count, sets.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				Logger::Error("Failed to Enumerate Descriptor Sets");
				return false;
			}

			for (const auto& reflectedSet : sets)
			{
				DescriptorSetLayoutData layout = {};
				layout.bindings.resize(reflectedSet->binding_count);

				for (uint32_t iBinding = 0; iBinding < reflectedSet->binding_count; iBinding++)
				{
					const auto& reflectedBinding = *reflectedSet->bindings[iBinding];
					auto& layoutBinding = layout.bindings[iBinding];

					layoutBinding.binding = reflectedBinding.binding;
					layoutBinding.descriptorType = static_cast<VkDescriptorType>(reflectedBinding.descriptor_type);

					for (const auto& [name, type] : overrides)
					{
						if (strcmp(reflectedBinding.name, name.c_str()) == 0)
						{
							layoutBinding.descriptorType = type;
						}
					}

					layoutBinding.descriptorCount = 1;
					for (uint32_t iDim = 0; iDim < reflectedBinding.array.dims_count; iDim++) 
					{
						layoutBinding.descriptorCount *= reflectedBinding.array.dims[iDim];
					}
					layoutBinding.stageFlags = static_cast<VkShaderStageFlagBits>(spvModule.shader_stage);

					const ReflectedBinding reflected { reflectedSet->set, layoutBinding.binding, layoutBinding.descriptorType };
					bindings[reflectedBinding.name] = reflected;
				}

				layout.setNumber = reflectedSet->set;
				layout.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				layout.createInfo.bindingCount = reflectedSet->binding_count;
				layout.createInfo.pBindings = layout.bindings.data();

				setLayoutData.push_back(layout);
			}

			result = spvReflectEnumeratePushConstantBlocks(&spvModule, &count, nullptr);
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				Logger::Error("Failed to Enumerate Push Constant Blocks to get the Count");
				return false;
			}

			std::vector<SpvReflectBlockVariable*> pushConstants(count);
			result = spvReflectEnumeratePushConstantBlocks(&spvModule, &count, pushConstants.data());
			if (result != SPV_REFLECT_RESULT_SUCCESS)
			{
				Logger::Error("Failed to Enumerate Push Constant Blocks");
				return false;
			}

			if (count > 0)
			{
				VkPushConstantRange pushConstantRange{};
				pushConstantRange.offset = pushConstants[0]->offset;
				pushConstantRange.size = pushConstants[0]->size;
				pushConstantRange.stageFlags = stage.stage;

				constantRanges.push_back(pushConstantRange);
			}
		}

		std::array<DescriptorSetLayoutData, 4> mergedLayouts;
		for (uint32_t i = 0; i < 4; i++)
		{
			auto& layoutData = mergedLayouts[i];
			layoutData.setNumber = i;
			layoutData.createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

			std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> binds;
			for (auto& s : setLayoutData)
			{
				if (s.setNumber == i)
				{
					for (auto& b : s.bindings)
					{
						if (binds.find(b.binding) == binds.end())
						{
							binds[b.binding] = b;
						}
						else
						{
							binds[b.binding].stageFlags |= b.stageFlags;
						}
					}
				}
			}

			for (auto& [k, v] : binds) layoutData.bindings.push_back(v);

			std::sort(layoutData.bindings.begin(), layoutData.bindings.end(), 
				[](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b)
				{
					return a.binding < b.binding;
				}
			);

			layoutData.createInfo.bindingCount = static_cast<uint32_t>(layoutData.bindings.size());
			layoutData.createInfo.pBindings = layoutData.bindings.data();
			layoutData.createInfo.flags = 0;
			layoutData.createInfo.pNext = nullptr;

			if (layoutData.createInfo.bindingCount > 0)
			{
				setHashes[i] = HashDescriptorLayoutInfo(&layoutData.createInfo);
				vkCreateDescriptorSetLayout(device, &layoutData.createInfo, nullptr, &setLayouts[i]);
			}
			else
			{
				setHashes[i] = 0;
				setLayouts[i] = VK_NULL_HANDLE;
			}
		}

		auto pipelineLayoutInfo = VkInit::PipelineLayoutCreateInfo();
		pipelineLayoutInfo.pPushConstantRanges = constantRanges.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(constantRanges.size());

		std::array<VkDescriptorSetLayout, 4> compactedLayouts{};
		int s = 0;
		for (int i = 0; i < 4; i++)
		{
			if (setLayouts[i] != VK_NULL_HANDLE)
			{
				compactedLayouts[s] = setLayouts[i];
				s++;
			}
		}

		pipelineLayoutInfo.setLayoutCount = s;
		pipelineLayoutInfo.pSetLayouts = compactedLayouts.data();

		vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &builtLayout);
	}

	void ShaderEffect::FillStages(std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages) const
	{
		for (const auto& s : m_stages)
		{
			pipelineStages.push_back(VkInit::PipelineShaderStageCreateInfo(s.stage, s.module->module));
		}
	}
}
