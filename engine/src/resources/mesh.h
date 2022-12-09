
#pragma once
#include "geometry.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"

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

		bool LoadFromResource(const char* resourceName);
		void Unload();

		u32 uniqueId;
		u8 generation;

		DynamicArray<Geometry*> geometries;

		Transform transform;

	private:
		static bool LoadJobEntryPoint(void* data, void* resultData);
		static void LoadJobSuccess(void* data);
		static void LoadJobFailure(void* data);
	};
}
