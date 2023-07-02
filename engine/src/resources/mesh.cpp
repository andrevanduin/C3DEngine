
#include "mesh.h"

#include "core/identifier.h"
#include "systems/geometry/geometry_system.h"
#include "systems/jobs/job_system.h"
#include "systems/resources/resource_system.h"

namespace C3D
{
    Mesh::Mesh() : m_logger("MESH") {}

    bool Mesh::Create(const SystemManager* pSystemsManager, const MeshConfig& config)
    {
        m_pSystemsManager = pSystemsManager;
        m_config = config;
        generation = INVALID_ID_U8;

        return true;
    }

    bool Mesh::Initialize()
    {
        if (m_config.resourceName) return true;

        const auto geometryCount = m_config.geometryConfigs.Size();
        if (geometryCount == 0) return false;

        // Reserve enough space for our geometry pointers in advance
        geometries.Reserve(geometryCount);

        return true;
    }

    bool Mesh::Load()
    {
        if (m_config.resourceName)
        {
            return LoadFromResource();
        }

        for (auto& config : m_config.geometryConfigs)
        {
            geometries.PushBack(Geometric.AcquireFromConfig(config, true));
            Geometric.DisposeConfig(config);
        }

        generation = 0;
        uniqueId = Identifier::GetNewId(this);
        return true;
    }

    bool Mesh::Unload()
    {
        for (const auto geometry : geometries)
        {
            Geometric.Release(geometry);
        }
        generation = INVALID_ID_U8;
        geometries.Destroy();
        return true;
    }

    bool Mesh::Destroy()
    {
        if (!geometries.Empty())
        {
            if (!Unload())
            {
                m_logger.Error("Destory() - failed to unload");
                return false;
            }
        }

        Identifier::ReleaseId(uniqueId);
        return true;
    }

    bool Mesh::LoadFromResource()
    {
        generation = INVALID_ID_U8;

        JobInfo info;
        info.entryPoint = [this]() { return Resources.Load(m_config.resourceName, m_resource); };
        info.onSuccess = [this]() { LoadJobSuccess(); };
        info.onFailure = [this]() { LoadJobFailure(); };

        Jobs.Submit(std::move(info));
        return true;
    }

    void Mesh::LoadJobSuccess()
    {
        Clock clock(m_pSystemsManager->GetSystemPtr<Platform>(PlatformSystemType));
        clock.Start();

        // NOTE: This also handles the GPU upload. Can't be jobified until the renderer is multiThreaded.
        // We can reserve enough space for our geometries instead of reallocating everytime the dynamic array grows
        geometries.Reserve(m_resource.geometryConfigs.Size());
        for (auto& c : m_resource.geometryConfigs)
        {
            geometries.PushBack(Geometric.AcquireFromConfig(c, true));
        }
        generation++;
        uniqueId = Identifier::GetNewId(this);

        clock.Update();
        m_logger.Info("LoadJobSuccess() - Successfully loaded: '{}' in {:.4f} ms", m_config.resourceName,
                      clock.GetElapsedMs());
        clock.Start();

        Resources.Unload(m_resource);

        clock.Update();
        m_logger.Info("LoadJobSuccess() - Resources.Unload took: {:.4f} ms", clock.GetElapsedMs());
    }

    void Mesh::LoadJobFailure()
    {
        // const auto meshParams = static_cast<MeshLoadParams*>(data);

        m_logger.Error("LoadJobFailure() - Failed to load: '{}'", m_config.resourceName);

        Resources.Unload(m_resource);
    }

    UIMesh::UIMesh() : m_logger("UI_MESH") {}

    bool UIMesh::LoadFromConfig(const SystemManager* pSystemsManager, const UIGeometryConfig& config)
    {
        m_pSystemsManager = pSystemsManager;

        uniqueId = Identifier::GetNewId(this);
        geometries.PushBack(Geometric.AcquireFromConfig(config, true));
        generation = 0;

        return true;
    }

    bool UIMesh::Unload()
    {
        for (const auto geometry : geometries)
        {
            Geometric.Release(geometry);
        }
        generation = INVALID_ID_U8;
        geometries.Destroy();
        return true;
    }

}  // namespace C3D
