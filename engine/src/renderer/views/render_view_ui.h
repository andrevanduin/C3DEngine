
#pragma once
#include "renderer/render_view.h"
#include "resources/shader.h"

namespace C3D
{
    class RenderViewUi final : public RenderView
    {
    public:
        explicit RenderViewUi(const RenderViewConfig& config);

        bool OnCreate() override;

        void OnResize() override;

        bool OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket) override;

        bool OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                      u64 renderTargetIndex) override;

    private:
        f32 m_nearClip;
        f32 m_farClip;

        mat4 m_projectionMatrix;
        mat4 m_viewMatrix;

        Shader* m_shader;
        u16 m_diffuseMapLocation;
        u16 m_diffuseColorLocation;
        u16 m_modelLocation;
    };
}  // namespace C3D