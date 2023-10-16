
#pragma once
#include "core/defines.h"

#ifdef C3D_PLATFORM_LINUX
#include <containers/dynamic_array.h>
#include <containers/string.h>

namespace C3D
{
    struct VulkanContext;
    class SystemManager;

    namespace VulkanPlatform
    {
        bool CreateSurface(const SystemManager* pSystemsManager, VulkanContext& context);

        DynamicArray<const char*> GetRequiredExtensionNames();
    }  // namespace VulkanPlatform

}  // namespace C3D
#endif