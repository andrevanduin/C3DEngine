#pragma once
#include <containers/dynamic_array.h>
#include <defines.h>
#include <string/string.h>

namespace C3D
{
    struct VulkanContext;
    class SystemManager;

    namespace VulkanPlatform
    {
        bool CreateSurface(VulkanContext& context);

        DynamicArray<const char*> GetRequiredExtensionNames();
    }  // namespace VulkanPlatform

}  // namespace C3D