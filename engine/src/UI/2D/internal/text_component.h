
#pragma once
#include "UI/2D/component.h"
#include "UI/2D/config.h"

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

        vec4 color;

        DynamicArray<Vertex2D> vertices;
        DynamicArray<u32> indices;

        u32 characterCapacity = 0;
        RenderableComponent renderable;

        bool isDirty = false;

        bool Initialize(Component& self, const Config& config);

        void OnPrepareRender(Component& self);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void RecalculateGeometry(Component& self);

        void SetText(Component& self, const char* _text);

        void Insert(Component& self, u32 index, char c);
        void Insert(Component& self, u32 index, const String& text);
        void RemoveAt(Component& self, u32 index);
        void RemoveRange(Component& self, u32 characterIndexStart, u32 characterIndexEnd, bool regenerate = true);

        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D