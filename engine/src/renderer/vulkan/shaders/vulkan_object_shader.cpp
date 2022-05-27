
#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/memory.h"
#include "math/math_types.h"
#include "renderer/vulkan/vulkan_shader_utils.h"

constexpr auto BUILTIN_SHADER_NAME_OBJECT = "Builtin.ObjectShader";

namespace C3D
{
	bool VulkanObjectShader::Create(const VulkanContext* context)
	{
		constexpr char stageTypeStrings[OBJECT_SHADER_STAGE_COUNT][5] = { "vert", "frag" };
		constexpr VkShaderStageFlagBits stageTypes[OBJECT_SHADER_STAGE_COUNT] = { VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT };

		for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
		{
			if (!CreateShaderModule(context, BUILTIN_SHADER_NAME_OBJECT, stageTypeStrings[i], stageTypes[i], i, m_stages))
			{
				Logger::PrefixError("VULKAN_OBJECT_SHADER", "Unable to create {} shader module for {}", stageTypeStrings[i], BUILTIN_SHADER_NAME_OBJECT);
				return false;
			}
		}

		// TODO: Descriptors

		// Pipeline generation
		VkViewport viewport;
		viewport.x = 0.0f;
		viewport.y = static_cast<f32>(context->frameBufferHeight);
		viewport.width = static_cast<f32>(context->frameBufferWidth);
		viewport.height = -static_cast<f32>(context->frameBufferHeight);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// Scissor
		VkRect2D scissor;
		scissor.offset.x = scissor.offset.y = 0;
		scissor.extent.width = context->frameBufferWidth;
		scissor.extent.height = context->frameBufferHeight;

		// Attributes
		u64 offset = 0;
		constexpr i32 attributeCount = 1;
		VkVertexInputAttributeDescription attributeDescriptions[attributeCount];

		// Position only so far
		constexpr VkFormat formats[attributeCount] = { VK_FORMAT_R32G32B32_SFLOAT };
		constexpr u64 sizes[attributeCount] = { sizeof(vec3) };

		for (u32 i = 0; i < attributeCount; i++)
		{
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = formats[i];
			attributeDescriptions[i].offset = static_cast<u32>(offset);
			offset += sizes[i];
		}

		// TODO: Descriptor set layouts

		// Stages
		VkPipelineShaderStageCreateInfo stageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
		Memory::Zero(stageCreateInfos, sizeof(stageCreateInfos));
		for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
		{
			stageCreateInfos[i].sType = m_stages[i].shaderStageCreateInfo.sType;
			stageCreateInfos[i] = m_stages[i].shaderStageCreateInfo;
		}

		if (!m_pipeline.Create(context, &context->mainRenderPass, attributeCount, attributeDescriptions, 0, nullptr, 
			OBJECT_SHADER_STAGE_COUNT, stageCreateInfos, viewport, scissor, false))
		{
			Logger::PrefixError("VULKAN_OBJECT_SHADER", "Failed to load graphics pipeline for object shader");
			return false;
		}

		return true;
	}

	void VulkanObjectShader::Destroy(const VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_OBJECT_SHADER");
		Logger::Info("Destroying pipeline");
		m_pipeline.Destroy(context);

		Logger::Info("Destroying modules");
		for (auto& stage : m_stages)
		{
			vkDestroyShaderModule(context->device.logicalDevice, stage.module, context->allocator);
			stage.module = nullptr;
		}

		Logger::PopPrefix();
	}

	void VulkanObjectShader::Use(VulkanContext* context)
	{
	}
}
