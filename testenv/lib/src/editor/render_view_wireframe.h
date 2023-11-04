
#pragma once
#include <containers/dynamic_array.h>
#include <core/events/event_context.h>
#include <renderer/render_view.h>

#include "test_env_types.h"

namespace C3D
{
    class Camera;
    class Shader;
}  // namespace C3D

struct RenderViewWireframeData
{
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> worldGeometries;
    C3D::DynamicArray<C3D::GeometryRenderData, C3D::LinearAllocator> terrainGeometries;
    C3D::DynamicArray<C3D::GeometryRenderData> debugGeometries;

    /** @brief The id of the object that we currently have selected. */
    u32 selectedId = INVALID_ID;
};

struct WireframeShaderLocations
{
    u16 projection = INVALID_ID_U16;
    u16 view       = INVALID_ID_U16;
    u16 model      = INVALID_ID_U16;
    u16 color      = INVALID_ID_U16;
};

/** @brief We will have one instance per color that we draw. */
struct WireframeColorInstance
{
    /** @brief The instance id. */
    u32 id          = INVALID_ID;
    u64 frameNumber = INVALID_ID_U64;
    u8 drawIndex    = INVALID_ID_U8;
    vec4 color;
};

struct WireframeShaderInfo
{
    C3D::Shader* shader;
    WireframeShaderLocations locations;

    WireframeColorInstance normalInstance;
    WireframeColorInstance selectedInstance;
};

class RenderViewWireframe final : public C3D::RenderView
{
public:
    RenderViewWireframe();

    bool OnCreate() override;

    void OnDestroy() override;

    bool OnBuildPacket(const C3D::FrameData& frameData, const C3D::Viewport& viewport, C3D::Camera* camera, void* data,
                       C3D::RenderViewPacket* outPacket) override;

    bool OnRender(const C3D::FrameData& frameData, const C3D::RenderViewPacket* packet) override;

private:
    void OnSetupPasses() override;

    u32 m_selectedId = INVALID_ID;

    WireframeShaderInfo m_meshShader;
    WireframeShaderInfo m_terrainShader;
};
