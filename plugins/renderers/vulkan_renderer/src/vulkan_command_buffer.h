
#pragma once
#include <core/defines.h>
#include <vulkan/vulkan.h>

namespace C3D
{
    struct VulkanContext;

    enum class VulkanCommandBufferState : u8
    {
        Ready,
        Recording,
        InRenderPass,
        RecordingEnded,
        Submitted,
        NotAllocated
    };

    class VulkanCommandBuffer
    {
    public:
        VulkanCommandBuffer() = default;

        void Allocate(const VulkanContext* context, VkCommandPool pool, bool isPrimary);

        void Free(const VulkanContext* context, VkCommandPool pool);

        void Begin(bool isSingleUse, bool isRenderPassContinue, bool isSimultaneousUse);

        void End();

        void UpdateSubmitted();

        void Reset();

        void AllocateAndBeginSingleUse(const VulkanContext* context, VkCommandPool pool);

        void EndSingleUse(const VulkanContext* context, VkCommandPool pool, VkQueue queue);

        VkCommandBuffer handle = nullptr;
        VulkanCommandBufferState state;
    };
}  // namespace C3D