
#include "simple_scene.h"

#include "core/frame_data.h"
#include "math/frustum.h"
#include "math/ray.h"
#include "renderer/renderer_types.h"
#include "renderer/viewport.h"
#include "resources/debug/debug_box_3d.h"
#include "resources/materials/material.h"
#include "resources/mesh.h"
#include "resources/skybox.h"
#include "resources/textures/texture.h"
#include "systems/lights/light_system.h"
#include "systems/resources/resource_system.h"
#include "systems/system_manager.h"
#include "systems/textures/texture_system.h"

namespace C3D
{
    struct LightDebugData
    {
        DebugBox3D box;
    };

    struct GeometryDistance
    {
        /** @brief The geometry render data. */
        GeometryRenderData g;
        /** @brief The distance from the camera. */
        f32 distance;
    };

    static u32 global_scene_id = 0;

    SimpleScene::SimpleScene() : m_name("NO_NAME"), m_description("NO_DESCRIPTION") {}

    bool SimpleScene::Create() { return Create({}); }

    bool SimpleScene::Create(const SimpleSceneConfig& config)
    {
        m_enabled = false;
        m_state   = SceneState::Uninitialized;
        m_id      = global_scene_id++;

        m_skybox = nullptr;

        m_meshes.Create();
        m_terrains.Create();

        m_config = config;

        DebugGridConfig gridConfig;
        gridConfig.orientation   = C3D::DebugGridOrientation::XZ;
        gridConfig.tileCountDim0 = 100;
        gridConfig.tileCountDim1 = 100;
        gridConfig.tileScale     = 1.0f;
        gridConfig.name          = "DEBUG_GRID";
        gridConfig.useThirdAxis  = true;

        if (!m_grid.Create(gridConfig))
        {
            ERROR_LOG("Failed to create debug grid.");
            return false;
        }

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
            if (!m_skybox->Create(config))
            {
                ERROR_LOG("Failed to create skybox from config.");
                Memory.Delete(m_skybox);
                return false;
            }

            AddSkybox(m_config.skyboxConfig.name, m_skybox);
        }

        if (!m_config.directionalLightConfig.name.Empty())
        {
            auto& dirLightConfig = m_config.directionalLightConfig;

            auto dirLight = DirectionalLight();

            dirLight.name                       = dirLightConfig.name;
            dirLight.data.color                 = dirLightConfig.color;
            dirLight.data.direction             = dirLightConfig.direction;
            dirLight.data.shadowDistance        = dirLightConfig.shadowDistance;
            dirLight.data.shadowFadeDistance    = dirLightConfig.shadowFadeDistance;
            dirLight.data.shadowSplitMultiplier = dirLightConfig.shadowSplitMultiplier;

            m_directionalLight = m_config.directionalLightConfig.name;

            if (!Lights.AddDirectionalLight(dirLight))
            {
                ERROR_LOG("Failed to add directional light from config.");
                return false;
            }

            // TODO: Add debug data and initialize it here
        }

        if (!m_config.pointLights.Empty())
        {
            for (auto& config : m_config.pointLights)
            {
                auto light           = PointLight();
                light.name           = config.name;
                light.data.color     = config.color;
                light.data.position  = config.position;
                light.data.fConstant = config.constant;
                light.data.linear    = config.linear;
                light.data.quadratic = config.quadratic;
                AddPointLight(light);
            }
        }

        for (auto& meshConfig : m_config.meshes)
        {
            if (meshConfig.name.Empty() || meshConfig.resourceName.Empty())
            {
                WARN_LOG("Mesh with empty name or empty resource name provided. Skipping.");
                continue;
            }

            auto config           = MeshConfig();
            config.name           = meshConfig.name;
            config.resourceName   = meshConfig.resourceName;
            config.parentName     = meshConfig.parentName;
            config.enableDebugBox = true;  // Enable debug boxes around our meshes

            Mesh mesh;
            if (!mesh.Create(config))
            {
                ERROR_LOG("Failed to create Mesh: '{}'. Skipping.", meshConfig.name);
                continue;
            }
            mesh.transform = meshConfig.transform;
            m_meshes.Set(meshConfig.name, mesh);
        }

        for (auto& terrainConfig : m_config.terrains)
        {
            if (terrainConfig.name.Empty() || terrainConfig.resourceName.Empty())
            {
                WARN_LOG("Terrain with empty name or empty resource name provided. Skipping.");
                continue;
            }

            auto config         = TerrainConfig();
            config.name         = terrainConfig.name;
            config.resourceName = terrainConfig.resourceName;

            Terrain terrain;
            if (!terrain.Create(config))
            {
                ERROR_LOG("Failed to create Terrain: '{}'. Skipping.", terrainConfig.name);
                continue;
            }

            terrain.SetTransform(terrainConfig.transform);

            m_terrains.Set(terrainConfig.name, terrain);
        }

        if (!m_grid.Initialize())
        {
            ERROR_LOG("Failed to initialize Grid.");
            return false;
        }

        for (auto& name : m_pointLights)
        {
            auto light = Lights.GetPointLight(name);
            auto debug = static_cast<LightDebugData*>(light->debugData);
            if (!debug->box.Initialize())
            {
                ERROR_LOG("Failed to initialize debug box for point light: '{}'.", light->name);
                return false;
            }
        }

        // TODO: Handle directional light debug lines

        // Handle mesh hierarchy
        for (auto& mesh : m_meshes)
        {
            if (!mesh.config.parentName.Empty())
            {
                try
                {
                    auto& parent = GetMesh(mesh.config.parentName);
                    mesh.transform.SetParent(&parent.transform);
                }
                catch (const std::invalid_argument& exc)
                {
                    WARN_LOG("Mesh: '{}' was configured to have mesh named: '{}' as a parent. But the parent does not exist in this scene.",
                             mesh.config.name, mesh.config.parentName);
                }
            }
        }

        if (m_skybox)
        {
            if (!m_skybox->Initialize())
            {
                ERROR_LOG("Failed to initialize Skybox.");
                m_skybox = nullptr;
            }
        }

        for (auto& mesh : m_meshes)
        {
            if (!mesh.Initialize())
            {
                ERROR_LOG("Failed to initialize Mesh: '{}'.", mesh.GetName());
            }
        }

        for (auto& terrain : m_terrains)
        {
            if (!terrain.Initialize())
            {
                ERROR_LOG("Failed to inialize Terrain: '{}'.", terrain.GetName());
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
                ERROR_LOG("Failed to load skybox.");
                m_skybox = nullptr;
            }
        }

        for (auto& mesh : m_meshes)
        {
            if (!mesh.Load())
            {
                ERROR_LOG("Failed to load Mesh: '{}'.", mesh.GetName());
            }
        }

        for (auto& terrain : m_terrains)
        {
            if (!terrain.Load())
            {
                ERROR_LOG("Failed to load Terrain: '{}'.", terrain.GetName());
            }
        }

        if (!m_grid.Load())
        {
            ERROR_LOG("Failed to load grid.");
            return false;
        }

        for (auto& name : m_pointLights)
        {
            auto light = Lights.GetPointLight(name);
            auto debug = static_cast<LightDebugData*>(light->debugData);
            if (!debug->box.Load())
            {
                ERROR_LOG("Failed to load debug box for point light: '{}'.", name);
            }
        }

        m_state = SceneState::Loaded;
        return true;
    }

    bool SimpleScene::Save()
    {
        // Create a config based on the current objects in the scene
        SimpleSceneConfig config;

        if (!Resources.Write(config))
        {
            ERROR_LOG("Failed to write scene config to a file.");
            return false;
        }

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

        for (auto& name : m_pointLights)
        {
            auto light = Lights.GetPointLight(name);
            auto debug = static_cast<LightDebugData*>(light->debugData);

            if (debug->box.IsValid())
            {
                debug->box.SetPosition(light->data.position);

                // TODO: Other ways of doing this?
                debug->box.SetColor(light->data.color);
            }
        }

        return true;
    }

    void SimpleScene::OnPrepareRender(FrameData& frameData) const
    {
        for (auto& mesh : m_meshes)
        {
            if (mesh.HasDebugBox())
            {
                auto box = mesh.GetDebugBox();
                box->OnPrepareRender(frameData);
            }
        }

        for (auto& name : m_pointLights)
        {
            auto light = Lights.GetPointLight(name);
            auto debug = static_cast<LightDebugData*>(light->debugData);

            if (debug->box.IsValid())
            {
                debug->box.OnPrepareRender(frameData);
            }
        }
    }

    void SimpleScene::UpdateLodFromViewPosition(FrameData& frameData, const vec3& viewPosition, f32 nearClip, f32 farClip)
    {
        for (auto& terrain : m_terrains)
        {
            mat4 model = terrain.GetModel();

            // Calculate LOD splits based on clip range
            f32 range = farClip - nearClip;

            u32 numberOfLods = terrain.GetNumberOfLods();

            // The first split distance is always 0
            f32* splits = frameData.allocator->Allocate<f32>(MemoryType::Array, numberOfLods + 1);
            splits[0]   = 0.0f;
            for (u32 l = 0; l < numberOfLods; ++l)
            {
                f32 pct = (l + 1) / static_cast<f32>(numberOfLods);
                // Linear splits
                splits[l + 1] = (nearClip + range) * pct;
            }

            // Calculate chunk LODs based on distance from the camera
            for (auto& chunk : terrain.GetChunks())
            {
                // Translate/scale the center
                vec3 center = model * vec4(chunk.GetCenter(), 1.0f);

                // Check the distance from our view position to the chunk
                f32 distanceToChunk = glm::distance(viewPosition, center);
                u8 lod              = INVALID_ID_U8;
                for (u8 l = 0; l < numberOfLods; ++l)
                {
                    // If the distance to the chunk is between this and the next split we have found the right LOD
                    if (distanceToChunk >= splits[l] && distanceToChunk <= splits[l + 1])
                    {
                        lod = l;
                        break;
                    }
                }
                if (lod == INVALID_ID_U8)
                {
                    // In case we can't find a distance (for example chunks outside of our frustum)
                    // We simply use the lowest LOD
                    lod = numberOfLods - 1;
                }

                chunk.SetCurrentLOD(lod);
            }
        }
    }

    void SimpleScene::QueryMeshes(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                                  DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const
    {
        DynamicArray<GeometryDistance, LinearAllocator> transparentGeometries(32, frameData.allocator);

        for (const auto& mesh : m_meshes)
        {
            if (mesh.generation != INVALID_ID_U8)
            {
                mat4 model           = mesh.transform.GetWorld();
                bool windingInverted = mesh.transform.GetDeterminant() < 0;

                for (const auto geometry : mesh.geometries)
                {
                    // AABB Calculation
                    const vec3 extentsMax = model * vec4(geometry->extents.max, 1.0f);
                    const vec3 center     = model * vec4(geometry->center, 1.0f);

                    const vec3 halfExtents = {
                        Abs(extentsMax.x - center.x),
                        Abs(extentsMax.y - center.y),
                        Abs(extentsMax.z - center.z),
                    };

                    if (frustum.IntersectsWithAABB({ center, halfExtents }))
                    {
                        GeometryRenderData data(mesh.GetId(), model, geometry, windingInverted);

                        // Check if transparent. If so, put into a separate, temp array to be
                        // sorted by distance from the camera. Otherwise, we can just directly insert into the geometries dynamic array
                        if (Textures.HasTransparency(geometry->material->maps[0].texture))
                        {
                            // For meshes _with_ transparency, add them to a separate list to be sorted by distance later.
                            // Get the center, extract the global position from the model matrix and add it to the center,
                            // then calculate the distance between it and the camera, and finally save it to a list to be sorted.
                            // NOTE: This isn't perfect for translucent meshes that intersect, but is enough for our purposes now.
                            f32 distance = glm::distance(center, cameraPosition);

                            transparentGeometries.EmplaceBack(data, distance);
                        }
                        else
                        {
                            meshData.PushBack(data);
                        }
                    }
                }
            }
        }

        // Sort opaque geometries by material.
        std::sort(meshData.begin(), meshData.end(),
                  [](const GeometryRenderData& a, const GeometryRenderData& b) { return a.material->internalId < b.material->internalId; });

        // Sort transparent geometries
        std::sort(transparentGeometries.begin(), transparentGeometries.end(),
                  [](const GeometryDistance& a, const GeometryDistance& b) { return a.distance > b.distance; });

        // Then add them to the end of our meshData
        for (auto& tg : transparentGeometries)
        {
            meshData.PushBack(tg.g);
        }
    }

    void SimpleScene::QueryMeshes(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                                  DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const

    {
        DynamicArray<GeometryDistance, LinearAllocator> transparentGeometries(32, frameData.allocator);

        for (const auto& mesh : m_meshes)
        {
            if (mesh.generation != INVALID_ID_U8)
            {
                mat4 model           = mesh.transform.GetWorld();
                bool windingInverted = mesh.transform.GetDeterminant() < 0;

                for (const auto geometry : mesh.geometries)
                {
                    // Translate/scale the extents
                    const vec3 extentsMin = model * vec4(geometry->extents.min, 1.0f);
                    const vec3 extentsMax = model * vec4(geometry->extents.max, 1.0f);
                    // Translate/scale the center
                    const vec3 transformedCenter = model * vec4(geometry->center, 1.0f);
                    // Find the one furthest from the center
                    f32 meshRadius = Max(glm::distance(extentsMin, transformedCenter), glm::distance(extentsMax, transformedCenter));
                    // Distance to the line
                    f32 distToLine = DistancePointToLine(transformedCenter, center, direction);

                    // If it's within the distance we include it
                    if ((distToLine - meshRadius) <= radius)
                    {
                        GeometryRenderData data(mesh.GetId(), model, geometry, windingInverted);

                        // Check if transparent. If so, put into a separate, temp array to be
                        // sorted by distance from the camera. Otherwise, we can just directly insert into the geometries dynamic array
                        if (Textures.HasTransparency(geometry->material->maps[0].texture))
                        {
                            // For meshes _with_ transparency, add them to a separate list to be sorted by distance later.
                            // Get the center, extract the global position from the model matrix and add it to the center,
                            // then calculate the distance between it and the camera, and finally save it to a list to be sorted.
                            // NOTE: This isn't perfect for translucent meshes that intersect, but is enough for our purposes now.
                            f32 distance = glm::distance(transformedCenter, center);
                            transparentGeometries.EmplaceBack(data, distance);
                        }
                        else
                        {
                            meshData.PushBack(data);
                        }
                    }
                }
            }
        }

        // Sort opaque geometries by material.
        std::sort(meshData.begin(), meshData.end(), [](const GeometryRenderData& a, const GeometryRenderData& b) {
            if (!a.material || !b.material) return false;
            return a.material->internalId < b.material->internalId;
        });

        // Sort transparent geometries
        std::sort(transparentGeometries.begin(), transparentGeometries.end(),
                  [](const GeometryDistance& a, const GeometryDistance& b) { return a.distance > b.distance; });

        // Then add them to the end of our meshData
        for (auto& tg : transparentGeometries)
        {
            meshData.PushBack(tg.g);
        }
    }

    void SimpleScene::QueryTerrains(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                                    DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const
    {
        for (auto& terrain : m_terrains)
        {
            if (terrain.GetId())
            {
                mat4 model           = terrain.GetModel();
                bool windingInverted = terrain.GetTransform()->GetDeterminant() < 0;

                for (const auto& chunk : terrain.GetChunks())
                {
                    if (chunk.generation != INVALID_ID_U8)
                    {
                        // AABB Calculation
                        const auto& extents = chunk.GetExtents();

                        const vec3 extentsMax = model * vec4(extents.max, 1.0f);
                        const vec3 center     = model * vec4(chunk.GetCenter(), 1.0f);

                        const vec3 halfExtents = {
                            Abs(extentsMax.x - center.x),
                            Abs(extentsMax.y - center.y),
                            Abs(extentsMax.z - center.z),
                        };

                        if (frustum.IntersectsWithAABB({ center, halfExtents }))
                        {
                            GeometryRenderData data;
                            data.uuid            = terrain.GetId();
                            data.material        = terrain.GetMaterial();
                            data.windingInverted = windingInverted;
                            data.model           = model;

                            data.vertexCount        = chunk.GetVertexCount();
                            data.vertexSize         = chunk.GetVertexSize();
                            data.vertexBufferOffset = chunk.GetVertexBufferOffset();

                            data.indexCount        = chunk.GetIndexCount();
                            data.indexSize         = chunk.GetIndexSize();
                            data.indexBufferOffset = chunk.GetIndexBufferOffset();

                            terrainData.PushBack(data);
                        }
                    }
                }
            }
        }
    }

    void SimpleScene::QueryTerrains(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                                    DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const
    {
        for (auto& terrain : m_terrains)
        {
            if (terrain.GetId())
            {
                mat4 model           = terrain.GetModel();
                bool windingInverted = terrain.GetTransform()->GetDeterminant() < 0;

                for (const auto& chunk : terrain.GetChunks())
                {
                    if (chunk.generation != INVALID_ID_U8)
                    {
                        // Translate/scale the extents
                        const auto& extents = chunk.GetExtents();

                        const vec3 extentsMin = model * vec4(extents.min, 1.0f);
                        const vec3 extentsMax = model * vec4(extents.max, 1.0f);

                        // Translate/scale the center
                        const vec3 transformedCenter = model * vec4(chunk.GetCenter(), 1.0f);

                        // Find the one furthest from the center
                        f32 chunkRadius = Max(glm::distance(extentsMin, transformedCenter), glm::distance(extentsMax, transformedCenter));

                        // Distance to the line
                        f32 distToLine = DistancePointToLine(transformedCenter, center, direction);

                        // If it's within the distance we include it
                        if ((distToLine - chunkRadius) <= radius)
                        {
                            GeometryRenderData data;
                            data.uuid            = terrain.GetId();
                            data.material        = terrain.GetMaterial();
                            data.windingInverted = windingInverted;
                            data.model           = model;

                            data.vertexCount        = chunk.GetVertexCount();
                            data.vertexSize         = chunk.GetVertexSize();
                            data.vertexBufferOffset = chunk.GetVertexBufferOffset();

                            data.indexCount        = chunk.GetIndexCount();
                            data.indexSize         = chunk.GetIndexSize();
                            data.indexBufferOffset = chunk.GetIndexBufferOffset();

                            terrainData.PushBack(data);
                        }
                    }
                }
            }
        }
    }

    void SimpleScene::QueryMeshes(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const
    {
        C3D::DynamicArray<GeometryDistance, LinearAllocator> transparentGeometries(32, frameData.allocator);

        for (const auto& mesh : m_meshes)
        {
            if (mesh.generation != INVALID_ID_U8)
            {
                mat4 model           = mesh.transform.GetWorld();
                bool windingInverted = mesh.transform.GetDeterminant() < 0;

                for (const auto geometry : mesh.geometries)
                {
                    meshData.EmplaceBack(mesh.GetId(), model, geometry, windingInverted);
                }
            }
        }

        // Sort opaque geometries by material.
        std::sort(meshData.begin(), meshData.end(),
                  [](const GeometryRenderData& a, const GeometryRenderData& b) { return a.material->internalId < b.material->internalId; });

        // Sort transparent geometries
        std::sort(transparentGeometries.begin(), transparentGeometries.end(),
                  [](const GeometryDistance& a, const GeometryDistance& b) { return a.distance > b.distance; });

        // Then add them to the end of our meshData
        for (auto& tg : transparentGeometries)
        {
            meshData.PushBack(tg.g);
        }
    }

    void SimpleScene::QueryTerrains(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const
    {
        for (auto& terrain : m_terrains)
        {
            if (terrain.GetId())
            {
                mat4 model           = terrain.GetModel();
                bool windingInverted = terrain.GetTransform()->GetDeterminant() < 0;

                for (auto& chunk : terrain.GetChunks())
                {
                    GeometryRenderData data;
                    data.uuid            = terrain.GetId();
                    data.material        = terrain.GetMaterial();
                    data.windingInverted = windingInverted;
                    data.model           = model;

                    data.vertexCount        = chunk.GetVertexCount();
                    data.vertexSize         = chunk.GetVertexSize();
                    data.vertexBufferOffset = chunk.GetVertexBufferOffset();

                    data.indexCount        = chunk.GetIndexCount();
                    data.indexSize         = chunk.GetIndexSize();
                    data.indexBufferOffset = chunk.GetIndexBufferOffset();

                    terrainData.PushBack(data);
                }
            }
        }
    }

    void SimpleScene::QueryDebugGeometry(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& debugData) const
    {
        // Grid
        constexpr auto identity = mat4(1.0f);
        auto gridGeometry       = m_grid.GetGeometry();
        if (gridGeometry->generation != INVALID_ID_U16)
        {
            debugData.EmplaceBack(m_grid.GetId(), identity, gridGeometry);
        }

        // TODO: Directional lights

        // Point Lights
        for (auto& name : m_pointLights)
        {
            auto light = Lights.GetPointLight(name);
            auto debug = static_cast<LightDebugData*>(light->debugData);

            if (debug)
            {
                debugData.EmplaceBack(debug->box.GetId(), debug->box.GetModel(), debug->box.GetGeometry());
            }
        }

        for (const auto& mesh : m_meshes)
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
                        debugData.EmplaceBack(box->GetId(), box->GetModel(), box->GetGeometry());
                    }
                }
            }
        }
    }

    bool SimpleScene::AddDirectionalLight(const String& name, DirectionalLight& light)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (!m_directionalLight.Empty())
        {
            // TODO: Do resource unloading when required
            if (!Lights.RemoveDirectionalLight(m_directionalLight))
            {
                ERROR_LOG("Failed to remove current directional light.");
                return false;
            }
            if (light.debugData)
            {
                // TODO: release debug data
            }
        }

        m_directionalLight = name;

        // TODO: Add debug info for directional lights
        return Lights.AddDirectionalLight(light);
    }

    bool SimpleScene::RemoveDirectionalLight(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (m_directionalLight)
        {
            // TODO: Cleanup debug data

            const auto result  = Lights.RemoveDirectionalLight(m_directionalLight);
            m_directionalLight = "";
            return result;
        }

        WARN_LOG("Could not remove since provided light is not part of this scene.");
        return false;
    }

    bool SimpleScene::AddPointLight(const PointLight& light)
    {
        if (!Lights.AddPointLight(light))
        {
            ERROR_LOG("Failed to add point light to lighting system.");
            return false;
        }

        auto pLight       = Lights.GetPointLight(light.name);
        pLight->debugData = Memory.New<LightDebugData>(MemoryType::Resource);
        auto debug        = static_cast<LightDebugData*>(pLight->debugData);

        if (!debug->box.Create(vec3(0.2f, 0.2f, 0.2f), nullptr))
        {
            ERROR_LOG("Failed to add debug box to point light: '{}'.", light.name);
            return false;
        }

        debug->box.SetPosition(light.data.position);

        if (m_state >= SceneState::Initialized)
        {
            if (!debug->box.Initialize())
            {
                ERROR_LOG("Failed to initialize debug box for point light: '{}'.", light.name);
                return false;
            }
        }

        if (m_state >= SceneState::Loaded)
        {
            if (!debug->box.Load())
            {
                ERROR_LOG("Failed to load debug box for point light: '{}'.", light.name);
                return false;
            }
        }

        m_pointLights.PushBack(light.name);
        return true;
    }

    bool SimpleScene::RemovePointLight(const String& name)
    {
        if (m_pointLights.Contains(name))
        {
            auto pLight = Lights.GetPointLight(name);
            auto debug  = static_cast<LightDebugData*>(pLight->debugData);
            debug->box.Unload();
            debug->box.Destroy();
            Memory.Delete(debug);
        }

        auto result = Lights.RemovePointLight(name);
        if (result)
        {
            m_pointLights.Remove(name);
            return true;
        }

        ERROR_LOG("Failed to remove Point Light.");
        return false;
    }

    PointLight* SimpleScene::GetPointLight(const String& name) { return Lights.GetPointLight(name); }

    bool SimpleScene::AddMesh(const String& name, Mesh& mesh)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (m_meshes.Has(name))
        {
            ERROR_LOG("A mesh with the name '{}' already exists.", name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the mesh now
            if (!mesh.Initialize())
            {
                ERROR_LOG("Failed to initialize mesh: '{}'.", name);
                return false;
            }
        }

        if (m_state >= SceneState::Loading)
        {
            // The scene has already been loaded (or is loading currently) so we need to load the mesh now
            if (!mesh.Load())
            {
                ERROR_LOG("Failed to load mesh: '{}'.", name);
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
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (!m_meshes.Has(name))
        {
            ERROR_LOG("Unknown name provided.");
            return false;
        }

        auto& mesh = m_meshes.Get(name);
        if (!mesh.Unload())
        {
            ERROR_LOG("Failed to unload mesh.");
            return false;
        }

        m_meshes.Delete(name);
        return true;
    }

    Mesh& SimpleScene::GetMesh(const String& name) { return m_meshes.Get(name); }

    bool SimpleScene::AddTerrain(const String& name, Terrain& terrain)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (m_terrains.Has(name))
        {
            ERROR_LOG("A terrain with the name: '{}' already exists.", name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the terrain now
            if (!terrain.Initialize())
            {
                ERROR_LOG("Failed to initialize terrain: '{}'.", name);
                return false;
            }
        }

        if (m_state >= SceneState::Loading)
        {
            // The scene has already been loaded (or is loading currently) so we need to load the terrain now
            if (!terrain.Load())
            {
                ERROR_LOG("Failed to load terrain: '{}'.", name);
                return false;
            }
        }

        m_terrains.Set(name, terrain);
        return true;
    }

    bool SimpleScene::RemoveTerrain(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (!m_terrains.Has(name))
        {
            ERROR_LOG("Unknown name provided: '{}'.", name);
            return false;
        }

        auto& terrain = m_terrains.Get(name);
        if (!terrain.Unload())
        {
            ERROR_LOG("Failed to unload terrain: '{}'.", name);
            return false;
        }

        m_terrains.Delete(name);
        return true;
    }

    Terrain& SimpleScene::GetTerrain(const String& name) { return m_terrains.Get(name); }

    bool SimpleScene::AddSkybox(const String& name, Skybox* skybox)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (!skybox)
        {
            ERROR_LOG("Invalid skybox provided.");
            return false;
        }

        // TODO: If one already exists, what do we do?
        m_skybox = skybox;

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the skybox now
            if (!m_skybox->Initialize())
            {
                ERROR_LOG("Failed to initialize Skybox.");
                m_skybox = nullptr;
                return false;
            }
        }

        if (m_state == SceneState::Loading || m_state == SceneState::Loaded)
        {
            if (!m_skybox->Load())
            {
                ERROR_LOG("Failed to load Skybox.");
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
            ERROR_LOG("Empty name provided.");
            return false;
        }

        if (m_skybox)
        {
            if (!m_skybox->Unload())
            {
                ERROR_LOG("Failed to unload Skybox.");
            }

            m_skybox->Destroy();

            Memory.Delete(m_skybox);
            m_skybox = nullptr;
            return true;
        }

        WARN_LOG("Could not remove since scene does not have a Skybox.");
        return false;
    }

    bool SimpleScene::RayCast(const Ray& ray, RayCastResult& result)
    {
        if (m_state < SceneState::Loaded)
        {
            return false;
        }

        // TODO: Optimize to not check every mesh (with Spatial partitioning)
        // to ensure we remain performant with many meshes
        for (auto& mesh : m_meshes)
        {
            f32 distance;

            if (ray.TestAgainstExtents(mesh.GetExtents(), mesh.transform.GetWorld(), distance))
            {
                // We have a hit
                vec3 position = ray.origin + (ray.direction * distance);
                result.hits.EmplaceBack(RAY_CAST_HIT_TYPE_OBB, mesh.GetId(), position, distance);
            }
        }

        return !result.hits.Empty();
    }

    Transform* SimpleScene::GetTransformById(UUID id)
    {
        for (auto& mesh : m_meshes)
        {
            if (mesh.GetId() == id)
            {
                return &mesh.transform;
            }
        }

        for (auto& terrain : m_terrains)
        {
            if (terrain.GetId() == id)
            {
                return terrain.GetTransform();
            }
        }

        return nullptr;
    }

    void SimpleScene::UnloadInternal()
    {
        if (m_skybox)
        {
            RemoveSkybox("SKYBOX");
        }

        for (auto& mesh : m_meshes)
        {
            if (mesh.generation == INVALID_ID_U8) continue;

            if (!mesh.Unload())
            {
                ERROR_LOG("Failed to unload Mesh: '{}'.", mesh.GetName());
            }

            mesh.Destroy();
        }

        for (auto& terrain : m_terrains)
        {
            if (!terrain.Unload())
            {
                ERROR_LOG("Failed to unload Terrain: '{}'.", terrain.GetName());
            }

            terrain.Destroy();
        }

        if (!m_grid.Unload())
        {
            ERROR_LOG("Failed to unload Grid.");
        }

        if (m_directionalLight)
        {
            // TODO: Cleanup debug data once we add it

            Lights.RemoveDirectionalLight(m_directionalLight);
        }

        for (auto& name : m_pointLights)
        {
            auto pLight = Lights.GetPointLight(name);
            auto debug  = static_cast<LightDebugData*>(pLight->debugData);
            debug->box.Unload();
            debug->box.Destroy();
            Memory.Delete(debug);
            Lights.RemovePointLight(name);
        }

        m_state = SceneState::Unloaded;

        m_pointLights.Destroy();
        m_meshes.Destroy();
        m_terrains.Destroy();

        m_directionalLight.Destroy();
        m_skybox  = nullptr;
        m_enabled = false;

        m_state = SceneState::Uninitialized;
    }
}  // namespace C3D