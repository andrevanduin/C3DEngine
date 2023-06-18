
#pragma once
#include "core/defines.h"
#include "math_types.h"

namespace C3D
{
    constexpr auto PI = 3.14159265358979323846f;
    constexpr auto PI_2 = 2.0f * PI;
    constexpr auto PI_4 = 4.0f * PI;
    constexpr auto HALF_PI = 0.5f * PI;
    constexpr auto QUARTER_PI = 0.25f * PI;
    constexpr auto ONE_OVER_PI = 1.0f / PI;
    constexpr auto ONE_OVER_TWO_PI = 1.0f / PI_2;
    constexpr auto SQRT_TWO = 1.41421356237309504880f;
    constexpr auto SQRT_THREE = 1.73205080756887729352f;
    constexpr auto SQRT_ONE_OVER_TWO = 0.70710678118654752440f;
    constexpr auto SQRT_ONE_OVER_THREE = 0.57735026918962576450f;

    constexpr auto DEG_2_RAD_MULTIPLIER = PI / 180.0f;
    constexpr auto RAD_2_DEG_MULTIPLIER = 180.0f / PI;

    constexpr auto SEC_TO_MS_MULTIPLIER = 1000.0f;

    constexpr auto MS_TO_SEC_MULTIPLIER = 0.001f;

    constexpr auto INF = 1e30f;

    // Smallest positive number where 1.0 + FLOAT_EPSILON != 0
    constexpr auto FLOAT_EPSILON = 1.192092896e-07f;
    constexpr auto FLOAT_EPSILON_F64 = 1.192092896e-07;

    C3D_INLINE bool IsPowerOf2(const u64 value) { return (value != 0) && ((value & (value - 1)) == 1); }

    C3D_INLINE constexpr f32 DegToRad(const f32 degrees) { return degrees * DEG_2_RAD_MULTIPLIER; }

    C3D_INLINE constexpr f64 DegToRad(const f64 degrees) { return degrees * static_cast<f64>(DEG_2_RAD_MULTIPLIER); }

    C3D_INLINE constexpr f32 RadToDeg(const f32 radians) { return radians * RAD_2_DEG_MULTIPLIER; }

    C3D_INLINE constexpr f64 RadToDeg(const f64 radians) { return radians * static_cast<f64>(RAD_2_DEG_MULTIPLIER); }

    template <typename T>
    C3D_INLINE constexpr C3D_API T Sin(T x)
    {
        return std::sin(x);
    }

    template <typename T>
    C3D_INLINE constexpr C3D_API T Tan(T x)
    {
        return std::tan(x);
    }

    template <typename T>
    C3D_INLINE constexpr C3D_API T ACos(T x)
    {
        return std::acos(x);
    }

    template <typename T>
    C3D_INLINE constexpr C3D_API T Sqrt(T x)
    {
        return std::sqrt(x);
    }

    template <typename T>
    C3D_INLINE constexpr C3D_API T Abs(T x)
    {
        return std::abs(x);
    }

    template <typename T>
    C3D_INLINE constexpr C3D_API T Mod(T x, T y)
    {
        return std::fmod(x, y);
    }

    template <typename T>
    C3D_API T Min(T a, T b)
    {
        return std::min(a, b);
    }

    template <typename T>
    C3D_API T Min(T a, T b, T c)
    {
        return std::min(std::min(a, b), c);
    }

    template <typename T>
    C3D_API T Max(T a, T b)
    {
        return std::max(a, b);
    }

    template <typename T>
    C3D_API T Max(T a, T b, T c)
    {
        return std::max(std::max(a, b), c);
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const f32 a, const f32 b, const f32 tolerance = FLOAT_EPSILON)
    {
        if (Abs(a - b) > tolerance) return false;
        return true;
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const f64 a, const f64 b, const f64 tolerance = FLOAT_EPSILON_F64)
    {
        if (Abs(a - b) > tolerance) return false;
        return true;
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const vec2& a, const vec2& b, const f32 tolerance = FLOAT_EPSILON)
    {
        if (Abs(a.x - b.x) > tolerance) return false;
        if (Abs(a.y - b.y) > tolerance) return false;

        return true;
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const vec3& a, const vec3& b, const f32 tolerance = FLOAT_EPSILON)
    {
        if (Abs(a.x - b.x) > tolerance) return false;
        if (Abs(a.y - b.y) > tolerance) return false;
        if (Abs(a.z - b.z) > tolerance) return false;

        return true;
    }
    C3D_API C3D_INLINE bool EpsilonEqual(const vec4& a, const vec4& b, const f32 tolerance = FLOAT_EPSILON)
    {
        if (Abs(a.x - b.x) > tolerance) return false;
        if (Abs(a.y - b.y) > tolerance) return false;
        if (Abs(a.z - b.z) > tolerance) return false;
        if (Abs(a.w - b.w) > tolerance) return false;

        return true;
    }

    C3D_API C3D_INLINE f32 RangeConvert(const f32 value, const f32 oldMin, const f32 oldMax, const f32 newMin,
                                        const f32 newMax)
    {
        return (value - oldMin) * (newMax - newMin) / (oldMax - oldMin) + newMin;
    }
}  // namespace C3D
