
#pragma once
#include "vulkan_types.h"

#include <VkBootstrap.h>

namespace C3D::VulkanDeviceManager
{
	bool Create(vkb::Instance instance, VulkanContext* context);
	void Destroy(VulkanContext* context);

	void QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VulkanSwapChainSupportInfo* supportInfo);

	bool DetectDepthFormat(VulkanDevice* device);
}
