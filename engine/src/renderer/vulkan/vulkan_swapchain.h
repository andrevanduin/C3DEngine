
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanSwapChainManager
{
	void Create(VulkanContext* context, u32 width, u32 height, VulkanSwapChain* outSwapChain);

	void Recreate(VulkanContext* context, u32 width, u32 height, VulkanSwapChain* swapChain);

	void Destroy(const VulkanContext* context, VulkanSwapChain* swapChain);

	bool AcquireNextImageIndex(VulkanContext* context, VulkanSwapChain* swapChain, u64 timeoutNs, 
	                           VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex);

	void Present(VulkanContext* context, VulkanSwapChain* swapChain, VkQueue graphicsQueue, VkQueue presentQueue, 
		VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);
}