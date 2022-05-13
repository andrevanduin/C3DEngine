
#pragma once
#include "../VkTypes.h"

#include <vector>

namespace C3D
{
	class DescriptorAllocator
	{
	public:
		struct PoolSizes
		{
			std::vector<std::pair<VkDescriptorType, float>> sizes =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 0.5f },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4.f },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2.f },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1.f },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0.5f }
			};
		};

		void Init(VkDevice device);

		void ResetPools();
		bool Allocate(VkDescriptorSet* set, VkDescriptorSetLayout layout);
		
		void Cleanup() const;

		VkDevice device{ VK_NULL_HANDLE };
	private:
		VkDescriptorPool CreatePool(float count, VkDescriptorPoolCreateFlags flags);
		VkDescriptorPool GrabPool();

		VkDescriptorPool m_currentPool{ VK_NULL_HANDLE };
		PoolSizes m_descriptorSizes;

		std::vector<VkDescriptorPool> m_usedPools;
		std::vector<VkDescriptorPool> m_freePools;
	};

}