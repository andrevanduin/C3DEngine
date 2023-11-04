
#include "mesh.h"

#include "core/identifier.h"
#include "core/scoped_timer.h"
#include "resources/debug/debug_box_3d.h"
#include "systems/geometry/geometry_system.h"
#include "systems/jobs/job_system.h"
#include "systems/resources/resource_system.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "MESH";

    bool Mesh::Create(const SystemManager* pSystemsManager, const MeshConfig& cfg)
    {
        m_pSystemsManager = pSystemsManager;
        config            = cfg;
        generation        = INVALID_ID_U8;

        if (cfg.enableDebugBox)
        {
            m_debugBox = Memory.New<DebugBox3D>(MemoryType::Resource);
            m_debugBox->Create(pSystemsManager, vec3(1.0f), nullptr);
        }

        return true;
    }

    bool Mesh::Initialize()
    {
        if (m_debugBox)
        {
            if (!m_debugBox->Initialize())
            {
                WARN_LOG("Failed to initialize Debug Box.");
                Memory.Delete(m_debugBox);
                m_debugBox = nullptr;
            }
        }

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
        uniqueId   = Identifier::GetNewId(this);

        if (m_debugBox)
        {
            if (!m_debugBox->Load())
            {
                WARN_LOG("Failed to load Debug Box.");
                m_debugBox->Destroy();

                Memory.Delete(m_debugBox);
                m_debugBox = nullptr;
            }
        }

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

        if (m_debugBox)
        {
            if (!m_debugBox->Unload())
            {
                WARN_LOG("Failed to unload Debug Box.");
                m_debugBox->Destroy();

                Memory.Delete(m_debugBox);
                m_debugBox = nullptr;
            }
        }
        return true;
    }

    bool Mesh::Destroy()
    {
        if (!geometries.Empty())
        {
            if (!Unload())
            {
                ERROR_LOG("Failed to unload.");
                return false;
            }
        }

        if (m_debugBox)
        {
            m_debugBox->Destroy();
            Memory.Delete(m_debugBox);
            m_debugBox = nullptr;
        }

        Identifier::ReleaseId(uniqueId);
        return true;
    }

    bool Mesh::LoadFromResource()
    {
        generation = INVALID_ID_U8;

        JobInfo info;
        info.entryPoint = [this]() { return Resources.Load(config.resourceName, m_resource); };
        info.onSuccess  = [this]() { LoadJobSuccess(); };
        info.onFailure  = [this]() { LoadJobFailure(); };

        Jobs.Submit(std::move(info));
        return true;
    }

    void Mesh::LoadJobSuccess()
    {
        {
            auto timer = ScopedTimer("Acquiring Geometry from Config", m_pSystemsManager);

            // NOTE: This also handles the GPU upload. Can't be jobified until the renderer is multiThreaded.
            // We can reserve enough space for our geometries instead of reallocating everytime the dynamic array grows
            geometries.Reserve(m_resource.geometryConfigs.Size());
            for (auto& c : m_resource.geometryConfigs)
            {
                Geometry* g = Geometric.AcquireFromConfig(c, true);

                Extents3D& local = g->extents;

                for (u32 v = 0; v < c.vertices.Size(); v++)
                {
                    auto& vert = c.vertices[v];

                    // Min
                    if (vert.position.x < local.min.x)
                    {
                        local.min.x = vert.position.x;
                    }
                    if (vert.position.y < local.min.y)
                    {
                        local.min.y = vert.position.y;
                    }
                    if (vert.position.z < local.min.z)
                    {
                        local.min.z = vert.position.z;
                    }

                    // Max
                    if (vert.position.x > local.max.x)
                    {
                        local.max.x = vert.position.x;
                    }
                    if (vert.position.y > local.max.y)
                    {
                        local.max.y = vert.position.y;
                    }
                    if (vert.position.z > local.max.z)
                    {
                        local.max.z = vert.position.z;
                    }
                }

                // Min
                if (local.min.x < m_extents.min.x)
                {
                    m_extents.min.x = local.min.x;
                }
                if (local.min.y < m_extents.min.y)
                {
                    m_extents.min.y = local.min.y;
                }
                if (local.min.z < m_extents.min.z)
                {
                    m_extents.min.z = local.min.z;
                }

                // Max
                if (local.max.x > m_extents.max.x)
                {
                    m_extents.max.x = local.max.x;
                }
                if (local.max.y > m_extents.max.y)
                {
                    m_extents.max.y = local.max.y;
                }
                if (local.max.z > m_extents.max.z)
                {
                    m_extents.max.z = local.max.z;
                }

                geometries.PushBack(g);
            }

            generation++;
            uniqueId = Identifier::GetNewId(this);

            if (m_debugBox)
            {
                m_debugBox->SetParent(&transform);

                if (!m_debugBox->Load())
                {
                    WARN_LOG("Failed to load Debug Box.");
                    m_debugBox->Destroy();

                    Memory.Delete(m_debugBox);
                    m_debugBox = nullptr;
                }
                else
                {
                    m_debugBox->SetExtents(m_extents);
                    m_debugBox->SetColor(vec4(0.0f, 1.0f, 0.0f, 1.0f));
                }
            }
        }

        {
            auto timer = ScopedTimer("Unloading Resource", m_pSystemsManager);
            Resources.Unload(m_resource);
        }
    }

    void Mesh::LoadJobFailure()
    {
        ERROR_LOG("Failed to load: '{}'.", config.resourceName);

        Resources.Unload(m_resource);
    }

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
