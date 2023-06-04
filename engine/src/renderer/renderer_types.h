
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

	enum RendererConfigFlagBits : u8
	{
		/* Sync frame rate to monitor refresh rate. */
		FlagVSyncEnabled		= 0x1,
		/* Configure renderer to try to save power wherever possible (useful when on battery power for example). */
		FlagPowerSavingEnabled	= 0x2,
	};

	typedef u8 RendererConfigFlags;

	struct RendererBackendConfig
	{
		const char* applicationName;
		u32 applicationVersion;

		RendererConfigFlags flags;

		const Engine* engine;
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
