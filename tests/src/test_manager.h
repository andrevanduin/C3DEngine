
#pragma once
#include <defines.h>
#include <logger/logger.h>
#include <string/string.h>

#include <string>
#include <vector>

#define REGISTER_TEST(name, description) manager.Register(name, #name, description)

#define TEST(name) void name()

constexpr auto FAILED  = 0;
constexpr auto PASSED  = 1;
constexpr auto SKIPPED = 2;

using TestFunc = void (*)();

struct TestResult
{
    u8 code;
    std::string message;
};

struct TestEntry
{
    int index;
    TestFunc func;
    TestResult result;

    std::string name;
    std::string description;
    std::string type;
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