
#pragma once
#include "containers/string.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"
#include "resources/geometry.h"
#include "resources/scene/simple_scene_config.h"

namespace C3D
{
    class SystemManager;
    class Mesh;
    class DebugBox3D;

    struct MeshConfig
    {
        MeshConfig() = default;

        MeshConfig(const SimpleSceneMeshConfig& cfg) : name(cfg.name), resourceName(cfg.resourceName)
        {
            if (!cfg.parentName.Empty())
            {
                parentName = cfg.parentName;
            }
        }

        String name;
        String resourceName;
        String parentName;
        DynamicArray<GeometryConfig> geometryConfigs;
        bool enableDebugBox = false;
    };

    struct MeshLoadParams
    {
        String resourceName;
        Mesh* outMesh = nullptr;
    };

    class C3D_API Mesh
    {
    public:
        Mesh();

        bool Create(const SystemManager* pSystemsManager, const MeshConfig& cfg);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Destroy();

        const String& GetName() const { return config.name; }

        DebugBox3D* GetDebugBox() const { return m_debugBox; }

        bool HasDebugBox() const { return m_debugBox != nullptr; }

        u32 uniqueId  = INVALID_ID;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;
        MeshConfig config;

    private:
        bool LoadFromResource();

        void LoadJobSuccess();

        void LoadJobFailure();

        LoggerInstance<8> m_logger;
        MeshResource m_resource;
        Extents3D m_extents;
        DebugBox3D* m_debugBox = nullptr;

        const SystemManager* m_pSystemsManager = nullptr;
    };

    class C3D_API UIMesh
    {
    public:
        UIMesh();

        bool LoadFromConfig(const SystemManager* pSystemsManager, const UIGeometryConfig& config);

        bool Unload();

        u32 uniqueId  = INVALID_ID;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;

    private:
        LoggerInstance<8> m_logger;
        const SystemManager* m_pSystemsManager = nullptr;
    };

}  // namespace C3D
