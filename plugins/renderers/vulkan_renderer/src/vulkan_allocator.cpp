
#include "vulkan_allocator.h"

#include <core/metrics/metrics.h>
#include <memory/global_memory_system.h>
#include <platform/platform.h>

namespace C3D
{
#ifdef C3D_VULKAN_USE_CUSTOM_ALLOCATOR
    /*
     * @brief Implementation of PFN_vkAllocationFunction
     * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkAllocationFunction.html
     */
    void* VulkanAllocatorAllocate(void* userData, const size_t size, const size_t alignment,
                                  const VkSystemAllocationScope allocationScope)
    {
        // The spec states that we should return nullptr if size == 0
        if (size == 0) return nullptr;

        void* result = Memory.AllocateBlock(MemoryType::Vulkan, size, static_cast<u16>(alignment));
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
        Logger::Trace("[VULKAN_ALLOCATE] - {} (Size = {}B, Alignment = {}).", fmt::ptr(result), size, alignment);
#endif

        return result;
    }

    /*
     * @brief Implementation of PFN_vkFreeFunction
     * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkFreeFunction.html
     */
    void VulkanAllocatorFree(void* userData, void* memory)
    {
        if (!memory)
        {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_FREE] - Block was null. Nothing to free.");
#endif
            return;
        }

        u64 size;
        u16 alignment;
        if (DynamicAllocator::GetSizeAlignment(memory, &size, &alignment))
        {
            Memory.Free(MemoryType::Vulkan, memory);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_FREE] - Block at: {} was Freed.", fmt::ptr(memory));
#endif
        }
        else
        {
            Logger::Error("[VULKAN_FREE] - Failed to get alignment lookup for block: {}.", fmt::ptr(memory));
        }
    }

    /*
     * @brief Implementation of PFN_vkReallocationFunction
     * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkReallocationFunction.html
     */
    void* VulkanAllocatorReallocate(void* userData, void* original, const size_t size, const size_t alignment,
                                    const VkSystemAllocationScope allocationScope)
    {
        // The spec states that we should do an simple allocation if original is nullptr
        if (!original)
        {
            return VulkanAllocatorAllocate(userData, size, alignment, allocationScope);
        }

        // The spec states that we should return nullptr if size == 0
        if (size == 0) return nullptr;

        u64 allocSize;
        u16 allocAlignment;
        if (!DynamicAllocator::GetSizeAlignment(original, &allocSize, &allocAlignment))
        {
            Logger::Error("[VULKAN_REALLOCATE] - Tried to do a reallocation of an unaligned block: {}.",
                          fmt::ptr(original));
            return nullptr;
        }

        // The spec states that the alignment provided should not differ from the original memory's alignment.
        if (alignment != allocAlignment)
        {
            Logger::Error(
                "[VULKAN_REALLOCATE] - Attempted to do a reallocation with a different alignment of: {}. Original "
                "alignment was: {}.",
                alignment, allocAlignment);
            return nullptr;
        }

#ifdef C3D_VULKAN_ALLOCATOR_TRACE
        Logger::Trace("[VULKAN_REALLOCATE] - Reallocating block: {}", fmt::ptr(original));
#endif

        void* result = VulkanAllocatorAllocate(userData, size, alignment, allocationScope);
        if (result)
        {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_REALLOCATE] - Successfully reallocated to: {}. Copying data.", fmt::ptr(result));
#endif
            std::memcpy(result, original, allocSize);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_REALLOCATE] - Freeing original block: {}.", fmt::ptr(original));
#endif
            Memory.Free(MemoryType::Vulkan, original);
        }
        else
        {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_REALLOCATE] - Failed to Reallocate: {}.", fmt::ptr(original));
#endif
        }

        return result;
    }

    /*
     * @brief Implementation of PFN_vkReallocationFunction
     * @link
     * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
     */
    void VulkanAllocatorInternalAllocation(void* userData, size_t size, VkInternalAllocationType allocationType,
                                           VkSystemAllocationScope allocationScope)
    {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
        Logger::Trace("[VULKAN_EXTERNAL_ALLOCATE] - Allocation of size {}.", size);
#endif
        Metrics.AllocateExternal(size);
    }

    /*
     * @brief Implementation of PFN_vkReallocationFunction
     * @link
     * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
     */
    void VulkanAllocatorInternalFree(void* userData, size_t size, VkInternalAllocationType allocationType,
                                     VkSystemAllocationScope allocationScope)
    {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
        Logger::Trace("[VULKAN_EXTERNAL_FREE] - Free of size {}.", size);
#endif
        Metrics.FreeExternal(size);
    }

    bool CreateVulkanAllocator(VkAllocationCallbacks* callbacks)
    {
        if (callbacks)
        {
            callbacks->pfnAllocation = VulkanAllocatorAllocate;
            callbacks->pfnReallocation = VulkanAllocatorReallocate;
            callbacks->pfnFree = VulkanAllocatorFree;
            callbacks->pfnInternalAllocation = VulkanAllocatorInternalAllocation;
            callbacks->pfnInternalFree = VulkanAllocatorInternalFree;
            callbacks->pUserData = nullptr;
            return true;
        }
        return false;
    }
#endif
}  // namespace C3D
