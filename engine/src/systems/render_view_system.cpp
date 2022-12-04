
#include "render_view_system.h"

#include "renderer/renderer_frontend.h"
#include "renderer/views/render_view_pick.h"

#include "renderer/views/render_view_ui.h"
#include "renderer/views/render_view_world.h"
#include "renderer/views/render_view_skybox.h"
#include "renderer/vulkan/vulkan_renderpass.h"

namespace C3D
{
	RenderViewSystem::RenderViewSystem()
		: System("RENDER_VIEW_SYSTEM")
	{
	}

	bool RenderViewSystem::Init(const RenderViewSystemConfig& config)
	{
		if (config.maxViewCount <= 2)
		{
			m_logger.Error("Init() - config.maxViewCount must be at least 2");
			return false;
		}

		m_config = config;
		m_registeredViews.Create(config.maxViewCount);

		return true;
	}

	void RenderViewSystem::Shutdown()
	{
		// Free all our views
		for (const auto view : m_registeredViews)
		{
			view->OnDestroy();
			delete view;
		}
		m_registeredViews.Destroy();
	}

	bool RenderViewSystem::Create(const RenderViewConfig& config)
	{
		if (config.passCount == 0)
		{
			m_logger.Error("Create() - Config must have at least one RenderPass.");
			return false;
		}

		if (config.name.Empty())
		{
			m_logger.Error("Create() - Config must have a valid name.");
			return false;
		}

		if (m_registeredViews.Has(config.name))
		{
			m_logger.Error("Create() - A view named '{}' already exists. A new one will not be created.", config.name);
			return false;
		}

		RenderView* view = nullptr;
		switch (config.type)
		{
			case RenderViewKnownType::World:
				view = new RenderViewWorld(config);
				break;
			case RenderViewKnownType::UI:
				view = new RenderViewUi(config);
				break;
			case RenderViewKnownType::Skybox:
				view = new RenderViewSkybox(config);
				break;
			case RenderViewKnownType::Pick:
				view = new RenderViewPick(config);
				break;
		}

		if (!view)
		{
			m_logger.Error("Create() - View was nullptr.");
			return false;
		}

		// Initialize passes for the view
		for (u8 i = 0; i < config.passCount; i++)
		{
			const auto pass = Renderer.CreateRenderPass(config.passes[i]);
			if (!pass)
			{
				m_logger.Error("Create() - RenderPass: '{}' could not be created.", config.passes[i].name);
				return false;
			}
			view->passes.PushBack(pass);
		}

		// Create our view
		if (!view->OnCreate())
		{
			m_logger.Error("Create() - view->OnCreate() failed for view: '{}'.", config.name);
			view->OnDestroy(); // Destroy the view to ensure passes memory is freed
			return false;
		}

		// Regenerate the render targets for the newly created view
		RegenerateRenderTargets(view);

		// Update our hashtable
		m_registeredViews.Set(config.name, view);
		return true;
	}

	void RenderViewSystem::OnWindowResize(const u32 width, const u32 height) const
	{
		for (const auto view : m_registeredViews)
		{
			view->OnBaseResize(width, height);
		}
	}

	RenderView* RenderViewSystem::Get(const char* name)
	{
		if (!m_registeredViews.Has(name))
		{
			m_logger.Warn("Get() - Failed to find view named: '{}'.", name);
			return nullptr;
		}
		return m_registeredViews.Get(name);
	}

	bool RenderViewSystem::BuildPacket(RenderView* view, void* data, RenderViewPacket* outPacket) const
	{
		if (view && outPacket)
		{
			return view->OnBuildPacket(data, outPacket);
		}

		m_logger.Error("BuildPacket() - Requires valid view and outPacket.");
		return false;
	}

	bool RenderViewSystem::OnRender(RenderView* view, const RenderViewPacket* packet, const u64 frameNumber, const u64 renderTargetIndex) const
	{
		if (view && packet)
		{
			return view->OnRender(packet, frameNumber, renderTargetIndex);
		}

		m_logger.Error("OnRender() - Requires a valid pointer to a view and packet.");
		return false;
	}

	void RenderViewSystem::RegenerateRenderTargets(RenderView* view) const
	{
		// TODO: More configurability
		for (u32 r = 0; r < view->passes.Size(); r++)
		{
			const auto pass = view->passes[r];

			for (u8 i = 0; i < pass->renderTargetCount; i++)
			{
				auto& target = pass->targets[i];
				// Destroy the old target if it exists
				Renderer.DestroyRenderTarget(&target, false);

				for (u32 a = 0; a < target.attachmentCount; a++)
				{
					auto& attachment = target.attachments[a];
					if (attachment.source == RenderTargetAttachmentSource::Default)
					{
						if (attachment.type == RenderTargetAttachmentType::Color)
						{
							attachment.texture = Renderer.GetWindowAttachment(i);
						}
						else if (attachment.type == RenderTargetAttachmentType::Depth)
						{
							attachment.texture = Renderer.GetDepthAttachment(i);
						}
						else
						{
							m_logger.Fatal("RegenerateRenderTargets() -  attachment type: {}", ToUnderlying(attachment.type));
						}
					}
					else if (attachment.source == RenderTargetAttachmentSource::View)
					{
						if (!view->RegenerateAttachmentTarget(r, &attachment))
						{
							m_logger.Error("RegenerateRenderTargets() - View failed to regenerate attachment target for attachment type: {}", ToUnderlying(attachment.type));
						}
					}
				}

				// Create the render target
				Renderer.CreateRenderTarget(target.attachmentCount, target.attachments, pass, target.attachments[0].texture->width, 
					target.attachments[0].texture->height, &pass->targets[i]);
			}
		}
	}
}
