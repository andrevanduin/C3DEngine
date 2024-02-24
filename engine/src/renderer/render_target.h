#pragma once
#include "containers/dynamic_array.h"
#include "core/defines.h"

namespace C3D
{
    struct Texture;

    enum class RenderTargetAttachmentType : i8
    {
        Color   = 0x01,
        Depth   = 0x02,
        Stencil = 0x04,
    };

    static inline const char* ToString(RenderTargetAttachmentType type)
    {
        switch (type)
        {
            case RenderTargetAttachmentType::Color:
                return "Color";
            case RenderTargetAttachmentType::Depth:
                return "Depth";
            case RenderTargetAttachmentType::Stencil:
                return "Stencil";
            default:
                return "UNKOWN";
        }
    }

    enum class RenderTargetAttachmentSource : i8
    {
        Default = 0x01,
        View    = 0x02,
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
        Texture* texture;
    };

    struct RenderTarget
    {
        DynamicArray<RenderTargetAttachment> attachments;
        void* internalFrameBuffer;
    };
}  // namespace C3D