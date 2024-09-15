
#include "vulkan_instance.h"

#include <string/string_utils.h>

#include <algorithm>

#include "platform/vulkan_platform.h"
#include "vulkan_types.h"
#include "vulkan_utils.h"

namespace C3D
{
    bool VulkanInstance::Create(VulkanContext& context, const char* applicationName, u32 applicationVersion)
    {
        u32 apiVersion = 0;
        vkEnumerateInstanceVersion(&apiVersion);

        context.apiMajor = VK_API_VERSION_MAJOR(apiVersion);
        context.apiMinor = VK_API_VERSION_MINOR(apiVersion);
        context.apiPatch = VK_API_VERSION_PATCH(apiVersion);

        VkApplicationInfo appInfo  = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.apiVersion         = apiVersion;
        appInfo.pApplicationName   = applicationName;
        appInfo.applicationVersion = applicationVersion;
        appInfo.pEngineName        = "C3DEngine";
        appInfo.engineVersion      = VK_MAKE_VERSION(0, 4, 0);

        VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        createInfo.pApplicationInfo     = &appInfo;

        // First we obtain the platform-specific extensions that we require
        auto requiredExtensions = VulkanPlatform::GetRequiredExtensionNames();
        // Add the defauult required surface extension
        requiredExtensions.EmplaceBack(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(_DEBUG)
        // If we are building in DEBUG mode we want to add the debug utilities extension
        requiredExtensions.EmplaceBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        INFO_LOG("Required instance extensions that need to be loaded: {}.", StringUtils::Join(requiredExtensions, ", "));

        // Check if all our required instance extensions are available
        u32 availableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);

        DynamicArray<VkExtensionProperties> availableExtensions;
        availableExtensions.Resize(availableExtensionCount);

        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.GetData());

        for (auto requiredExtension : requiredExtensions)
        {
            auto index = std::find_if(availableExtensions.begin(), availableExtensions.end(),
                                      [requiredExtension](const VkExtensionProperties& props) {
                                          return StringUtils::Equals(props.extensionName, requiredExtension);
                                      });
            if (index == availableExtensions.end())
            {
                ERROR_LOG("Required extension: '{}' is not available.", requiredExtension);
                return false;
            }

            INFO_LOG("Required extension: '{}' was found.", requiredExtension);
        }

        INFO_LOG("All required extensions are present.");

        createInfo.enabledExtensionCount   = requiredExtensions.Size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.GetData();

        DynamicArray<const char*> requiredValidationLayerNames;

        if (context.useValidationLayers)
        {
            // If we are require validation layers we add them here
            INFO_LOG("Validation layers are enabled.");
            requiredValidationLayerNames.EmplaceBack("VK_LAYER_KHRONOS_validation");
            // NOTE: If needed for debugging we can enable this validation layer
            // requiredValidationLayerNames.EmplaceBack("VK_LAYER_LUNARG_api_dump");

            INFO_LOG("Required instance layers that need to be loaded: {}.", StringUtils::Join(requiredValidationLayerNames, ", "));

            // Check if all our required validation layers are available
            u32 availableLayerCount = 0;
            VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr));

            DynamicArray<VkLayerProperties> availableLayers;
            availableLayers.Resize(availableLayerCount);

            VK_CHECK(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.GetData()));

            for (auto requiredValidationLayerName : requiredValidationLayerNames)
            {
                auto index = std::find_if(availableLayers.begin(), availableLayers.end(),
                                          [requiredValidationLayerName](const VkLayerProperties& props) {
                                              return StringUtils::Equals(props.layerName, requiredValidationLayerName);
                                          });
                if (index == availableLayers.end())
                {
                    ERROR_LOG("Required layer: '{}' is not available.", requiredValidationLayerName);
                    return false;
                }

                INFO_LOG("Required layer: '{}' was found.", requiredValidationLayerName);
            }

            INFO_LOG("All required layers are present.");
        }

        createInfo.enabledLayerCount   = requiredValidationLayerNames.Size();
        createInfo.ppEnabledLayerNames = requiredValidationLayerNames.GetData();

#if C3D_PLATFORM_APPLE == 1
        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

        auto result = vkCreateInstance(&createInfo, context.allocator, &context.instance);
        if (!VulkanUtils::IsSuccess(result))
        {
            auto resultString = VulkanUtils::ResultString(result);
            ERROR_LOG("vkCreateInstance failed with the following error: '{}'.", resultString);
            return false;
        }

        INFO_LOG("Vulkan Instance created.");
        return true;
    }

}  // namespace C3D