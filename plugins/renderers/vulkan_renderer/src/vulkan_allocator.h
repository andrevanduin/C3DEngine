
#pragma once
#include <vulkan/vulkan.h>

namespace C3D
{
#ifdef C3D_VULKAN_USE_CUSTOM_ALLOCATOR
	/*
	 * @brief Implementation of PFN_vkAllocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkAllocationFunction.html
	 */
	void* VulkanAllocatorAllocate(void* userData, const size_t size, const size_t alignment, const VkSystemAllocationScope allocationScope);

	/*
	 * @brief Implementation of PFN_vkFreeFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkFreeFunction.html
	 */
	void VulkanAllocatorFree(void* userData, void* memory);

	/*
	 * @brief Implementation of PFN_vkReallocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkReallocationFunction.html
	 */
	void* VulkanAllocatorReallocate(void* userData, void* original, const size_t size, const size_t alignment, const VkSystemAllocationScope allocationScope);

	/*
	 * @brief Implementation of PFN_vkReallocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
	 */
	void VulkanAllocatorInternalAllocation(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);

	/*
	 * @brief Implementation of PFN_vkReallocationFunction
	 * @link https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/PFN_vkInternalAllocationNotification.html
	 */
	void VulkanAllocatorInternalFree(void* userData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);

	bool CreateVulkanAllocator(VkAllocationCallbacks* callbacks);
#endif
}