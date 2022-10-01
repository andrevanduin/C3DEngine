
#include "render_view_system.h"

#include "renderer/renderer_frontend.h"
#include "renderer/views/render_view_ui.h"
#include "renderer/views/render_view_world.h"

namespace C3D
{
	RenderViewSystem::RenderViewSystem()
		: System("RENDER_VIEW_SYSTEM"), m_registeredViews(nullptr)
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
		m_viewLookup.Create(config.maxViewCount);
		m_viewLookup.Fill(INVALID_ID_U16);

		m_registeredViews = Memory.Allocate<RenderView*>(config.maxViewCount, MemoryType::RenderSystem);

		return true;
	}

	void RenderViewSystem::Shutdown()
	{
		// Free all our views
		for (u16 i = 0; i < m_config.maxViewCount; i++)
		{
			if (m_registeredViews[i])
			{
				m_registeredViews[i]->OnDestroy();
				delete m_registeredViews[i];
			}
		}

		// Free the array holding pointers to the views
		Memory.Free(m_registeredViews, sizeof(RenderView*) * m_config.maxViewCount, MemoryType::RenderSystem);
		m_viewLookup.Destroy();
	}

	bool RenderViewSystem::Create(const RenderViewConfig& config)
	{
		if (config.passCount == 0)
		{
			m_logger.Error("Create() - Config must have at least one RenderPass.");
			return false;
		}

		if (!config.name || StringLength(config.name) == 0)
		{
			m_logger.Error("Create() - Config must have a valid name");
			return false;
		}

		u16 id = m_viewLookup.Get(config.name);
		if (id != INVALID_ID_U16)
		{
			m_logger.Error("Create() - A view named '{}' already exists. A new one will not be created.", config.name);
			return false;
		}

		// Find a new id
		for (u16 i = 0; i < m_config.maxViewCount; i++)
		{
			if (!m_registeredViews[i])
			{
				id = i;
				break;
			}
		}

		// Check if we found a valid entry for our view
		if (id == INVALID_ID_U16)
		{
			m_logger.Error("Create() - No available space for a new view. Change system config to allow more views.");
			return false;
		}

		switch (config.type)
		{
			case RenderViewKnownType::World:
				m_registeredViews[id] = new RenderViewWorld(id, config);
				break;
			case RenderViewKnownType::UI:
				m_registeredViews[id] = new RenderViewUi(id, config);
				break;
		}

		RenderView* view = m_registeredViews[id];
		if (!view)
		{
			m_logger.Error("Create() - View was nullptr.");
			return false;
		}

		view->passes.Resize(config.passCount);
		for (u8 i = 0; i < view->passes.Size(); i++)
		{
			view->passes[i] = Renderer.GetRenderPass(config.passes[i].name);
			if (!view->passes[i])
			{
				m_logger.Fatal("Create() - RenderPass: '{}' could not be found.", config.passes[i].name);
				return false;
			}
		}

		// Create our view
		if (!view->OnCreate())
		{
			m_logger.Error("Create() - view->OnCreate() failed for view: '{}'.", config.name);
			view->OnDestroy(); // Destroy the view to ensure passes memory is freed
			return false;
		}

		// Update our hashtable
		m_viewLookup.Set(config.name, &id);
		return true;
	}

	void RenderViewSystem::OnWindowResize(const u32 width, const u32 height) const
	{
		for (u16 i = 0; i < m_config.maxViewCount; i++)
		{
			if (m_registeredViews[i] && m_registeredViews[i]->id != INVALID_ID_U16)
			{
				m_registeredViews[i]->OnResize(width, height);
			}
		}
	}

	RenderView* RenderViewSystem::Get(const char* name)
	{
		if (const u16 id = m_viewLookup.Get(name); id != INVALID_ID_U16)
		{
			return m_registeredViews[id];
		}
		m_logger.Warn("Get() - Failed to find view named: '{}'.", name);
		return nullptr;
	}

	bool RenderViewSystem::BuildPacket(const RenderView* view, void* data, RenderViewPacket* outPacket) const
	{
		if (view && outPacket)
		{
			return view->OnBuildPacket(data, outPacket);
		}

		m_logger.Error("BuildPacket() - Requires valid view and outPacket.");
		return false;
	}

	bool RenderViewSystem::OnRender(const RenderView* view, const RenderViewPacket* packet, const u64 frameNumber, const u64 renderTargetIndex) const
	{
		if (view && packet)
		{
			return view->OnRender(packet, frameNumber, renderTargetIndex);
		}

		m_logger.Error("OnRender() - Requires valid view and packet.");
		return false;
	}
}
