
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanUtils
{
    bool IsSuccess(VkResult result);

    const char* ResultString(VkResult result, bool getExtended = true);

    template <typename T>
    T LoadExtensionFunction(VkInstance instance, const char* name)
    {
        static_assert(std::is_pointer_v<T>, "LoadVulkanExtensionFunction only accepts a template argument of type pointer.");

        T func = reinterpret_cast<T>(vkGetInstanceProcAddr(instance, name));
        if (!func)
        {
            Logger::Fatal("LoadVulkanExtensionFunction() - Failed to obtain extension function: '{}'.", name);
            return nullptr;
        }

        return func;
    }

#if defined(_DEBUG)
    const char* VkMessageTypeToString(VkDebugUtilsMessageTypeFlagsEXT s);

    VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    void SetDebugObjectName(const VulkanContext* context, VkObjectType type, void* handle, const String& name);
    void SetDebugObjectTag(const VulkanContext* context, VkObjectType type, void* handle, u64 tagSize, const void* tagData);
    void BeginCmdDebugLabel(const VulkanContext* context, VkCommandBuffer buffer, const String& label, const vec4& color);
    void EndCmdDebugLabel(const VulkanContext* context, VkCommandBuffer buffer);

#define VK_SET_DEBUG_OBJECT_NAME(context, type, handle, name) VulkanUtils::SetDebugObjectName(context, type, handle, name)

#define VK_SET_DEBUG_OBJECT_TAG(context, type, handle, tagSize, tagData) \
    VulkanUtils::SetDebugObjectTag(context, type, handle, tagSize, tagData)

#define VK_BEGIN_CMD_DEBUG_LABEL(context, buffer, label, color) VulkanUtils::BeginCmdDebugLabel(context, buffer, label, color)

#define VK_END_CMD_DEBUG_LABEL(context, buffer) VulkanUtils::EndCmdDebugLabel(context, buffer)

#else
/* Both macros simply do nothing in non-debug builds */
#define VK_SET_DEBUG_OBJECT_NAME(context, type, handle, name)
#define VK_SET_DEBUG_OBJECT_TAG(context, type, handle, tagSize, tagData)
#define VK_BEGIN_CMD_DEBUG_LABEL(context, buffer, label, color)
#define VK_END_CMD_DEBUG_LABEL(context, buffer)
#endif
}  // namespace C3D::VulkanUtils