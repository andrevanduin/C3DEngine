
#include "clipping_component.h"

#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    bool ClippingComponent::Initialize(Component& self, const char* name, const u16vec2& _size)
    {
        static u8 CURRENT_STENCIL_ID = 1;

        auto& geometrySystem = self.GetSystem<GeometrySystem>();

        // Id used to do stencil testing
        id   = CURRENT_STENCIL_ID++;
        size = _size;

        // Generate a config for the NineSlice
        auto config = GeometryUtils::GenerateUIQuadConfig(name, size, u16vec2(1, 1), u16vec2(0, 0), u16vec2(0, 0));

        // Create Geometry from the config
        geometry = geometrySystem.AcquireFromConfig(config, true);
        if (!geometry)
        {
            ERROR_LOG("Failed to Acquire geometry.");
            return false;
        }

        // Cache our Geometry Render Data
        renderData = GeometryRenderData(self.GetID(), geometry);
        return true;
    }

    void ClippingComponent::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& renderSystem = self.GetSystem<RenderSystem>();
        auto& shaderSystem = self.GetSystem<ShaderSystem>();

        // Enable writing, disable test.
        renderSystem.SetStencilTestingEnabled(true);
        renderSystem.SetDepthTestingEnabled(false);
        renderSystem.SetStencilReference(static_cast<u32>(id));
        renderSystem.SetStencilWriteMask(0xFF);
        renderSystem.SetStencilOperation(StencilOperation::Replace, StencilOperation::Replace, StencilOperation::Replace,
                                         CompareOperation::Always);

        auto model = self.GetWorld();
        model[3][0] += offsetX;
        model[3][1] += offsetY;

        shaderSystem.SetUniformByIndex(locations.model, &model);

        // Draw the clip mask geometry.
        renderSystem.DrawGeometry(renderData);

        // Disable writing, enable test.
        renderSystem.SetStencilWriteMask(0x00);
        renderSystem.SetStencilTestingEnabled(true);
        renderSystem.SetStencilCompareMask(0xFF);
        renderSystem.SetStencilOperation(StencilOperation::Keep, StencilOperation::Replace, StencilOperation::Keep,
                                         CompareOperation::Equal);
    }

    void ClippingComponent::ResetClipping(Component& self)
    {
        auto& renderSystem = self.GetSystem<RenderSystem>();

        renderSystem.SetStencilWriteMask(0x0);
        renderSystem.SetStencilTestingEnabled(false);
        renderSystem.SetStencilOperation(StencilOperation::Keep, StencilOperation::Keep, StencilOperation::Keep, CompareOperation::Always);
    }

    void ClippingComponent::OnResize(Component& self, const u16vec2& _size)
    {
        size = _size;

        GeometryUtils::RegenerateUIQuadGeometry(static_cast<Vertex2D*>(geometry->vertices), size, u16vec2(1, 1), u16vec2(0, 0),
                                                u16vec2(0, 0));

        auto& renderSystem = self.GetSystem<RenderSystem>();
        renderSystem.UpdateGeometryVertices(*geometry, 0, geometry->vertexCount, geometry->vertices);
    }

    void ClippingComponent::Destroy(Component& self)
    {
        if (geometry)
        {
            auto& geometrySystem = self.GetSystem<GeometrySystem>();
            geometrySystem.Release(geometry);
        }
    }
}  // namespace C3D::UI_2D