
#include "vulkan_debugger.h"

#include <defines.h>
#include <logger/logger.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_utils.h"

namespace C3D
{
    bool VulkanDebugger::Create(VulkanContext& context)
    {
#if defined(_DEBUG)
        Logger::Info("[VULKAN_DEBUGGER] - Create() - Started.");

        u32 logSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

        u32 messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        debugCreateInfo.messageSeverity                    = logSeverity;
        debugCreateInfo.messageType                        = messageType;
        debugCreateInfo.pfnUserCallback                    = VulkanUtils::VkDebugLog;

        // Load the vkCreateDebugUtilsMessengerEXT func
        auto createDebugUtilsMessengerFunc =
            VulkanUtils::LoadExtensionFunction<PFN_vkCreateDebugUtilsMessengerEXT>(context.instance, "vkCreateDebugUtilsMessengerEXT");

        VK_CHECK(createDebugUtilsMessengerFunc(context.instance, &debugCreateInfo, context.allocator, &context.debugMessenger));

        // Load up our debug function pointers
        context.pfnSetDebugUtilsObjectNameEXT =
            VulkanUtils::LoadExtensionFunction<PFN_vkSetDebugUtilsObjectNameEXT>(context.instance, "vkSetDebugUtilsObjectNameEXT");
        context.pfnSetDebugUtilsObjectTagEXT =
            VulkanUtils::LoadExtensionFunction<PFN_vkSetDebugUtilsObjectTagEXT>(context.instance, "vkSetDebugUtilsObjectTagEXT");
        context.pfnCmdBeginDebugUtilsLabelEXT =
            VulkanUtils::LoadExtensionFunction<PFN_vkCmdBeginDebugUtilsLabelEXT>(context.instance, "vkCmdBeginDebugUtilsLabelEXT");
        context.pfnCmdEndDebugUtilsLabelEXT =
            VulkanUtils::LoadExtensionFunction<PFN_vkCmdEndDebugUtilsLabelEXT>(context.instance, "vkCmdEndDebugUtilsLabelEXT");

        Logger::Info("[VULKAN_DEBUGGER] - Create() - Successful.");
#endif

        return true;
    }

    void VulkanDebugger::Destroy(VulkanContext& context)
    {
#if defined(_DEBUG)
        if (context.debugMessenger)
        {
            Logger::Info("[VULKAN_DEBUGGER] - Destroying Vulkan Debugger.");

            auto destroyDebugUtilsMessengerFunc = VulkanUtils::LoadExtensionFunction<PFN_vkDestroyDebugUtilsMessengerEXT>(
                context.instance, "vkDestroyDebugUtilsMessengerEXT");

            destroyDebugUtilsMessengerFunc(context.instance, context.debugMessenger, context.allocator);
            context.debugMessenger = nullptr;
        }
#endif
    }
}  // namespace C3D