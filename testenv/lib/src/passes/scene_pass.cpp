
#include "scene_pass.h"

#include <math/frustum.h>
#include <renderer/camera.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <resources/debug/debug_box_3d.h>
#include <resources/debug/debug_line_3d.h>
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
constexpr const char* MATERIAL_SHADER_NAME = "Shader.Builtin.Material";
constexpr const char* TERRAIN_SHADER_NAME  = "Shader.Builtin.Terrain";
constexpr const char* COLOR_3D_SHADER_NAME = "Shader.Builtin.Color3D";
constexpr const char* PBR_SHADER_NAME      = "Shader.PBR";

constexpr const char* SHADER_NAMES[4] = { MATERIAL_SHADER_NAME, TERRAIN_SHADER_NAME, COLOR_3D_SHADER_NAME, PBR_SHADER_NAME };

ScenePass::ScenePass() : Renderpass() {}

ScenePass::ScenePass(const C3D::SystemManager* pSystemsManager) : Renderpass("SCENE", pSystemsManager) {}

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

    C3D::Shader* SHADERS[4] = { 0, 0, 0, 0 };
    for (u8 i = 0; i < 4; i++)
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

    m_shader        = SHADERS[0];
    m_terrainShader = SHADERS[1];
    m_colorShader   = SHADERS[2];
    m_pbrShader     = SHADERS[3];

    m_debugLocations.view       = m_colorShader->GetUniformIndex("view");
    m_debugLocations.projection = m_colorShader->GetUniformIndex("projection");
    m_debugLocations.model      = m_colorShader->GetUniformIndex("model");

    m_geometries.SetAllocator(frameAllocator);
    m_terrains.SetAllocator(frameAllocator);
    m_debugGeometries.SetAllocator(frameAllocator);
    m_transparentGeometries.SetAllocator(frameAllocator);

    return true;
}

bool ScenePass::LoadResources()
{
    auto shadowMapSink = GetSinkByName("SHADOW_MAP");
    if (!shadowMapSink)
    {
        ERROR_LOG("No Sink could be found for the SHADOW_MAP.");
        return false;
    }

    m_shadowMapSource = shadowMapSink->boundSource;
    m_shadowMaps.Resize(Renderer.GetWindowAttachmentCount());
    for (u32 i = 0; i < m_shadowMaps.Size(); i++)
    {
        auto& shadowMap         = m_shadowMaps[i];
        shadowMap.repeatU       = C3D::TextureRepeat::ClampToEdge;
        shadowMap.repeatV       = C3D::TextureRepeat::ClampToEdge;
        shadowMap.minifyFilter  = C3D::TextureFilter::ModeLinear;
        shadowMap.magnifyFilter = C3D::TextureFilter::ModeLinear;
        shadowMap.texture       = m_shadowMapSource->textures[i];
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
                        const mat4& shadowCameraLookat, const mat4& shadowCameraProjection)
{
    m_geometries.Reset();
    m_terrains.Reset();
    m_debugGeometries.Reset();
    m_transparentGeometries.Reset();

    m_viewport   = viewport;
    m_camera     = camera;
    m_renderMode = renderMode;

    // HACK: Use our skybox cube as irradiance texture for now
    m_irradianceCubeTexture      = scene.GetSkybox()->cubeMap.texture;
    m_directionalLightView       = shadowCameraLookat;
    m_directionalLightProjection = shadowCameraProjection;

    // Update the frustum
    vec3 forward = camera->GetForward();
    vec3 right   = camera->GetRight();
    vec3 up      = camera->GetUp();

    const auto viewportRect = viewport->GetRect2D();

    C3D::Frustum frustum = C3D::Frustum(camera->GetPosition(), forward, right, up, viewport);

    for (const auto& mesh : scene.m_meshes)
    {
        if (mesh.generation != INVALID_ID_U8)
        {
            mat4 model           = mesh.transform.GetWorld();
            bool windingInverted = mesh.transform.GetDeterminant() < 0;

            if (mesh.HasDebugBox())
            {
                const auto box = mesh.GetDebugBox();
                if (box->IsValid())
                {
                    m_debugGeometries.EmplaceBack(box->GetId(), box->GetModel(), box->GetGeometry());
                }
            }

            for (const auto geometry : mesh.geometries)
            {
                // AABB Calculation
                const vec3 extentsMax = model * vec4(geometry->extents.max, 1.0f);
                const vec3 center     = model * vec4(geometry->center, 1.0f);

                const vec3 halfExtents = {
                    C3D::Abs(extentsMax.x - center.x),
                    C3D::Abs(extentsMax.y - center.y),
                    C3D::Abs(extentsMax.z - center.z),
                };

                if (frustum.IntersectsWithAABB({ center, halfExtents }))
                {
                    C3D::GeometryRenderData data(mesh.GetId(), model, geometry, windingInverted);

                    // Check if transparent. If so, put into a separate, temp array to be
                    // sorted by distance from the camera. Otherwise, we can just directly insert into the geometries dynamic array
                    if (geometry->material->type == C3D::MaterialType::Phong &&
                        geometry->material->maps[0].texture->flags & C3D::TextureFlag::HasTransparency)
                    {
                        // For meshes _with_ transparency, add them to a separate list to be sorted by distance later.
                        // Get the center, extract the global position from the model matrix and add it to the center,
                        // then calculate the distance between it and the camera, and finally save it to a list to be sorted.
                        // NOTE: This isn't perfect for translucent meshes that intersect, but is enough for our purposes now.
                        vec3 center  = model * vec4(geometry->center, 1.0f);
                        f32 distance = glm::distance(center, m_camera->GetPosition());

                        m_transparentGeometries.EmplaceBack(data, distance);
                    }
                    else
                    {
                        m_geometries.PushBack(data);
                    }

                    m_geometries.EmplaceBack(mesh.GetId(), model, geometry, windingInverted);
                    frameData.drawnMeshCount++;
                }
            }
        }
    }

    // Sort opaque geometries by material.
    std::sort(m_geometries.begin(), m_geometries.end(), [](const C3D::GeometryRenderData& a, const C3D::GeometryRenderData& b) {
        return a.material->internalId < b.material->internalId;
    });

    // Sort transparent geometries, then add them to our geometries array.
    std::sort(m_transparentGeometries.begin(), m_transparentGeometries.end(),
              [](const GeometryDistance& a, const GeometryDistance& b) { return a.distance > b.distance; });

    for (auto& tg : m_transparentGeometries)
    {
        m_geometries.PushBack(tg.g);
    }

    for (auto& terrain : scene.m_terrains)
    {
        if (terrain.GetId())
        {
            // TODO: Check terrain generation
            // TODO: Frustum culling
            m_terrains.EmplaceBack(terrain.GetId(), terrain.GetModel(), terrain.GetGeometry());
            // TODO: Seperate counter for terrain meshes/geometry
            frameData.drawnMeshCount++;
        }
    }

    // Debug geometry
    // Grid
    constexpr auto identity = mat4(1.0f);
    auto gridGeometry       = scene.m_grid.GetGeometry();
    if (gridGeometry->generation != INVALID_ID_U16)
    {
        m_debugGeometries.EmplaceBack(scene.m_grid.GetId(), identity, gridGeometry);
    }

    // TODO: Directional lights

    // Point Lights
    for (auto& name : scene.m_pointLights)
    {
        auto light = Lights.GetPointLight(name);
        auto debug = static_cast<LightDebugData*>(light->debugData);

        if (debug)
        {
            m_debugGeometries.EmplaceBack(debug->box.GetId(), debug->box.GetModel(), debug->box.GetGeometry());
        }
    }

    // Debug geometry
    for (const auto& line : debugLines)
    {
        m_debugGeometries.EmplaceBack(line.GetId(), line.GetModel(), line.GetGeometry());
    }

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

    // HACK: This is just here for padding
    vec4 ambientColor = vec4(0.99, 0.98, 0.97, 0.96);

    mat4 lightSpace = m_directionalLightProjection * m_directionalLightView;
    Materials.SetDirectionalLightSpaceMatrix(lightSpace);
    // TODO: index for cascading
    Materials.SetShadowMap(m_shadowMapSource->textures[frameData.renderTargetIndex], 0);

    // Terrains
    if (!m_terrains.Empty())
    {
        if (!Shaders.UseById(m_terrainShader->id))
        {
            ERROR_LOG("Failed to use Terrain Shader.");
            return false;
        }

        // Apply globals
        if (!Materials.ApplyGlobal(m_terrainShader->id, frameData, &projectionMatrix, &viewMatrix, &ambientColor, &viewPosition,
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
            Materials.ApplyLocal(m, &terrain.model);

            // Draw the terrain
            Renderer.DrawGeometry(terrain);
        }
    }

    // Static geometry
    if (!m_geometries.Empty())
    {
        // Update globals for material and PBR shader
        if (!Shaders.UseById(m_shader->id))
        {
            ERROR_LOG("Failed to use Material Shader.");
            return false;
        }

        // Apply globals
        if (!Materials.ApplyGlobal(m_shader->id, frameData, &projectionMatrix, &viewMatrix, &ambientColor, &viewPosition, m_renderMode))
        {
            ERROR_LOG("Failed to apply globals for Material Shader.");
            return false;
        }

        if (!Shaders.UseById(m_pbrShader->id))
        {
            ERROR_LOG("Failed to use PBR Shader.");
            return false;
        }

        // Apply globals
        if (!Materials.ApplyGlobal(m_pbrShader->id, frameData, &projectionMatrix, &viewMatrix, &ambientColor, &viewPosition, m_renderMode))
        {
            ERROR_LOG("Failed to apply globals for PBR Shader.");
            return false;
        }

        u32 currentMaterialId                 = INVALID_ID;
        C3D::MaterialType currentMaterialType = C3D::MaterialType::Unkown;

        for (const auto& data : m_geometries)
        {
            C3D::Material* m = data.material ? data.material : Materials.GetDefault();

            // Swap shaders if the material type changes
            if (m->type != currentMaterialType)
            {
                if (!Shaders.UseById(m->type == C3D::MaterialType::PBR ? m_pbrShader->id : m_shader->id))
                {
                    ERROR_LOG("Failed to switch shaders on material change.");
                    return false;
                }
                currentMaterialType = m->type;
            }

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
            Materials.ApplyLocal(m, &data.model);

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
        Shaders.SetUniformByIndex(m_debugLocations.projection, &projectionMatrix);
        Shaders.SetUniformByIndex(m_debugLocations.view, &viewMatrix);

        Shaders.ApplyGlobal(true);

        for (const auto& debug : m_debugGeometries)
        {
            // NOTE: No instance-level uniforms to be set here
            // Locals
            Shaders.SetUniformByIndex(m_debugLocations.model, &debug.model);

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