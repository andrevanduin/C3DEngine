
#include "forward_rendergraph.h"

#include "memory/allocators/linear_allocator.h"
#include "resources/scenes/simple_scene.h"

namespace C3D
{
    bool ForwardRendergraph::Create(const String& name, const ForwardRendergraphConfig& config)
    {
        m_name   = name;
        m_config = config;

        // Add our global sources
        if (!AddGlobalSource("COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor, C3D::RendergraphSourceOrigin::Global))
        {
            ERROR_LOG("Failed to add global color buffer source to Rendergraph.");
            return false;
        }
        if (!AddGlobalSource("DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil, C3D::RendergraphSourceOrigin::Global))
        {
            ERROR_LOG("Failed to add global depth buffer source to Rendergraph.");
            return false;
        }

        // Skybox pass
        if (!AddPass("SKYBOX", &m_skyboxPass))
        {
            ERROR_LOG("Failed to add SKYBOX pass.");
            return false;
        }
        if (!AddSink("SKYBOX", "COLOR_BUFFER"))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER sink to Skybox pass.");
            return false;
        }
        if (!AddSource("SKYBOX", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor, C3D::RendergraphSourceOrigin::Other))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER source to Skybox pass.");
            return false;
        }
        if (!Link("COLOR_BUFFER", "SKYBOX", "COLOR_BUFFER"))
        {
            ERROR_LOG("Failed to link Global COLOR_BUFFER source to SKYBOX COLOR_BUFFER sink.");
            return false;
        }

        // ShadowMap pass
        ShadowMapPassConfig shadowMapPassConfig;
        shadowMapPassConfig.resolution = m_config.shadowMapResolution;

        m_shadowMapPass = ShadowMapPass("SHADOW", shadowMapPassConfig);
        if (!AddPass("SHADOW", &m_shadowMapPass))
        {
            ERROR_LOG("Failed to add: SHADOW pass.");
            return false;
        }
        if (!AddSource("SHADOW", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil, C3D::RendergraphSourceOrigin::Self))
        {
            ERROR_LOG("Failed to add DEPTH_BUFFER to Shadow pass.");
            return false;
        }

        // Scene pass
        if (!AddPass("SCENE", &m_scenePass))
        {
            ERROR_LOG("Failed to add SCENE pass.");
            return false;
        }
        if (!AddSink("SCENE", "COLOR_BUFFER"))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER sink to Scene pass.");
            return false;
        }
        if (!AddSink("SCENE", "DEPTH_BUFFER"))
        {
            ERROR_LOG("Failed to add DEPTH_BUFFER sink to Scene pass.");
            return false;
        }
        if (!AddSink("SCENE", "SHADOW_MAP"))
        {
            ERROR_LOG("Failed to add SHADOW_MAP_0 sink to Scene pass.");
            return false;
        }
        if (!AddSource("SCENE", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor, C3D::RendergraphSourceOrigin::Other))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER source to Scene pass.");
            return false;
        }
        if (!AddSource("SCENE", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil, C3D::RendergraphSourceOrigin::Global))
        {
            ERROR_LOG("Failed to add DEPTH_BUFFER source to Scene pass.");
            return false;
        }
        if (!Link("SKYBOX", "COLOR_BUFFER", "SCENE", "COLOR_BUFFER"))
        {
            ERROR_LOG("Failed to link SKYBOX COLOR_BUFFER source to SCENE COLOR_BUFFER sink.");
            return false;
        }
        if (!Link("DEPTH_BUFFER", "SCENE", "DEPTH_BUFFER"))
        {
            ERROR_LOG("Failed to link Global DEPTH_BUFFER source to SCENE DEPTH_BUFFER sink.");
            return false;
        }

        if (!Link("SHADOW", "DEPTH_BUFFER", "SCENE", "SHADOW_MAP"))
        {
            ERROR_LOG("Failed to link SHADOW  DEPTH_BUFFER source to SCENE SHADOW_MAP sink.");
            return false;
        }

        if (!Finalize(config.pFrameAllocator))
        {
            ERROR_LOG("Failed to finalize main rendergraph.");
            return false;
        }

        return true;
    }

    bool ForwardRendergraph::OnPrepareRender(FrameData& frameData, const Viewport& currentViewport, Camera* currentCamera,
                                             const SimpleScene& scene, u32 renderMode, const DynamicArray<DebugLine3D>& debugLines,
                                             const DynamicArray<DebugBox3D>& debugBoxes)
    {
        m_skyboxPass.Prepare(currentViewport, currentCamera, scene.GetSkybox());

        // Only when the scene is loaded we prepare the shadow, scene and editor pass
        if (scene.GetState() == SceneState::Loaded)
        {
            // Prepare our scene for rendering
            scene.OnPrepareRender(frameData);

            // Prepare the shadow pass
            m_shadowMapPass.Prepare(frameData, currentViewport, currentCamera);

            // Query meshes and terrains seen by the furthest out cascade since all passes will "see" the same
            // Get all the relevant meshes from the scene
            auto& cullingData = m_shadowMapPass.GetCullingData();

            scene.QueryMeshes(frameData, cullingData.lightDirection, cullingData.center, cullingData.radius, cullingData.geometries);
            // Keep track of how many meshes are being used in our shadow pass
            frameData.drawnShadowMeshCount = cullingData.geometries.Size();

            // Get all the relevant terrains from the scene
            scene.QueryTerrains(frameData, cullingData.lightDirection, cullingData.center, cullingData.radius, cullingData.terrains);

            // Also keep track of how many terrains are being used in our shadow pass
            frameData.drawnShadowMeshCount += cullingData.terrains.Size();

            // Prepare the scene pass
            m_scenePass.Prepare(currentViewport, currentCamera, frameData, scene, renderMode, debugLines, debugBoxes,
                                m_shadowMapPass.GetCascadeData());
        }

        return true;
    }
}  // namespace C3D