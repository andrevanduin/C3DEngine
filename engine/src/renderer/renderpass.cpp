#include "renderpass.h"

#include "core/memory.h"
#include "services/services.h"

namespace C3D
{
	RenderPass::RenderPass()
		: id(INVALID_ID_U16), renderArea(0), renderTargetCount(0), targets(nullptr), m_clearFlags(0), m_clearColor(0)
	{}

	RenderPass::RenderPass(const RenderPassConfig& config)
		: id(INVALID_ID_U16), renderArea(config.renderArea), renderTargetCount(config.renderTargetCount), targets(nullptr), m_clearFlags(config.clearFlags), m_clearColor(config.clearColor)
	{
		targets = Memory.Allocate<RenderTarget>(config.renderTargetCount, MemoryType::Array);

		// Copy over config for each target
		for (u32 t = 0; t < renderTargetCount; t++)
		{
			auto& target = targets[t];
			target.attachmentCount = config.target.attachmentCount;
			target.attachments = Memory.Allocate<RenderTargetAttachment>(target.attachmentCount, MemoryType::Array);

			// Each attachment for the target
			for (u32 a = 0; a < target.attachmentCount; a++)
			{
				auto& attachment = target.attachments[a];
				const auto& attachmentConfig = config.target.attachments[a];

				attachment.source = attachmentConfig.source;
				attachment.type = attachmentConfig.type;
				attachment.loadOperation = attachmentConfig.loadOperation;
				attachment.storeOperation = attachmentConfig.storeOperation;
				attachment.texture = nullptr;
			}
		}
	}

	void RenderPass::Destroy()
	{
		if (targets && renderTargetCount > 0)
		{
			Memory.Free(targets, renderTargetCount * sizeof(RenderTarget), MemoryType::Array);
			targets = nullptr;
		}
	}
}
