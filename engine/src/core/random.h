
#pragma once
#include <random>

#include "containers/array.h"
#include "containers/dynamic_array.h"

namespace C3D
{
    class C3D_API Random
    {
    public:
        explicit Random(Array<u32, 8> seedData = GenerateSeedData())
        {
            std::seed_seq seedSeq(seedData.begin(), seedData.end());
            m_generator.seed(seedSeq);
        }

        [[nodiscard]] int Generate(const int low, const int high)
        {
            std::uniform_int_distribution distribution(low, high);
            return distribution(m_generator);
        }

        DynamicArray<int> Generate(int amount, const int low, const int high)
        {
            std::uniform_int_distribution distribution(low, high);
            DynamicArray<int> values(amount);
            std::generate(values.begin(), values.end(), [&] { return distribution(m_generator); });
            return values;
        }

        [[nodiscard]] float Generate(const float low, const float high)
        {
            std::uniform_real_distribution distribution(low, high);
            return distribution(m_generator);
        }

        DynamicArray<float> Generate(int amount, const float low, const float high)
        {
            std::uniform_real_distribution distribution(low, high);
            DynamicArray<float> values(amount);
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
}  // namespace C3D