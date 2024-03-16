
#include "vulkan_device.h"

#include <core/logger.h>
#include <core/metrics/metrics.h>
#include <systems/system_manager.h>

#include "vulkan_utils.h"

namespace C3D
{
    constexpr auto VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME = "VK_KHR_portability_subset";

    constexpr const char* INSTANCE_NAME = "VULKAN_DEVICE";

    bool VulkanDevice::Create(VulkanContext* context)
    {
        m_context = context;

        if (!SelectPhysicalDevice())
        {
            ERROR_LOG("Failed to select Physical device.");
            return false;
        }

        INFO_LOG("Creating logical device.");

        u32 indexCount = 0;
        u32 indices[8];

        // We always need at least the graphics queue
        indices[indexCount] = m_graphicsQueueIndex;
        indexCount++;

        if (m_graphicsQueueIndex != m_presentQueueIndex)
        {
            // Our present queue does not share with the graphics queue
            indices[indexCount] = m_presentQueueIndex;
            indexCount++;
        }
        if (m_graphicsQueueIndex != m_transferQueueIndex)
        {
            // Our transfer queue does not share with the graphics queue
            indices[indexCount] = m_transferQueueIndex;
            indexCount++;
        }

        VkDeviceQueueCreateInfo queueCreateInfos[8];
        f32 queuePriorities[2] = { 0.9f, 1.0f };
        for (u32 i = 0; i < indexCount; i++)
        {
            auto& queueCreateInfo = queueCreateInfos[i];

            queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = indices[i];
            queueCreateInfo.queueCount       = 1;

            if (m_graphicsQueueIndex == m_presentQueueIndex && indices[i] == m_presentQueueIndex)
            {
                queueCreateInfo.queueCount = 2;
            }

            // TODO: Future enhancement with multiple graphics queue count

            queueCreateInfo.flags            = 0;
            queueCreateInfo.pNext            = nullptr;
            queueCreateInfo.pQueuePriorities = queuePriorities;
        }

        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.samplerAnisotropy        = VK_TRUE;  // Request Anisotropy

        DynamicArray<const char*> requestedExtensions(4);
        // We always require the swapchain extension
        requestedExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // If we require portability (on Mac OS for example) we add the extension for it
        if (m_requiresPortability)
        {
            requestedExtensions.PushBack(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
        }

        auto dynamicTopologyResult = CheckForSupportAndAddExtensionIfNeeded(
            "Dynamic Topology", VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_TOPOLOGY_BIT, VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_TOPOLOGY_BIT,
            VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, requestedExtensions);

        if (dynamicTopologyResult == VulkanDeviceSupportResult::Extension)
        {
            // We need to use an extension to get the functionality so let's get the function pointer for it
            m_context->pfnCmdSetPrimitiveTopologyEXT =
                VulkanUtils::LoadExtensionFunction<PFN_vkCmdSetPrimitiveTopologyEXT>(m_context->instance, "vkCmdSetPrimitiveTopologyEXT");
        }

        auto frontFaceResult = CheckForSupportAndAddExtensionIfNeeded(
            "Dynamic Front Face Swapping", VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_FRONT_FACE_BIT,
            VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_FRONT_FACE_BIT, VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME, requestedExtensions);

        if (frontFaceResult == VulkanDeviceSupportResult::Extension)
        {
            // We need to use an extension to get the functionality so let's get the function pointer for it
            m_context->pfnCmdSetFrontFaceEXT =
                VulkanUtils::LoadExtensionFunction<PFN_vkCmdSetFrontFaceEXT>(m_context->instance, "vkCmdSetFrontFaceEXT");
        }

        // If we support smooth rasterization of lines we load the extension
        if (HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERIZATION_BIT))
        {
            INFO_LOG("We have support for smooth line rasterization through the: '{}' extension. We are enabling it!",
                     VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME);

            requestedExtensions.PushBack(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME);
        }

        VkDeviceCreateInfo deviceCreateInfo      = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        deviceCreateInfo.queueCreateInfoCount    = indexCount;
        deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos;
        deviceCreateInfo.pEnabledFeatures        = &deviceFeatures;
        deviceCreateInfo.enabledExtensionCount   = requestedExtensions.Size();
        deviceCreateInfo.ppEnabledExtensionNames = requestedExtensions.GetData();

        // These are deprecated and ignored so we null them
        deviceCreateInfo.enabledLayerCount   = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;

        VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicState = {
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT
        };
        extendedDynamicState.extendedDynamicState = VK_TRUE;
        deviceCreateInfo.pNext                    = &extendedDynamicState;

        if (HasSupportFor(VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERIZATION_BIT))
        {
            // If we support smooth lines we add the required structures to the pNext
            VkPhysicalDeviceLineRasterizationFeaturesEXT lineRasterization = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT
            };
            lineRasterization.smoothLines = VK_TRUE;
            extendedDynamicState.pNext    = &lineRasterization;
        }

        // Actually create our logical device
        VK_CHECK(vkCreateDevice(m_physicalDevice, &deviceCreateInfo, m_context->allocator, &m_logicalDevice));

        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_DEVICE, m_logicalDevice, "VULKAN_LOGICAL_DEVICE");

        INFO_LOG("Logical Device created.");

        vkGetDeviceQueue(m_logicalDevice, m_graphicsQueueIndex, 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_logicalDevice, m_presentQueueIndex, m_graphicsQueueIndex == m_presentQueueIndex ? 1 : 0, &m_presentQueue);
        vkGetDeviceQueue(m_logicalDevice, m_transferQueueIndex, 0, &m_transferQueue);

        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_QUEUE, m_graphicsQueue, "VULKAN_GRAHPICS_QUEUE");
        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_QUEUE, m_presentQueue, "VULKAN_PRESENT_QUEUE");
        VK_SET_DEBUG_OBJECT_NAME(m_context, VK_OBJECT_TYPE_QUEUE, m_transferQueue, "VULKAN_TRANSFER_QUEUE");

        INFO_LOG("Queues obtained.");

        VkCommandPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        poolCreateInfo.queueFamilyIndex        = m_graphicsQueueIndex;
        poolCreateInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK(vkCreateCommandPool(m_logicalDevice, &poolCreateInfo, context->allocator, &m_graphicsCommandPool));
        INFO_LOG("Graphics command pool created.");

        return true;
    }

    void VulkanDevice::Destroy()
    {
        INFO_LOG("Destroying Queue indices.");
        m_graphicsQueueIndex = -1;
        m_presentQueueIndex  = -1;
        m_transferQueueIndex = -1;

        INFO_LOG("Destroying command pool.");
        vkDestroyCommandPool(m_logicalDevice, m_graphicsCommandPool, m_context->allocator);

        INFO_LOG("Destroying Logical Device.");
        vkDestroyDevice(m_logicalDevice, m_context->allocator);
        m_logicalDevice = nullptr;

        INFO_LOG("Releasing Physical Device Handle.");
        m_physicalDevice = nullptr;

        INFO_LOG("Destroying SwapChainSupport formats and present modes.");
        m_swapChainSupport.formats.Destroy();
        m_swapChainSupport.presentModes.Destroy();
    }

    bool VulkanDevice::DetectDepthFormat() const
    {
        constexpr u64 candidateCount                  = 3;
        constexpr VkFormat candidates[candidateCount] = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
        constexpr u8 sizes[candidateCount]            = { 4, 4, 3 };

        constexpr u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        for (u64 i = 0; i < candidateCount; i++)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_physicalDevice, candidates[i], &props);

            if ((props.linearTilingFeatures & flags) == flags)
            {
                m_depthFormat       = candidates[i];
                m_depthChannelCount = sizes[i];
                return true;
            }
            if ((props.optimalTilingFeatures & flags) == flags)
            {
                m_depthFormat       = candidates[i];
                m_depthChannelCount = sizes[i];
                return true;
            }
        }

        return false;
    }

    VkResult VulkanDevice::WaitIdle() const { return vkDeviceWaitIdle(m_logicalDevice); }

    bool VulkanDevice::HasSupportFor(const VulkanDeviceSupportFlagBits feature) const { return m_supportFlags & feature; }

    i32 VulkanDevice::FindMemoryIndex(const u32 typeFilter, const u32 propertyFlags) const
    {
        for (u32 i = 0; i < m_memory.memoryTypeCount; i++)
        {
            if (typeFilter & 1 << i && (m_memory.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
            {
                return static_cast<i32>(i);
            }
        }
        return -1;
    }

    const char* VkPhysicalDeviceTypeToString(VkPhysicalDeviceType type)
    {
        switch (type)
        {
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                return "CPU";
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                return "Integrated";
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                return "Discrete";
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                return "Virtual";
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
            default:
                return "Unknown";
        }
    }

    bool VulkanDevice::SelectPhysicalDevice()
    {
        u32 physicalDeviceCount = 0;
        VK_CHECK(vkEnumeratePhysicalDevices(m_context->instance, &physicalDeviceCount, nullptr));

        if (physicalDeviceCount == 0)
        {
            ERROR_LOG("No phyiscal devices that support Vulkan were found.");
            return false;
        }

        // TODO: These requirements should be driven by the engine's configuration
        VulkanPhysicalDeviceRequirements requirements;
        requirements.graphicsQueue = true;
        requirements.presentQueue  = true;
        requirements.transferQueue = true;
        // NOTE: Currently we don't use compute shaders but when we want to start we need to enable this flag
        requirements.computeQueue      = true;
        requirements.samplerAnisotropy = true;

#if C3D_PLATFORM_APPLE
        requirements.discreteGpu = false;
#else
        requirements.discreteGpu = true;
#endif

        requirements.extensionNames.EmplaceBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // Iterate physical devices to find one that we can use
        VkPhysicalDevice physicalDevices[32];
        VK_CHECK(vkEnumeratePhysicalDevices(m_context->instance, &physicalDeviceCount, physicalDevices));

        for (u32 i = 0; i < physicalDeviceCount; i++)
        {
            m_physicalDevice = physicalDevices[i];

            vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);

            INFO_LOG("Evaluating device: '{}'.", m_properties.deviceName);

            vkGetPhysicalDeviceFeatures(m_physicalDevice, &m_features);

            VkPhysicalDeviceFeatures2 features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            // Check for dynamic topology support via extension
            VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateNext = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT
            };
            features2.pNext = &dynamicStateNext;
            // Check for smooth line rasterisation support via extension
            VkPhysicalDeviceLineRasterizationFeaturesEXT smoothLineNext = {
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT
            };
            dynamicStateNext.pNext = &smoothLineNext;
            // Perform the query
            vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);

            vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &m_memory);

            bool supportsDeviceLocalHostVisible = false;
            for (auto& memoryType : m_memory.memoryTypes)
            {
                // Check each memory type to see if its bit is set to 1.
                if (((memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) &&
                    ((memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0))
                {
                    supportsDeviceLocalHostVisible = true;
                    break;
                }
            }

            VulkanPhysicalDeviceQueueFamilyInfo queueInfo;
            if (!DoesPhysicalDeviceSupportRequirements(m_physicalDevice, requirements, queueInfo))
            {
                m_physicalDevice = nullptr;
                continue;
            }

            INFO_LOG("Selected is:");

            f32 gpuMemory = 0.0f;
            for (u32 i = 0; i < m_memory.memoryHeapCount; i++)
            {
                const auto heap = m_memory.memoryHeaps[i];
                if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                {
                    gpuMemory += static_cast<f32>(heap.size) / 1024.0f / 1024.0f / 1024.0f;
                }
            }

            Metrics.SetAllocatorAvailableSpace(GPU_ALLOCATOR_ID, GibiBytes(static_cast<u32>(gpuMemory)));

            m_driverVersionMajor = VK_VERSION_MAJOR(m_properties.driverVersion);
            m_driverVersionMinor = VK_VERSION_MINOR(m_properties.driverVersion);
            m_driverVersionPatch = VK_VERSION_PATCH(m_properties.driverVersion);

            m_apiVersionMajor = VK_API_VERSION_MAJOR(m_properties.apiVersion);
            m_apiVersionMinor = VK_API_VERSION_MINOR(m_properties.apiVersion);
            m_apiVersionPatch = VK_API_VERSION_PATCH(m_properties.apiVersion);

            INFO_LOG("GPU            - {}", m_properties.deviceName);
            INFO_LOG("Type           - {}", VkPhysicalDeviceTypeToString(m_properties.deviceType));
            INFO_LOG("GPU Memory     - {:.2f}GiB", gpuMemory);
            INFO_LOG("Driver Version - {}.{}.{}", m_driverVersionMajor, m_driverVersionMinor, m_driverVersionPatch);
            INFO_LOG("API Version    - {}.{}.{}", m_apiVersionMajor, m_apiVersionMinor, m_apiVersionPatch);

            m_graphicsQueueIndex = queueInfo.graphicsFamilyIndex;
            m_presentQueueIndex  = queueInfo.presentFamilyIndex;
            m_transferQueueIndex = queueInfo.transferFamilyIndex;
            m_computeQueueIndex  = queueInfo.computeFamilyIndex;

            if (supportsDeviceLocalHostVisible)
            {
                m_supportFlags |= VULKAN_DEVICE_SUPPORT_FLAG_DEVICE_LOCAL_HOST_VISIBILE_MEMORY_BIT;
            }
            if (dynamicStateNext.extendedDynamicState)
            {
                // Both of these are part of the extended dynamic state extension
                m_supportFlags |= VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_TOPOLOGY_BIT;
                m_supportFlags |= VULKAN_DEVICE_SUPPORT_FLAG_DYNAMIC_FRONT_FACE_BIT;
            }
            if (m_apiVersionMajor == 1 && m_apiVersionMinor >= 3)
            {
                // If we are using Vulkan API >= 1.3 we have native support for both of these
                m_supportFlags |= VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_TOPOLOGY_BIT;
                m_supportFlags |= VULKAN_DEVICE_SUPPORT_FLAG_NATIVE_DYNAMIC_FRONT_FACE_BIT;
            }
            if (smoothLineNext.smoothLines)
            {
                m_supportFlags |= VULKAN_DEVICE_SUPPORT_FLAG_LINE_SMOOTH_RASTERIZATION_BIT;
            }

            break;
        }

        if (!m_physicalDevice)
        {
            ERROR_LOG("Failed to find a suitable PhysicalDevice.");
            return false;
        }

        return true;
    }

    bool VulkanDevice::DoesPhysicalDeviceSupportRequirements(VkPhysicalDevice device, const VulkanPhysicalDeviceRequirements& requirements,
                                                             VulkanPhysicalDeviceQueueFamilyInfo& outQueueFamilyInfo)
    {
        // We are checking the next device so we reset our portability flag.
        m_requiresPortability = false;
        auto surface          = m_context->surface;

        if (requirements.discreteGpu && m_properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            INFO_LOG("Skipping: '{}' since it's not a discrete GPU which is a requirement.", m_properties.deviceName);
            return false;
        }

        u32 queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        VkQueueFamilyProperties queueFamilies[32];
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);

        // Look at each queue and check what it supports
        u8 minTransferScore = 255;
        for (u8 i = 0; i < queueFamilyCount; i++)
        {
            auto& queueFamily = queueFamilies[i];

            u8 currentTransferScore = 0;

            // Graphics queue
            if (outQueueFamilyInfo.graphicsFamilyIndex == -1 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                outQueueFamilyInfo.graphicsFamilyIndex = i;
                currentTransferScore++;

                // If it's also a present queue we make sure to prioritize the grouping
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
                if (supportsPresent)
                {
                    outQueueFamilyInfo.presentFamilyIndex = i;
                    currentTransferScore++;
                }
            }

            // Compute queue
            if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                outQueueFamilyInfo.computeFamilyIndex = i;
                currentTransferScore++;
            }

            // Transfer queue
            if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)
            {
                // Take the index if its the current lowest. This increases the likelyhood that it is a dedicated
                // transfer queue.
                if (currentTransferScore <= minTransferScore)
                {
                    minTransferScore                       = currentTransferScore;
                    outQueueFamilyInfo.transferFamilyIndex = i;
                }
            }
        }

        // If we have not yet found a present queue, we iterate again and take the first one.
        // This should only happen when we found a queue that supports grahpics but not present.
        if (outQueueFamilyInfo.presentFamilyIndex == -1)
        {
            for (u8 i = 0; i < queueFamilyCount; i++)
            {
                auto& queueFamily        = queueFamilies[i];
                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent));
                if (supportsPresent)
                {
                    outQueueFamilyInfo.presentFamilyIndex = i;

                    if (outQueueFamilyInfo.presentFamilyIndex != outQueueFamilyInfo.graphicsFamilyIndex)
                    {
                        WARN_LOG("Present and Graphics queue indices do not match!");
                    }
                    break;
                }
            }
        }

        INFO_LOG("Name: '{}' | Graphics: {} | Present: {} | Compute: {} | Transfer: {}.", m_properties.deviceName,
                 outQueueFamilyInfo.graphicsFamilyIndex != -1, outQueueFamilyInfo.presentFamilyIndex != -1,
                 outQueueFamilyInfo.computeFamilyIndex != -1, outQueueFamilyInfo.transferFamilyIndex != -1);

        if (requirements.graphicsQueue && outQueueFamilyInfo.graphicsFamilyIndex == -1)
        {
            INFO_LOG("Device does not support Graphics Queue as required.");
            return false;
        }

        if (requirements.presentQueue && outQueueFamilyInfo.presentFamilyIndex == -1)
        {
            INFO_LOG("Device does not support Present Queue as required.");
            return false;
        }

        if (requirements.transferQueue && outQueueFamilyInfo.transferFamilyIndex == -1)
        {
            INFO_LOG("Device does not support Transfer Queue as required.");
            return false;
        }

        if (requirements.computeQueue && outQueueFamilyInfo.computeFamilyIndex == -1)
        {
            INFO_LOG("Device does not support Compute Queue as required.");
            return false;
        }

        QuerySwapChainSupport();

        if (m_swapChainSupport.formats.Empty() || m_swapChainSupport.presentModes.Empty())
        {
            INFO_LOG("Device does not have the required SwapChain support.");
            return false;
        }

        if (!requirements.extensionNames.Empty())
        {
            u32 availableExtensionCount = 0;
            VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, nullptr));

            if (availableExtensionCount > 0)
            {
                DynamicArray<VkExtensionProperties> availableExtensions;
                availableExtensions.Resize(availableExtensionCount);

                VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &availableExtensionCount, availableExtensions.GetData()));

                for (auto& extension : requirements.extensionNames)
                {
                    auto index = std::find_if(
                        availableExtensions.begin(), availableExtensions.end(),
                        [extension](const VkExtensionProperties& props) { return std::strcmp(props.extensionName, extension) == 0; });

                    if (index == availableExtensions.end())
                    {
                        INFO_LOG("Device does not support the: '{}' extension which is required.", extension);
                        return false;
                    }
                }

                // Check if the VK_KHR_portability_subset is available, if it is we must enable it to enable portability
                // to make sure we can run on platforms like Mac OS.
                // See: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_portability_subset.html
                auto index = std::find_if(availableExtensions.begin(), availableExtensions.end(), [](const VkExtensionProperties& props) {
                    return props.extensionName == VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
                });

                if (index != availableExtensions.end())
                {
                    INFO_LOG("VK_KHR_portability_subset extension is present so we must add it as a required extension.");

                    m_requiresPortability = true;
                }
            }
        }

        if (requirements.samplerAnisotropy && !m_features.samplerAnisotropy)
        {
            ERROR_LOG("Device does not support SamplerAnisotropy which is required.");
            return false;
        }

        if (!m_features.fillModeNonSolid)
        {
            ERROR_LOG("Device does not support FillModeNonSolid which is required.");
            return false;
        }

        return true;
    }

    VulkanDeviceSupportResult VulkanDevice::CheckForSupportAndAddExtensionIfNeeded(const char* feature,
                                                                                   VulkanDeviceSupportFlagBits nativeBit,
                                                                                   VulkanDeviceSupportFlagBits extensionBit,
                                                                                   const char* extensionName,
                                                                                   DynamicArray<const char*>& requestedExtensions)
    {
        if (HasSupportFor(nativeBit))
        {
            // We natively support the feature
            INFO_LOG("We have native support for: '{}'.", feature);
            return VulkanDeviceSupportResult::Native;
        }

        if (HasSupportFor(extensionBit))
        {
            // We do not support the feature natively but we do support it by extension

            // Check if this extension is already requested and only add it if it's not
            auto it = std::find_if(requestedExtensions.begin(), requestedExtensions.end(),
                                   [extensionName](const char* ext) { return std::strcmp(ext, extensionName); });
            if (it == requestedExtensions.end())
            {
                requestedExtensions.PushBack(extensionName);
                INFO_LOG("No native support for: '{}' but there is support through the: '{}' extension. We are enabling it!", feature,
                         extensionName);
            }
            else
            {
                INFO_LOG(
                    "No native support for: '{}' but there is support through the: '{}' extension. We already have that extension enabled "
                    "so we are good.",
                    feature, extensionName);
            }

            return VulkanDeviceSupportResult::Extension;
        }

        // We do not support the feature natively or by extension
        WARN_LOG("No support for: '{}'.", feature);
        return VulkanDeviceSupportResult::None;
    }

    void VulkanDevice::QuerySwapChainSupport() const
    {
        auto surface = m_context->surface;

        VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, surface, &m_swapChainSupport.capabilities))

        u32 formatCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, nullptr))

        if (formatCount != 0)
        {
            m_swapChainSupport.formats.Resize(formatCount);
            VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, surface, &formatCount, m_swapChainSupport.formats.GetData()))
        }

        u32 presentModeCount = 0;
        VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &presentModeCount, nullptr))

        if (presentModeCount != 0)
        {
            m_swapChainSupport.presentModes.Resize(presentModeCount);
            VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, surface, &presentModeCount,
                                                               m_swapChainSupport.presentModes.GetData()))
        }

        INFO_LOG("SwapChain support information obtained.");
    }
}  // namespace C3D
