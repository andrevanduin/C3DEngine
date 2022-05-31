
#pragma once
#include "core/defines.h"

#include "math/math_types.h"
#include "resources/geometry.h"

namespace C3D
{
	enum class RendererBackendType
	{
		Vulkan,
		OpenGl,
		DirectX
	};

	// This structure should be 256 bytes for certain Nvidia cards
	struct GlobalUniformObject
	{
		mat4 projection;	// 64 bytes
		mat4 view;			// 64 bytes

		mat4 mat4Padding0;	// 64 reserved bytes
		mat4 mat4Padding1;	// 64 reserved bytes
	};

	// This structure should be 64 bytes
	struct MaterialUniformObject
	{
		vec4 diffuseColor;	// 16 bytes
		vec4 vec4Padding0;	// 16 bytes, reserved for future use
		vec4 vec4Padding1;	// 16 bytes, reserved for future use
		vec4 vec4Padding2;	// 16 bytes, reserved for future use
	};

	struct GeometryRenderData
	{
		mat4 model;
		Geometry* geometry;
	};

	struct RenderPacket
	{
		f32 deltaTime;

		u32 geometryCount;
		GeometryRenderData* geometries;
	};

	struct RendererBackendState
	{
		u64 frameNumber;
	};
}
