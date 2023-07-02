
#include "simple_scene.h"

#include "core/frame_data.h"
#include "renderer/renderer_types.h"
#include "resources/mesh.h"
#include "systems/render_views/render_view_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    static u32 global_scene_id = 0;

    SimpleScene::SimpleScene() : m_logger("SIMPLE_SCENE") {}

    bool SimpleScene::Create(const SystemManager* pSystemsManager)
    {
        m_pSystemsManager = pSystemsManager;

        m_enabled = false;
        m_state = SceneState::Uninitialized;
        m_id = global_scene_id++;

        m_skybox = nullptr;
        m_directionalLight = nullptr;

        // TODO: Process config

        return true;
    }

    bool SimpleScene::Initialize()
    {
        // TODO: Process configuration and setup hierarchy

        if (m_skybox)
        {
            if (!m_skybox->Initialize())
            {
                m_logger.Error("Initialize() - Failed to initialize skybox");
                m_skybox = nullptr;
                return false;
            }
        }

        for (auto mesh : m_meshes)
        {
            if (!mesh->Initialize())
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

        for (auto mesh : m_meshes)
        {
            if (!mesh->Load())
            {
                m_logger.Error("Initialize() - Failed to initialize mesh");
                return false;
            }
        }

        m_state = SceneState::Loaded;
        return true;
    }

    bool SimpleScene::Unload() { return true; }

    bool SimpleScene::Update(FrameData& frameData) { return true; }

    bool SimpleScene::PopulateRenderPacket(FrameData& frameData, RenderPacket& packet)
    {
        // TODO: Cache this somewhere so we don't check every time
        for (auto& viewPacket : packet.views)
        {
            const auto view = viewPacket.view;
            if (view->type == RenderViewKnownType::Skybox)
            {
                SkyboxPacketData skyboxData = {m_skybox};
                if (!Views.BuildPacket(view, frameData.frameAllocator, &skyboxData, &viewPacket))
                {
                    m_logger.Error("PopulateRenderPacket() - Failed to populate render packet with skybox data");
                    return false;
                }
                break;
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

                for (const auto mesh : m_meshes)
                {
                    // auto meshSet = false;

                    if (mesh->generation != INVALID_ID_U8)
                    {
                        mat4 model = mesh->transform.GetWorld();

                        for (const auto geometry : mesh->geometries)
                        {
                            m_worldGeometries.EmplaceBack(model, geometry, mesh->uniqueId);
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

    bool SimpleScene::AddDirectionalLight(DirectionalLight* light) { return true; }

    bool SimpleScene::RemoveDirectionalLight(DirectionalLight* light) { return true; }

    bool SimpleScene::AddPointLight(PointLight* light) { return true; }

    bool SimpleScene::RemovePointLight(PointLight* light) { return true; }

    bool SimpleScene::AddMesh(Mesh* mesh)
    {
        if (!mesh)
        {
            m_logger.Error("AddMesh() - Invalid mesh provided");
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the skybox now
            if (!mesh->Initialize())
            {
                m_logger.Error("AddMesh() - Failed to initialize mesh");
                return false;
            }
        }

        if (m_state >= SceneState::Loading)
        {
            if (!mesh->Load())
            {
                m_logger.Error("AddMesh() - Failed to load mesh");
                return false;
            }
        }

        m_meshes.PushBack(mesh);
        return true;
    }

    bool SimpleScene::RemoveMesh(Mesh* mesh) { return true; }

    bool SimpleScene::AddSkybox(Skybox* skybox)
    {
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

    bool SimpleScene::RemoveSkybox(Skybox* skybox) { return true; }

    bool SimpleScene::Destroy()
    {
        if (m_state == SceneState::Loaded)
        {
            if (!Unload())
            {
                m_logger.Error("Destroy() - Failed to unload the scene before destroying");
                return false;
            }
        }

        m_pointLights.Destroy();
        m_meshes.Destroy();
        m_directionalLight = nullptr;
        m_skybox = nullptr;

        m_state = SceneState::Uninitialized;
        return true;
    }
}  // namespace C3D
