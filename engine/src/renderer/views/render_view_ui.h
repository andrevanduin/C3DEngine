
#pragma once
#include "core/events/event_context.h"
#include "renderer/render_view.h"
#include "resources/shader.h"

namespace C3D
{
	class RenderViewUi final : public RenderView
	{
	public:
		explicit RenderViewUi(const RenderViewConfig& config);

		bool OnCreate() override;

		void OnResize(u32 width, u32 height) override;

		bool OnBuildPacket(void* data, RenderViewPacket* outPacket) override;

		bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const override;

	private:
		f32 m_nearClip;
		f32 m_farClip;

		mat4 m_projectionMatrix;
		mat4 m_viewMatrix;

		Shader* m_shader;
		u16 m_diffuseMapLocation;
		u16 m_diffuseColorLocation;
		u16 m_modelLocation;
	};
}