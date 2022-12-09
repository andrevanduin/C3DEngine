
#pragma once
#include <SDL2/SDL.h>
#undef main

#include "core/defines.h"
#include "containers/dynamic_array.h"

#include "renderpass.h"
#include "render_view.h"

namespace C3D
{
	enum class RendererBackendType
	{
		Vulkan,
		OpenGl,
		DirectX,
	};

	enum RendererViewMode : i32
	{
		Default		= 0,
		Lighting	= 1,
		Normals		= 2,
	};

	struct RenderPacket
	{
		f32 deltaTime = 0;
		DynamicArray<RenderViewPacket, LinearAllocator> views;
	};

	struct RendererBackendConfig
	{
		const char* applicationName;
		u32 applicationVersion;

		//u16 renderPassCount;
		//RenderPassConfig* passConfigs;
		//RenderSystem* frontend;

		SDL_Window* window;
	};

	struct RendererBackendState
	{
		u64 frameNumber;
	};

	enum class ShaderStage
	{
		Vertex, Geometry, Fragment, Compute
	};
}
