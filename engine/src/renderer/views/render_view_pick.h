
#pragma once
#include "core/defines.h"
#include "core/events/event_context.h"
#include "renderer/render_view.h"
#include "resources/shader.h"

namespace C3D
{
	struct RenderViewPickShaderInfo
	{
		Shader* shader;
		RenderPass* pass;

		u16 idColorLocation;
		u16 modelLocation;
		u16 projectionLocation;
		u16 viewLocation;

		mat4 projection;
		mat4 view;

		f32 nearClip;
		f32 farClip;
		f32 fov;
	};

	class RenderViewPick final : public RenderView
	{
	public:
		explicit RenderViewPick(const RenderViewConfig& config);

		bool OnCreate() override;
		void OnDestroy() override;

		void OnResize(u32 width, u32 height) override;

		bool OnBuildPacket(void* data, RenderViewPacket* outPacket) override;

		bool OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const override;

		void GetMatrices(mat4* outView, mat4* outProjection);

	private:
		bool OnMouseMovedEvent(u16 code, void* sender, EventContext context);

		RenderViewPickShaderInfo m_uiShaderInfo;
		RenderViewPickShaderInfo m_worldShaderInfo;

		Texture m_colorTargetAttachmentTexture;
		Texture m_depthTargetAttachmentTexture;

		i32 m_instanceCount;
		DynamicArray<bool> m_instanceUpdated;

		i16 m_mouseX, m_mouseY;
	};
}