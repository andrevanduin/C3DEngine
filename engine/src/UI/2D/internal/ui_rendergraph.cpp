
#include "ui_rendergraph.h"

#include "systems/UI/2D/ui2d_system.h"

namespace C3D
{
    bool UI2DRendergraph::Create(const C3D::String& name, const UI2DRendergraphConfig& config)
    {
        m_name   = name;
        m_config = config;

        // UI Pass
        if (!AddGlobalSource("COLOR_BUFFER", RendergraphSourceType::RenderTargetColor, RendergraphSourceOrigin::Global))
        {
            ERROR_LOG("Failed to add global COLOR_BUFFER.");
            return false;
        }

        if (!AddGlobalSource("DEPTH_BUFFER", RendergraphSourceType::RenderTargetDepthStencil, RendergraphSourceOrigin::Global))
        {
            ERROR_LOG("Failed to add global DEPTH_BUFFER.");
            return false;
        }

        if (!AddPass("UI", &m_uiPass))
        {
            ERROR_LOG("Failed to add UI pass.");
            return false;
        }
        if (!AddSink("UI", "COLOR_BUFFER"))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER sink to UI pass.");
            return false;
        }
        if (!AddSink("UI", "DEPTH_BUFFER"))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER sink to UI pass.");
            return false;
        }
        if (!AddSource("UI", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor, C3D::RendergraphSourceOrigin::Other))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER source to UI pass.");
            return false;
        }
        if (!AddSource("UI", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil, C3D::RendergraphSourceOrigin::Global))
        {
            ERROR_LOG("Failed to add COLOR_BUFFER source to UI pass.");
            return false;
        }
        if (!Link("COLOR_BUFFER", "UI", "COLOR_BUFFER"))
        {
            ERROR_LOG("Failed to link Editor COLOR_BUFFER source to UI COLOR_BUFFER sink.");
            return false;
        }
        if (!Link("DEPTH_BUFFER", "UI", "DEPTH_BUFFER"))
        {
            ERROR_LOG("Failed to link Global DEPTH_BUFFER source to UI DEPTH_BUFFER sink.");
            return false;
        }

        if (!Finalize(m_config.pFrameAllocator))
        {
            ERROR_LOG("Failed to finilaze UI2D rendergraph.");
            return false;
        }

        return true;
    }

    bool UI2DRendergraph::OnPrepareRender(FrameData& frameData, const Viewport& viewport)
    {
        if (!UI2D.OnPrepareRender(frameData))
        {
            ERROR_LOG("Failed to prepare UI2D components for rendering.");
        }

        m_uiPass.Prepare(viewport, UI2D.GetComponents());

        return true;
    }

}  // namespace C3D