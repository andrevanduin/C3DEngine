
#pragma once
#include <VkBootstrap.h>

#include "core/defines.h"
#include "core/logger.h"

namespace C3D
{
	struct VulkanContext;

	struct VulkanSwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;

		u32 formatCount;

		VkSurfaceFormatKHR* formats;
		u32 presentModeCount;

		VkPresentModeKHR* presentModes;
	};

	class VulkanDevice
	{
	public:
		VulkanDevice();

		bool Create(vkb::Instance instance, VulkanContext* context);

		void Destroy(const VulkanContext* context);

		void QuerySwapChainSupport(VkSurfaceKHR surface, VulkanSwapChainSupportInfo* supportInfo) const;

		bool DetectDepthFormat();

		void WaitIdle() const;

		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;

		VulkanSwapChainSupportInfo swapChainSupport;

		bool supportsDeviceLocalHostVisible;

		VkCommandPool graphicsCommandPool;

		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkQueue transferQueue;

		VkFormat depthFormat;

		u32 graphicsQueueIndex;
		u32 presentQueueIndex;
		u32 transferQueueIndex;

		VkPhysicalDeviceProperties properties;
	private:
		LoggerInstance m_logger;

		VkPhysicalDeviceFeatures m_features;
		VkPhysicalDeviceMemoryProperties m_memory;
	};
}
