
#include "test_manager.h"

#include <core/clock.h>
#include <core/logger.h>

constexpr const char* INSTANCE_NAME = "TEST_MANAGER";

TestManager::TestManager(const u64 memorySize)
{
    C3D::Logger::Init();
    Metrics.Init();
    C3D::GlobalMemorySystem::Init({ memorySize });
}

TestManager::~TestManager() { C3D::GlobalMemorySystem::Destroy(); }

void TestManager::StartType(const std::string& type) { m_currentType = type; }

void TestManager::Register(u8 (*pFnTest)(), const std::string& description)
{
    TestEntry e;
    e.func        = pFnTest;
    e.description = description;
    e.type        = m_currentType;
    m_tests.push_back(e);
}

void TestManager::RunTests()
{
    u32 passed  = 0;
    u32 failed  = 0;
    u32 skipped = 0;

    C3D::PlatformSystemConfig config;
    config.applicationName = "Tests";
    config.width           = 1280;
    config.height          = 720;
    config.x               = 0;
    config.y               = 0;
    config.makeWindow      = false;

    C3D::Platform platform;
    platform.OnInit(config);

    C3D::Clock testTime(&platform);

    auto i = 0;
    for (auto& test : m_tests)
    {
        if (m_prevType != test.type)
        {
            INFO_LOG("{:-^70}", test.type);
            m_prevType = test.type;
        }

        i++;

        testTime.Begin();

        const u8 result = test.func();

        testTime.End();

        if (result == PASSED)
            passed++;
        else if (result == SKIPPED)
        {
            skipped++;
            WARN_LOG("[SKIPPED] - {}.", test.description);
        }
        else if (result == FAILED)
        {
            ERROR_LOG("[FAILED] - {}.", test.description);
            failed++;
        }

        auto status = failed ? "*** FAILED ***" : "SUCCESS";
        INFO_LOG("Executed {} of {} (skipped {}) {} ({:.4f} sec / {:.4f} sec total).", i, m_tests.size(), skipped, status,
                 testTime.GetElapsed(), testTime.GetTotalElapsed());
    }

    INFO_LOG("Results: {} passed, {} failed and {} skipped. Ran in {:.4f} sec.", passed, failed, skipped, testTime.GetTotalElapsed());

    platform.OnShutdown();
}
