
#pragma once
#include "core/defines.h"

#include "math/math_types.h"
#include "resources/texture.h"

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
	struct ObjectUniformObject
	{
		vec4 diffuseColor;	// 16 bytes
		vec4 vec4Padding0;	// 16 bytes, reserved for future use
		vec4 vec4Padding1;	// 16 bytes, reserved for future use
		vec4 vec4Padding2;	// 16 bytes, reserved for future use
	};

	struct GeometryRenderData
	{
		u32 objectId;
		mat4 model;
		Texture* textures[16];
	};

	struct RenderPacket
	{
		f32 deltaTime;
	};

	struct RendererBackendState
	{
		u64 frameNumber;
		Texture* defaultDiffuseTexture;
	};
}
