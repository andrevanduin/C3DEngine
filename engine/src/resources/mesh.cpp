
#include "mesh.h"

#include "systems/resource_system.h"
#include "systems/jobs/job_system.h"

namespace C3D
{
	bool Mesh::LoadFromResource(const char* resourceName)
	{
		generation = INVALID_ID_U8;

		MeshLoadParams params;
		params.resourceName = resourceName;
		params.outMesh = this;

		JobInfo info;
		info.entryPoint = LoadJobEntryPoint;
		info.onSuccess = LoadJobSuccess;
		info.onFailure = LoadJobFailure;
		info.SetData(&params, sizeof(MeshLoadParams));

		Jobs.Submit(info);
		return true;
	}

	void Mesh::Unload()
	{
		for (u16 i = 0; i < geometryCount; i++)
		{
			Geometric.Release(geometries[i]);
		}

		Memory.Free(geometries, sizeof(Geometry*) * geometryCount, MemoryType::Array);
		geometries = nullptr;
		geometryCount = 0;
		generation = INVALID_ID_U8;
	}

	bool Mesh::LoadJobEntryPoint(void* data, void* resultData)
	{
		const auto loadParams = static_cast<MeshLoadParams*>(data);

		const bool result = Resources.Load(loadParams->resourceName, &loadParams->meshResource);

		// NOTE: The load params are also used as the result data here, only the meshResource field is populated now.
		Memory.Copy(resultData, loadParams, sizeof(MeshLoadParams));

		return result;
	}

	void Mesh::LoadJobSuccess(void* data)
	{
		const auto meshParams = static_cast<MeshLoadParams*>(data);

		// NOTE: This also handles the GPU upload. Can't be jobified until the renderer is multiThreaded.
		auto& configs = meshParams->meshResource.geometryConfigs;
		const auto configCount = configs.Size();

		meshParams->outMesh->geometryCount = static_cast<u16>(configCount);
		meshParams->outMesh->geometries = Memory.Allocate<Geometry*>(configCount, MemoryType::Array);
		for (u64 i = 0; i < configCount; i++)
		{
			meshParams->outMesh->geometries[i] = Geometric.AcquireFromConfig(configs[i], true);
		}
		meshParams->outMesh->generation++;

		Logger::Trace("[MESH] - Successfully loaded: '{}'.", meshParams->resourceName);

		Resources.Unload(&meshParams->meshResource);
	}

	void Mesh::LoadJobFailure(void* data)
	{
		const auto meshParams = static_cast<MeshLoadParams*>(data);

		Logger::Error("[MESH] - Failed to load: '{}'.", meshParams->resourceName);

		Resources.Unload(&meshParams->meshResource);
	}
}
