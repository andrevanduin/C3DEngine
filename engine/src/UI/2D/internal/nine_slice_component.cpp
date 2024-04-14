
#include "nine_slice_component.h"

#include "core/colors.h"
#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    constexpr const char* INSTANCE_NAME = "UI2D_SYSTEM";

    bool NineSliceComponent::Initialize(Component& self, const char* name, AtlasID _atlasID, const u16vec2& size,
                                        const u16vec2& _cornerSize, const vec4& _color)
    {
        auto& uiSystem       = self.GetSystem<UI2DSystem>();
        auto& geometrySystem = self.GetSystem<GeometrySystem>();
        auto& renderSystem   = self.GetSystem<RenderSystem>();

        atlasID = _atlasID;
        color   = _color;

        auto& descriptions = uiSystem.GetAtlasDescriptions(atlasID);
        cornerSize         = _cornerSize;
        atlasMin           = descriptions.defaultMin;
        atlasMax           = descriptions.defaultMax;

        // Generate a config for the NineSlice
        auto config = GeometryUtils::GenerateUINineSliceConfig(name, size, cornerSize, descriptions.size, descriptions.cornerSize, atlasMin,
                                                               atlasMax);

        // Create Geometry from the config
        geometry = geometrySystem.AcquireFromConfig(config, true);

        // Acquire shader instance resources
        const TextureMap* maps[1] = { &uiSystem.GetAtlas() };
        if (!renderSystem.AcquireShaderInstanceResources(uiSystem.GetShader(), 1, maps, &renderable.instanceId))
        {
            ERROR_LOG("Failed to Acquire Shader Instance resources.");
            return false;
        }

        // Cache our Geometry Render Data
        renderable.renderData = GeometryRenderData(self.GetID(), geometry);
        return true;
    }

    void NineSliceComponent::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        auto& shaderSystem = self.GetSystem<ShaderSystem>();
        auto& uiSystem     = self.GetSystem<UI2DSystem>();
        auto& renderer     = self.GetSystem<RenderSystem>();

        // Apply instance
        bool needsUpdate = renderable.frameNumber != frameData.frameNumber || renderable.drawIndex != frameData.drawIndex;

        shaderSystem.BindInstance(renderable.instanceId);

        shaderSystem.SetUniformByIndex(locations.properties, &color);
        shaderSystem.SetUniformByIndex(locations.diffuseTexture, &uiSystem.GetAtlas());
        shaderSystem.ApplyInstance(needsUpdate);

        // Sync frame number
        renderable.frameNumber = frameData.frameNumber;
        renderable.drawIndex   = frameData.drawIndex;

        // Apply local
        auto model = self.GetWorld();
        shaderSystem.SetUniformByIndex(locations.model, &model);

        renderer.DrawGeometry(renderable.renderData);
    }

    void NineSliceComponent::OnResize(Component& self, const u16vec2& size)
    {
        auto& uiSystem     = self.GetSystem<UI2DSystem>();
        auto& renderSystem = self.GetSystem<RenderSystem>();

        auto& descriptions = uiSystem.GetAtlasDescriptions(atlasID);
        GeometryUtils::RegenerateUINineSliceGeometry(static_cast<Vertex2D*>(geometry->vertices), size, cornerSize, descriptions.size,
                                                     descriptions.cornerSize, atlasMin, atlasMax);

        renderSystem.UpdateGeometryVertices(*geometry, 0, geometry->vertexCount, geometry->vertices);
    }

    void NineSliceComponent::Destroy(Component& self)
    {
        if (geometry)
        {
            auto& geometrySystem = self.GetSystem<GeometrySystem>();
            geometrySystem.Release(geometry);
        }

        if (renderable.instanceId != INVALID_ID)
        {
            auto& renderSystem = self.GetSystem<RenderSystem>();
            auto& uiSystem     = self.GetSystem<UI2DSystem>();

            renderSystem.ReleaseShaderInstanceResources(uiSystem.GetShader(), renderable.instanceId);
        }
    }
}  // namespace C3D::UI_2D