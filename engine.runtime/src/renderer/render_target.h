#pragma once
#include "containers/dynamic_array.h"
#include "defines.h"
#include "resources/textures/texture_types.h"

namespace C3D
{
    enum RenderTargetAttachmentTypeBits : u8
    {
        RenderTargetAttachmentTypeColor   = 0x01,
        RenderTargetAttachmentTypeDepth   = 0x02,
        RenderTargetAttachmentTypeStencil = 0x04,
    };

    using RenderTargetAttachmentType = u8;

    static inline const char* ToString(RenderTargetAttachmentType type)
    {
        switch (type)
        {
            case RenderTargetAttachmentTypeColor:
                return "Color";
            case RenderTargetAttachmentTypeDepth:
                return "Depth";
            case RenderTargetAttachmentTypeStencil:
                return "Stencil";
            default:
                return "UNKNOWN";
        }
    }

    enum class RenderTargetAttachmentSource : i8
    {
        Default = 0x01,
        Self    = 0x02,
    };

    enum class RenderTargetAttachmentLoadOperation : i8
    {
        DontCare = 0x0,
        Load     = 0x1,
    };

    enum class RenderTargetAttachmentStoreOperation : i8
    {
        DontCare = 0x0,
        Store    = 0x1,
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
        DynamicArray<RenderTargetAttachmentConfig> attachments;
    };

    struct RenderTargetAttachment
    {
        RenderTargetAttachmentType type;
        RenderTargetAttachmentSource source;
        RenderTargetAttachmentLoadOperation loadOperation;
        RenderTargetAttachmentStoreOperation storeOperation;
        bool presentAfter;
        TextureHandle texture;
    };

    struct RenderTarget
    {
        DynamicArray<RenderTargetAttachment> attachments;
        void* internalFrameBuffer = nullptr;
    };
}  // namespace C3D