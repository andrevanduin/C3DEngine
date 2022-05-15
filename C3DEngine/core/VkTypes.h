#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#define NVIDIA_VENDOR_ID 0x10DE
#define AMD_VENDOR_ID 0x1002
#define INTEL_VENDOR_ID 0x8086

struct AllocatedBufferUntyped {
    VkBuffer buffer{};
    VmaAllocation allocation{};
    VkDeviceSize size{ 0 };

    [[nodiscard]] VkDescriptorBufferInfo GetInfo(const VkDeviceSize offset = 0) const
    {
        const VkDescriptorBufferInfo info { buffer, offset, size };
        return info;
    }
};

template<typename T>
struct AllocatedBuffer : AllocatedBufferUntyped
{
	AllocatedBuffer& operator=(const AllocatedBufferUntyped& other)
	{
        buffer = other.buffer;
        allocation = other.allocation;
        size = other.size;
        return this;
	}

	explicit AllocatedBuffer(const AllocatedBufferUntyped& other) {
        buffer = other.buffer;
        allocation = other.allocation;
        size = other.size;
    }

    AllocatedBuffer() = default;
};

struct AllocatedImage
{
    VkImage image;
    VmaAllocation allocation;
    VkImageView defaultView;
    uint32_t mipLevels;
};

enum class MeshPassType : uint8_t
{
	None = 0, Forward, Transparency, DirectionalShadow
};