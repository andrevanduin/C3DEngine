
#pragma once
#include <core/defines.h>
#include <string>
#include <vector>

#include "core/logger.h"

#define FAILED 0
#define PASSED 1
#define SKIPPED 2

typedef u8 (*pFnTest)();

struct TestEntry
{
	pFnTest func;
	std::string description;
};

class TestManager
{
public:
	TestManager();

	void Register(pFnTest, const std::string& description);

	void RunTests();

private:
	C3D::LoggerInstance m_logger;

	std::vector<TestEntry> m_tests;
};