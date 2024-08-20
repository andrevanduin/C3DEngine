
#pragma once
#include <core/asserts.h>
#include <core/defines.h>
#include <vulkan/vulkan.h>

#include "vulkan_buffer.h"
#include "vulkan_device.h"
#include "vulkan_renderpass.h"
#include "vulkan_swapchain.h"

#define VK_CHECK(expr) { C3D_ASSERT((expr) == VK_SUCCESS) }

struct shaderc_compiler;

namespace C3D
{
    class VulkanImage;
    struct Shader;

    struct VulkanTextureData
    {
        // Internal Vulkan image.
        VulkanImage image;
    };

    enum VulkanTopologyClass
    {
        VULKAN_TOPOLOGY_CLASS_POINT    = 0,
        VULKAN_TOPOLOGY_CLASS_LINE     = 1,
        VULKAN_TOPOLOGY_CLASS_TRIANGLE = 2,
        VULKAN_TOPOLOGY_CLASS_MAX      = 3
    };

    struct VulkanContext
    {
        VkInstance instance;
        VkSurfaceKHR surface;
        VkAllocationCallbacks* allocator = nullptr;

        /** @brief A boolean indicating if we should be using validation layers. */
        bool useValidationLayers = false;

        VulkanDevice device;
        VulkanSwapchain swapchain;

        DynamicArray<VulkanCommandBuffer> graphicsCommandBuffers;

        DynamicArray<VkSemaphore> imageAvailableSemaphores;
        DynamicArray<VkSemaphore> queueCompleteSemaphores;

        u32 inFlightFenceCount = 0;
        VkFence inFlightFences[2];

        /** @brief Holds pointers to fences which exist and are owned elsewhere, one per frame. */
        VkFence imagesInFlight[3];

        /** @brief A dynamic array containing all used samplers. */
        DynamicArray<VkSampler> samplers;

        u32 imageIndex           = 0;
        mutable u32 currentFrame = 0;

        u32 frameBufferWidth = 0, frameBufferHeight = 0;

        u64 frameBufferSizeGeneration     = INVALID_ID_U64;
        u64 frameBufferSizeLastGeneration = INVALID_ID_U64;

        vec4 viewportRect;
        vec4 scissorRect;

        /** @brief Render targets used for world rendering. One per frame. */
        RenderTarget worldRenderTargets[3];

        /** @brief Indicates if multiThreading is supported by this device. */
        bool multiThreadingEnabled = false;
        bool recreatingSwapChain   = false;
        bool renderFlagChanged     = false;

        /** @brief Version of vulkan as reported by the Vulkan Instance. */
        u32 apiMajor = 0, apiMinor = 0, apiPatch = 0;

#if defined(_DEBUG)
        /** @brief Function pointer to set debug object names. */
        PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
        /** @brief Function pointer to set debug object tag data. */
        PFN_vkSetDebugUtilsObjectTagEXT pfnSetDebugUtilsObjectTagEXT;
        /** @brief Function pointer to set the start of a debug label for a cmd. */
        PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
        /** @brief Function pointer to set the end of a debug label for a cmd. */
        PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;

        VkDebugUtilsMessengerEXT debugMessenger;
#endif

        PFN_vkCmdSetPrimitiveTopologyEXT pfnCmdSetPrimitiveTopologyEXT;
        PFN_vkCmdSetFrontFaceEXT pfnCmdSetFrontFaceEXT;
        PFN_vkCmdSetStencilTestEnableEXT pfnCmdSetStencilTestEnableEXT;
        PFN_vkCmdSetDepthTestEnableEXT pfnCmdSetDepthTestEnableEXT;
        PFN_vkCmdSetStencilOpEXT pfnCmdSetStencilOpEXT;

        /** @brief A pointer to the currently bound shader. */
        const Shader* boundShader;

        /** @brief The ShaderC lib to dynamically compile vulkan shaders. */
        shaderc_compiler* shaderCompiler;

        /** @brief A reusable staging buffer to transfer data from a resource to the GPU-only buffer. */
        mutable VulkanBuffer stagingBuffer = VulkanBuffer(this, "VULKAN_STAGING_BUFFER");
    };
}  // namespace C3D