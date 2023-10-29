
#include "system_manager.h"

#include "core/engine.h"
#include "core/logger.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "SYSTEM_MANAGER";

    void SystemManager::OnInit()
    {
        INFO_LOG("Initializing Systems Manager.");

        // 8 mb of total space for all our systems
        constexpr u64 systemsAllocatorTotalSize = MebiBytes(8);
        m_allocator.Create("LINEAR_SYSTEM_ALLOCATOR", systemsAllocatorTotalSize);
    }

    void SystemManager::OnShutdown()
    {
        INFO_LOG("Shutting down all Systems.");

        for (const auto system : m_systems)
        {
            system->OnShutdown();
            m_allocator.Delete(MemoryType::CoreSystem, system);
        }

        m_allocator.Destroy();
    }
}  // namespace C3D
