
#pragma once
#include "containers/dynamic_array.h"
#include "containers/string.h"
#include "core/defines.h"

namespace C3D
{
    struct Texture;

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