
#pragma once
#include "../core/defines.h"
#include "renderer_types.h"

namespace C3D
{
	class Application;

	class RendererBackend
	{
	public:
		RendererBackend() : type(), state() {}

		bool virtual Init(Application* application) = 0;
		void virtual Shutdown() = 0;

		void virtual OnResize(u16 width, u16 height) = 0;

		bool virtual BeginFrame(f32 deltaTime) = 0;

		void virtual UpdateGlobalState(mat4 projection, mat4 view, vec3 viewPosition, vec4 ambientColor, i32 mode) = 0;

		void virtual UpdateObject(mat4 model) = 0;

		bool virtual EndFrame(f32 deltaTime) = 0;

		RendererBackendType type;
		RendererBackendState state;
	};
}

