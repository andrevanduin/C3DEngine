
#pragma once
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

		void Create(VulkanContext* context, u32 width, u32 height, RendererConfigFlags flags);

		void Recreate(VulkanContext* context, u32 width, u32 height, RendererConfigFlags flags);

		void Destroy(const VulkanContext* context);

		bool AcquireNextImageIndex(VulkanContext* context, u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex);

		void Present(VulkanContext* context, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);

		VkSwapchainKHR handle;

		VkSurfaceFormatKHR imageFormat;
		u32 imageCount;

		u8 maxFramesInFlight;

		Texture* renderTextures;

		/* @brief An array of depth texture. */
		Texture* depthTextures;
		/* @brief Render targets used for on-screen rendering, one per frame. */
		RenderTarget renderTargets[3];
	private:
		void CreateInternal(VulkanContext* context, u32 width, u32 height, RendererConfigFlags flags);

		void DestroyInternal(const VulkanContext* context) const;

		VkPresentModeKHR GetPresentMode(const VulkanContext* context) const;

		RendererConfigFlags m_flags;
		VkPresentModeKHR m_presentMode;
	};
}
