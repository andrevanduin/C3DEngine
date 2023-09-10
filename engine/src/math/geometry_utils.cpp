
#include "geometry_utils.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>

#include "math/c3d_math.h"
#include "systems/system_manager.h"

namespace C3D::GeometryUtils
{
    bool Vertex3DEqual(const Vertex3D& vert0, const Vertex3D& vert1)
    {
        return EpsilonEqual(vert0.position, vert1.position, FLOAT_EPSILON) &&
               EpsilonEqual(vert0.normal, vert1.normal, FLOAT_EPSILON) &&
               EpsilonEqual(vert0.texture, vert1.texture, FLOAT_EPSILON) &&
               EpsilonEqual(vert0.color, vert1.color, FLOAT_EPSILON) &&
               EpsilonEqual(vert0.tangent, vert1.tangent, FLOAT_EPSILON);
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
        Memory.Free(MemoryType::Array, uniqueVertices);

        u64 removedCount = oldVertexCount - uniqueVertexCount;
        Logger::Debug("GeometryUtils::DeduplicateVertices() - removed {} vertices, Originally: {} | Now: {}",
                      removedCount, oldVertexCount, uniqueVertexCount);
    }
}  // namespace C3D::GeometryUtils
