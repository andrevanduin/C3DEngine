
#pragma once
#include "../core/defines.h"

namespace C3D
{
	enum class RendererBackendType
	{
		Vulkan,
		OpenGl,
		DirectX
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