
#pragma once
#include "../VkTypes.h"
#include "DescriptorAllocator.h"
#include "ShaderEffect.h"

#include <string>

namespace C3D
{
	class ShaderDescriptorBinder
	{
	public:

		struct BufferWriteDescriptor
		{
			uint32_t dstSet;
			uint32_t dstBinding;

			VkDescriptorType descriptorType;
			VkDescriptorBufferInfo bufferInfo;

			uint32_t dynamicOffset;
		};

		void BindBuffer(const std::string& name, const VkDescriptorBufferInfo& bufferInfo);

		void BindDynamicBuffer(const std::string& name, uint32_t offset, const VkDescriptorBufferInfo& bufferInfo);

		void ApplyBinds(VkCommandBuffer  cmd) const;

		void BuildSets(VkDevice device, DescriptorAllocator& allocator);

		void SetShader(ShaderEffect* newShader);

		std::array<VkDescriptorSet, 4> cachedDescriptorSets;

	private:
		struct DynamicOffsets
		{
			std::array<uint32_t, 16> offsets;
			uint32_t count{ 0 };
		};

		std::array<DynamicOffsets, 4> m_setOffset{};
		std::vector<BufferWriteDescriptor> m_bufferWrites;

		ShaderEffect* m_shaders{ nullptr };
	};
}
