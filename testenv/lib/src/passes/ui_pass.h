
#pragma once
#include <containers/dynamic_array.h>
#include <core/defines.h>
#include <renderer/render_view_types.h>
#include <renderer/rendergraph/rendergraph_pass.h>

namespace C3D
{
    class Shader;
    class UIMesh;
}  // namespace C3D

struct ShaderLocations
{
    u16 diffuseMap = INVALID_ID_U16;
    u16 properties = INVALID_ID_U16;
    u16 model      = INVALID_ID_U16;
};

class UIPass : public C3D::RendergraphPass
{
public:
    UIPass();
    UIPass(const C3D::SystemManager* pSystemsManager);

    bool Initialize(const C3D::LinearAllocator* frameAllocator) override;
    bool Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::UIMesh* meshes,
                 const C3D::DynamicArray<C3D::UIText*, C3D::LinearAllocator>& texts);
    bool Execute(const C3D::FrameData& frameData) override;

private:
    C3D::Shader* m_shader = nullptr;

    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> m_geometries;
    C3D::DynamicArray<C3D::UIText*, C3D::LinearAllocator> m_texts;

    ShaderLocations m_locations;
};