
#include "geometry_utils.h"

#include "services/system_manager.h"
#include "math/c3d_math.h"

#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>

namespace C3D::GeometryUtils
{
	void GenerateNormals(Vertex3D* vertices, const u32 indexCount, const u32* indices)
	{
		for (u32 i = 0; i < indexCount; i += 3)
		{
			const u32 i0 = indices[i + 0];
			const u32 i1 = indices[i + 1];
			const u32 i2 = indices[i + 2];

			const vec3 edge1 = vertices[i1].position - vertices[i0].position;
			const vec3 edge2 = vertices[i2].position - vertices[i0].position;

			const vec3 normal = normalize(cross(edge1, edge2));

			// NOTE: This is a simple surface normal. Smoothing out should be done separately if required.
			vertices[i0].normal = normal;
			vertices[i1].normal = normal;
			vertices[i2].normal = normal;
		}
	}

	void GenerateTangents(Vertex3D* vertices, const u64 indexCount, const u32* indices)
	{
		for (u32 i = 0; i < indexCount; i += 3)
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
			const f32 fc = 1.0f / dividend;

			vec3 tangent =
			{
				fc * (deltaV2 * edge1.x - deltaV1 * edge2.x),
				fc * (deltaV2 * edge1.y - deltaV1 * edge2.y),
				fc * (deltaV2 * edge1.z - deltaV1 * edge2.z),
			};

			tangent = normalize(tangent);

			const f32 sx = deltaU1, sy = deltaU2;
			const f32 tx = deltaV1, ty = deltaV2;
			const f32 handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

			const auto t3 = tangent * handedness;
			vertices[i0].tangent = t3;
			vertices[i1].tangent = t3;
			vertices[i2].tangent = t3;
		}
	}

	bool Vertex3DEqual(const Vertex3D& vert0, const Vertex3D& vert1)
	{
		return EpsilonEqual(vert0.position, vert1.position, FLOAT_EPSILON) &&
			EpsilonEqual(vert0.normal, vert1.normal, FLOAT_EPSILON) &&
			EpsilonEqual(vert0.texture, vert1.texture, FLOAT_EPSILON) &&
			EpsilonEqual(vert0.color, vert1.color, FLOAT_EPSILON) &&
			EpsilonEqual(vert0.tangent, vert1.tangent, FLOAT_EPSILON);
	}

	/*
	bool Vertex3DEqual(const Vertex3D& vert0, const Vertex3D& vert1)
	{
		return	all(epsilonEqual(vert0.position, vert1.position, FLOAT_EPSILON)) &&
			all(epsilonEqual(vert0.normal, vert1.normal, FLOAT_EPSILON)) &&
			all(epsilonEqual(vert0.texture, vert1.texture, FLOAT_EPSILON)) &&
			all(epsilonEqual(vert0.color, vert1.color, FLOAT_EPSILON)) &&
			all(epsilonEqual(vert0.tangent, vert1.tangent, FLOAT_EPSILON));
	}*/

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

	/*
	void DeduplicateVertices(GeometryConfig<Vertex3D, u32>& config)
	{
		std::unordered_map<Vertex3D, u32> uniqueVertices;
		const auto oldVertexCount = config.vertices.Size();

		uniqueVertices.reserve(oldVertexCount);
		auto* newVertices = Memory.Allocate<Vertex3D>(oldVertexCount, MemoryType::Array);
		auto* newIndices = Memory.Allocate<u32>(oldVertexCount, MemoryType::Array);

		for (u32 i = 0; i < config.vertices.Size(); i++)
		{
			auto& vertex = config.vertices[i];
			const auto index = config.indices[i];

			if (uniqueVertices.find(vertex) == uniqueVertices.end())
			{
				newVertices[uniqueVertices.size()] = vertex;
				uniqueVertices[vertex] = index;
				newIndices[i] = index;
			}
			else
			{
				newIndices[i] = uniqueVertices[vertex];
			}
		}

		config.vertices.Copy(newVertices, uniqueVertices.size());
		config.indices.Copy(newIndices, config.indices.Size());

		Memory.Free(newVertices, oldVertexCount * sizeof(Vertex3D), MemoryType::Array);
		Memory.Free(newIndices, oldVertexCount * sizeof(u32), MemoryType::Array);

		u64 removedCount = oldVertexCount - uniqueVertices.size();
		Logger::Debug("GeometryUtils::DeduplicateVertices() - removed {} vertices, Originally: {} | Now: {}", removedCount, oldVertexCount, uniqueVertices.size());
	}*/

	void DeduplicateVertices(GeometryConfig<Vertex3D, u32>& config)
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
		Logger::Debug("GeometryUtils::DeduplicateVertices() - removed {} vertices, Originally: {} | Now: {}", removedCount, oldVertexCount, uniqueVertexCount);
	}
}
