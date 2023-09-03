
#include "simple_scene.h"

#include "core/frame_data.h"
#include "renderer/renderer_types.h"
#include "resources/mesh.h"
#include "systems/lights/light_system.h"
#include "systems/render_views/render_view_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    static u32 global_scene_id = 0;

    SimpleScene::SimpleScene() : m_logger("SIMPLE_SCENE"), m_name("NO_NAME"), m_description("NO_DESCRIPTION") {}

    bool SimpleScene::Create(const SystemManager* pSystemsManager) { return Create(pSystemsManager, {}); }

    bool SimpleScene::Create(const SystemManager* pSystemsManager, const SimpleSceneConfig& config)
    {
        m_pSystemsManager = pSystemsManager;

        m_enabled = false;
        m_state   = SceneState::Uninitialized;
        m_id      = global_scene_id++;

        m_skybox = nullptr;

        m_meshes.Create(1024);

        m_config = config;
        return true;
    }

    bool SimpleScene::Initialize()
    {
        if (!m_config.name.Empty())
        {
            m_name = m_config.name;
        }

        if (!m_config.description.Empty())
        {
            m_description = m_config.description;
        }

        if (!m_config.skyboxConfig.name.Empty() && !m_config.skyboxConfig.cubemapName.Empty())
        {
            SkyboxConfig config = { m_config.skyboxConfig.cubemapName };
            m_skybox            = Memory.New<Skybox>(MemoryType::Scene);
            if (!m_skybox->Create(m_pSystemsManager, config))
            {
                m_logger.Error("Create() - Failed to create skybox from config");
                Memory.Delete(MemoryType::Scene, m_skybox);
                return false;
            }

            AddSkybox(m_config.skyboxConfig.name, m_skybox);
        }

        if (!m_config.directionalLightConfig.name.Empty())
        {
            const auto dirLight = DirectionalLight(m_config.directionalLightConfig);
            m_directionalLight  = m_config.directionalLightConfig.name;

            if (!Lights.AddDirectionalLight(dirLight))
            {
                m_logger.Error("Create() - Failed to add directional light from config");
                return false;
            }
        }

        if (!m_config.pointLights.Empty())
        {
            for (auto& pointLightConfig : m_config.pointLights)
            {
                AddPointLight(PointLight(pointLightConfig));
            }
        }

        if (!m_config.meshes.Empty())
        {
            for (auto& meshConfig : m_config.meshes)
            {
                if (meshConfig.name.Empty() || meshConfig.resourceName.Empty())
                {
                    m_logger.Warn("Initialize() - Mesh with empty name or empty resource name provided. Skipping");
                    continue;
                }

                auto config = MeshConfig(meshConfig);
                Mesh mesh(config);
                if (!mesh.Create(m_pSystemsManager, config))
                {
                    m_logger.Error("Initialize() - Failed to create mesh: '{}'. Skipping", meshConfig.name);
                    continue;
                }
                mesh.transform = meshConfig.transform;
                m_meshes.Set(meshConfig.name, mesh);
            }
        }

        // Handle mesh hierarchy
        for (auto& mesh : m_meshes)
        {
            if (!mesh.config.parentName.Empty())
            {
                try
                {
                    auto& parent = GetMesh(mesh.config.parentName);
                    mesh.transform.SetParent(&parent.transform);
                } catch (const std::invalid_argument& exc)
                {
                    m_logger.Warn(
                        "Initialize() - Mesh: '{}' was configured to have mesh named: '{}' as a parent. But the parent "
                        "does not exist in this scene",
                        mesh.config.name, mesh.config.parentName);
                }
            }
        }

        if (m_skybox)
        {
            if (!m_skybox->Initialize())
            {
                m_logger.Error("Initialize() - Failed to initialize skybox");
                m_skybox = nullptr;
                return false;
            }
        }

        for (auto& mesh : m_meshes)
        {
            if (!mesh.Initialize())
            {
                m_logger.Error("Initialize() - Failed to initialize mesh");
                return false;
            }
        }

        m_state = SceneState::Initialized;
        return true;
    }

    bool SimpleScene::Load()
    {
        m_state = SceneState::Loading;

        if (m_skybox && m_skybox->instanceId == INVALID_ID)
        {
            // Skybox exists but is not loaded yet
            if (!m_skybox->Load())
            {
                m_logger.Error("Load() - Failed to load skybox");
                m_skybox = nullptr;
                return false;
            }
        }

        for (auto& mesh : m_meshes)
        {
            if (!mesh.Load())
            {
                m_logger.Error("Load() - Failed to load mesh");
                return false;
            }
        }

        m_state = SceneState::Loaded;
        return true;
    }

    bool SimpleScene::Unload(bool immediate /* = false*/)
    {
        m_state = SceneState::Unloading;
        if (immediate)
        {
            UnloadInternal();
        }
        return true;
    }

    bool SimpleScene::Update(FrameData& frameData)
    {
        if (m_state == SceneState::Unloading)
        {
            UnloadInternal();
            return true;
        }

        if (m_state != SceneState::Loaded) return true;

        // TODO: Update the scene
        return true;
    }

    bool SimpleScene::PopulateRenderPacket(FrameData& frameData, RenderPacket& packet)
    {
        if (m_state != SceneState::Loaded) return true;

        // TODO: Cache this somewhere so we don't check every time
        if (m_skybox)
        {
            for (auto& viewPacket : packet.views)
            {
                const auto view = viewPacket.view;
                if (view->type == RenderViewKnownType::Skybox)
                {
                    SkyboxPacketData skyboxData = { m_skybox };
                    if (!Views.BuildPacket(view, frameData.frameAllocator, &skyboxData, &viewPacket))
                    {
                        m_logger.Error("PopulateRenderPacket() - Failed to populate render packet with skybox data");
                        return false;
                    }
                    break;
                }
            }
        }

        for (auto& viewPacket : packet.views)
        {
            const auto view = viewPacket.view;
            if (view->type == RenderViewKnownType::World)
            {
                // Reserve a reasonable amount of space for our world geometries
                m_worldGeometries.Reset();
                m_worldGeometries.SetAllocator(frameData.frameAllocator);
                m_worldGeometries.Reserve(512);

                for (const auto& mesh : m_meshes)
                {
                    // auto meshSet = false;

                    if (mesh.generation != INVALID_ID_U8)
                    {
                        mat4 model = mesh.transform.GetWorld();

                        for (const auto geometry : mesh.geometries)
                        {
                            m_worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId);
                            frameData.drawnMeshCount++;

                            /*
                            // Bounding sphere calculations
                            vec3 extentsMin = vec4(geometry->extents.min, 1.0f) * model;
                            vec3 extentsMax = vec4(geometry->extents.max, 1.0f) * model;

                            f32 min = C3D::Min(extentsMin.x, extentsMin.y, extentsMin.z);
                            f32 max = C3D::Max(extentsMax.x, extentsMax.y, extentsMax.z);
                            f32 diff = C3D::Abs(max - min);
                            f32 radius = diff * 0.5f;

                            // Translate / Scale the center
                            const vec3 center = vec4(geometry->center, 1.0f) * model;

                            if (m_cameraFrustum.IntersectsWithSphere({ center, radius }))
                            {
                                    m_frameData.worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId);
                                    drawCount++;
                            }

                            // AABB Calculation
                            const vec3 extentsMin = vec4(geometry->extents.min, 1.0f);
                            const vec3 extentsMax = vec4(geometry->extents.max, 1.0f);
                            const vec3 center = vec4(geometry->center, 1.0f);

                            const vec3 halfExtents =
                            {
                                    C3D::Abs(extentsMax.x - center.x),
                                    C3D::Abs(extentsMax.y - center.y),
                                    C3D::Abs(extentsMax.z - center.z),
                            };

                            if (!m_hasPrimitives[count])
                            {
                                    // Only add primitives for the mesh once
                                    auto pMesh = m_primitiveRenderer.AddBox(geometry->center, halfExtents);
                                    pMesh->transform.SetParent(&mesh.transform);

                                    m_primitiveMeshes[m_primitiveMeshCount] = pMesh;
                                    m_primitiveMeshCount++;
                                    meshSet = true;
                            }

                            if (m_cameraFrustum.IntersectsWithAABB({ geometry->center, halfExtents }))
                            {
                                    m_frameData.worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId);
                                    drawCount++;
                            }*/
                        }
                    }

                    /*
                    if (meshSet)
                    {
                            m_hasPrimitives[count] = true;
                    }*/
                }

                if (!Views.BuildPacket(view, frameData.frameAllocator, &m_worldGeometries, &viewPacket))
                {
                    m_logger.Error("PopulateRenderPacket() - Failed to populate render packet with world data");
                    return false;
                }
                break;
            }
        }

        return true;
    }

    bool SimpleScene::AddDirectionalLight(const String& name, DirectionalLight& light)
    {
        if (name.Empty())
        {
            m_logger.Error("AddDirectionalLight() - Empty name provided");
            return false;
        }

        if (!m_directionalLight.Empty())
        {
            // TODO: Do resource unloading when required
            if (!Lights.RemoveDirectionalLight(m_directionalLight))
            {
                m_logger.Error("AddDirectionalLight() - Failed to remove current directional light");
                return false;
            }
        }

        m_directionalLight = name;
        return Lights.AddDirectionalLight(light);
    }

    bool SimpleScene::RemoveDirectionalLight(const String& name)
    {
        if (name.Empty())
        {
            m_logger.Error("RemoveDirectionalLight() - Empty name provided");
            return false;
        }

        if (m_directionalLight)
        {
            const auto result  = Lights.RemoveDirectionalLight(m_directionalLight);
            m_directionalLight = "";
            return result;
        }

        m_logger.Warn("RemoveDirectionalLight() - Could not remove since provided light is not part of this scene");
        return false;
    }

    bool SimpleScene::AddPointLight(const PointLight& light)
    {
        if (!Lights.AddPointLight(light))
        {
            m_logger.Error("AddPointLight() - Failed to add point light to lighting system");
            return false;
        }
        m_pointLights.PushBack(light.name);
        return true;
    }

    bool SimpleScene::RemovePointLight(const String& name)
    {
        auto result = Lights.RemovePointLight(name);
        if (result)
        {
            m_pointLights.Remove(name);
            return true;
        }

        m_logger.Error("RemovePointLight() - Failed to remove Point Light");
        return false;
    }

    PointLight* SimpleScene::GetPointLight(const String& name) { return Lights.GetPointLight(name); }

    bool SimpleScene::AddMesh(const String& name, Mesh& mesh)
    {
        if (name.Empty())
        {
            m_logger.Error("AddMesh() - Empty name provided");
            return false;
        }

        if (m_meshes.Has(name))
        {
            m_logger.Error("AddMesh() - A mesh with the name '{}' already exists", name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the mesh now
            if (!mesh.Initialize())
            {
                m_logger.Error("AddMesh() - Failed to initialize mesh");
                return false;
            }
        }

        if (m_state >= SceneState::Loading)
        {
            if (!mesh.Load())
            {
                m_logger.Error("AddMesh() - Failed to load mesh");
                return false;
            }
        }

        m_meshes.Set(name, mesh);
        return true;
    }

    bool SimpleScene::RemoveMesh(const String& name)
    {
        if (name.Empty())
        {
            m_logger.Error("RemoveMesh() - Empty name provided");
            return false;
        }

        if (!m_meshes.Has(name))
        {
            m_logger.Error("RemoveMesh() - Unknown name provided");
            return false;
        }

        auto& mesh = m_meshes.Get(name);
        if (!mesh.Unload())
        {
            m_logger.Error("RemoveMesh() - Failed to unload mesh");
            return false;
        }

        return true;
    }

    Mesh& SimpleScene::GetMesh(const String& name) { return m_meshes.Get(name); }

    bool SimpleScene::AddSkybox(const String& name, Skybox* skybox)
    {
        if (name.Empty())
        {
            m_logger.Error("AddSkybox() - Empty name provided");
            return false;
        }

        if (!skybox)
        {
            m_logger.Error("AddSkybox() - Invalid skybox provided");
            return false;
        }

        // TODO: If one already exists, what do we do?
        m_skybox = skybox;

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the skybox now
            if (!m_skybox->Initialize())
            {
                m_logger.Error("AddSkybox() - Failed to initialize skybox");
                m_skybox = nullptr;
                return false;
            }
        }

        if (m_state == SceneState::Loading || m_state == SceneState::Loaded)
        {
            if (!m_skybox->Load())
            {
                m_logger.Error("AddSkybox() - Failed to load skybox");
                m_skybox = nullptr;
                return false;
            }
        }

        return true;
    }

    bool SimpleScene::RemoveSkybox(const String& name)
    {
        if (name.Empty())
        {
            m_logger.Error("RemoveSkybox() - Empty name provided");
            return false;
        }

        if (m_skybox)
        {
            m_skybox = nullptr;
            return true;
        }

        m_logger.Warn("RemoveSkybox() - Could not remove since scene does not have a skybox");
        return false;
    }

    void SimpleScene::UnloadInternal()
    {
        if (m_skybox)
        {
            if (!m_skybox->Unload())
            {
                m_logger.Error("Unload() - Failed to unload skybox");
            }

            m_skybox->Destroy();
        }

        for (auto& mesh : m_meshes)
        {
            if (mesh.generation == INVALID_ID_U8) continue;

            if (!mesh.Unload())
            {
                m_logger.Error("Unload() - Failed to unload mesh");
            }
        }

        if (m_directionalLight)
        {
            Lights.RemoveDirectionalLight(m_directionalLight);
        }

        for (auto& light : m_pointLights)
        {
            Lights.RemovePointLight(light);
        }

        m_state = SceneState::Unloaded;

        m_pointLights.Destroy();
        m_meshes.Destroy();

        m_directionalLight = "";
        m_skybox           = nullptr;
        m_enabled          = false;

        m_state = SceneState::Uninitialized;
    }
}  // namespace C3D
