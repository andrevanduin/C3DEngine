
#pragma once
#include <containers/dynamic_array.h>
#include <containers/hash_map.h>
#include <core/defines.h>
#include <core/uuid.h>
#include <math/frustum.h>
#include <renderer/camera.h>
#include <renderer/transform.h>
#include <renderer/vertex.h>
#include <resources/debug/debug_grid.h>
#include <resources/mesh.h>
#include <resources/terrain/terrain.h>

#include "simple_scene_config.h"

namespace C3D
{
    class SystemManager;
    struct FrameData;
    struct RenderPacket;
    struct DirectionalLight;
    struct PointLight;
    struct Skybox;

    class Ray;
    class Transform;
    class Viewport;
    struct RayCastResult;

}  // namespace C3D

enum class SceneState
{
    /** @brief Created, but not initialized yet */
    Uninitialized,
    /** @brief Configuration is parsed and hierarchy is setup but not loaded yet */
    Initialized,
    /** @brief Loading the actual hierarchy */
    Loading,
    /** @brief Loading is done and the scene is ready to play */
    Loaded,
    /** @brief The scene is currently unloading (can't play anymore) */
    Unloading,
    /** @brief The scene is unloaded and ready to be destroyed */
    Unloaded,
};

using namespace C3D;

class SimpleScene
{
public:
    SimpleScene();

    /**
     * @brief Creates a new scene with the given config. No resources are allocated yet
     *
     * @return True if successful, false otherwise
     */
    bool Create(const SystemManager* pSystemsManager);
    bool Create(const SystemManager* pSystemsManager, const SimpleSceneConfig& config);

    /**
     * @brief Initializes the scene. Processes configuration and sets up the hierarchy
     *
     * @return True if successful, false otherwise
     */
    bool Initialize();

    /**
     * @brief Loads the scene. Allocates the resources required to actually show the scene.
     * After calling this method the scene becomes playable
     *
     * @return True if successful, false otherwise
     */
    bool Load();

    /**
     * @brief Marks the scene to be unloaded. Will start unloading as soon as possible.
     *
     * @param immediate Boolean specifying if the unload should immediatly happen
     * Can have unforseen consequences so this flag should only be set to true when you have no other options.
     * For example on application shutdown.
     *
     * @return True if successful, false otherwise
     */
    bool Unload(bool immediate = false);

    /**
     * @brief Updates the scene.
     *
     * @param frameData The Frame specific data
     * @return True if successful, false otherwise
     */
    bool Update(C3D::FrameData& frameData);

    void QueryMeshes(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                     DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const;
    void QueryMeshes(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                     DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const;

    void QueryTerrains(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                       DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const;

    void QueryMeshes(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const;
    void QueryTerrains(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const;
    void QueryDebugGeometry(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& debugData) const;

    bool AddDirectionalLight(const String& name, C3D::DirectionalLight& light);
    bool RemoveDirectionalLight(const String& name);

    bool AddPointLight(const PointLight& light);
    bool RemovePointLight(const String& name);
    PointLight* GetPointLight(const String& name);

    bool AddMesh(const String& name, Mesh& mesh);
    bool RemoveMesh(const String& name);
    Mesh& GetMesh(const String& name);

    bool AddTerrain(const String& name, Terrain& terrain);
    bool RemoveTerrain(const String& name);
    Terrain& GetTerrain(const String& name);

    bool AddSkybox(const String& name, Skybox* skybox);
    bool RemoveSkybox(const String& name);

    bool RayCast(const Ray& ray, RayCastResult& result);

    Transform* GetTransformById(UUID uuid);

    [[nodiscard]] u32 GetId() const { return m_id; }
    [[nodiscard]] SceneState GetState() const { return m_state; }
    [[nodiscard]] bool IsEnabled() const { return m_enabled; }

    [[nodiscard]] C3D::Skybox* GetSkybox() const { return m_skybox; }

private:
    /** @brief Unloads the scene. Deallocates the resources for the scene.
     * After calling this method the scene is in an unloaded state ready to be destroyed.*/
    void UnloadInternal();

    u32 m_id           = INVALID_ID;
    SceneState m_state = SceneState::Uninitialized;
    bool m_enabled     = false;

    SimpleSceneConfig m_config;
    C3D::String m_name;
    C3D::String m_description;

    C3D::Skybox* m_skybox = nullptr;

    C3D::DebugGrid m_grid;

    C3D::String m_directionalLight;
    C3D::DynamicArray<C3D::String> m_pointLights;

    C3D::HashMap<C3D::String, C3D::Mesh> m_meshes;
    C3D::HashMap<C3D::String, C3D::Terrain> m_terrains;

    C3D::Transform m_transform;

    friend class ScenePass;
    friend class ShadowMapPass;

    const C3D::SystemManager* m_pSystemsManager = nullptr;
};
