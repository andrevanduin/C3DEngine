
#include "scene.h"

#include "frame_data.h"
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
    static u32 SCENE_OBJECT_ID = 0;
    static u32 METADATA_ID     = 0;

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

    Scene::Scene() : m_name("NO_NAME"), m_description("NO_DESCRIPTION") {}

    bool Scene::Create() { return Create({}); }

    bool Scene::Create(const SceneConfig& config)
    {
        m_enabled = false;
        m_state   = SceneState::Uninitialized;
        m_id      = global_scene_id++;

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

    bool Scene::Initialize()
    {
        m_state = SceneState::Initialized;

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
            AddSkybox(m_config.skyboxConfig);
        }

        if (!m_config.directionalLightConfig.name.Empty())
        {
            AddDirectionalLight(m_config.directionalLightConfig);

            // TODO: Add debug data and initialize it here
        }

        if (!m_config.pointLights.Empty())
        {
            for (auto& config : m_config.pointLights)
            {
                AddPointLight(config);
            }
        }

        for (auto& meshConfig : m_config.meshes)
        {
            AddMesh(meshConfig);
        }

        for (auto& terrainConfig : m_config.terrains)
        {
            AddTerrain(terrainConfig);
        }

        if (!m_grid.Initialize())
        {
            ERROR_LOG("Failed to initialize Grid.");
        }

        // TODO: Handle directional light debug lines

        // Handle mesh hierarchy
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Mesh) continue;

            auto& child = m_meshes[object.resourceIndex];
            if (!child.config.parentName.Empty())
            {
                // We have a parent
                auto& parent = GetParent(child.config.parentName);
                // Make the object a child of the parent
                m_graph.AddChild(parent.node, object.node);
            }
        }

        return true;
    }

    bool Scene::Load()
    {
        m_state = SceneState::Loading;

        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            switch (object.type)
            {
                case SceneObjectType::Skybox:
                {
                    auto& skybox = m_skyboxes[object.resourceIndex];
                    if (skybox.instanceId == INVALID_ID)
                    {
                        // Skybox is not loaded yet
                        if (!skybox.Load())
                        {
                            ERROR_LOG("Failed to load skybox.");
                        }
                    }
                }
                break;
                case SceneObjectType::Mesh:
                {
                    auto& mesh = m_meshes[object.resourceIndex];
                    if (!mesh.Load())
                    {
                        ERROR_LOG("Failed to load Mesh: '{}'.", mesh.GetName());
                    }
                }
                break;
                case SceneObjectType::Terrain:
                {
                    auto& terrain = m_terrains[object.resourceIndex];
                    if (!terrain.Load())
                    {
                        ERROR_LOG("Failed to load Terrain: '{}'.", terrain.GetName());
                    }
                }
                break;
                case SceneObjectType::PointLight:
                {
                    const auto& light = m_pointLights[object.resourceIndex];
                    auto debug        = static_cast<LightDebugData*>(light.debugData);
                    if (debug && !debug->box.Load())
                    {
                        auto& metadata = m_metadatas[object.metadataIndex];
                        ERROR_LOG("Failed to load debug box for Point Light: '{}'.", metadata.name);
                    }
                }
                break;
            }
        }

        if (!m_grid.Load())
        {
            ERROR_LOG("Failed to load grid.");
            return false;
        }

        m_state = SceneState::Loaded;
        return true;
    }

    bool Scene::Save()
    {
        // Create a config based on the current objects in the scene
        /*
        SceneConfig config;
        config.name        = m_name;
        config.description = m_description;

        if (m_skybox)
        {
            config.skyboxConfig = m_config.skyboxConfig;
        }

        if (!m_directionalLight.Empty())
        {
            auto dirLight = Lights.GetDirectionalLight();

            config.directionalLightConfig.name                  = m_directionalLight;
            config.directionalLightConfig.color                 = dirLight->data.color;
            config.directionalLightConfig.direction             = dirLight->data.direction;
            config.directionalLightConfig.shadowDistance        = dirLight->data.shadowDistance;
            config.directionalLightConfig.shadowFadeDistance    = dirLight->data.shadowFadeDistance;
            config.directionalLightConfig.shadowSplitMultiplier = dirLight->data.shadowSplitMultiplier;
        }

        for (auto& name : m_pointLights)
        {
            auto light = Lights.GetPointLight(name);

            ScenePointLightConfig lightConfig;
            lightConfig.name      = name;
            lightConfig.color     = light->data.color;
            lightConfig.position  = light->data.position;
            lightConfig.constant  = light->data.fConstant;
            lightConfig.linear    = light->data.linear;
            lightConfig.quadratic = light->data.quadratic;

            config.pointLights.PushBack(lightConfig);
        }

        for (auto& meshCfg : m_config.meshes)
        {
            SceneMeshConfig meshConfig;

            meshConfig.name = meshCfg.name;
            // TODO: Parent could have changed so this is wrong...
            meshConfig.parentName   = meshCfg.parentName;
            meshConfig.resourceName = meshCfg.resourceName;
            meshConfig.transform    = m_meshes[meshCfg.name].transform;

            config.meshes.PushBack(meshConfig);
        }

        for (auto& terrainCfg : m_config.terrains)
        {
            SceneTerrainConfig terrainConfig;
            terrainConfig.name         = terrainCfg.name;
            terrainConfig.resourceName = terrainCfg.resourceName;
            terrainConfig.transform    = m_terrains[terrainCfg.name].GetTransform();

            config.terrains.PushBack(terrainConfig);
        }

        if (!Resources.Write(config))
        {
            ERROR_LOG("Failed to write scene config to a file.");
            return false;
        }
*/
        return true;
    }

    bool Scene::Unload(bool immediate /* = false*/)
    {
        m_state = SceneState::Unloading;
        if (immediate)
        {
            UnloadInternal();
        }
        return true;
    }

    bool Scene::Update(FrameData& frameData)
    {
        if (m_state == SceneState::Unloading)
        {
            UnloadInternal();
            return true;
        }

        if (m_state != SceneState::Loaded) return true;

        if (!m_graph.Update())
        {
            ERROR_LOG("Failed to update Scene Graph.");
            return false;
        }

        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            switch (object.type)
            {
                case SceneObjectType::PointLight:
                    const auto& light = m_pointLights[object.resourceIndex];
                    auto debug        = static_cast<LightDebugData*>(light.debugData);
                    if (debug && debug->box.IsValid())
                    {
                        debug->box.SetPosition(light.data.position);

                        // TODO: Other ways of doing this?
                        debug->box.SetColor(light.data.color);
                    }
                    break;
            }
        }

        return true;
    }

    void Scene::OnPrepareRender(FrameData& frameData) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            switch (object.type)
            {
                case SceneObjectType::Mesh:
                {
                    const auto& mesh = m_meshes[object.resourceIndex];
                    if (mesh.HasDebugBox())
                    {
                        auto box = mesh.GetDebugBox();
                        box->OnPrepareRender(frameData);
                    }
                }

                break;
                case SceneObjectType::PointLight:
                {
                    const auto& light = m_pointLights[object.resourceIndex];
                    auto debug        = static_cast<LightDebugData*>(light.debugData);
                    if (debug && debug->box.IsValid())
                    {
                        debug->box.OnPrepareRender(frameData);
                    }
                }
                break;
            }
        }
    }

    void Scene::UpdateLodFromViewPosition(FrameData& frameData, const vec3& viewPosition, f32 nearClip, f32 farClip)
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Terrain) continue;

            auto& terrain = m_terrains[object.resourceIndex];
            if (!terrain.GetId()) continue;

            auto transform = m_graph.GetTransform(object.node);
            mat4 model     = Transforms.GetWorld(transform);

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

    void Scene::QueryMeshes(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                            DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const
    {
        DynamicArray<GeometryDistance, LinearAllocator> transparentGeometries(32, frameData.allocator);

        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Mesh) continue;

            const auto& mesh = m_meshes[object.resourceIndex];
            if (mesh.generation != INVALID_ID_U8)
            {
                auto transform       = m_graph.GetTransform(object.node);
                mat4 model           = Transforms.GetWorld(transform);
                bool windingInverted = Transforms.GetDeterminant(transform) < 0;

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

    void Scene::QueryMeshes(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                            DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const

    {
        DynamicArray<GeometryDistance, LinearAllocator> transparentGeometries(32, frameData.allocator);

        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type == SceneObjectType::Mesh)
            {
                const auto& mesh = m_meshes[object.resourceIndex];
                if (mesh.generation != INVALID_ID_U8)
                {
                    auto transform       = m_graph.GetTransform(object.node);
                    mat4 model           = Transforms.GetWorld(transform);
                    bool windingInverted = Transforms.GetDeterminant(transform) < 0;

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
                            // sorted by distance from the camera. Otherwise, we can just directly insert into the geometries dynamic
                            // array
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

    void Scene::QueryTerrains(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                              DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Terrain) continue;

            auto& terrain = m_terrains[object.resourceIndex];
            if (terrain.GetId())
            {
                auto transform       = m_graph.GetTransform(object.node);
                mat4 model           = Transforms.GetWorld(transform);
                bool windingInverted = Transforms.GetDeterminant(transform) < 0;

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

    void Scene::QueryTerrains(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                              DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Terrain) continue;

            auto& terrain = m_terrains[object.resourceIndex];
            if (terrain.GetId())
            {
                auto transform       = m_graph.GetTransform(object.node);
                mat4 model           = Transforms.GetWorld(transform);
                bool windingInverted = Transforms.GetDeterminant(transform) < 0;

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

    void Scene::QueryMeshes(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const
    {
        C3D::DynamicArray<GeometryDistance, LinearAllocator> transparentGeometries(32, frameData.allocator);

        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type == SceneObjectType::Mesh)
            {
                const auto& mesh = m_meshes[object.resourceIndex];
                if (mesh.generation != INVALID_ID_U8)
                {
                    auto transform       = m_graph.GetTransform(object.node);
                    mat4 model           = Transforms.GetWorld(transform);
                    bool windingInverted = Transforms.GetDeterminant(transform) < 0;

                    for (const auto geometry : mesh.geometries)
                    {
                        meshData.EmplaceBack(mesh.GetId(), model, geometry, windingInverted);
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

    void Scene::QueryTerrains(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Terrain) continue;

            auto& terrain = m_terrains[object.resourceIndex];
            if (terrain.GetId())
            {
                auto transform       = m_graph.GetTransform(object.node);
                mat4 model           = Transforms.GetWorld(transform);
                bool windingInverted = Transforms.GetDeterminant(transform) < 0;

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

    void Scene::QueryDebugGeometry(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& debugData) const
    {
        // Grid
        constexpr auto identity = mat4(1.0f);
        auto gridGeometry       = m_grid.GetGeometry();
        if (gridGeometry->generation != INVALID_ID_U16)
        {
            debugData.EmplaceBack(m_grid.GetId(), identity, gridGeometry);
        }

        // TODO: Directional lights

        for (const auto& object : m_objects)
        {
            if (object.type == SceneObjectType::PointLight)
            {
                auto& light = m_pointLights[object.resourceIndex];
                if (light.debugData)
                {
                    auto debug = static_cast<LightDebugData*>(light.debugData);
                    debugData.EmplaceBack(debug->box.GetId(), debug->box.GetModel(), debug->box.GetGeometry());
                }
            }
            else if (object.type == SceneObjectType::Mesh)
            {
                auto& mesh = m_meshes[object.resourceIndex];
                if (mesh.generation != INVALID_ID_U8)
                {
                    auto transform = m_graph.GetTransform(object.node);

                    mat4 model           = Transforms.GetWorld(transform);
                    bool windingInverted = Transforms.GetDeterminant(transform) < 0;

                    if (mesh.HasDebugBox())
                    {
                        const auto box = mesh.GetDebugBox();
                        if (box->IsValid())
                        {
                            debugData.EmplaceBack(box->GetId(), model, box->GetGeometry());
                        }
                    }
                }
            }
        }
    }

    void Scene::QueryDirectionalLights(FrameData& frameData, DynamicArray<DirectionalLightData, LinearAllocator>& lightData) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::DirectionalLight) continue;

            auto& light = m_directionalLights[object.resourceIndex];
            lightData.PushBack(light.data);
        }
    }

    void Scene::QueryPointLights(FrameData& frameData, DynamicArray<PointLightData, LinearAllocator>& lightData) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::PointLight) continue;

            auto& light = m_pointLights[object.resourceIndex];
            lightData.PushBack(light.data);
        }
    }

    Skybox& Scene::GetSkybox() { return m_skyboxes[0]; }

    PointLight* Scene::GetPointLight(const String& name)
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::PointLight) continue;

            auto& metadata = m_metadatas[object.metadataIndex];
            if (metadata.name == name)
            {
                return &m_pointLights[object.resourceIndex];
            }
        }

        ERROR_LOG("Failed to get Point Light named: '{}'.", name);
        return nullptr;
    }

    const SceneObject& Scene::GetParent(const String& name) const
    {
        for (const auto& object : m_objects)
        {
            if (object.id == INVALID_ID) continue;
            if (object.type != SceneObjectType::Mesh) continue;

            auto& metadata = m_metadatas[object.metadataIndex];
            if (metadata.name == name)
            {
                return object;
            }
        }

        FATAL_LOG("Failed to find Mesh named: '{}'.");
        return m_objects[0];
    }

    bool Scene::AddSkybox(const SceneSkyboxConfig& config)
    {
        if (config.name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        // Add a SceneObject
        SceneObject object;
        object.id = SCENE_OBJECT_ID++;
        // Store that this object is a Skybox
        object.type = SceneObjectType::Skybox;

        // Add some metadata to our skybox
        SceneMetadata metadata;
        metadata.id           = METADATA_ID++;
        metadata.name         = config.name;
        metadata.resourceName = config.cubemapName;

        // Add the skybox resource
        SkyboxConfig c;
        c.name        = config.name;
        c.cubemapName = config.cubemapName;

        Skybox skybox;
        if (!skybox.Create(c))
        {
            ERROR_LOG("Failed to Create Skybox: '{}'.", c.name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the skybox now
            if (!skybox.Initialize())
            {
                ERROR_LOG("Failed to initialize Skybox: '{}'.", c.name);
                return false;
            }
        }

        if (m_state == SceneState::Loading || m_state == SceneState::Loaded)
        {
            if (!skybox.Load())
            {
                ERROR_LOG("Failed to load Skybox: '{}'.", c.name);
                return false;
            }
        }

        // Add the resource to our array
        object.resourceIndex = m_skyboxes.Size();
        m_skyboxes.PushBack(skybox);

        // Add the metadata to our array
        object.metadataIndex = m_metadatas.Size();
        m_metadatas.PushBack(metadata);

        // Finally store the object in our scene
        m_objects.PushBack(object);

        return true;
    }

    bool Scene::RemoveSkybox(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        for (auto& object : m_objects)
        {
            // Ignore invalid objects
            if (!object.id == INVALID_ID) continue;
            // Ignore objects that are not of Skybox type
            if (object.type != SceneObjectType::Skybox) continue;
            // Get the metadata for this object
            auto& metadata = m_metadatas[object.resourceIndex];
            // Check the name against the provided one
            if (metadata.name == name)
            {
                // We have found our Skybox
                auto& skybox = m_skyboxes[object.metadataIndex];
                // Unload first
                if (!skybox.Unload())
                {
                    ERROR_LOG("Failed to unload Skybox: '{}'.", name);
                    return false;
                }
                // Then destroy
                skybox.Destroy();

                // Finally also destroy the scene object
                object.Destroy();
                return true;
            }
        }

        ERROR_LOG("Failed to remove Skybox: '{}' from the scene.", name);
        return false;
    }

    bool Scene::AddDirectionalLight(const SceneDirectionalLightConfig& config)
    {
        if (config.name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        // Add a SceneObject
        SceneObject object;
        object.id = SCENE_OBJECT_ID++;
        // Store that this object is a Directional Light
        object.type = SceneObjectType::DirectionalLight;

        // Add some metadata to our Directional Light
        SceneMetadata metadata;
        metadata.id   = METADATA_ID++;
        metadata.name = config.name;

        // Add the Directional Light resource
        DirectionalLight light;
        light.name                       = config.name;
        light.data.color                 = config.color;
        light.data.direction             = config.direction;
        light.data.shadowDistance        = config.shadowDistance;
        light.data.shadowFadeDistance    = config.shadowFadeDistance;
        light.data.shadowSplitMultiplier = config.shadowSplitMultiplier;

        // Add the resource to our array
        object.resourceIndex = m_directionalLights.Size();
        m_directionalLights.PushBack(light);

        // Add the metadata to our array at the same index as our resource
        m_metadatas.PushBack(metadata);

        // Finally store the object in our scene
        m_objects.PushBack(object);

        // TODO: Add debug info for directional lights
        return true;
    }

    bool Scene::RemoveDirectionalLight(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        for (auto& object : m_objects)
        {
            // Ignore invalid objects
            if (!object.id == INVALID_ID) continue;
            // Ignore objects that are not of Directional Light type
            if (object.type != SceneObjectType::DirectionalLight) continue;
            // Get the metadata for this object
            auto& metadata = m_metadatas[object.metadataIndex];
            // Check the name against the provided one
            if (metadata.name == name)
            {
                // We have found our directional light
                // Destroy the object
                object.Destroy();
                return true;
            }
        }

        ERROR_LOG("Failed to remove Directional Light: '{}' from the scene.", name);
        return false;
    }

    bool Scene::AddPointLight(const ScenePointLightConfig& config)
    {
        if (config.name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        // Add a SceneObject
        SceneObject object;
        object.id = SCENE_OBJECT_ID++;
        // Store that this object is a Point Light
        object.type = SceneObjectType::PointLight;

        // Add some metadata to our Point Light
        SceneMetadata metadata;
        metadata.id   = METADATA_ID++;
        metadata.name = config.name;

        // Add the Point Light resource
        PointLight light     = PointLight();
        light.name           = config.name;
        light.data.color     = config.color;
        light.data.position  = config.position;
        light.data.fConstant = config.constant;
        light.data.linear    = config.linear;
        light.data.quadratic = config.quadratic;

        // Add a debug box
        light.debugData = Memory.New<LightDebugData>(MemoryType::Resource);
        auto debug      = static_cast<LightDebugData*>(light.debugData);

        if (!debug->box.Create(vec3(0.2f, 0.2f, 0.2f)))
        {
            ERROR_LOG("Failed to add debug box to Point Light: '{}'.", light.name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            if (!debug->box.Initialize())
            {
                ERROR_LOG("Failed to initialize debug box for Point Light: '{}'.", light.name);
                return false;
            }
        }

        if (m_state >= SceneState::Loaded)
        {
            if (!debug->box.Load())
            {
                ERROR_LOG("Failed to load debug box for Point Light: '{}'.", light.name);
                return false;
            }
        }

        // Add the resource to our array
        object.resourceIndex = m_pointLights.Size();
        m_pointLights.PushBack(light);

        // Add the metadata to our array
        object.metadataIndex = m_metadatas.Size();
        m_metadatas.PushBack(metadata);

        // Finally store the object in our scene
        m_objects.PushBack(object);

        return true;
    }

    bool Scene::RemovePointLight(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        for (auto& object : m_objects)
        {
            // Ignore invalid objects
            if (!object.id == INVALID_ID) continue;
            // Ignore objects that are not of Point Light type
            if (object.type != SceneObjectType::PointLight) continue;
            // Get the metadata for this object
            auto& metadata = m_metadatas[object.metadataIndex];
            // Check the name against the provided one
            if (metadata.name == name)
            {
                // We have found our Point Light
                // Get the Point Light
                auto& light = m_pointLights[object.resourceIndex];
                // Delete the debug info
                auto debug = static_cast<LightDebugData*>(light.debugData);
                if (debug)
                {
                    debug->box.Unload();
                    debug->box.Destroy();
                    Memory.Delete(debug);
                }

                // Destroy the object
                object.Destroy();
                return true;
            }
        }

        ERROR_LOG("Failed to remove Point Light: '{}' from the scene.", name);
        return false;
    }

    bool Scene::AddMesh(const SceneMeshConfig& config)
    {
        if (config.name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        // Add a SceneObject
        SceneObject object;
        object.id = SCENE_OBJECT_ID++;
        // Store that this object is a Mesh
        object.type = SceneObjectType::Mesh;
        // Add the transform to the scene object
        object.node = m_graph.AddNode(config.transform);

        // Add some metadata to our Mesh
        SceneMetadata metadata;
        metadata.id           = METADATA_ID++;
        metadata.name         = config.name;
        metadata.resourceName = config.resourceName;

        // Add the Mesh resource
        MeshConfig c;
        c.name           = config.name;
        c.parentName     = config.parentName;
        c.resourceName   = config.resourceName;
        c.enableDebugBox = true;

        Mesh mesh;
        if (!mesh.Create(c))
        {
            ERROR_LOG("Failed to create Mesh: '{}'.", c.name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the mesh now
            if (!mesh.Initialize())
            {
                ERROR_LOG("Failed to initialize Mesh: '{}'.", config.name);
                return false;
            }
        }

        if (m_state >= SceneState::Loading)
        {
            // The scene has already been loaded (or is loading currently) so we need to load the mesh now
            if (!mesh.Load())
            {
                ERROR_LOG("Failed to load Mesh: '{}'.", config.name);
                return false;
            }
        }

        // Add the resource to our array
        object.resourceIndex = m_meshes.Size();
        m_meshes.PushBack(mesh);

        // Add the metadata to our array
        object.metadataIndex = m_metadatas.Size();
        m_metadatas.PushBack(metadata);

        // Finally store the object in our scene
        m_objects.PushBack(object);

        return true;
    }

    bool Scene::RemoveMesh(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        for (auto& object : m_objects)
        {
            // Ignore invalid objects
            if (!object.id == INVALID_ID) continue;
            // Ignore objects that are not of Mesh type
            if (object.type != SceneObjectType::Mesh) continue;
            // Get the metadata for this object
            auto& metadata = m_metadatas[object.metadataIndex];
            // Check the name against the provided one
            if (metadata.name == name)
            {
                // We have found our Mesh
                // Get the mesh and unload it
                auto& mesh = m_meshes[object.resourceIndex];
                if (!mesh.Destroy())
                {
                    ERROR_LOG("Failed to destroy Mesh: '{}'.", name);
                }
                // Destroy the object
                object.Destroy();
                return true;
            }
        }

        ERROR_LOG("Failed to remove Mesh: '{}' from the scene.", name);
        return false;
    }

    bool Scene::AddTerrain(const SceneTerrainConfig& config)
    {
        if (config.name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        // Add a SceneObject
        SceneObject object;
        object.id = SCENE_OBJECT_ID++;
        // Store that this object is a Terrain
        object.type = SceneObjectType::Terrain;
        // Add a node and transform for this Terrain
        object.node = m_graph.AddNode(config.transform);

        // Add some metadata to our Terrain
        SceneMetadata metadata;
        metadata.id           = METADATA_ID++;
        metadata.name         = config.name;
        metadata.resourceName = config.resourceName;

        // Add the Terrain resource
        TerrainConfig c;
        c.name         = config.name;
        c.resourceName = config.resourceName;

        Terrain terrain;
        if (!terrain.Create(c))
        {
            ERROR_LOG("Failed to create Terrain: '{}'.", c.name);
            return false;
        }

        if (m_state >= SceneState::Initialized)
        {
            // The scene has already been initialized so we need to initialize the terrain now
            if (!terrain.Initialize())
            {
                ERROR_LOG("Failed to initialize Terrain: '{}'.", config.name);
                return false;
            }
        }

        if (m_state >= SceneState::Loading)
        {
            // The scene has already been loaded (or is loading currently) so we need to load the terrain now
            if (!terrain.Load())
            {
                ERROR_LOG("Failed to load Terrain: '{}'.", config.name);
                return false;
            }
        }

        // Add the resource to our array
        object.resourceIndex = m_terrains.Size();
        m_terrains.PushBack(terrain);

        // Add the metadata to our array
        object.metadataIndex = m_metadatas.Size();
        m_metadatas.PushBack(metadata);

        // Finally store the object in our scene
        m_objects.PushBack(object);

        return true;
    }

    bool Scene::RemoveTerrain(const String& name)
    {
        if (name.Empty())
        {
            ERROR_LOG("Empty name provided.");
            return false;
        }

        for (auto& object : m_objects)
        {
            // Ignore invalid objects
            if (!object.id == INVALID_ID) continue;
            // Ignore objects that are not of Terrain type
            if (object.type != SceneObjectType::Terrain) continue;
            // Get the metadata for this object
            auto& metadata = m_metadatas[object.metadataIndex];
            // Check the name against the provided one
            if (metadata.name == name)
            {
                // We have found our Terrain
                // Get the terrain and unload it
                auto& terrain = m_terrains[object.resourceIndex];
                terrain.Destroy();
                // Destroy the object
                object.Destroy();
                return true;
            }
        }

        ERROR_LOG("Failed to remove Terrain: '{}' from the scene.", name);
        return false;
    }

    bool Scene::RayCast(const Ray& ray, RayCastResult& result)
    {
        if (m_state < SceneState::Loaded)
        {
            return false;
        }

        // TODO: Optimize to not check every mesh (with Spatial partitioning) to ensure we remain performant with many meshes
        for (auto& object : m_objects)
        {
            // We only care about interactions with meshes
            if (object.type == SceneObjectType::Mesh)
            {
                f32 distance;

                auto& mesh = m_meshes[object.resourceIndex];

                auto transform = m_graph.GetTransform(object.node);
                if (ray.TestAgainstExtents(mesh.GetExtents(), Transforms.GetWorld(transform), distance))
                {
                    // We have a hit
                    vec3 position = ray.origin + (ray.direction * distance);
                    result.hits.EmplaceBack(RAY_CAST_HIT_TYPE_OBB, object.id, position, distance);
                }
            }
        }

        return !result.hits.Empty();
    }

    Handle<Transform> Scene::GetTransformById(u32 id)
    {
        for (auto& object : m_objects)
        {
            if (object.id == id)
            {
                // We have found the thing that we hit.
                return m_graph.GetTransform(object.node);
            }
        }

        ERROR_LOG("Failed to find an object with id: {}. Returning invalid handle.", id);
        return Handle<Transform>();
    }

    void Scene::UnloadInternal()
    {
        for (auto& object : m_objects)
        {
            // Skip invalid objects
            if (object.id == INVALID_ID) continue;
            // Check if the metadata index is invalid we skip
            if (object.metadataIndex == INVALID_ID) continue;

            auto& metadata = m_metadatas[object.metadataIndex];
            switch (object.type)
            {
                case SceneObjectType::Skybox:
                    RemoveSkybox(metadata.name);
                    break;
                case SceneObjectType::DirectionalLight:
                    RemoveDirectionalLight(metadata.name);
                    break;
                case SceneObjectType::PointLight:
                    RemovePointLight(metadata.name);
                    break;
                case SceneObjectType::Mesh:
                    RemoveMesh(metadata.name);
                    break;
            }
        }

        if (!m_grid.Unload())
        {
            ERROR_LOG("Failed to unload Grid.");
        }

        m_state = SceneState::Unloaded;

        m_skyboxes.Destroy();
        m_directionalLights.Destroy();
        m_pointLights.Destroy();
        m_meshes.Destroy();
        m_terrains.Destroy();

        m_enabled = false;

        m_state = SceneState::Uninitialized;
    }
}  // namespace C3D