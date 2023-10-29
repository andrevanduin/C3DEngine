
#pragma once
#include <containers/dynamic_array.h>
#include <containers/hash_map.h>
#include <core/defines.h>
#include <renderer/camera.h>
#include <renderer/render_view.h>
#include <renderer/render_view_types.h>
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

class SimpleScene
{
public:
    SimpleScene();

    /**
     * @brief Creates a new scene with the given config. No resources are allocated yet
     *
     * @return True if successful, false otherwise
     */
    bool Create(const C3D::SystemManager* pSystemsManager);
    bool Create(const C3D::SystemManager* pSystemsManager, const SimpleSceneConfig& config);

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

    /**
     * @brief Populates the render packet with everything that needs to be rendered by this scene.
     *
     * @param frameData The frame specific data
     * @param packet The packet that needs to be filled with render data
     * @return True if successful, false otherwise
     */
    bool PopulateRenderPacket(C3D::FrameData& frameData, C3D::Camera* camera, const C3D::Viewport& viewport, C3D::RenderPacket& packet);

    bool AddDirectionalLight(const C3D::String& name, C3D::DirectionalLight& light);
    bool RemoveDirectionalLight(const C3D::String& name);

    bool AddPointLight(const C3D::PointLight& light);
    bool RemovePointLight(const C3D::String& name);
    C3D::PointLight* GetPointLight(const C3D::String& name);

    bool AddMesh(const C3D::String& name, C3D::Mesh& mesh);
    bool RemoveMesh(const C3D::String& name);
    C3D::Mesh& GetMesh(const C3D::String& name);

    bool AddTerrain(const C3D::String& name, C3D::Terrain& terrain);
    bool RemoveTerrain(const C3D::String& name);
    C3D::Terrain& GetTerrain(const C3D::String& name);

    bool AddSkybox(const C3D::String& name, C3D::Skybox* skybox);
    bool RemoveSkybox(const C3D::String& name);

    bool RayCast(const C3D::Ray& ray, C3D::RayCastResult& result);

    C3D::Transform* GetTransformById(u32 uniqueId);

    [[nodiscard]] u32 GetId() const { return m_id; }
    [[nodiscard]] SceneState GetState() const { return m_state; }
    [[nodiscard]] bool IsEnabled() const { return m_enabled; }

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

    C3D::RenderViewWorldData m_worldData;

    C3D::Transform m_transform;

    const C3D::SystemManager* m_pSystemsManager = nullptr;
};
