
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

    enum class RendergraphSourceOrigin
    {
        Global,
        Other,
        Self,
    };

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
        String name;
        RendergraphSource* boundSource = nullptr;
    };
}  // namespace C3D