
#include "test_manager.h"

#include <core/logger.h>
#include <core/clock.h>

TestManager::TestManager() : m_logger("TEST_MANAGER") {}

void TestManager::StartType(const std::string& type)
{
	m_currentType = type;
}

void TestManager::Register(u8 (*pFnTest)(), const std::string& description)
{
	TestEntry e;
	e.func = pFnTest;
	e.description = description;
	e.type = m_currentType;
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
		if (m_prevType != test.type)
		{
			m_logger.Info("------------------- {} -------------------", test.type);
			m_prevType = test.type;
		}

		i++;

		C3D::Clock testTime;
		testTime.Start();
		
		const u8 result = test.func();

		testTime.Update();
		if (result == PASSED) passed++;
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
		m_logger.Info("Executed {} of {} (skipped {}) {} ({:.4f} sec / {:.4f} sec total)", i, m_tests.size(), skipped, status, testTime.GetElapsed(), totalTime.GetElapsed());
	}

	totalTime.Stop();
	m_logger.Info("Results: {} passed, {} failed and {} skipped. Ran in {:.4f} sec", passed, failed, skipped, totalTime.GetElapsed());
}
