
#include "math/c3d_math.h"
#include "render_view_skybox.h"

#include "core/events/event.h"
#include "services/system_manager.h"

#include "systems/shader_system.h"
#include "systems/camera_system.h"
#include "renderer/renderer_frontend.h"
#include "resources/resource_types.h"
#include "resources/loaders/shader_loader.h"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"

namespace C3D 
{
	RenderViewSkybox::RenderViewSkybox(const RenderViewConfig& config)
		: RenderView(ToUnderlying(RenderViewKnownType::Skybox), config), m_shader(nullptr), m_fov(DegToRad(45.0f)), m_nearClip(0.1f), m_farClip(1000.0f),
		m_projectionMatrix(), m_camera(nullptr), m_projectionLocation(0), m_viewLocation(0), m_cubeMapLocation(0)
	{}

	bool RenderViewSkybox::OnCreate()
	{
		// Builtin skybox shader
		const auto shaderName = "Shader.Builtin.Skybox";
		ShaderResource res;
		if (!Resources.Load(shaderName, res))
		{
			m_logger.Error("OnCreate() - Failed to load ShaderResource");
			return false;
		}
		// NOTE: Since this view only has 1 pass we assume index 0
		if (!Shaders.Create(passes[0], res.config))
		{
			m_logger.Error("OnCreate() - Failed to create {}", shaderName);
			return false;
		}

		Resources.Unload(res);

		m_shader = Shaders.Get(m_customShaderName ? m_customShaderName : shaderName);
		m_projectionLocation = Shaders.GetUniformIndex(m_shader, "projection");
		m_viewLocation = Shaders.GetUniformIndex(m_shader, "view");
		m_cubeMapLocation = Shaders.GetUniformIndex(m_shader, "cubeTexture");

		const auto fWidth = static_cast<f32>(m_width);
		const auto fHeight = static_cast<f32>(m_height);

		m_projectionMatrix = glm::perspectiveRH_NO(m_fov, fWidth / fHeight, m_nearClip, m_farClip);
		m_camera = Cam.GetDefault();

		return true;
	}

	void RenderViewSkybox::OnResize()
	{
		const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
		m_projectionMatrix = glm::perspectiveRH_NO(m_fov, aspectRatio, m_nearClip, m_farClip);
	}

	bool RenderViewSkybox::OnBuildPacket(LinearAllocator& frameAllocator, void* data, RenderViewPacket* outPacket)
	{
		if (!data || !outPacket)
		{
			m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
			return false;
		}

		const auto skyboxData = static_cast<SkyboxPacketData*>(data);

		outPacket->view = this;
		outPacket->projectionMatrix = m_projectionMatrix;
		outPacket->viewMatrix = m_camera->GetViewMatrix();
		outPacket->viewPosition = m_camera->GetPosition();
		outPacket->extendedData = frameAllocator.Allocate<SkyboxPacketData>(MemoryType::RenderView);
		Platform::Copy<SkyboxPacketData>(outPacket->extendedData, data);

		return true;
	}

	bool RenderViewSkybox::OnRender(const RenderViewPacket* packet, const u64 frameNumber, const u64 renderTargetIndex)
	{
		const auto skyBoxData = static_cast<SkyboxPacketData*>(packet->extendedData);

		for (const auto pass : passes)
		{
			const auto shaderId = m_shader->id;

			if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
			{
				m_logger.Error("OnRender() - BeginRenderPass failed for pass width id '{}'", pass->id);
				return false;
			}

			if (!Shaders.UseById(shaderId))
			{
				m_logger.Error("OnRender() - Failed to use shader with id {}", shaderId);
				return false;
			}

			// Get the view matrix, but zero out the position so the skybox stays in the same spot
			auto view = m_camera->GetViewMatrix();
			view[3][0] = 0.0f;
			view[3][1] = 0.0f;
			view[3][2] = 0.0f;

			// TODO: Change this

			// Global
			Renderer.ShaderBindGlobals(Shaders.GetById(shaderId));
			if (!Shaders.SetUniformByIndex(m_projectionLocation, &packet->projectionMatrix))
			{
				m_logger.Error("Failed to apply projection matrix.");
				return false;
			}
			if (!Shaders.SetUniformByIndex(m_viewLocation, &view))
			{
				m_logger.Error("Failed to apply view position.");
				return false;
			}
			Shaders.ApplyGlobal();

			// Instance
			Shaders.BindInstance(skyBoxData->box->instanceId);
			if (!Shaders.SetUniformByIndex(m_cubeMapLocation, &skyBoxData->box->cubeMap))
			{
				m_logger.Error("Failed to apply cube map uniform.");
				return false;
			}
			bool needsUpdate = skyBoxData->box->frameNumber != frameNumber;
			Shaders.ApplyInstance(needsUpdate);

			// Sync the frame number
			skyBoxData->box->frameNumber = frameNumber;

			// Draw it
			GeometryRenderData data(skyBoxData->box->g);
			Renderer.DrawGeometry(data);

			if (!Renderer.EndRenderPass(pass))
			{
				m_logger.Error("OnRender() - EndRenderPass failed for pass with id '{}'", pass->id);
				return false;
			}
		}

		return true;
	}
}
