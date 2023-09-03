
#pragma once
#include <core/asserts.h>
#include <core/defines.h>
#include <vulkan/vulkan.h>

#include "vulkan_device.h"
#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"

#define VK_CHECK(expr)                   \
    {                                    \
        C3D_ASSERT((expr) == VK_SUCCESS) \
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
        VkInstance instance;
        VkAllocationCallbacks* allocator;
        VkSurfaceKHR surface;

        VulkanDevice device;
        VulkanSwapChain swapChain;

        DynamicArray<VulkanCommandBuffer> graphicsCommandBuffers;

        DynamicArray<VkSemaphore> imageAvailableSemaphores;
        DynamicArray<VkSemaphore> queueCompleteSemaphores;

        u32 inFlightFenceCount;
        VkFence inFlightFences[2];

        /* @brief Holds pointers to fences which exist and are owned elsewhere, one per frame */
        VkFence* imagesInFlight[3];

        u32 imageIndex;
        u32 currentFrame;

        u32 frameBufferWidth, frameBufferHeight;

        u64 frameBufferSizeGeneration;
        u64 frameBufferSizeLastGeneration;

        vec4 viewportRect;
        vec4 scissorRect;

        /* @brief Render targets used for world rendering. One per frame. */
        RenderTarget worldRenderTargets[3];

        /* @brief Indicates if multiThreading is supported by this device. */
        bool multiThreadingEnabled;
        bool recreatingSwapChain;
        bool renderFlagChanged;

#if defined(_DEBUG)
        /** @brief Function pointer to set debug object names. */
        PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
        /** @brief Function pointer to set debug object tag data. */
        PFN_vkSetDebugUtilsObjectTagEXT pfnSetDebugUtilsObjectTagEXT;
        /** @brief Function pointer to set the start of a debug label for a cmd. */
        PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
        /** @brief Function pointer to set the end of a debug label for a cmd. */
        PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
#endif
        [[nodiscard]] i32 FindMemoryIndex(const u32 typeFilter, const u32 propertyFlags) const
        {
            VkPhysicalDeviceMemoryProperties memoryProperties;
            vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);

            for (u32 i = 0; i < memoryProperties.memoryTypeCount; i++)
            {
                if (typeFilter & 1 << i &&
                    (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
                {
                    return static_cast<i32>(i);
                }
            }
            return -1;
        }
    };
}  // namespace C3D