
#include "render_view_ui.h"

#include "resources/mesh.h"
#include "renderer/renderer_frontend.h"
#include "services/services.h"
#include "systems/shader_system.h"
#include "systems/material_system.h"

namespace C3D
{
	RenderViewUi::RenderViewUi(const u16 _id, const RenderViewConfig& config)
		: RenderView(_id, config), m_shaderId(INVALID_ID), m_nearClip(-100.0f), m_farClip(100.0f), m_projectionMatrix(1), m_viewMatrix(1)
	{
	}

	bool RenderViewUi::OnCreate()
	{
		m_shaderId = Shaders.GetId(m_customShaderName ? m_customShaderName : "Shader.Builtin.UI");
		// TODO: Set this from configuration
		m_nearClip = -100.0f;
		m_farClip = 100.0f;

		const auto fWidth = static_cast<f32>(m_width);
		const auto fHeight = static_cast<f32>(m_height);

		m_projectionMatrix = glm::orthoRH_NO(0.0f, fWidth, fHeight, 0.0f, m_nearClip, m_farClip);
		return true;
	}

	void RenderViewUi::OnResize(const u32 width, const u32 height)
	{
		// Only resize if the width or height changed
		if (width != m_width || height != m_height)
		{
			m_width = static_cast<u16>(width);
			m_height = static_cast<u16>(height);

			m_projectionMatrix = glm::orthoRH_NO(0.0f, static_cast<f32>(width), static_cast<f32>(height), 0.0f, m_nearClip, m_farClip);

			for (auto pass : passes)
			{
				pass->renderArea = ivec4(0, 0, width, height);
			}
		}
	}

	bool RenderViewUi::OnBuildPacket(void* data, RenderViewPacket* outPacket)
	{
		if (!data || !outPacket)
		{
			m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
			return false;
		}

		const auto meshData = static_cast<MeshPacketData*>(data);

		outPacket->view = this;
		outPacket->projectionMatrix = m_projectionMatrix;
		outPacket->viewMatrix = m_viewMatrix;

		for (const auto mesh : meshData->meshes)
		{
			for (u16 i = 0; i < mesh->geometryCount; i++)
			{
				GeometryRenderData renderData
				{
					mesh->transform.GetWorld(),
					mesh->geometries[i],
				};

				outPacket->geometries.PushBack(renderData);
			}
		}

		return true;
	}

	bool RenderViewUi::OnRender(const RenderViewPacket* packet, const u64 frameNumber, const u64 renderTargetIndex) const
	{
		for (const auto pass : passes)
		{
			if (!Renderer.BeginRenderPass(pass, &pass->targets[renderTargetIndex]))
			{
				m_logger.Error("OnRender() - BeginRenderPass failed for pass with id '{}'", pass->id);
				return false;
			}

			if (!Shaders.UseById(m_shaderId))
			{
				m_logger.Error("OnRender() - Failed to use shader with id {}", m_shaderId);
				return false;
			}

			if (!Materials.ApplyGlobal(m_shaderId, frameNumber, &packet->projectionMatrix, &packet->viewMatrix, nullptr, nullptr, 0))
			{
				m_logger.Error("OnRender() - Failed to apply globals for shader with id {}", m_shaderId);
				return false;
			}

			for (auto& geometry : packet->geometries)
			{
				Material* m = geometry.geometry->material ? geometry.geometry->material : Materials.GetDefault();

				const bool needsUpdate = m->renderFrameNumber != frameNumber;
				if (!Materials.ApplyInstance(m, needsUpdate))
				{
					m_logger.Warn("Failed to apply material '{}'. Skipping draw.", m->name);
					continue;
				}

				// Sync the frame number with the current
				m->renderFrameNumber = static_cast<u32>(frameNumber);

				Materials.ApplyLocal(m, &geometry.model);
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
