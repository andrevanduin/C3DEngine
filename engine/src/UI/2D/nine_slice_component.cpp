
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

    bool NineSliceComponent::Initialize(Component& self, const char* name, ComponentType type, const u16vec2& _cornerSize)
    {
        auto& uiSystem       = self.GetSystem<UI2DSystem>();
        auto& geometrySystem = self.GetSystem<GeometrySystem>();
        auto& renderSystem   = self.GetSystem<RenderSystem>();

        auto& descriptions = uiSystem.GetAtlasDescriptions(type);
        atlasMin           = descriptions.defaultMin;
        atlasMax           = descriptions.defaultMax;
        cornerSize         = _cornerSize;

        // Generate a config for the NineSlice
        auto config = GeometryUtils::GenerateUINineSliceConfig(name, self.GetSize(), cornerSize, descriptions.size, descriptions.cornerSize,
                                                               atlasMin, atlasMax);

        // Create Geometry from the config
        geometry = geometrySystem.AcquireFromConfig(config, true);

        // Acquire shader instance resources
        TextureMap* maps[1] = { &uiSystem.GetAtlas() };
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

        shaderSystem.SetUniformByIndex(locations.properties, &WHITE);
        shaderSystem.SetUniformByIndex(locations.diffuseTexture, &uiSystem.GetAtlas());
        shaderSystem.ApplyInstance(needsUpdate);

        // Sync frame number
        renderable.frameNumber = frameData.frameNumber;
        renderable.drawIndex   = frameData.drawIndex;

        renderer.SetStencilWriteMask(0x0);
        renderer.SetStencilTestingEnabled(false);

        // Apply local
        auto model = self.GetWorld();
        shaderSystem.SetUniformByIndex(locations.model, &model);

        renderer.DrawGeometry(renderable.renderData);
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