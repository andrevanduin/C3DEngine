
#pragma once
#include <containers/string.h>
#include <core/defines.h>
#include <resources/textures/texture_types.h>
#include <vulkan/vulkan.h>

#include "vulkan_command_buffer.h"

namespace C3D
{
    struct VulkanContext;

    class VulkanImage
    {
    public:
        ~VulkanImage();

        bool Create(const VulkanContext* context, const String& name, TextureType type, u32 _width, u32 _height, VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView, u8 mipLevels,
                    VkImageAspectFlags viewAspectFlags);

        bool CreateMipMaps(const VulkanCommandBuffer* commandBuffer);

        void CreateView(TextureType type, VkImageAspectFlags aspectFlags);

        void TransitionLayout(const VulkanCommandBuffer* commandBuffer, TextureType type, VkFormat format, VkImageLayout oldLayout,
                              VkImageLayout newLayout) const;

        void CopyFromBuffer(TextureType type, VkBuffer buffer, u64 offset, const VulkanCommandBuffer* commandBuffer) const;
        void CopyToBuffer(TextureType type, VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const;
        void CopyPixelToBuffer(TextureType type, VkBuffer buffer, u32 x, u32 y, const VulkanCommandBuffer* commandBuffer) const;

        void Destroy();

        VkImage handle   = nullptr;
        VkImageView view = nullptr;

        u32 width = 0, height = 0;

    private:
        String m_name;
        VkDeviceMemory m_memory = nullptr;
        VkMemoryRequirements m_memoryRequirements;
        VkMemoryPropertyFlags m_memoryFlags = 0;
        VkFormat m_format;

        u8 m_mipLevels;

        const VulkanContext* m_context = nullptr;
    };
}  // namespace C3D
