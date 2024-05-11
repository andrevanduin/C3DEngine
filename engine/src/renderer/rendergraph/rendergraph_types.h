
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/colors.h"
#include "core/defines.h"
#include "renderer/render_target.h"

namespace C3D
{
    struct Texture;

    enum RenderpassClearFlag : u8
    {
        ClearNone          = 0x0,
        ClearColorBuffer   = 0x1,
        ClearDepthBuffer   = 0x2,
        ClearStencilBuffer = 0x4
    };

    struct RenderpassConfig
    {
        String name;
        f32 depth   = 0.f;
        u32 stencil = 0;

        vec4 clearColor = WHITE;

        u8 clearFlags = ClearNone;

        u8 renderTargetCount = 0;
        RenderTargetConfig target;
    };

    enum class RendergraphSourceType
    {
        RenderTargetColor,
        RenderTargetDepthStencil,
    };

    static C3D::String ToString(RendergraphSourceType type)
    {
        switch (type)
        {
            case C3D::RendergraphSourceType::RenderTargetColor:
                return "RenderTargetColor";
            case C3D::RendergraphSourceType::RenderTargetDepthStencil:
                return "RenderTargetDepthStencil";
            default:
                return "UNKOWN";
        }
    }

    enum class RendergraphSourceOrigin
    {
        Global,
        Other,
        Self,
    };

    static C3D::String ToString(RendergraphSourceOrigin origin)
    {
        switch (origin)
        {
            case C3D::RendergraphSourceOrigin::Global:
                return "Global";
            case C3D::RendergraphSourceOrigin::Other:
                return "Other";
            case C3D::RendergraphSourceOrigin::Self:
                return "Self";
            default:
                return "UNKOWN";
        }
    }

    struct RendergraphSource
    {
        RendergraphSource(const String& name, RendergraphSourceType type, RendergraphSourceOrigin origin)
            : name(name), type(type), origin(origin)
        {}

        String name;
        RendergraphSourceType type;
        RendergraphSourceOrigin origin;
        DynamicArray<Texture*> textures;
    };

    struct RendergraphSink
    {
        RendergraphSink(const String& name) : name(name) {}

        String name;
        RendergraphSource* boundSource = nullptr;
    };
}  // namespace C3D