
#pragma once
#include "containers/string.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"
#include "resources/geometry.h"

namespace C3D
{
    class SystemManager;
    class Mesh;

    struct MeshConfig
    {
        const char* resourceName = nullptr;
        DynamicArray<GeometryConfig> geometryConfigs;
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

        bool Create(const SystemManager* pSystemsManager, const MeshConfig& config);

        bool Initialize();

        bool Load();

        bool Unload();

        bool Destroy();

        u32 uniqueId = INVALID_ID;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;

    private:
        bool LoadFromResource();

        void LoadJobSuccess();

        void LoadJobFailure();

        LoggerInstance<8> m_logger;

        MeshConfig m_config;
        MeshResource m_resource;

        const SystemManager* m_pSystemsManager = nullptr;
    };

    class C3D_API UIMesh
    {
    public:
        UIMesh();

        bool LoadFromConfig(const SystemManager* pSystemsManager, const UIGeometryConfig& config);

        bool Unload();

        u32 uniqueId = INVALID_ID;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;

    private:
        LoggerInstance<8> m_logger;
        const SystemManager* m_pSystemsManager = nullptr;
    };

}  // namespace C3D
