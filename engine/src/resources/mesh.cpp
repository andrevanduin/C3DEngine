
#include "mesh.h"

#include "systems/resource_system.h"
#include "systems/jobs/job_system.h"

namespace C3D
{
	Mesh::Mesh()
		: uniqueId(INVALID_ID), generation(INVALID_ID_U8)
	{}

	bool Mesh::LoadFromResource(const char* resourceName)
	{
		generation = INVALID_ID_U8;

		MeshLoadParams params;
		params.resourceName = resourceName;
		params.outMesh = this;

		JobInfo<MeshLoadParams, MeshLoadParams> info;
		info.entryPoint = LoadJobEntryPoint;
		info.onSuccess = LoadJobSuccess;
		info.onFailure = LoadJobFailure;
		info.input = params;

		Jobs.Submit(info);
		return true;
	}

	void Mesh::Unload()
	{
		for (const auto geometry : geometries)
		{
			Geometric.Release(geometry);
		}
		generation = INVALID_ID_U8;
		geometries.Clear();
	}

	bool Mesh::LoadJobEntryPoint(void* data, void* resultData)
	{
		const auto loadParams = static_cast<MeshLoadParams*>(data);

		const bool result = Resources.Load(loadParams->resourceName.Data(), loadParams->meshResource);

		// NOTE: The load params are also used as the result data here, only the meshResource field is populated now.
		const auto rData = static_cast<MeshLoadParams*>(resultData);
		*rData = *loadParams;

		return result;
	}

	void Mesh::LoadJobSuccess(void* data)
	{
		const auto meshParams = static_cast<MeshLoadParams*>(data);

		// NOTE: This also handles the GPU upload. Can't be jobified until the renderer is multiThreaded.
		auto& configs = meshParams->meshResource.geometryConfigs;
		const auto configCount = configs.Size();

		for (u64 i = 0; i < configCount; i++)
		{
			meshParams->outMesh->geometries.PushBack(Geometric.AcquireFromConfig(configs[i], true));
		}
		meshParams->outMesh->generation++;

		Logger::Trace("[MESH] - Successfully loaded: '{}'.", meshParams->resourceName);

		Resources.Unload(meshParams->meshResource);
	}

	void Mesh::LoadJobFailure(void* data)
	{
		const auto meshParams = static_cast<MeshLoadParams*>(data);

		Logger::Error("[MESH] - Failed to load: '{}'.", meshParams->resourceName);

		Resources.Unload(meshParams->meshResource);
	}
}
