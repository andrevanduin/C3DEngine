
#pragma once
#include "UI/2D/component.h"
#include "core/colors.h"

namespace C3D::UI_2D
{
    /** @brief Describes the internal data needed for a Component that is a quad */
    struct QuadComponent
    {
        Geometry* geometry = nullptr;
        RenderableComponent renderable;

        u16vec2 size;
        u16vec2 atlasMin;
        u16vec2 atlasMax;
        AtlasID atlasID;
        vec4 color;

        f32 offsetX = 0.0f;
        f32 offsetY = 0.0f;

        bool Initialize(Component& self, const char* name, AtlasID _atlasID, const u16vec2& _size, const vec4& _color = WHITE);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self, const u16vec2& _size);

        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D