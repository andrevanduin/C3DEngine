
#pragma once
#include "component.h"

namespace C3D::UI_2D
{
    /** @brief Describes the internal data needed for a Component that has text. */
    struct TextComponent
    {
        FontHandle font;
        String text;

        f32 maxX    = 0;
        f32 maxY    = 0;
        f32 offsetX = 0.0f;
        f32 offsetY = 0.0f;

        DynamicArray<Vertex2D> vertices;
        DynamicArray<u32> indices;

        u32 capacity = 0;

        RenderableComponent renderable;

        bool Initialize(Component& self, FontHandle f, const String& t);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void Regenerate(Component& self);
        void SetText(Component& self, const char* _text);
        void Append(Component& self, char c);
        void RemoveLast(Component& self);

        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D