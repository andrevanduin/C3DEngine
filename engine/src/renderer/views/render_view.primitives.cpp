
#include "render_view_primitives.h"
#include "core/events/event.h"

#include "resources/mesh.h"
#include "renderer/renderer_frontend.h"
#include "resources/loaders/shader_loader.h"
#include "systems/system_manager.h"
#include "systems/camera_system.h"
#include "systems/shader_system.h"
#include "systems/material_system.h"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"

namespace C3D
{
	RenderViewPrimitives::RenderViewPrimitives(const RenderViewConfig& config)
		: RenderView(ToUnderlying(RenderViewKnownType::Primitives), config), m_fov(DegToRad(45.0f)),
		  m_nearClip(0.1f), m_farClip(1000.0f), m_shader(nullptr), m_projectionMatrix(1), m_camera(nullptr)
	{}

	bool RenderViewPrimitives::OnCreate()
	{
		// Builtin skybox shader
		const auto shaderName = "Shader.Builtin.Primitives";
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
		m_camera = Cam.GetDefault();

		m_locations.projection = Shaders.GetUniformIndex(m_shader, "projection");
		m_locations.view = Shaders.GetUniformIndex(m_shader, "view");
		m_locations.viewPosition = Shaders.GetUniformIndex(m_shader, "viewPosition");
		m_locations.model = Shaders.GetUniformIndex(m_shader, "model");

		const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
		m_projectionMatrix = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);

		return true;
	}

	void RenderViewPrimitives::OnResize()
	{
		const auto aspectRatio = static_cast<f32>(m_width) / static_cast<f32>(m_height);
		m_projectionMatrix = glm::perspective(m_fov, aspectRatio, m_nearClip, m_farClip);
	}

	bool RenderViewPrimitives::OnBuildPacket(LinearAllocator& frameAllocator, void* data, RenderViewPacket* outPacket)
	{
		if (!data || !outPacket)
		{
			m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
			return false;
		}

		const auto primitiveData = static_cast<PrimitivePacketData*>(data);

		outPacket->view = this;
		outPacket->projectionMatrix = m_projectionMatrix;
		outPacket->viewMatrix = m_camera->GetViewMatrix();
		outPacket->viewPosition = m_camera->GetPosition();

		for (const auto mesh : primitiveData->meshes)
		{
			for (const auto geometry : mesh->geometries)
			{
				outPacket->geometries.EmplaceBack(mesh->transform.GetWorld(), geometry, mesh->uniqueId);
			}
		}

		return true;
	}

	bool RenderViewPrimitives::OnRender(const RenderViewPacket* packet, const u64 frameNumber, const u64 renderTargetIndex)
	{
		for (const auto pass : passes)
		{
			const auto shaderId = m_shader->id;

			if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
			{
				m_logger.Error("OnRender() - BeginRenderPass failed for pass with id '{}'", pass->id);
				return false;
			}

			if (!Shaders.UseById(shaderId))
			{
				m_logger.Error("OnRender() - Failed to use shader with id {}", shaderId);
				return false;
			}

			Shaders.SetUniformByIndex(m_locations.projection, &packet->projectionMatrix);
			Shaders.SetUniformByIndex(m_locations.view, &packet->viewMatrix);
			Shaders.SetUniformByIndex(m_locations.viewPosition, &packet->viewPosition);

			for (auto& geometry : packet->geometries)
			{
				if (!Shaders.SetUniformByIndex(m_locations.model, &geometry.model))
				{
					m_logger.Error("OnRender() - Failed to set model for shader with id {}", shaderId);
				}

				Renderer.SetLineWidth(1.0f);
				Renderer.DrawGeometry(geometry);
			}

			if (!Renderer.EndRenderPass(pass))
			{
				m_logger.Error("OnRender() - EndRenderPass failed for pass with id '{}'", pass->id);
				return false;
			}
		}

		return true;
	}
}
