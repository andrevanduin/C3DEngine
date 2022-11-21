#pragma once
#include "core/defines.h"

namespace C3D
{
	struct Texture;

	enum RenderTargetAttachmentType : i32
	{
		RenderTargetAttachmentTypeColor = 0x01,
		RenderTargetAttachmentTypeDepth = 0x02,
		RenderTargetAttachmentTypeStencil = 0x04,
	};

	enum RenderTargetAttachmentSource : i32
	{
		RenderTargetAttachmentSourceDefault = 0x01,
		RenderTargetAttachmentSourceView = 0x02,
	};

	enum RenderTargetAttachmentLoadOperation : i32
	{
		RenderTargetAttachmentLoadOperationDontCare = 0x0,
		RenderTargetAttachmentLoadOperationLoad = 0x1,
	};

	enum RenderTargetAttachmentStoreOperation : i32
	{
		RenderTargetAttachmentStoreOperationDontCare = 0x0,
		RenderTargetAttachmentStoreOperationStore = 0x1,
	};

	struct RenderTargetAttachmentConfig
	{
		RenderTargetAttachmentType type;
		RenderTargetAttachmentSource source;
		RenderTargetAttachmentLoadOperation loadOperation;
		RenderTargetAttachmentStoreOperation storeOperation;
		bool presentAfter;
	};

	struct RenderTargetConfig
	{
		u8 attachmentCount;
		RenderTargetAttachmentConfig* attachments;
	};

	struct RenderTargetAttachment
	{
		RenderTargetAttachmentType type;
		RenderTargetAttachmentSource source;
		RenderTargetAttachmentLoadOperation loadOperation;
		RenderTargetAttachmentStoreOperation storeOperation;
		bool presentAfter;
		Texture* texture;
	};

	struct RenderTarget
	{
		u8 attachmentCount;
		RenderTargetAttachment* attachments;

		void* internalFrameBuffer;
	};
}