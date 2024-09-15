
#include "defines.h"

#ifdef C3D_PLATFORM_LINUX
#include <xcb/xcb.h>

#include "vulkan_platform_linux.h"

// For surface creation
#define VK_USE_PLATFORM_XCB_KHR
#include <platform/platform.h>
#include <systems/system_manager.h>
#include <vulkan/vulkan.h>

#include "vulkan_types.h"
#include "vulkan_utils.h"

namespace C3D::VulkanPlatform
{
    struct LinuxHandleInfo
    {
    };

    bool CreateSurface(VulkanContext& context)
    {
        LinuxHandleInfo& handle = *static_cast<LinuxHandleInfo>(Platform::GetHandleInfo());

        VkXcbSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.connection                = handle.connection;
        createInfo.window                    = handle.window;

        auto result = vkCreateXcbSurfaceKHR(context.instance, &createInfo, context.allocator, &context.surface);
        if (!VulkanUtils::IsSuccess(result))
        {
            Logger::Error("[VULKAN_PLATFORM] - CreateSurface() - vkCreateXcbSurfaceKHR failed with the following error: '{}'.",
                          VulkanUtils::ResultString(result));
            return false;
        }

        return true;
    }

    DynamicArray<const char*> GetRequiredExtensionNames()
    {
        auto requiredExtensions = DynamicArray<const char*>();
        requiredExtensions.EmplaceBack("VK_KHR_xcb_surface");
        return requiredExtensions;
    }
}  // namespace C3D::VulkanPlatform

#endif