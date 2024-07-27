
#include "geometry_system.h"

#include "core/logger.h"
#include "math/geometry_utils.h"
#include "renderer/renderer_frontend.h"
#include "renderer/vertex.h"
#include "systems/materials/material_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "GEOMETRY_SYSTEM";

    bool GeometrySystem::OnInit(const GeometrySystemConfig& config)
    {
        INFO_LOG("Initializing.");
        m_config = config;

        m_registeredGeometries = Memory.Allocate<GeometryReference>(MemoryType::Geometry, m_config.maxGeometryCount);

        const u32 count = m_config.maxGeometryCount;
        for (u32 i = 0; i < count; i++)
        {
            m_registeredGeometries[i].geometry.id         = INVALID_ID;
            m_registeredGeometries[i].geometry.generation = INVALID_ID_U16;
        }

        if (!CreateDefaultGeometries())
        {
            ERROR_LOG("Failed to create default geometries.");
            return false;
        }

        m_initialized = true;
        return true;
    }

    void GeometrySystem::OnShutdown()
    {
        INFO_LOG("Shutting down.");

        // Cleanup the default geometry
        DestroyGeometry(&m_defaultGeometry);

        Memory.Free(m_registeredGeometries);
        m_registeredGeometries = nullptr;
    }

    Geometry* GeometrySystem::AcquireById(const u32 id) const
    {
        if (id != INVALID_ID && m_registeredGeometries[id].geometry.id != INVALID_ID)
        {
            m_registeredGeometries->referenceCount++;
            return &m_registeredGeometries[id].geometry;
        }

        // NOTE: possibly should return the default geometry instead
        ERROR_LOG("Cannot load invalid geometry id. Returning nullptr.");
        return nullptr;
    }

    void GeometrySystem::Release(const Geometry* geometry) const
    {
        if (geometry && geometry->id != INVALID_ID)
        {
            GeometryReference* ref = &m_registeredGeometries[geometry->id];

            if (ref->geometry.id == geometry->id)
            {
                if (ref->referenceCount > 0) ref->referenceCount--;

                if (ref->referenceCount < 1 && ref->autoRelease)
                {
                    DestroyGeometry(&ref->geometry);
                    ref->referenceCount = 0;
                    ref->autoRelease    = false;
                }
            }
            else
            {
                FATAL_LOG("Geometry id mismatch. Check registration logic as this should never occur!");
            }
            return;
        }

        WARN_LOG("Called with invalid geometry id. Noting was done.");
    }

    Geometry* GeometrySystem::GetDefault()
    {
        if (!m_initialized)
        {
            FATAL_LOG("Called before system was initialized.");
            return nullptr;
        }

        return &m_defaultGeometry;
    }

    GeometryConfig GeometrySystem::GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 tileX, f32 tileY,
                                                       const String& name, const String& materialName)
    {
        if (width == 0.f) width = 1.0f;
        if (height == 0.f) height = 1.0f;
        if (xSegmentCount < 1) xSegmentCount = 1;
        if (ySegmentCount < 1) ySegmentCount = 1;
        if (tileX == 0.f) tileX = 1.0f;
        if (tileY == 0.f) tileY = 1.0f;

        GeometryConfig config{};
        config.vertices.Resize(xSegmentCount * ySegmentCount * 4);
        config.indices.Resize(xSegmentCount * ySegmentCount * 6);

        // TODO: this generates extra vertices, but we can always deduplicate them later
        const f32 segWidth   = width / static_cast<f32>(xSegmentCount);
        const f32 segHeight  = height / static_cast<f32>(ySegmentCount);
        const f32 halfWidth  = width * 0.5f;
        const f32 halfHeight = height * 0.5f;

        for (u32 y = 0; y < ySegmentCount; y++)
        {
            for (u32 x = 0; x < xSegmentCount; x++)
            {
                const f32 fx = static_cast<f32>(x);
                const f32 fy = static_cast<f32>(y);

                const f32 fxSegmentCount = static_cast<f32>(xSegmentCount);
                const f32 fySegmentCount = static_cast<f32>(ySegmentCount);

                // Generate vertices
                const f32 minX   = (fx * segWidth) - halfWidth;
                const f32 minY   = (fy * segHeight) - halfHeight;
                const f32 maxX   = minX + segWidth;
                const f32 maxY   = minY + segHeight;
                const f32 minUvx = (fx / fxSegmentCount) * tileX;
                const f32 minUvy = (fy / fySegmentCount) * tileY;
                const f32 maxUvx = ((fx + 1) / fxSegmentCount) * tileX;
                const f32 maxUvy = ((fy + 1) / fySegmentCount) * tileY;

                const u32 vOffset = (y * xSegmentCount + x) * 4;
                Vertex3D* v0      = &config.vertices[vOffset + 0];
                Vertex3D* v1      = &config.vertices[vOffset + 1];
                Vertex3D* v2      = &config.vertices[vOffset + 2];
                Vertex3D* v3      = &config.vertices[vOffset + 3];

                v0->position.x = minX;
                v0->position.y = minY;
                v0->texture.x  = minUvx;
                v0->texture.y  = minUvy;

                v1->position.x = maxX;
                v1->position.y = maxY;
                v1->texture.x  = maxUvx;
                v1->texture.y  = maxUvy;

                v2->position.x = minX;
                v2->position.y = maxY;
                v2->texture.x  = minUvx;
                v2->texture.y  = maxUvy;

                v3->position.x = maxX;
                v3->position.y = minY;
                v3->texture.x  = maxUvx;
                v3->texture.y  = minUvy;

                // Generate indices
                const u32 iOffset           = (y * xSegmentCount + x) * 6;
                config.indices[iOffset + 0] = vOffset + 0;
                config.indices[iOffset + 1] = vOffset + 1;
                config.indices[iOffset + 2] = vOffset + 2;
                config.indices[iOffset + 3] = vOffset + 0;
                config.indices[iOffset + 4] = vOffset + 3;
                config.indices[iOffset + 5] = vOffset + 1;
            }
        }

        if (!name.Empty())
        {
            config.name = name.Data();
        }
        else
        {
            config.name = DEFAULT_GEOMETRY_NAME;
        }

        if (!materialName.Empty())
        {
            config.materialName = materialName.Data();
        }
        else
        {
            config.materialName = DEFAULT_PBR_MATERIAL_NAME;
        }

        return config;
    }

    GeometryConfig GeometrySystem::GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 tileX, f32 tileY, const String& name,
                                                      const String& materialName)
    {
        if (width == 0.f) width = 1.0f;
        if (height == 0.f) height = 1.0f;
        if (tileX == 0.f) tileX = 1.0f;
        if (tileY == 0.f) tileY = 1.0f;

        GeometryConfig config;
        config.vertices.Resize(4 * 6);  // 4 Vertices per side with 6 sides
        config.indices.Resize(6 * 6);   // 6 indices per side with 6 sides

        const f32 halfWidth  = width * 0.5f;
        const f32 halfHeight = height * 0.5f;
        const f32 halfDepth  = depth * 0.5f;

        f32 minX   = -halfWidth;
        f32 minY   = -halfHeight;
        f32 minZ   = -halfDepth;
        f32 maxX   = halfWidth;
        f32 maxY   = halfHeight;
        f32 maxZ   = halfDepth;
        f32 minUvX = 0.0f;
        f32 minUvY = 0.0f;
        f32 maxUvX = tileX;
        f32 maxUvY = tileY;

        config.minExtents = { minX, minY, minZ };
        config.maxExtents = { maxX, maxY, maxZ };
        config.center     = { 0, 0, 0 };

        // Front face
        config.vertices[(0 * 4) + 0].position = { minX, minY, maxZ };
        config.vertices[(0 * 4) + 1].position = { maxX, maxY, maxZ };
        config.vertices[(0 * 4) + 2].position = { minX, maxY, maxZ };
        config.vertices[(0 * 4) + 3].position = { maxX, minY, maxZ };
        config.vertices[(0 * 4) + 0].texture  = { minUvX, minUvY };
        config.vertices[(0 * 4) + 1].texture  = { maxUvX, maxUvY };
        config.vertices[(0 * 4) + 2].texture  = { minUvX, maxUvY };
        config.vertices[(0 * 4) + 3].texture  = { maxUvX, minUvY };
        config.vertices[(0 * 4) + 0].normal   = { 0.0f, 0.0f, 1.0f };
        config.vertices[(0 * 4) + 1].normal   = { 0.0f, 0.0f, 1.0f };
        config.vertices[(0 * 4) + 2].normal   = { 0.0f, 0.0f, 1.0f };
        config.vertices[(0 * 4) + 3].normal   = { 0.0f, 0.0f, 1.0f };

        // Back face
        config.vertices[(1 * 4) + 0].position = { maxX, minY, minZ };
        config.vertices[(1 * 4) + 1].position = { minX, maxY, minZ };
        config.vertices[(1 * 4) + 2].position = { maxX, maxY, minZ };
        config.vertices[(1 * 4) + 3].position = { minX, minY, minZ };
        config.vertices[(1 * 4) + 0].texture  = { minUvX, minUvY };
        config.vertices[(1 * 4) + 1].texture  = { maxUvX, maxUvY };
        config.vertices[(1 * 4) + 2].texture  = { minUvX, maxUvY };
        config.vertices[(1 * 4) + 3].texture  = { maxUvX, minUvY };
        config.vertices[(1 * 4) + 0].normal   = { 0.0f, 0.0f, -1.0f };
        config.vertices[(1 * 4) + 1].normal   = { 0.0f, 0.0f, -1.0f };
        config.vertices[(1 * 4) + 2].normal   = { 0.0f, 0.0f, -1.0f };
        config.vertices[(1 * 4) + 3].normal   = { 0.0f, 0.0f, -1.0f };

        // Left face
        config.vertices[(2 * 4) + 0].position = { minX, minY, minZ };
        config.vertices[(2 * 4) + 1].position = { minX, maxY, maxZ };
        config.vertices[(2 * 4) + 2].position = { minX, maxY, minZ };
        config.vertices[(2 * 4) + 3].position = { minX, minY, maxZ };
        config.vertices[(2 * 4) + 0].texture  = { minUvX, minUvY };
        config.vertices[(2 * 4) + 1].texture  = { maxUvX, maxUvY };
        config.vertices[(2 * 4) + 2].texture  = { minUvX, maxUvY };
        config.vertices[(2 * 4) + 3].texture  = { maxUvX, minUvY };
        config.vertices[(2 * 4) + 0].normal   = { -1.0f, 0.0f, 0.0f };
        config.vertices[(2 * 4) + 1].normal   = { -1.0f, 0.0f, 0.0f };
        config.vertices[(2 * 4) + 2].normal   = { -1.0f, 0.0f, 0.0f };
        config.vertices[(2 * 4) + 3].normal   = { -1.0f, 0.0f, 0.0f };

        // Right face
        config.vertices[(3 * 4) + 0].position = { maxX, minY, maxZ };
        config.vertices[(3 * 4) + 1].position = { maxX, maxY, minZ };
        config.vertices[(3 * 4) + 2].position = { maxX, maxY, maxZ };
        config.vertices[(3 * 4) + 3].position = { maxX, minY, minZ };
        config.vertices[(3 * 4) + 0].texture  = { minUvX, minUvY };
        config.vertices[(3 * 4) + 1].texture  = { maxUvX, maxUvY };
        config.vertices[(3 * 4) + 2].texture  = { minUvX, maxUvY };
        config.vertices[(3 * 4) + 3].texture  = { maxUvX, minUvY };
        config.vertices[(3 * 4) + 0].normal   = { 1.0f, 0.0f, 0.0f };
        config.vertices[(3 * 4) + 1].normal   = { 1.0f, 0.0f, 0.0f };
        config.vertices[(3 * 4) + 2].normal   = { 1.0f, 0.0f, 0.0f };
        config.vertices[(3 * 4) + 3].normal   = { 1.0f, 0.0f, 0.0f };

        // Bottom face
        config.vertices[(4 * 4) + 0].position = { maxX, minY, maxZ };
        config.vertices[(4 * 4) + 1].position = { minX, minY, minZ };
        config.vertices[(4 * 4) + 2].position = { maxX, minY, minZ };
        config.vertices[(4 * 4) + 3].position = { minX, minY, maxZ };
        config.vertices[(4 * 4) + 0].texture  = { minUvX, minUvY };
        config.vertices[(4 * 4) + 1].texture  = { maxUvX, maxUvY };
        config.vertices[(4 * 4) + 2].texture  = { minUvX, maxUvY };
        config.vertices[(4 * 4) + 3].texture  = { maxUvX, minUvY };
        config.vertices[(4 * 4) + 0].normal   = { 0.0f, -1.0f, 0.0f };
        config.vertices[(4 * 4) + 1].normal   = { 0.0f, -1.0f, 0.0f };
        config.vertices[(4 * 4) + 2].normal   = { 0.0f, -1.0f, 0.0f };
        config.vertices[(4 * 4) + 3].normal   = { 0.0f, -1.0f, 0.0f };

        // Top face
        config.vertices[(5 * 4) + 0].position = { minX, maxY, maxZ };
        config.vertices[(5 * 4) + 1].position = { maxX, maxY, minZ };
        config.vertices[(5 * 4) + 2].position = { minX, maxY, minZ };
        config.vertices[(5 * 4) + 3].position = { maxX, maxY, maxZ };
        config.vertices[(5 * 4) + 0].texture  = { minUvX, minUvY };
        config.vertices[(5 * 4) + 1].texture  = { maxUvX, maxUvY };
        config.vertices[(5 * 4) + 2].texture  = { minUvX, maxUvY };
        config.vertices[(5 * 4) + 3].texture  = { maxUvX, minUvY };
        config.vertices[(5 * 4) + 0].normal   = { 0.0f, 1.0f, 0.0f };
        config.vertices[(5 * 4) + 1].normal   = { 0.0f, 1.0f, 0.0f };
        config.vertices[(5 * 4) + 2].normal   = { 0.0f, 1.0f, 0.0f };
        config.vertices[(5 * 4) + 3].normal   = { 0.0f, 1.0f, 0.0f };

        for (u32 i = 0; i < 6; i++)
        {
            const u32 vOffset = i * 4;
            const u32 iOffset = i * 6;

            config.indices[iOffset + 0] = vOffset + 0;
            config.indices[iOffset + 1] = vOffset + 1;
            config.indices[iOffset + 2] = vOffset + 2;
            config.indices[iOffset + 3] = vOffset + 0;
            config.indices[iOffset + 4] = vOffset + 3;
            config.indices[iOffset + 5] = vOffset + 1;
        }

        if (!name.Empty())
        {
            config.name = name.Data();
        }
        else
        {
            config.name = DEFAULT_GEOMETRY_NAME;
        }

        if (!materialName.Empty())
        {
            config.materialName = materialName.Data();
        }
        else
        {
            config.materialName = DEFAULT_PBR_MATERIAL_NAME;
        }

        GeometryUtils::GenerateTangents(config.vertices, config.indices);
        return config;
    }

    void GeometrySystem::DestroyGeometry(Geometry* g) const
    {
        Renderer.DestroyGeometry(*g);
        g->generation = INVALID_ID_U16;
        g->id         = INVALID_ID;
        g->name.Destroy();

        // Release the material
        if (g->material && !g->material->name.Empty())
        {
            Materials.Release(g->material->name.Data());
            g->material = nullptr;
        }
    }

    bool GeometrySystem::CreateDefaultGeometries()
    {
        // Create default geometry
        constexpr u32 vertexCount      = 4;
        Vertex3D vertices[vertexCount] = {};

        constexpr f32 f = 10.0f;

        vertices[0].position.x = -0.5f * f;
        vertices[0].position.y = -0.5f * f;
        vertices[0].texture.x  = 0.0f;
        vertices[0].texture.y  = 0.0f;

        vertices[1].position.x = 0.5f * f;
        vertices[1].position.y = 0.5f * f;
        vertices[1].texture.x  = 1.0f;
        vertices[1].texture.y  = 1.0f;

        vertices[2].position.x = -0.5f * f;
        vertices[2].position.y = 0.5f * f;
        vertices[2].texture.x  = 0.0f;
        vertices[2].texture.y  = 1.0f;

        vertices[3].position.x = 0.5f * f;
        vertices[3].position.y = -0.5f * f;
        vertices[3].texture.x  = 1.0f;
        vertices[3].texture.y  = 0.0f;

        constexpr u32 indexCount      = 6;
        const u32 indices[indexCount] = { 0, 1, 2, 0, 3, 1 };

        if (!Renderer.CreateGeometry(m_defaultGeometry, sizeof(Vertex3D), vertexCount, vertices, sizeof(u32), indexCount, indices))
        {
            FATAL_LOG("Failed to create default geometry.");
            return false;
        }

        if (!Renderer.UploadGeometry(m_defaultGeometry))
        {
            FATAL_LOG("Failed to upload default geometry.");
            return false;
        }

        // Acquire the default material
        m_defaultGeometry.material = Materials.GetDefault();

        return true;
    }
}  // namespace C3D
