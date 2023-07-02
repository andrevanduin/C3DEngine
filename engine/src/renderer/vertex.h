
#pragma once
#include "math/c3d_math.h"
#include "math/math_types.h"

namespace C3D
{
    struct Vertex3D
    {
        /* @brief: The position of the vertex. */
        vec3 position;
        /* @brief: The normal of the vertex. */
        vec3 normal;
        /* @brief: The texture coordinates (u, v). */
        vec2 texture;
        /* @brief: The color of the vertex. */
        vec4 color;
        /* @brief: The tangent of the vertex. */
        vec3 tangent;

        bool operator==(const Vertex3D& other) const
        {
            return all(epsilonEqual(position, other.position, FLOAT_EPSILON)) &&
                   all(epsilonEqual(normal, other.normal, FLOAT_EPSILON)) &&
                   all(epsilonEqual(texture, other.texture, FLOAT_EPSILON)) &&
                   all(epsilonEqual(color, other.color, FLOAT_EPSILON)) &&
                   all(epsilonEqual(tangent, other.tangent, FLOAT_EPSILON));
        }
    };

    struct Vertex2D
    {
        Vertex2D() = default;

        Vertex2D(const vec2& position, const vec2& texture) : position(position), texture(texture) {}

        vec2 position;
        vec2 texture;
    };
}  // namespace C3D

namespace std
{
    template <>
    struct hash<C3D::Vertex3D>
    {
        std::size_t operator()(const C3D::Vertex3D& vertex) const noexcept
        {
            return hash<vec3>()(vertex.position) ^ hash<vec3>()(vertex.normal) ^ hash<vec2>()(vertex.texture) ^
                   hash<vec4>()(vertex.color) ^ hash<vec3>()(vertex.tangent);
        }
    };
}  // namespace std