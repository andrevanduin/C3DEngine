
#pragma once
#include "core/defines.h"
#include "core/events/event_context.h"
#include "renderer/render_view.h"
#include "resources/textures/texture.h"

namespace C3D
{
    class Shader;

    struct RenderViewPickShaderInfo
    {
        Shader* shader;
        RenderPass* pass;

        u16 idColorLocation;
        u16 modelLocation;
        u16 projectionLocation;
        u16 viewLocation;

        mat4 projection;
        mat4 view;

        f32 nearClip;
        f32 farClip;
        f32 fov;
    };

    class RenderViewPick final : public RenderView
    {
    public:
        explicit RenderViewPick(const RenderViewConfig& config);

        bool OnCreate() override;
        void OnDestroy() override;

        void OnResize() override;

        bool OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket) override;

        bool OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                      u64 renderTargetIndex) override;

        void GetMatrices(mat4* outView, mat4* outProjection);

        bool RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment) override;

    private:
        bool OnMouseMovedEvent(u16 code, void* sender, const EventContext& context);

        void AcquireShaderInstances();
        void ReleaseShaderInstances();

        RenderViewPickShaderInfo m_uiShaderInfo;
        RenderViewPickShaderInfo m_worldShaderInfo;
        RenderViewPickShaderInfo m_terrainShaderInfo;

        Texture m_colorTargetAttachmentTexture;
        Texture m_depthTargetAttachmentTexture;

        u32 m_instanceCount = 0;
        DynamicArray<bool> m_instanceUpdated;

        RegisteredEventCallback m_onEventCallback;

        i16 m_mouseX = 0, m_mouseY = 0;
    };
}  // namespace C3D