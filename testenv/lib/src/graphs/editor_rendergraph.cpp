
#include "editor_rendergraph.h"

#include "editor/editor_gizmo.h"
#include "resources/scenes/scene.h"

bool EditorRendergraph::Create(const String& name, const EditorRendergraphConfig& config)
{
    m_name   = name;
    m_config = config;

    // Editor pass
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

    if (!AddPass("EDITOR", &m_editorPass))
    {
        ERROR_LOG("Failed to add EDITOR pass.");
        return false;
    }
    if (!AddSink("EDITOR", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER sink to Editor pass.");
        return false;
    }
    if (!AddSink("EDITOR", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER sink to Editor pass.");
        return false;
    }
    if (!AddSource("EDITOR", "COLOR_BUFFER", C3D::RendergraphSourceType::RenderTargetColor, C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add COLOR_BUFFER source to Editor pass.");
        return false;
    }
    if (!AddSource("EDITOR", "DEPTH_BUFFER", C3D::RendergraphSourceType::RenderTargetDepthStencil, C3D::RendergraphSourceOrigin::Other))
    {
        ERROR_LOG("Failed to add DEPTH_BUFFER source to Editor pass.");
        return false;
    }
    if (!Link("COLOR_BUFFER", "EDITOR", "COLOR_BUFFER"))
    {
        ERROR_LOG("Failed to link SCENE COLOR_BUFFER source to EDITOR COLOR_BUFFER sink.");
        return false;
    }
    if (!Link("DEPTH_BUFFER", "EDITOR", "DEPTH_BUFFER"))
    {
        ERROR_LOG("Failed to link SCENE DEPTH_BUFFER source to EDITOR DEPTH_BUFFER sink.");
        return false;
    }

    if (!Finalize(m_config.pFrameAllocator))
    {
        ERROR_LOG("Failed to finilize the editor rendergraph.");
        return false;
    }

    return true;
}

bool EditorRendergraph::OnPrepareRender(FrameData& frameData, const Viewport& currentViewport, Camera* currentCamera, const Scene& scene)
{
    // Only when the scene is loaded we prepare the editor pass
    if (scene.GetState() == C3D::SceneState::Loaded)
    {
        // Prepare our gizmo
        if (m_pGizmo)
        {
            m_pGizmo->OnPrepareRender(frameData);
        }

        // Prepare the editor pass
        m_editorPass.Prepare(currentViewport, currentCamera, m_pGizmo);
    }

    return true;
}

void EditorRendergraph::SetGizmo(EditorGizmo* gizmo) { m_pGizmo = gizmo; }