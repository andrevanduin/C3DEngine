
#pragma once
#include <renderer/render_target.h>
#include <renderer/renderer_types.h>

#include "vulkan_image.h"

namespace C3D
{
    class SystemManager;
    struct VulkanContext;
    struct Texture;

    class VulkanSwapchain
    {
    public:
        void Create(const SystemManager* pSystemsManager, const VulkanContext* context, u32 width, u32 height, RendererConfigFlags flags);

        void Recreate(u32 width, u32 height, RendererConfigFlags flags);

        void Destroy();

        bool AcquireNextImageIndex(u64 timeoutNs, VkSemaphore imageAvailableSemaphore, VkFence fence, u32* outImageIndex);

        void Present(VkQueue presentQueue, VkSemaphore renderCompleteSemaphore, u32 presentImageIndex);

        VkSwapchainKHR handle = nullptr;

        VkSurfaceFormatKHR imageFormat;
        u32 imageCount = 0;

        u8 maxFramesInFlight = 0;

        Texture* renderTextures = nullptr;

        /** @brief An array of depth texture. */
        Texture* depthTextures = nullptr;
        /** @brief Render targets used for on-screen rendering, one per frame. */
        RenderTarget renderTargets[3] = {};

    private:
        void CreateInternal(u32 width, u32 height, RendererConfigFlags flags);

        void DestroyInternal() const;

        VkPresentModeKHR GetPresentMode() const;
        VkSurfaceFormatKHR GetSurfaceFormat() const;

        RendererConfigFlags m_flags = 0;
        VkPresentModeKHR m_presentMode;

        const SystemManager* m_pSystemsManager = nullptr;
        const VulkanContext* m_context         = nullptr;
    };
}  // namespace C3D
