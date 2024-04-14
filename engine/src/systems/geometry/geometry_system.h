
#pragma once
#include "containers/dynamic_array.h"
#include "core/clock.h"
#include "core/defines.h"
#include "core/engine.h"
#include "core/logger.h"
#include "renderer/geometry.h"
#include "renderer/renderer_frontend.h"
#include "renderer/vertex.h"
#include "resources/geometry_config.h"
#include "systems/materials/material_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr auto DEFAULT_GEOMETRY_NAME = "default";

    struct GeometrySystemConfig
    {
        // Max number of geometries that can be loaded at once.
        // NOTE: Should be significantly greater than the number of static meshes
        // because there can and will be more than one of these per mesh
        // take other systems into account as well
        u32 maxGeometryCount;
    };

    struct GeometryReference
    {
        GeometryReference() = default;

        u64 referenceCount = 0;
        Geometry geometry;
        bool autoRelease = false;
    };

    class C3D_API GeometrySystem final : public SystemWithConfig<GeometrySystemConfig>
    {
    public:
        explicit GeometrySystem(const SystemManager* pSystemsManager);

        bool OnInit(const GeometrySystemConfig& config) override;
        void OnShutdown() override;

        [[nodiscard]] Geometry* AcquireById(u32 id) const;

        template <typename VertexType, typename IndexType>
        [[nodiscard]] Geometry* AcquireFromConfig(const IGeometryConfig<VertexType, IndexType>& config, bool autoRelease) const;

        template <typename VertexType, typename IndexType>
        static void DisposeConfig(IGeometryConfig<VertexType, IndexType>& config)
        {
            config.vertices.Destroy();
            config.indices.Destroy();
        }

        void Release(const Geometry* geometry) const;

        Geometry* GetDefault();

        // NOTE: Vertex and index arrays are dynamically allocated so they should be freed by the user
        [[nodiscard]] static GeometryConfig GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 tileX,
                                                                f32 tileY, const String& name, const String& materialName);

        [[nodiscard]] static GeometryConfig GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 tileX, f32 tileY, const String& name,
                                                               const String& materialName);

    private:
        template <typename VertexType, typename IndexType>
        bool CreateGeometry(const IGeometryConfig<VertexType, IndexType>& config, Geometry* g) const;

        void DestroyGeometry(Geometry* g) const;

        bool CreateDefaultGeometries();

        Geometry m_defaultGeometry;

        GeometryReference* m_registeredGeometries = nullptr;
    };

    template <typename VertexType, typename IndexType>
    Geometry* GeometrySystem::AcquireFromConfig(const IGeometryConfig<VertexType, IndexType>& config, const bool autoRelease) const
    {
        Geometry* g = nullptr;
        for (u32 i = 0; i < m_config.maxGeometryCount; i++)
        {
            if (m_registeredGeometries[i].geometry.id == INVALID_ID)
            {
                // We found an empty slot
                m_registeredGeometries[i].autoRelease    = autoRelease;
                m_registeredGeometries[i].referenceCount = 1;

                g     = &m_registeredGeometries[i].geometry;
                g->id = i;
                break;
            }
        }

        if (!g)
        {
            INSTANCE_ERROR_LOG("GEOMETRY_SYSTEM",
                               "Unable to obtain free slot for geometry. Adjust config to allow for more space. Returning nullptr.");
            return nullptr;
        }

        if (!CreateGeometry(config, g))
        {
            INSTANCE_ERROR_LOG("GEOMETRY_SYSTEM", "Failed to create geometry. Returning nullptr.");
            return nullptr;
        }

        return g;
    }

    template <typename VertexType, typename IndexType>
    bool GeometrySystem::CreateGeometry(const IGeometryConfig<VertexType, IndexType>& config, Geometry* g) const
    {
        if (!g)
        {
            INSTANCE_ERROR_LOG("GEOMETRY_SYSTEM", "Requires a valid pointer to geometry.");
            return false;
        }

        // Send the geometry off to the renderer to be uploaded to the gpu
        if (!Renderer.CreateGeometry(*g, sizeof(VertexType), config.vertices.Size(), config.vertices.GetData(), sizeof(IndexType),
                                     config.indices.Size(), config.indices.GetData()))
        {
            INSTANCE_ERROR_LOG("GEOMETRY_SYSTEM", "Creating geometry failed during the Renderer's CreateGeometry.");
            m_registeredGeometries[g->id].referenceCount = 0;
            m_registeredGeometries[g->id].autoRelease    = false;
            g->id                                        = INVALID_ID;
            g->generation                                = INVALID_ID_U16;

            return false;
        }

        if (!Renderer.UploadGeometry(*g))
        {
            INSTANCE_ERROR_LOG("GEOMETRY_SYSTEM", "Creating geometry failed during the Renderer's UploadGeometry.");
            m_registeredGeometries[g->id].referenceCount = 0;
            m_registeredGeometries[g->id].autoRelease    = false;
            g->id                                        = INVALID_ID;
            g->generation                                = INVALID_ID_U16;

            return false;
        }

        // Copy over the center and extents
        g->center      = config.center;
        g->extents.min = config.minExtents;
        g->extents.max = config.maxExtents;
        g->name        = config.name;

        // Acquire the material
        if (!config.materialName.Empty())
        {
            g->material = Materials.Acquire(config.materialName.Data());
            if (!g->material)
            {
                g->material = Materials.GetDefault();
            }
        }

        return true;
    }
}  // namespace C3D
