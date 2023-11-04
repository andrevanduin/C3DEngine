#include "renderpass.h"

#include "memory/global_memory_system.h"
#include "systems/system_manager.h"

namespace C3D
{
    RenderPass::RenderPass(const SystemManager* pSystemsManager, const RenderPassConfig& config)
        : renderTargetCount(config.renderTargetCount),
          m_name(config.name),
          m_clearFlags(config.clearFlags),
          m_clearColor(config.clearColor),
          m_pSystemsManager(pSystemsManager)
    {
        targets = Memory.Allocate<RenderTarget>(MemoryType::Array, config.renderTargetCount);

        // Copy over config for each target
        for (u32 t = 0; t < renderTargetCount; t++)
        {
            auto& target           = targets[t];
            target.attachmentCount = static_cast<u8>(config.target.attachments.Size());
            target.attachments     = Memory.Allocate<RenderTargetAttachment>(MemoryType::Array, target.attachmentCount);

            // Each attachment for the target
            for (u32 a = 0; a < target.attachmentCount; a++)
            {
                auto& attachment             = target.attachments[a];
                const auto& attachmentConfig = config.target.attachments[a];

                attachment.source         = attachmentConfig.source;
                attachment.type           = attachmentConfig.type;
                attachment.loadOperation  = attachmentConfig.loadOperation;
                attachment.storeOperation = attachmentConfig.storeOperation;
                attachment.texture        = nullptr;
            }
        }
    }

    void RenderPass::Destroy()
    {
        if (targets && renderTargetCount > 0)
        {
            Memory.Free(targets);
            targets = nullptr;
        }
    }
}  // namespace C3D
