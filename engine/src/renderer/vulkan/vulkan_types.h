
#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"
#include "vulkan_device.h"
#include "containers/hash_table.h"

#include "core/defines.h"
#include "core/asserts.h"

#define VK_CHECK(expr)							\
	{											\
		C3D_ASSERT((expr) == VK_SUCCESS)		\
	}

namespace C3D
{
	constexpr auto VULKAN_MAX_GEOMETRY_COUNT = 4096;

	class VulkanImage;

	struct VulkanTextureData
	{
		// Internal Vulkan image.
		VulkanImage image;
	};

	struct VulkanGeometryData
	{
		u32 id;
		u32 generation;

		u32 vertexCount;
		u32 vertexElementSize;
		u64 vertexBufferOffset;

		u32 indexCount;
		u32 indexElementSize;
		u64 indexBufferOffset;
	};

	struct VulkanContext
	{
		f32 frameDeltaTime;

		VkInstance instance;
		VkAllocationCallbacks* allocator;
		VkSurfaceKHR surface;

		VulkanDevice device;
		VulkanSwapChain swapChain;

		HashTable<u32> renderPassTable;
		VulkanRenderPass registeredRenderPasses[VULKAN_MAX_REGISTERED_RENDER_PASSES];

		std::vector<VulkanCommandBuffer> graphicsCommandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> queueCompleteSemaphores;

		u32 inFlightFenceCount;
		VkFence inFlightFences[2];

		/* @brief Holds pointers to fences which exist and are owned elsewhere, one per frame */
		VkFence* imagesInFlight[3];

		u32 imageIndex;
		u32 currentFrame;

		u32 frameBufferWidth, frameBufferHeight;

		u64 frameBufferSizeGeneration;
		u64 frameBufferSizeLastGeneration;

		/* @brief Render targets used for world rendering. One per frame. */
		RenderTarget worldRenderTargets[3];

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

		const RenderSystem* frontend;
	};
}