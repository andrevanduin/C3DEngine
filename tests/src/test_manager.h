
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
	std::string type;
};

class TestManager
{
public:
	TestManager();

	void StartType(const std::string& type);
	void Register(pFnTest, const std::string& description);

	void RunTests();

private:
	C3D::LoggerInstance m_logger;

	std::string m_currentType;
	std::string m_prevType;

	std::vector<TestEntry> m_tests;
};