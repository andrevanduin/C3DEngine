
#pragma once
#include <renderer/render_view.h>

namespace C3D
{
    class Shader;
    class UIMesh;
    class UIText;
}  // namespace C3D

struct UIMeshPacketData
{
    C3D::DynamicArray<C3D::UIMesh*, C3D::LinearAllocator> meshes;
};

struct UIPacketData
{
    UIMeshPacketData meshData;
    // TEMP
    C3D::DynamicArray<C3D::UIText*, C3D::LinearAllocator> texts;
    // TEMP END
};

class RenderViewUi final : public C3D::RenderView
{
public:
    RenderViewUi();

    bool OnCreate() override;

    bool OnBuildPacket(const C3D::FrameData& frameData, const C3D::Viewport& viewport, C3D::Camera* camera, void* data,
                       C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet) override;

private:
    void OnSetupPasses() override;

    f32 m_nearClip = -100.0f;
    f32 m_farClip  = 100.0f;

    mat4 m_viewMatrix = mat4(1);

    C3D::Shader* m_shader    = nullptr;
    u16 m_diffuseMapLocation = INVALID_ID_U16;
    u16 m_propertiesLocation = INVALID_ID_U16;
    u16 m_modelLocation      = INVALID_ID_U16;
};