
#pragma once
#include <algorithm>

#include "core/defines.h"
#include "math_types.h"

namespace C3D
{
    constexpr auto PI                  = 3.14159265358979323846f;
    constexpr auto PI_2                = 2.0f * PI;
    constexpr auto PI_4                = 4.0f * PI;
    constexpr auto HALF_PI             = 0.5f * PI;
    constexpr auto QUARTER_PI          = 0.25f * PI;
    constexpr auto ONE_OVER_PI         = 1.0f / PI;
    constexpr auto ONE_OVER_TWO_PI     = 1.0f / PI_2;
    constexpr auto SQRT_TWO            = 1.41421356237309504880f;
    constexpr auto SQRT_THREE          = 1.73205080756887729352f;
    constexpr auto SQRT_ONE_OVER_TWO   = 0.70710678118654752440f;
    constexpr auto SQRT_ONE_OVER_THREE = 0.57735026918962576450f;

    constexpr auto DEG_2_RAD_MULTIPLIER = PI / 180.0f;
    constexpr auto RAD_2_DEG_MULTIPLIER = 180.0f / PI;

    constexpr auto SEC_TO_MS_MULTIPLIER = 1000.0;
    constexpr auto SEC_TO_US_MULTIPLIER = 1000000.0;

    constexpr auto MS_TO_SEC_MULTIPLIER = 0.001;
    constexpr auto US_TO_SEC_MULTIPLIER = 0.000001;

    constexpr auto INF = 1e30f;

    /** @brief A vec3 pointing up. */
    constexpr auto VEC3_UP = vec3(0.0f, 1.0f, 0.0f);
    /** @brief A vec3 pointing down. */
    constexpr auto VEC3_DOWN = vec3(0.0f, -1.0f, 0.0f);
    /** @brief A vec3 pointing left. */
    constexpr auto VEC3_LEFT = vec3(-1.0f, 0.0f, 0.0f);
    /** @brief A vec3 pointing right. */
    constexpr auto VEC3_RIGHT = vec3(1.0f, 0.0f, 0.0f);
    /** @brief A vec3 pointing forward. */
    constexpr auto VEC3_FORWARD = vec3(0.0f, 0.0f, -1.0f);
    /** @brief A vec3 pointing backward. */
    constexpr auto VEC3_BACKWARD = vec3(0.0f, 0.0f, 1.0f);

    /** @brief Smallest positive f32 that is > 0 */
    constexpr auto F32_EPSILON = std::numeric_limits<f32>::epsilon();
    /** @brief Smallest possible f32 */
    constexpr auto F32_MIN = std::numeric_limits<f32>::min();
    /** @brief Largest possible f32 */
    constexpr auto F32_MAX = std::numeric_limits<f32>::max();
    /** @brief Smallest positive f64 that is > 0 */
    constexpr auto F64_EPSILON = std::numeric_limits<f64>::epsilon();
    /** @brief Smallest possible f64 */
    constexpr auto F64_MIN = std::numeric_limits<f64>::min();
    /** @brief Largest possible f64 */
    constexpr auto F64_MAX = std::numeric_limits<f64>::max();

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
    C3D_INLINE constexpr C3D_API T Cos(T x)
    {
        return std::cos(x);
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

    /**
     * @brief A method that returns the smallest number that was provided as an argument.
     *
     * @param a The first number
     * @param b The second number
     * @return The number that is smallest
     */
    template <typename T>
    C3D_API C3D_INLINE T Min(T a, T b)
    {
        return std::min(a, b);
    }

    /**
     * @brief A method that returns the smallest number that was provided as an argument.
     *
     * @param a The first number
     * @param b The second number
     * @param c The third number
     * @return The number that is smallest
     */
    template <typename T>
    C3D_API C3D_INLINE T Min(T a, T b, T c)
    {
        return std::min(std::min(a, b), c);
    }

    /**
     * @brief A method that returns the largest number that was provided as an argument.
     *
     * @param a The first number
     * @param b The second number
     * @return The number that is largest
     */
    template <typename T>
    C3D_API C3D_INLINE T Max(T a, T b)
    {
        return std::max(a, b);
    }

    /**
     * @brief A method that returns the provided value clamped in the range of [min, max]
     *
     * @param value The value you want to clamp
     * @param min The minimal value (inclusive) that you want to clamp to
     * @param max The maximum value (inclusive) that you want to clamp to
     * @return The provided value but clamped between in range [min, max]
     */
    template <typename T>
    C3D_API C3D_INLINE T Clamp(T value, T min, T max)
    {
        return std::clamp(value, min, max);
    }

    /**
     * @brief A method that returns the largest integer value less than or equal to x.
     *
     * @param x The number you want to floor
     * @return the largest integer value less than or equal to x
     */
    template <typename T>
    C3D_API C3D_INLINE T Floor(T x)
    {
        return std::floor(x);
    }

    /**
     * @brief A method that returns the base-2 logarithm of input x.
     *
     * @param x The number you want to take the base-2 log of.
     * @return How many times x can be divided by 2.
     */
    template <typename T>
    C3D_API C3D_INLINE T Log2(T x)
    {
        return std::log2(x);
    }

    /**
     * @brief A method that returns the largest number that was provided as an argument.
     *
     * @param a The first number
     * @param b The second number
     * @param c The third number
     * @return The number that is largest
     */
    template <typename T>
    C3D_API C3D_INLINE T Max(T a, T b, T c)
    {
        return std::max(std::max(a, b), c);
    }

    /** @brief Checks if the provided f32 value x is not a number. */
    C3D_API C3D_INLINE bool IsNaN(f32 x) { return std::isnan(x); }

    /** @brief Checks if the provided f64 value x is not a number. */
    C3D_API C3D_INLINE bool IsNaN(f64 x) { return std::isnan(x); }

    C3D_API C3D_INLINE bool EpsilonEqual(const f32 a, const f32 b, const f32 tolerance = F32_EPSILON)
    {
        if (Abs(a - b) > tolerance) return false;
        return true;
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const f64 a, const f64 b, const f64 tolerance = F64_EPSILON)
    {
        if (Abs(a - b) > tolerance) return false;
        return true;
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const vec2& a, const vec2& b, const f32 tolerance = F32_EPSILON)
    {
        if (Abs(a.x - b.x) > tolerance) return false;
        if (Abs(a.y - b.y) > tolerance) return false;

        return true;
    }

    C3D_API C3D_INLINE bool EpsilonEqual(const vec3& a, const vec3& b, const f32 tolerance = F32_EPSILON)
    {
        if (Abs(a.x - b.x) > tolerance) return false;
        if (Abs(a.y - b.y) > tolerance) return false;
        if (Abs(a.z - b.z) > tolerance) return false;

        return true;
    }
    C3D_API C3D_INLINE bool EpsilonEqual(const vec4& a, const vec4& b, const f32 tolerance = F32_EPSILON)
    {
        if (Abs(a.x - b.x) > tolerance) return false;
        if (Abs(a.y - b.y) > tolerance) return false;
        if (Abs(a.z - b.z) > tolerance) return false;
        if (Abs(a.w - b.w) > tolerance) return false;

        return true;
    }

    C3D_API C3D_INLINE f32 RangeConvert(const f32 value, const f32 oldMin, const f32 oldMax, const f32 newMin, const f32 newMax)
    {
        return (value - oldMin) * (newMax - newMin) / (oldMax - oldMin) + newMin;
    }

    /**
     * @brief Perform Hermite interpolation between two values.
     * See https://en.wikipedia.org/wiki/Hermite_interpolation for more information.
     *
     * @param edge0 The lower edge of the Hermite function.
     * @param edge1 The upper edgeo f the Hermite function.
     * @param x The value to interpolate.
     * @return The interpolated value.
     */
    C3D_API C3D_INLINE f32 SmoothStep(f32 edge0, f32 edge1, f32 x)
    {
        const f32 t = Clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
        return t * t * (3.0 - 2.0 * t);
    }

    /** @brief Compares x with edge. Returns 0.0f if x < edge otherwise returns 1.0f. */
    C3D_API C3D_INLINE f32 Step(const f32 edge, const f32 x) { return x < edge ? 0.0f : 1.0f; }

    /** @brief Extract the position from the provided model matrix. */
    C3D_API C3D_INLINE vec3 GetPositionFromModel(const mat4& model) { return vec3(model[3][0], model[3][1], model[3][2]); }

    /** @brief Extract the forward axis from the provided matrix. */
    C3D_API C3D_INLINE vec3 GetForward(const mat4& mat) { return normalize(vec3(-mat[0][2], -mat[1][2], -mat[2][2])); }

    /** @brief Extract the backward axis from the provided matrix. */
    C3D_API C3D_INLINE vec3 GetBackward(const mat4& mat) { return normalize(vec3(mat[0][2], mat[1][2], mat[2][2])); }

    /** @brief Extract the left axis from the provided matrix. */
    C3D_API C3D_INLINE vec3 GetLeft(const mat4& mat) { return normalize(vec3(-mat[0][0], -mat[1][0], -mat[2][0])); }

    /** @brief Extract the right axis from the provided matrix. */
    C3D_API C3D_INLINE vec3 GetRight(const mat4& mat) { return normalize(vec3(mat[0][0], mat[1][0], mat[2][0])); }

    /** @brief Extract the up axis from the provided matrix. */
    C3D_API C3D_INLINE vec3 GetUp(const mat4& mat) { return normalize(vec3(mat[0][1], mat[1][1], mat[2][1])); }

    /** @brief Extract the down axis from the provided matrix. */
    C3D_API C3D_INLINE vec3 GetDown(const mat4& mat) { return normalize(vec3(-mat[0][1], -mat[1][1], -mat[2][1])); }

    /** @brief Returns 0.0f if x == 0.0, 1.0f if x > 0.0f and -1.0f if x < 0.0f. */
    C3D_API C3D_INLINE f32 Sign(const f32 x)
    {
        if (x == 0.0f) return 0.0f;
        return x < 0.0f ? -1.0f : 1.0f;
    }

}  // namespace C3D
