
#include "test_manager.h"

#include <core/clock.h>
#include <core/logger.h>

TestManager::TestManager(const u64 memorySize) : m_logger("TEST_MANAGER")
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
    platform.Init(config);

    C3D::Clock totalTime(&platform);
    totalTime.Start();

    auto i = 0;
    for (auto& test : m_tests)
    {
        if (m_prevType != test.type)
        {
            m_logger.Info("{:-^70}", test.type);
            m_prevType = test.type;
        }

        i++;

        C3D::Clock testTime(&platform);
        testTime.Start();

        const u8 result = test.func();

        testTime.Update();
        if (result == PASSED)
            passed++;
        else if (result == SKIPPED)
        {
            skipped++;
            m_logger.Warn("[SKIPPED] - {}", test.description);
        }
        else if (result == FAILED)
        {
            m_logger.Error("[FAILED] - {}", test.description);
            failed++;
        }

        totalTime.Update();

        auto status = failed ? "*** FAILED ***" : "SUCCESS";
        m_logger.Info("Executed {} of {} (skipped {}) {} ({:.4f} sec / {:.4f} sec total)", i, m_tests.size(), skipped, status,
                      testTime.GetElapsed(), totalTime.GetElapsed());
    }

    totalTime.Stop();
    m_logger.Info("Results: {} passed, {} failed and {} skipped. Ran in {:.4f} sec", passed, failed, skipped, totalTime.GetElapsed());

    platform.Shutdown();
}
