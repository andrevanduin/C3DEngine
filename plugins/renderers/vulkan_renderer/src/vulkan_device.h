
#pragma once
#include <core/defines.h>
#include <core/logger.h>
#include <vendor/VkBootstrap/VkBootstrap.h>

namespace C3D
{
    struct VulkanContext;

    struct VulkanSwapChainSupportInfo
    {
        VkSurfaceCapabilitiesKHR capabilities;

        u32 formatCount;

        VkSurfaceFormatKHR* formats;
        u32 presentModeCount;

        VkPresentModeKHR* presentModes;
    };

    class VulkanDevice
    {
    public:
        VulkanDevice();

        bool Create(vkb::Instance instance, VulkanContext* context, VkAllocationCallbacks* callbacks);

        void Destroy(const VulkanContext* context);

        void QuerySwapChainSupport(VkSurfaceKHR surface, VulkanSwapChainSupportInfo* supportInfo) const;

        bool DetectDepthFormat();

        void WaitIdle() const;

        VkPhysicalDevice physicalDevice = nullptr;
        VkDevice logicalDevice          = nullptr;

        VulkanSwapChainSupportInfo swapChainSupport;

        bool supportsDeviceLocalHostVisible = false;

        VkCommandPool graphicsCommandPool = nullptr;

        VkQueue graphicsQueue = nullptr;
        VkQueue presentQueue  = nullptr;
        VkQueue transferQueue = nullptr;

        VkFormat depthFormat;
        u8 depthChannelCount = 0;

        u32 graphicsQueueIndex = 0;
        u32 presentQueueIndex  = 0;
        u32 transferQueueIndex = 0;

        VkPhysicalDeviceProperties properties;

    private:
        LoggerInstance<16> m_logger;

        VkPhysicalDeviceFeatures m_features;
        VkPhysicalDeviceMemoryProperties m_memory;
    };
}  // namespace C3D
