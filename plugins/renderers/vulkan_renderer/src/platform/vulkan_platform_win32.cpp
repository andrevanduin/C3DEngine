
#include <defines.h>

#ifdef C3D_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Undef Windows macros that cause issues with C3D Engine
#undef CopyFile
#undef max
#undef min
#undef RGB
#undef CreateWindow

#include <platform/platform.h>
#include <systems/system_manager.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "vulkan_platform.h"
#include "vulkan_types.h"
#include "vulkan_utils.h"

namespace C3D::VulkanPlatform
{
    struct Win32HandleInfo
    {
        HINSTANCE hInstance = nullptr;
        HWND hwnd           = nullptr;
    };

    bool CreateSurface(VulkanContext& context)
    {
        auto handle = static_cast<Win32HandleInfo*>(Platform::GetHandleInfo());

        VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
        createInfo.hinstance                   = handle->hInstance;
        createInfo.hwnd                        = handle->hwnd;

        auto result = vkCreateWin32SurfaceKHR(context.instance, &createInfo, context.allocator, &context.surface);
        if (!VulkanUtils::IsSuccess(result))
        {
            Logger::Error("[VULKAN_PLATFORM] - CreateSurface() - vkCreateWin32SurfaceKHR failed with the following error: '{}'.",
                          VulkanUtils::ResultString(result));
            return false;
        }

        return true;
    }

    DynamicArray<const char*> GetRequiredExtensionNames()
    {
        auto requiredExtensions = DynamicArray<const char*>();
        requiredExtensions.EmplaceBack("VK_KHR_win32_surface");
        return requiredExtensions;
    }
}  // namespace C3D::VulkanPlatform
#endif