
#pragma once
#include <containers/string.h>
#include <core/defines.h>
#include <resources/texture.h>
#include <vulkan/vulkan.h>

#include "vulkan_command_buffer.h"

namespace C3D
{
    struct VulkanContext;

    class VulkanImage
    {
    public:
        ~VulkanImage();

        void Create(const VulkanContext* context, const String& name, TextureType type, u32 _width, u32 _height,
                    VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags,
                    bool createView, VkImageAspectFlags viewAspectFlags);

        void CreateView(const VulkanContext* context, TextureType type, VkFormat format,
                        VkImageAspectFlags aspectFlags);

        void TransitionLayout(const VulkanContext* context, const VulkanCommandBuffer* commandBuffer, TextureType type,
                              VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;

        void CopyFromBuffer(TextureType type, VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const;
        void CopyToBuffer(TextureType type, VkBuffer buffer, const VulkanCommandBuffer* commandBuffer) const;
        void CopyPixelToBuffer(TextureType type, VkBuffer buffer, u32 x, u32 y,
                               const VulkanCommandBuffer* commandBuffer) const;

        void Destroy();

        VkImage handle   = nullptr;
        VkImageView view = nullptr;

        u32 width = 0, height = 0;

    private:
        String m_name;
        VkDeviceMemory m_memory = nullptr;
        VkMemoryRequirements m_memoryRequirements;
        VkMemoryPropertyFlags m_memoryFlags = 0;

        const VulkanContext* m_context = nullptr;
    };
}  // namespace C3D
