
#include "renderpass.h"

#include "renderer/renderer_frontend.h"
#include "resources/textures/texture.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDERPASS";

    Renderpass::Renderpass() : m_name("EMPTY"), m_pSystemsManager(nullptr) {}

    Renderpass::Renderpass(const String& name, const SystemManager* pSystemsManager) : m_name(name), m_pSystemsManager(pSystemsManager)
    {
        m_sources.Reserve(16);
        m_sinks.Reserve(16);
    }

    bool Renderpass::CreateInternals(const RenderpassConfig& config)
    {
        m_targets.Reserve(config.renderTargetCount);

        // Copy over config for each target
        for (u32 t = 0; t < config.renderTargetCount; t++)
        {
            RenderTarget target = {};
            target.attachments.Reserve(config.target.attachments.Size());

            // Each attachment for the target
            for (auto& attachmentConfig : config.target.attachments)
            {
                RenderTargetAttachment attachment = {};
                attachment.source                 = attachmentConfig.source;
                attachment.type                   = attachmentConfig.type;
                attachment.loadOperation          = attachmentConfig.loadOperation;
                attachment.storeOperation         = attachmentConfig.storeOperation;
                attachment.texture                = nullptr;

                target.attachments.PushBack(attachment);
            }

            m_targets.PushBack(target);
        }

        Renderer.CreateRenderpassInternals(config, &m_pInternalData);
        if (!m_pInternalData)
        {
            ERROR_LOG("Failed to create Renderpass.");
            return false;
        }

        return true;
    }

    void Renderpass::Begin(const FrameData& frameData) const
    {
        const auto& target = m_targets[frameData.renderTargetIndex];
        Renderer.BeginRenderpass(m_pInternalData, target);
    }

    void Renderpass::End() const { Renderer.EndRenderpass(m_pInternalData); }

    bool Renderpass::LoadResources() { return true; }

    void Renderpass::Destroy()
    {
        // Destroy every Render target that is a part of this RenderPass
        for (auto& target : m_targets)
        {
            Renderer.DestroyRenderTarget(target, true);
        }
        m_targets.Destroy();

        // Destroy the render internals
        Renderer.DestroyRenderpassInternals(m_pInternalData);
        m_pInternalData = nullptr;

        m_name.Destroy();
        m_sources.Destroy();
        m_sinks.Destroy();
    }

    bool Renderpass::RegenerateRenderTargets(const u32 width, const u32 height)
    {
        for (u32 i = 0; i < m_targets.Size(); i++)
        {
            auto& target = m_targets[i];

            // Destroy the old target
            Renderer.DestroyRenderTarget(target, false);

            for (auto& attachment : target.attachments)
            {
                if (attachment.source == RenderTargetAttachmentSource::Default)
                {
                    if (attachment.type & RenderTargetAttachmentTypeColor)
                    {
                        attachment.texture = Renderer.GetWindowAttachment(i);
                    }
                    else if (attachment.type & RenderTargetAttachmentTypeDepth || attachment.type & RenderTargetAttachmentTypeStencil)
                    {
                        attachment.texture = Renderer.GetDepthAttachment(i);
                    }
                    else
                    {
                        FATAL_LOG("Unknown attachment type: '{}'.", attachment.type);
                        return false;
                    }
                }
                else if (attachment.source == RenderTargetAttachmentSource::Self)
                {
                    if (!RegenerateAttachmentTextures(width, height))
                    {
                        ERROR_LOG("Failed to regenerate attachment textures for Renderpass: '{}'.", m_name);
                    }

                    if (!PopulateAttachment(attachment))
                    {
                        ERROR_LOG("Failed to populate attachment.");
                        return false;
                    }
                }
            }

            auto w = width;
            auto h = height;
            if (target.attachments[0].source == RenderTargetAttachmentSource::Self)
            {
                w = target.attachments[0].texture->width;
                h = target.attachments[0].texture->height;
            }

            // Create the underlying target
            Renderer.CreateRenderTarget(m_pInternalData, target, 0, w, h);
        }

        return true;
    }

    void Renderpass::AddSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin)
    {
        m_sources.EmplaceBack(name, type, origin);
    }

    void Renderpass::AddSink(const String& name) { m_sinks.EmplaceBack(name); }

    bool Renderpass::SourcesContains(const String& name) const
    {
        return std::find_if(m_sources.begin(), m_sources.end(), [&](const RendergraphSource& source) { return source.name == name; }) !=
               m_sources.end();
    }

    bool Renderpass::SinksContains(const String& name) const
    {
        return std::find_if(m_sinks.begin(), m_sinks.end(), [&](const RendergraphSink& sink) { return sink.name == name; }) !=
               m_sinks.end();
    }

    RendergraphSource* Renderpass::GetSourceByName(const String& name) const
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

    RendergraphSink* Renderpass::GetSinkByName(const String& name) const
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