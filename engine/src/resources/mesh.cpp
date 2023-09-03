
#include "mesh.h"

#include "core/identifier.h"
#include "systems/geometry/geometry_system.h"
#include "systems/jobs/job_system.h"
#include "systems/resources/resource_system.h"

namespace C3D
{
    Mesh::Mesh() : m_logger("MESH") {}

    Mesh::Mesh(const MeshConfig& cfg) : m_logger("MESH"), config(cfg) {}

    bool Mesh::Create(const SystemManager* pSystemsManager, const MeshConfig& cfg)
    {
        m_pSystemsManager = pSystemsManager;
        config = cfg;
        generation = INVALID_ID_U8;

        return true;
    }

    bool Mesh::Initialize()
    {
        if (config.resourceName) return true;

        const auto geometryCount = config.geometryConfigs.Size();
        if (geometryCount == 0) return false;

        // Reserve enough space for our geometry pointers in advance
        geometries.Reserve(geometryCount);

        return true;
    }

    bool Mesh::Load()
    {
        if (config.resourceName)
        {
            return LoadFromResource();
        }

        for (auto& gConfig : config.geometryConfigs)
        {
            geometries.PushBack(Geometric.AcquireFromConfig(gConfig, true));
            Geometric.DisposeConfig(gConfig);
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
        info.entryPoint = [this]() { return Resources.Load(config.resourceName, m_resource); };
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
        m_logger.Info("LoadJobSuccess() - Successfully loaded: '{}' in {:.4f} ms", config.resourceName,
                      clock.GetElapsedMs());
        clock.Start();

        Resources.Unload(m_resource);

        clock.Update();
        m_logger.Info("LoadJobSuccess() - Resources.Unload took: {:.4f} ms", clock.GetElapsedMs());
    }

    void Mesh::LoadJobFailure()
    {
        m_logger.Error("LoadJobFailure() - Failed to load: '{}'", config.resourceName);

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
