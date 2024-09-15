
#include "vulkan_allocator.h"

#include <metrics/metrics.h>
#include <memory/global_memory_system.h>
#include <platform/platform.h>

namespace C3D
{
#ifdef C3D_VULKAN_USE_CUSTOM_ALLOCATOR
    /*
     * @brief Implementation of PFN_vkAllocationFunction
     * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkAllocationFunction.html
     */
    void* VulkanAllocator::Allocate(void* userData, const size_t size, const size_t alignment,
                                    const VkSystemAllocationScope allocationScope)
    {
        // The spec states that we should return nullptr if size == 0
        if (size == 0) return nullptr;

        void* result = Memory.AllocateBlock(MemoryType::Vulkan, size, static_cast<u16>(alignment));
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
        Logger::Trace("[VULKAN_ALLOCATE] - {} (Size = {}B, Alignment = {}).", result, size, alignment);
#endif

        return result;
    }

    /*
     * @brief Implementation of PFN_vkFreeFunction
     * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkFreeFunction.html
     */
    void VulkanAllocator::Free(void* userData, void* memory)
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
        if (Memory.GetSizeAlignment(memory, &size, &alignment))
        {
            Memory.Free(memory);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_FREE] - Block at: {} was Freed.", memory);
#endif
        }
        else
        {
            Logger::Error("[VULKAN_FREE] - Failed to get alignment lookup for block: {}.", memory);
        }
    }

    /*
     * @brief Implementation of PFN_vkReallocationFunction
     * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkReallocationFunction.html
     */
    void* VulkanAllocator::Reallocate(void* userData, void* original, const size_t size, const size_t alignment,
                                      const VkSystemAllocationScope allocationScope)
    {
        // The spec states that we should do an simple allocation if original is nullptr
        if (!original)
        {
            return Allocate(userData, size, alignment, allocationScope);
        }

        // The spec states that we should return nullptr if size == 0
        if (size == 0)
        {
            Free(userData, original);
            return nullptr;
        }

        u64 allocSize;
        u16 allocAlignment;
        if (!Memory.GetSizeAlignment(original, &allocSize, &allocAlignment))
        {
            Logger::Error("[VULKAN_REALLOCATE] - Tried to do a reallocation of an unaligned block: {}.", original);
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
        Logger::Trace("[VULKAN_REALLOCATE] - Reallocating block: {}", original);
#endif

        void* result = Allocate(userData, size, alignment, allocationScope);
        if (result)
        {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_REALLOCATE] - Successfully reallocated to: {}. Copying data.", result);
#endif
            std::memcpy(result, original, allocSize);
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_REALLOCATE] - Freeing original block: {}.", original);
#endif
            Memory.Free(original);
        }
        else
        {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
            Logger::Trace("[VULKAN_REALLOCATE] - Failed to Reallocate: {}.", original);
#endif
        }

        return result;
    }

    /*
     * @brief Implementation of PFN_vkReallocationFunction
     * @link
     * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
     */
    void VulkanAllocator::InternalAllocation(void* userData, size_t size, VkInternalAllocationType allocationType,
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
    void VulkanAllocator::InternalFree(void* userData, size_t size, VkInternalAllocationType allocationType,
                                       VkSystemAllocationScope allocationScope)
    {
#ifdef C3D_VULKAN_ALLOCATOR_TRACE
        Logger::Trace("[VULKAN_EXTERNAL_FREE] - Free of size {}.", size);
#endif
        Metrics.FreeExternal(size);
    }

    bool VulkanAllocator::Create(VkAllocationCallbacks* callbacks)
    {
        if (callbacks)
        {
            callbacks->pfnAllocation         = Allocate;
            callbacks->pfnReallocation       = Reallocate;
            callbacks->pfnFree               = Free;
            callbacks->pfnInternalAllocation = InternalAllocation;
            callbacks->pfnInternalFree       = InternalFree;
            callbacks->pUserData             = nullptr;
            return true;
        }
        return false;
    }
#endif
}  // namespace C3D
