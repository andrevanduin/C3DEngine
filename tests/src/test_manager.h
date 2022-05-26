
#pragma once
#include <core/defines.h>
#include <string>
#include <vector>

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
	void Init();

	void Register(pFnTest, const std::string& description);

	void RunTests();

private:
	std::vector<TestEntry> m_tests;
};