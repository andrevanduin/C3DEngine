
#pragma once
#include <containers/dynamic_array.h>
#include <core/defines.h>
#include <memory/allocators/linear_allocator.h>
#include <renderer/renderer_types.h>
#include <renderer/rendergraph/rendergraph_pass.h>

#include "test_env_types.h"

namespace C3D
{
    class Shader;
    class SystemManager;
    class Camera;
}  // namespace C3D

class EditorGizmo;

class EditorPass : public C3D::RendergraphPass
{
public:
    EditorPass();
    EditorPass(const C3D::SystemManager* pSystemsManager);

    bool Initialize(const C3D::LinearAllocator* frameAllocator) override;
    bool Prepare(C3D::Viewport* viewport, C3D::Camera* camera, EditorGizmo* gizmo);
    bool Execute(const C3D::FrameData& frameData) override;

private:
    C3D::Shader* m_shader = nullptr;

    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_geometries;

    DebugColorShaderLocations m_locations;
};