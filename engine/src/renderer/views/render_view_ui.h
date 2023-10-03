
#pragma once
#include "renderer/render_view.h"

namespace C3D
{
    class Shader;

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
        f32 m_nearClip = -100.0f;
        f32 m_farClip  = 100.0f;

        mat4 m_projectionMatrix = mat4(1);
        mat4 m_viewMatrix       = mat4(1);

        Shader* m_shader         = nullptr;
        u16 m_diffuseMapLocation = INVALID_ID_U16;
        u16 m_propertiesLocation = INVALID_ID_U16;
        u16 m_modelLocation      = INVALID_ID_U16;
    };
}  // namespace C3D