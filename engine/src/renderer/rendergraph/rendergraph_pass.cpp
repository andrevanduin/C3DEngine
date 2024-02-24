
#include "rendergraph_pass.h"

#include <core/exceptions.h>
#include <renderer/renderer_frontend.h>
#include <renderer/renderpass.h>
#include <systems/system_manager.h>

namespace C3D
{
    RendergraphPass::RendergraphPass() : m_name("EMPTY"), m_pSystemsManager(nullptr) {}

    RendergraphPass::RendergraphPass(const String& name, const SystemManager* pSystemsManager)
        : m_name(name), m_pSystemsManager(pSystemsManager)
    {}

    void RendergraphPass::Destroy()
    {
        m_sources.Destroy();
        m_sinks.Destroy();

        if (m_pass)
        {
            m_pass->Destroy();
            Renderer.DestroyRenderPass(m_pass);
            m_pass = nullptr;
        }

        m_name.Destroy();
    }

    bool RendergraphPass::RegenerateRenderTargets(const u32 width, const u32 height) const
    {
        return m_pass->RegenerateRenderTargets(width, height);
    }

    void RendergraphPass::AddSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin)
    {
        m_sources.EmplaceBack(name, type, origin);
    }

    void RendergraphPass::AddSink(const String& name) { m_sinks.EmplaceBack(name); }

    bool RendergraphPass::SourcesContains(const String& name) const
    {
        return std::find_if(m_sources.begin(), m_sources.end(), [&](const RendergraphSource& source) { return source.name == name; }) !=
               m_sources.end();
    }

    bool RendergraphPass::SinksContains(const String& name) const
    {
        return std::find_if(m_sinks.begin(), m_sinks.end(), [&](const RendergraphSink& sink) { return sink.name == name; }) !=
               m_sinks.end();
    }

    RendergraphSource* RendergraphPass::GetSourceByName(const String& name) const
    {
        for (auto& source : m_sources)
        {
            if (source.name == name)
            {
                return &source;
            }
        }
        return nullptr;
    }

    RendergraphSink* RendergraphPass::GetSinkByName(const String& name) const
    {
        for (auto& sink : m_sinks)
        {
            if (sink.name == name)
            {
                return &sink;
            }
        }
        return nullptr;
    }
}  // namespace C3D