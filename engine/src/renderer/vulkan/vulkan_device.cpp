
#include "vulkan_device.h"
#include "vulkan_types.h"

#include "core/memory.h"
#include "core/logger.h"

namespace C3D
{
	VulkanDevice::VulkanDevice()
		: physicalDevice(nullptr), logicalDevice(nullptr), swapChainSupport(), supportsDeviceLocalHostVisible(false),
		  graphicsCommandPool(nullptr), graphicsQueue(nullptr), presentQueue(nullptr), transferQueue(nullptr), depthFormat(),
		  graphicsQueueIndex(0), presentQueueIndex(0), transferQueueIndex(0), m_properties(), m_features(), m_memory()
	{
	}

	bool VulkanDevice::Create(vkb::Instance instance, VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_DEVICE_MANAGER");

		// Use VKBootstrap to select a GPU for us
		vkb::PhysicalDeviceSelector selector{ instance };
		m_features.pipelineStatisticsQuery = true;
		m_features.multiDrawIndirect = true;
		m_features.drawIndirectFirstInstance = true;
		m_features.samplerAnisotropy = true;

		selector.set_required_features(m_features);

		vkb::PhysicalDevice vkbPhysicalDevice = selector
			.set_minimum_version(1, 2)
			.set_surface(context->surface)
			.select()
			.value();

		Logger::Info("Suitable Physical Device found");

		// Create the Vulkan logicalDevice
		vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
		vkb::Device vkbDevice = deviceBuilder.build().value();

		Logger::Info("Logical Device Created");

		logicalDevice = vkbDevice.device;
		physicalDevice = vkbPhysicalDevice.physical_device;

		graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
		transferQueue = vkbDevice.get_queue(vkb::QueueType::transfer).value();

		graphicsQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		presentQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::present).value();
		transferQueueIndex = vkbDevice.get_queue_index(vkb::QueueType::transfer).value();

		Logger::Info("Queues obtained");

		QuerySwapChainSupport(context->surface, &swapChainSupport);
		Logger::Info("SwapChain support information obtained");

		VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		poolCreateInfo.queueFamilyIndex = graphicsQueueIndex;
		poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VK_CHECK(vkCreateCommandPool(logicalDevice, &poolCreateInfo, context->allocator, &graphicsCommandPool));
		Logger::Info("Graphics command pool created");

		vkGetPhysicalDeviceProperties(physicalDevice, &m_properties);

		auto driverVersion = m_properties.driverVersion;
		auto apiVersion = m_properties.apiVersion;

		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memory);
		for (u32 i = 0; i < m_memory.memoryTypeCount; i++)
		{
			if ((m_memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0 && 
				(m_memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0)
			{
				supportsDeviceLocalHostVisible = true;
				break;
			}
		}

		f32 gpuMemory = 0.0f;
		for (u32 i = 0; i < m_memory.memoryHeapCount; i++)
		{
			const auto heap = m_memory.memoryHeaps[i];
			if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
			{
				gpuMemory = static_cast<f32>(heap.size) / 1024.0f / 1024.0f / 1024.0f;
				break;
			}
		}

		Logger::Info("GPU            - {}", m_properties.deviceName);
		Logger::Info("GPU Memory     - {:.2f}GiB", gpuMemory);
		Logger::Info("Driver Version - {}.{}.{}", VK_VERSION_MAJOR(driverVersion), VK_VERSION_MINOR(driverVersion), VK_VERSION_PATCH(driverVersion));
		Logger::Info("API Version    - {}.{}.{}", VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion));

		Logger::PopPrefix();
		return true;
	}

	void VulkanDevice::Destroy(const VulkanContext* context)
	{
		Logger::PushPrefix("VULKAN_DEVICE_MANAGER");
		Logger::Info("Destroying Queue indices");
		graphicsQueueIndex = 0;
		presentQueueIndex = 0;
		transferQueueIndex = 0;

		Logger::Info("Destroying Logical Device");
		vkDestroyDevice(logicalDevice, context->allocator);

		Logger::Info("Destroying Physical Device Handle");
		physicalDevice = nullptr;
		Logger::PopPrefix();
	}

	void VulkanDevice::QuerySwapChainSupport(VkSurfaceKHR surface, VulkanSwapChainSupportInfo* supportInfo) const
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

	bool VulkanDevice::DetectDepthFormat()
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
			vkGetPhysicalDeviceFormatProperties(physicalDevice, candidate, &properties);

			if ((properties.linearTilingFeatures & flags) == flags)
			{
				depthFormat = candidate;
				return true;
			}
			if ((properties.optimalTilingFeatures & flags) == flags)
			{
				depthFormat = candidate;
				return true;
			}
		}

		return false;
	}
}
