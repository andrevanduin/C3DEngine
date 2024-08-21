
#include "quad_component.h"

#include "core/colors.h"
#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "systems/UI/2D/ui2d_system.h"
#include "systems/geometry/geometry_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D::UI_2D
{
    bool QuadComponent::Initialize(Component& self, const char* name, AtlasID _atlasID, const u16vec2& _size, const vec4& _color)
    {
        atlasID = _atlasID;
        color   = _color;
        size    = _size;

        auto& descriptions = UI2D.GetAtlasDescriptions(atlasID);
        atlasMin           = descriptions.defaultMin;
        atlasMax           = descriptions.defaultMax;

        // Generate a config for the NineSlice
        auto config = GeometryUtils::GenerateUIQuadConfig(name, size, descriptions.size, atlasMin, atlasMax);

        // Create Geometry from the config
        geometry = Geometric.AcquireFromConfig(config, true);

        // Acquire shader instance resources
        auto& shader = UI2D.GetShader();

        TextureMap* maps[1] = { &UI2D.GetAtlas() };

        ShaderInstanceUniformTextureConfig textureConfig;
        textureConfig.uniformLocation = shader.GetUniformIndex("diffuseTexture");
        textureConfig.textureMaps     = maps;

        ShaderInstanceResourceConfig instanceConfig;
        instanceConfig.uniformConfigs     = &textureConfig;
        instanceConfig.uniformConfigCount = 1;

        if (!Renderer.AcquireShaderInstanceResources(shader, instanceConfig, renderable.instanceId))
        {
            ERROR_LOG("Failed to Acquire Shader Instance resources.");
            return false;
        }

        // Cache our Geometry Render Data
        renderable.renderData = GeometryRenderData(self.GetID(), geometry);
        return true;
    }

    void QuadComponent::OnPrepareRender(Component& self)
    {
        if (isDirty)
        {
            auto& descriptions = UI2D.GetAtlasDescriptions(atlasID);

            GeometryUtils::RegenerateUIQuadGeometry(static_cast<Vertex2D*>(geometry->vertices), size, descriptions.size, atlasMin,
                                                    atlasMax);

            Renderer.UpdateGeometryVertices(*geometry, 0, geometry->vertexCount, geometry->vertices, false);
            isDirty = false;
        }
    }

    void QuadComponent::OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations)
    {
        // Apply instance
        bool needsUpdate = renderable.frameNumber != frameData.frameNumber || renderable.drawIndex != frameData.drawIndex;

        Shaders.BindInstance(renderable.instanceId);

        Shaders.SetUniformByIndex(locations.properties, &color);
        Shaders.SetUniformByIndex(locations.diffuseTexture, &UI2D.GetAtlas());
        Shaders.ApplyInstance(frameData, needsUpdate);

        // Sync frame number
        renderable.frameNumber = frameData.frameNumber;
        renderable.drawIndex   = frameData.drawIndex;

        // Apply local
        auto model = self.GetWorld();
        model[3][0] += offsetX;
        model[3][1] += offsetY;

        Shaders.BindLocal();
        Shaders.SetUniformByIndex(locations.model, &model);
        Shaders.ApplyLocal(frameData);

        Renderer.DrawGeometry(renderable.renderData);
    }

    void QuadComponent::OnResize(Component& self, const u16vec2& _size)
    {
        size    = _size;
        isDirty = true;
    }

    void QuadComponent::Destroy(Component& self)
    {
        if (geometry)
        {
            Geometric.Release(geometry);
        }

        if (renderable.instanceId != INVALID_ID)
        {
            Renderer.ReleaseShaderInstanceResources(UI2D.GetShader(), renderable.instanceId);
        }
    }
}  // namespace C3D::UI_2D