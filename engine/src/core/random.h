
#pragma once
#include <random>

#include "containers/dynamic_array.h"
#include "containers/array.h"

namespace C3D
{
	class __declspec(dllexport) Random
	{
    public:
        explicit Random(Array<u32, 8> seedData = GenerateSeedData())
        {
            std::seed_seq seedSeq(seedData.begin(), seedData.end());
            m_generator.seed(seedSeq);
        }

        template <typename T>
        [[nodiscard]] T Generate(const T low, const T high)
        {
            std::uniform_int_distribution distribution(low, high);
            return distribution(m_generator);
        }

        template <typename T>
        DynamicArray<T> Generate(int amount, const T low, const T high)
        {
            std::uniform_int_distribution distribution(low, high);
            DynamicArray<T> values(amount);
            std::generate(values.begin(), values.end(), [&] { return distribution(m_generator); });
            return values;
        }

    private:
        std::mt19937 m_generator;

        static Array<uint32_t, 8> GenerateSeedData()
        {
            Array<u32, 8> randomData{};
            std::random_device randomSource;
            std::generate(randomData.begin(), randomData.end(), std::ref(randomSource));
            return randomData;
        }
	};
}