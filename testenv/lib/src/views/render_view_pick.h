
#pragma once
#include <core/defines.h>
#include <core/events/event_context.h>
#include <renderer/render_view.h>
#include <resources/textures/texture.h>

namespace C3D
{
    class Shader;
    class UIText;
}  // namespace C3D

struct RenderViewPickShaderInfo
{
    C3D::Shader* shader;
    C3D::RenderPass* pass;

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

struct PickPacketData
{
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator>* worldMeshData;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator>* terrainData;

    C3D::UIMeshPacketData uiMeshData;
    u32 worldGeometryCount   = 0;
    u32 terrainGeometryCount = 0;
    u32 uiGeometryCount      = 0;

    // TEMP:
    C3D::DynamicArray<C3D::UIText*, C3D::LinearAllocator> texts;
    // TEMP END
};

class RenderViewPick final : public C3D::RenderView
{
public:
    explicit RenderViewPick();

    bool OnCreate() override;
    void OnDestroy() override;

    void OnResize() override;

    bool OnBuildPacket(C3D::LinearAllocator* frameAllocator, void* data, C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet, u64 frameNumber,
                  u64 renderTargetIndex) override;

    void GetMatrices(mat4* outView, mat4* outProjection);

    bool RegenerateAttachmentTarget(u32 passIndex, C3D::RenderTargetAttachment* attachment) override;

private:
    bool OnMouseMovedEvent(u16 code, void* sender, const C3D::EventContext& context);

    void AcquireShaderInstances();
    void ReleaseShaderInstances();

    void OnSetupPasses() override;

    RenderViewPickShaderInfo m_uiShaderInfo;
    RenderViewPickShaderInfo m_worldShaderInfo;
    RenderViewPickShaderInfo m_terrainShaderInfo;

    C3D::Texture m_colorTargetAttachmentTexture;
    C3D::Texture m_depthTargetAttachmentTexture;

    u32 m_instanceCount = 0;
    C3D::DynamicArray<bool> m_instanceUpdated;

    C3D::RegisteredEventCallback m_onEventCallback;

    i16 m_mouseX = 0, m_mouseY = 0;
};