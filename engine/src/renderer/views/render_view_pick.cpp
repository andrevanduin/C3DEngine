
#include "render_view_pick.h"

#include "core/events/event.h"
#include "math/c3d_math.h"
#include "resources/loaders/shader_loader.h"
#include "systems/resource_system.h"
#include "systems/shader_system.h"

namespace C3D
{
	RenderViewPick::RenderViewPick(const RenderViewConfig& config)
		: RenderView(ToUnderlying(RenderViewKnownType::Pick), config), m_uiShaderInfo(), m_worldShaderInfo(), m_instanceCount(0), m_mouseX(0),m_mouseY(0)
	{}

	bool RenderViewPick::OnCreate()
	{
		m_worldShaderInfo.pass = passes[0];
		m_uiShaderInfo.pass = passes[1];

		// UI Shader
		constexpr auto uiShaderName = "Shader.Builtin.UIPick";
		ShaderResource res;
		if (!Resources.Load(uiShaderName, &res))
		{
			m_logger.Error("Failed to load builtin UI Pick shader.");
			return false;
		}

		if (!Shaders.Create(m_uiShaderInfo.pass, res.config))
		{
			m_logger.Error("Failed to create builtin UI Pick Shader.");
			return false;
		}

		Resources.Unload(&res);
		m_uiShaderInfo.shader = Shaders.Get(uiShaderName);

		// Get the uniform locations
		m_uiShaderInfo.idColorLocation = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "idColor");
		m_uiShaderInfo.modelLocation = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "model");
		m_uiShaderInfo.projectionLocation = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "projection");
		m_uiShaderInfo.viewLocation = Shaders.GetUniformIndex(m_uiShaderInfo.shader, "view");

		// Default UI properties
		m_uiShaderInfo.nearClip = -100.0f;
		m_uiShaderInfo.farClip = 100.0f;
		m_uiShaderInfo.fov = 0;
		m_uiShaderInfo.projection = glm::orthoRH_NO(0.0f, 1280.0f, 720.0f, 0.0f, m_uiShaderInfo.nearClip, m_uiShaderInfo.farClip);
		m_uiShaderInfo.view = mat4(1.0f);

		// World Shader
		constexpr auto worldShaderName = "Shader.Builtin.WorldPick";
		if (!Resources.Load(worldShaderName, &res))
		{
			m_logger.Error("Failed to load builtin World Pick shader.");
			return false;
		}

		if (!Shaders.Create(m_worldShaderInfo.pass, res.config))
		{
			m_logger.Error("Failed to create builtin World Pick Shader.");
			return false;
		}

		Resources.Unload(&res);
		m_worldShaderInfo.shader = Shaders.Get(worldShaderName);

		// Get the uniform locations
		m_worldShaderInfo.idColorLocation = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "idColor");
		m_worldShaderInfo.modelLocation = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "model");
		m_worldShaderInfo.projectionLocation = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "projection");
		m_worldShaderInfo.viewLocation = Shaders.GetUniformIndex(m_worldShaderInfo.shader, "view");

		// Default world properties
		m_worldShaderInfo.nearClip = 0.1f;
		m_worldShaderInfo.farClip = 1000.0f;
		m_worldShaderInfo.fov = DegToRad(45.0f);
		m_worldShaderInfo.projection = glm::perspectiveRH_NO(m_worldShaderInfo.fov, 1280 / 720.0f, m_worldShaderInfo.nearClip, m_worldShaderInfo.farClip);
		m_worldShaderInfo.view = mat4(1.0f);

		m_instanceCount = 0;

		Memory.Zero(&m_colorTargetAttachmentTexture, sizeof(Texture));
		Memory.Zero(&m_depthTargetAttachmentTexture, sizeof(Texture));

		if (!Event.Register(SystemEventCode::MouseMoved, new EventCallback(this, &RenderViewPick::OnMouseMovedEvent)))
		{
			m_logger.Error("Unable to listen for mouse moved event.");
			return false;
		}

		return true;
	}

	void RenderViewPick::OnDestroy()
	{
		RenderView::OnDestroy();
		Event.UnRegister(SystemEventCode::MouseMoved, new EventCallback(this, &RenderViewPick::OnMouseMovedEvent));
	}

	void RenderViewPick::OnResize(u32 width, u32 height)
	{
	}

	bool RenderViewPick::OnBuildPacket(void* data, RenderViewPacket* outPacket)
	{
		return true;
	}

	bool RenderViewPick::OnRender(const RenderViewPacket* packet, u64 frameNumber, u64 renderTargetIndex) const
	{
		return true;
	}

	void RenderViewPick::GetMatrices(mat4* outView, mat4* outProjection)
	{
	}

	bool RenderViewPick::OnMouseMovedEvent(u16 code, void* sender, EventContext context)
	{
		if (code == SystemEventCode::MouseMoved)
		{
			m_mouseX = context.data.i16[0];
			m_mouseY = context.data.i16[1];
			return true;
		}

		return false;
	}
}
