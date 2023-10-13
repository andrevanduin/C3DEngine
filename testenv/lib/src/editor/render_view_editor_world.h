
#pragma once
#include <core/defines.h>
#include <renderer/render_view.h>

#include "test_env_types.h"

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

    void OnResize() override;

    bool OnBuildPacket(C3D::LinearAllocator* frameAllocator, void* data, C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet, u64 frameNumber,
                  u64 renderTargetIndex) override;

private:
    void OnSetupPasses() override;

    C3D::Shader* m_shader = nullptr;

    f32 m_fov      = C3D::DegToRad(45.0f);
    f32 m_nearClip = 0.1f;
    f32 m_farClip  = 4000.0f;

    DebugColorShaderLocations m_debugShaderLocations;

    mat4 m_projectionMatrix;
    C3D::Camera* m_camera = nullptr;
};
