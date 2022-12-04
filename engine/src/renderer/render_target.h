#pragma once
#include "core/defines.h"

namespace C3D
{
	struct Texture;

	enum class RenderTargetAttachmentType : i8
	{
		Color = 0x01,
		Depth = 0x02,
		Stencil = 0x04,
	};

	enum class RenderTargetAttachmentSource : i8
	{
		Default = 0x01,
		View = 0x02,
	};

	enum class RenderTargetAttachmentLoadOperation : i8
	{
		DontCare = 0x0,
		Load = 0x1,
	};

	enum class RenderTargetAttachmentStoreOperation : i8
	{
		DontCare = 0x0,
		Store = 0x1,
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