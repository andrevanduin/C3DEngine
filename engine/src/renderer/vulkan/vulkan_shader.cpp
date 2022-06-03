
#include "vulkan_shader.h"

#include "core/c3d_string.h"

namespace C3D
{
	// Static lookup table for getting our renderer shader types into vulkan types (an their corresponding size in bytes)
	static VulkanFormatSize* TYPES;

	VulkanShader::VulkanShader()
		: id(0), m_mappedUniformBufferBlock(nullptr), m_logger("VULKAN_SHADER"), m_context(nullptr), m_config(),
		  m_name(nullptr), m_renderPass(nullptr), m_stages{}, m_descriptorPool(nullptr), m_descriptorSetLayouts{}, m_globalDescriptorSets{},
		  m_instanceCount(0), m_instanceStates{}, m_useInstances(false), m_useLocals(false)
	{
		if (!TYPES)
		{
			// This gets done to have a static lookup table for the sizes
			// Since TYPES is static this only happens at startup once for the first VulkanShader
			VulkanFormatSize types[ShaderAttributeType::FinalValue];
			types[ShaderAttributeType::Float32] = {VK_FORMAT_R32_SFLOAT, 4};
			types[ShaderAttributeType::Float32_2] = {VK_FORMAT_R32G32_SFLOAT, 8};
			types[ShaderAttributeType::Float32_3] = {VK_FORMAT_R32G32B32_SFLOAT, 12};
			types[ShaderAttributeType::Float32_4] = {VK_FORMAT_R32G32B32A32_SFLOAT, 16};
			types[ShaderAttributeType::Int8] = {VK_FORMAT_R8_SINT, 1};
			types[ShaderAttributeType::Int8_2] = {VK_FORMAT_R8G8_SINT, 2};
			types[ShaderAttributeType::Int8_3] = {VK_FORMAT_R8G8B8_SINT, 3};
			types[ShaderAttributeType::Int8_4] = {VK_FORMAT_R8G8B8A8_SINT, 4};
			types[ShaderAttributeType::UInt8] = {VK_FORMAT_R8_UINT, 1};
			types[ShaderAttributeType::UInt8_2] = {VK_FORMAT_R8G8_UINT, 2};
			types[ShaderAttributeType::UInt8_3] = {VK_FORMAT_R8G8B8_UINT, 3};
			types[ShaderAttributeType::UInt8_4] = {VK_FORMAT_R8G8B8A8_UINT, 4};
			types[ShaderAttributeType::Int16] = {VK_FORMAT_R16_SINT, 2};
			types[ShaderAttributeType::Int16_2] = {VK_FORMAT_R16G16_SINT, 4};
			types[ShaderAttributeType::Int16_3] = {VK_FORMAT_R16G16B16_SINT, 6};
			types[ShaderAttributeType::Int16_4] = {VK_FORMAT_R16G16B16A16_SINT, 8};
			types[ShaderAttributeType::UInt16] = {VK_FORMAT_R16_UINT, 2};
			types[ShaderAttributeType::UInt16_2] = {VK_FORMAT_R16G16_UINT, 4};
			types[ShaderAttributeType::UInt16_3] = {VK_FORMAT_R16G16B16_UINT, 6};
			types[ShaderAttributeType::UInt16_4] = {VK_FORMAT_R16G16B16A16_UINT, 8};
			types[ShaderAttributeType::Int32] = {VK_FORMAT_R32_SINT, 4};
			types[ShaderAttributeType::Int32_2] = {VK_FORMAT_R32G32_SINT, 8};
			types[ShaderAttributeType::Int32_3] = {VK_FORMAT_R32G32B32_SINT, 12};
			types[ShaderAttributeType::Int32_4] = {VK_FORMAT_R32G32B32A32_SINT, 16};
			types[ShaderAttributeType::UInt32] = {VK_FORMAT_R32_UINT, 4};
			types[ShaderAttributeType::UInt32_2] = {VK_FORMAT_R32G32_UINT, 8};
			types[ShaderAttributeType::UInt32_3] = {VK_FORMAT_R32G32B32_UINT, 12};
			types[ShaderAttributeType::UInt32_4] = {VK_FORMAT_R32G32B32A32_UINT, 16};

			TYPES = types;
		}
	}

	bool VulkanShader::Create(VulkanContext* context, const char* name, VulkanRenderPass* renderPass, VkShaderStageFlags stages, u32 maxDescriptorSetCount, bool useInstances, bool useLocals)
	{
		if (!context || !name)
		{
			m_logger.Error("Create() - must supply valid pointers to context and name");
			return false;
		}

		if (stages == 0)
		{
			m_logger.Error("Create() - stages must be nonzero");
			return false;
		}

		m_context = context;
		m_name = StringDuplicate(name);
		m_useInstances = useInstances;
		m_useLocals = useLocals;
		m_renderPass = renderPass;
		m_config.
	}

	bool VulkanShader::AddSampler(const char* name, VulkanShaderScope scope, u32* outLocation)
	{
		if (scope == VulkanShaderScope::Instance && !m_useInstances)
		{
			m_logger.Error("AddSampler() - cannot add an instance sampler for a shader that does not use instances");
			return false;
		}

		// Samples cannot be used with push constants
		if (scope == VulkanShaderScope::Local)
		{
			m_logger.Error("AddSampler() - cannot add a sampler at local scope");
			return false;
		}

		if (!UniformNameValid(name) || !UniformAddStateValid())
		{
			return false;
		}

		const u32 setIndex = scope == VulkanShaderScope::Global ? DESC_SET_INDEX_GLOBAL : DESC_SET_INDEX_INSTANCE;
		VulkanDescriptorSetConfig* setConfig = &m_config.descriptorSets[setIndex];


	}

	bool VulkanShader::UniformNameValid(const char* name)
	{

	}

	bool VulkanShader::UniformAddStateValid()
	{

	}
}
