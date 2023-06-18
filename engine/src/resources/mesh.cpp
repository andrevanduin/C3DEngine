
#include "mesh.h"

#include "core/identifier.h"
#include "systems/jobs/job_system.h"
#include "systems/resources/resource_system.h"

namespace C3D
{
    Mesh::Mesh() : uniqueId(INVALID_ID), generation(INVALID_ID_U8), m_engine(nullptr) {}

    bool Mesh::LoadCube(const Engine* engine, const f32 width, const f32 height, const f32 depth, const f32 tileX,
                        const f32 tileY, const String& name, const String& materialName)
    {
        m_engine = engine;

        auto cubeConfig = Geometric.GenerateCubeConfig(width, height, depth, tileX, tileY, name, materialName);

        geometries.PushBack(Geometric.AcquireFromConfig(cubeConfig, true));
        transform = Transform();
        generation = 0;
        uniqueId = Identifier::GetNewId(this);

        Geometric.DisposeConfig(&cubeConfig);

        return true;
    }

    bool Mesh::LoadFromResource(const Engine* engine, const char* resourceName)
    {
        m_engine = engine;
        generation = INVALID_ID_U8;

        MeshLoadParams params;
        params.resourceName = resourceName;
        params.outMesh = this;

        JobInfo<MeshLoadParams, MeshLoadParams> info;
        info.entryPoint = [this](void* data, void* resultData) { return LoadJobEntryPoint(data, resultData); };
        info.onSuccess = [this](void* data) { LoadJobSuccess(data); };
        info.onFailure = [this](void* data) { LoadJobFailure(data); };
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

    bool Mesh::LoadJobEntryPoint(void* data, void* resultData) const
    {
        const auto loadParams = static_cast<MeshLoadParams*>(data);

        const bool result = Resources.Load(loadParams->resourceName.Data(), loadParams->meshResource);

        // NOTE: The load params are also used as the result data here, only the meshResource field is populated now.
        const auto rData = static_cast<MeshLoadParams*>(resultData);
        *rData = *loadParams;

        return result;
    }

    void Mesh::LoadJobSuccess(void* data) const
    {
        const auto meshParams = static_cast<MeshLoadParams*>(data);

        // NOTE: This also handles the GPU upload. Can't be jobified until the renderer is multiThreaded.
        auto& configs = meshParams->meshResource.geometryConfigs;
        // We can reserve enough space for our geometries instead of reallocating everytime the dynamic array grows
        meshParams->outMesh->geometries.Reserve(configs.Size());
        for (auto& c : configs)
        {
            meshParams->outMesh->geometries.PushBack(Geometric.AcquireFromConfig(c, true));
        }
        meshParams->outMesh->generation++;

        Logger::Trace("[MESH] - Successfully loaded: '{}'.", meshParams->resourceName);

        Resources.Unload(meshParams->meshResource);
    }

    void Mesh::LoadJobFailure(void* data) const
    {
        const auto meshParams = static_cast<MeshLoadParams*>(data);

        Logger::Error("[MESH] - Failed to load: '{}'.", meshParams->resourceName);

        Resources.Unload(meshParams->meshResource);
    }
}  // namespace C3D
