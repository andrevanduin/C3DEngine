
#include "test_manager.h"

#include <core/clock.h>
#include <core/logger.h>
#include <systems/system_manager.h>

#include "expect.h"

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
            C3D::Logger::Info("--- Running '{}' Tests ---", test.type);
            m_prevType = test.type;
        }

        i++;

        C3D::Logger::Info("Executing ({}/{}): {} - {}", i, m_tests.size(), test.name, test.description);

        testTime.Begin();

        try
        {
            test.func();
            test.result.code = true;
        }
        catch (const ExpectException& exc)
        {
            auto what = exc.what();

            test.result.code    = false;
            test.result.message = what;

            C3D::Logger::Error(what);
        }

        testTime.End();

        // Store off the index so we can print it later if needed
        test.index = i;

        // Print the result if successful, otherwise store the result in m_skipped or m_failures depending on the status
        if (test.result.code == PASSED)
        {
            passed++;
            C3D::Logger::Info("Result: SUCCESS (Ran in {:.4f} sec)", testTime.GetElapsed());
        }
        else if (test.result.code == SKIPPED)
        {
            C3D::Logger::Warn("Result: SKIPPED (Ran in {:.4f} sec)", testTime.GetElapsed());
            m_skipped.push_back(test);
        }
        else if (test.result.code == FAILED)
        {
            C3D::Logger::Error("Result: FAILED (Ran in {:.4f} sec)", testTime.GetElapsed());
            m_failures.push_back(test);
        }
    }

    INFO_LOG("Results: {} passed, {} failed and {} skipped. Total runtime {:.4f} sec.", passed, m_failures.size(), m_skipped.size(),
             testTime.GetTotalElapsed());

    if (!m_skipped.empty())
    {
        C3D::Logger::Info("The following tests have been SKIPPED:");
        for (auto& test : m_skipped)
        {
            C3D::Logger::Warn("({}/{}): {} - {}", test.index, m_tests.size(), test.name, test.description);
            C3D::Logger::Warn(test.result.message.data());
        }
    }

    if (!m_failures.empty())
    {
        C3D::Logger::Info("The following tests have FAILED:");
        for (auto& test : m_failures)
        {
            C3D::Logger::Error("({}/{}): {} - {}", test.index, m_tests.size(), test.name, test.description);
            C3D::Logger::Error(test.result.message.data());
        }
    }

    systemsManager.OnShutdown();
}
