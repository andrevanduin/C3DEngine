
#pragma once
#include <core/defines.h>
#include <string>
#include <vector>

#include "core/logger.h"

constexpr auto FAILED  = 0;
constexpr auto PASSED  = 1;
constexpr auto SKIPPED = 2;

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
	TestManager(u64 memorySize);
	~TestManager();

	void StartType(const std::string& type);
	void Register(pFnTest, const std::string& description);

	void RunTests();

private:
	C3D::LoggerInstance<16> m_logger;

	std::string m_currentType;
	std::string m_prevType;

	std::vector<TestEntry> m_tests;
};