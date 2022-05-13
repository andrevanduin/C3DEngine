
#include "DescriptorBuilder.h"

namespace C3D
{
	DescriptorBuilder DescriptorBuilder::Begin(DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator)
	{
		DescriptorBuilder builder;
		builder.m_cache = layoutCache;
		builder.m_allocator = allocator;
		return builder;
	}

	DescriptorBuilder& DescriptorBuilder::BindBuffer(const uint32_t binding, const VkDescriptorBufferInfo* bufferInfo, const VkDescriptorType type, const VkShaderStageFlags stageFlags)
	{
		const VkDescriptorSetLayoutBinding layoutBinding{ binding, type, 1, stageFlags, nullptr };
		m_bindings.push_back(layoutBinding);

		VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.pNext = nullptr;

		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pBufferInfo = bufferInfo;
		write.dstBinding = binding;

		m_writes.push_back(write);
		return *this;
	}

	DescriptorBuilder& DescriptorBuilder::BindImage(const uint32_t binding, const VkDescriptorImageInfo* imageInfo, const VkDescriptorType type, const VkShaderStageFlags stageFlags)
	{
		const VkDescriptorSetLayoutBinding layoutBinding{ binding, type, 1, stageFlags, nullptr };
		m_bindings.push_back(layoutBinding);

		VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write.pNext = nullptr;

		write.descriptorCount = 1;
		write.descriptorType = type;
		write.pImageInfo = imageInfo;
		write.dstBinding = binding;

		m_writes.push_back(write);
		return *this;
	}

	bool DescriptorBuilder::Build(VkDescriptorSet& set, VkDescriptorSetLayout& layout)
	{
		VkDescriptorSetLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		createInfo.pNext = nullptr;

		createInfo.pBindings = m_bindings.data();
		createInfo.bindingCount = static_cast<uint32_t>(m_bindings.size());

		layout = m_cache->CreateDescriptorLayout(&createInfo);

		if (!m_allocator->Allocate(&set, layout)) return false;

		for (auto& w : m_writes) w.dstSet = set;

		vkUpdateDescriptorSets(m_allocator->device, static_cast<uint32_t>(m_writes.size()), m_writes.data(), 0, nullptr);
		return true;
	}

	bool DescriptorBuilder::Build(VkDescriptorSet& set)
	{
		VkDescriptorSetLayout layout;
		return Build(set, layout);
	}
}
