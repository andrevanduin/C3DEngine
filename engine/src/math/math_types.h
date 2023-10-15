
#pragma once
#include <fmt/format.h>

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/transform.hpp>

#include "core/defines.h"

typedef glm::vec2 vec2;
typedef glm::ivec2 ivec2;

typedef glm::vec3 vec3;
typedef glm::ivec3 ivec3;

typedef glm::vec4 vec4;
typedef glm::ivec4 ivec4;

typedef glm::mat4 mat4;

typedef glm::quat quat;

namespace C3D
{
    struct Extents2D
    {
        vec2 min = { 0, 0 };
        vec2 max = { 0, 0 };
    };

    struct Extents3D
    {
        vec3 min = { 0, 0, 0 };
        vec3 max = { 0, 0, 0 };
    };

    struct Sphere
    {
        vec3 center;
        f32 radius;
    };

    struct AABB
    {
        vec3 center;
        vec3 extents;
    };
}  // namespace C3D

template <>
struct fmt::formatter<vec2>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(vec2 const& vector, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "({}, {})", vector.x, vector.y);
    }
};

template <>
struct fmt::formatter<vec3>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(vec3 const& vector, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "({}, {}, {})", vector.x, vector.y, vector.z);
    }
};

template <>
struct fmt::formatter<vec4>
{
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(vec4 const& vector, FormatContext& ctx)
    {
        return fmt::format_to(ctx.out(), "({}, {}, {}, {})", vector.x, vector.y, vector.z, vector.w);
    }
};