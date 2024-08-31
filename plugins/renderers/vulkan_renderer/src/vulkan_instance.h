
#pragma once
#include <defines.h>

namespace C3D
{
    struct VulkanContext;

    namespace VulkanInstance
    {
        bool Create(VulkanContext& context, const char* applicationName, u32 applicationVersion);
    }
}  // namespace C3D