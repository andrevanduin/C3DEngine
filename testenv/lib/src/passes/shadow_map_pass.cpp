
#include "shadow_map_pass.h"

#include <math/frustum.h>
#include <renderer/camera.h>
#include <renderer/geometry.h>
#include <renderer/renderer_frontend.h>
#include <renderer/viewport.h>
#include <resources/mesh.h>
#include <resources/shaders/shader_types.h>
#include <resources/terrain/terrain.h>
#include <resources/textures/texture.h>
#include <systems/lights/light_system.h>
#include <systems/materials/material_system.h>
#include <systems/resources/resource_system.h>
#include <systems/shaders/shader_system.h>
#include <systems/system_manager.h>
#include <systems/textures/texture_system.h>

#include "resources/scenes/simple_scene.h"
#include "resources/skybox.h"

constexpr const char* INSTANCE_NAME       = "SHADOW_PASS";
constexpr const char* SHADER_NAME         = "Shader.ShadowMap";
constexpr const char* TERRAIN_SHADER_NAME = "Shader.ShadowMapTerrain";

ShadowMapPass::ShadowMapPass() : Renderpass() {}

ShadowMapPass::ShadowMapPass(const C3D::SystemManager* pSystemsManager, const ShadowMapPassConfig& config)
    : Renderpass("SHADOW", pSystemsManager), m_config(config)
{}

bool ShadowMapPass::Initialize(const C3D::LinearAllocator* frameAllocator)
{
    u8 attachmentCount = Renderer.GetWindowAttachmentCount();

    m_colorTextures.Resize(attachmentCount);
    m_depthTextures.Resize(attachmentCount);

    for (u32 i = 0; i < attachmentCount; ++i)
    {
        // Color
        auto& ct = m_colorTextures[i];
        ct.flags |= C3D::TextureFlag::IsWritable;
        ct.width        = m_config.resolution;
        ct.height       = m_config.resolution;
        ct.name         = C3D::String::FromFormat("SHADOW_MAP_PASS_{}_{}x{}_COLOR_TEXTURE", i, m_config.resolution, m_config.resolution);
        ct.mipLevels    = 1;
        ct.channelCount = 4;
        ct.generation   = INVALID_ID;

        Renderer.CreateWritableTexture(&ct);

        // Depth
        auto& dt = m_depthTextures[i];
        dt.flags |= C3D::TextureFlag::IsDepth | C3D::TextureFlag::IsWritable;
        dt.width        = m_config.resolution;
        dt.height       = m_config.resolution;
        dt.name         = C3D::String::FromFormat("SHADOW_MAP_PASS_{}_{}x{}_DEPTH_TEXTURE", i, m_config.resolution, m_config.resolution);
        dt.mipLevels    = 1;
        dt.channelCount = 4;
        dt.generation   = INVALID_ID;

        Renderer.CreateWritableTexture(&dt);
    }

    // Setup our renderpass
    C3D::RenderpassConfig pass;
    pass.name       = "Renderpass.ShadowMap";
    pass.clearColor = { 0, 0, 0.2f, 1.0f };
    pass.clearFlags = C3D::ClearColorBuffer | C3D::ClearDepthBuffer;
    pass.depth      = 1.0f;
    pass.stencil    = 0;

    C3D::RenderTargetAttachmentConfig targetAttachments[2]{};
    // Color attachment
    targetAttachments[0].type           = C3D::RenderTargetAttachmentTypeColor;
    targetAttachments[0].source         = C3D::RenderTargetAttachmentSource::Self;
    targetAttachments[0].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    targetAttachments[0].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[0].presentAfter   = false;

    // Depth attachment
    targetAttachments[1].type           = C3D::RenderTargetAttachmentTypeDepth;
    targetAttachments[1].source         = C3D::RenderTargetAttachmentSource::Self;
    targetAttachments[1].loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
    targetAttachments[1].storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
    targetAttachments[1].presentAfter   = true;

    pass.target.attachments.PushBack(targetAttachments[0]);
    pass.target.attachments.PushBack(targetAttachments[1]);
    pass.renderTargetCount = attachmentCount;

    if (!CreateInternals(pass))
    {
        ERROR_LOG("Failed to create Renderpass internals.");
        return false;
    }

    C3D::ShaderConfig config;
    if (!Resources.Load(SHADER_NAME, config))
    {
        ERROR_LOG("Failed to load ShaderResource for: '{}'.", SHADER_NAME);
        return false;
    }

    if (!Shaders.Create(m_pInternalData, config))
    {
        ERROR_LOG("Failed to Create: '{}'.", SHADER_NAME);
        return false;
    }
    Resources.Unload(config);

    m_shader = Shaders.Get(SHADER_NAME);
    if (!m_shader)
    {
        ERROR_LOG("Failed to get the: '{}'.", SHADER_NAME);
        return false;
    }

    if (!Resources.Load(TERRAIN_SHADER_NAME, config))
    {
        ERROR_LOG("Failed to load ShaderResource for: '{}'.", TERRAIN_SHADER_NAME);
        return false;
    }

    if (!Shaders.Create(m_pInternalData, config))
    {
        ERROR_LOG("Failed to Create: '{}'.", TERRAIN_SHADER_NAME);
        return false;
    }
    Resources.Unload(config);

    m_terrainShader = Shaders.Get(TERRAIN_SHADER_NAME);
    if (!m_terrainShader)
    {
        ERROR_LOG("Failed to get the: '{}'.", TERRAIN_SHADER_NAME);
        return false;
    }

    m_locations.projection = m_shader->GetUniformIndex("projection");
    m_locations.view       = m_shader->GetUniformIndex("view");
    m_locations.model      = m_shader->GetUniformIndex("model");
    m_locations.colorMap   = m_shader->GetUniformIndex("colorMap");

    m_terrainLocations.projection = m_terrainShader->GetUniformIndex("projection");
    m_terrainLocations.view       = m_terrainShader->GetUniformIndex("view");
    m_terrainLocations.model      = m_terrainShader->GetUniformIndex("model");
    m_terrainLocations.colorMap   = m_terrainShader->GetUniformIndex("colorMap");

    m_geometries.SetAllocator(frameAllocator);
    m_terrains.SetAllocator(frameAllocator);
    m_transparentGeometries.SetAllocator(frameAllocator);

    return true;
}

bool ShadowMapPass::LoadResources()
{
    // Create a default texture map to be used for diffuse/albedo transparency samples
    m_defaultColorMap.mipLevels     = 1;
    m_defaultColorMap.generation    = INVALID_ID;
    m_defaultColorMap.repeatU       = C3D::TextureRepeat::ClampToEdge;
    m_defaultColorMap.repeatV       = C3D::TextureRepeat::ClampToEdge;
    m_defaultColorMap.repeatW       = C3D::TextureRepeat::ClampToEdge;
    m_defaultColorMap.minifyFilter  = C3D::TextureFilter::ModeLinear;
    m_defaultColorMap.magnifyFilter = C3D::TextureFilter::ModeLinear;
    m_defaultColorMap.texture       = Textures.GetDefaultDiffuse();

    // Create a default terrain texture map to be used for diffuse/albedo transparency samples
    m_defaultTerrainColorMap.mipLevels     = 1;
    m_defaultTerrainColorMap.generation    = INVALID_ID;
    m_defaultTerrainColorMap.repeatU       = C3D::TextureRepeat::ClampToEdge;
    m_defaultTerrainColorMap.repeatV       = C3D::TextureRepeat::ClampToEdge;
    m_defaultTerrainColorMap.repeatW       = C3D::TextureRepeat::ClampToEdge;
    m_defaultTerrainColorMap.minifyFilter  = C3D::TextureFilter::ModeLinear;
    m_defaultTerrainColorMap.magnifyFilter = C3D::TextureFilter::ModeLinear;
    m_defaultTerrainColorMap.texture       = Textures.GetDefaultDiffuse();

    if (!Renderer.AcquireTextureMapResources(m_defaultColorMap))
    {
        ERROR_LOG("Failed to acquire texture map resources for default color map.");
        return false;
    }

    if (!Renderer.AcquireTextureMapResources(m_defaultTerrainColorMap))
    {
        ERROR_LOG("Failed to acquire texture map resources for default terrain color map.");
        return false;
    }

    const C3D::TextureMap* maps[1] = { &m_defaultColorMap };
    if (!Renderer.AcquireShaderInstanceResources(*m_shader, 1, maps, &m_defaultInstanceId))
    {
        ERROR_LOG("Failed to acquire instance resources for default color map.");
        return false;
    }

    const C3D::TextureMap* terrainMaps[1] = { &m_defaultTerrainColorMap };
    if (!Renderer.AcquireShaderInstanceResources(*m_terrainShader, 1, terrainMaps, &m_terrainInstanceId))
    {
        ERROR_LOG("Failed to acquire instance resources for default terrain color map.");
        return false;
    }

    // Calculate viewport
    // HACK: Defaulting to ortho viewport. Point lights need perspective and a FOV.
    C3D::Rect2D viewportRect = { 0, 0, static_cast<f32>(m_config.resolution), static_cast<f32>(m_config.resolution) };
    if (!m_viewport.Create(viewportRect, m_config.fov, m_config.nearClip, m_config.farClip,
                           C3D::RendererProjectionMatrixType::Orthographic))
    {
        ERROR_LOG("Failed to create viewport.");
        return false;
    }

    // NOTE: for cascading, might need several.
    // NOTE: Don't use the viewport projection matrix, as this will result in really small objects.
    // TODO: Create the projection matrix based on the extents of all visible shadow-casting/recieving objects.
    if (m_config.matrixType == C3D::RendererProjectionMatrixType::Orthographic)
    {
        m_projectionMatrix = glm::ortho(m_config.bounds.x, m_config.bounds.y, m_config.bounds.width, m_config.bounds.height,
                                        m_config.nearClip, m_config.farClip);
    }
    else
    {
        m_projectionMatrix =
            glm::perspective(m_config.fov, (m_config.bounds.width / m_config.bounds.height), m_config.nearClip, m_config.farClip);
    }

    m_viewport.SetProjectionMatrix(m_projectionMatrix);

    return true;
}

bool ShadowMapPass::Prepare(const SimpleScene& scene)
{
    m_geometries.Reset();
    m_transparentGeometries.Reset();
    m_terrains.Reset();

    m_directionalLight = Lights.GetDirectionalLight();

    mat4 shadowCameraLookat     = mat4(1.0f);
    mat4 shadowCameraProjection = mat4(1.0f);

    vec3 lightDir = glm::normalize(vec3(m_directionalLight->data.direction));
    // NOTE: each pass for cascades will need to do this
    // Light direction is down (negative) so we need to go up
    vec3 shadowCamPos  = lightDir * -100.0f;
    shadowCameraLookat = glm::lookAt(shadowCamPos, vec3(0), C3D::VEC3_UP);

    // NOTE: this pass will use its own viewport.
    /* state->shadowmap_pass.pass_data.vp = &state->world_viewport; */
    // HACK: TODO: View matrix needs to be inverse.
    m_viewMatrix = shadowCameraLookat;

    // Read internal projection matrix.
    shadowCameraProjection = m_projectionMatrix;

    // Iterate the scene and get a list of all geometries within the view of the light.
    m_geometries.Reserve(512);
    m_transparentGeometries.Reserve(16);
    m_terrains.Reserve(16);

    for (auto& mesh : scene.m_meshes)
    {
        if (mesh.generation != INVALID_ID_U8)
        {
            mat4 model           = mesh.transform.GetWorld();
            bool windingInverted = mesh.transform.GetDeterminant() < 0;

            for (const auto geometry : mesh.geometries)
            {
                // AABB calculation
                {
                    // // Translate/scale the extents.
                    // // vec3 extents_min = vec3_mul_mat4(g->extents.min, model);
                    // vec3 extents_max = vec3_mul_mat4(g->extents.max, model);

                    // // Translate/scale the center.
                    // vec3 center = vec3_mul_mat4(g->center, model);
                    // vec3 half_extents = {
                    //     kabs(extents_max.x - center.x),
                    //     kabs(extents_max.y - center.y),
                    //     kabs(extents_max.z - center.z),
                    // };

                    // if (frustum_intersects_aabb(&f, &center, &half_extents)) {
                    // Add it to the list to be rendered.
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
                    // }
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

    // Add terrain(s)
    for (auto& terrain : scene.m_terrains)
    {
        if (terrain.GetId())
        {
            m_terrains.EmplaceBack(terrain.GetId(), terrain.GetModel(), terrain.GetGeometry());
        }
    }

    m_prepared = true;
    return true;
}

bool ShadowMapPass::Execute(const C3D::FrameData& frameData)
{
    Renderer.SetActiveViewport(&m_viewport);

    Begin(frameData);

    if (!Shaders.UseById(m_shader->id))
    {
        ERROR_LOG("Failed to use Shader.");
        return false;
    }

    // Apply globals
    Renderer.BindShaderGlobals(*m_shader);
    // NOTE: We use our internal projection matrix (not the one passed in)
    if (!Shaders.SetUniformByIndex(m_locations.projection, &m_projectionMatrix))
    {
        ERROR_LOG("Failed to set projection.");
        return false;
    }

    if (!Shaders.SetUniformByIndex(m_locations.view, &m_viewMatrix))
    {
        ERROR_LOG("Failed to set view.");
        return false;
    }

    Shaders.ApplyGlobal(true);

    // Ensure we have enough instances for every geometry by finding the highest internalId and adding 1 for the default
    u32 highestId = 0;
    for (auto& geometry : m_geometries)
    {
        C3D::Material* m = geometry.material;
        if (m->internalId > highestId)
        {
            // NOTE: Use +1 to account for default instance
            highestId = m->internalId + 1;
        }
    }

    // Increment by 1 for the terrains
    highestId++;

    if (highestId > m_instanceCount)
    {
        // We need more resources for our instances
        for (u32 i = m_instanceCount; i < highestId; i++)
        {
            u32 instanceId;
            const C3D::TextureMap* maps[1] = { &m_defaultColorMap };
            Renderer.AcquireShaderInstanceResources(*m_shader, 1, maps, &instanceId);
        }
        m_instanceCount = highestId;
    }

    // Static geometries
    for (auto& geometry : m_geometries)
    {
        u32 bindId               = INVALID_ID;
        C3D::TextureMap* bindMap = 0;
        u64* renderNumber        = 0;
        u8* drawIndex            = 0;

        // Decide what bindings to use.
        if (geometry.material && !geometry.material->maps.Empty())
        {
            // Use current material's internal id.
            // NOTE: +1 to account for the first id being taken by the default instance.
            bindId = geometry.material->internalId + 1;
            // Use the current material's diffuse/albedo map.
            bindMap      = &geometry.material->maps[0];
            renderNumber = &m_shader->frameNumber;
            drawIndex    = &m_shader->drawIndex;
        }
        else
        {
            // Use the default instance.
            bindId = m_defaultInstanceId;
            // Use the default colour map.
            bindMap      = &m_defaultColorMap;
            renderNumber = &m_defaultInstanceFrameNumber;
            drawIndex    = &m_defaultInstanceDrawIndex;
        }

        bool needsUpdate = *renderNumber != frameData.frameNumber || *drawIndex != frameData.drawIndex;

        // Use the bindings.
        Shaders.BindInstance(bindId);
        if (!Shaders.SetUniformByIndex(m_locations.colorMap, bindMap))
        {
            ERROR_LOG("Failed to apply shadowmap colorMap uniform to static geometry.");
            return false;
        }
        Shaders.ApplyInstance(needsUpdate);

        // Sync the frame number and draw index.
        *renderNumber = frameData.frameNumber;
        *drawIndex    = frameData.drawIndex;

        // Apply the locals
        Shaders.SetUniformByIndex(m_locations.model, &geometry.model);

        Renderer.DrawGeometry(geometry);
    }

    // Terrain
    Shaders.UseById(m_terrainShader->id);

    // Apply globals
    Renderer.BindShaderGlobals(*m_terrainShader);
    // NOTE: We use our internal projection matrix (not the one passed in)
    if (!Shaders.SetUniformByIndex(m_terrainLocations.projection, &m_projectionMatrix))
    {
        ERROR_LOG("Failed to set projection for terrain.");
        return false;
    }

    if (!Shaders.SetUniformByIndex(m_terrainLocations.view, &m_viewMatrix))
    {
        ERROR_LOG("Failed to set view for terrain.");
        return false;
    }

    Shaders.ApplyGlobal(true);

    for (auto& terrain : m_terrains)
    {
        bool needsUpdate = m_terrainInstanceFrameNumber != frameData.frameNumber || m_terrainInstanceDrawIndex != frameData.drawIndex;

        Shaders.BindInstance(m_terrainInstanceId);
        if (!Shaders.SetUniformByIndex(m_terrainLocations.colorMap, &m_defaultTerrainColorMap))
        {
            ERROR_LOG("Failed to apply shadowmap colorMap uniform to terrain geometry.");
            return false;
        }
        Shaders.ApplyInstance(needsUpdate);

        m_terrainInstanceFrameNumber = frameData.frameNumber;
        m_terrainInstanceDrawIndex   = frameData.drawIndex;

        // Apply the locals
        Shaders.SetUniformByIndex(m_terrainLocations.model, &terrain.model);

        // Draw it
        Renderer.DrawGeometry(terrain);
    }

    End();

    return true;
}

void ShadowMapPass::Destroy()
{
    INFO_LOG("Destroying internal color and depth textures.");
    for (auto& t : m_colorTextures)
    {
        Renderer.DestroyTexture(&t);
    }
    m_colorTextures.Destroy();

    for (auto& t : m_depthTextures)
    {
        Renderer.DestroyTexture(&t);
    }
    m_depthTextures.Destroy();

    INFO_LOG("Releasing texture map and shader instance resources.");
    Renderer.ReleaseTextureMapResources(m_defaultColorMap);
    Renderer.ReleaseTextureMapResources(m_defaultTerrainColorMap);
    Renderer.ReleaseShaderInstanceResources(*m_shader, m_defaultInstanceId);
    Renderer.ReleaseShaderInstanceResources(*m_terrainShader, m_terrainInstanceId);

    INFO_LOG("Destroying internals.");
    Renderpass::Destroy();
}

C3D::Texture* ShadowMapPass::GetAttachmentTexture(C3D::RenderTargetAttachmentType type, u8 frameNumber)
{
    if (type == C3D::RenderTargetAttachmentTypeColor)
    {
        return &m_colorTextures[frameNumber];
    }
    else if (type & C3D::RenderTargetAttachmentTypeDepth)
    {
        return &m_depthTextures[frameNumber];
    }

    ERROR_LOG("Unknown attachment type: {}. Returning nullptr.", type);
    return nullptr;
}