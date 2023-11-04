
#pragma once
#include <core/defines.h>
#include <renderer/render_view.h>

#include "editor_types.h"

namespace C3D
{
    class Camera;
    class Shader;
}  // namespace C3D

class EditorGizmo;

struct EditorWorldPacketData
{
    EditorGizmo* gizmo;
};

class RenderViewEditorWorld final : public C3D::RenderView
{
public:
    RenderViewEditorWorld();

    bool OnCreate() override;

    bool OnBuildPacket(const C3D::FrameData& frameData, const C3D::Viewport& viewport, C3D::Camera* camera, void* data,
                       C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet) override;

private:
    void OnSetupPasses() override;

    C3D::Shader* m_shader = nullptr;

    DebugColorShaderLocations m_debugShaderLocations;
};
