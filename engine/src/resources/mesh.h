
#pragma once
#include "geometry.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"
#include "core/identifier.h"

namespace C3D
{
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

		bool LoadCube(const Engine* engine, f32 width, f32 height, f32 depth, f32 tileX, f32 tileY, const String& name, const String& materialName);
		bool LoadFromResource(const Engine* engine, const char* resourceName);

		template <typename VertexType, typename IndexType>
		bool LoadFromConfig(const Engine* engine, const GeometryConfig<VertexType, IndexType>& config) 
		{
			m_engine = engine;

			uniqueId = Identifier::GetNewId(this);
			geometries.PushBack(Geometric.AcquireFromConfig(config, true));
			generation = 0;

			return true;
		}

		void Unload();

		u32 uniqueId;
		u8 generation;

		DynamicArray<Geometry*> geometries;

		Transform transform;

	private:
		bool LoadJobEntryPoint(void* data, void* resultData) const;
		void LoadJobSuccess(void* data) const;
		void LoadJobFailure(void* data) const;

		const Engine* m_engine;
	};
}
