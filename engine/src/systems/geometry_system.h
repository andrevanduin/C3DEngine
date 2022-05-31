
#pragma once
#include "core/defines.h"
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
		u32 vertexCount;
		Vertex3D* vertices;
		u32 indexCount;
		u32* indices;

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
		Geometry* AcquireFromConfig(const GeometryConfig& config, bool autoRelease);

		void Release(const Geometry* geometry);

		Geometry* GetDefault();

		// NOTE: Vertex and index arrays are dynamically allocated so they should be freed by the user
		[[nodiscard]] GeometryConfig GeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCount, u32 ySegmentCount, f32 tileX, f32 tileY, const string& name, const string& materialName) const;

	private:
		bool CreateGeometry(const GeometryConfig& config, Geometry* g) const;

		static void DestroyGeometry(Geometry* g);

		bool CreateDefaultGeometry();

		bool m_initialized;

		GeometrySystemConfig m_config;

		Geometry m_defaultGeometry;

		GeometryReference* m_registeredGeometries;
	};
}
