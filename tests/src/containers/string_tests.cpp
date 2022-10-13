
#include "string_tests.h"
#include "../expect.h"
#include "../util.h"

#include <vector>
#include <iostream>

#include "services/services.h"
#include "core/memory.h"

u8 StringShouldCreateEmptyWithEmptyCtor()
{
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::C3DString));
	{
		const C3D::String str;

		ExpectShouldBe(0, str.Size());
		ExpectShouldBe('\0', str[0]);
	}
	ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::C3DString));
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
		ExpectShouldBe(true, std::strcmp(stdString.data(), s.Data()) == 0);
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
		C3D::String str(std::string(size, 'a').data());
		ExpectShouldBe(0, Memory.GetMemoryUsage(C3D::MemoryType::C3DString));
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
	ExpectToBeTrue(std::strcmp("Long String That we will add to another very long string to test if it also works when not using SSO", e.Data()) == 0);
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

void String::RegisterTests(TestManager* manager)
{
	manager->StartType("String");
	manager->Register(StringShouldCreateEmptyWithEmptyCtor, "Strings should be created empty with default CTOR.");
	manager->Register(StringShouldDoIntegerConversion, "You should be able to create string from integers.");
	manager->Register(StringShouldDoBooleanConversion, "You should be able to create string from booleans.");
	manager->Register(StringShouldUseSso, "Strings should not dynamically allocate memory if they are small (SSO).");
	manager->Register(StringShouldCompare, "String should compare with each other and with char* .");
	manager->Register(StringShouldBeTruthy, "String should evaluate to truthy values.");
	manager->Register(StringShouldAppend, "Strings and chars should append to strings.");
	manager->Register(StringShouldTrim, "String should properly trim.");
	manager->Register(StringShouldSplit, "String should properly split into parts.");
}