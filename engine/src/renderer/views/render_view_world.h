
#pragma once
#include "core/events/event_context.h"
#include "renderer/render_view.h"

namespace C3D
{
	class Camera;

	class RenderViewWorld final : public RenderView
	{
	public:
		RenderViewWorld(u16 _id, const RenderViewConfig& config);

		bool OnCreate() override;
		void OnDestroy() override;

		void OnResize(u32 width, u32 height) override;

		bool OnBuildPacket(void* data, RenderViewPacket* outPacket) const override;

		bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const override;

	private:
		bool OnEvent(u16 code, void* sender, EventContext context);

		u32 m_shaderId;

		f32 m_fov;
		f32 m_nearClip;
		f32 m_farClip;

		mat4 m_projectionMatrix;
		Camera* m_camera;

		vec4 m_ambientColor;
		u32 m_renderMode;
	};
}