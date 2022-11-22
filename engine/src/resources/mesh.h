
#pragma once
#include "geometry.h"
#include "loaders/mesh_loader.h"
#include "renderer/transform.h"

namespace C3D
{
	class Mesh
	{
	public:
		Mesh() : uniqueId(INVALID_ID), generation(INVALID_ID_U8), geometryCount(0), geometries(nullptr) {}

		bool LoadFromResource(const char* resourceName);
		void Unload();

		u32 uniqueId;
		u8 generation;

		u16 geometryCount;
		Geometry** geometries;

		Transform transform;

	private:
		static bool LoadJobEntryPoint(void* data, void* resultData);
		static void LoadJobSuccess(void* data);
		static void LoadJobFailure(void* data);
	};

	struct MeshLoadParams
	{
		const char* resourceName;
		Mesh* outMesh;
		MeshResource meshResource;
	};
}
