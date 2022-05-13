
#include "DescriptorAllocator.h"
#include "../Logger.h"

namespace C3D
{
	void DescriptorAllocator::Init(VkDevice device)
	{
		device = device;
	}

	void DescriptorAllocator::ResetPools()
	{
		for (const auto p : m_usedPools)
		{
			vkResetDescriptorPool(device, p, 0);
		}

		m_freePools = m_usedPools;
		m_usedPools.clear();
		m_currentPool = VK_NULL_HANDLE;
	}

	bool DescriptorAllocator::Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout)
	{
		if (m_currentPool == VK_NULL_HANDLE)
		{
			m_currentPool = GrabPool();
			m_usedPools.push_back(m_currentPool);
		}

		VkDescriptorSetAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		allocInfo.pNext = nullptr;

		allocInfo.pSetLayouts = &layout;
		allocInfo.descriptorPool = m_currentPool;
		allocInfo.descriptorSetCount = 1;

		const auto allocResult = vkAllocateDescriptorSets(device, &allocInfo, set);
		if (allocResult == VK_SUCCESS) return true;

		if (allocResult == VK_ERROR_FRAGMENTED_POOL || 
			allocResult == VK_ERROR_OUT_OF_POOL_MEMORY)
		{
			m_currentPool = GrabPool();
			m_usedPools.push_back(m_currentPool);
			allocInfo.descriptorPool = m_currentPool;

			vkAllocateDescriptorSets(device, &allocInfo, set);

			if (allocResult == VK_SUCCESS) return true;
		}

		// Failed to allocate twice something is really wrong!
		Logger::Error("Allocating Descriptor Sets failed twice!");
		return false;
	}

	void DescriptorAllocator::Cleanup() const
	{
		for (const auto p : m_freePools)
		{
			vkDestroyDescriptorPool(device, p, nullptr);
		}

		for (const auto p : m_usedPools)
		{
			vkDestroyDescriptorPool(device, p, nullptr);
		}
	}

	VkDescriptorPool DescriptorAllocator::CreatePool(const float count, const VkDescriptorPoolCreateFlags flags)
	{
		std::vector<VkDescriptorPoolSize> sizes;
		sizes.reserve(m_descriptorSizes.sizes.size());

		for (const auto& [type, size]: m_descriptorSizes.sizes)
		{
			sizes.push_back({ type, static_cast<uint32_t>(size * count) });
		}

		VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		poolCreateInfo.flags = flags;
		poolCreateInfo.maxSets = static_cast<uint32_t>(count);
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
		poolCreateInfo.pPoolSizes = sizes.data();

		VkDescriptorPool descriptorPool;
		vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool);
		return descriptorPool;
	}

	VkDescriptorPool DescriptorAllocator::GrabPool()
	{
		if (!m_freePools.empty())
		{
			const auto pool = m_freePools.back();
			m_freePools.pop_back();
			return pool;
		}

		return CreatePool(1000, 0);
	}
}
