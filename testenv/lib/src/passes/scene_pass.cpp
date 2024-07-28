
#include "scene_pass.h"

#include <math/frustum.h>
#include <renderer/camera.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <resources/debug/debug_box_3d.h>
#include <resources/debug/debug_line_3d.h>
#include <resources/loaders/shader_loader.h>
#include <resources/mesh.h>
#include <resources/shaders/shader_types.h>
#include <resources/terrain/terrain.h>
#include <resources/textures/texture.h>
#include <systems/lights/light_system.h>
#include <systems/materials/material_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>

#include "resources/scenes/simple_scene.h"
#include "resources/skybox.h"

constexpr const char* INSTANCE_NAME        = "SCENE_PASS";
constexpr const char* PBR_SHADER_NAME      = "Shader.PBR";
constexpr const char* TERRAIN_SHADER_NAME  = "Shader.Builtin.Terrain";
constexpr const char* COLOR_3D_SHADER_NAME = "Shader.Builtin.Color3D";

constexpr const char* SHADER_NAMES[3] = {
    PBR_SHADER_NAME,
    TERRAIN_SHADER_NAME,
    COLOR_3D_SHADER_NAME,
};

ScenePass::ScenePass() : Renderpass("SCENE") {}

bool ScenePass::Initialize(const C3D::LinearAllocator* frameAllocator)
{
    C3D::RenderpassConfig pass;
    pass.name       = "Renderpass.Scene";
    pass.clearColor = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags = C3D::ClearDepthBuffer | C3D::ClearStencilBuffer;
    pass.depth      = 1.0f;
    pass.stencil    = 0;

    C3D::RenderTargetAttachmentConfig targetAttachments[2]{};
    targetAttachments[0].type           = C3D::RenderTargetAttachmentTypeColor;
    targetAttachments[0].source         = C3D::RenderTargetAttachmentSource::Default;
    targetAttachments[0].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::Load;
    targetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[0].presentAfter   = false;

    targetAttachments[1].type           = C3D::RenderTargetAttachmentTypeDepth;
    targetAttachments[1].source         = C3D::RenderTargetAttachmentSource::Default;
    targetAttachments[1].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    targetAttachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[1].presentAfter   = false;

    pass.target.attachments.PushBack(targetAttachments[0]);
    pass.target.attachments.PushBack(targetAttachments[1]);
    pass.renderTargetCount = Renderer.GetWindowAttachmentCount();

    if (!CreateInternals(pass))
    {
        ERROR_LOG("Failed to create Renderpass internals.");
        return false;
    }

    C3D::Shader* SHADERS[] = { 0, 0, 0 };
    for (u8 i = 0; i < 3; i++)
    {
        const char* name = SHADER_NAMES[i];

        C3D::ShaderConfig config;
        if (!Resources.Load(name, config))
        {
            ERROR_LOG("Failed to load ShaderResource for: '{}'.", name);
            return false;
        }

        if (!Shaders.Create(m_pInternalData, config))
        {
            ERROR_LOG("Failed to Create: '{}'.", name);
            return false;
        }
        Resources.Unload(config);

        SHADERS[i] = Shaders.Get(name);
        if (!SHADERS[i])
        {
            ERROR_LOG("Failed to get the: '{}'.", name);
            return false;
        }
    }

    m_pbrShader     = SHADERS[0];
    m_terrainShader = SHADERS[1];
    m_colorShader   = SHADERS[2];

    m_debugLocations.view       = m_colorShader->GetUniformIndex("view");
    m_debugLocations.projection = m_colorShader->GetUniformIndex("projection");
    m_debugLocations.model      = m_colorShader->GetUniformIndex("model");

    m_geometries.SetAllocator(frameAllocator);
    m_terrains.SetAllocator(frameAllocator);
    m_debugGeometries.SetAllocator(frameAllocator);

    return true;
}

bool ScenePass::LoadResources()
{
    auto frameCount = Renderer.GetWindowAttachmentCount();

    auto shadowMapSink = GetSinkByName("SHADOW_MAP");
    if (!shadowMapSink)
    {
        ERROR_LOG("No Sink could be found with the name: 'SHADOW_MAP'.");
        return false;
    }

    m_shadowMapSource = shadowMapSink->boundSource;
    m_shadowMaps.Resize(frameCount);
    for (u32 s = 0; s < m_shadowMaps.Size(); s++)
    {
        auto& shadowMap         = m_shadowMaps[s];
        shadowMap.repeatU       = C3D::TextureRepeat::ClampToBorder;
        shadowMap.repeatV       = C3D::TextureRepeat::ClampToBorder;
        shadowMap.repeatW       = C3D::TextureRepeat::ClampToBorder;
        shadowMap.minifyFilter  = C3D::TextureFilter::ModeLinear;
        shadowMap.magnifyFilter = C3D::TextureFilter::ModeLinear;
        shadowMap.texture       = m_shadowMapSource->textures[s];
        shadowMap.generation    = INVALID_ID;

        if (!Renderer.AcquireTextureMapResources(shadowMap))
        {
            ERROR_LOG("Failed to acquire texture map resources for shadow map.");
            return false;
        }
    }

    return true;
}

bool ScenePass::Prepare(C3D::Viewport* viewport, C3D::Camera* camera, C3D::FrameData& frameData, const SimpleScene& scene, u32 renderMode,
                        const C3D::DynamicArray<C3D::DebugLine3D>& debugLines, const C3D::DynamicArray<C3D::DebugBox3D>& debugBoxes,
                        C3D::ShadowMapCascadeData* cascadeData)
{
    m_geometries.Reset();
    m_terrains.Reset();
    m_debugGeometries.Reset();

    m_viewport   = viewport;
    m_camera     = camera;
    m_renderMode = renderMode;

    // HACK: Use our skybox cube as irradiance texture for now
    m_irradianceCubeTexture = scene.GetSkybox()->cubeMap.texture;

    for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
    {
        m_directionalLightViews[i]       = cascadeData[i].view;
        m_directionalLightProjections[i] = cascadeData[i].projection;
    }

    m_cascadeSplits = vec4(cascadeData[0].splitDepth, cascadeData[1].splitDepth, cascadeData[2].splitDepth, cascadeData[3].splitDepth);

    // Update the frustum
    vec3 forward = camera->GetForward();
    vec3 right   = camera->GetRight();
    vec3 up      = camera->GetUp();

    const auto viewportRect = viewport->GetRect2D();
    const auto cameraPos    = camera->GetPosition();

    C3D::Frustum frustum = C3D::Frustum(cameraPos, forward, right, up, viewport);

    // Get all the meshes in our current frustum from the scene
    scene.QueryMeshes(frameData, frustum, cameraPos, m_geometries);
    frameData.drawnMeshCount = m_geometries.Size();

    // Get all terrains in our current frustum from the scene
    scene.QueryTerrains(frameData, frustum, cameraPos, m_terrains);
    frameData.drawnTerrainCount = m_terrains.Size();

    // Get all debug geometry from the scene
    scene.QueryDebugGeometry(frameData, m_debugGeometries);
    frameData.drawnDebugCount = m_debugGeometries.Size();

    // Get all debug lines from our main game
    for (const auto& line : debugLines)
    {
        m_debugGeometries.EmplaceBack(line.GetId(), line.GetModel(), line.GetGeometry());
    }

    // Get all debug boxes from our main game
    for (const auto& box : debugBoxes)
    {
        m_debugGeometries.EmplaceBack(box.GetId(), box.GetModel(), box.GetGeometry());
    }

    m_prepared = true;
    return true;
}

bool ScenePass::Execute(const C3D::FrameData& frameData)
{
    Renderer.SetActiveViewport(m_viewport);

    Begin(frameData);

    const auto& projectionMatrix = m_viewport->GetProjection();
    const auto& viewMatrix       = m_camera->GetViewMatrix();
    const auto& viewPosition     = m_camera->GetPosition();

    Materials.SetIrradiance(m_irradianceCubeTexture);

    for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; ++i)
    {
        mat4 lightSpace = m_directionalLightProjections[i] * m_directionalLightViews[i];
        Materials.SetDirectionalLightSpaceMatrix(lightSpace, i);
        Materials.SetShadowMap(m_shadowMapSource->textures[frameData.renderTargetIndex], i);
    }

    // Terrains
    if (!m_terrains.Empty())
    {
        if (!Shaders.UseById(m_terrainShader->id))
        {
            ERROR_LOG("Failed to use Terrain Shader.");
            return false;
        }

        // Apply globals
        if (!Materials.ApplyGlobal(m_terrainShader->id, frameData, &projectionMatrix, &viewMatrix, &m_cascadeSplits, &viewPosition,
                                   m_renderMode))
        {
            ERROR_LOG("Failed to apply globals for Terrain Shader.");
            return false;
        }

        for (const auto& terrain : m_terrains)
        {
            C3D::Material* m = terrain.material ? terrain.material : Materials.GetDefaultTerrain();

            bool needsUpdate = m->renderFrameNumber != frameData.frameNumber || m->renderDrawIndex != frameData.drawIndex;
            if (!Materials.ApplyInstance(m, frameData, needsUpdate))
            {
                WARN_LOG("Failed to apply Terrain Material: '{}'. Skipping.", m->name);
                continue;
            }

            // Sync the frame number and draw index
            m->renderFrameNumber = frameData.frameNumber;
            m->renderDrawIndex   = frameData.drawIndex;

            // Apply the locals
            Materials.ApplyLocal(frameData, m, &terrain.model);

            // Draw the terrain
            Renderer.DrawGeometry(terrain);
        }
    }

    // Static geometry
    if (!m_geometries.Empty())
    {
        if (!Shaders.UseById(m_pbrShader->id))
        {
            ERROR_LOG("Failed to use PBR Shader.");
            return false;
        }

        // Apply globals
        if (!Materials.ApplyGlobal(m_pbrShader->id, frameData, &projectionMatrix, &viewMatrix, &m_cascadeSplits, &viewPosition,
                                   m_renderMode))
        {
            ERROR_LOG("Failed to apply globals for PBR Shader.");
            return false;
        }

        u32 currentMaterialId = INVALID_ID;

        for (const auto& data : m_geometries)
        {
            C3D::Material* m = data.material ? data.material : Materials.GetDefault();

            if (m->id != currentMaterialId)
            {
                bool needsUpdate = m->renderFrameNumber != frameData.frameNumber || m->renderDrawIndex != frameData.drawIndex;
                if (!Materials.ApplyInstance(m, frameData, needsUpdate))
                {
                    WARN_LOG("Failed to apply Material: '{}'. Skipping.", m->name);
                    continue;
                }

                // Sync the frame number and draw index
                m->renderFrameNumber = frameData.frameNumber;
                m->renderDrawIndex   = frameData.drawIndex;

                currentMaterialId = m->id;
            }

            // Apply the locals
            Materials.ApplyLocal(frameData, m, &data.model);

            // Draw the static geometry
            Renderer.DrawGeometry(data);
        }
    }

    // Debug geometry
    if (!m_debugGeometries.Empty())
    {
        if (!Shaders.UseById(m_colorShader->id))
        {
            ERROR_LOG("Failed to use Color Shader.");
            return false;
        }

        // Globals
        Renderer.BindShaderGlobals(*m_colorShader);
        Shaders.SetUniformByIndex(m_debugLocations.projection, &projectionMatrix);
        Shaders.SetUniformByIndex(m_debugLocations.view, &viewMatrix);

        Shaders.ApplyGlobal(frameData, true);

        for (const auto& debug : m_debugGeometries)
        {
            // NOTE: No instance-level uniforms to be set here
            // Locals
            Shaders.BindLocal();
            Shaders.SetUniformByIndex(m_debugLocations.model, &debug.model);
            Shaders.ApplyLocal(frameData);

            // Draw it
            Renderer.DrawGeometry(debug);
        }

        // HACK: This should be handled better
        m_colorShader->frameNumber = frameData.frameNumber;
        m_colorShader->drawIndex   = frameData.drawIndex;
    }

    End();

    return true;
}