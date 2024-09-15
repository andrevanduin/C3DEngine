
#pragma once
#include "UI/2D/component.h"
#include "colors.h"

namespace C3D::UI_2D
{
    /** @brief Describes the internal data needed for a Component that has a nine slice. */
    struct NineSliceComponent
    {
        Geometry* geometry = nullptr;

        u16vec2 cornerSize;
        u16vec2 atlasMin;
        u16vec2 atlasMax;
        u16vec2 newSize;

        AtlasID atlasID;
        vec4 color;

        bool isDirty = true;

        RenderableComponent renderable;

        bool Initialize(Component& self, const char* name, AtlasID _atlasID, const u16vec2& size, const u16vec2& _cornerSize,
                        const vec4& _color = WHITE);
        void OnPrepareRender(Component& self);
        void OnRender(Component& self, const FrameData& frameData, const ShaderLocations& locations);
        void OnResize(Component& self, const u16vec2& size);
        void Destroy(Component& self);
    };
}  // namespace C3D::UI_2D