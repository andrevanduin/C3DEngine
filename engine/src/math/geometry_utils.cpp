
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

    UIGeometryConfig GenerateUIQuadConfig(const char* name, f32 width, f32 height, f32 uMin, f32 uMax, f32 vMin, f32 vMax)
    {
        UIGeometryConfig config = {};
        config.vertices.Resize(4);
        config.indices.Resize(6);

        config.materialName = "";
        config.name         = name;

        // Top left corner
        config.vertices[0].position.x = 0.0f;
        config.vertices[0].position.y = 0.0f;
        config.vertices[0].texture.x  = uMin;
        config.vertices[0].texture.y  = vMin;

        config.vertices[1].position.x = width;
        config.vertices[1].position.y = height;
        config.vertices[1].texture.x  = uMax;
        config.vertices[1].texture.y  = vMax;

        config.vertices[2].position.x = 0.0f;
        config.vertices[2].position.y = height;
        config.vertices[2].texture.x  = uMin;
        config.vertices[2].texture.y  = vMax;

        config.vertices[3].position.x = width;
        config.vertices[3].position.y = 0.0f;
        config.vertices[3].texture.x  = uMax;
        config.vertices[3].texture.y  = vMin;

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
        /** Create a geometry config for our nine slice which will look as follows
         * The 9 different quads are from here on refered to by the letters shown below.
         *
         *  @ = vertex
         *  - and | = edge
         *
         * @ - @ - - - @ - @
         * | A |   B   | C |
         * @ - @ - - - @ - @
         * | D |   E   | F |
         * |   |       |   |
         * @ - @ - - - @ - @
         * | G |   H   | I |
         * @ - @ - - - @ - @
         */
        UIGeometryConfig config = {};
        config.vertices.Resize(9 * 4);
        config.indices.Resize(9 * 6);

        config.materialName = "";
        config.name         = name;

        // UV Coordinates

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
        const f32 aVMin = atlasMinV;
        const f32 aUMax = atlasMinU + cornerAtlasSizeU;
        const f32 aVMax = atlasMinV + cornerAtlasSizeV;

        // Corner C
        const f32 cUMin = atlasMaxU - cornerAtlasSizeU;
        const f32 cVMin = atlasMinV;
        const f32 cUMax = atlasMaxU;
        const f32 cVMax = atlasMinV + cornerAtlasSizeV;

        // B
        const f32 bUMin = aUMax;
        const f32 bVMin = atlasMinV;
        const f32 bUMax = cUMin;
        const f32 bVMax = atlasMinV + cornerAtlasSizeV;

        // D
        const f32 dUMin = atlasMinU;
        const f32 dVMin = atlasMinV + cornerAtlasSizeV;
        const f32 dUMax = atlasMinU + cornerAtlasSizeU;
        const f32 dVMax = atlasMaxV - cornerAtlasSizeV;

        // E
        const f32 eUMin = cornerAtlasSizeU;
        const f32 eVMin = cornerAtlasSizeV;
        const f32 eUMax = atlasMaxU - cornerAtlasSizeU;
        const f32 eVMax = atlasMaxV - cornerAtlasSizeV;

        // F
        const f32 fUMin = atlasMaxU - cornerAtlasSizeU;
        const f32 fVMin = cornerAtlasSizeV;
        const f32 fUMax = atlasMaxU;
        const f32 fVMax = atlasMaxV - cornerAtlasSizeV;

        // G
        const f32 gUMin = atlasMinU;
        const f32 gVMin = atlasMaxV - cornerAtlasSizeV;
        const f32 gUMax = atlasMinU + cornerAtlasSizeU;
        const f32 gVMax = atlasMaxV;

        // H
        const f32 hUMin = cornerAtlasSizeU;
        const f32 hVMin = atlasMaxV - cornerAtlasSizeV;
        const f32 hUMax = atlasMaxU - cornerAtlasSizeU;
        const f32 hVMax = atlasMaxV;

        // I
        const f32 iUMin = atlasMaxU - cornerAtlasSizeU;
        const f32 iVMin = atlasMaxV - cornerAtlasSizeV;
        const f32 iUMax = atlasMaxU;
        const f32 iVMax = atlasMaxV;

        // B and H quad sizes
        const f32 bhWidth  = static_cast<f32>(size.x) - (cornerSize.x * 2.0f);
        const f32 bhHeight = cornerSize.y;

        // D and F quad sizes
        const f32 dfWidth  = cornerSize.x;
        const f32 dfHeight = static_cast<f32>(size.y) - (cornerSize.y * 2.0f);

        // A
        config.vertices[0].position.x = 0.0f;
        config.vertices[0].position.y = 0.0f;
        config.vertices[0].texture.x  = aUMin;
        config.vertices[0].texture.y  = aVMin;

        config.vertices[1].position.x = cornerSize.x;
        config.vertices[1].position.y = cornerSize.y;
        config.vertices[1].texture.x  = aUMax;
        config.vertices[1].texture.y  = aVMax;

        config.vertices[2].position.x = 0.0f;
        config.vertices[2].position.y = cornerSize.y;
        config.vertices[2].texture.x  = aUMin;
        config.vertices[2].texture.y  = aVMax;

        config.vertices[3].position.x = cornerSize.x;
        config.vertices[3].position.y = 0.0f;
        config.vertices[3].texture.x  = aUMax;
        config.vertices[3].texture.y  = aVMin;

        // B
        config.vertices[4].position.x = cornerSize.x;
        config.vertices[4].position.y = 0.0f;
        config.vertices[4].texture.x  = bUMin;
        config.vertices[4].texture.y  = bVMin;

        config.vertices[5].position.x = cornerSize.x + bhWidth;
        config.vertices[5].position.y = bhHeight;
        config.vertices[5].texture.x  = bUMax;
        config.vertices[5].texture.y  = bVMax;

        config.vertices[6].position.x = cornerSize.x;
        config.vertices[6].position.y = bhHeight;
        config.vertices[6].texture.x  = bUMin;
        config.vertices[6].texture.y  = bVMax;

        config.vertices[7].position.x = cornerSize.x + bhWidth;
        config.vertices[7].position.y = 0.0f;
        config.vertices[7].texture.x  = bUMax;
        config.vertices[7].texture.y  = bVMin;

        // C
        config.vertices[8].position.x = size.x - cornerSize.x;
        config.vertices[8].position.y = 0.0f;
        config.vertices[8].texture.x  = cUMin;
        config.vertices[8].texture.y  = cVMin;

        config.vertices[9].position.x = size.x;
        config.vertices[9].position.y = cornerSize.y;
        config.vertices[9].texture.x  = cUMax;
        config.vertices[9].texture.y  = cVMax;

        config.vertices[10].position.x = size.x - cornerSize.x;
        config.vertices[10].position.y = cornerSize.y;
        config.vertices[10].texture.x  = cUMin;
        config.vertices[10].texture.y  = cVMax;

        config.vertices[11].position.x = size.x;
        config.vertices[11].position.y = 0.0f;
        config.vertices[11].texture.x  = cUMax;
        config.vertices[11].texture.y  = cVMin;

        // D
        config.vertices[12].position.x = 0.0f;
        config.vertices[12].position.y = cornerSize.y;
        config.vertices[12].texture.x  = dUMin;
        config.vertices[12].texture.y  = dVMin;

        config.vertices[13].position.x = dfWidth;
        config.vertices[13].position.y = cornerSize.y + dfHeight;
        config.vertices[13].texture.x  = dUMax;
        config.vertices[13].texture.y  = dVMax;

        config.vertices[14].position.x = 0.0f;
        config.vertices[14].position.y = cornerSize.y + dfHeight;
        config.vertices[14].texture.x  = dUMin;
        config.vertices[14].texture.y  = dVMax;

        config.vertices[15].position.x = dfWidth;
        config.vertices[15].position.y = cornerSize.y;
        config.vertices[15].texture.x  = dUMax;
        config.vertices[15].texture.y  = dVMin;

        // E
        config.vertices[16].position.x = cornerSize.x;
        config.vertices[16].position.y = cornerSize.y;
        config.vertices[16].texture.x  = eUMin;
        config.vertices[16].texture.y  = eVMin;

        config.vertices[17].position.x = cornerSize.x + bhWidth;
        config.vertices[17].position.y = cornerSize.y + dfHeight;
        config.vertices[17].texture.x  = eUMax;
        config.vertices[17].texture.y  = eVMax;

        config.vertices[18].position.x = cornerSize.x;
        config.vertices[18].position.y = cornerSize.y + dfHeight;
        config.vertices[18].texture.x  = eUMin;
        config.vertices[18].texture.y  = eVMax;

        config.vertices[19].position.x = cornerSize.x + bhWidth;
        config.vertices[19].position.y = cornerSize.y;
        config.vertices[19].texture.x  = eUMax;
        config.vertices[19].texture.y  = eVMin;

        // F
        config.vertices[20].position.x = size.x - cornerSize.x;
        config.vertices[20].position.y = cornerSize.y;
        config.vertices[20].texture.x  = fUMin;
        config.vertices[20].texture.y  = fVMin;

        config.vertices[21].position.x = size.x;
        config.vertices[21].position.y = cornerSize.y + dfHeight;
        config.vertices[21].texture.x  = fUMax;
        config.vertices[21].texture.y  = fVMax;

        config.vertices[22].position.x = size.x - cornerSize.x;
        config.vertices[22].position.y = cornerSize.y + dfHeight;
        config.vertices[22].texture.x  = fUMin;
        config.vertices[22].texture.y  = fVMax;

        config.vertices[23].position.x = size.x;
        config.vertices[23].position.y = cornerSize.y;
        config.vertices[23].texture.x  = fUMax;
        config.vertices[23].texture.y  = fVMin;

        // G
        config.vertices[24].position.x = 0.0f;
        config.vertices[24].position.y = size.y - cornerSize.y;
        config.vertices[24].texture.x  = gUMin;
        config.vertices[24].texture.y  = gVMin;

        config.vertices[25].position.x = cornerSize.x;
        config.vertices[25].position.y = size.y;
        config.vertices[25].texture.x  = gUMax;
        config.vertices[25].texture.y  = gVMax;

        config.vertices[26].position.x = 0.0f;
        config.vertices[26].position.y = size.y;
        config.vertices[26].texture.x  = gUMin;
        config.vertices[26].texture.y  = gVMax;

        config.vertices[27].position.x = cornerSize.x;
        config.vertices[27].position.y = size.y - cornerSize.y;
        config.vertices[27].texture.x  = gUMax;
        config.vertices[27].texture.y  = gVMin;

        // H
        config.vertices[28].position.x = cornerSize.x;
        config.vertices[28].position.y = size.y - cornerSize.y;
        config.vertices[28].texture.x  = hUMin;
        config.vertices[28].texture.y  = hVMin;

        config.vertices[29].position.x = size.x - cornerSize.x;
        config.vertices[29].position.y = size.y;
        config.vertices[29].texture.x  = hUMax;
        config.vertices[29].texture.y  = hVMax;

        config.vertices[30].position.x = cornerSize.x;
        config.vertices[30].position.y = size.y;
        config.vertices[30].texture.x  = hUMin;
        config.vertices[30].texture.y  = hVMax;

        config.vertices[31].position.x = size.x - cornerSize.x;
        config.vertices[31].position.y = size.y - cornerSize.y;
        config.vertices[31].texture.x  = hUMax;
        config.vertices[31].texture.y  = hVMin;

        // I
        config.vertices[32].position.x = size.x - cornerSize.x;
        config.vertices[32].position.y = size.y - cornerSize.y;
        config.vertices[32].texture.x  = iUMin;
        config.vertices[32].texture.y  = iVMin;

        config.vertices[33].position.x = size.x;
        config.vertices[33].position.y = size.y;
        config.vertices[33].texture.x  = iUMax;
        config.vertices[33].texture.y  = iVMax;

        config.vertices[34].position.x = size.x - cornerSize.x;
        config.vertices[34].position.y = size.y;
        config.vertices[34].texture.x  = iUMin;
        config.vertices[34].texture.y  = iVMax;

        config.vertices[35].position.x = size.x;
        config.vertices[35].position.y = size.y - cornerSize.y;
        config.vertices[35].texture.x  = iUMax;
        config.vertices[35].texture.y  = iVMin;

        for (u8 i = 0; i <= 32; i += 4)
        {
            // Counter-clockwise
            config.indices.PushBack(2 + i);
            config.indices.PushBack(1 + i);
            config.indices.PushBack(0 + i);
            config.indices.PushBack(3 + i);
            config.indices.PushBack(0 + i);
            config.indices.PushBack(1 + i);
        }

        return config;
    }
}  // namespace C3D::GeometryUtils
