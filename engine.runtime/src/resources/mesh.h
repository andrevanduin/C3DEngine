
#pragma once
#include "identifiers/uuid.h"
#include "managers/mesh_manager.h"
#include "resources/geometry_config.h"
#include "string/string.h"
#include "systems/transforms/transform_system.h"

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

        bool Create(const MeshConfig& cfg);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Destroy();

        UUID GetId() const { return m_id; }

        const String& GetName() const { return config.name; }

        DebugBox3D* GetDebugBox() const { return m_debugBox; }

        bool HasDebugBox() const { return m_debugBox != nullptr; }

        const Extents3D& GetExtents() const { return m_extents; }

        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        MeshConfig config;

    private:
        void LoadFromResource();

        bool LoadJobEntry();

        void LoadJobSuccess();

        void LoadJobFailure();

        UUID m_id;

        MeshResource m_resource;
        Extents3D m_extents;
        DebugBox3D* m_debugBox = nullptr;
    };
}  // namespace C3D
