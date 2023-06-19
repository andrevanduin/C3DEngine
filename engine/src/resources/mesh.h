
#pragma once
#include "core/identifier.h"
#include "geometry.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"

namespace C3D
{
    class SystemManager;

    struct MeshLoadParams
    {
        MeshLoadParams() : outMesh(nullptr) {}

        String resourceName;
        Mesh* outMesh;
        MeshResource meshResource;
    };

    class C3D_API Mesh
    {
    public:
        Mesh();

        /**
         * @brief Loads a cube with the provided arguments
         *
         * @param pSystemsManager A const pointer to the Systems Manager
         * @param width The width of the cube
         * @param height The height of the cube
         * @param depth The depth of the cube
         * @param tileX How the texture should tile on the cube in x direction (1.0 is no tiling)
         * @param tileY How the texture should tile on the cube in y direction (1.0 is no tiling)
         * @param name The name for the cube
         * @param materialName The name for the material of the cube
         * @return True on successful load, false otherwise
         */
        bool LoadCube(const SystemManager* pSystemsManager, f32 width, f32 height, f32 depth, f32 tileX, f32 tileY,
                      const String& name, const String& materialName);
        bool LoadFromResource(const SystemManager* pSystemsManager, const char* resourceName);

        template <typename VertexType, typename IndexType>
        bool LoadFromConfig(const SystemManager* pSystemsManager, const GeometryConfig<VertexType, IndexType>& config)
        {
            m_pSystemsManager = pSystemsManager;

            uniqueId = Identifier::GetNewId(this);
            geometries.PushBack(Geometric.AcquireFromConfig(config, true));
            generation = 0;

            return true;
        }

        void Unload();

        u32 uniqueId = INVALID_ID;
        u8 generation = INVALID_ID_U8;

        DynamicArray<Geometry*> geometries;

        Transform transform;

    private:
        bool LoadJobEntryPoint(void* data, void* resultData) const;
        void LoadJobSuccess(void* data) const;
        void LoadJobFailure(void* data) const;

        const SystemManager* m_pSystemsManager = nullptr;
    };
}  // namespace C3D
