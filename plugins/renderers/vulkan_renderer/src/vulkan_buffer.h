
#pragma once
#include <core/defines.h>
#include <renderer/render_buffer.h>
#include <vulkan/vulkan.h>

namespace C3D
{
    struct VulkanContext;

    class VulkanBuffer final : public RenderBuffer
    {
    public:
        explicit VulkanBuffer(const VulkanContext* context, const String& name);

        bool Create(RenderBufferType bufferType, u64 size, bool useFreelist) override;
        void Destroy() override;

        bool Bind(u64 offset) override;

        void* MapMemory(u64 offset, u64 size) override;
        void UnMapMemory(u64 offset, u64 size) override;

        bool Flush(u64 offset, u64 size) override;
        bool Resize(u64 newSize) override;

        bool Read(u64 offset, u64 size, void** outMemory) override;
        bool LoadRange(u64 offset, u64 size, const void* data) override;
        bool CopyRange(u64 srcOffset, RenderBuffer* dest, u64 dstOffset, u64 size) override;

        bool Draw(u64 offset, u32 elementCount, bool bindOnly) override;

        [[nodiscard]] bool IsDeviceLocal() const;
        [[nodiscard]] bool IsHostVisible() const;
        [[nodiscard]] bool IsHostCoherent() const;

        VkBuffer handle;

    protected:
        bool CopyRangeInternal(u64 srcOffset, VkBuffer dst, u64 dstOffset, u64 size) const;

        VkBufferUsageFlagBits m_usage;

        VkDeviceMemory m_memory;
        VkMemoryRequirements m_memoryRequirements;

        i32 m_memoryIndex         = 0;
        u32 m_memoryPropertyFlags = 0;

        bool m_isLocked = true;

        const VulkanContext* m_context = nullptr;
    };
}  // namespace C3D
