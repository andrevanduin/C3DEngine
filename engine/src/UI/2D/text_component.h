
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    /** @brief Describes the internal data needed for a Component that has text. */
    struct TextComponent
    {
        TextComponent() { Logger::Info("TextComponent ctor called"); }

        FontHandle font;
        String text;

        DynamicArray<Vertex2D> vertices;
        DynamicArray<u32> indices;

        u32 capacity = 0;

        RenderableComponent renderable;

        bool Initialize(Component& self, FontHandle f, const String& t);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void Regenerate(Component& self);
        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D