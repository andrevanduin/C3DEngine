
#pragma once
#include <core/defines.h>
#include <math/math_types.h>
#include <renderer/renderpass.h>

#include "vulkan_command_buffer.h"

namespace C3D
{
    struct VulkanContext;

    enum class VulkanRenderPassState : u8
    {
        Ready,
        Recording,
        InRenderPass,
        RecordingEnded,
        Submitted,
        NotAllocated
    };

    class VulkanRenderPass final : public RenderPass
    {
    public:
        VulkanRenderPass();
        VulkanRenderPass(const SystemManager* pSystemsManager, VulkanContext* context, const RenderPassConfig& config);

        bool Create(const RenderPassConfig& config) override;

        void Destroy() override;

        void Begin(VulkanCommandBuffer* commandBuffer, const RenderTarget* target) const;

        void End(VulkanCommandBuffer* commandBuffer) const;

        VkRenderPass handle = nullptr;
        VulkanRenderPassState state;

    private:
        f32 m_depth   = 0.0f;
        u32 m_stencil = 0;

        VulkanContext* m_context = nullptr;
    };
}  // namespace C3D
