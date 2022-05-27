
#include "test_manager.h"

#include <core/logger.h>
#include <core/clock.h>


void TestManager::Init()
{
	C3D::Logger::PushPrefix("TEST_MANAGER");
}

void TestManager::Register(u8 (*pFnTest)(), const std::string& description)
{
	TestEntry e;
	e.func = pFnTest;
	e.description = description;
	m_tests.push_back(e);
}

void TestManager::RunTests()
{
	u32 passed = 0;
	u32 failed = 0;
	u32 skipped = 0;

	C3D::Clock totalTime;
	totalTime.Start();

	auto i = 0;
	for (auto& test : m_tests)
	{
		i++;

		C3D::Clock testTime;
		testTime.Start();

		const bool result = test.func();

		testTime.Update();
		if (result == PASSED) passed++;
		else if (result == SKIPPED)
		{
			skipped++;
			C3D::Logger::Warn("[SKIPPED] - {}", test.description);
		}
		else if (result == FAILED) 
		{
			C3D::Logger::Error("[FAILED] - {}", test.description);
			failed++;
		}

		totalTime.Update();

		auto status = failed ? "*** FAILED ***" : "SUCCESS";
		C3D::Logger::Info("Executed {} of {} (skipped {}) {} ({:.4f} sec / {:.4f} sec total", i, m_tests.size(), skipped, status, testTime.GetElapsed(), totalTime.GetElapsed());
	}

	totalTime.Stop();
	C3D::Logger::Info("Results: {} passed, {} failed and {} skipped. Ran in {:.4f} sec", passed, failed, skipped, totalTime.GetElapsed());
}
