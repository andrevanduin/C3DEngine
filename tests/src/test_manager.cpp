
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

    u32 passed = 0;

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
            WARN_LOG("Result: SKIPPED (Ran in {:.4f} sec)", testTime.GetElapsed());
            m_skipped.push_back(test);
        }
        else if (result == FAILED)
        {
            ERROR_LOG("Result: FAILED (Ran in {:.4f} sec)", testTime.GetElapsed());
            m_failures.push_back(test);
        }
    }

    INFO_LOG("Results: {} passed, {} failed and {} skipped. Total runtime {:.4f} sec.", passed, m_failures.size(), m_skipped.size(),
             testTime.GetTotalElapsed());

    if (!m_skipped.empty())
    {
        INFO_LOG("The following tests have been SKIPPED:");
        for (auto& test : m_skipped)
        {
            WARN_LOG("({}/{}): {} - {}", i, m_tests.size(), test.name, test.description);
        }
    }

    if (!m_failures.empty())
    {
        INFO_LOG("The following tests have FAILED:");
        for (auto& test : m_failures)
        {
            ERROR_LOG("({}/{}): {} - {}", i, m_tests.size(), test.name, test.description)
        }
    }

    systemsManager.OnShutdown();
}
