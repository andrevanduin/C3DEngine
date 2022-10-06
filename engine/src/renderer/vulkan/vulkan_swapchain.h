
#pragma once
#include <vulkan/vulkan.h>

#include "vulkan_image.h"
#include "renderer/renderer_types.h"
#include "resources/texture.h"

namespace C3D
{
	struct VulkanContext;

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain();

		void Create(VulkanContext* context, u32 width, u32 height);

		void Recreate(VulkanContext* context, u32 width, u32 height);

		void Destroy(const VulkanContext* context) const;

		bool AcquireNextImageIndex(VulkanContext* context, u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex);

		void Present(VulkanContext* context, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);

		VkSwapchainKHR handle;

		VkSurfaceFormatKHR imageFormat;
		u32 imageCount;

		u8 maxFramesInFlight;

		Texture** renderTextures;

		/* @brief The depth texture. */
		Texture* depthTexture;
		/* @brief Render targets used for on-screen rendering, one per frame. */
		RenderTarget renderTargets[3];
	private:
		void CreateInternal(VulkanContext* context, u32 width, u32 height);

		void DestroyInternal(const VulkanContext* context) const;
		
		VkPresentModeKHR m_presentMode;
	};
}
