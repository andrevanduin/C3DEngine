
#include "ShaderDescriptorBinder.h"
#include "../VkInitializers.h"

#include <algorithm>

namespace C3D
{
	void ShaderDescriptorBinder::BindBuffer(const std::string& name, const VkDescriptorBufferInfo& bufferInfo)
	{
		BindDynamicBuffer(name, -1, bufferInfo);
	}

	void ShaderDescriptorBinder::BindDynamicBuffer(const std::string& name, uint32_t offset, const VkDescriptorBufferInfo& bufferInfo)
	{
		auto found = m_shaders->bindings.find(name);
		if (found != m_shaders->bindings.end())
		{
			const auto& bind = (*found).second;

			for (auto& write : m_bufferWrites)
			{
				if (write.dstBinding == bind.binding && write.dstSet == bind.set)
				{
					if (write.bufferInfo.buffer != bufferInfo.buffer ||
						write.bufferInfo.range != bufferInfo.range ||
						write.bufferInfo.offset != bufferInfo.offset)
					{
						write.bufferInfo = bufferInfo;
						write.dynamicOffset = offset;
					}
					else
					{
						write.dynamicOffset = offset;
					}

					return;
				}
			}

			BufferWriteDescriptor writeDescriptor {};
			writeDescriptor.dstSet = bind.set;
			writeDescriptor.dstBinding = bind.binding;
			writeDescriptor.descriptorType = bind.type;
			writeDescriptor.bufferInfo = bufferInfo;
			writeDescriptor.dynamicOffset = offset;

			cachedDescriptorSets[bind.set] = VK_NULL_HANDLE;

			m_bufferWrites.push_back(writeDescriptor);
		}
	}

	void ShaderDescriptorBinder::ApplyBinds(VkCommandBuffer cmd) const
	{
		for (int i = 0; i < 2; i++)
		{
			if (cachedDescriptorSets[i] != VK_NULL_HANDLE)
			{
				vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_shaders->builtLayout, i, 1, 
					&cachedDescriptorSets[i], m_setOffset[i].count, m_setOffset[i].offsets.data());
			}
		}
	}

	void ShaderDescriptorBinder::BuildSets(VkDevice device, DescriptorAllocator& allocator)
	{
		std::array<std::vector<VkWriteDescriptorSet>, 4> writes{};

		std::sort(m_bufferWrites.begin(), m_bufferWrites.end(), [](BufferWriteDescriptor& a, BufferWriteDescriptor& b)
			{
				if (b.dstSet != a.dstSet) return a.dstSet < b.dstSet;
				return a.dstBinding < b.dstBinding;
			}
		);

		for (auto& [offsets, count] : m_setOffset) count = 0;

		for (auto& w : m_bufferWrites)
		{
			const auto set = w.dstSet;
			auto write = VkInit::WriteDescriptorBuffer(w.descriptorType, VK_NULL_HANDLE, &w.bufferInfo, w.dstBinding);

			writes[set].push_back(write);

			if (w.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || 
				w.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
			{
				DynamicOffsets& offsetSet = m_setOffset[set];
				offsetSet.offsets[offsetSet.count] = w.dynamicOffset;
				offsetSet.count++;
			}
		}

		for (int i = 0; i < 4; i++)
		{
			auto& write = writes[i];
			if (!write.empty())
			{
				if (cachedDescriptorSets[i] == VK_NULL_HANDLE)
				{
					const auto layout = m_shaders->setLayouts[i];

					VkDescriptorSet descriptor;
					allocator.Allocate(&descriptor, layout);

					for (auto& w : write) w.dstSet = descriptor;

					vkUpdateDescriptorSets(device, static_cast<uint32_t>(write.size()), write.data(), 0, nullptr);

					cachedDescriptorSets[i] = descriptor;
				}
			}
		}
	}

	void ShaderDescriptorBinder::SetShader(ShaderEffect* newShader)
	{
		if (m_shaders && m_shaders != newShader)
		{
			for (int i = 0; i < 4; i++)
			{
				if (newShader->setHashes[i] != m_shaders->setHashes[i] || newShader->setHashes[i] == 0)
				{
					cachedDescriptorSets[i] = VK_NULL_HANDLE;
				}
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
			{
				cachedDescriptorSets[i] = VK_NULL_HANDLE;
			}
		}

		m_shaders = newShader;
	}
}
