
#pragma once
#include "renderer/camera.h"
#include "renderer/render_view.h"
#include "resources/shader.h"

namespace C3D
{
	struct PrimitiveUniformLocations
	{
		u16 projection		= INVALID_ID_U16;
		u16 view			= INVALID_ID_U16;
		u16 viewPosition	= INVALID_ID_U16;
		u16 model			= INVALID_ID_U16;
	};

	class RenderViewPrimitives final : public RenderView
	{
	public:
		explicit RenderViewPrimitives(const RenderViewConfig& config);

		bool OnCreate() override;

		void OnResize() override;

		bool OnBuildPacket(LinearAllocator& frameAllocator, void* data, RenderViewPacket* outPacket) override;

		bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) override;

	private:
		f32 m_fov;
		f32 m_nearClip;
		f32 m_farClip;

		Shader* m_shader;
		PrimitiveUniformLocations m_locations;

		mat4 m_projectionMatrix;
		Camera* m_camera;
	};
}