
#include "vulkan_object_shader.h"

#include "core/logger.h"
#include "core/memory.h"
#include "math/c3d_math.h"
#include "math/math_types.h"

#include "renderer/vulkan/vulkan_shader_utils.h"
#include "renderer/vulkan/vulkan_utils.h"

constexpr auto BUILTIN_SHADER_NAME_OBJECT = "Builtin.ObjectShader";

namespace C3D
{
	VulkanObjectShader::VulkanObjectShader()
		: globalUbo(), m_stages{}, m_globalDescriptorPool(nullptr), m_globalDescriptorSetLayout(nullptr),
		  m_globalDescriptorSets{}, m_objectDescriptorPool(nullptr), m_objectDescriptorSetLayout(nullptr),
		  m_objectUniformBufferIndex(0), m_objectStates{}, m_defaultDiffuse(nullptr), m_pipeline()
	{
	}

	bool VulkanObjectShader::Create(const VulkanContext* context, Texture* defaultDiffuseTexture)
	{
		m_defaultDiffuse = defaultDiffuseTexture;

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

		// Local/Object Descriptors
		constexpr u32 localSamplerCount = 1;
		constexpr VkDescriptorType descriptorTypes[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT] =
		{
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,				// Binding 0 - uniform buffer
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,		// Binding 1 - Diffuse sampler layout		
		};

		VkDescriptorSetLayoutBinding bindings[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
		Memory::Zero(&bindings, sizeof(VkDescriptorSetLayoutBinding) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);

		for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; i++)
		{
			bindings[i].binding = i;
			bindings[i].descriptorCount = 1;
			bindings[i].descriptorType = descriptorTypes[i];
			bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		}

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutCreateInfo.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT;
		layoutCreateInfo.pBindings = bindings;
		VK_CHECK(vkCreateDescriptorSetLayout(context->device.logicalDevice, &layoutCreateInfo, context->allocator, &m_objectDescriptorSetLayout));

		// Local/Object Descriptor pool
		VkDescriptorPoolSize objectPoolSizes[2];
		// The first section is used for uniform buffers
		objectPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objectPoolSizes[0].descriptorCount = VULKAN_OBJECT_MAX_OBJECT_COUNT;
		// The second section is used for image samplers
		objectPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		objectPoolSizes[1].descriptorCount = localSamplerCount * VULKAN_OBJECT_MAX_OBJECT_COUNT;

		VkDescriptorPoolCreateInfo objectPoolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		objectPoolCreateInfo.poolSizeCount = 2;
		objectPoolCreateInfo.pPoolSizes = objectPoolSizes;
		objectPoolCreateInfo.maxSets = VULKAN_OBJECT_MAX_OBJECT_COUNT;

		VK_CHECK(vkCreateDescriptorPool(context->device.logicalDevice, &objectPoolCreateInfo, context->allocator, &m_objectDescriptorPool));

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
		constexpr i32 attributeCount = 2;
		VkVertexInputAttributeDescription attributeDescriptions[attributeCount];

		// Position and texcoord
		constexpr VkFormat formats[attributeCount] = 
		{
			VK_FORMAT_R32G32B32_SFLOAT,		// Position
			VK_FORMAT_R32G32_SFLOAT,		// Texture coordinates
		};
		constexpr u64 sizes[attributeCount] = 
		{
			sizeof(vec3),					// Position
			sizeof(vec2),					// Texture coordinates
		};

		for (u32 i = 0; i < attributeCount; i++)
		{
			attributeDescriptions[i].binding = 0;
			attributeDescriptions[i].location = i;
			attributeDescriptions[i].format = formats[i];
			attributeDescriptions[i].offset = static_cast<u32>(offset);
			offset += sizes[i];
		}

		// Descriptor set layouts
		constexpr i32 descriptorSetLayoutCount = 2;
		VkDescriptorSetLayout layouts[descriptorSetLayoutCount] = {
			m_globalDescriptorSetLayout,
			m_objectDescriptorSetLayout
		};

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
		const u32 deviceLocalBits = context->device.supportsDeviceLocalHostVisible ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : 0;
		if (!m_globalUniformBuffer.Create(context, sizeof(GlobalUniformObject) * 3, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | deviceLocalBits, true))
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

		if (!m_objectUniformBuffer.Create(context, sizeof(ObjectUniformObject), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true))
		{
			Logger::PrefixError("VULKAN_OBJECT_SHADER", "Failed to create Material instance buffer");
			return false;
		}

		return true;
	}

	void VulkanObjectShader::Destroy(const VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_OBJECT_SHADER");
		VkDevice logicalDevice = context->device.logicalDevice;

		Logger::Info("Destroying object descriptor pool");
		vkDestroyDescriptorPool(logicalDevice, m_objectDescriptorPool, context->allocator);

		Logger::Info("Destroying object descriptor set layout");
		vkDestroyDescriptorSetLayout(logicalDevice, m_objectDescriptorSetLayout, context->allocator);

		Logger::Info("Destroying object uniform buffer");
		m_objectUniformBuffer.Destroy(context);

		Logger::Info("Destroying global uniform buffer");
		m_globalUniformBuffer.Destroy(context);

		Logger::Info("Destroying pipeline");
		m_pipeline.Destroy(context);

		Logger::Info("Destroying global descriptor pool");
		vkDestroyDescriptorPool(logicalDevice, m_globalDescriptorPool, context->allocator);

		Logger::Info("Destroying global descriptor set layout");
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

	void VulkanObjectShader::UpdateGlobalState(const VulkanContext* context, f32 deltaTime) const
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

	void VulkanObjectShader::UpdateObject(const VulkanContext* context, const GeometryRenderData data)
	{
		const u32 imageIndex = context->imageIndex;
		const auto commandBuffer = context->graphicsCommandBuffers[imageIndex].handle;

		vkCmdPushConstants(commandBuffer, m_pipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &data.model);

		// Obtain material data
		VulkanObjectShaderObjectState* objectState = &m_objectStates[data.objectId];
		VkDescriptorSet objectDescriptorSet = objectState->descriptorSets[imageIndex];

		// TODO: if needs update
		VkWriteDescriptorSet descriptorWrites[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
		Memory::Zero(descriptorWrites, sizeof(VkWriteDescriptorSet) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);

		u32 descriptorCount = 0;
		u32 descriptorIndex = 0;

		// Descriptor 0 - Uniform buffer
		u32 range = sizeof(ObjectUniformObject);
		u64 offset = sizeof(ObjectUniformObject) * data.objectId; // Also the index into the array.
		ObjectUniformObject obo{};

		// TODO: get diffuse color from material
		static f32 accumulator = 0.0f;
		accumulator += context->frameDeltaTime;

		const f32 s = (Sin(accumulator) + 1.0f) / 2.0f; // scale [-1, 1] to [0, 1]
		obo.diffuseColor = vec4(s, s, s, 1.0f);

		// Load the data into our uniform buffer
		m_objectUniformBuffer.LoadData(context, offset, range, 0, &obo);

		// Only do this if the descriptor has not yet been updated
		if (objectState->descriptorStates[descriptorIndex].generations[imageIndex] == INVALID_ID)
		{
			VkDescriptorBufferInfo bufferInfo;
			bufferInfo.buffer = m_objectUniformBuffer.handle;
			bufferInfo.offset = offset;
			bufferInfo.range = range;

			VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptor.dstSet = objectDescriptorSet;
			descriptor.dstBinding = descriptorIndex;
			descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptor.descriptorCount = 1;
			descriptor.pBufferInfo = &bufferInfo;

			descriptorWrites[descriptorCount] = descriptor;
			descriptorCount++;

			// Update the frame generation. In this case it is only needed once since this is a buffer
			objectState->descriptorStates[descriptorIndex].generations[imageIndex] = 1;
		}
		descriptorIndex++;

		// TODO: samplers
		constexpr u32 samplerCount = 1;
		VkDescriptorImageInfo imageInfos[samplerCount];
		for (u32 samplerIndex = 0; samplerIndex < samplerCount; samplerIndex++)
		{
			Texture* t = data.textures[samplerIndex];
			u32* descriptorGeneration = &objectState->descriptorStates[descriptorIndex].generations[imageIndex];

			// If the texture hasn't been loaded yet, use the default
			// TODO: Determine which use the texture has and pull appropriate default based on that
			if (t->generation == INVALID_ID)
			{
				t = m_defaultDiffuse;

				// Reset the descriptor generation if using the default texture
				*descriptorGeneration = INVALID_ID;
			}

			// Check if the descriptor needs updating first
			if (t && (*descriptorGeneration != t->generation || *descriptorGeneration == INVALID_ID))
			{
				auto internalData = static_cast<VulkanTextureData*>(t->internalData);

				// Assign view and sampler
				imageInfos[samplerIndex].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfos[samplerIndex].imageView = internalData->image.view;
				imageInfos[samplerIndex].sampler = internalData->sampler;

				VkWriteDescriptorSet descriptor = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
				descriptor.dstSet = objectDescriptorSet;
				descriptor.dstBinding = descriptorIndex;
				descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptor.descriptorCount = 1;
				descriptor.pImageInfo = &imageInfos[samplerIndex];

				descriptorWrites[descriptorCount] = descriptor;
				descriptorCount++;

				// Sync frame generation if not using a default texture
				if (t->generation != INVALID_ID)
				{
					*descriptorGeneration = t->generation;
				}
				descriptorIndex++;
			}
		}

		if (descriptorCount > 0)
		{
			vkUpdateDescriptorSets(context->device.logicalDevice, descriptorCount, descriptorWrites, 0, nullptr);
		}

		// Bind the descriptor set to be updated, or in case the shader changed
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.layout, 1, 1, &objectDescriptorSet, 0, nullptr);
	}

	bool VulkanObjectShader::AcquireResources(const VulkanContext* context, u32* outObjectId)
	{
		// TODO: freelist
		*outObjectId = m_objectUniformBufferIndex;
		m_objectUniformBufferIndex++;

		const u32 objectId = *outObjectId;
		VulkanObjectShaderObjectState* objectState = &m_objectStates[objectId];
		for (auto& descriptorState : objectState->descriptorStates)
		{
			for (u32& generation : descriptorState.generations)
			{
				generation = INVALID_ID;
			}
		}

		// Allocate descriptor sets
		const VkDescriptorSetLayout layouts[3] = { m_objectDescriptorSetLayout, m_objectDescriptorSetLayout, m_objectDescriptorSetLayout };

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.descriptorPool = m_objectDescriptorPool;
		allocInfo.descriptorSetCount = 3; // One per frame
		allocInfo.pSetLayouts = layouts;

		const VkResult result = vkAllocateDescriptorSets(context->device.logicalDevice, &allocInfo, objectState->descriptorSets);
		if (!VulkanUtils::IsSuccess(result))
		{
			Logger::PrefixError("VULKAN_OBJECT_SHADER", "Error allocating descriptor sets: {}", VulkanUtils::ResultString(result));
			return false;
		}

		return true;
	}

	void VulkanObjectShader::ReleaseResources(const VulkanContext* context, const u32 objectId)
	{
		VulkanObjectShaderObjectState* objectState = &m_objectStates[objectId];

		constexpr u32 descriptorSetCount = 2;

		const VkResult result = vkFreeDescriptorSets(context->device.logicalDevice, m_objectDescriptorPool, descriptorSetCount, objectState->descriptorSets);
		if (!VulkanUtils::IsSuccess(result))
		{
			Logger::PrefixError("VULKAN_OBJECT_SHADER", "Failed to free descriptor sets: {}", VulkanUtils::ResultString(result));
		}

		for (auto& descriptorState : objectState->descriptorStates)
		{
			for (u32& generation : descriptorState.generations)
			{
				generation = INVALID_ID;
			}
		}

		// TODO: add the objectId to the freelist
	}
}
