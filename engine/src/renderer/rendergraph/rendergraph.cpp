
#include "rendergraph.h"

#include "core/application.h"
#include "renderer/renderer_frontend.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDERGRAPH";

    Rendergraph::Rendergraph() : m_globalSink("GLOBAL_SINK") {}

    bool Rendergraph::Create(const String& name, const Application* application)
    {
        INFO_LOG("Creating Rendergraph: '{}'.", name);

        m_pSystemsManager = application->GetSystemsManager();
        m_name            = name;
        m_application     = application;

        return true;
    }

    bool Rendergraph::LoadResources()
    {
        for (auto pass : m_passes)
        {
            for (auto& source : pass->m_sources)
            {
                if (source.origin == RendergraphSourceOrigin::Self)
                {
                    u32 textureType = (source.type == RendergraphSourceType::RenderTargetColor)
                                          ? RenderTargetAttachmentTypeColor
                                          : (RenderTargetAttachmentTypeDepth | RenderTargetAttachmentTypeStencil);

                    u32 frameCount = Renderer.GetWindowAttachmentCount();
                    source.textures.Resize(frameCount);
                    for (u32 i = 0; i < frameCount; i++)
                    {
                        source.textures[i] = pass->GetAttachmentTexture(textureType, i);
                    }
                }
            }

            if (!pass->LoadResources())
            {
                return false;
            }
        }
        return true;
    }

    bool Rendergraph::OnResize(u32 width, u32 height)
    {
        for (auto pass : m_passes)
        {
            if (!pass->RegenerateRenderTargets(width, height))
            {
                ERROR_LOG("Failed to Regenerate Render Targets for: '{}'.", pass->GetName());
                return false;
            }
        }
        return true;
    }

    void Rendergraph::Destroy()
    {
        INFO_LOG("Destroying Rendergraph: '{}'.", m_name);

        m_globalSources.Destroy();

        for (auto pass : m_passes)
        {
            pass->Destroy();
        }
        m_passes.Destroy();
        m_name.Destroy();

        m_application = nullptr;
    }

    bool Rendergraph::AddGlobalSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin)
    {
        m_globalSources.EmplaceBack(name, type, origin);
        return true;
    }

    bool Rendergraph::AddPass(const String& name, Renderpass* pass)
    {
        for (const auto pass : m_passes)
        {
            if (pass->GetName() == name)
            {
                ERROR_LOG("Unable to add Pass since there is already a Pass named: '{}'.", name);
                return false;
            }
        }
        m_passes.PushBack(pass);
        return true;
    }

    bool Rendergraph::AddSource(const String& passName, const String& sourceName, RendergraphSourceType type,
                                RendergraphSourceOrigin origin)
    {
        auto pass = GetPassByName(passName);
        if (!pass) return false;

        // Ensure that the pass does not already have a source with the same name
        if (pass->SourcesContains(sourceName))
        {
            ERROR_LOG("The pass: '{}' already has a source named: '{}', so we can't add another one with the same name.", passName,
                      sourceName);
            return false;
        }

        // Add the source to the pass
        pass->AddSource(sourceName, type, origin);
        return true;
    }

    bool Rendergraph::AddSink(const String& passName, const String& sinkName)
    {
        auto pass = GetPassByName(passName);
        if (!pass) return false;

        // Ensure that the pass does not already have a source with the same name
        if (pass->SinksContains(sinkName))
        {
            ERROR_LOG("The pass: '{}' already has a sink named: '{}', so we can't add another one with the same name.", passName, sinkName);
            return false;
        }

        // Add the sink to the pass
        pass->AddSink(sinkName);
        return true;
    }

    bool Rendergraph::Link(const String& sourcePassName, const String& sourceName, const String& sinkPassName, const String& sinkName)
    {
        auto sinkPass = GetPassByName(sinkPassName);
        if (!sinkPass) return false;

        RendergraphSink* sink = sinkPass->GetSinkByName(sinkName);
        if (!sink)
        {
            ERROR_LOG("Unable to find sink named: '{}'.", sourceName);
            return false;
        }

        RendergraphSource* source = nullptr;
        if (sourcePassName.Empty())
        {
            // Global Source
            for (auto& globalSource : m_globalSources)
            {
                if (globalSource.name == sourceName)
                {
                    source = &globalSource;
                    break;
                }
            }
        }
        else
        {
            // Pass Source
            auto sourcePass = GetPassByName(sourcePassName);
            if (!sourcePass) return false;

            source = sourcePass->GetSourceByName(sourceName);
        }

        if (!source)
        {
            ERROR_LOG("Unable to find source named: '{}'.", sourceName);
            return false;
        }

        sink->boundSource = source;
        return true;
    }

    bool Rendergraph::Link(const String& sourceName, const String& sinkPassName, const String& sinkName)
    {
        return Link("", sourceName, sinkPassName, sinkName);
    }

    bool Rendergraph::SourceHasLinkedSink(RendergraphSource* source) const
    {
        for (auto pass : m_passes)
        {
            for (auto& sink : pass->m_sinks)
            {
                if (sink.boundSource == source)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool Rendergraph::Finalize(const C3D::LinearAllocator* frameAllocator)
    {
        // Get global texture references for the global sources and hook them up
        for (auto& globalSource : m_globalSources)
        {
            if (globalSource.origin == RendergraphSourceOrigin::Global)
            {
                u8 attachmentCount = Renderer.GetWindowAttachmentCount();
                globalSource.textures.Resize(attachmentCount);

                for (u8 i = 0; i < attachmentCount; i++)
                {
                    if (globalSource.type == RendergraphSourceType::RenderTargetColor)
                    {
                        globalSource.textures[i] = Renderer.GetWindowAttachment(i);
                    }
                    else if (globalSource.type == RendergraphSourceType::RenderTargetDepthStencil)
                    {
                        globalSource.textures[i] = Renderer.GetDepthAttachment(i);
                    }
                    else
                    {
                        ERROR_LOG("Unsupported source type of: '{}' in global source: '{}'.", ToString(globalSource.type),
                                  globalSource.name);
                    }
                }
            }
        }

        // Traverse the sinks and find the pass that is associated with the global color source, this will be our "first user"
        // We do this to ensure that we have something linked to the global color source since otherwise we can't ever draw something
        Renderpass* firstUser = nullptr;
        for (auto pass : m_passes)
        {
            for (auto sink : pass->m_sinks)
            {
                if (sink.boundSource && sink.boundSource->origin == RendergraphSourceOrigin::Global &&
                    sink.boundSource->type == RendergraphSourceType::RenderTargetColor)
                {
                    // We have found the first user
                    firstUser = pass;
                }
            }
        }

        if (!firstUser)
        {
            ERROR_LOG("Configuration error: No reference to a global source exists.");
            return false;
        }

        // Traverse the passes again and parse their sources to ensure that they are linked to a sink somewhere
        for (auto pass : m_passes)
        {
            for (auto& source : pass->m_sources)
            {
                if (source.type == RendergraphSourceType::RenderTargetColor)
                {
                    if (source.origin == RendergraphSourceOrigin::Other)
                    {
                        // Other means this source's origin is hooked up to the sink of another pass
                        if (!SourceHasLinkedSink(&source))
                        {
                            // This source is not linked to a sink on any pass so we assume it is linked to the final global sink
                            m_globalSink.boundSource = &source;
                            pass->m_presentsAfter    = true;
                        }
                    }
                }
                else if (source.type == RendergraphSourceType::RenderTargetDepthStencil)
                {
                    if (source.origin == RendergraphSourceOrigin::Other)
                    {
                        // Other means this source's origin is hooked up to the sink of another pass
                        if (!SourceHasLinkedSink(&source))
                        {
                            WARN_LOG("NO source found with depth stencil texture available.");
                        }
                    }
                    else if (source.origin == RendergraphSourceOrigin::Self)
                    {
                        // If the origin is self, we hook up the textures to the source
                        u32 frameCount = Renderer.GetWindowAttachmentCount();
                        source.textures.Resize(frameCount);
                        for (u32 i = 0; i < frameCount; i++)
                        {
                            source.textures[i] = pass->GetAttachmentTexture(RenderTargetAttachmentTypeDepth, i);
                        }
                    }
                }
            }
        }

        if (!m_globalSink.boundSource)
        {
            ERROR_LOG("Configuration error: No source could be linked to the global sink.");
            return false;
        }

        // Once the linking is complete, we initialize each pass and regenerate it's render targets
        for (auto pass : m_passes)
        {
            if (!pass->Initialize(frameAllocator))
            {
                ERROR_LOG("Error Initializing pass: '{}'.", pass->GetName());
                return false;
            }

            // TODO: Get default resolution here instead of hardcoded 1280x720
            if (!pass->RegenerateRenderTargets(1280, 720))
            {
                ERROR_LOG("Error Regenerating Render Targets for pass: '{}'.", pass->GetName());
                return false;
            }
        }

        return true;
    }

    bool Rendergraph::ExecuteFrame(const FrameData& frameData)
    {
        for (auto pass : m_passes)
        {
            if (!pass->m_prepared)
            {
                continue;
            }

            if (!pass->Execute(frameData))
            {
                ERROR_LOG("Failed to Execute pass: '{}'.", pass->GetName());
                return false;
            }

            // Ensure that every frame a pass gets reset to not prepared so we only render when the user actually prepares the pass
            pass->m_prepared = false;
        }

        return true;
    }

    Renderpass* Rendergraph::GetPassByName(const String& name) const
    {
        // First we check if the pass even exists
        auto it = std::find_if(m_passes.begin(), m_passes.end(), [&](const Renderpass* pass) { return pass->GetName() == name; });
        if (it == m_passes.end())
        {
            ERROR_LOG("The pass: '{}' could not be found.", name);
            return nullptr;
        }
        return *it;
    }
}  // namespace C3D