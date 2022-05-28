
#pragma once
#include "core/defines.h"
#include "math/math_types.h"

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

		mat4 padding0;		// 64 reserved bytes
		mat4 padding1;		// 64 reserved bytes
	};

	struct RenderPacket
	{
		f32 deltaTime;
	};

	struct RendererBackendState
	{
		u64 frameNumber;
	};
}
