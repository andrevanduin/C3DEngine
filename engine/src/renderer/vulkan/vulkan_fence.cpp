
#include "vulkan_fence.h"
#include "core/logger.h"

namespace C3D
{
	void VulkanFenceManager::Create(const VulkanContext* context, const bool createSignaled, VulkanFence* fence)
	{
		fence->isSignaled = createSignaled;

		VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		if (fence->isSignaled) createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VK_CHECK(vkCreateFence(context->device.logicalDevice, &createInfo, context->allocator, &fence->handle));
	}

	void VulkanFenceManager::Destroy(const VulkanContext* context, VulkanFence* fence)
	{
		if (fence->handle)
		{
			vkDestroyFence(context->device.logicalDevice, fence->handle, context->allocator);
			fence->handle = nullptr;
		}
		fence->isSignaled = false;
	}

	bool VulkanFenceManager::Wait(const VulkanContext* context, VulkanFence* fence, const u64 timeoutNs)
	{
		if (!fence->isSignaled)
		{
			const auto result = vkWaitForFences(context->device.logicalDevice, 1, &fence->handle, true, timeoutNs);
			switch (result)
			{
				case VK_SUCCESS:
					fence->isSignaled = true;
					return true;
				case VK_TIMEOUT:
					Logger::PrefixWarn("VULKAN_FENCE_MANAGER", "Waiting for Fence - timed out");
					break;
				case VK_ERROR_DEVICE_LOST:
					Logger::PrefixError("VULKAN_FENCE_MANAGER", "Waiting for Fence - device lost");
					break;
				case VK_ERROR_OUT_OF_HOST_MEMORY:
					Logger::PrefixError("VULKAN_FENCE_MANAGER", "Waiting for Fence - out of host memory");
					break;
				case VK_ERROR_OUT_OF_DEVICE_MEMORY:
					Logger::PrefixError("VULKAN_FENCE_MANAGER", "Waiting for Fence - out of device memory");
					break;
				default:
					Logger::PrefixError("VULKAN_FENCE_MANAGER", "Waiting for Fence - unknown error");
					break;
			}

			return false;
		}
		return true;
	}

	void VulkanFenceManager::Reset(const VulkanContext* context, VulkanFence* fence)
	{
		if (fence->isSignaled)
		{
			VK_CHECK(vkResetFences(context->device.logicalDevice, 1, &fence->handle));
			fence->isSignaled = false;
		}
	}
}
