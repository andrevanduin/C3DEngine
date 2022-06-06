
#pragma once
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
		vec4 tangent;
	};

	struct Vertex2D
	{
		vec2 position;
		vec2 texture;
	};
}