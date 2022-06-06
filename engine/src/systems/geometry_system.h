
#pragma once
#include "core/defines.h"
#include "core/logger.h"
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

	struct GeometryConfig
	{
		u32 vertexSize;
		u32 vertexCount;
		void* vertices;

		u32 indexSize;
		u32 indexCount;
		void* indices;

		char name[GEOMETRY_NAME_MAX_LENGTH];
		char materialName[MATERIAL_NAME_MAX_LENGTH];
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
		[[nodiscard]] Geometry* AcquireFromConfig(const GeometryConfig& config, bool autoRelease) const;

		void Release(const Geometry* geometry) const;

		Geometry* GetDefault();
		Geometry* GetDefault2D();

		// NOTE: Vertex and index arrays are dynamically allocated so they should be freed by the user
		[[nodiscard]] GeometryConfig GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 tileX, f32 tileY, const string& name, const string& materialName) const;

		[[nodiscard]] GeometryConfig GenerateCubeConfig(f32 width, f32 height, f32 depth, f32 tileX, f32 tileY, const string& name, const string& materialName) const;

	private:
		bool CreateGeometry(const GeometryConfig& config, Geometry* g) const;

		static void DestroyGeometry(Geometry* g);

		bool CreateDefaultGeometries();

		LoggerInstance m_logger;

		bool m_initialized;

		GeometrySystemConfig m_config;

		Geometry m_defaultGeometry;
		Geometry m_default2DGeometry;

		GeometryReference* m_registeredGeometries;
	};
}
