
#include "system_manager.h"

#include "core/engine.h"
#include "core/logger.h"

namespace C3D
{
    SystemManager::SystemManager() : m_logger("SYSTEM_MANAGER") {}

    void SystemManager::Init()
    {
        m_logger.Info("Init()");

        // 8 mb of total space for all our systems
        constexpr u64 systemsAllocatorTotalSize = MebiBytes(8);
        m_allocator.Create("LINEAR_SYSTEM_ALLOCATOR", systemsAllocatorTotalSize);
    }

    void SystemManager::Shutdown()
    {
        m_logger.Info("Shutdown() - Started");

        for (const auto system : m_systems)
        {
            system->Shutdown();
            m_allocator.Delete(MemoryType::CoreSystem, system);
        }

        m_allocator.Destroy();
        m_logger.Info("Shutdown() - Finished");
    }
}  // namespace C3D
