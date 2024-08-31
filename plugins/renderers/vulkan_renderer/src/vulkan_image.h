
#pragma once
#include <defines.h>
#include <resources/textures/texture_types.h>
#include <string/string.h>
#include <vulkan/vulkan.h>

#include "vulkan_command_buffer.h"

namespace C3D
{
    struct VulkanContext;

    class VulkanImage
    {
    public:
        ~VulkanImage();

        bool Create(const VulkanContext* context, const String& name, TextureType type, u32 w, u32 h, u16 layerCount, VkFormat format,
                    VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags, bool createView, u8 mipLevels,
                    VkImageAspectFlags viewAspectFlags);

        bool CreateMipMaps(const VulkanCommandBuffer* commandBuffer);

        VkImageView CreateView(TextureType type, u16 layerCount, i32 layerIndex, VkImageAspectFlags aspectFlags);

        void TransitionLayout(const VulkanCommandBuffer* commandBuffer, VkFormat format, VkImageLayout oldLayout,
                              VkImageLayout newLayout) const;

        void CopyFromBuffer(VkBuffer buffer, u64 offset, const VulkanCommandBuffer* commandBuffer) const;
        void CopyToBuffer(VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const;
        void CopyPixelToBuffer(VkBuffer buffer, u32 x, u32 y, const VulkanCommandBuffer* commandBuffer) const;

        void Destroy();

        VkImage handle   = nullptr;
        VkImageView view = nullptr;
        DynamicArray<VkImageView> layerViews;

        u32 width = 0, height = 0;

    private:
        String m_name;
        VkDeviceMemory m_memory = nullptr;
        VkMemoryRequirements m_memoryRequirements;
        VkMemoryPropertyFlags m_memoryFlags = 0;
        VkFormat m_format;

        u16 m_layerCount = 0;
        u8 m_mipLevels   = 0;

        const VulkanContext* m_context = nullptr;
    };
}  // namespace C3D
