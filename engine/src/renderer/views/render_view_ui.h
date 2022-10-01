
#pragma once
#include "renderer/render_view.h"

namespace C3D
{
	class RenderViewUi final : public RenderView
	{
	public:
		RenderViewUi(u16 _id, const RenderViewConfig& config);

		bool OnCreate() override;

		void OnResize(u32 width, u32 height) override;

		bool OnBuildPacket(void* data, RenderViewPacket* outPacket) const override;

		bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const override;

	private:
		u32 m_shaderId;
		f32 m_nearClip;
		f32 m_farClip;

		mat4 m_projectionMatrix;
		mat4 m_viewMatrix;
	};
}