
#include "render_view_pick.h"

#include "core/colors.h"
#include "core/uuid.h"
#include "math/c3d_math.h"
#include "renderer/renderer_frontend.h"
#include "resources/loaders/shader_loader.h"
#include "resources/mesh.h"
#include "resources/ui_text.h"
#include "systems/cameras/camera_system.h"
#include "systems/events/event_system.h"
#include "systems/resources/resource_system.h"
#include "systems/shaders/shader_system.h"

namespace C3D
{
    RenderViewPick::RenderViewPick(const RenderViewConfig& config)
        : RenderView(ToUnderlying(RenderViewKnownType::Pick), config)
    {}

    bool RenderViewPick::OnCreate()
    {
        m_worldShaderInfo.pass   = passes[0];
        m_terrainShaderInfo.pass = passes[0];
        m_uiShaderInfo.pass      = passes[1];

        // UI Shader
        constexpr auto uiShaderName = "Shader.Builtin.UIPick";
        ShaderConfig shaderConfig;
        if (!Resources.Load(uiShaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load builtin UI Pick shader.");
            return false;
        }

        if (!Shaders.Create(m_uiShaderInfo.pass, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create builtin UI Pick Shader.");
            return false;
        }

        Resources.Unload(shaderConfig);
        m_uiShaderInfo.shader = Shaders.Get(uiShaderName);

        // Get the uniform locations
        m_uiShaderInfo.idColorLocation    = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "idColor");
        m_uiShaderInfo.modelLocation      = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "model");
        m_uiShaderInfo.projectionLocation = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "projection");
        m_uiShaderInfo.viewLocation       = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "view");

        // Default UI properties
        m_uiShaderInfo.nearClip = -100.0f;
        m_uiShaderInfo.farClip  = 100.0f;
        m_uiShaderInfo.fov      = 0;
        m_uiShaderInfo.projection =
            glm::ortho(0.0f, 1280.0f, 720.0f, 0.0f, m_uiShaderInfo.nearClip, m_uiShaderInfo.farClip);
        m_uiShaderInfo.view = mat4(1.0f);

        // World Shader
        constexpr auto worldShaderName = "Shader.Builtin.WorldPick";
        if (!Resources.Load(worldShaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load builtin World Pick shader.");
            return false;
        }

        if (!Shaders.Create(m_worldShaderInfo.pass, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create builtin World Pick Shader.");
            return false;
        }

        Resources.Unload(shaderConfig);
        m_worldShaderInfo.shader = Shaders.Get(worldShaderName);

        // Get the uniform locations
        m_worldShaderInfo.idColorLocation    = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "idColor");
        m_worldShaderInfo.modelLocation      = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "model");
        m_worldShaderInfo.projectionLocation = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "projection");
        m_worldShaderInfo.viewLocation       = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "view");

        // Default world properties
        m_worldShaderInfo.nearClip   = 0.1f;
        m_worldShaderInfo.farClip    = 4000.0f;
        m_worldShaderInfo.fov        = DegToRad(45.0f);
        m_worldShaderInfo.projection = glm::perspective(m_worldShaderInfo.fov, 1280 / 720.0f,
                                                        m_worldShaderInfo.nearClip, m_worldShaderInfo.farClip);
        m_worldShaderInfo.view       = mat4(1.0f);

        // Terrain Shader
        constexpr auto terrainShaderName = "Shader.Builtin.TerrainPick";
        if (!Resources.Load(terrainShaderName, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to load builtin Terrain Pick shader.");
            return false;
        }

        if (!Shaders.Create(m_terrainShaderInfo.pass, shaderConfig))
        {
            m_logger.Error("OnCreate() - Failed to create builtin World Pick Shader.");
            return false;
        }

        Resources.Unload(shaderConfig);
        m_terrainShaderInfo.shader = Shaders.Get(terrainShaderName);

        // Get the uniform locations
        m_terrainShaderInfo.idColorLocation    = Shaders.GetUniformIndex(m_terrainShaderInfo.shader, "idColor");
        m_terrainShaderInfo.modelLocation      = Shaders.GetUniformIndex(m_terrainShaderInfo.shader, "model");
        m_terrainShaderInfo.projectionLocation = Shaders.GetUniformIndex(m_terrainShaderInfo.shader, "projection");
        m_terrainShaderInfo.viewLocation       = Shaders.GetUniformIndex(m_terrainShaderInfo.shader, "view");

        // Default world properties
        m_terrainShaderInfo.nearClip   = 0.1f;
        m_terrainShaderInfo.farClip    = 4000.0f;
        m_terrainShaderInfo.fov        = DegToRad(45.0f);
        m_terrainShaderInfo.projection = glm::perspective(m_terrainShaderInfo.fov, 1280 / 720.0f,
                                                          m_terrainShaderInfo.nearClip, m_terrainShaderInfo.farClip);
        m_terrainShaderInfo.view       = mat4(1.0f);

        m_instanceCount = 0;

        std::memset(&m_colorTargetAttachmentTexture, 0, sizeof(Texture));
        std::memset(&m_depthTargetAttachmentTexture, 0, sizeof(Texture));

        m_onEventCallback =
            Event.Register(EventCodeMouseMoved, [this](const u16 code, void* sender, const EventContext& context) {
                return OnMouseMovedEvent(code, sender, context);
            });
        return true;
    }

    void RenderViewPick::OnDestroy()
    {
        RenderView::OnDestroy();
        Event.Unregister(m_onEventCallback);

        ReleaseShaderInstances();

        Renderer.DestroyTexture(&m_colorTargetAttachmentTexture);
        Renderer.DestroyTexture(&m_depthTargetAttachmentTexture);
    }

    void RenderViewPick::OnResize()
    {
        const auto fWidth  = static_cast<f32>(m_width);
        const auto fHeight = static_cast<f32>(m_height);
        const auto aspect  = fWidth / fHeight;

        m_uiShaderInfo.projection =
            glm::ortho(0.0f, fWidth, fHeight, 0.0f, m_uiShaderInfo.nearClip, m_uiShaderInfo.farClip);
        m_worldShaderInfo.projection =
            glm::perspective(m_worldShaderInfo.fov, aspect, m_worldShaderInfo.nearClip, m_worldShaderInfo.farClip);
        m_terrainShaderInfo.projection = glm::perspective(m_terrainShaderInfo.fov, aspect, m_terrainShaderInfo.nearClip,
                                                          m_terrainShaderInfo.farClip);
    }

    bool RenderViewPick::OnBuildPacket(LinearAllocator* frameAllocator, void* data, RenderViewPacket* outPacket)
    {
        if (!data || !outPacket)
        {
            m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
            return false;
        }

        const auto packetData = static_cast<PickPacketData*>(data);
        outPacket->view       = this;

        // TODO: Get active camera
        const auto worldCam    = Cam.GetDefault();
        m_worldShaderInfo.view = worldCam->GetViewMatrix();

        packetData->uiGeometryCount = 0;
        outPacket->extendedData     = frameAllocator->New<PickPacketData>(MemoryType::RenderView);

        u32 highestInstanceId = 0;

        // Iterate all terrains in world data
        const auto& terrainData = *packetData->terrainData;
        for (const auto& terrain : terrainData)
        {
            if (terrain.geometry->id == INVALID_ID) continue;

            outPacket->geometries.PushBack(terrain);
            packetData->terrainGeometryCount++;

            if (terrain.uniqueId > highestInstanceId)
            {
                highestInstanceId = terrain.uniqueId;
            }
        }

        // Iterate all geometries in world data
        const auto& worldMeshData = *packetData->worldMeshData;
        for (const auto& geometry : worldMeshData)
        {
            outPacket->geometries.PushBack(geometry);
            packetData->worldGeometryCount++;

            if (geometry.uniqueId > highestInstanceId)
            {
                highestInstanceId = geometry.uniqueId;
            }
        }

        for (const auto mesh : packetData->uiMeshData.meshes)
        {
            for (const auto geometry : mesh->geometries)
            {
                outPacket->geometries.EmplaceBack(mesh->transform.GetWorld(), geometry, mesh->uniqueId);
                packetData->uiGeometryCount++;
            }

            if (mesh->uniqueId > highestInstanceId)
            {
                highestInstanceId = mesh->uniqueId;
            }
        }

        for (const auto text : packetData->texts)
        {
            if (text->uniqueId > highestInstanceId)
            {
                highestInstanceId = text->uniqueId;
            }
        }

        // TODO: This needs to take into account the highest id, not the count, because they can skip ids.
        if (const u32 requiredInstanceCount = highestInstanceId + 1; requiredInstanceCount > m_instanceCount)
        {
            const auto diff = requiredInstanceCount - m_instanceCount;
            for (u32 i = 0; i < diff; i++)
            {
                AcquireShaderInstances();
            }
        }

        // Copy over the packet data
        auto outPickPacketData = static_cast<PickPacketData*>(outPacket->extendedData);
        *outPickPacketData     = *packetData;
        return true;
    }

    bool RenderViewPick::OnRender(const FrameData& frameData, const RenderViewPacket* packet, u64 frameNumber,
                                  u64 renderTargetIndex)
    {
        // We start at the 0-th pass (world)
        auto pass = passes[0];

        if (renderTargetIndex == 0)
        {
            // Reset
            for (auto& instance : m_instanceUpdated)
            {
                instance = false;
            }

            if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
            {
                m_logger.Error("OnRender() - BeginRenderPass() failed for pass: '{}'.", pass->GetName());
                return false;
            }

            const auto packetData = static_cast<PickPacketData*>(packet->extendedData);

            u32 currentInstanceId = 0;

            // Begin Terrain
            const auto terrainIndexStart = 0;
            const auto terrainIndexEnd   = terrainIndexStart + packetData->terrainGeometryCount;
            const auto terrainCount      = terrainIndexEnd - terrainIndexStart;

            if (terrainCount > 0)
            {
                if (!Shaders.UseById(m_terrainShaderInfo.shader->id))
                {
                    m_logger.Error("OnRender() - Failed to use terrain pick shader. Render frame failed.");
                    return false;
                }

                // Apply globals
                if (!Shaders.SetUniformByIndex(m_terrainShaderInfo.projectionLocation, &m_terrainShaderInfo.projection))
                {
                    m_logger.Error("OnRender() - Failed to apply projection matrix.");
                }

                if (!Shaders.SetUniformByIndex(m_terrainShaderInfo.viewLocation, &m_terrainShaderInfo.view))
                {
                    m_logger.Error("OnRender() - Failed to apply view matrix.");
                }

                if (!Shaders.ApplyGlobal())
                {
                    m_logger.Error("OnRender() - Failed to apply globals.");
                }

                // Draw terrain geometries.
                for (u32 i = terrainIndexStart; i < terrainIndexEnd; i++)
                {
                    auto& geo         = packet->geometries[i];
                    currentInstanceId = geo.uniqueId;

                    if (!Shaders.BindInstance(currentInstanceId))
                    {
                        m_logger.Error("OnRender() - Failed to bind instance with id: {}.", currentInstanceId);
                    }

                    u32 r, g, b;
                    U32ToRgb(geo.uniqueId, &r, &g, &b);
                    vec3 color = RgbToVec3(r, g, b);

                    if (!Shaders.SetUniformByIndex(m_terrainShaderInfo.idColorLocation, &color))
                    {
                        m_logger.Error("OnRender() - Failed to apply id color uniform.");
                        return false;
                    }

                    Shaders.ApplyInstance(!m_instanceUpdated[currentInstanceId]);
                    m_instanceUpdated[currentInstanceId] = true;

                    // Apply the locals
                    if (!Shaders.SetUniformByIndex(m_terrainShaderInfo.modelLocation, &geo.model))
                    {
                        m_logger.Error("OnRender() - Failed to apply model matrix for terrain geometry.");
                    }

                    // Actually draw the geometry
                    Renderer.DrawGeometry(packet->geometries[i]);
                }
            }
            // End Terrain

            // Begin World
            const auto worldIndexStart = terrainIndexEnd;
            const auto worldIndexEnd   = worldIndexStart + packetData->worldGeometryCount;
            const auto worldCount      = worldIndexEnd - worldIndexStart;

            if (worldCount > 0)
            {
                if (!Shaders.UseById(m_worldShaderInfo.shader->id))
                {
                    m_logger.Error("OnRender() - Failed to use world pick shader. Render frame failed.");
                    return false;
                }

                // Apply globals
                if (!Shaders.SetUniformByIndex(m_worldShaderInfo.projectionLocation, &m_worldShaderInfo.projection))
                {
                    m_logger.Error("OnRender() - Failed to apply projection matrix.");
                }

                if (!Shaders.SetUniformByIndex(m_worldShaderInfo.viewLocation, &m_worldShaderInfo.view))
                {
                    m_logger.Error("OnRender() - Failed to apply view matrix.");
                }

                if (!Shaders.ApplyGlobal())
                {
                    m_logger.Error("OnRender() - Failed to apply globals.");
                }

                // Draw world geometries.
                for (u32 i = worldIndexStart; i < worldIndexEnd; i++)
                {
                    auto& geo         = packet->geometries[i];
                    currentInstanceId = geo.uniqueId;

                    if (!Shaders.BindInstance(currentInstanceId))
                    {
                        m_logger.Error("OnRender() - Failed to bind instance with id: {}.", currentInstanceId);
                    }

                    u32 r, g, b;
                    U32ToRgb(geo.uniqueId, &r, &g, &b);
                    vec3 color = RgbToVec3(r, g, b);

                    if (!Shaders.SetUniformByIndex(m_worldShaderInfo.idColorLocation, &color))
                    {
                        m_logger.Error("OnRender() - Failed to apply id color uniform.");
                        return false;
                    }

                    Shaders.ApplyInstance(!m_instanceUpdated[currentInstanceId]);
                    m_instanceUpdated[currentInstanceId] = true;

                    // Apply the locals
                    if (!Shaders.SetUniformByIndex(m_worldShaderInfo.modelLocation, &geo.model))
                    {
                        m_logger.Error("OnRender() - Failed to apply model matrix for world geometry.");
                    }

                    // Actually draw the geometry
                    Renderer.DrawGeometry(packet->geometries[i]);
                }
            }
            // End World

            if (!Renderer.EndRenderPass(pass))
            {
                m_logger.Error("OnRender() - EndRenderPass() failed for pass: '{}'.", pass->id);
                return false;
            }

            // Second (UI) pass
            pass = passes[1];

            if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
            {
                m_logger.Error("OnRender() - BeginRenderPass() failed for pass: '{}'.", pass->id);
                return false;
            }

            // UI
            if (!Shaders.UseById(m_uiShaderInfo.shader->id))
            {
                m_logger.Error("OnRender() - Failed to use world pick shader. Render frame failed.");
                return false;
            }

            // Apply globals
            if (!Shaders.SetUniformByIndex(m_uiShaderInfo.projectionLocation, &m_uiShaderInfo.projection))
            {
                m_logger.Error("OnRender() - Failed to apply projection matrix.");
            }

            if (!Shaders.SetUniformByIndex(m_uiShaderInfo.viewLocation, &m_uiShaderInfo.view))
            {
                m_logger.Error("OnRender() - Failed to apply view matrix.");
            }

            if (!Shaders.ApplyGlobal())
            {
                m_logger.Error("OnRender() - Failed to apply globals.");
            }

            // Draw our ui geometries. We start where the terrain geometries left off.
            const auto uiIndexStart = worldIndexEnd;
            const auto uiIndexEnd   = uiIndexStart + packetData->uiGeometryCount;
            for (u64 i = uiIndexStart; i < uiIndexEnd; i++)
            {
                auto& geo         = packet->geometries[i];
                currentInstanceId = geo.uniqueId;

                if (!Shaders.BindInstance(currentInstanceId))
                {
                    m_logger.Error("OnRender() - Failed to bind instance with id: {}.", currentInstanceId);
                }

                u32 r, g, b;
                U32ToRgb(geo.uniqueId, &r, &g, &b);
                vec3 color = RgbToVec3(r, g, b);

                if (!Shaders.SetUniformByIndex(m_uiShaderInfo.idColorLocation, &color))
                {
                    m_logger.Error("OnRender() - Failed to apply id color uniform.");
                    return false;
                }

                Shaders.ApplyInstance(!m_instanceUpdated[currentInstanceId]);
                m_instanceUpdated[currentInstanceId] = true;

                // Apply the locals
                if (!Shaders.SetUniformByIndex(m_uiShaderInfo.modelLocation, &geo.model))
                {
                    m_logger.Error("OnRender() - Failed to apply model matrix for ui geometry.");
                }

                // Actually draw the geometry
                Renderer.DrawGeometry(packet->geometries[i]);
            }

            // Draw bitmap text
            for (const auto text : packetData->texts)
            {
                currentInstanceId = text->uniqueId;
                if (!Shaders.BindInstance(currentInstanceId))
                {
                    m_logger.Error("OnRender() - Failed to bind instance with id: {}.", currentInstanceId);
                }

                u32 r, g, b;
                U32ToRgb(text->uniqueId, &r, &g, &b);
                vec3 color = RgbToVec3(r, g, b);

                if (!Shaders.SetUniformByIndex(m_uiShaderInfo.idColorLocation, &color))
                {
                    m_logger.Error("OnRender() - Failed to apply id color uniform.");
                    return false;
                }

                if (!Shaders.ApplyInstance(true))
                {
                    m_logger.Error("OnRender() - Failed to apply instance.");
                }

                // Apply the locals
                mat4 model = text->transform.GetWorld();
                if (!Shaders.SetUniformByIndex(m_uiShaderInfo.modelLocation, &model))
                {
                    m_logger.Error("OnRender() - Failed to apply model matrix for text.");
                }

                // Actually draw the text
                text->Draw();
            }

            if (!Renderer.EndRenderPass(pass))
            {
                m_logger.Error("OnRender() - EndRenderPass() failed for pass: '{}'.", pass->id);
                return false;
            }
        }

        u8 pixelRgba[4] = { 0 };
        u8* pixel       = &pixelRgba[0];

        const u16 xCoord = C3D_CLAMP(m_mouseX, 0, m_width - 1);
        const u16 yCoord = C3D_CLAMP(m_mouseY, 0, m_height - 1);
        Renderer.ReadPixelFromTexture(&m_colorTargetAttachmentTexture, xCoord, yCoord, &pixel);

        // Extract the id from the sampled color
        auto id = RgbToU32(pixel[0], pixel[1], pixel[2]);
        if (id == 0x00FFFFFF)
        {
            // This is pure white.
            id = INVALID_ID;
        }

        EventContext context{};
        context.data.u32[0] = id;
        Event.Fire(EventCodeObjectHoverIdChanged, nullptr, context);

        return true;
    }

    void RenderViewPick::GetMatrices(mat4* outView, mat4* outProjection) {}

    bool RenderViewPick::RegenerateAttachmentTarget(u32 passIndex, RenderTargetAttachment* attachment)
    {
        if (attachment->type == RenderTargetAttachmentType::Color)
        {
            attachment->texture = &m_colorTargetAttachmentTexture;
        }
        else if (attachment->type == RenderTargetAttachmentType::Depth)
        {
            attachment->texture = &m_depthTargetAttachmentTexture;
        }
        else
        {
            m_logger.Error("RegenerateAttachmentTarget() - Unknown attachment type: '{}'",
                           ToUnderlying(attachment->type));
        }

        if (passIndex == 1)
        {
            // No need to regenerate for both passes since they both use the same attachment.
            return true;
        }

        if (attachment->texture->internalData)
        {
            Renderer.DestroyTexture(attachment->texture);
            std::memset(attachment->texture, 0, sizeof(Texture));
        }

        // Setup a new texture
        // Generate a UUID to act as the name
        const auto textureNameUUID = UUIDS::Generate();

        const u32 width  = passes[passIndex]->renderArea.z;
        const u32 height = passes[passIndex]->renderArea.w;
        // TODO: make this configurable
        constexpr bool hasTransparency = false;

        attachment->texture->id     = INVALID_ID;
        attachment->texture->type   = TextureType::Type2D;
        attachment->texture->name   = textureNameUUID.value;
        attachment->texture->width  = width;
        attachment->texture->height = height;
        // TODO: Configurable
        attachment->texture->channelCount = 4;
        attachment->texture->generation   = INVALID_ID;
        attachment->texture->flags |= hasTransparency ? TextureFlag::HasTransparency : 0;
        attachment->texture->flags |= TextureFlag::IsWritable;
        if (attachment->type == RenderTargetAttachmentType::Depth)
        {
            attachment->texture->flags |= TextureFlag::IsDepth;
        }
        attachment->texture->internalData = nullptr;

        Renderer.CreateWritableTexture(attachment->texture);
        return true;
    }

    bool RenderViewPick::OnMouseMovedEvent(const u16 code, void*, const EventContext& context)
    {
        if (code == EventCodeMouseMoved)
        {
            m_mouseX = context.data.i16[0];
            m_mouseY = context.data.i16[1];
            return true;
        }

        return false;
    }

    void RenderViewPick::AcquireShaderInstances()
    {
        u32 instance;
        if (!Renderer.AcquireShaderInstanceResources(m_uiShaderInfo.shader, 0, nullptr, &instance))
        {
            m_logger.Fatal("AcquireShaderInstances() - Failed to acquire UI shader resources from Renderer.");
        }

        if (!Renderer.AcquireShaderInstanceResources(m_worldShaderInfo.shader, 0, nullptr, &instance))
        {
            m_logger.Fatal("AcquireShaderInstances() - Failed to acquire World shader resources from Renderer.");
        }

        if (!Renderer.AcquireShaderInstanceResources(m_terrainShaderInfo.shader, 0, nullptr, &instance))
        {
            m_logger.Fatal("AcquireShaderInstances() - Failed to acquire Terrain shader resources from Renderer.");
        }

        m_instanceCount++;
        m_instanceUpdated.PushBack(false);
    }

    void RenderViewPick::ReleaseShaderInstances()
    {
        for (u32 i = 0; i < m_instanceCount; i++)
        {
            if (!Renderer.ReleaseShaderInstanceResources(m_uiShaderInfo.shader, i))
            {
                m_logger.Warn("ReleaseShaderInstances() - Failed to release UI shader resources.");
            }

            if (!Renderer.ReleaseShaderInstanceResources(m_worldShaderInfo.shader, i))
            {
                m_logger.Warn("ReleaseShaderInstances() - Failed to release World shader resources.");
            }
        }
        m_instanceUpdated.Clear();
    }
}  // namespace C3D
