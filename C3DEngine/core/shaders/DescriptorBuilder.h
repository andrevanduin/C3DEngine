
#pragma once
#include "../VkTypes.h"
#include "DescriptorLayoutCache.h"
#include "DescriptorAllocator.h"

#include <vector>

namespace C3D
{
	class DescriptorBuilder
	{
	public:
		static DescriptorBuilder Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& BindBuffer(uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);
		DescriptorBuilder& BindImage(uint32_t binding, const VkDescriptorImageInfo* imageInfo, VkDescriptorType type, VkShaderStageFlags stageFlags);

		bool Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
		bool Build(VkDescriptorSet& set);

	private:

		std::vector<VkWriteDescriptorSet> m_writes;
		std::vector<VkDescriptorSetLayoutBinding> m_bindings;

		DescriptorLayoutCache* m_cache = nullptr;
		DescriptorAllocator* m_allocator = nullptr;
	};
}