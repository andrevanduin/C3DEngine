#pragma once
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#define NVIDIA_VENDOR_ID 0x10DE
#define AMD_VENDOR_ID 0x1002
#define INTEL_VENDOR_ID 0x8086

struct AllocatedBuffer {
    VkBuffer buffer;
    VmaAllocation allocation;
};

struct AllocatedImage
{
    VkImage image;
    VmaAllocation allocation;
};