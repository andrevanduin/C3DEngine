
#include "string_tests.h"

#include <any>
#include <iostream>
#include <variant>
#include <vector>

#include "../expect.h"
#include "../util.h"
#include "containers/string.h"
#include "core/metrics/metrics.h"

u8 StringShouldCreateEmptyWithEmptyCtor()
{
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));
    {
        const C3D::String str;

        ExpectShouldBe(0, str.Size());
        ExpectShouldBe('\0', str[0]);
    }
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));
    return true;
}

u8 StringOperatorEqualsConstChar()
{
    // Starting string is stack allocated
    {
        // operator=(const char* other) with other.length < 15
        C3D::String stack = "1234";
        const char* other = "123456";

        stack = other;
        ExpectToBeTrue(stack == C3D::String("123456"));

        // Since both are stack allocated we expect no dynamic memory usage
        ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));
    }
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));

    {
        // operator=(const char* other) with other.length >= 15
        C3D::String stack = "1234";
        const char* other = "1234567891011121314151617";

        stack = other;
        ExpectToBeTrue(stack == C3D::String("1234567891011121314151617"));

        // Since other is > 15 we expect a dynamic memory allocation
        ExpectShouldBe(std::strlen(other) + 1, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::C3DString));
    }
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));

    // Starting string is heap allocated
    {
        // operator=(const char* other) with other.length < 15
        C3D::String heap  = "123456789101112131415";
        const char* other = "123456";

        heap = other;
        ExpectToBeTrue(heap == C3D::String("123456"));

        // Since other is < 15 we expect no memory usage
        ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));
    }
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));

    {
        // operator=(const char* other) with other.length >= 15
        C3D::String heap  = "123456789101112131415";
        const char* other = "1234567891011121314151617";

        heap = other;
        ExpectToBeTrue(heap == C3D::String("1234567891011121314151617"));

        // Since other is > 15 we expect a dynamic memory allocation
        ExpectShouldBe(std::strlen(other) + 1, Metrics.GetRequestedMemoryUsage(C3D::MemoryType::C3DString));
    }
    ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));

    return true;
}

u8 StringShouldDoIntegerConversion()
{
    Util util;

    const auto randomIntegers = util.GenerateRandom<i32>(500, std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    for (auto& randomInteger : randomIntegers)
    {
        // Generate our string
        C3D::String s(randomInteger);

        // Use to std::string implementation to compare
        auto stdString = std::to_string(randomInteger);

        // Compare the char* that we can get from both to ensure it's the same
        if (std::strcmp(stdString.data(), s.Data()) != 0)
        {
            return false;
        }
        // ExpectToBeTrue();
    }

    return true;
}

u8 StringShouldDoBooleanConversion()
{
    ExpectToBeTrue(C3D::String("true") == C3D::String(true));
    ExpectToBeTrue(C3D::String("false") == C3D::String(false));
    return true;
}

u8 StringShouldUseSso()
{
    const std::vector<u64> sizes = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    for (const auto size : sizes)
    {
        // All sizes < 16 characters should not allocate memory
        const C3D::String str(std::string(size, 'a').data());
        ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));

        // Copying should also not result in allocated memory
        C3D::String otherStr = str;
        ExpectShouldBe(0, Metrics.GetMemoryUsage(C3D::MemoryType::C3DString));
    }

    return true;
}

u8 StringShouldCompare()
{
    ExpectToBeTrue(C3D::String("Test2") == C3D::String("Test2"));
    ExpectToBeTrue(C3D::String("Test") != C3D::String("Test2"));

    return true;
}

u8 StringShouldBeTruthy()
{
    const C3D::String empty;
    const C3D::String notEmpty("This string is not empty");

    if (empty)
    {
        AssertFail("Empty string should evaluate to false");
    }
    if (!notEmpty)
    {
        AssertFail("Non-Empty string should evaluate to true");
    }

    return true;
}

u8 StringShouldAppend()
{
    C3D::String a("Hello ");
    const C3D::String b("World");

    a.Append(b);
    ExpectToBeTrue(std::strcmp("Hello World", a.Data()) == 0);
    ExpectShouldBe(11, a.Size());

    C3D::String c("Longer string that has to be heap ");
    const C3D::String d("allocated");

    c.Append(d);
    ExpectToBeTrue(std::strcmp("Longer string that has to be heap allocated", c.Data()) == 0);
    ExpectShouldBe(43, c.Size());

    C3D::String e("Long String That we will add to another");
    const C3D::String f(" very long string to test if it also works when not using SSO");

    e.Append(f);
    ExpectToBeTrue(
        std::strcmp("Long String That we will add to another very long string to test if it also works when not using SSO", e.Data()) == 0);
    ExpectShouldBe(100, e.Size());

    C3D::String ch("Test123");
    ch.Append('4');

    ExpectShouldBe("Test1234", ch);
    ExpectShouldBe(8, ch.Size());

    return true;
}

u8 StringShouldTrim()
{
    C3D::String right("Test123  ");
    right.TrimRight();
    ExpectShouldBe("Test123", right);

    C3D::String left("   Test123");
    left.TrimLeft();
    ExpectShouldBe("Test123", left);

    C3D::String trim("    Test 1234567    ");
    trim.Trim();
    ExpectShouldBe("Test 1234567", trim);

    C3D::String newLines("\n\nTest1234\n\n\n\n");
    newLines.Trim();
    ExpectShouldBe("Test1234", newLines);

    return true;
}

u8 StringShouldSplit()
{
    C3D::String test("size=21");

    auto result = test.Split('=');

    ExpectShouldBe(2, result.Size());
    ExpectShouldBe("size", result[0]);
    ExpectShouldBe("21", result[1]);

    return true;
}

u8 StringInsert()
{
    C3D::String test("134");

    // Test if we can insert at a random spot
    test.Insert(1, '2');
    ExpectToBeTrue(test == C3D::String("1234"));

    // Test if we can insert at the start
    test.Insert(0, '0');
    ExpectToBeTrue(test == C3D::String("01234"));

    // Test if we can insert at the end
    test.Insert(5, '5');
    ExpectToBeTrue(test == C3D::String("012345"));

    // This should also work for heap allocated strings
    C3D::String heapTest("aaaaaaaaaaaaaaaa");
    heapTest.Insert(16, 'b');
    ExpectToBeTrue(heapTest == C3D::String("aaaaaaaaaaaaaaaab"));
    ExpectShouldBe(16, heapTest.Size());

    heapTest.Insert(5, '5');
    ExpectToBeTrue(heapTest == C3D::String("aaaaa5aaaaaaaaaaab"));
    ExpectShouldBe(17, heapTest.Size());

    return true;
}

u8 StringRemoveAt()
{
    C3D::String test("012234");

    // Test if we can remove at a random location
    test.RemoveAt(2);
    ExpectToBeTrue(test == C3D::String("01234"));

    // Test if we can remove at the start
    test.RemoveAt(0);
    ExpectToBeTrue(test == C3D::String("1234"));

    // Test if we can remove at the end
    test.RemoveAt(3);
    ExpectToBeTrue(test == C3D::String("123"));

    // Ensure we don't crash when we try to remove at index > size
    test.RemoveAt(100);

    // Ensure we don't crash when we try to RemoveAt on a string with 0 chars
    C3D::String empty;
    empty.RemoveAt(4);

    return true;
}

void String::RegisterTests(TestManager& manager)
{
    manager.StartType("String");

    REGISTER_TEST(StringShouldCreateEmptyWithEmptyCtor, "Strings should be created empty with default CTOR.");
    REGISTER_TEST(StringOperatorEqualsConstChar, "Strings should correctly allocate when operator=(const char*) is used");
    REGISTER_TEST(StringShouldDoIntegerConversion, "You should be able to create string from integers.");
    REGISTER_TEST(StringShouldDoBooleanConversion, "You should be able to create string from booleans.");
    REGISTER_TEST(StringShouldUseSso, "Strings should not dynamically allocate memory if they are small (SSO).");
    REGISTER_TEST(StringShouldCompare, "String should compare with each other and with char* .");
    REGISTER_TEST(StringShouldBeTruthy, "String should evaluate to truthy values.");
    REGISTER_TEST(StringShouldAppend, "Strings and chars should append to strings.");
    REGISTER_TEST(StringShouldTrim, "String should properly trim.");
    REGISTER_TEST(StringShouldSplit, "String should properly split into parts.");
    REGISTER_TEST(StringInsert, "String should allow chars to be inserted at arbitrary points.");
    REGISTER_TEST(StringRemoveAt, "String should allow chars to be removed at arbitrary locations.");
}