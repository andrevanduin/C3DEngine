
#include "clipping_component.h"

#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    bool ClippingComponent::Initialize(Component& self, const char* name, const u16vec2& _size)
    {
        static u8 CURRENT_STENCIL_ID = 1;

        // Id used to do stencil testing
        id   = CURRENT_STENCIL_ID++;
        size = _size;

        // Generate a config for the NineSlice
        auto config = GeometryUtils::GenerateUIQuadConfig(name, size, u16vec2(1, 1), u16vec2(0, 0), u16vec2(0, 0));

        // Create Geometry from the config
        geometry = Geometric.AcquireFromConfig(config, true);
        if (!geometry)
        {
            ERROR_LOG("Failed to Acquire geometry.");
            return false;
        }

        // Cache our Geometry Render Data
        renderData = GeometryRenderData(self.GetID(), geometry);
        return true;
    }

    void ClippingComponent::OnPrepareRender(Component& self)
    {
        if (isDirty)
        {
            GeometryUtils::RegenerateUIQuadGeometry(static_cast<Vertex2D*>(geometry->vertices), size, u16vec2(1, 1), u16vec2(0, 0),
                                                    u16vec2(0, 0));

            Renderer.UpdateGeometryVertices(*geometry, 0, geometry->vertexCount, geometry->vertices, false);

            isDirty = false;
        }
    }

    void ClippingComponent::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        // Enable writing, disable test.
        Renderer.SetStencilTestingEnabled(true);
        Renderer.SetDepthTestingEnabled(false);
        Renderer.SetStencilReference(static_cast<u32>(id));
        Renderer.SetStencilWriteMask(0xFF);
        Renderer.SetStencilOperation(StencilOperation::Replace, StencilOperation::Replace, StencilOperation::Replace,
                                     CompareOperation::Always);

        auto model = self.GetWorld();
        model[3][0] += offsetX;
        model[3][1] += offsetY;

        Shaders.SetUniformByIndex(locations.model, &model);

        // Draw the clip mask geometry.
        Renderer.DrawGeometry(renderData);

        // Disable writing, enable test.
        Renderer.SetStencilWriteMask(0x00);
        Renderer.SetStencilTestingEnabled(true);
        Renderer.SetStencilCompareMask(0xFF);
        Renderer.SetStencilOperation(StencilOperation::Keep, StencilOperation::Replace, StencilOperation::Keep, CompareOperation::Equal);
    }

    void ClippingComponent::ResetClipping(Component& self)
    {
        Renderer.SetStencilWriteMask(0x0);
        Renderer.SetStencilTestingEnabled(false);
        Renderer.SetStencilOperation(StencilOperation::Keep, StencilOperation::Keep, StencilOperation::Keep, CompareOperation::Always);
    }

    void ClippingComponent::OnResize(Component& self, const u16vec2& _size)
    {
        size    = _size;
        isDirty = true;
    }

    void ClippingComponent::Destroy(Component& self)
    {
        if (geometry)
        {
            Geometric.Release(geometry);
        }
    }
}  // namespace C3D::UI_2D