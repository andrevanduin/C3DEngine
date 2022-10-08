
#pragma once
#include <core/defines.h>
#include <vector>
#include <random>
#include <array>

class Util
{
public:
    Util()
    {
        const auto seedData = GenerateSeedData();
        std::seed_seq seedSeq(seedData.begin(), seedData.end());
        generator.seed(seedSeq);
    }

    template <typename T>
    T GenerateRandom(const T low, const T high)
    {
        std::uniform_int_distribution distribution(low, high);
        return distribution(generator);
    }

    template <typename T>
    std::vector<T> GenerateRandom(int amount, const T low, const T high)
    {
        std::uniform_int_distribution distribution(low, high);
        std::vector<T> values(amount);
        std::generate(values.begin(), values.end(), [&] { return distribution(generator); });
        return values;
    }

    std::mt19937 generator;

private:
    [[nodiscard]] std::array<uint32_t, 8> GenerateSeedData() const
    {
        std::array<u32, 8> randomData{};
        std::random_device randomSource;
        std::generate(randomData.begin(), randomData.end(), std::ref(randomSource));
        return randomData;
    }
};