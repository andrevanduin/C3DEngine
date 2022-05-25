
#include "vulkan_device.h"
#include "core/logger.h"
#include "core/memory.h"

namespace C3D
{
	void VulkanDeviceManager::QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VulkanSwapChainSupportInfo* supportInfo)
	{
		VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &supportInfo->capabilities))

		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportInfo->formatCount, nullptr))

		if (supportInfo->formatCount != 0)
		{
			if (!supportInfo->formats)
			{
				supportInfo->formats = Memory::Allocate<VkSurfaceFormatKHR>(supportInfo->formatCount, MemoryType::Renderer);
			}

			VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &supportInfo->formatCount, supportInfo->formats))
		}

		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportInfo->presentModeCount, nullptr))

		if (supportInfo->presentModeCount != 0)
		{
			if (!supportInfo->presentModes)
			{
				supportInfo->presentModes = Memory::Allocate<VkPresentModeKHR>(supportInfo->presentModeCount, MemoryType::Renderer);
			}

			VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &supportInfo->presentModeCount, supportInfo->presentModes))
		}
	}

	bool VulkanDeviceManager::DetectDepthFormat(VulkanDevice* device)
	{
		VkFormat candidates[3] =
		{
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT
		};

		constexpr u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
		for (const auto& candidate : candidates)
		{
			VkFormatProperties properties;
			vkGetPhysicalDeviceFormatProperties(device->physicalDevice, candidate, &properties);

			if ((properties.linearTilingFeatures & flags) == flags)
			{
				device->depthFormat = candidate;
				return true;
			}
			if ((properties.optimalTilingFeatures & flags) == flags) 
			{
				device->depthFormat = candidate;
				return true;
			}
		}

		return false;
	}

	bool VulkanDeviceManager::Create(vkb::Instance instance, VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_DEVICE_MANAGER");

		// Use VKBootstrap to select a GPU for us
		vkb::PhysicalDeviceSelector selector{ instance };
		VkPhysicalDeviceFeatures features{};
		features.pipelineStatisticsQuery = true;
		features.multiDrawIndirect = true;
		features.drawIndirectFirstInstance = true;
		features.samplerAnisotropy = true;

		selector.set_required_features(features);

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 2)
			.set_surface(context->surface)
			.select()
			.value();

		Logger::Info("Suitable Physical Device found");

		// Create the Vulkan logicalDevice
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();

		Logger::Info("Logical Device Created");

		context->device.logicalDevice = vkbDevice.device;
		context->device.physicalDevice = physicalDevice.physical_device;

		context->device.graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		context->device.presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
		context->device.transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();

		context->device.graphicsQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		context->device.presentQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::present).value();
		context->device.transferQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

		Logger::Info("Queues obtained");

		QuerySwapChainSupport(physicalDevice, context->surface, &context->device.swapChainSupport);
		Logger::Info("SwapChain support information obtained");

		VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.queueFamilyIndex = context->device.graphicsQueueIndex;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK(vkCreateCommandPool(context->device.logicalDevice, &poolCreateInfo, context->allocator, &context->device.graphicsCommandPool));
		Logger::Info("Graphics command pool created");

		vkGetPhysicalDeviceProperties(context->device.physicalDevice, &context->device.properties);

		auto driverVersion = context->device.properties.driverVersion;
		auto apiVersion = context->device.properties.apiVersion;

		VkPhysicalDeviceMemoryProperties memory;
		vkGetPhysicalDeviceMemoryProperties(context->device.physicalDevice, &memory);

		f32 gpuMemory = 0.0f;
		for (u32 i = 0; i < memory.memoryHeapCount; i++)
		{
			const auto heap = memory.memoryHeaps[i];
			if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				gpuMemory = static_cast<f32>(heap.size) / 1024.0f / 1024.0f / 1024.0f;
				break;
			}
		}

		Logger::Info("GPU            - {}", context->device.properties.deviceName);
		Logger::Info("GPU Memory     - {:.2f}GiB", gpuMemory);
		Logger::Info("Driver Version - {}.{}.{}", VK_VERSION_MAJOR(driverVersion), VK_VERSION_MINOR(driverVersion), VK_VERSION_PATCH(driverVersion));
		Logger::Info("API Version    - {}.{}.{}", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));

		Logger::PopPrefix();
		return true;
	}

	void VulkanDeviceManager::Destroy(VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_DEVICE_MANAGER");
		Logger::Info("Destroying Queue indices");
		context->device.graphicsQueueIndex = 0;
		context->device.presentQueueIndex = 0;
		context->device.transferQueueIndex = 0;

		Logger::Info("Destroying Logical Device");
		vkDestroyDevice(context->device.logicalDevice, context->allocator);

		Logger::Info("Destroying Physical Device Handle");
		context->device.physicalDevice = nullptr;
		Logger::PopPrefix();
	}
}
