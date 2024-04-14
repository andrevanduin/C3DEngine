
#include "geometry_utils.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>

#include "math/c3d_math.h"
#include "systems/system_manager.h"

namespace C3D::GeometryUtils
{
    void GenerateTangents(DynamicArray<Vertex3D>& vertices, const DynamicArray<u32>& indices)
    {
        for (u32 i = 0; i < indices.Size(); i += 3)
        {
            const u32 i0 = indices[i + 0];
            const u32 i1 = indices[i + 1];
            const u32 i2 = indices[i + 2];

            const vec3 edge1 = vertices[i1].position - vertices[i0].position;
            const vec3 edge2 = vertices[i2].position - vertices[i0].position;

            const f32 deltaU1 = vertices[i1].texture.x - vertices[i0].texture.x;
            const f32 deltaV1 = vertices[i1].texture.y - vertices[i0].texture.y;

            const f32 deltaU2 = vertices[i2].texture.x - vertices[i0].texture.x;
            const f32 deltaV2 = vertices[i2].texture.y - vertices[i0].texture.y;

            const f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
            const f32 fc       = 1.0f / dividend;

            vec3 tangent = {
                fc * (deltaV2 * edge1.x - deltaV1 * edge2.x),
                fc * (deltaV2 * edge1.y - deltaV1 * edge2.y),
                fc * (deltaV2 * edge1.z - deltaV1 * edge2.z),
            };

            tangent = normalize(tangent);

            const f32 sx = deltaU1, sy = deltaU2;
            const f32 tx = deltaV1, ty = deltaV2;
            const f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

            const auto t3        = tangent * handedness;
            vertices[i0].tangent = t3;
            vertices[i1].tangent = t3;
            vertices[i2].tangent = t3;
        }
    }

    void GenerateTerrainTangents(DynamicArray<TerrainVertex>& vertices, const DynamicArray<u32>& indices)
    {
        for (u32 i = 0; i < indices.Size(); i += 3)
        {
            const u32 i0 = indices[i + 0];
            const u32 i1 = indices[i + 1];
            const u32 i2 = indices[i + 2];

            const vec3 edge1 = vertices[i1].position - vertices[i0].position;
            const vec3 edge2 = vertices[i2].position - vertices[i0].position;

            const f32 deltaU1 = vertices[i1].texture.x - vertices[i0].texture.x;
            const f32 deltaV1 = vertices[i1].texture.y - vertices[i0].texture.y;

            const f32 deltaU2 = vertices[i2].texture.x - vertices[i0].texture.x;
            const f32 deltaV2 = vertices[i2].texture.y - vertices[i0].texture.y;

            const f32 dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);
            const f32 fc       = 1.0f / dividend;

            vec3 tangent = {
                fc * (deltaV2 * edge1.x - deltaV1 * edge2.x),
                fc * (deltaV2 * edge1.y - deltaV1 * edge2.y),
                fc * (deltaV2 * edge1.z - deltaV1 * edge2.z),
            };

            tangent = normalize(tangent);

            const f32 sx = deltaU1, sy = deltaU2;
            const f32 tx = deltaV1, ty = deltaV2;
            const f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

            const auto t3        = tangent * handedness;
            vertices[i0].tangent = vec4(t3, 0.0f);
            vertices[i1].tangent = vec4(t3, 0.0f);
            vertices[i2].tangent = vec4(t3, 0.0f);
        }
    }

    bool Vertex3DEqual(const Vertex3D& vert0, const Vertex3D& vert1)
    {
        return EpsilonEqual(vert0.position, vert1.position, F32_EPSILON) && EpsilonEqual(vert0.normal, vert1.normal, F32_EPSILON) &&
               EpsilonEqual(vert0.texture, vert1.texture, F32_EPSILON) && EpsilonEqual(vert0.color, vert1.color, F32_EPSILON) &&
               EpsilonEqual(vert0.tangent, vert1.tangent, F32_EPSILON);
    }

    void ReassignIndex(const DynamicArray<u32>& indices, const u32 from, const u32 to)
    {
        for (auto& index : indices)
        {
            if (index == from)
            {
                index = to;
            }
            else if (index > from)
            {
                // All indices higher than 'from' need to be decremented by 1
                index--;
            }
        }
    }

    void DeduplicateVertices(GeometryConfig& config)
    {
        // Store the unique vertex count
        u64 uniqueVertexCount = 0;
        // Store the current vertex count
        const u64 oldVertexCount = config.vertices.Size();
        // Allocate enough memory for the worst case where every vertex is unique
        auto* uniqueVertices = Memory.Allocate<Vertex3D>(MemoryType::Array, oldVertexCount);

        u32 foundCount = 0;
        for (u32 v = 0; v < oldVertexCount; v++)
        {
            bool found = false;
            for (u32 u = 0; u < uniqueVertexCount; u++)
            {
                if (Vertex3DEqual(config.vertices[v], uniqueVertices[u]))
                {
                    // We have found a match so we simply use the indices and we do not copy over the vertex
                    ReassignIndex(config.indices, v - foundCount, u);
                    found = true;
                    foundCount++;
                    break;
                }
            }

            // We have not found a match so we copy the vertex over
            if (!found)
            {
                uniqueVertices[uniqueVertexCount] = config.vertices[v];
                uniqueVertexCount++;
            }
        }

        // Copy over the unique vertices (resizing the dynamic array to fit the smaller amount)
        config.vertices.Copy(uniqueVertices, uniqueVertexCount);
        // Destroy our temporary array
        Memory.Free(uniqueVertices);

        u64 removedCount = oldVertexCount - uniqueVertexCount;
        Logger::Debug("GeometryUtils::DeduplicateVertices() - removed {} vertices, Originally: {} | Now: {}", removedCount, oldVertexCount,
                      uniqueVertexCount);
    }

    UIGeometryConfig GenerateUIQuadConfig(const char* name, const u16vec2& size, const u16vec2& atlasSize, const u16vec2& atlasMin,
                                          const u16vec2& atlasMax)
    {
        UIGeometryConfig config = {};
        config.vertices.Resize(4);
        config.indices.Reserve(6);

        config.materialName = "";
        config.name         = name;

        RegenerateUIQuadGeometry(config.vertices.GetData(), size, atlasSize, atlasMin, atlasMax);

        // Counter-clockwise
        config.indices.PushBack(2);
        config.indices.PushBack(1);
        config.indices.PushBack(0);
        config.indices.PushBack(3);
        config.indices.PushBack(0);
        config.indices.PushBack(1);

        return config;
    }

    UIGeometryConfig GenerateUINineSliceConfig(const char* name, const u16vec2& size, const u16vec2& cornerSize, const u16vec2& atlasSize,
                                               const u16vec2& cornerAtlasSize, const u16vec2& atlasMin, const u16vec2& atlasMax)
    {
        // 4 Vertices per corner with 4 corners
        // 6 indices per quad and nine quads in a nine slice
        UIGeometryConfig config = {};
        config.vertices.Resize(16);
        config.indices.Reserve(9 * 6);

        config.materialName = "";
        config.name         = name;

        RegenerateUINineSliceGeometry(config.vertices.GetData(), size, cornerSize, atlasSize, cornerAtlasSize, atlasMin, atlasMax);

        /*
        Indices:
            00, 04, 05 and 01, 00, 05
            01, 05, 06 and 02, 01, 06
            02, 06, 07 and 03, 02, 07

            04, 08, 09 and 05, 04, 09
            05, 09, 10 and 06, 05, 10
            06, 10, 11 and 07, 06, 11

            08, 12, 13 and 09, 08, 13
            09, 13, 14 and 10, 09, 14
            10, 14, 15 and 11, 10, 15
        */

        for (u8 j = 0; j <= 8; j += 4)
        {
            for (u8 i = 0; i < 3; i++)
            {
                config.indices.PushBack(0 + j + i);
                config.indices.PushBack(4 + j + i);
                config.indices.PushBack(5 + j + i);
                config.indices.PushBack(1 + j + i);
                config.indices.PushBack(0 + j + i);
                config.indices.PushBack(5 + j + i);
            }
        }

        return config;
    }

    void RegenerateUINineSliceGeometry(Vertex2D* vertices, const u16vec2& size, const u16vec2& cornerSize, const u16vec2& atlasSize,
                                       const u16vec2& cornerAtlasSize, const u16vec2& atlasMin, const u16vec2& atlasMax)
    {
        /** Create a geometry config for our nine slice which will look as follows
         * The 9 different quads are from here on refered to by the letters shown below.
         *
         *  01       == vertex
         *  - and || == edge
         *
         * 00 - 01 - - - 02 - 03
         * || A ||   B   || C ||
         * 04 - 05 - - - 06 - 07
         * ||   ||       ||   ||
         * || D ||   E   || F ||
         * ||   ||       ||   ||
         * 08 - 09 - - - 10 - 11
         * || G ||   H   || I ||
         * 12 - 13 - - - 14 - 15
         */

        // Min UV coordinates we will use in the atlas
        const f32 atlasMinU = static_cast<f32>(atlasMin.x) / atlasSize.x;
        const f32 atlasMinV = static_cast<f32>(atlasMin.y) / atlasSize.y;

        // Max UV coordinates we will use in the atlas
        const f32 atlasMaxU = static_cast<f32>(atlasMax.x) / atlasSize.x;
        const f32 atlasMaxV = static_cast<f32>(atlasMax.y) / atlasSize.y;

        // Size of a corner in uv space
        const f32 cornerAtlasSizeU = static_cast<f32>(cornerAtlasSize.x) / atlasSize.x;
        const f32 cornerAtlasSizeV = static_cast<f32>(cornerAtlasSize.y) / atlasSize.y;

        // Corner A
        const f32 aUMin = atlasMinU;
        const f32 aUMax = atlasMinU + cornerAtlasSizeU;

        // Corner C
        const f32 cUMin = atlasMaxU - cornerAtlasSizeU;
        const f32 cUMax = atlasMaxU;

        // Corner A + C
        const f32 acVMin = atlasMinV;
        const f32 acVMax = atlasMinV + cornerAtlasSizeV;

        // Corner G
        const f32 gUMin = atlasMinU;
        const f32 gUMax = atlasMinU + cornerAtlasSizeU;

        // Corner I
        const f32 iUMin = atlasMaxU - cornerAtlasSizeU;
        const f32 iUMax = atlasMaxU;

        // Corner G + I
        const f32 giVMin = atlasMaxV - cornerAtlasSizeV;
        const f32 giVMax = atlasMaxV;

        vertices[0].position.x = 0.0f;
        vertices[0].position.y = 0.0f;
        vertices[0].texture.x  = aUMin;
        vertices[0].texture.y  = acVMin;

        vertices[1].position.x = cornerSize.x;
        vertices[1].position.y = 0.0f;
        vertices[1].texture.x  = aUMax;
        vertices[1].texture.y  = acVMin;

        vertices[2].position.x = size.x - cornerSize.x;
        vertices[2].position.y = 0.0f;
        vertices[2].texture.x  = cUMin;
        vertices[2].texture.y  = acVMin;

        vertices[3].position.x = size.x;
        vertices[3].position.y = 0.0f;
        vertices[3].texture.x  = cUMax;
        vertices[3].texture.y  = acVMin;

        vertices[4].position.x = 0.0f;
        vertices[4].position.y = cornerSize.y;
        vertices[4].texture.x  = aUMin;
        vertices[4].texture.y  = acVMax;

        vertices[5].position.x = cornerSize.x;
        vertices[5].position.y = cornerSize.y;
        vertices[5].texture.x  = aUMax;
        vertices[5].texture.y  = acVMax;

        vertices[6].position.x = size.x - cornerSize.x;
        vertices[6].position.y = cornerSize.y;
        vertices[6].texture.x  = cUMin;
        vertices[6].texture.y  = acVMax;

        vertices[7].position.x = size.x;
        vertices[7].position.y = cornerSize.y;
        vertices[7].texture.x  = cUMax;
        vertices[7].texture.y  = acVMax;

        vertices[8].position.x = 0.0f;
        vertices[8].position.y = size.y - cornerSize.y;
        vertices[8].texture.x  = gUMin;
        vertices[8].texture.y  = giVMin;

        vertices[9].position.x = cornerSize.x;
        vertices[9].position.y = size.y - cornerSize.y;
        vertices[9].texture.x  = gUMax;
        vertices[9].texture.y  = giVMin;

        vertices[10].position.x = size.x - cornerSize.x;
        vertices[10].position.y = size.y - cornerSize.y;
        vertices[10].texture.x  = iUMin;
        vertices[10].texture.y  = giVMin;

        vertices[11].position.x = size.x;
        vertices[11].position.y = size.y - cornerSize.y;
        vertices[11].texture.x  = iUMax;
        vertices[11].texture.y  = giVMin;

        vertices[12].position.x = 0.0f;
        vertices[12].position.y = size.y;
        vertices[12].texture.x  = gUMin;
        vertices[12].texture.y  = giVMax;

        vertices[13].position.x = cornerSize.x;
        vertices[13].position.y = size.y;
        vertices[13].texture.x  = gUMax;
        vertices[13].texture.y  = giVMax;

        vertices[14].position.x = size.x - cornerSize.x;
        vertices[14].position.y = size.y;
        vertices[14].texture.x  = iUMin;
        vertices[14].texture.y  = giVMax;

        vertices[15].position.x = size.x;
        vertices[15].position.y = size.y;
        vertices[15].texture.x  = iUMax;
        vertices[15].texture.y  = giVMax;
    }

    void RegenerateUIQuadGeometry(Vertex2D* vertices, const u16vec2& size, const u16vec2& atlasSize, const u16vec2& atlasMin,
                                  const u16vec2& atlasMax)
    {
        f32 uMin = static_cast<f32>(atlasMin.x) / atlasSize.x;
        f32 uMax = static_cast<f32>(atlasMax.x) / atlasSize.x;
        f32 vMin = static_cast<f32>(atlasMin.y) / atlasSize.y;
        f32 vMax = static_cast<f32>(atlasMax.y) / atlasSize.y;

        vertices[0].position.x = 0.0f;
        vertices[0].position.y = 0.0f;
        vertices[0].texture.x  = uMin;
        vertices[0].texture.y  = vMin;

        vertices[1].position.x = size.x;
        vertices[1].position.y = size.y;
        vertices[1].texture.x  = uMax;
        vertices[1].texture.y  = vMax;

        vertices[2].position.x = 0.0f;
        vertices[2].position.y = size.y;
        vertices[2].texture.x  = uMin;
        vertices[2].texture.y  = vMax;

        vertices[3].position.x = size.x;
        vertices[3].position.y = 0.0f;
        vertices[3].texture.x  = uMax;
        vertices[3].texture.y  = vMin;
    }
}  // namespace C3D::GeometryUtils
