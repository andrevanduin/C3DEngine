
#pragma once
#include "vulkan_types.h"

namespace C3D::VulkanUtils
{
    bool IsSuccess(VkResult result);

    const char* ResultString(VkResult result, bool getExtended = true);

#if defined(_DEBUG)
    VkBool32 VkDebugLog(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

    bool SetDebugObjectName(const VulkanContext* context, VkObjectType type, void* handle, const String& name);
    bool SetDebugObjectTag(const VulkanContext* context, VkObjectType type, void* handle, u64 tagSize,
                           const void* tagData);
    bool BeginCmdDebugLabel(const VulkanContext* context, VkCommandBuffer buffer, const String& label,
                            const vec4& color);
    bool EndCmdDebugLabel(const VulkanContext* context, VkCommandBuffer buffer);

#define VK_SET_DEBUG_OBJECT_NAME(context, type, handle, name) \
    VulkanUtils::SetDebugObjectName(context, type, handle, name)

#define VK_SET_DEBUG_OBJECT_TAG(context, type, handle, tagSize, tagData) \
    VulkanUtils::SetDebugObjectTag(context, type, handle, tagSize, tagData)

#define VK_BEGIN_CMD_DEBUG_LABEL(context, buffer, label, color) \
    VulkanUtils::BeginCmdDebugLabel(context, buffer, label, color)

#define VK_END_CMD_DEBUG_LABEL(context, buffer) VulkanUtils::EndCmdDebugLabel(context, buffer)

#else
/* Both macros simply do nothing in non-debug builds */
#define VK_SET_DEBUG_OBJECT_NAME(context, type, handle, name)
#define VK_SET_DEBUG_OBJECT_TAG(context, type, handle, tagSize, tagData)
#define VK_BEGIN_CMD_DEBUG_LABEL(context, buffer, label, color)
#define VK_END_CMD_DEBUG_LABEL(context, buffer)
#endif
}  // namespace C3D::VulkanUtils