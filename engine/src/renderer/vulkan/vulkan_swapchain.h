
#pragma once
#include <vector>

#include <vulkan/vulkan.h>

#include "vulkan_image.h"
#include "vulkan_framebuffer.h"

namespace C3D
{
	struct VulkanContext;

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain();

		void Create(VulkanContext* context, u32 width, u32 height);

		void Recreate(VulkanContext* context, u32 width, u32 height);

		void Destroy(const VulkanContext* context);

		bool AcquireNextImageIndex(VulkanContext* context, u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex);

		void Present(VulkanContext* context, VkQueue graphicsQueue, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);

		VkSwapchainKHR handle;

		VkSurfaceFormatKHR imageFormat;
		u32 imageCount;

		u8 maxFramesInFlight;

		VkImageView* views;

		VulkanImage depthAttachment;

		std::vector<VulkanFrameBuffer> frameBuffers;
	private:
		void CreateInternal(VulkanContext* context, u32 width, u32 height);

		void DestroyInternal(const VulkanContext* context);
		
		VkPresentModeKHR m_presentMode;

		VkImage* m_images;
		
	};
}