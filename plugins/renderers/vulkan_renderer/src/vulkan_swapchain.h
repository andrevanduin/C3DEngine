
#pragma once
#include <renderer/renderer_types.h>
#include <resources/texture.h>

#include "vulkan_image.h"

namespace C3D
{
    class SystemManager;
    struct VulkanContext;

    class VulkanSwapChain
    {
    public:
        void Create(const SystemManager* pSystemsManager, VulkanContext* context, u32 width, u32 height,
                    RendererConfigFlags flags);

        void Recreate(VulkanContext* context, u32 width, u32 height, RendererConfigFlags flags);

        void Destroy(const VulkanContext* context);

        bool AcquireNextImageIndex(VulkanContext* context, u64 timeoutNs, VkSemaphore imageAvailableSemaphore,
                                   VkFence fence, u32* outImageIndex);

        void Present(VulkanContext* context, VkQueue presentQueue, VkSemaphore renderCompleteSemaphore,
                     u32 presentImageIndex);

        VkSwapchainKHR handle = nullptr;

        VkSurfaceFormatKHR imageFormat;
        u32 imageCount = 0;

        u8 maxFramesInFlight = 0;

        Texture* renderTextures = nullptr;

        /* @brief An array of depth texture. */
        Texture* depthTextures = nullptr;
        /* @brief Render targets used for on-screen rendering, one per frame. */
        RenderTarget renderTargets[3] = {};

    private:
        void CreateInternal(const SystemManager* pSystemsManager, VulkanContext* context, u32 width, u32 height,
                            RendererConfigFlags flags);

        void DestroyInternal(const VulkanContext* context) const;

        VkPresentModeKHR GetPresentMode(const VulkanContext* context) const;

        RendererConfigFlags m_flags = 0;
        VkPresentModeKHR m_presentMode;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D
