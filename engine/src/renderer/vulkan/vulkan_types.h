
#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"
#include "vulkan_device.h"

#include "core/defines.h"
#include "core/asserts.h"

#define VK_CHECK(expr)							\
	{											\
		C3D_ASSERT((expr) == VK_SUCCESS)		\
	}

namespace C3D
{
	class VulkanImage;

	struct VulkanFence
	{
		VkFence handle;
		bool isSignaled;
	};

	struct VulkanShaderStage
	{
		VkShaderModuleCreateInfo createInfo;
		VkShaderModule module;
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
	};

	struct VulkanTextureData
	{
		VulkanImage image;
		VkSampler sampler;
	};

	struct VulkanContext
	{
		f32 frameDeltaTime;

		VkInstance instance;
		VkAllocationCallbacks* allocator;
		VkSurfaceKHR surface;

		VulkanDevice device;
		VulkanSwapChain swapChain;
		VulkanRenderPass mainRenderPass;

		std::vector<VulkanCommandBuffer> graphicsCommandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> queueCompleteSemaphores;
		std::vector<VulkanFence> inFlightFences;
		std::vector<VulkanFence*> imagesInFlight;

		u32 imageIndex;
		u32 currentFrame;

		u32 frameBufferWidth, frameBufferHeight;
		u32 cachedFrameBufferWidth, cachedFrameBufferHeight;

		u64 frameBufferSizeGeneration;
		u64 frameBufferSizeLastGeneration;

		bool recreatingSwapChain;

		[[nodiscard]] i32 FindMemoryIndex(const u32 typeFilter, const u32 propertyFlags) const
		{
			VkPhysicalDeviceMemoryProperties memoryProperties;
			vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);

			for (u32 i = 0; i < memoryProperties.memoryTypeCount; i++)
			{
				if (typeFilter & 1 << i && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
				{
					return static_cast<i32>(i);
				}
			}
			return -1;
		}
	};
}