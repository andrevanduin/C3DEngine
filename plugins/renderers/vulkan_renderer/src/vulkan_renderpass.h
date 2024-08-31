
#pragma once
#include <defines.h>
#include <frame_data.h>
#include <math/math_types.h>
#include <renderer/rendergraph/rendergraph_types.h>

#include "vulkan_command_buffer.h"

namespace C3D
{
    struct VulkanContext;
    class Viewport;

    enum class VulkanRenderpassState : u8
    {
        Ready,
        Recording,
        InRenderPass,
        RecordingEnded,
        Submitted,
        NotAllocated
    };

    class VulkanRenderpass
    {
    public:
        bool Create(const RenderpassConfig& config, const VulkanContext* context);
        void Destroy();

        void Begin(VulkanCommandBuffer* commandBuffer, const Viewport* viewport, const RenderTarget& target) const;
        void End(VulkanCommandBuffer* commandBuffer) const;

        const String& GetName() const { return m_name; }

        VkRenderPass handle = nullptr;

    private:
        String m_name;
        VulkanRenderpassState m_state;

        f32 m_depth       = 0.f;
        u32 m_stencil     = 0;
        u8 m_clearFlags   = 0;
        vec4 m_clearColor = vec4(1.0);

        const VulkanContext* m_context = nullptr;
    };
}  // namespace C3D
