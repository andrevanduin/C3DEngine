
#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "../../core/defines.h"
#include "../../core/asserts.h"

#define VK_CHECK(expr)							\
	{											\
		C3D_ASSERT((expr) == VK_SUCCESS)	\
	}

namespace C3D
{
	struct VulkanSwapChainSupportInfo
	{
		VkSurfaceCapabilitiesKHR capabilities;

		u32 formatCount;

		VkSurfaceFormatKHR* formats;
		u32 presentModeCount;

		VkPresentModeKHR* presentModes;
	};

	struct VulkanDevice
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;

		VulkanSwapChainSupportInfo swapChainSupport;

		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memory;

		VkFormat depthFormat;

		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkQueue transferQueue;

		VkCommandPool graphicsCommandPool;

		u32 graphicsQueueIndex;
		u32 presentQueueIndex;
		u32 transferQueueIndex;
	};

	struct VulkanImage
	{
		VkImage handle;
		VkDeviceMemory memory;
		VkImageView view;

		u32 width, height;
	};

	enum class VulkanRenderPassState : u8
	{
		Ready, Recording, InRenderPass, RecordingEnded, Submitted, NotAllocated
	};

	struct VulkanRenderPass
	{
		VkRenderPass handle;
		i32 x, y, w, h;
		f32 r, g, b, a;

		f32 depth;
		u32 stencil;

		VulkanRenderPassState state;
	};

	struct VulkanSwapChain
	{
		u8 maxFramesInFlight;

		VkSurfaceFormatKHR imageFormat;
		VkPresentModeKHR presentMode;

		VkSwapchainKHR handle;

		u32 imageCount;

		VkImage* images;
		VkImageView* views;

		VulkanImage depthAttachment;
	};

	enum class VulkanCommandBufferState : u8
	{
		Ready, Recording, InRenderPass, RecordingEnded, Submitted, NotAllocated		
	};

	struct VulkanCommandBuffer
	{
		VkCommandBuffer handle;
		VulkanCommandBufferState state;
	};

	struct VulkanContext
	{
		VkInstance instance;
		VkAllocationCallbacks* allocator;
		VkSurfaceKHR surface;

		VulkanDevice device;
		VulkanSwapChain swapChain;
		VulkanRenderPass mainRenderPass;

		std::vector<VulkanCommandBuffer> graphicsCommandBuffers;

		u32 imageIndex;
		u32 currentFrame;
		u32 frameBufferWidth, frameBufferHeight;

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