
#pragma once
#include <core/defines.h>

#include <string>
#include <vector>

#include "core/logger.h"

#define REGISTER_TEST(name, description) manager.Register(name, #name, description)

constexpr auto FAILED  = 0;
constexpr auto PASSED  = 1;
constexpr auto SKIPPED = 2;

using TestFunc = u8 (*)();

struct TestEntry
{
    TestFunc func;
    std::string name;
    std::string description;
    std::string type;
    std::string failureMsg;
};

class TestManager
{
public:
    TestManager(u64 memorySize);
    ~TestManager();

    void StartType(const std::string& type);
    void Register(TestFunc func, const std::string& name, const std::string& description);

    void RunTests();

private:
    std::string m_currentType;
    std::string m_prevType;

    std::vector<TestEntry> m_tests;
    std::vector<TestEntry> m_skipped;
    std::vector<TestEntry> m_failures;
};