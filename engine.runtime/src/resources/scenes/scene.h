
#pragma once
#include "containers/dynamic_array.h"
#include "containers/hash_map.h"
#include "defines.h"
#include "graphs/hierarchy_graph.h"
#include "identifiers/uuid.h"
#include "math/frustum.h"
#include "renderer/camera.h"
#include "renderer/vertex.h"
#include "resources/debug/debug_grid.h"
#include "resources/mesh.h"
#include "resources/skybox.h"
#include "resources/terrain/terrain.h"
#include "scene_config.h"
#include "systems/lights/light_system.h"
#include "systems/transforms/transform_system.h"

namespace C3D
{
    class SystemManager;
    struct FrameData;
    struct RenderPacket;

    class Ray;
    class Transform;
    class Viewport;
    struct RayCastResult;

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

    struct SceneMetadata
    {
        /** @brief The id for this metadata. Will be INVALID for empty nodes. */
        u32 id = INVALID_ID;
        /** @brief The name of the object. */
        String name;
        /** @brief The name of the object's resource. */
        String resourceName;
    };

    enum class SceneObjectType
    {
        None = 0,
        Skybox,
        DirectionalLight,
        PointLight,
        Mesh,
        Terrain,
    };

    struct SceneObject
    {
        /** @brief The node's id. Will be INVALID_ID for empty objects. */
        u32 id = INVALID_ID;
        /** @brief A handle to the hierarchy graph node. */
        Handle<HierarchyGraphNode> node;
        /** @brief The type of this object. */
        SceneObjectType type = SceneObjectType::None;
        /** @brief An index into the resource array. */
        u32 resourceIndex = INVALID_ID;
        /** @brief An index into the metadata array. */
        u32 metadataIndex = INVALID_ID;

        void Destroy()
        {
            type          = SceneObjectType::None;
            id            = INVALID_ID;
            resourceIndex = INVALID_ID;
            metadataIndex = INVALID_ID;
        }
    };

    class C3D_API Scene
    {
    public:
        Scene();

        /**
         * @brief Creates a new scene with the given config. No resources are allocated yet
         *
         * @return True if successful, false otherwise
         */
        bool Create();
        bool Create(const SceneConfig& config);

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
         * @return True if successful; False otherwise
         */
        bool Load();

        /**
         * @brief Saves the scene to a file.
         *
         * @return True if successful; False otherwise
         */
        bool Save();

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

        void OnPrepareRender(FrameData& frameData) const;

        void UpdateLodFromViewPosition(FrameData& frameData, const vec3& viewPosition, f32 nearClip, f32 farClip);

        void QueryMeshes(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                         DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const;
        void QueryMeshes(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                         DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const;

        void QueryTerrains(FrameData& frameData, const Frustum& frustum, const vec3& cameraPosition,
                           DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const;
        void QueryTerrains(FrameData& frameData, const vec3& direction, const vec3& center, f32 radius,
                           DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const;

        void QueryMeshes(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& meshData) const;
        void QueryTerrains(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& terrainData) const;
        void QueryDebugGeometry(FrameData& frameData, DynamicArray<GeometryRenderData, LinearAllocator>& debugData) const;

        void QueryDirectionalLights(FrameData& frameData, DynamicArray<DirectionalLightData, LinearAllocator>& lightData) const;
        void QueryPointLights(FrameData& frameData, DynamicArray<PointLightData, LinearAllocator>& lightData) const;

        bool RayCast(const Ray& ray, RayCastResult& result);

        Handle<Transform> GetTransformById(u32 id);

        [[nodiscard]] u32 GetId() const { return m_id; }
        [[nodiscard]] SceneState GetState() const { return m_state; }
        [[nodiscard]] bool IsEnabled() const { return m_enabled; }

        Skybox& GetSkybox();
        PointLight* GetPointLight(const String& name);

    private:
        const SceneObject& GetParent(const String& name) const;

        bool AddSkybox(const SceneSkyboxConfig& config);
        bool RemoveSkybox(const String& name);

        bool AddDirectionalLight(const SceneDirectionalLightConfig& config);
        bool RemoveDirectionalLight(const String& name);

        bool AddPointLight(const ScenePointLightConfig& config);
        bool RemovePointLight(const String& name);

        bool AddMesh(const SceneMeshConfig& config);
        bool RemoveMesh(const String& name);

        bool AddTerrain(const SceneTerrainConfig& config);
        bool RemoveTerrain(const String& name);

        /** @brief Unloads the scene. Deallocates the resources for the scene.
         * After calling this method the scene is in an unloaded state ready to be destroyed.*/
        void UnloadInternal();

        u32 m_id           = INVALID_ID;
        SceneState m_state = SceneState::Uninitialized;
        bool m_enabled     = false;

        SceneConfig m_config;
        String m_name;
        String m_description;

        HierarchyGraph m_graph;

        DynamicArray<SceneObject> m_objects;

        DynamicArray<Skybox> m_skyboxes;
        DynamicArray<DirectionalLight> m_directionalLights;
        DynamicArray<PointLight> m_pointLights;
        DynamicArray<Mesh> m_meshes;
        DynamicArray<Terrain> m_terrains;

        DynamicArray<SceneMetadata> m_metadatas;

        DebugGrid m_grid;
    };

}  // namespace C3D
