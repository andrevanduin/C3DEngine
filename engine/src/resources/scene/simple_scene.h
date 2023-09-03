
#pragma once
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "core/defines.h"
#include "renderer/render_view.h"
#include "renderer/transform.h"
#include "renderer/vertex.h"
#include "resources/geometry.h"
#include "resources/mesh.h"
#include "simple_scene_config.h"

namespace C3D
{
    class SystemManager;
    struct FrameData;
    struct RenderPacket;
    struct DirectionalLight;
    struct PointLight;
    struct Skybox;

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

    class C3D_API SimpleScene
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
        bool Update(FrameData& frameData);

        /**
         * @brief Populates the render packet with everything that needs to be rendered by this scene.
         *
         * @param frameData The frame specific data
         * @param packet The packet that needs to be filled with render data
         * @return True if successful, false otherwise
         */
        bool PopulateRenderPacket(FrameData& frameData, RenderPacket& packet);

        bool AddDirectionalLight(const String& name, DirectionalLight& light);
        bool RemoveDirectionalLight(const String& name);

        bool AddPointLight(const PointLight& light);
        bool RemovePointLight(const String& name);
        PointLight* GetPointLight(const String& name);

        bool AddMesh(const String& name, Mesh& mesh);
        bool RemoveMesh(const String& name);
        Mesh& GetMesh(const String& name);

        bool AddSkybox(const String& name, Skybox* skybox);
        bool RemoveSkybox(const String& name);

        [[nodiscard]] u32 GetId() const { return m_id; }
        [[nodiscard]] SceneState GetState() const { return m_state; }
        [[nodiscard]] bool IsEnabled() const { return m_enabled; }

    private:
        /** @brief Unloads the scene. Deallocates the resources for the scene.
         * After calling this method the scene is in an unloaded state ready to be destroyed.*/
        void UnloadInternal();

        LoggerInstance<16> m_logger;

        u32 m_id           = INVALID_ID;
        SceneState m_state = SceneState::Uninitialized;
        bool m_enabled     = false;

        SimpleSceneConfig m_config;
        String m_name;
        String m_description;

        Skybox* m_skybox = nullptr;

        String m_directionalLight;
        DynamicArray<String> m_pointLights;
        HashMap<String, Mesh> m_meshes;

        DynamicArray<GeometryRenderData, LinearAllocator> m_worldGeometries;

        Transform m_transform;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D