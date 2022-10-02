
#pragma once
#include "renderer/render_view.h"
#include "renderer/camera.h"

namespace C3D 
{
	class RenderViewSkybox final : public RenderView
	{
	public:
		RenderViewSkybox(u16 _id, const RenderViewConfig& config);

		bool OnCreate() override;

		void OnResize(u32 width, u32 height) override;

		bool OnBuildPacket(void* data, RenderViewPacket* outPacket) override;

		bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const override;

	private:
		u32 m_shaderId;

		f32 m_fov;
		f32 m_nearClip;
		f32 m_farClip;

		mat4 m_projectionMatrix;
		
		Camera* m_camera;

		u16 m_projectionLocation;
		u16 m_viewLocation;
		u16 m_cubeMapLocation;
	};
}