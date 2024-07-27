
#include "test_manager.h"

#include <core/clock.h>
#include <core/logger.h>
#include <systems/system_manager.h>

constexpr const char* INSTANCE_NAME = "TEST_MANAGER";

TestManager::TestManager(const u64 memorySize)
{
    C3D::Logger::Init();
    Metrics.Init();
    C3D::GlobalMemorySystem::Init({ memorySize });
}

TestManager::~TestManager() { C3D::GlobalMemorySystem::Destroy(); }

void TestManager::StartType(const std::string& type) { m_currentType = type; }

void TestManager::Register(TestFunc func, const std::string& name, const std::string& description)
{
    TestEntry e;
    e.func        = func;
    e.name        = name;
    e.description = description;
    e.type        = m_currentType;
    m_tests.push_back(e);
}

void TestManager::RunTests()
{
    using namespace C3D;

    u32 passed  = 0;
    u32 failed  = 0;
    u32 skipped = 0;

    PlatformSystemConfig config;
    config.applicationName           = "Tests";
    config.windowConfig.shouldCreate = false;

    auto& systemsManager = SystemManager::GetInstance();
    systemsManager.OnInit();
    systemsManager.RegisterSystem<Platform>(PlatformSystemType, config);

    Clock testTime;

    auto i = 0;
    for (auto& test : m_tests)
    {
        if (m_prevType != test.type)
        {
            INFO_LOG("{:-^70}", test.type);
            m_prevType = test.type;
        }

        i++;

        INFO_LOG("Executing ({}/{}): {} - {}", i, m_tests.size(), test.name, test.description);

        testTime.Begin();

        const u8 result = test.func();

        testTime.End();

        if (result == PASSED)
        {
            passed++;
            INFO_LOG("Result: SUCCESS (Ran in {:.4f} sec)", testTime.GetElapsed());
        }
        else if (result == SKIPPED)
        {
            skipped++;
            WARN_LOG("Result: SKIPPED (Ran in {:.4f} sec)", testTime.GetElapsed());
        }
        else if (result == FAILED)
        {
            failed++;
            ERROR_LOG("Result: FAILED (Ran in {:.4f} sec)", testTime.GetElapsed());
        }
    }

    INFO_LOG("Results: {} passed, {} failed and {} skipped. Total runtime {:.4f} sec.", passed, failed, skipped,
             testTime.GetTotalElapsed());

    systemsManager.OnShutdown();
}
