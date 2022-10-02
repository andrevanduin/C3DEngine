
#pragma once
#include "material_system.h"
#include "containers/dynamic_array.h"
#include "core/defines.h"
#include "core/logger.h"
#include "renderer/renderer_frontend.h"
#include "renderer/vertex.h"
#include "resources/geometry.h"

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

	template<typename VertexType, typename IndexType>
	struct GeometryConfig
	{
		DynamicArray<VertexType> vertices;
		DynamicArray<IndexType> indices;

		vec3 center;
		vec3 minExtents;
		vec3 maxExtents;

		char name[GEOMETRY_NAME_MAX_LENGTH];
		char materialName[MATERIAL_NAME_MAX_LENGTH];

		[[nodiscard]] static constexpr u32 GetVertexSize() { return sizeof(VertexType); }
		[[nodiscard]] static constexpr u32 GetIndexSize()  { return sizeof(IndexType);  }
	};

	struct GeometryReference
	{
		u64 referenceCount;
		Geometry geometry;
		bool autoRelease;
	};

	class GeometrySystem
	{
	public:
		GeometrySystem();

		bool Init(const GeometrySystemConfig& config);
		void Shutdown() const;

		[[nodiscard]] Geometry* AcquireById(u32 id) const;

		template<typename VertexType, typename IndexType>
		[[nodiscard]] Geometry* AcquireFromConfig(const GeometryConfig<VertexType, IndexType>& config, bool autoRelease) const;

		template<typename VertexType, typename IndexType>
		static void DisposeConfig(GeometryConfig<VertexType, IndexType>* config);

		void Release(const Geometry* geometry) const;

		Geometry* GetDefault();
		Geometry* GetDefault2D();

		// NOTE: Vertex and index arrays are dynamically allocated so they should be freed by the user
		[[nodiscard]] static GeometryConfig<Vertex3D, u32> GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 tileX, f32 tileY, const string& name, const string& materialName);

		[[nodiscard]] static GeometryConfig<Vertex3D, u32> GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 tileX, f32 tileY, const string& name, const string& materialName);

	private:
		template<typename VertexType, typename IndexType>
		bool CreateGeometry(const GeometryConfig<VertexType, IndexType>& config, Geometry* g) const;

		static void DestroyGeometry(Geometry* g);

		bool CreateDefaultGeometries();

		LoggerInstance m_logger;

		bool m_initialized;

		GeometrySystemConfig m_config;

		Geometry m_defaultGeometry;
		Geometry m_default2DGeometry;

		GeometryReference* m_registeredGeometries;
	};

	template <typename VertexType, typename IndexType>
	Geometry* GeometrySystem::AcquireFromConfig(const GeometryConfig<VertexType, IndexType>& config, const bool autoRelease) const
	{
		Geometry* g = nullptr;
		for (u32 i = 0; i < m_config.maxGeometryCount; i++)
		{
			if (m_registeredGeometries[i].geometry.id == INVALID_ID)
			{
				// We found an empty slot
				m_registeredGeometries[i].autoRelease = autoRelease;
				m_registeredGeometries[i].referenceCount = 1;

				g = &m_registeredGeometries[i].geometry;
				g->id = i;
				break;
			}
		}

		if (!g)
		{
			m_logger.Error("Unable to obtain free slot for geometry. Adjust config to allow for more space. Returning nullptr");
			return nullptr;
		}

		if (!CreateGeometry(config, g))
		{
			m_logger.Error("Failed to create geometry. Returning nullptr");
			return nullptr;
		}

		return g;
	}

	template <typename VertexType, typename IndexType>
	void GeometrySystem::DisposeConfig(GeometryConfig<VertexType, IndexType>* config)
	{
		config->vertices.Destroy();
		config->indices.Destroy();
	}

	template <typename VertexType, typename IndexType>
	bool GeometrySystem::CreateGeometry(const GeometryConfig<VertexType, IndexType>& config, Geometry* g) const
	{
		// Send the geometry off to the renderer to be uploaded to the gpu
		if (!Renderer.CreateGeometry(g, sizeof(VertexType), config.vertices.Size(), config.vertices.GetData(), sizeof(IndexType), config.indices.Size(), config.indices.GetData()))
		{
			m_registeredGeometries[g->id].referenceCount = 0;
			m_registeredGeometries[g->id].autoRelease = false;
			g->id = INVALID_ID;
			g->generation = INVALID_ID_U16;
			g->internalId = INVALID_ID;

			return false;
		}

		// Copy over the center and extents
		g->center = config.center;
		g->extents.min = config.minExtents;
		g->extents.max = config.maxExtents;

		// Acquire the material
		if (StringLength(config.materialName) > 0)
		{
			g->material = Materials.Acquire(config.materialName);
			if (!g->material)
			{
				g->material = Materials.GetDefault();
			}
		}

		return true;
	}
}
