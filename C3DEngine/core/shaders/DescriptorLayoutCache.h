
#pragma once
#include "../VkTypes.h"

#include <unordered_map>
#include <vector>

namespace C3D
{
	class DescriptorLayoutCache
	{
	public:
		void Init(VkDevice device);
		void Cleanup();

		VkDescriptorSetLayout CreateDescriptorLayout(const VkDescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;

			bool operator==(const DescriptorLayoutInfo& other) const;

			[[nodiscard]] size_t Hash() const;
		};

	private:
		struct DescriptorLayoutHash
		{
			std::size_t operator()(const DescriptorLayoutInfo& k) const
			{
				return k.Hash();
			}
		};

		std::unordered_map<DescriptorLayoutInfo, VkDescriptorSetLayout, DescriptorLayoutHash> m_layoutCache;
		VkDevice m_device { VK_NULL_HANDLE };
	};
}