
#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/memory.h"
#include "math/math_types.h"
#include "renderer/vulkan/vulkan_shader_utils.h"

constexpr auto BUILTIN_SHADER_NAME_OBJECT = "Builtin.ObjectShader";

namespace C3D
{
	VulkanObjectShader::VulkanObjectShader()
		: m_stages{}, m_globalDescriptorPool(nullptr), m_globalDescriptorSetLayout(nullptr), m_globalDescriptorSets{}, globalUbo(), m_pipeline()
	{}

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

		// Global Descriptors
		VkDescriptorSetLayoutBinding globalUboLayoutBinding;
		globalUboLayoutBinding.binding = 0;
		globalUboLayoutBinding.descriptorCount = 1;
		globalUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalUboLayoutBinding.pImmutableSamplers = nullptr;
		globalUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo globalLayoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		globalLayoutInfo.bindingCount = 1;
		globalLayoutInfo.pBindings = &globalUboLayoutBinding;
		VK_CHECK(vkCreateDescriptorSetLayout(context->device.logicalDevice, &globalLayoutInfo, context->allocator, &m_globalDescriptorSetLayout));

		// Global Descriptor pool used for global items such as view / projection matrix
		VkDescriptorPoolSize globalPoolSize;
		globalPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalPoolSize.descriptorCount = context->swapChain.imageCount;

		VkDescriptorPoolCreateInfo globalPoolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		globalPoolInfo.poolSizeCount = 1;
		globalPoolInfo.pPoolSizes = &globalPoolSize;
		globalPoolInfo.maxSets = context->swapChain.imageCount;

		VK_CHECK(vkCreateDescriptorPool(context->device.logicalDevice, &globalPoolInfo, context->allocator, &m_globalDescriptorPool));

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

		// Descriptor set layouts
		constexpr i32 descriptorSetLayoutCount = 1;
		VkDescriptorSetLayout layouts[descriptorSetLayoutCount] = { m_globalDescriptorSetLayout };

		// Stages
		VkPipelineShaderStageCreateInfo stageCreateInfos[OBJECT_SHADER_STAGE_COUNT];
		Memory::Zero(stageCreateInfos, sizeof(stageCreateInfos));
		for (u32 i = 0; i < OBJECT_SHADER_STAGE_COUNT; i++)
		{
			stageCreateInfos[i].sType = m_stages[i].shaderStageCreateInfo.sType;
			stageCreateInfos[i] = m_stages[i].shaderStageCreateInfo;
		}

		if (!m_pipeline.Create(context, &context->mainRenderPass, attributeCount, attributeDescriptions, descriptorSetLayoutCount, layouts, 
			OBJECT_SHADER_STAGE_COUNT, stageCreateInfos, viewport, scissor, false))
		{
			Logger::PrefixError("VULKAN_OBJECT_SHADER", "Failed to load graphics pipeline for object shader");
			return false;
		}

		// Create uniform buffer
		if (!m_globalUniformBuffer.Create(context, sizeof(GlobalUniformObject) * 3, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true))
		{
			Logger::PrefixError("VULKAN_OBJECT_SHADER", "Failed to create global uniform buffer");
			return false;
		}

		// Allocate global descriptor sets
		const VkDescriptorSetLayout globalLayouts[3] =
		{
			m_globalDescriptorSetLayout, m_globalDescriptorSetLayout, m_globalDescriptorSetLayout
		};

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_globalDescriptorPool;
		allocInfo.descriptorSetCount = 3;
		allocInfo.pSetLayouts = globalLayouts;
		VK_CHECK(vkAllocateDescriptorSets(context->device.logicalDevice, &allocInfo, m_globalDescriptorSets));

		return true;
	}

	void VulkanObjectShader::Destroy(const VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_OBJECT_SHADER");
		VkDevice logicalDevice = context->device.logicalDevice;

		Logger::Info("Destroying global uniform buffer");
		m_globalUniformBuffer.Destroy(context);

		Logger::Info("Destroying pipeline");
		m_pipeline.Destroy(context);

		Logger::Info("Destroying global descriptor pool");
		vkDestroyDescriptorPool(logicalDevice, m_globalDescriptorPool, context->allocator);

		Logger::Info("Destroying descriptor set layout");
		vkDestroyDescriptorSetLayout(logicalDevice, m_globalDescriptorSetLayout, context->allocator);

		Logger::Info("Destroying modules");
		for (auto& stage : m_stages)
		{
			vkDestroyShaderModule(logicalDevice, stage.module, context->allocator);
			stage.module = nullptr;
		}

		Logger::PopPrefix();
	}

	void VulkanObjectShader::Use(const VulkanContext* context) const
	{
		const u32 imageIndex = context->imageIndex;
		m_pipeline.Bind(&context->graphicsCommandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS);
	}

	void VulkanObjectShader::UpdateGlobalState(const VulkanContext* context) const
	{
		const u32 imageIndex = context->imageIndex;

		const auto commandBuffer = context->graphicsCommandBuffers[imageIndex].handle;
		const auto globalDescriptor = m_globalDescriptorSets[imageIndex];

		// Configure the descriptors for the given index
		constexpr u32 range = sizeof(GlobalUniformObject);
		const u64 offset = sizeof(GlobalUniformObject) * imageIndex;

		// Copy data to the buffer
		m_globalUniformBuffer.LoadData(context, offset, range, 0, &globalUbo);

		VkDescriptorBufferInfo bufferInfo;
		bufferInfo.buffer = m_globalUniformBuffer.handle;
		bufferInfo.offset = offset;
		bufferInfo.range = range;

		VkWriteDescriptorSet descriptorWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		descriptorWrite.dstSet = m_globalDescriptorSets[imageIndex];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context->device.logicalDevice, 1, &descriptorWrite, 0, nullptr);

		// Bind the global descriptor set to be updated
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout, 0, 1, &globalDescriptor, 0, nullptr);
	}

	void VulkanObjectShader::UpdateObject(const VulkanContext* context, const mat4 model) const
	{
		const u32 imageIndex = context->imageIndex;
		const auto commandBuffer = context->graphicsCommandBuffers[imageIndex].handle;

		vkCmdPushConstants(commandBuffer, m_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
	}
}
