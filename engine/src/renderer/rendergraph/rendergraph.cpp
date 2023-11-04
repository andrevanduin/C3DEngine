
#include "rendergraph.h"

#include "core/application.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDERGRAPH";

    bool Rendergraph::Create(const String& name, Application* application)
    {
        INFO_LOG("Initializing Rendergraph: '{}'.", name);

        m_pSystemsManager = application->GetSystemsManager();
        m_name            = name;
        m_application     = application;

        return true;
    }

    void Rendergraph::Destroy()
    {
        INFO_LOG("Destroying Rendergraph: '{}'.", m_name);

        m_globalSources.Destroy();
        m_passes.Destroy();
        m_name.Destroy();

        m_application = nullptr;
    }

    bool Rendergraph::AddGlobalSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin)
    {
        m_globalSources.EmplaceBack(name, type, origin);
        return true;
    }

    bool Rendergraph::AddPass(const String& name, RendergraphPass* pass) { return true; }

    bool Rendergraph::AddSource(const String& passName, const String& sourceName, RendergraphSourceType type,
                                RendergraphSourceOrigin origin)

    {}
    bool Rendergraph::AddSink(const String& passName, const String& sinkName) {}

    bool Rendergraph::Link(const String& sourcePassName, const String& sourceName, const String& sinkPassName, const String& sinkName) {}

    bool Rendergraph::Finalize() {}

    bool Rendergraph::ExecuteFrame(const FrameData& frameData) {}
}  // namespace C3D