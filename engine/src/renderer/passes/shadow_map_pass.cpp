
#include "shadow_map_pass.h"

#include "math/frustum.h"
#include "renderer/camera.h"
#include "renderer/geometry.h"
#include "renderer/renderer_frontend.h"
#include "renderer/viewport.h"
#include "resources/loaders/shader_loader.h"
#include "resources/mesh.h"
#include "resources/shaders/shader_types.h"
#include "resources/skybox.h"
#include "resources/terrain/terrain.h"
#include "resources/textures/texture.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    constexpr const char* SHADER_NAME         = "Shader.ShadowMap";
    constexpr const char* TERRAIN_SHADER_NAME = "Shader.ShadowMapTerrain";

    ShadowMapPass::ShadowMapPass() : Renderpass() {}

    ShadowMapPass::ShadowMapPass(const C3D::String& name, const ShadowMapPassConfig& config) : Renderpass(name), m_config(config) {}

    bool ShadowMapPass::Initialize(const C3D::LinearAllocator* frameAllocator)
    {
        u8 frameCount = Renderer.GetWindowAttachmentCount();

        m_depthTextures.Reserve(frameCount);

        for (u32 i = 0; i < frameCount; ++i)
        {
            auto name   = C3D::String::FromFormat("SHADOW_MAP_PASS_{}x{}_DEPTH_TEXTURE_{}", m_config.resolution, m_config.resolution, i);
            auto handle = Textures.AcquireArrayWritable(name, m_config.resolution, m_config.resolution, 4, MAX_SHADOW_CASCADE_COUNT,
                                                        TextureFlag::IsDepth);

            m_depthTextures.PushBack(handle);
        }

        // Setup our renderpass
        C3D::RenderpassConfig pass;
        pass.name       = "Renderpass.ShadowMap";
        pass.clearColor = { 0, 0, 0.2f, 1.0f };
        pass.clearFlags = C3D::ClearDepthBuffer;
        pass.depth      = 1.0f;
        pass.stencil    = 0;

        C3D::RenderTargetAttachmentConfig targetAttachment;
        // Depth attachment
        targetAttachment.type           = C3D::RenderTargetAttachmentTypeDepth;
        targetAttachment.source         = C3D::RenderTargetAttachmentSource::Self;
        targetAttachment.loadOperation  = C3D::RenderTargetAttachmentLoadOperation::DontCare;
        targetAttachment.storeOperation = C3D::RenderTargetAttachmentStoreOperation::Store;
        targetAttachment.presentAfter   = true;

        pass.target.attachments.PushBack(targetAttachment);
        pass.renderTargetCount = frameCount;

        if (!CreateInternals(pass))
        {
            ERROR_LOG("Failed to create Renderpass internals.");
            return false;
        }

        // Get our shader
        m_shader = Shaders.Get(SHADER_NAME);
        if (!m_shader)
        {
            // If it does not yet exist we load and create it
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
        }

        // Get our terrain shader
        m_terrainShader = Shaders.Get(TERRAIN_SHADER_NAME);
        if (!m_terrainShader)
        {
            // If it does not yet exist we load and create it
            C3D::ShaderConfig config;
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
        }

        m_locations.projections  = m_shader->GetUniformIndex("projections");
        m_locations.views        = m_shader->GetUniformIndex("views");
        m_locations.model        = m_shader->GetUniformIndex("model");
        m_locations.cascadeIndex = m_shader->GetUniformIndex("cascadeIndex");
        m_locations.colorMap     = m_shader->GetUniformIndex("colorMap");

        m_terrainLocations.projections  = m_terrainShader->GetUniformIndex("projections");
        m_terrainLocations.views        = m_terrainShader->GetUniformIndex("views");
        m_terrainLocations.model        = m_terrainShader->GetUniformIndex("model");
        m_terrainLocations.cascadeIndex = m_terrainShader->GetUniformIndex("cascadeIndex");
        m_terrainLocations.colorMap     = m_terrainShader->GetUniformIndex("colorMap");

        m_cullingData.geometries.SetAllocator(frameAllocator);
        m_cullingData.terrains.SetAllocator(frameAllocator);

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

        {
            // Reserve an instance id for the default material to render to
            C3D::TextureMap* maps[1] = { &m_defaultColorMap };

            ShaderInstanceResourceConfig config;
            ShaderInstanceUniformTextureConfig textureConfig;
            textureConfig.uniformLocation = m_locations.colorMap;
            textureConfig.textureMapCount = 1;
            textureConfig.textureMaps     = maps;

            config.uniformConfigCount = 1;
            config.uniformConfigs     = &textureConfig;

            if (!Renderer.AcquireShaderInstanceResources(*m_shader, config, m_defaultInstanceId))
            {
                ERROR_LOG("Failed to acquire instance resources for default color map.");
                return false;
            }
        }

        {
            // Reserve an instance id for the default material to render to
            C3D::TextureMap* maps[1] = { &m_defaultTerrainColorMap };

            ShaderInstanceResourceConfig config;
            ShaderInstanceUniformTextureConfig textureConfig;
            textureConfig.uniformLocation = m_terrainLocations.colorMap;
            textureConfig.textureMapCount = 1;
            textureConfig.textureMaps     = maps;

            config.uniformConfigCount = 1;
            config.uniformConfigs     = &textureConfig;

            if (!Renderer.AcquireShaderInstanceResources(*m_terrainShader, config, m_defaultTerrainInstanceId))
            {
                ERROR_LOG("Failed to acquire instance resources for default terrain color map.");
                return false;
            }
        }

        // Setup default viewport. We only use the underlying viewport rect but for rendering it's required to set the viewport.
        // The projection matrix in here is not used which is why we can ignore fov and clip planes
        C3D::Rect2D viewportRect = { 0, 0, static_cast<f32>(m_config.resolution), static_cast<f32>(m_config.resolution) };
        if (!m_viewport.Create(viewportRect, 0.0f, 0.0f, 0.0f, C3D::RendererProjectionMatrixType::Orthographic))
        {
            ERROR_LOG("Failed to create viewport.");
            return false;
        }

        // Create the depth attachments, one per frame.
        u8 frameCount = Renderer.GetWindowAttachmentCount();
        // Renderpass attachments.
        for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
        {
            auto& cascade = m_cascades[i];
            // Targets per frame
            cascade.targets = Memory.NewArray<RenderTarget>(MemoryType::Array, frameCount);
            for (u32 f = 0; f < frameCount; ++f)
            {
                // One render target per pass
                auto& target = cascade.targets[f];

                RenderTargetAttachment attachment;
                attachment.type           = RenderTargetAttachmentTypeDepth;
                attachment.source         = RenderTargetAttachmentSource::Self;
                attachment.texture        = m_depthTextures[f];
                attachment.presentAfter   = true;
                attachment.loadOperation  = RenderTargetAttachmentLoadOperation::DontCare;
                attachment.storeOperation = RenderTargetAttachmentStoreOperation::Store;

                target.attachments.PushBack(attachment);

                // Create the underlying render target.
                Renderer.CreateRenderTarget(m_pInternalData, target, i, m_config.resolution, m_config.resolution);
            }
        }

        return true;
    }

    bool ShadowMapPass::Prepare(FrameData& frameData, const Viewport& viewport, Camera* camera)
    {
        m_cullingData.geometries.Reset();
        m_cullingData.terrains.Reset();

        auto dirLight = Lights.GetDirectionalLight();

        f32 nearClip  = viewport.GetNearClip();
        f32 farClip   = dirLight ? (dirLight->data.shadowDistance + dirLight->data.shadowFadeDistance) : 0.0f;
        f32 clipRange = farClip - nearClip;

        f32 minZ  = nearClip;
        f32 maxZ  = nearClip + clipRange;
        f32 range = maxZ - minZ;
        f32 ratio = maxZ / minZ;

        f32 cascadeSplitMultiplier = dirLight ? dirLight->data.shadowSplitMultiplier : 0.0f;

        vec4 splits;
        for (u32 c = 0; c < MAX_SHADOW_CASCADE_COUNT; ++c)
        {
            f32 p       = (c + 1) / static_cast<f32>(MAX_SHADOW_CASCADE_COUNT);
            f32 log     = minZ * C3D::Pow(ratio, p);
            f32 uniform = minZ + range * p;

            f32 d     = cascadeSplitMultiplier * (log - uniform) + uniform;
            splits[c] = (d - nearClip) / clipRange;
        }

        mat4 shadowCameraLookats[4]     = { mat4(1.0f), mat4(1.0f), mat4(1.0f) };
        mat4 shadowCameraProjections[4] = { mat4(1.0f), mat4(1.0f), mat4(1.0f) };
        vec3 shadowCameraPositions[4]   = { vec3(0), vec3(0), vec3(0) };

        if (dirLight)
        {
            // Keep track of the last split distance
            f32 lastSplitDist = 0.0f;
            // Obtain our current direction light's direction
            m_cullingData.lightDirection = glm::normalize(vec3(dirLight->data.direction));

            // Get the view-projection matrix
            mat4 shadowDistProjection = glm::perspective(viewport.GetFov(), viewport.GetAspectRatio(), nearClip, farClip);
            mat4 camViewProjection    = glm::transpose(shadowDistProjection * camera->GetViewMatrix());

            for (u32 c = 0; c < MAX_SHADOW_CASCADE_COUNT; c++)
            {
                auto& cascade        = m_cascadeData[c];
                cascade.cascadeIndex = c;

                // Get the corners of the view frustum in world-space.
                vec4 corners[8];
                C3D::FrustumCornerPointsInWorldSpace(camViewProjection, corners);

                // Adjust the corners by pulling/pushing the near/far according to the current split
                f32 splitDist = splits[c];
                for (u32 i = 0; i < 4; i++)
                {
                    vec4 dist      = corners[i + 4] - corners[i];
                    corners[i + 4] = corners[i] + (dist * splitDist);
                    corners[i]     = corners[i] + (dist * lastSplitDist);
                }

                // Calculate the center of the camera's frustum by averaging the points.
                // This is also used as the lookat point for the shadow "camera".
                vec3 center = vec3(0);
                for (u32 i = 0; i < 8; ++i)
                {
                    center += vec3(corners[i]);
                }
                center /= 8.0f;
                // Get the furthest-out point from the center and use that as our extents.
                f32 radius = 0.f;
                for (u32 i = 0; i < 8; ++i)
                {
                    f32 distance = glm::distance(vec3(corners[i]), center);
                    radius       = C3D::Max(radius, distance);
                }
                // Calculate the extents by using the radius
                C3D::Extents3D extents;
                extents.max = vec3(radius);
                extents.min = extents.max * -1.0f;

                // "Pull" the min inward and "push" the max outwards on the z-axis to ensure that shadow casters outside ofr the view are
                // also captured.
                // TODO: Make this configurable
                constexpr f32 zMultiplier = 10.0f;
                if (extents.min.z < 0)
                {
                    extents.min.z *= zMultiplier;
                }
                else
                {
                    extents.min.z /= zMultiplier;
                }

                if (extents.max.z < 0)
                {
                    extents.max.z /= zMultiplier;
                }
                else
                {
                    extents.max.z *= zMultiplier;
                }

                // Generate lookat by moving along the opposite direction of the directional light by the
                // minimum extents. This is negated because the directional light points "down" and the camera
                // needs to be "up".
                shadowCameraPositions[c] = center - (m_cullingData.lightDirection * -extents.min.z);
                shadowCameraLookats[c]   = glm::lookAt(shadowCameraPositions[c], center, C3D::VEC3_UP);

                // Generate ortho projection based on extents
                shadowCameraProjections[c] =
                    glm::ortho(extents.min.x, extents.max.x, extents.min.y, extents.max.y, extents.min.z, extents.max.z - extents.min.z);

                // Save these values off to be used in Execture()
                cascade.view       = shadowCameraLookats[c];
                cascade.projection = shadowCameraProjections[c];
                cascade.splitDepth = (nearClip + splitDist * clipRange) * 1.0f;

                if (c == MAX_SHADOW_CASCADE_COUNT - 1)
                {
                    m_cullingData.radius = radius;
                    m_cullingData.center = center;
                }

                lastSplitDist = splitDist;
            }
        }

        m_prepared = true;
        return true;
    }

    bool ShadowMapPass::Execute(const C3D::FrameData& frameData)
    {
        Renderer.SetActiveViewport(&m_viewport);

        for (u32 c = 0; c < MAX_SHADOW_CASCADE_COUNT; c++)
        {
            const auto& cascade = m_cascadeData[c];
            const auto& target  = m_cascades[c].targets[frameData.renderTargetIndex];
            Renderer.BeginRenderpass(m_pInternalData, target);

            if (!Shaders.UseById(m_shader->id))
            {
                ERROR_LOG("Failed to use Shader.");
                return false;
            }

            // Only the first iteration we need to update our globals (since we set them all at once)
            bool globalsNeedUpdate = (c == 0);
            if (globalsNeedUpdate)
            {
                // Apply globals
                Renderer.BindShaderGlobals(*m_shader);
                for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
                {
                    // NOTE: We use our internal projection matrix (not the one passed in)
                    if (!Shaders.SetArrayUniformByIndex(m_locations.projections, i, &m_cascadeData[i].projection))
                    {
                        ERROR_LOG("Failed to set projection.");
                        return false;
                    }

                    if (!Shaders.SetArrayUniformByIndex(m_locations.views, i, &m_cascadeData[i].view))
                    {
                        ERROR_LOG("Failed to set view.");
                        return false;
                    }
                }
            }

            Shaders.ApplyGlobal(frameData, globalsNeedUpdate);

            // Ensure we have enough instances for every geometry by finding the highest internalId and adding 1 for the default
            u32 highestId = 0;
            for (auto& geometry : m_cullingData.geometries)
            {
                C3D::Material* m = geometry.material;
                if (m && m->internalId > highestId)
                {
                    // NOTE: Use +1 to account for default instance
                    highestId = m->internalId + 1;
                }
            }

            // Increment by 1 for the terrains
            highestId++;

            if (highestId > m_instanceCount)
            {
                // Clear out our old instance data
                m_instances.Clear();
                // Reserve enough space for all our instance data
                m_instances.Reserve(highestId + 1);

                // We need more resources for our instances
                for (u32 i = m_instanceCount; i < highestId; i++)
                {
                    u32 instanceId;
                    C3D::TextureMap* maps[1] = { &m_defaultColorMap };

                    ShaderInstanceResourceConfig instanceConfig;
                    ShaderInstanceUniformTextureConfig textureConfig;
                    textureConfig.uniformLocation = m_locations.colorMap;
                    textureConfig.textureMapCount = 1;
                    textureConfig.textureMaps     = maps;

                    instanceConfig.uniformConfigCount = 1;
                    instanceConfig.uniformConfigs     = &textureConfig;

                    Renderer.AcquireShaderInstanceResources(*m_shader, instanceConfig, instanceId);
                    m_instances.PushBack(ShadowShaderInstanceData());
                }
                m_instanceCount = highestId;
            }

            // Static geometries
            for (auto& geometry : m_cullingData.geometries)
            {
                u32 bindId               = INVALID_ID;
                C3D::TextureMap* bindMap = nullptr;
                u64* renderNumber        = nullptr;
                u8* drawIndex            = nullptr;

                // Decide what bindings to use.
                if (geometry.material && !geometry.material->maps.Empty())
                {
                    // Use current material's internal id.
                    // NOTE: +1 to account for the first id being taken by the default instance.
                    bindId = geometry.material->internalId + 1;
                    // Use the current material's diffuse/albedo map.
                    bindMap = &geometry.material->maps[0];

                    auto& instance = m_instances[geometry.material->internalId + 1];
                    renderNumber   = &instance.frameNumber;
                    drawIndex      = &instance.drawIndex;
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
                Shaders.ApplyInstance(frameData, needsUpdate);

                // Sync the frame number and draw index.
                *renderNumber = frameData.frameNumber;
                *drawIndex    = frameData.drawIndex;

                // Apply the locals
                Shaders.BindLocal();
                Shaders.SetUniformByIndex(m_locations.model, &geometry.model);
                Shaders.SetUniformByIndex(m_locations.cascadeIndex, &c);
                Shaders.ApplyLocal(frameData);

                Renderer.DrawGeometry(geometry);
            }

            // Terrain
            Shaders.UseById(m_terrainShader->id);

            if (globalsNeedUpdate)
            {
                // Apply globals
                Renderer.BindShaderGlobals(*m_terrainShader);
                for (u32 i = 0; i < MAX_SHADOW_CASCADE_COUNT; i++)
                {
                    if (!Shaders.SetArrayUniformByIndex(m_terrainLocations.projections, i, &m_cascadeData[i].projection))
                    {
                        ERROR_LOG("Failed to set projection for terrain.");
                        return false;
                    }

                    if (!Shaders.SetArrayUniformByIndex(m_terrainLocations.views, i, &m_cascadeData[i].view))
                    {
                        ERROR_LOG("Failed to set view for terrain.");
                        return false;
                    }
                }
            }
            Shaders.ApplyGlobal(frameData, globalsNeedUpdate);

            for (auto& terrain : m_cullingData.terrains)
            {
                bool needsUpdate = m_defaultTerrainInstanceFrameNumber != frameData.frameNumber ||
                                   m_defaultTerrainInstanceDrawIndex != frameData.drawIndex;

                Shaders.BindInstance(m_defaultTerrainInstanceId);
                if (!Shaders.SetUniformByIndex(m_terrainLocations.colorMap, &m_defaultTerrainColorMap))
                {
                    ERROR_LOG("Failed to apply shadowmap colorMap uniform to terrain geometry.");
                    return false;
                }
                Shaders.ApplyInstance(frameData, needsUpdate);

                m_defaultTerrainInstanceFrameNumber = frameData.frameNumber;
                m_defaultTerrainInstanceDrawIndex   = frameData.drawIndex;

                // Apply the locals
                Shaders.BindLocal();
                Shaders.SetUniformByIndex(m_terrainLocations.model, &terrain.model);
                Shaders.SetUniformByIndex(m_terrainLocations.cascadeIndex, &c);
                Shaders.ApplyLocal(frameData);

                // Draw it
                Renderer.DrawGeometry(terrain);
            }

            End();
        }

        return true;
    }

    void ShadowMapPass::Destroy()
    {
        INFO_LOG("Destroying render targets")
        u8 frameCount = Renderer.GetWindowAttachmentCount();

        for (auto& cascade : m_cascades)
        {
            for (u32 f = 0; f < frameCount; ++f)
            {
                auto& target = cascade.targets[f];
                Renderer.DestroyRenderTarget(target, true);
            }

            Memory.DeleteArray(cascade.targets, frameCount);
        }

        INFO_LOG("Destroying internal depth textures.");
        for (auto& t : m_depthTextures)
        {
            Textures.Release(t);
        }
        m_depthTextures.Destroy();

        INFO_LOG("Releasing texture map and shader instance resources.");
        Renderer.ReleaseTextureMapResources(m_defaultColorMap);
        Renderer.ReleaseTextureMapResources(m_defaultTerrainColorMap);
        Renderer.ReleaseShaderInstanceResources(*m_shader, m_defaultInstanceId);
        Renderer.ReleaseShaderInstanceResources(*m_terrainShader, m_defaultTerrainInstanceId);

        INFO_LOG("Destroying internals.");
        Renderpass::Destroy();
    }

    bool ShadowMapPass::PopulateSource(RendergraphSource& source)
    {
        u32 frameCount = Renderer.GetWindowAttachmentCount();
        if (source.textures.Empty())
        {
            source.textures.Resize(frameCount);
        }

        if (source.name.IEquals("DEPTH_BUFFER"))
        {
            // Ensure we only populate once we actually have depth textures
            if (m_depthTextures.Size() == frameCount)
            {
                for (u32 i = 0; i < frameCount; ++i)
                {
                    source.textures[i] = m_depthTextures[i];
                }
            }

            return true;
        }

        ERROR_LOG("Could not populate source: '{}' as it was not recognized.", source.name);
        return false;
    }

    bool ShadowMapPass::PopulateAttachment(RenderTargetAttachment& attachment)
    {
        if (attachment.type == RenderTargetAttachmentTypeDepth)
        {
            attachment.texture = m_depthTextures[0];
            return true;
        }

        return false;
    }
}  // namespace C3D