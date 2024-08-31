
#pragma once
#include <string/string.h>

namespace C3D
{
    namespace Tests
    {
        static u32 TEST_OBJECT_COUNTER = 0;

        struct CountingObject
        {
            CountingObject() { TEST_OBJECT_COUNTER++; }

            ~CountingObject() { TEST_OBJECT_COUNTER--; }

            CountingObject(const CountingObject& other)
            {
                if (this != &other)
                {
                    mockStr = other.mockStr;
                    mockInt = other.mockInt;

                    TEST_OBJECT_COUNTER++;
                }
            }

            CountingObject& operator=(const CountingObject& other)
            {
                if (this != &other)
                {
                    mockStr = other.mockStr;
                    mockInt = other.mockInt;

                    TEST_OBJECT_COUNTER++;
                }
                return *this;
            }

            CountingObject(CountingObject&& other) noexcept
            {
                if (this != &other)
                {
                    mockStr = std::move(other.mockStr);
                    mockInt = other.mockInt;
                }
            }

            CountingObject& operator=(CountingObject&& other) noexcept
            {
                if (this != &other)
                {
                    mockStr = std::move(other.mockStr);
                    mockInt = other.mockInt;
                }
                return *this;
            }

            String mockStr = "MOCK_MOCK_MOCK_MOCK_MOCK";
            int mockInt    = 69;
            int* pCounter  = nullptr;
        };
    }  // namespace Tests
}  // namespace C3D