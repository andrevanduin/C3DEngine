
#include "render_view_world.h"

#include "core/events/event.h"
#include "math/c3d_math.h"
#include "renderer/renderer_frontend.h"
#include "renderer/renderer_types.h"
#include "systems/camera_system.h"
#include "systems/material_system.h"
#include "systems/shader_system.h"
#include "resources/mesh.h"
#include "resources/loaders/shader_loader.h"
#include "systems/render_view_system.h"
#include "systems/resource_system.h"

namespace C3D
{
	RenderViewWorld::RenderViewWorld(const RenderViewConfig& config) // TODO: Set from configuration
		: RenderView(ToUnderlying(RenderViewKnownType::World), config), m_shader(nullptr), m_fov(DegToRad(45.0f)), m_nearClip(0.1f), m_farClip(1000.0f),
			m_projectionMatrix(), m_camera(nullptr), m_ambientColor(), m_renderMode(0)
	{}

	bool RenderViewWorld::OnCreate()
	{
		// Builtin skybox shader
		const auto shaderName = "Shader.Builtin.Material";
		ShaderResource res;
		if (!Resources.Load(shaderName, &res))
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

		Resources.Unload(&res);

		m_shader = Shaders.Get(m_customShaderName ? m_customShaderName : shaderName);

		const auto fWidth = static_cast<f32>(m_width);
		const auto fHeight = static_cast<f32>(m_height);

		m_projectionMatrix = glm::perspectiveRH_NO(m_fov, fWidth / fHeight, m_nearClip, m_farClip);
		m_camera = Cam.GetDefault();

		// TODO: Obtain from scene
		m_ambientColor = vec4(0.25f, 0.25f, 0.25f, 1.0f);

		// Register our render mode change event listener
		if (!Event.Register(SystemEventCode::SetRenderMode, new EventCallback(this, &RenderViewWorld::OnEvent)))
		{
			m_logger.Error("OnCreate() - Failed to register render mode set event.");
			return false;
		}

		return true;
	}

	void RenderViewWorld::OnDestroy()
	{
		RenderView::OnDestroy();
		Event.UnRegister(SystemEventCode::SetRenderMode, new EventCallback(this, &RenderViewWorld::OnEvent));
	}

	void RenderViewWorld::OnResize(const u32 width, const u32 height)
	{
		if (width != m_width || height != m_height)
		{
			m_width = static_cast<u16>(width);
			m_height = static_cast<u16>(height);

			const auto aspectRatio = static_cast<f32>(width) / static_cast<f32>(height);
			m_projectionMatrix = glm::perspectiveRH_NO(m_fov, aspectRatio, m_nearClip, m_farClip);

			for (const auto pass : passes)
			{
				pass->renderArea = ivec4(0, 0, m_width, m_height);
			}
		}
	}

	bool RenderViewWorld::OnBuildPacket(void* data, RenderViewPacket* outPacket)
	{
		if (!data || !outPacket)
		{
			m_logger.Warn("OnBuildPacket() - Requires a valid pointer to data and outPacket");
			return false;
		}

		const auto meshData = static_cast<MeshPacketData*>(data);

		outPacket->view = this;
		outPacket->projectionMatrix = m_projectionMatrix;
		outPacket->viewMatrix = m_camera->GetViewMatrix();
		outPacket->viewPosition = m_camera->GetPosition();
		outPacket->ambientColor = m_ambientColor;

		for (const auto mesh : meshData->meshes)
		{
			auto model = mesh->transform.GetWorld();

			for (u16 i = 0; i < mesh->geometryCount; i++)
			{
				GeometryRenderData renderData
				{
					model,
					mesh->geometries[i],
				};

				if ((mesh->geometries[i]->material->diffuseMap.texture->flags & TextureFlag::HasTransparency) == 0)
				{
					// Material has no transparency
					outPacket->geometries.PushBack(renderData);
				}
				else
				{
					vec3 center = vec4(renderData.geometry->center, 1.0f) * model;
					const f32 distance = glm::distance(center, m_camera->GetPosition());

					GeometryDistance gDistance
					{
						renderData,
						Abs(distance)
					};
					m_distances.PushBack(gDistance);
				}
			}
		}

		std::sort(m_distances.begin(), m_distances.end(),
			[](const GeometryDistance& a, const GeometryDistance& b)
			{
				return a.distance < b.distance;
			}
		);

		for (auto& [g, distance] : m_distances)
		{
			outPacket->geometries.PushBack(g);
		}

		m_distances.Clear();
		return true;
	}

	bool RenderViewWorld::OnRender(const RenderViewPacket* packet, const u64 frameNumber, const u64 renderTargetIndex) const
	{
		for (auto& pass : passes)
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

			// TODO: Generic way to request data such as ambient color (which should come from a scene)
			if (!Materials.ApplyGlobal(shaderId, frameNumber, &packet->projectionMatrix, &packet->viewMatrix, &packet->ambientColor, &packet->viewPosition, m_renderMode))
			{
				m_logger.Error("OnRender() - Failed to apply globals for shader with id {}", shaderId);
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

	bool RenderViewWorld::OnEvent(const u16 code, void* sender, const EventContext context)
	{
		if (code == SystemEventCode::SetRenderMode)
		{
			switch (const i32 mode = context.data.i32[0])
			{
				case RendererViewMode::Default:
					m_logger.Debug("Renderer mode set to default");
					m_renderMode = RendererViewMode::Default;
					break;
				case RendererViewMode::Lighting:
					m_logger.Debug("Renderer mode set to lighting");
					m_renderMode = RendererViewMode::Lighting;
					break;
				case RendererViewMode::Normals:
					m_logger.Debug("Renderer mode set to normals");
					m_renderMode = RendererViewMode::Normals;
					break;
			}
		}

		return false;
	}
}
