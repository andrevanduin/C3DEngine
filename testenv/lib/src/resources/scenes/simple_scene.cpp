
#include "simple_scene.h"

#include <core/frame_data.h>
#include <math/frustum.h>
#include <math/ray.h>
#include <renderer/renderer_types.h>
#include <resources/debug/debug_box_3d.h>
#include <resources/mesh.h>
#include <systems/lights/light_system.h>
#include <systems/render_views/render_view_system.h>
#include <systems/resources/resource_system.h>
#include <systems/system_manager.h>

#include "test_env_types.h"

struct LightDebugData
{
    C3D::DebugBox3D box;
};

static u32 global_scene_id = 0;

SimpleScene::SimpleScene() : m_logger("SIMPLE_SCENE"), m_name("NO_NAME"), m_description("NO_DESCRIPTION") {}

bool SimpleScene::Create(const C3D::SystemManager* pSystemsManager) { return Create(pSystemsManager, {}); }

bool SimpleScene::Create(const C3D::SystemManager* pSystemsManager, const SimpleSceneConfig& config)
{
    m_pSystemsManager = pSystemsManager;

    m_enabled = false;
    m_state   = SceneState::Uninitialized;
    m_id      = global_scene_id++;

    m_skybox = nullptr;

    m_meshes.Create(1024);
    m_terrains.Create(512);

    // Reserve a reasonable amount of space for geometries (to avoid reallocs)
    m_worldData.worldGeometries.Reserve(512);

    m_config = config;

    C3D::DebugGridConfig gridConfig;
    gridConfig.orientation   = C3D::DebugGridOrientation::XZ;
    gridConfig.tileCountDim0 = 100;
    gridConfig.tileCountDim1 = 100;
    gridConfig.tileScale     = 1.0f;
    gridConfig.name          = "DEBUG_GRID";
    gridConfig.useThirdAxis  = true;

    if (!m_grid.Create(m_pSystemsManager, gridConfig))
    {
        m_logger.Error("Create() - Failed to create debug grid.");
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
        C3D::SkyboxConfig config = { m_config.skyboxConfig.cubemapName };
        m_skybox                 = Memory.New<C3D::Skybox>(C3D::MemoryType::Scene);
        if (!m_skybox->Create(m_pSystemsManager, config))
        {
            m_logger.Error("Create() - Failed to create skybox from config");
            Memory.Delete(C3D::MemoryType::Scene, m_skybox);
            return false;
        }

        AddSkybox(m_config.skyboxConfig.name, m_skybox);
    }

    if (!m_config.directionalLightConfig.name.Empty())
    {
        auto& dirLightConfig = m_config.directionalLightConfig;

        auto dirLight = C3D::DirectionalLight();

        dirLight.name           = dirLightConfig.name;
        dirLight.data.color     = dirLightConfig.color;
        dirLight.data.direction = dirLightConfig.direction;

        m_directionalLight = m_config.directionalLightConfig.name;

        if (!Lights.AddDirectionalLight(dirLight))
        {
            m_logger.Error("Create() - Failed to add directional light from config");
            return false;
        }

        // TODO: Add debug data and initialize it here
    }

    if (!m_config.pointLights.Empty())
    {
        for (auto& config : m_config.pointLights)
        {
            auto light           = C3D::PointLight();
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
            m_logger.Warn("Initialize() - Mesh with empty name or empty resource name provided. Skipping");
            continue;
        }

        auto config           = C3D::MeshConfig();
        config.name           = meshConfig.name;
        config.resourceName   = meshConfig.resourceName;
        config.parentName     = meshConfig.parentName;
        config.enableDebugBox = true;  // Enable debug boxes around our meshes

        C3D::Mesh mesh;
        if (!mesh.Create(m_pSystemsManager, config))
        {
            m_logger.Error("Initialize() - Failed to create Mesh: '{}'. Skipping", meshConfig.name);
            continue;
        }
        mesh.transform = meshConfig.transform;
        m_meshes.Set(meshConfig.name, mesh);
    }

    for (auto& terrainConfig : m_config.terrains)
    {
        if (terrainConfig.name.Empty() || terrainConfig.resourceName.Empty())
        {
            m_logger.Warn("Initialize() - Terrain with empty name or empty resource name provided. Skipping");
            continue;
        }

        auto config         = C3D::TerrainConfig();
        config.name         = terrainConfig.name;
        config.resourceName = terrainConfig.resourceName;

        C3D::Terrain terrain;
        if (!terrain.Create(m_pSystemsManager, config))
        {
            m_logger.Error("Initialize() - Failed to create Terrain: '{}'. Skipping.", terrainConfig.name);
            continue;
        }

        terrain.SetTransform(terrainConfig.transform);

        m_terrains.Set(terrainConfig.name, terrain);
    }

    if (!m_grid.Initialize())
    {
        m_logger.Error("Initialize() - Failed to initialize Grid.");
        return false;
    }

    for (auto& name : m_pointLights)
    {
        auto light = Lights.GetPointLight(name);
        auto debug = static_cast<LightDebugData*>(light->debugData);
        if (!debug->box.Initialize())
        {
            m_logger.Error("AddPointLight() - Failed to initialize debug box for point light: '{}'.", light->name);
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
                m_logger.Warn(
                    "Initialize() - Mesh: '{}' was configured to have mesh named: '{}' as a parent. But the parent "
                    "does not exist in this scene.",
                    mesh.config.name, mesh.config.parentName);
            }
        }
    }

    if (m_skybox)
    {
        if (!m_skybox->Initialize())
        {
            m_logger.Error("Initialize() - Failed to initialize Skybox.");
            m_skybox = nullptr;
        }
    }

    for (auto& mesh : m_meshes)
    {
        if (!mesh.Initialize())
        {
            m_logger.Error("Initialize() - Failed to initialize Mesh: '{}'.", mesh.GetName());
        }
    }

    for (auto& terrain : m_terrains)
    {
        if (!terrain.Initialize())
        {
            m_logger.Error("Initialize() - Failed to inialize Terrain: '{}'.", terrain.GetName());
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
        }
    }

    for (auto& mesh : m_meshes)
    {
        if (!mesh.Load())
        {
            m_logger.Error("Load() - Failed to load Mesh: '{}'.", mesh.GetName());
        }
    }

    for (auto& terrain : m_terrains)
    {
        if (!terrain.Load())
        {
            m_logger.Error("Load() - Failed to load Terrain: '{}'.", terrain.GetName());
        }
    }

    if (!m_grid.Load())
    {
        m_logger.Error("Load() - Failed to load grid.");
        return false;
    }

    for (auto& name : m_pointLights)
    {
        auto light = Lights.GetPointLight(name);
        auto debug = static_cast<LightDebugData*>(light->debugData);
        if (!debug->box.Load())
        {
            m_logger.Error("Load() - Failed to load debug box for point light: '{}'.", name);
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

bool SimpleScene::Update(C3D::FrameData& frameData)
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

bool SimpleScene::PopulateRenderPacket(C3D::FrameData& frameData, const C3D::Camera* camera, f32 aspect, C3D::RenderPacket& packet)
{
    if (m_state != SceneState::Loaded) return true;

    if (m_skybox)
    {
        auto& skyboxViewPacket = packet.views[TEST_ENV_VIEW_SKYBOX];

        C3D::SkyboxPacketData skyboxData = { m_skybox };
        if (!Views.BuildPacket(skyboxViewPacket.view, frameData.frameAllocator, &skyboxData, &skyboxViewPacket))
        {
            m_logger.Error("PopulateRenderPacket() - Failed to populate render packet with skybox data");
            return false;
        }
    }

    auto& worldViewPacket = packet.views[TEST_ENV_VIEW_WORLD];
    // Clear our world geometries
    m_worldData.worldGeometries.Clear();
    m_worldData.terrainGeometries.Clear();
    m_worldData.debugGeometries.Clear();

    vec3 forward = camera->GetForward();
    vec3 right   = camera->GetRight();
    vec3 up      = camera->GetUp();

    C3D::Frustum frustum = C3D::Frustum(camera->GetPosition(), forward, right, up, aspect, C3D::DegToRad(45.0f), 0.1f, 1000.0f);

    for (const auto& mesh : m_meshes)
    {
        if (mesh.generation != INVALID_ID_U8)
        {
            mat4 model           = mesh.transform.GetWorld();
            bool windingInverted = mesh.transform.GetDeterminant() < 0;

            if (mesh.HasDebugBox())
            {
                const auto box = mesh.GetDebugBox();
                m_worldData.debugGeometries.EmplaceBack(box->GetModel(), box->GetGeometry(), box->GetUniqueId());
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
                    m_worldData.worldGeometries.EmplaceBack(model, geometry, mesh.uniqueId, windingInverted);
                    frameData.drawnMeshCount++;
                }
            }
        }
    }

    for (auto& terrain : m_terrains)
    {
        // TODO: Check terrain generation
        // TODO: Frustum culling
        m_worldData.terrainGeometries.EmplaceBack(terrain.GetModel(), terrain.GetGeometry(), terrain.uniqueId);
        // TODO: Seperate counter for terrain meshes/geometry
        frameData.drawnMeshCount++;
    }

    // Debug geometry
    // Grid
    constexpr auto identity = mat4(1.0f);
    m_worldData.debugGeometries.EmplaceBack(identity, m_grid.GetGeometry(), INVALID_ID);

    // TODO: Directional lights

    // Point Lights
    for (auto& name : m_pointLights)
    {
        auto light = Lights.GetPointLight(name);
        auto debug = static_cast<LightDebugData*>(light->debugData);

        if (debug && debug->box.IsValid())
        {
            m_worldData.debugGeometries.EmplaceBack(debug->box.GetModel(), debug->box.GetGeometry(), debug->box.GetUniqueId());
        }
    }

    // World
    if (!Views.BuildPacket(worldViewPacket.view, frameData.frameAllocator, &m_worldData, &worldViewPacket))
    {
        m_logger.Error("PopulateRenderPacket() - Failed to populate render packet with world data.");
        return false;
    }

    return true;
}

bool SimpleScene::AddDirectionalLight(const C3D::String& name, C3D::DirectionalLight& light)
{
    if (name.Empty())
    {
        m_logger.Error("AddDirectionalLight() - Empty name provided.");
        return false;
    }

    if (!m_directionalLight.Empty())
    {
        // TODO: Do resource unloading when required
        if (!Lights.RemoveDirectionalLight(m_directionalLight))
        {
            m_logger.Error("AddDirectionalLight() - Failed to remove current directional light.");
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

bool SimpleScene::RemoveDirectionalLight(const C3D::String& name)
{
    if (name.Empty())
    {
        m_logger.Error("RemoveDirectionalLight() - Empty name provided.");
        return false;
    }

    if (m_directionalLight)
    {
        // TODO: Cleanup debug data

        const auto result  = Lights.RemoveDirectionalLight(m_directionalLight);
        m_directionalLight = "";
        return result;
    }

    m_logger.Warn("RemoveDirectionalLight() - Could not remove since provided light is not part of this scene.");
    return false;
}

bool SimpleScene::AddPointLight(const C3D::PointLight& light)
{
    if (!Lights.AddPointLight(light))
    {
        m_logger.Error("AddPointLight() - Failed to add point light to lighting system.");
        return false;
    }

    auto pLight       = Lights.GetPointLight(light.name);
    pLight->debugData = Memory.New<LightDebugData>(C3D::MemoryType::Resource);
    auto debug        = static_cast<LightDebugData*>(pLight->debugData);

    if (!debug->box.Create(m_pSystemsManager, vec3(0.2f, 0.2f, 0.2f), nullptr))
    {
        m_logger.Error("AddPointLight() - Failed to add debug box to point light: '{}'", light.name);
        return false;
    }

    debug->box.SetPosition(light.data.position);

    if (m_state >= SceneState::Initialized)
    {
        if (!debug->box.Initialize())
        {
            m_logger.Error("AddPointLight() - Failed to initialize debug box for point light: '{}'.", light.name);
            return false;
        }
    }

    if (m_state >= SceneState::Loaded)
    {
        if (!debug->box.Load())
        {
            m_logger.Error("AddPointLight() - Failed to load debug box for point light: '{}'.", light.name);
            return false;
        }
    }

    m_pointLights.PushBack(light.name);
    return true;
}

bool SimpleScene::RemovePointLight(const C3D::String& name)
{
    if (m_pointLights.Contains(name))
    {
        auto pLight = Lights.GetPointLight(name);
        auto debug  = static_cast<LightDebugData*>(pLight->debugData);
        debug->box.Unload();
        debug->box.Destroy();
        Memory.Delete(C3D::MemoryType::Resource, debug);
    }

    auto result = Lights.RemovePointLight(name);
    if (result)
    {
        m_pointLights.Remove(name);
        return true;
    }

    m_logger.Error("RemovePointLight() - Failed to remove Point Light.");
    return false;
}

C3D::PointLight* SimpleScene::GetPointLight(const C3D::String& name) { return Lights.GetPointLight(name); }

bool SimpleScene::AddMesh(const C3D::String& name, C3D::Mesh& mesh)
{
    if (name.Empty())
    {
        m_logger.Error("AddMesh() - Empty name provided.");
        return false;
    }

    if (m_meshes.Has(name))
    {
        m_logger.Error("AddMesh() - A mesh with the name '{}' already exists.", name);
        return false;
    }

    if (m_state >= SceneState::Initialized)
    {
        // The scene has already been initialized so we need to initialize the mesh now
        if (!mesh.Initialize())
        {
            m_logger.Error("AddMesh() - Failed to initialize mesh: '{}'.", name);
            return false;
        }
    }

    if (m_state >= SceneState::Loading)
    {
        // The scene has already been loaded (or is loading currently) so we need to load the mesh now
        if (!mesh.Load())
        {
            m_logger.Error("AddMesh() - Failed to load mesh: '{}'", name);
            return false;
        }
    }

    m_meshes.Set(name, mesh);
    return true;
}

bool SimpleScene::RemoveMesh(const C3D::String& name)
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

    m_meshes.Delete(name);
    return true;
}

C3D::Mesh& SimpleScene::GetMesh(const C3D::String& name) { return m_meshes.Get(name); }

bool SimpleScene::AddTerrain(const C3D::String& name, C3D::Terrain& terrain)
{
    if (name.Empty())
    {
        m_logger.Error("AddTerrain() - Empty name provided");
        return false;
    }

    if (m_terrains.Has(name))
    {
        m_logger.Error("AddTerrain() - A terrain with the name '{}' already exists", name);
        return false;
    }

    if (m_state >= SceneState::Initialized)
    {
        // The scene has already been initialized so we need to initialize the terrain now
        if (!terrain.Initialize())
        {
            m_logger.Error("AddTerrain() - Failed to initialize terrain: '{}'", name);
            return false;
        }
    }

    if (m_state >= SceneState::Loading)
    {
        // The scene has already been loaded (or is loading currently) so we need to load the terrain now
        if (!terrain.Load())
        {
            m_logger.Error("AddTerrain() - Failed to load terrain: '{}'", name);
            return false;
        }
    }

    m_terrains.Set(name, terrain);
    return true;
}

bool SimpleScene::RemoveTerrain(const C3D::String& name)
{
    if (name.Empty())
    {
        m_logger.Error("RemoveTerrain() - Empty name provided");
        return false;
    }

    if (!m_terrains.Has(name))
    {
        m_logger.Error("RemoveTerrain() - Unknown name provided: '{}'", name);
        return false;
    }

    auto& terrain = m_terrains.Get(name);
    if (!terrain.Unload())
    {
        m_logger.Error("RemoveTerrain() - Failed to unload terrain: '{}'", name);
        return false;
    }

    m_terrains.Delete(name);
    return true;
}

C3D::Terrain& SimpleScene::GetTerrain(const C3D::String& name) { return m_terrains.Get(name); }

bool SimpleScene::AddSkybox(const C3D::String& name, C3D::Skybox* skybox)
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

bool SimpleScene::RemoveSkybox(const C3D::String& name)
{
    if (name.Empty())
    {
        m_logger.Error("RemoveSkybox() - Empty name provided.");
        return false;
    }

    if (m_skybox)
    {
        if (!m_skybox->Unload())
        {
            m_logger.Error("RemoveSkybox() - Failed to unload skybox.");
        }

        m_skybox->Destroy();

        Memory.Delete(C3D::MemoryType::Scene, m_skybox);
        m_skybox = nullptr;
        return true;
    }

    m_logger.Warn("RemoveSkybox() - Could not remove since scene does not have a skybox.");
    return false;
}

bool SimpleScene::RayCast(const C3D::Ray& ray, C3D::RayCastResult& result)
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
            result.hits.EmplaceBack(C3D::RAY_CAST_HIT_TYPE_OBB, mesh.uniqueId, position, distance);
        }
    }

    return !result.hits.Empty();
}

C3D::Transform* SimpleScene::GetTransformById(u32 uniqueId)
{
    for (auto& mesh : m_meshes)
    {
        if (mesh.uniqueId == uniqueId)
        {
            return &mesh.transform;
        }
    }

    for (auto& terrain : m_terrains)
    {
        if (terrain.uniqueId == uniqueId)
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
            m_logger.Error("Unload() - Failed to unload Mesh: '{}'.", mesh.GetName());
        }

        mesh.Destroy();
    }

    for (auto& terrain : m_terrains)
    {
        if (!terrain.Unload())
        {
            m_logger.Error("Unload() - Failed to unload Terrain: '{}'.", terrain.GetName());
        }

        terrain.Destroy();
    }

    if (!m_grid.Unload())
    {
        m_logger.Error("Unload() - Failed to unload Grid.");
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
        Memory.Delete(C3D::MemoryType::Resource, debug);
        Lights.RemovePointLight(name);
    }

    m_state = SceneState::Unloaded;

    m_pointLights.Destroy();
    m_meshes.Destroy();
    m_terrains.Destroy();

    m_worldData.worldGeometries.Destroy();
    m_worldData.terrainGeometries.Destroy();
    m_worldData.debugGeometries.Destroy();

    m_directionalLight = "";
    m_skybox           = nullptr;
    m_enabled          = false;

    m_state = SceneState::Uninitialized;
}
