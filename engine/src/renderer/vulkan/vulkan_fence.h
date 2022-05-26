
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanFenceManager
{
	void Create(const VulkanContext* context, bool createSignaled, VulkanFence* fence);

	void Destroy(const VulkanContext* context, VulkanFence* fence);

	bool Wait(const VulkanContext* context, VulkanFence* fence, u64 timeoutNs);

	void Reset(const VulkanContext* context, VulkanFence* fence);
}