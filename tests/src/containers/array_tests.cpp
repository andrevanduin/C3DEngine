
#include "array_tests.h"

#include <containers/array.h>
#include <core/defines.h>

#include "../expect.h"

u8 ArrayShouldCreate()
{
    C3D::Array<int, 10> array{};
    array[0] = 5;

    ExpectEqual(10, array.Size());
    ExpectNotEqual(nullptr, (void*)array.Data());

    ExpectEqual(5, array[0]);
    return true;
}

u8 ArrayShouldCreateWithInitializerList()
{
    C3D::Array<int, 10> array = { 1, 2, 3, 4 };

    ExpectEqual(10, array.Size());
    ExpectNotEqual(nullptr, (void*)array.Data());

    ExpectEqual(1, array[0]);
    return true;
}

u8 ArrayShouldSetAndGetValuesCorrectly()
{
    C3D::Array<int, 10> array = { 1, 2, 3, 4 };

    ExpectEqual(1, array[0]);
    array[0] = 18;
    ExpectEqual(18, array[0]);
    return true;
}

u8 ArrayShouldBeIterable()
{
    C3D::Array<int, 4> array = { 1, 2, 3, 4 };
    int expectedValues[4]    = { 1, 2, 3, 4 };

    for (auto i = 0; i < 4; i++)
    {
        ExpectEqual(expectedValues[i], array[i]);
    }
    return true;
}

u8 ArrayShouldBeIterableWithBeginAndEnd()
{
    const C3D::Array<int, 4> array = { 1, 2, 3, 4 };
    int expectedValues[4]          = { 1, 2, 3, 4 };

    auto index = 0;
    for (auto i : array)
    {
        ExpectEqual(expectedValues[index], i);
        index++;
    }
    return true;
}

void Array::RegisterTests(TestManager& manager)
{
    manager.StartType("Array");
    REGISTER_TEST(ArrayShouldCreate, "Array should properly create and hold values.");
    REGISTER_TEST(ArrayShouldCreateWithInitializerList,
                  "Array should properly create and hold values when provided with an initializer list.");
    REGISTER_TEST(ArrayShouldSetAndGetValuesCorrectly, "Array should set and get values properly.");
    REGISTER_TEST(ArrayShouldBeIterable, "Arrays should be iterable with the [] operator.");
    REGISTER_TEST(ArrayShouldBeIterableWithBeginAndEnd, "Arrays should be iterable with begin() and end() methods.");
}
