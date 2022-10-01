
#pragma once
#include <SDL2/SDL.h>

#include "core/defines.h"
#include "containers/dynamic_array.h"

#include "resources/geometry.h"
#include "renderpass.h"
#include "render_view.h"

namespace C3D
{
	constexpr auto BUILTIN_SHADER_NAME_MATERIAL = "Shader.Builtin.Material";
	constexpr auto BUILTIN_SHADER_NAME_UI = "Shader.Builtin.UI";

	enum class RendererBackendType
	{
		Vulkan,
		OpenGl,
		DirectX,
	};

	enum RendererViewMode : i32
	{
		Default = 0,
		Lighting = 1,
		Normals = 2,
	};

	struct RenderPacket
	{
		f32 deltaTime = 0;
		DynamicArray<RenderViewPacket> views;
	};

	struct RenderTarget
	{
		bool syncToWindowSize;

		u8 attachmentCount;
		Texture** attachments;

		void* internalFrameBuffer;
	};

	struct RendererBackendConfig
	{
		const char* applicationName;
		u32 applicationVersion;

		u16 renderPassCount;

		RenderPassConfig* passConfigs;
		RenderSystem* frontend;

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
