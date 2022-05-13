
#include "DescriptorLayoutCache.h"

#include <algorithm>

namespace C3D
{
	void DescriptorLayoutCache::Init(VkDevice device)
	{
		m_device = device;
	}

	void DescriptorLayoutCache::Cleanup()
	{
		for (const auto& [info, setLayout] : m_layoutCache)
		{
			vkDestroyDescriptorSetLayout(m_device, setLayout, nullptr);
		}
	}

	VkDescriptorSetLayout DescriptorLayoutCache::CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo* info)
	{
		DescriptorLayoutInfo layoutInfo;
		layoutInfo.bindings.reserve(info->bindingCount);
		bool isSorted = true;

		uint32_t lastBinding = 0;
		for (uint32_t i = 0; i < info->bindingCount; i++)
		{
			layoutInfo.bindings.push_back(info->pBindings[i]);
			if (info->pBindings[i].binding >= lastBinding)
			{
				lastBinding = info->pBindings[i].binding;
			}
			else
			{
				isSorted = false;
				break;
			}
		}

		if (!isSorted)
		{
			std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(), 
				[](const VkDescriptorSetLayoutBinding& a, const VkDescriptorSetLayoutBinding& b)
				{
					return a.binding < b.binding;
				}
			);
		}

		if (const auto it = m_layoutCache.find(layoutInfo); it != m_layoutCache.end())
		{
			return (*it).second;
		}

		VkDescriptorSetLayout layout;
		vkCreateDescriptorSetLayout(m_device, info, nullptr, &layout);

		m_layoutCache[layoutInfo] = layout;
		return layout;
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo& other) const
	{
		if (other.bindings.size() != bindings.size()) return false;

		for (size_t i = 0; i < bindings.size(); i++)
		{
			if (other.bindings[i].binding != bindings[i].binding) return false;
			if (other.bindings[i].descriptorType != bindings[i].descriptorType) return false;
			if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) return false;
			if (other.bindings[i].stageFlags != bindings[i].stageFlags) return false;
		}
		return true;
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::Hash() const
	{
		using std::hash;

		auto result = hash<size_t>()(bindings.size());
		for (const auto& b : bindings)
		{
			size_t bindingHash = b.binding | b.descriptorType << 8 | b.descriptorCount << 16 | b.stageFlags << 24;
			result ^= hash<size_t>()(bindingHash);
		}
		return result;
	}
}
