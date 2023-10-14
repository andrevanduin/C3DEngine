
#pragma once
#include <containers/dynamic_array.h>
#include <containers/string.h>
#include <core/defines.h>
#include <core/logger.h>
#include <vulkan/vulkan_core.h>

namespace C3D
{
    struct VulkanContext;

    struct VulkanPhysicalDeviceRequirements
    {
        bool graphicsQueue = false;
        bool presentQueue  = false;
        bool computeQueue  = false;
        bool transferQueue = false;

        bool samplerAnisotropy = false;
        bool discreteGpu       = false;

        DynamicArray<String> extensionNames;
    };

    struct VulkanPhysicalDeviceQueueFamilyInfo
    {
        i32 graphicsFamilyIndex = -1;
        i32 presentFamilyIndex  = -1;
        i32 computeFamilyIndex  = -1;
        i32 transferFamilyIndex = -1;
    };

    struct VulkanSwapChainSupportInfo
    {
        VkSurfaceCapabilitiesKHR capabilities;
        DynamicArray<VkSurfaceFormatKHR> formats;
        DynamicArray<VkPresentModeKHR> presentModes;
    };

    enum VulkanDeviceSupportFlagBits
    {
        VULKAN_DEVICE_SUPPORT_FLAG_NONE_BIT = 0,
        /** @brief Indicates if the device supports native dynamic topology (Must be using Vulkan API >= V1.3). */
        VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_TOPOLOGY_BIT = 1,
        /** @brief Indicates if the device supports dynamic topology by means of extension. */
        VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_TOPOLOGY_BIT = 2,
        /** @brief Indicates if the device supports smooth line rasterization. */
        VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERIZATION_BIT = 4,
        /** @brief Indicates if the device supports device local host visible memory. */
        VULKAN_DEVICE_SUPPORT_FLAG_DEVICE_LOCAL_HOST_VISIBILE_MEMORY_BIT = 8
    };

    using VulkanDeviceSupportFlags = u32;

    class VulkanDevice
    {
    public:
        VulkanDevice();

        bool Create(VulkanContext* context);

        void Destroy();

        bool DetectDepthFormat() const;
        void QuerySwapChainSupport() const;

        VkResult WaitIdle() const;

        bool HasSupportFor(VulkanDeviceSupportFlagBits feature) const;

        [[nodiscard]] i32 FindMemoryIndex(const u32 typeFilter, const u32 propertyFlags) const;

        VkDevice GetLogical() const { return m_logicalDevice; }

        VkCommandPool GetGraphicsCommandPool() const { return m_graphicsCommandPool; }

        VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
        u32 GetGraphicsQueueIndex() const { return m_graphicsQueueIndex; }

        VkQueue GetPresentQueue() const { return m_presentQueue; }
        u32 GetPresentQueueIndex() const { return m_presentQueueIndex; }

        VkQueue GetTransferQueue() const { return m_transferQueue; }
        u32 GetTransferQueueIndex() const { return m_transferQueueIndex; }

        VkQueue GetComputeQueue() const { return m_computeQueue; }
        u32 GetComputeQueueIndex() const { return m_computeQueueIndex; }

        VkFormat GetDepthFormat() const { return m_depthFormat; }
        u8 GetDepthChannelCount() const { return m_depthChannelCount; }

        const DynamicArray<VkSurfaceFormatKHR>& GetSurfaceFormats() const { return m_swapChainSupport.formats; }
        const DynamicArray<VkPresentModeKHR>& GetPresentModes() const { return m_swapChainSupport.presentModes; }
        const VkSurfaceCapabilitiesKHR& GetSurfaceCapabilities() const { return m_swapChainSupport.capabilities; }

        u64 GetMinUboAlignment() const { return m_properties.limits.minUniformBufferOffsetAlignment; }

    private:
        bool SelectPhysicalDevice();
        bool DoesPhysicalDeviceSupportRequirements(VkPhysicalDevice device, const VulkanPhysicalDeviceRequirements& requirements,
                                                   VulkanPhysicalDeviceQueueFamilyInfo& outQueueFamilyInfo);

        LoggerInstance<16> m_logger;

        VkPhysicalDevice m_physicalDevice = nullptr;
        VkDevice m_logicalDevice          = nullptr;

        VkCommandPool m_graphicsCommandPool = nullptr;

        mutable VkFormat m_depthFormat;
        mutable u8 m_depthChannelCount = 0;

        VulkanDeviceSupportFlags m_supportFlags;

        VkPhysicalDeviceProperties m_properties;
        VkPhysicalDeviceFeatures m_features;
        VkPhysicalDeviceMemoryProperties m_memory;

        mutable VulkanSwapChainSupportInfo m_swapChainSupport;

        /** @brief Boolean is set to true if we require the portability extension in order to run.
         * This extension is used for platforms that do not natively fully support Vulkan like Mac OS.
         */
        bool m_requiresPortability = false;

        /** @brief The different types of queues used by Vulkan. */
        VkQueue m_graphicsQueue = nullptr;
        VkQueue m_presentQueue  = nullptr;
        VkQueue m_transferQueue = nullptr;
        VkQueue m_computeQueue  = nullptr;

        /** @brief Indices for the different types of queues. */
        u32 m_graphicsQueueIndex = 0;
        u32 m_presentQueueIndex  = 0;
        u32 m_transferQueueIndex = 0;
        u32 m_computeQueueIndex  = 0;

        /** @brief The Vulkan API version supported by this device. */
        u32 m_apiVersionMajor = 0;
        u32 m_apiVersionMinor = 0;
        u32 m_apiVersionPatch = 0;

        /** @brief The Driver version supported by this device. */
        u32 m_driverVersionMajor = 0;
        u32 m_driverVersionMinor = 0;
        u32 m_driverVersionPatch = 0;

        VulkanContext* m_context;
    };
}  // namespace C3D
