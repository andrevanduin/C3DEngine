
#include "array_tests.h"
#include "../expect.h"

#include <containers/array.h>
#include <core/defines.h>

u8 ArrayShouldCreate()
{
	C3D::Array<int, 10> array{};
	array[0] = 5;

	ExpectShouldBe(10, array.Size());
	ExpectShouldNotBe(nullptr, (void*)array.Data());

	ExpectShouldBe(5, array[0]);
	return true;
}

u8 ArrayShouldCreateWithInitializerList()
{
	C3D::Array<int, 10> array = { 1, 2, 3, 4 };

	ExpectShouldBe(10, array.Size());
	ExpectShouldNotBe(nullptr, (void*)array.Data());

	ExpectShouldBe(1, array[0]);
	return true;
}

u8 ArrayShouldSetAndGetValuesCorrectly()
{
	C3D::Array<int, 10> array = { 1, 2, 3, 4 };

	ExpectShouldBe(1, array[0]);
	array[0] = 18;
	ExpectShouldBe(18, array[0]);
	return true;
}

u8 ArrayShouldBeIterable()
{
	C3D::Array<int, 4> array = { 1, 2, 3, 4 };
	int expectedValues[4] = { 1, 2, 3, 4 };

	for (auto i = 0; i < 4; i++)
	{
		ExpectShouldBe(expectedValues[i], array[i]);
	}
	return true;
}

u8 ArrayShouldBeIterableWithBeginAndEnd()
{
	const C3D::Array<int, 4> array = { 1, 2, 3, 4 };
	int expectedValues[4] = { 1, 2, 3, 4 };

	auto index = 0;
	for (auto i : array)
	{
		ExpectShouldBe(expectedValues[index], i);
		index++;
	}
	return true;
}

void Array::RegisterTests(TestManager* manager)
{
	manager->Register(ArrayShouldCreate, "Array should properly create and hold values.");
	manager->Register(ArrayShouldCreateWithInitializerList, "Array should properly create and hold values when provided with an initializer list.");
	manager->Register(ArrayShouldSetAndGetValuesCorrectly, "Array should set and get values properly.");
	manager->Register(ArrayShouldBeIterable, "Arrays should be iterable with the [] operator.");
	manager->Register(ArrayShouldBeIterableWithBeginAndEnd, "Arrays should be iterable with begin() and end() methods.");
}
