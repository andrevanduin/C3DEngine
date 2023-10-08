
#include "render_view_system.h"

#include "core/engine.h"
#include "renderer/renderer_frontend.h"
#include "resources/textures/texture.h"

namespace C3D
{
    RenderViewSystem::RenderViewSystem(const SystemManager* pSystemsManager)
        : SystemWithConfig(pSystemsManager, "RENDER_VIEW_SYSTEM")
    {}

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
            Memory.Free(MemoryType::RenderView, view);
        }
        m_registeredViews.Destroy();
    }

    bool RenderViewSystem::Register(RenderView* view)
    {
        if (!view)
        {
            m_logger.Error("Register() - Failed to register since no valid view was provided.");
            return false;
        }

        const auto& name = view->GetName();

        if (name.Empty())
        {
            m_logger.Error("Register() - Failed to register since provided view has no name.");
            return false;
        }

        if (m_registeredViews.Has(name))
        {
            m_logger.Error("Register() - The View named: '{}' is already registered.", name);
            return false;
        }

        // Create our view
        if (!view->OnRegister(m_pSystemsManager))
        {
            m_logger.Error("Register() - OnRegister() call failed for view: '{}'.", name);
            view->OnDestroy();  // Destroy the view to ensure passes memory is freed
            return false;
        }

        // Regenerate the render targets for the newly created view
        view->RegenerateRenderTargets();

        // Update our hashtable
        m_registeredViews.Set(name, view);

        return true;
    }

    void RenderViewSystem::OnWindowResize(const u32 width, const u32 height) const
    {
        for (const auto view : m_registeredViews)
        {
            view->OnBaseResize(width, height);
        }
    }

    RenderView* RenderViewSystem::Get(const String& name)
    {
        if (!m_registeredViews.Has(name))
        {
            m_logger.Warn("Get() - Failed to find view named: '{}'.", name);
            return nullptr;
        }
        return m_registeredViews.Get(name);
    }

    bool RenderViewSystem::BuildPacket(RenderView* view, LinearAllocator* pFrameAllocator, void* data,
                                       RenderViewPacket* outPacket) const
    {
        if (view && outPacket)
        {
            return view->OnBuildPacket(pFrameAllocator, data, outPacket);
        }

        m_logger.Error("BuildPacket() - Requires valid view and outPacket.");
        return false;
    }

    void RenderViewSystem::DestroyPacket(RenderView* view, RenderViewPacket& packet) const
    {
        if (view)
        {
            view->OnDestroyPacket(packet);
        }
    }

    bool RenderViewSystem::OnRender(const FrameData& frameData, RenderView* view, const RenderViewPacket* packet,
                                    const u64 frameNumber, const u64 renderTargetIndex) const
    {
        if (view && packet)
        {
            return view->OnRender(frameData, packet, frameNumber, renderTargetIndex);
        }

        m_logger.Error("OnRender() - Requires a valid pointer to a view and packet.");
        return false;
    }
}  // namespace C3D
