#include "renderpass.h"

#include "memory/global_memory_system.h"
#include "renderer/renderer_frontend.h"
#include "resources/textures/texture.h"
#include "systems/system_manager.h"

namespace C3D
{
    constexpr const char* INSTANCE_NAME = "RENDER_PASS";

    RenderPass::RenderPass(const SystemManager* pSystemsManager, const RenderPassConfig& config)
        : m_name(config.name), m_clearFlags(config.clearFlags), m_clearColor(config.clearColor), m_pSystemsManager(pSystemsManager)
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
    }

    void RenderPass::Destroy()
    {
        // Destroy every Render target that is a part of this RenderPass
        for (auto& target : m_targets)
        {
            Renderer.DestroyRenderTarget(target, true);
        }

        m_targets.Destroy();
        m_name.Destroy();
    }

    bool RenderPass::RegenerateRenderTargets(const u32 width, const u32 height)
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
                else if (attachment.source == RenderTargetAttachmentSource::View)
                {
                    // TODO: Support for self-owned attachments
                    FATAL_LOG("Self-owned attachments are not yet supported");
                    return false;
                }
            }

            // Create the underlying target
            Renderer.CreateRenderTarget(this, target, width, height);
        }

        return true;
    }
}  // namespace C3D
