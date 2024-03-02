
#pragma once
#include "containers/string.h"
#include "core/uuid.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"
#include "resources/geometry_config.h"

namespace C3D
{
    class SystemManager;
    class Mesh;
    class DebugBox3D;
    class Geometry;

    struct MeshConfig
    {
        MeshConfig() = default;

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
        Mesh() = default;

        bool Create(const SystemManager* pSystemsManager, const MeshConfig& cfg);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Destroy();

        const String& GetName() const { return config.name; }

        DebugBox3D* GetDebugBox() const { return m_debugBox; }

        bool HasDebugBox() const { return m_debugBox != nullptr; }

        const Extents3D& GetExtents() const { return m_extents; }

        UUID uuid;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;
        MeshConfig config;

    private:
        bool LoadFromResource();

        void LoadJobSuccess();

        void LoadJobFailure();

        MeshResource m_resource;
        Extents3D m_extents;
        DebugBox3D* m_debugBox = nullptr;

        const SystemManager* m_pSystemsManager = nullptr;
    };

    class C3D_API UIMesh
    {
    public:
        UIMesh() = default;

        bool LoadFromConfig(const SystemManager* pSystemsManager, const UIGeometryConfig& config);

        bool Unload();

        UUID uuid;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;

    private:
        const SystemManager* m_pSystemsManager = nullptr;
    };

}  // namespace C3D
